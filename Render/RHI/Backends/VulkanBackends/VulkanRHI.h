#pragma once

#include "../../Definitions.h"

#include <vulkan/vulkan.h>

namespace render::rhi
{

VkFormat ToVkFormat(EFormat format);
VkIndexType ToVkIndexType(EIndexFormat format);
VkShaderStageFlags ToVkShaderStageFlags(EShaderStage stage);
VkBufferUsageFlags ToVkBufferUsage(EBufferUsage usage);
VkImageUsageFlags ToVkImageUsage(ETextureUsage usage);
VkPrimitiveTopology ToVkPrimitiveTopology(EPrimitiveTopology topology);
VkPolygonMode ToVkPolygonMode(EFillMode mode);
VkCullModeFlags ToVkCullMode(ECullMode mode);
VkCompareOp ToVkCompareOp(ECompareOp op);
VkStencilOp ToVkStencilOp(EStencilOp op);
VkSampleCountFlagBits ToVkSampleCount(ESampleCount count);
VkBlendFactor ToVkBlendFactor(EBlendFactor factor);
VkBlendOp ToVkBlendOp(EBlendOp op);
VkImageLayout ToVkImageLayout(EResourceState state);
VkPipelineStageFlags ToVkPipelineStage(EResourceState state);
VkAccessFlags ToVkAccessMask(EResourceState state);
bool IsDepthFormat(EFormat format);

} // namespace render::rhi
