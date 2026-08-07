#pragma once

#include "../../RHI.h"
#include <vulkan/vulkan.h>

namespace render::rhi
{

VkFormat toVkFormat(EFormat);
VkIndexType toVkIndexType(EIndexFormat);
VkShaderStageFlags toVkShaderStageFlags(EShaderStage);
VkBufferUsageFlags toVkBufferUsage(EBufferUsage);
VkImageUsageFlags toVkImageUsage(EImageUsage);
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
VkImageAspectFlags toVkImageAspectMask(ETextureAspect Aspect, EFormat TextureFormat);
VkImageAspectFlags toVkImageAspectMask(const RTextureViewDescriptor& ViewDescriptor, EFormat TextureFormat);
uint64_t clampCopySize(uint64_t Offset, uint64_t RequestedSize, uint64_t MaxSize);
VkImageType toVkImageType(EImageDimension Dimension);

bool buildMappedMemoryRange(
	class VulkanDevice* Device,
	VkDeviceMemory Memory,
	uint64_t BufferSize,
	uint64_t Offset,
	uint64_t Size,
	VkMappedMemoryRange& OutRange);

bool isDepthFormat(EFormat);

} // namespace render::rhi
