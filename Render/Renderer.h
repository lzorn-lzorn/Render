#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <expected>

#include "RHI/Definitions.h"
#include "Materials/Material.h"
#include "Geometry.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCompiler/DrawConstants.h"

namespace render
{
using rhi::RShaderHandle;
using rhi::RClearValue;

enum class ERenderError
{

};

enum class ERenderExecutionType
{
	Demand,     // 场景有变化, 相机变化, UI 变化或者显式调用 RenderRequest() 时才渲染.
	Continuous, // 每帧渲染, 例如动画, 粒子, 与time有关的 shader
	Incremental // 渐进式渲染, 尽可能复用上一帧的数据
};

enum class ERenderType : std::uint8_t
{
	Forward,
	Deferred,
	Offline,
	Mobile
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

/**
 * @brief UI 渲染器, 其内部不会使用 RDG, 而是以逐帧渲染为基础, 在一个 RenderTarget 上渲染UI
 */
class UIRenderer
{
public:
	explicit UIRenderer() = default;

	UIRenderer(UIRenderer&&) = delete;
	UIRenderer& operator=(UIRenderer&&) = delete;

	UIRenderer(const UIRenderer&) = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;

	UIRenderer& setRDevice(std::shared_ptr<rhi::RDevice> InDevice);

	void beginFrame();
	void draw();
	void endFrame();
};

class ForwardSceneRenderer
{

};


class DeferredSceneRenderer
{
public:
	explicit DeferredSceneRenderer() = default;

	DeferredSceneRenderer(DeferredSceneRenderer&&) = delete;
	DeferredSceneRenderer& operator=(DeferredSceneRenderer&&) = delete;

	DeferredSceneRenderer(const DeferredSceneRenderer&) = delete;
	DeferredSceneRenderer& operator=(const DeferredSceneRenderer&) = delete;

public:
	// 初始化相关
	DeferredSceneRenderer& setRDevice(std::shared_ptr<rhi::RDevice> InDevice);

    // 编译器将 HLSL 编译为 SPIR-V 后,通过此接口注册.
    // DeferredSceneRenderer 复制 SPIR-V 并创建 VkShaderModule.
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

};

class RenderBuilder
{
public:
	RenderBuilder() = default;

	RenderBuilder(RenderBuilder&&) = delete;
	RenderBuilder& operator=(RenderBuilder&&) = delete;

	RenderBuilder(const RenderBuilder&) = delete;
	RenderBuilder& operator=(const RenderBuilder&) = delete;

	RenderBuilder& setRenderType(ERenderType Type);
	RenderBuilder& setRHI(std::unique_ptr<rhi::RHIFactory> Factory);
	RenderBuilder& setShaderCompiler(std::unique_ptr<ShaderCompiler> Compiler);
	RenderBuilder& setRenderGraph();

	std::expected<std::unique_ptr<DeferredSceneRenderer>, ERenderError> build();
};
	
} // namespace render