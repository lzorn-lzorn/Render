#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "RHI/Definitions.h"
#include "Material.h"
#include "Geometry.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCompiler/DrawConstants.h"

namespace render
{
using rhi::RShaderHandle;
using rhi::RClearValue;

template <typename InPlatformType>
class Renderer
{
	friend class RendererBuilder;

	using PlatformType = InPlatformType;
public:

	Renderer(Renderer&&) = default;
	Renderer& operator=(Renderer&&) = default;

    // 编译器将 HLSL 编译为 SPIR-V 后,通过此接口注册.
    // Renderer 复制 SPIR-V 并创建 VkShaderModule.
	void registerShader(RShaderHandle InShaderHandle, std::span<const uint8_t> InShaderCode);

	void removeShader(RShaderHandle InShaderHandle);
	void beginFrame();
	void beginRender(RClearValue InClearColor, float InClearDepth = 1.0f, uint32_t InClearStencil = 0);

	void draw(
		const RGeometryView& InGeometryView,
		const RMaterialBindingDescriptor& InMaterialBindingDescriptor,
		const DrawConstants& InDrawConstants
	);
	void endRender();
	void endFrame();

	// 在 SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED、SDL_EVENT_WINDOW_RESIZED
    // 或 VK_ERROR_OUT_OF_DATE_KHR 时调用.
    void notifyWindowResized() noexcept;
private:
	explicit Renderer();
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

private:
	// 在 Cpp 中实现对应的 Impl 类, 用于隐藏平台相关的实现细节.
	struct Impl;
	std::unique_ptr<Impl> pImpl;

};

	
} // namespace render