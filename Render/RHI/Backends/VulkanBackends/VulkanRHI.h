#pragma once

#include "../../Definitions.h"

#include <vulkan/vulkan.h>

namespace render::rhi
{

VkFormat toVkFormat(EFormat);
VkIndexType toVkIndexType(EIndexFormat);
VkShaderStageFlags toVkShaderStageFlags(EShaderStage);
VkBufferUsageFlags toVkBufferUsage(EBufferUsage);
VkImageUsageFlags toVkImageUsage(ETextureUsage);
VkPrimitiveTopology toVkPrimitiveTopology(EPrimitiveTopology);
VkPolygonMode toVkPolygonMode(EFillMode);
VkCullModeFlags toVkCullMode(ECullMode);
VkCompareOp toVkCompareOp(ECompareOp);
VkStencilOp toVkStencilOp(EStencilOp);
VkSampleCountFlagBits toVkSampleCount(ESampleCount);
VkBlendFactor toVkBlendFactor(EBlendFactor);
VkBlendOp toVkBlendOp(EBlendOp);
VkImageLayout toVkImageLayout(EResourceState);
VkPipelineStageFlags toVkPipelineStage(EResourceState);
VkAccessFlags toVkAccessMask(EResourceState);
VkSharingMode toVkSharingMode(ESharingMode);
VkMemoryPropertyFlags toVkMemoryPropertyFlags(EMemoryProperty);
bool isDepthFormat(EFormat);

} // namespace render::rhi
