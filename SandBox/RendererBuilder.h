
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include <SDL3/SDL.h>

#include "RHI/RHI.h"
#include "Renderer.h"

namespace render
{
template <typename WindowHandle>
class RendererBuilder;

template <>
class RendererBuilder<SDL_Window>
{
public:
	RendererBuilder() = default;
	~RendererBuilder() = default;

	RendererBuilder& setWidth(uint32_t InWidth) { Width = InWidth; return *this; }
	RendererBuilder& setHeight(uint32_t InHeight) { Height = InHeight; return *this; }
	RendererBuilder& setSampleCount(rhi::ESampleCount InSampleCount) { SampleCount = InSampleCount; return *this; }
	RendererBuilder& setColorFormat(rhi::EFormat InColorFormat) { ColorFormat = InColorFormat; return *this; }
	RendererBuilder& setDepthFormat(rhi::EFormat InDepthFormat) { DepthFormat = InDepthFormat; return *this; }

	std::unique_ptr<Renderer<SDL_Window>> build();

private:
	uint32_t Width = 0;
	uint32_t Height = 0;
	const char* Title = nullptr;
	bool bIsVSyncEnabled = true;
	bool bIsValidationEnabled = false;
	rhi::ESampleCount SampleCount = rhi::ESampleCount::Count1;
	rhi::EFormat ColorFormat = rhi::EFormat::Undefined;
	rhi::EFormat DepthFormat = rhi::EFormat::Undefined;
};

} // namespace render