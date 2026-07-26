#pragma once

#include "Definitions.h"

#include <memory>

namespace render::rhi
{

std::unique_ptr<RDevice> CreateVulkanDevice();

} // namespace render::rhi
