#include "SDL/include/SDL3/SDL.h"
#include "SDL/include/SDL3/SDL_properties.h"
#include "SDL/include/SDL3/SDL_video.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <unknwn.h>
#endif

#include <dxc/dxcapi.h>
#include <wrl/client.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Handle.h"
#include "RHI/Definitions.h"
#include "RHI/VulkanFactory.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{

namespace rhi = render::rhi;

struct Vertex
{
    glm::vec4 Position;
    glm::vec4 Normal;
};

struct MeshData
{
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
};

struct alignas(16) PushConstants
{
    glm::mat4 ModelViewProjection;
    glm::mat4 Model;
};

static_assert(sizeof(PushConstants) == 128, "Push constant size must match pipeline layout range");

std::filesystem::path ResolveAssetPath(const std::string& relativePath)
{
    const std::vector<std::filesystem::path> roots = {
        std::filesystem::current_path(),
        std::filesystem::current_path() / "..",
        std::filesystem::current_path() / ".." / "..",
        std::filesystem::current_path() / ".." / ".." / ".."
    };

    for (const std::filesystem::path& root : roots)
    {
        std::filesystem::path candidate = root / "Assets" / relativePath;
        if (std::filesystem::exists(candidate))
        {
            return candidate;
        }
    }

    throw std::runtime_error("Asset not found: Assets/" + relativePath);
}

std::string LoadTextFile(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open file: " + filePath.string());
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

struct VertexKey
{
    int PositionIndex = 0;
    int NormalIndex = 0;

    bool operator==(const VertexKey& other) const
    {
        return PositionIndex == other.PositionIndex && NormalIndex == other.NormalIndex;
    }
};

struct VertexKeyHasher
{
    std::size_t operator()(const VertexKey& key) const
    {
        const uint64_t a = static_cast<uint32_t>(key.PositionIndex);
        const uint64_t b = static_cast<uint32_t>(key.NormalIndex);
        return static_cast<std::size_t>((a << 32u) ^ b);
    }
};

struct FaceIndex
{
    int Position = 0;
    int Normal = 0;
};

FaceIndex ParseFaceIndex(const std::string& token)
{
    FaceIndex index{};
    const std::size_t firstSlash = token.find('/');
    if (firstSlash == std::string::npos)
    {
        index.Position = std::stoi(token);
        return index;
    }

    index.Position = std::stoi(token.substr(0, firstSlash));

    const std::size_t secondSlash = token.find('/', firstSlash + 1);
    if (secondSlash == std::string::npos)
    {
        return index;
    }

    if (secondSlash + 1 < token.size())
    {
        index.Normal = std::stoi(token.substr(secondSlash + 1));
    }

    return index;
}

MeshData LoadObjMesh(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath);
    if (!file)
    {
        throw std::runtime_error("Failed to open OBJ file: " + filePath.string());
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::unordered_map<VertexKey, uint32_t, VertexKeyHasher> vertexMap;
    MeshData mesh{};

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream parser(line);
        std::string tag;
        parser >> tag;

        if (tag == "v")
        {
            glm::vec3 position{};
            parser >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        else if (tag == "vn")
        {
            glm::vec3 normal{};
            parser >> normal.x >> normal.y >> normal.z;
            normals.push_back(glm::normalize(normal));
        }
        else if (tag == "f")
        {
            std::vector<FaceIndex> faceIndices;
            std::string token;
            while (parser >> token)
            {
                faceIndices.push_back(ParseFaceIndex(token));
            }

            auto appendVertex = [&](const FaceIndex& faceIndex) -> uint32_t
            {
                VertexKey key{};
                key.PositionIndex = faceIndex.Position;
                key.NormalIndex = faceIndex.Normal;

                auto found = vertexMap.find(key);
                if (found != vertexMap.end())
                {
                    return found->second;
                }

                if (faceIndex.Position <= 0 || faceIndex.Position > static_cast<int>(positions.size()))
                {
                    throw std::runtime_error("Invalid position index in OBJ");
                }

                glm::vec3 position = positions[static_cast<std::size_t>(faceIndex.Position - 1)];
                glm::vec3 normal = glm::normalize(position);
                if (faceIndex.Normal > 0 && faceIndex.Normal <= static_cast<int>(normals.size()))
                {
                    normal = normals[static_cast<std::size_t>(faceIndex.Normal - 1)];
                }

                Vertex vertex{};
                vertex.Position = glm::vec4(position, 1.0f);
                vertex.Normal = glm::vec4(normal, 0.0f);

                const uint32_t index = static_cast<uint32_t>(mesh.Vertices.size());
                mesh.Vertices.push_back(vertex);
                vertexMap.emplace(key, index);
                return index;
            };

            for (std::size_t i = 1; i + 1 < faceIndices.size(); ++i)
            {
                mesh.Indices.push_back(appendVertex(faceIndices[0]));
                mesh.Indices.push_back(appendVertex(faceIndices[i]));
                mesh.Indices.push_back(appendVertex(faceIndices[i + 1]));
            }
        }
    }

    if (mesh.Vertices.empty() || mesh.Indices.empty())
    {
        throw std::runtime_error("OBJ mesh has no usable geometry");
    }

    return mesh;
}

struct EdgeKey
{
    uint32_t A = 0;
    uint32_t B = 0;

    bool operator==(const EdgeKey& other) const noexcept
    {
        return A == other.A && B == other.B;
    }
};

struct EdgeKeyHasher
{
    std::size_t operator()(const EdgeKey& key) const noexcept
    {
        const uint64_t packed = (static_cast<uint64_t>(key.A) << 32u) | static_cast<uint64_t>(key.B);
        return static_cast<std::size_t>(packed);
    }
};

std::vector<uint32_t> BuildUniqueEdgeIndices(const std::vector<uint32_t>& triangleIndices)
{
    std::vector<uint32_t> edgeIndices;
    if ((triangleIndices.size() % 3u) != 0u)
    {
        return edgeIndices;
    }

    std::unordered_set<EdgeKey, EdgeKeyHasher> uniqueEdges;
    uniqueEdges.reserve(triangleIndices.size());
    edgeIndices.reserve(triangleIndices.size() * 2u);

    auto addEdge = [&](uint32_t i0, uint32_t i1)
    {
        const EdgeKey key{ std::min(i0, i1), std::max(i0, i1) };
        if (uniqueEdges.insert(key).second)
        {
            edgeIndices.push_back(i0);
            edgeIndices.push_back(i1);
        }
    };

    for (std::size_t i = 0; (i + 2u) < triangleIndices.size(); i += 3u)
    {
        const uint32_t i0 = triangleIndices[i + 0u];
        const uint32_t i1 = triangleIndices[i + 1u];
        const uint32_t i2 = triangleIndices[i + 2u];

        addEdge(i0, i1);
        addEdge(i1, i2);
        addEdge(i2, i0);
    }

    return edgeIndices;
}

std::vector<std::byte> CompileHlslToSpirv(
    const std::filesystem::path& filePath,
    const wchar_t* entryPoint,
    const wchar_t* targetProfile)
{
    ComPtr<IDxcUtils> utils;
    ComPtr<IDxcCompiler3> compiler;
    ComPtr<IDxcIncludeHandler> includeHandler;

    if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils))) ||
        FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler))))
    {
        throw std::runtime_error("Failed to create DXC interfaces");
    }

    if (FAILED(utils->CreateDefaultIncludeHandler(&includeHandler)))
    {
        throw std::runtime_error("Failed to create DXC include handler");
    }

    const std::string source = LoadTextFile(filePath);
    DxcBuffer sourceBuffer{};
    sourceBuffer.Ptr = source.data();
    sourceBuffer.Size = source.size();
    sourceBuffer.Encoding = DXC_CP_UTF8;

    const std::wstring fileName = filePath.wstring();
    std::vector<LPCWSTR> arguments = {
        fileName.c_str(),
        L"-E", entryPoint,
        L"-T", targetProfile,
        L"-spirv",
        L"-fspv-target-env=vulkan1.3",
        L"-fvk-use-dx-layout",
        L"-Zi",
        L"-Qembed_debug"
    };

    ComPtr<IDxcResult> result;
    if (FAILED(compiler->Compile(
        &sourceBuffer,
        arguments.data(),
        static_cast<uint32_t>(arguments.size()),
        includeHandler.Get(),
        IID_PPV_ARGS(&result))))
    {
        throw std::runtime_error("DXC compile invocation failed");
    }

    ComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors && errors->GetStringLength() > 0)
    {
        SDL_Log("DXC: %s", errors->GetStringPointer());
    }

    HRESULT status = S_OK;
    result->GetStatus(&status);
    if (FAILED(status))
    {
        throw std::runtime_error("HLSL compile failed: " + filePath.string());
    }

    ComPtr<IDxcBlob> spirvObject;
    if (FAILED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirvObject), nullptr)) || !spirvObject)
    {
        throw std::runtime_error("Failed to retrieve SPIR-V output");
    }

    std::vector<std::byte> bytes(spirvObject->GetBufferSize());
    std::memcpy(bytes.data(), spirvObject->GetBufferPointer(), spirvObject->GetBufferSize());
    return bytes;
}

void* GetNativeWindowHandle(SDL_Window* window)
{
    if (!window)
    {
        return nullptr;
    }

#if defined(_WIN32)
    const SDL_PropertiesID props = SDL_GetWindowProperties(window);
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#else
    (void)window;
    return nullptr;
#endif
}

} // namespace

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Render Sandbox - Rotating Icosahedron",
        1280,
        720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    if (!window)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    try
    {
        auto device = rhi::CreateVulkanDevice();
        if (!device)
        {
            throw std::runtime_error("Failed to create Vulkan RHI device");
        }

        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);

        rhi::RSwapchainDescriptor swapchainDesc{};
        swapchainDesc.Width = static_cast<uint32_t>(width);
        swapchainDesc.Height = static_cast<uint32_t>(height);
        swapchainDesc.Format = rhi::EFormat::BGRA8_UNorm;
        swapchainDesc.BufferCount = 3;
        swapchainDesc.VSync = true;
        swapchainDesc.NativeWindowHandle = GetNativeWindowHandle(window);

        Handle<rhi::RSwapchain> swapchain(device->createSwapchain(swapchainDesc));
        if (!swapchain.valid())
        {
            throw std::runtime_error("Failed to create swapchain");
        }

        Handle<rhi::RCommandList> commandList(device->createCommandList(rhi::ECommandQueueType::Graphics));
        if (!commandList.valid())
        {
            throw std::runtime_error("Failed to create command list");
        }

        const MeshData mesh = LoadObjMesh(ResolveAssetPath("Models/icosahedron.obj"));
        const std::vector<uint32_t> edgeIndices = BuildUniqueEdgeIndices(mesh.Indices);
        if (edgeIndices.empty())
        {
            throw std::runtime_error("Failed to build edge index list");
        }

        rhi::RBufferDescriptor vertexBufferDesc{};
        vertexBufferDesc.Usage = rhi::EBufferUsage::Vertex;
        vertexBufferDesc.Size = static_cast<uint32_t>(mesh.Vertices.size() * sizeof(Vertex));
        vertexBufferDesc.IsCpuVisible = true;
        vertexBufferDesc.InitialData = mesh.Vertices.data();
        vertexBufferDesc.InitialDataSize = vertexBufferDesc.Size;
        vertexBufferDesc.Name = "IcosahedronVertices";

        Handle<rhi::RBuffer> vertexBuffer(device->createBuffer(vertexBufferDesc));
        if (!vertexBuffer.valid() || !vertexBuffer->isValid())
        {
            throw std::runtime_error("Failed to create vertex buffer");
        }

        rhi::RBufferDescriptor indexBufferDesc{};
        indexBufferDesc.Usage = rhi::EBufferUsage::Index;
        indexBufferDesc.Size = static_cast<uint32_t>(mesh.Indices.size() * sizeof(uint32_t));
        indexBufferDesc.IsCpuVisible = true;
        indexBufferDesc.InitialData = mesh.Indices.data();
        indexBufferDesc.InitialDataSize = indexBufferDesc.Size;
        indexBufferDesc.Name = "IcosahedronIndices";

        Handle<rhi::RBuffer> indexBuffer(device->createBuffer(indexBufferDesc));
        if (!indexBuffer.valid() || !indexBuffer->isValid())
        {
            throw std::runtime_error("Failed to create index buffer");
        }

        rhi::RBufferDescriptor edgeIndexBufferDesc{};
        edgeIndexBufferDesc.Usage = rhi::EBufferUsage::Index;
        edgeIndexBufferDesc.Size = static_cast<uint32_t>(edgeIndices.size() * sizeof(uint32_t));
        edgeIndexBufferDesc.IsCpuVisible = true;
        edgeIndexBufferDesc.InitialData = edgeIndices.data();
        edgeIndexBufferDesc.InitialDataSize = edgeIndexBufferDesc.Size;
        edgeIndexBufferDesc.Name = "IcosahedronEdges";

        Handle<rhi::RBuffer> edgeIndexBuffer(device->createBuffer(edgeIndexBufferDesc));
        if (!edgeIndexBuffer.valid() || !edgeIndexBuffer->isValid())
        {
            throw std::runtime_error("Failed to create edge index buffer");
        }

        const std::vector<std::byte> vertexSpirv = CompileHlslToSpirv(
            ResolveAssetPath("Shaders/icosahedron.vert.hlsl"),
            L"main",
            L"vs_6_7");

        const std::vector<std::byte> fragmentSpirv = CompileHlslToSpirv(
            ResolveAssetPath("Shaders/icosahedron.frag.hlsl"),
            L"main",
            L"ps_6_7");

        const std::vector<std::byte> edgeVertexSpirv = CompileHlslToSpirv(
            ResolveAssetPath("Shaders/icosahedron.edge.vert.hlsl"),
            L"main",
            L"vs_6_7");

        const std::vector<std::byte> edgeFragmentSpirv = CompileHlslToSpirv(
            ResolveAssetPath("Shaders/icosahedron.edge.frag.hlsl"),
            L"main",
            L"ps_6_7");

        rhi::RShaderDescriptor vertexShaderDesc{};
        vertexShaderDesc.Stage = rhi::EShaderStage::Vertex;
        vertexShaderDesc.ByteCodes = vertexSpirv;
        vertexShaderDesc.EntryPoint = "main";
        vertexShaderDesc.Name = "IcosahedronVS";

        Handle<rhi::RShader> vertexShader(device->createShader(vertexShaderDesc));
        if (!vertexShader.valid() || !vertexShader->isValid())
        {
            throw std::runtime_error("Failed to create vertex shader");
        }

        rhi::RShaderDescriptor pixelShaderDesc{};
        pixelShaderDesc.Stage = rhi::EShaderStage::Pixel;
        pixelShaderDesc.ByteCodes = fragmentSpirv;
        pixelShaderDesc.EntryPoint = "main";
        pixelShaderDesc.Name = "IcosahedronPS";

        Handle<rhi::RShader> pixelShader(device->createShader(pixelShaderDesc));
        if (!pixelShader.valid() || !pixelShader->isValid())
        {
            throw std::runtime_error("Failed to create pixel shader");
        }

        rhi::RShaderDescriptor edgeVertexShaderDesc{};
        edgeVertexShaderDesc.Stage = rhi::EShaderStage::Vertex;
        edgeVertexShaderDesc.ByteCodes = edgeVertexSpirv;
        edgeVertexShaderDesc.EntryPoint = "main";
        edgeVertexShaderDesc.Name = "IcosahedronEdgeVS";

        Handle<rhi::RShader> edgeVertexShader(device->createShader(edgeVertexShaderDesc));
        if (!edgeVertexShader.valid() || !edgeVertexShader->isValid())
        {
            throw std::runtime_error("Failed to create edge vertex shader");
        }

        rhi::RShaderDescriptor edgePixelShaderDesc{};
        edgePixelShaderDesc.Stage = rhi::EShaderStage::Pixel;
        edgePixelShaderDesc.ByteCodes = edgeFragmentSpirv;
        edgePixelShaderDesc.EntryPoint = "main";
        edgePixelShaderDesc.Name = "IcosahedronEdgePS";

        Handle<rhi::RShader> edgePixelShader(device->createShader(edgePixelShaderDesc));
        if (!edgePixelShader.valid() || !edgePixelShader->isValid())
        {
            throw std::runtime_error("Failed to create edge pixel shader");
        }

        rhi::RGraphicsPipelineDescriptor pipelineDesc{};
        pipelineDesc.VertexShader = vertexShader.get();
        pipelineDesc.PixelShader = pixelShader.get();
        pipelineDesc.VertexInputLayout.Bindings.push_back({
            0,
            static_cast<uint32_t>(sizeof(Vertex)),
            0,
            rhi::RVertexBindingDescriptor::EVertexInputRate::Vertex
        });
        pipelineDesc.VertexInputLayout.Attributes.push_back({
            0,
            0,
            rhi::EFormat::RGBA32_Float,
            static_cast<uint32_t>(offsetof(Vertex, Position))
        });
        pipelineDesc.VertexInputLayout.Attributes.push_back({
            1,
            0,
            rhi::EFormat::RGBA32_Float,
            static_cast<uint32_t>(offsetof(Vertex, Normal))
        });
        pipelineDesc.RenderTargetFormats[0] = swapchain->getFormat();
        pipelineDesc.RenderTargetCount = 1;
        pipelineDesc.DepthStencilFormat = rhi::EFormat::D32_Float;
        pipelineDesc.RasterizerState.CullMode = rhi::ECullMode::None;
        pipelineDesc.RasterizerState.FrontCounterClockwise = true;
        pipelineDesc.DepthStencilState.DepthTestEnable = true;
        pipelineDesc.DepthStencilState.DepthWriteEnable = true;
        pipelineDesc.DepthStencilState.DepthFunc = rhi::ECompareOp::Less;
        pipelineDesc.PrimitiveTopology = rhi::EPrimitiveTopology::TriangleList;
        pipelineDesc.SampleCount = 1;

        Handle<rhi::RPipelineState> pipeline(device->createGraphicsPipeline(pipelineDesc));
        if (!pipeline.valid() || !pipeline->isValid())
        {
            throw std::runtime_error("Failed to create graphics pipeline");
        }

        rhi::RGraphicsPipelineDescriptor edgePipelineDesc{};
        edgePipelineDesc.VertexShader = edgeVertexShader.get();
        edgePipelineDesc.PixelShader = edgePixelShader.get();
        edgePipelineDesc.VertexInputLayout = pipelineDesc.VertexInputLayout;
        edgePipelineDesc.RenderTargetFormats[0] = swapchain->getFormat();
        edgePipelineDesc.RenderTargetCount = 1;
        edgePipelineDesc.DepthStencilFormat = rhi::EFormat::D32_Float;
        edgePipelineDesc.RasterizerState.CullMode = rhi::ECullMode::None;
        edgePipelineDesc.DepthStencilState.DepthTestEnable = false;
        edgePipelineDesc.DepthStencilState.DepthWriteEnable = false;
        edgePipelineDesc.PrimitiveTopology = rhi::EPrimitiveTopology::LineList;
        edgePipelineDesc.SampleCount = 1;

        Handle<rhi::RPipelineState> edgePipeline(device->createGraphicsPipeline(edgePipelineDesc));
        if (!edgePipeline.valid() || !edgePipeline->isValid())
        {
            throw std::runtime_error("Failed to create edge pipeline");
        }

        Handle<rhi::RTexture> depthTexture;
        rhi::RTextureView* depthView = nullptr;
        bool depthInitialized = false;
        std::vector<uint8_t> swapchainImageInitialized(swapchain->getTextureCount(), 0);

        auto rebuildDepthTarget = [&]()
        {
            rhi::RTextureDescriptor depthDesc{};
            depthDesc.Usage = rhi::ETextureUsage::DepthStencil | rhi::ETextureUsage::Target;
            depthDesc.Format = rhi::EFormat::D32_Float;
            depthDesc.Width = swapchain->getWidth();
            depthDesc.Height = swapchain->getHeight();
            depthDesc.Depth = 1;
            depthDesc.MipLevels = 1;
            depthDesc.ArrayLayers = 1;
            depthDesc.SampleCount = rhi::ESampleCount::Count1;
            depthDesc.Name = "DepthTexture";

            depthTexture = Handle<rhi::RTexture>(device->createTexture(depthDesc));
            if (!depthTexture.valid() || !depthTexture->isValid())
            {
                throw std::runtime_error("Failed to create depth texture");
            }

            rhi::RTextureViewDescriptor depthViewDesc{};
            depthViewDesc.Type = rhi::RTextureViewDescriptor::EViewType::DSV;
            depthViewDesc.Format = rhi::EFormat::D32_Float;
            depthView = depthTexture->createView(depthViewDesc);
            if (!depthView || !depthView->isValid())
            {
                throw std::runtime_error("Failed to create depth texture view");
            }

            depthInitialized = false;
        };

        rebuildDepthTarget();

        bool running = true;
        SDL_Event event{};
        const uint64_t startTicks = SDL_GetTicks();

        while (running)
        {
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                {
                    int newWidth = 0;
                    int newHeight = 0;
                    SDL_GetWindowSizeInPixels(window, &newWidth, &newHeight);
                    if (newWidth > 0 && newHeight > 0)
                    {
                        swapchain->resize(static_cast<uint32_t>(newWidth), static_cast<uint32_t>(newHeight));
                        swapchainImageInitialized.assign(swapchain->getTextureCount(), 0);
                        rebuildDepthTarget();
                    }
                    break;
                }
                default:
                    break;
                }
            }

            if (swapchain->getWidth() == 0 || swapchain->getHeight() == 0)
            {
                SDL_Delay(16);
                continue;
            }

            rhi::RTexture* backBuffer = swapchain->acquireNextTexture();
            if (!backBuffer || !backBuffer->isValid())
            {
                continue;
            }

            uint32_t currentBackBufferIndex = swapchain->getCurrentTextureIndex();
            if (currentBackBufferIndex >= swapchainImageInitialized.size())
            {
                swapchainImageInitialized.assign(swapchain->getTextureCount(), 0);
                if (currentBackBufferIndex >= swapchainImageInitialized.size())
                {
                    continue;
                }
            }

            rhi::RTextureViewDescriptor colorViewDesc{};
            colorViewDesc.Type = rhi::RTextureViewDescriptor::EViewType::RTV;
            colorViewDesc.Format = swapchain->getFormat();
            rhi::RTextureView* colorView = backBuffer->createView(colorViewDesc);
            if (!colorView || !colorView->isValid() || !depthView)
            {
                continue;
            }

            commandList->begin();

            rhi::RResourceBarrier colorToTarget{};
            colorToTarget.Texture = backBuffer;
            colorToTarget.Before = swapchainImageInitialized[currentBackBufferIndex] != 0
                ? rhi::EResourceState::Present
                : rhi::EResourceState::Undefined;
            colorToTarget.After = rhi::EResourceState::Target;
            commandList->resourceBarrier(colorToTarget);
            swapchainImageInitialized[currentBackBufferIndex] = 1;

            rhi::RResourceBarrier depthToWritable{};
            depthToWritable.Texture = depthTexture.get();
            depthToWritable.Before = depthInitialized
                ? rhi::EResourceState::DepthWrite
                : rhi::EResourceState::Undefined;
            depthToWritable.After = rhi::EResourceState::DepthWrite;
            commandList->resourceBarrier(depthToWritable);
            depthInitialized = true;

            rhi::RRenderTargetAttachment colorAttachment{};
            colorAttachment.TextureView = colorView;
            colorAttachment.LoadOp = rhi::ELoadOp::Clear;
            colorAttachment.StoreOp = rhi::EStoreOp::Store;
            colorAttachment.ClearValue.Color = { 0.07f, 0.09f, 0.13f, 1.0f };

            rhi::RDepthStencilAttachment depthAttachment{};
            depthAttachment.TextureView = depthView;
            depthAttachment.DepthLoadOp = rhi::ELoadOp::Clear;
            depthAttachment.DepthStoreOp = rhi::EStoreOp::Store;
            depthAttachment.StencilLoadOp = rhi::ELoadOp::DontCare;
            depthAttachment.StencilStoreOp = rhi::EStoreOp::DontCare;
            depthAttachment.ClearValue.Depth = 1.0f;

            rhi::RRenderPassDescriptor passDesc{};
            passDesc.ColorAttachmentCount = 1;
            passDesc.ColorAttachments[0] = colorAttachment;
            passDesc.DepthStencilAttachment = &depthAttachment;

            commandList->beginRenderPass(passDesc);
            commandList->setGraphicsPipeline(pipeline.get());
            commandList->setVertexBuffer(0, vertexBuffer.get());
            commandList->setIndexBuffer(indexBuffer.get(), rhi::EIndexFormat::UInt32);

            rhi::RViewport viewport{};
            viewport.X = 0.0f;
            viewport.Y = 0.0f;
            viewport.Width = static_cast<float>(swapchain->getWidth());
            viewport.Height = static_cast<float>(swapchain->getHeight());
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            commandList->setViewport(viewport);

            rhi::RRect scissor{};
            scissor.X = 0;
            scissor.Y = 0;
            scissor.Width = swapchain->getWidth();
            scissor.Height = swapchain->getHeight();
            commandList->setScissorRect(scissor);

            const float seconds = static_cast<float>(SDL_GetTicks() - startTicks) * 0.001f;
            const glm::mat4 model =
                glm::rotate(glm::mat4(1.0f), seconds, glm::vec3(0.0f, 1.0f, 0.0f)) *
                glm::rotate(glm::mat4(1.0f), seconds * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::mat4 view = glm::lookAtRH(
                glm::vec3(0.0f, 0.0f, 5.5f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 projection = glm::perspectiveRH_ZO(
                glm::radians(60.0f),
                static_cast<float>(swapchain->getWidth()) / static_cast<float>(swapchain->getHeight()),
                0.1f,
                100.0f);
            projection[1][1] *= -1.0f;

            PushConstants pushConstants{};
            pushConstants.Model = model;
            pushConstants.ModelViewProjection = projection * view * model;
            commandList->setPushConstants(
                rhi::EShaderStage::Vertex | rhi::EShaderStage::Pixel,
                0,
                static_cast<uint32_t>(sizeof(PushConstants)),
                &pushConstants);

            commandList->drawIndexed(static_cast<uint32_t>(mesh.Indices.size()));

            commandList->setGraphicsPipeline(edgePipeline.get());
            commandList->setVertexBuffer(0, vertexBuffer.get());
            commandList->setIndexBuffer(edgeIndexBuffer.get(), rhi::EIndexFormat::UInt32);
            commandList->setPushConstants(
                rhi::EShaderStage::Vertex | rhi::EShaderStage::Pixel,
                0,
                static_cast<uint32_t>(sizeof(PushConstants)),
                &pushConstants);
            commandList->drawIndexed(static_cast<uint32_t>(edgeIndices.size()));
            commandList->endRenderPass();

            rhi::RResourceBarrier colorToPresent{};
            colorToPresent.Texture = backBuffer;
            colorToPresent.Before = rhi::EResourceState::Target;
            colorToPresent.After = rhi::EResourceState::Present;
            commandList->resourceBarrier(colorToPresent);

            commandList->end();

            rhi::RCommandList* submitLists[] = { commandList.get() };
            rhi::RDevice::QueueSubmitDescriptor submitDesc{};
            submitDesc.CommandLists = submitLists;
            submitDesc.CommandListCount = 1;
            device->submitCommandLists(rhi::ECommandQueueType::Graphics, submitDesc);

            swapchain->present();
        }

        device->waitIdle();
    }
    catch (const std::exception& error)
    {
        SDL_Log("Sandbox failed: %s", error.what());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}