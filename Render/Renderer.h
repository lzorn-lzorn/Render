#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "RHI/Definitions.h"
#include "Materials/Material.h"
#include "Geometry.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCompiler/DrawConstants.h"

namespace render
{
using rhi::RShaderHandle;
using rhi::RClearValue;

template <typename WindowHandle>
class RendererBuilder;

enum class ERenderExecutionType
{
	Demand,     // 场景有变化, 相机变化, UI 变化或者显式调用 RenderRequest() 时才渲染.
	Continuous, // 每帧渲染, 例如动画, 粒子, 与time有关的 shader
	Incremental // 渐进式渲染, 尽可能复用上一帧的数据
};

enum class ERenderPath
{
	Forward,
	Deferred
};

enum class EDrawDomain : std::uint8_t
{
	Scene, // 场景绘制
	UI,    // UI 绘制
	Overlay, // 叠加绘制, 例如调试信息, 文字
	PostProcess, // 后处理绘制, 例如全屏特效
	Debug, // 调试绘制, 例如线框, 辅助线
	Gizmo, // 变换控件绘制, 例如旋转, 缩放, 平移控件
};

enum class ERenderReason : std::uint32_t
{
	None                 = 0,
	SceneChanged         = 1u << 0,
	CameraChanged        = 1u << 1,
	UIChanged            = 1u << 2,
	WindowResized        = 1u << 3,
	TransientDrawChanged = 1u << 4,
	AnimationChanged     = 1u << 5,
	ExplicitRequest      = 1u << 6,
};

constexpr ERenderReason operator|(ERenderReason lhs, ERenderReason rhs)
{
	return static_cast<ERenderReason>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

template <typename InPlatformType>
class Renderer
{
	template <typename WindowHandle>
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