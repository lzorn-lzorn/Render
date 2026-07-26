
#pragma once 
#include <cstddef>
#include <SDL3/SDL.h>

#include "Render/RHI/Definitions.h"
#include "Render/Renderer.h"

namespace render
{
template <typename WindowHandle>
class RendererBuilder;

class RendererBuilder<SDL_Window>
{
public:
	RendererBuilder() = default;
	~RendererBuilder() = default;

	RendererBuilder& setWidth(uint32_t InWidth) { Width = InWidth; return *this; }
	RendererBuilder& setHeight(uint32_t InHeight) { Height = InHeight; return *this; }
	RendererBuilder& setSampleCount(ESampleCount InSampleCount) { SampleCount = InSampleCount; return *this; }
	RendererBuilder& setColorFormat(EFormat InColorFormat) { ColorFormat = InColorFormat; return *this; }
	RendererBuilder& setDepthFormat(EFormat InDepthFormat) { DepthFormat = InDepthFormat; return *this; }

	std::unique_ptr<Renderer> build();

private:
	uint32_t Width = 0;
	uint32_t Height = 0;
	const char* Title = nullptr;
	bool bIsVSyncEnabled = true;
	bool bIsValidationEnabled = false;
	ESampleCount SampleCount = ESampleCount::COUNT_1;
	EFormat ColorFormat = EFormat::UNKNOWN;
	EFormat DepthFormat = EFormat::UNKNOWN;
};

} // namespace render