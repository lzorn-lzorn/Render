#pragma once

#include "../../Definitions.h"

#include <vulkan/vulkan.h>

namespace render::rhi
{

VkFormat ToVkFormat(EFormat);
VkIndexType ToVkIndexType(EIndexFormat);
VkShaderStageFlags ToVkShaderStageFlags(EShaderStage);
VkBufferUsageFlags ToVkBufferUsage(EBufferUsage);
VkImageUsageFlags ToVkImageUsage(ETextureUsage);
VkPrimitiveTopology ToVkPrimitiveTopology(EPrimitiveTopology);
VkPolygonMode ToVkPolygonMode(EFillMode);
VkCullModeFlags ToVkCullMode(ECullMode);
VkCompareOp ToVkCompareOp(ECompareOp);
VkStencilOp ToVkStencilOp(EStencilOp);
VkSampleCountFlagBits ToVkSampleCount(ESampleCount);
VkBlendFactor ToVkBlendFactor(EBlendFactor);
VkBlendOp ToVkBlendOp(EBlendOp);
VkImageLayout ToVkImageLayout(EResourceState);
VkPipelineStageFlags ToVkPipelineStage(EResourceState);
VkAccessFlags ToVkAccessMask(EResourceState);
VkSharingMode ToVkSharingMode(ESharingMode);
VkMemoryPropertyFlags ToVkMemoryPropertyFlags(EMemoryProperty);
bool IsDepthFormat(EFormat);

} // namespace render::rhi
