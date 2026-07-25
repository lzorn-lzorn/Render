
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <span>
#include <cstddef>
#include <type_traits>
namespace render::rhi
{

using DeviceSizeType = uint64_t; // device memory size and offset values
// Vk: typedef uint64_t VkDeviceSize;

enum class EResourceType
{
	Buffer,
	Texture,
	Sampler,
	Shader,
	Pipeline,
	DescriptorSet,
	DescriptorSetLayout,
	Fence,
	Swapchain
};

enum class EResourceState {
    Undefined,
    Common,
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,
    ShaderResource,
    UnorderedAccess,
    Target,
    DepthWrite,
    DepthRead,
    CopySrc,
    CopyDst,
    Present,
};

enum class EFormat : uint32_t
{
	Undefined,
	RGBA8_UNorm,
	RGBA8_sRGB,
	BGRA8_UNorm,
	RGBA16_Float,
	RGBA32_Float,
	D24_UNorm_S8_UInt,
	D32_Float,
	// ...
};

enum class EBufferUsage : uint32_t
{
	None        = 0,
	Vertex      = 1 << 0,
	Index       = 1 << 1,
	Uniform     = 1 << 2,
	Storage     = 1 << 3,
	Indirect    = 1 << 4,
	TransferSrc = 1 << 5,
	TransferDst = 1 << 6,

};

inline EBufferUsage operator|(EBufferUsage a, EBufferUsage b)
{
	return static_cast<EBufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class ETextureUsage : uint32_t
{
	None         = 0,
	Sampled      = 1 << 0,
	Storage      = 1 << 1,
	Target       = 1 << 2,
	DepthStencil = 1 << 3,
	TransferSrc  = 1 << 4,
	TransferDst  = 1 << 5,
	Present      = 1 << 6,
};
inline ETextureUsage operator|(ETextureUsage a, ETextureUsage b)
{
	return static_cast<ETextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class EShaderStage : uint32_t
{
	Vertex        = 1 << 0,
	Pixel         = 1 << 1,
	Compute       = 1 << 2,
	Geometry      = 1 << 3,
	Mesh	      = 1 << 4,
	Amplification = 1 << 5,
};

enum class ECommandQueueType
{
	Graphics,
	Compute,
	Copy
};

enum class ELoadOp
{
	Load,
	Clear,
	DontCare
};

enum class EStoreOp
{
	Store,
	DontCare
};

enum class EIndexFormat
{
	None,
	UInt16,
	UInt32
};

enum class EPrimitiveTopology
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip
};

enum class EDescriptorType
{
	UniformBuffer,
	StorageBuffer,
	Sampler,
	SampledTexture,
	StorageTexture
};

enum class EBlendFactor
{
	Zero,
	One,
	SrcAlpha,
	OneMinusSrcAlpha,
	/* ... */
};

enum class EBlendOp
{
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max
	/* ... */
};

enum class ECompareOp
{
	Less,
	LessEqual,
	Equal,
	NotEqual,
	GreaterEqual,
	Greater,
	Always,
	Never
	/* ... */
};

enum class EStencilOp
{
	Keep,
	Zero,
	Replace,
	IncrementAndClamp,
	DecrementAndClamp,
	Invert,
	IncrementAndWrap,
	DecrementAndWrap
};

enum class ECullMode
{
	None,
	Front,
	Back
};

enum class EFillMode
{
	Solid,
	Wireframe
};

enum class ESampleCount
{
	Count1 = 1,
	Count2 = 2,
	Count4 = 4,
	Count8 = 8,
	Count16 = 16,
	Count32 = 32,
	Count64 = 64
};

struct RBufferDescriptor
{
	EBufferUsage Usage = EBufferUsage::None;
	uint32_t Size = 0;
	bool IsCpuVisible = false;
	std::string Name;
};

struct RTextureDescriptor
{
	ETextureUsage Usage = ETextureUsage::None;
	EFormat Format = EFormat::BGRA8_UNorm;
	uint32_t Width = 1;
	uint32_t Height = 1;
	uint32_t Depth = 1;
	uint32_t MipLevels = 1;
	uint32_t ArrayLayers = 1;
	ESampleCount SampleCount = ESampleCount::Count1;
	std::string Name;
};

struct RSamplerDescriptor
{
	enum class EFilter
	{
		Nearest,
		Linear
	};
	enum class EAddressMode
	{
		Repeat,
		Mirror,
		Clamp
	};
	
	EFilter MinFilter = EFilter::Linear;
	EFilter MipFilter = EFilter::Linear;
	EFilter MagFilter = EFilter::Linear;
	EAddressMode AddressModeU = EAddressMode::Repeat;
	EAddressMode AddressModeV = EAddressMode::Repeat;
	EAddressMode AddressModeW = EAddressMode::Repeat;
	std::string Name;
};

struct RShaderDescriptor
{
	EShaderStage Stage;
	std::vector<std::byte> ByteCodes;
	std::string Name;
	std::string EntryPoint = "main";
	std::string SourceCode;
};

struct RShaderHandle
{
	uint64_t Value = 0;
};

struct RVertexAttribute
{
	uint32_t Location;
	uint32_t Binding;
	EFormat Format;
	uint32_t Offset;
};

struct RVertexBindingDescriptor
{
	enum class EVertexInputRate
	{
		// 着色器每处理一个顶点, 就会从绑定的缓冲区中按 Stride 前进一次, 读取下一组顶点数据
		Vertex, 
		// 着色器每处理一个实例,才按 stride 前进一次. 也就是说, 同一实例内的所有顶点共享同一份实例数据
		Instance
	};
	uint32_t Binding;
	uint32_t Stride;

	// 着色器在绘制多少个实例后, 才从该绑定中读取下一组实例数据, 仅在 Instance 模式下有效
	uint32_t InstanceStepRate;
	EVertexInputRate Rate = EVertexInputRate::Vertex;
};

struct RVertexBufferView
{
	RBuffer VertexBuffer;
	DeviceSizeType Size;
	RVertexBindingDescriptor BindingDescriptor; 
	bool isValid() const noexcept { return VertexBuffer.isValid(); }
};

struct RIndexBufferView
{
	RBuffer IndexBuffer;
	DeviceSizeType Offset;
	DeviceSizeType Size;
	EIndexFormat Format;

	bool isValid() const noexcept { return IndexBuffer.isValid(); }
};


struct RVertexInputLayout
{
	std::vector<RVertexAttribute> Attributes;
	std::vector<RVertexBindingDescriptor> Bindings;
};

struct RGeometryView
{
	std::span<const RVertexBufferView> VertexBuffers;
	std::span<const RVertexAttribute> VertexAttributes;
	
	RIndexBufferView IndexBuffer;
	uint32_t VertexCount = 0;
	uint32_t FirstVertex = 0;
	uint32_t FirstInstance = 0;
	uint32_t InstanceCount = 1;

	[[nodiscard]] bool isIndexed() const { return IndexBuffer.isValid(); }
};



struct RRasterizerState
{
	EFillMode FillMode = EFillMode::Solid;
	ECullMode CullMode = ECullMode::Back;
	bool FrontCounterClockwise = false;
	int32_t DepthBias = 0;
	float DepthBiasClamp = 0.0f;
	float DepthBiasSlopeFactor = 0.0f;
	float SlopeScaledDepthBias = 0.0f;
	bool DepthClipEnable = true;
	bool ScissorEnable = false;
	bool MultisampleEnable = false;
	bool AntialiasedLineEnable = false;
};

struct RBlendAttachment
{
	bool BlendEnable = false;
	EBlendFactor SrcColorBlendFactor = EBlendFactor::One;
	EBlendFactor DstColorBlendFactor = EBlendFactor::Zero;
	EBlendOp ColorBlendOp = EBlendOp::Add;
	EBlendFactor SrcAlphaBlendFactor = EBlendFactor::One;
	EBlendFactor DstAlphaBlendFactor = EBlendFactor::Zero;
	EBlendOp AlphaBlendOp = EBlendOp::Add;
};

struct RBlendState
{
	bool BlendEnable = false;
	std::vector<RBlendAttachment> Attachments;
};

struct RDepthStencilState
{
	bool DepthTestEnable = true;
	bool DepthWriteEnable = true;
	ECompareOp DepthFunc = ECompareOp::Less;
	bool StencilTestEnable = false;
	uint32_t StencilReadMask = 0xFF;
	uint32_t StencilWriteMask = 0xFF;
	struct StencilOpState
	{
		EStencilOp StencilFailOp = EStencilOp::Keep;
		EStencilOp StencilDepthFailOp = EStencilOp::Keep;
		EStencilOp StencilPassOp = EStencilOp::Keep;
		ECompareOp StencilFunc = ECompareOp::Always;
	};
	StencilOpState FrontFace;
	StencilOpState BackFace;

};

struct RGraphicsPipelineDescriptor
{
	class RShader* VertexShader = nullptr;
	class RShader* PixelShader = nullptr;
	class RShader* GeometryShader = nullptr;
	class RShader* MeshShader = nullptr;
	class RShader* AmplificationShader = nullptr;
	
	EVertexInputLayout VertexInputLayout;
	RRasterizerState RasterizerState;
	RBlendState BlendState;
	RDepthStencilState DepthStencilState;

	std::array<EFormat, 8> RenderTargetFormats = {};
	uint32_t RenderTargetCount = 0;
	EFormat DepthStencilFormat = EFormat::Unknown;
	
	EPrimitiveTopology PrimitiveTopology = EPrimitiveTopology::TriangleList;
	uint32_t SampleCount = 1;
};

struct RComputePipelineDescriptor
{
	class RShader* ComputeShader = nullptr;
};
struct RTextureViewDescriptor
{
	enum class EViewType
	{
		SRV,
		UAV,
		RTV,
		DSV
	};
	EViewType Type = EViewType::SRV;
	EFormat Format = EFormat::Unknown;
	uint32_t BaseMipLevel = 0;
	uint32_t MipLevelCount = 1;
	uint32_t BaseArrayLayer = 0;
	uint32_t ArrayLayerCount = 1;
};

struct RClearValue
{
	std::array<float, 4> Color = {0.0f, 0.0f, 0.0f, 1.0f};
	float Depth = 1.0f;
	uint32_t Stencil = 0;
};

struct RRenderTargetAttachment
{
	class RTextureView* TextureView = nullptr;
	ELoadOp LoadOp = ELoadOp::Clear;
	EStoreOp StoreOp = EStoreOp::Store;
	RClearValue ClearValue;
};

struct RDepthStencilAttachment
{
	class RTextureView* TextureView = nullptr;
	ELoadOp DepthLoadOp = ELoadOp::Clear;
	EStoreOp DepthStoreOp = EStoreOp::Store;
	ELoadOp StencilLoadOp = ELoadOp::Clear;
	EStoreOp StencilStoreOp = EStoreOp::Store;
	RClearValue ClearValue;
};

struct RRenderPassDescriptor
{
	std::array<RRenderTargetAttachment, 8> ColorAttachments;
	uint32_t ColorAttachmentCount = 0;
	RDepthStencilAttachment* DepthStencilAttachment;
};

struct RResourceBarrier
{
	class RTexture* Texture = nullptr;
	EResourceState Before = EResourceState::Undefined;
	EResourceState After = EResourceState::Common;
};

struct RDescriptorSetLayoutEntry
{
	uint32_t Binding;
	EShaderStage Stage;
	EDescriptorType Type;
	uint32_t Count = 1;
};

using RDescriptorSetLayoutDescriptor = std::vector<RDescriptorSetLayoutEntry>;

struct RDescriptorBinding
{
	union {
		class RBuffer* Buffer;
		class RTexture* Texture;
		class RSampler* Sampler;
	};
	uint32_t ArrayIndex = 0;
};

struct RDescriptorSetDescriptor
{
	class RDescriptorSetLayout* Layout = nullptr;
	std::vector<RDescriptorBinding> Bindings;
};


struct RViewport 
{
	float X = 0.0f;
	float Y = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;
	float MinDepth = 0.0f;
	float MaxDepth = 1.0f;
};

struct RRect
{
	int32_t X = 0;
	int32_t Y = 0;
	uint32_t Width = 0;
	uint32_t Height = 0;
};

struct RBufferCopyDescriptor
{
	uint32_t SrcOffset = 0;
	uint32_t DstOffset = 0;
	uint32_t Size = 0;
};

struct RTextureCopyDescriptor
{
	uint32_t SrcMipLevel = 0;
	uint32_t SrcArrayLayer = 0;
	uint32_t DstMipLevel = 0;
	uint32_t DstArrayLayer = 0;
	uint32_t Width = 0;
	uint32_t Height = 0;
	uint32_t Depth = 1;
};


/**
 * @brief RResource 内封装的是一个 GPU 资源的句柄对象.
 */
class RResource
{
public:
	virtual ~RResource() = default;
	virtual EResourceType getType() const = 0;
	virtual bool isValid() const = 0;
	void setName(const std::string& InName) 
	{ 
		Name = InName; 
		SetDebugName(Name);
	}
	const std::string& getName() const { return Name; }
protected:
	virtual void SetDebugName(const std::string& InName) = 0;
	std::string Name;
};

class RBuffer : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Buffer; }
	virtual uint64_t getSize() const = 0;
};

class RTexture : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Texture; }
	virtual class RTextureView* createView(const ETextureViewDescriptor& Descriptor) = 0;
	virtual uint32_t getWidth() const = 0;
	virtual uint32_t getHeight() const = 0;
	virtual uint32_t getDepth() const = 0;
	virtual EFormat getFormat() const = 0;
};

class RTextureView : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Texture; }
	virtual RTexture* getTexture() const = 0;
	virtual ETextureViewDescriptor getDescriptor() const = 0;
};

class RSampler : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Sampler; }
};


class RShader: public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Shader; }
	virtual EShaderStage getStage() const = 0;
};

class RPipelineState : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Pipeline; }
	virtual bool isGraphicsPipeline() const = 0;
};

class RDescriptorSetLayout : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::DescriptorSetLayout; }

};

class RDescriptorSet : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::DescriptorSet; }

};



class RCommandList 
{
public:
	virtual ~RCommandList() = default;
	virtual void begin() = 0;
	virtual void end() = 0;

	virtual void beginRenderPass(const RRenderPassDescriptor& Descriptor) = 0;
	virtual void endRenderPass() = 0;

	virtual void setGraphicsPipeline(class RPipelineState* Pipeline) = 0;
	virtual void setComputePipeline(class RPipelineState* Pipeline) = 0;
	virtual void setDescriptorSet(uint32_t Index, class RDescriptorSet* DescriptorSet) = 0;
	virtual void setVertexBuffer(uint32_t Slot, class RBuffer* Buffer) = 0;
	virtual void setIndexBuffer(class RBuffer* Buffer, EIndexFormat Format) = 0;
	virtual void setViewport(const RViewport& Viewport) = 0;
	virtual void setScissorRect(const RRect& Rect) = 0;

	virtual void draw(uint32_t VertexCount, uint32_t InstanceCount = 1, uint32_t FirstVertex = 0, uint32_t FirstInstance = 0) = 0;
	virtual void drawIndexed(uint32_t IndexCount, uint32_t InstanceCount = 1, uint32_t FirstIndex = 0, int32_t VertexOffset = 0, uint32_t FirstInstance = 0) = 0;

	virtual void dispatch(uint32_t GroupX, uint32_t GroupY = 1, uint32_t GroupZ = 1) = 0;
	
	virtual void copyBuffer(RBuffer* Src, RBuffer* Dst, const RBufferCopyDescriptor& Descriptor) = 0;
	virtual void copyTexture(RTexture* Src, RTexture* Dst, const RTextureCopyDescriptor& Descriptor) = 0;

	virtual void resourceBarrier(const RResourceBarrier& Barriers) = 0;
	virtual void resourceBarriers(std::span<const RResourceBarrier> Barriers) = 0;
};

class RSwapchain
{
public:
	virtual ~RSwapchain() = default;
	virtual EResourceType getType() const { return EResourceType::Swapchain; }
	virtual RTexture* acquireNextTexture() = 0;
	virtual void present() = 0;
	virtual void resize(uint32_t Width, uint32_t Height) = 0;
	virtual EFormat getFormat() const = 0;
	virtual uint32_t getWidth() const = 0;
	virtual uint32_t getHeight() const = 0;
};

class RFence 
{
public:
	virtual ~RFence() = default;
	virtual EResourceType getType() const { return EResourceType::Fence; }
	virtual void signal(uint64_t Value) = 0;
	virtual uint64_t getCompletedValue() const = 0;
	virtual void wait(uint64_t Value) = 0;

};

class RDevice
{
public:
	virtual ~RDevice() = default;

	// 资源创建
	virtual RBuffer* createBuffer(const EBufferDescriptor& Descriptor) = 0;
	virtual RTexture* createTexture(const ETextureDescriptor& Descriptor) = 0;
	virtual RSampler* createSampler(const ESamplerDescriptor& Descriptor) = 0;
	virtual RShader* createShader(const EShaderDescriptor& Descriptor) = 0;
	virtual RPipelineState* createGraphicsPipeline(const RGraphicsPipelineDescriptor& Descriptor) = 0;
	virtual RPipelineState* createComputePipeline(const RComputePipelineDescriptor& Descriptor) = 0;
	virtual RDescriptorSetLayout* createDescriptorSetLayout(const RDescriptorSetLayoutDescriptor& Descriptor) = 0;
	virtual RDescriptorSet* createDescriptorSet(const RDescriptorSetDescriptor& Descriptor) = 0;
	
	// 命令与同步
	virtual RCommandList* createCommandList(ECommandQueueType Type) = 0;
	virtual RSwapchain* createSwapchain(const ETextureDescriptor& Descriptor) = 0;
	virtual RFence* createFence() = 0;

	// 队列提交
	struct QueueSubmitDescriptor
	{
		RCommandList** CommandLists;
		uint32_t CommandListCount;
		RFence* SignalFence = nullptr;
		uint64_t SignalValue = 0;
	};
	virtual void submitCommandLists(ECommandQueueType Type, const QueueSubmitDescriptor& Descriptor) = 0;

	// 同步
	virtual void waitIdle() = 0;

	// 析构资源
	virtual void destroyResource(RResource* Resource) = 0;
};

struct RSwapchainDescriptor
{
	uint32_t Width = 800;
	uint32_t Height = 600;
	EFormat Format = EFormat::BGRA8_UNorm;
	uint32_t BufferCount = 3;
	bool VSync = true;
	std::string Name;
};

} // namespace render::rhi