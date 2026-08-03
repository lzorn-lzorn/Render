#include "VulkanResources.h"
#include "VulkanDevice.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace render::rhi
{

namespace
{

VkImageType toVkImageType(ETextureDimension Dimension)
{
	switch (Dimension)
	{
	case ETextureDimension::Texture1D:
	case ETextureDimension::Texture1DArray:
		return VK_IMAGE_TYPE_1D;
	case ETextureDimension::Texture3D:
		return VK_IMAGE_TYPE_3D;
	case ETextureDimension::Texture2D:
	case ETextureDimension::Texture2DArray:
	case ETextureDimension::Cube:
	case ETextureDimension::CubeArray:
	default:
		return VK_IMAGE_TYPE_2D;
	}
}

VkImageViewType toVkImageViewType(ETextureDimension Dimension)
{
	switch (Dimension)
	{
	case ETextureDimension::Texture1D:
		return VK_IMAGE_VIEW_TYPE_1D;
	case ETextureDimension::Texture1DArray:
		return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
	case ETextureDimension::Texture2D:
		return VK_IMAGE_VIEW_TYPE_2D;
	case ETextureDimension::Texture2DArray:
		return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	case ETextureDimension::Texture3D:
		return VK_IMAGE_VIEW_TYPE_3D;
	case ETextureDimension::Cube:
		return VK_IMAGE_VIEW_TYPE_CUBE;
	case ETextureDimension::CubeArray:
		return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
	default:
		return VK_IMAGE_VIEW_TYPE_2D;
	}
}

VkExtent3D getMipExtent(const RTextureDescriptor& TextureDesc, uint32_t MipLevel)
{
	const uint32_t mip_width = std::max(1u, TextureDesc.Width >> MipLevel);
	const uint32_t mip_height = std::max(1u, TextureDesc.Height >> MipLevel);
	const uint32_t mip_depth = std::max(1u, TextureDesc.Depth >> MipLevel);

	if (TextureDesc.Dimension == ETextureDimension::Texture1D || TextureDesc.Dimension == ETextureDimension::Texture1DArray)
	{
		return { mip_width, 1, 1 };
	}
	if (TextureDesc.Dimension == ETextureDimension::Texture3D)
	{
		return { mip_width, mip_height, mip_depth };
	}
	return { mip_width, mip_height, 1 };
}

VkImageLayout getPreferredImageLayout(const RTextureDescriptor& TextureDesc)
{
	if (hasAnyFlags(TextureDesc.Usage, ETextureUsage::Present))
	{
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	if (hasAnyFlags(TextureDesc.Usage, ETextureUsage::DepthStencil) || isDepthFormat(TextureDesc.Format))
	{
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
	if (hasAnyFlags(TextureDesc.Usage, ETextureUsage::Target))
	{
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (hasAnyFlags(TextureDesc.Usage, ETextureUsage::Storage))
	{
		return VK_IMAGE_LAYOUT_GENERAL;
	}
	if (hasAnyFlags(TextureDesc.Usage, ETextureUsage::Sampled))
	{
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	return VK_IMAGE_LAYOUT_GENERAL;
}

VkPipelineStageFlags getPipelineStageForLayout(VkImageLayout Layout)
{
	switch (Layout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	case VK_IMAGE_LAYOUT_UNDEFINED:
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	case VK_IMAGE_LAYOUT_GENERAL:
	default:
		return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
}

VkAccessFlags getAccessMaskForLayout(VkImageLayout Layout)
{
	switch (Layout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		return VK_ACCESS_TRANSFER_READ_BIT;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return VK_ACCESS_TRANSFER_WRITE_BIT;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return VK_ACCESS_SHADER_READ_BIT;
	case VK_IMAGE_LAYOUT_GENERAL:
		return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
	case VK_IMAGE_LAYOUT_UNDEFINED:
	default:
		return 0;
	}
}

RTextureViewDescriptor::EViewType getDefaultViewType(const RTextureDescriptor& TextureDesc)
{
	if (hasAnyFlags(TextureDesc.Usage, ETextureUsage::DepthStencil) || isDepthFormat(TextureDesc.Format))
	{
		return RTextureViewDescriptor::EViewType::DSV;
	}
	if (hasAnyFlags(TextureDesc.Usage, ETextureUsage::Target) || hasAnyFlags(TextureDesc.Usage, ETextureUsage::Present))
	{
		return RTextureViewDescriptor::EViewType::RTV;
	}
	return RTextureViewDescriptor::EViewType::SRV;
}

} // namespace

VulkanTexture::VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc)
	: Device(InDevice)
	, TextureDesc(InTextureDesc)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	if (!initializeImage() || !allocateImageMemory())
	{
		return;
	}

	DefaultView = createView(buildDefaultViewDescriptor());
	if (!TextureDesc.Name.empty())
	{
		setDebugName(TextureDesc.Name);
	}
}

VulkanTexture::VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, const RTextureBulkData& InBulkData)
	: Device(InDevice)
	, TextureDesc(InTextureDesc)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	if (!initializeImage() || !allocateImageMemory())
	{
		return;
	}

	uploadBulkData(InBulkData);
	if (TextureDesc.ShouldGenerateMipmaps && TextureDesc.MipLevels > 1)
	{
		generateMipmaps();
	}

	DefaultView = createView(buildDefaultViewDescriptor());
	if (!TextureDesc.Name.empty())
	{
		setDebugName(TextureDesc.Name);
	}
}

VulkanTexture::VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, VkImage InExternalImage, VkImageView InExternalView)
	: Device(InDevice)
	, TextureDesc(InTextureDesc)
{
	ImageResource.Image = InExternalImage;
	ImageResource.Memory = VK_NULL_HANDLE;
	ImageResource.Layout = hasAnyFlags(TextureDesc.Usage, ETextureUsage::Present)
		? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		: VK_IMAGE_LAYOUT_UNDEFINED;
	ImageResource.Ownership = EVulkanImageOwnership::External;

	RTextureViewDescriptor default_descriptor = buildDefaultViewDescriptor();
	DefaultView = new VulkanTextureView(this, default_descriptor, InExternalView, true);
	Views.push_back(DefaultView);
}

VulkanTexture::~VulkanTexture()
{
	for (RTextureView* view : Views)
	{
		delete view;
	}
	Views.clear();
	DefaultView = nullptr;

	if (!Device)
	{
		return;
	}

	VkDevice vk_device = Device->getVkDevice();
	if (vk_device == VK_NULL_HANDLE)
	{
		return;
	}

	if (ImageResource.ownsImage() && ImageResource.Image != VK_NULL_HANDLE)
	{
		vkDestroyImage(vk_device, ImageResource.Image, nullptr);
		ImageResource.Image = VK_NULL_HANDLE;
	}

	if (ImageResource.ownsMemory() && ImageResource.Memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vk_device, ImageResource.Memory, nullptr);
		ImageResource.Memory = VK_NULL_HANDLE;
	}
}

bool VulkanTexture::isValid() const
{
	return ImageResource.isValid();
}

void VulkanTexture::updateTexture(const RTextureUpdateRegion& Region, const void* SrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch)
{
	if (!Device || !isValid() || !SrcData)
	{
		return;
	}
	if (Region.MipIndex >= TextureDesc.MipLevels || Region.ArrayLayer >= TextureDesc.ArrayLayers)
	{
		return;
	}

	const uint32_t pixel_size = calPixelSizeFormEFormat(TextureDesc.Format);
	if (pixel_size == 0)
	{
		return;
	}

	const VkExtent3D mip_extent = getMipExtent(TextureDesc, Region.MipIndex);
	if (Region.DstX >= mip_extent.width || Region.DstY >= mip_extent.height || Region.DstZ >= mip_extent.depth)
	{
		return;
	}

	const uint32_t copy_width = std::min<uint32_t>(Region.Width, mip_extent.width - Region.DstX);
	const uint32_t copy_height = std::min<uint32_t>(Region.Height, mip_extent.height - Region.DstY);
	const uint32_t copy_depth = std::min<uint32_t>(Region.Depth, mip_extent.depth - Region.DstZ);
	if (copy_width == 0 || copy_height == 0 || copy_depth == 0)
	{
		return;
	}

	const uint32_t row_pitch = SrcRowPitch != 0 ? SrcRowPitch : copy_width * pixel_size;
	const uint32_t depth_pitch = SrcDepthPitch != 0 ? SrcDepthPitch : row_pitch * copy_height;
	if (row_pitch < copy_width * pixel_size)
	{
		return;
	}

	const uint64_t upload_size = static_cast<uint64_t>(depth_pitch) * copy_depth;
	auto* staging_buffer = static_cast<VulkanBuffer*>(Device->createStagingBuffer(nullptr, upload_size));
	if (!staging_buffer || !staging_buffer->isValid())
	{
		Device->destroyResource(staging_buffer);
		return;
	}

	uint8_t* mapped_ptr = static_cast<uint8_t*>(staging_buffer->mapRange(EBufferMapMode::Write, 0, upload_size));
	if (!mapped_ptr)
	{
		Device->destroyResource(staging_buffer);
		return;
	}

	const uint8_t* src_ptr = static_cast<const uint8_t*>(SrcData);
	for (uint32_t z = 0; z < copy_depth; ++z)
	{
		for (uint32_t y = 0; y < copy_height; ++y)
		{
			const size_t src_offset = static_cast<size_t>(z) * depth_pitch + static_cast<size_t>(y) * row_pitch;
			const size_t dst_offset = src_offset;
			std::memcpy(mapped_ptr + dst_offset, src_ptr + src_offset, static_cast<size_t>(copy_width) * pixel_size);
		}
	}

	staging_buffer->flushMappedRange(0, upload_size);
	staging_buffer->unmap();

	const VkImageLayout previous_layout = ImageResource.Layout;
	VkCommandBuffer command_buffer = Device->beginImmediateCommand();
	transitionImageLayout(
		command_buffer,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		getPipelineStageForLayout(previous_layout),
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		getAccessMaskForLayout(previous_layout),
		VK_ACCESS_TRANSFER_WRITE_BIT);

	VkBufferImageCopy copy_region{};
	copy_region.bufferOffset = 0;
	copy_region.bufferRowLength = row_pitch / pixel_size;
	copy_region.bufferImageHeight = copy_height;
	copy_region.imageSubresource.aspectMask = getAspectMask();
	copy_region.imageSubresource.mipLevel = Region.MipIndex;
	copy_region.imageSubresource.baseArrayLayer = Region.ArrayLayer;
	copy_region.imageSubresource.layerCount = 1;
	copy_region.imageOffset = {
		static_cast<int32_t>(Region.DstX),
		static_cast<int32_t>(Region.DstY),
		static_cast<int32_t>(Region.DstZ)
	};
	copy_region.imageExtent = { copy_width, copy_height, copy_depth };

	vkCmdCopyBufferToImage(
		command_buffer,
		staging_buffer->getVkBuffer(),
		ImageResource.Image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&copy_region);

	const VkImageLayout restore_layout = previous_layout == VK_IMAGE_LAYOUT_UNDEFINED
		? getPreferredImageLayout(TextureDesc)
		: previous_layout;
	if (restore_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		transitionImageLayout(
			command_buffer,
			restore_layout,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			getPipelineStageForLayout(restore_layout),
			VK_ACCESS_TRANSFER_WRITE_BIT,
			getAccessMaskForLayout(restore_layout));
	}

	Device->endImmediateCommand(command_buffer);
	Device->destroyResource(staging_buffer);
}

void VulkanTexture::generateMipmaps()
{
	if (!Device || !isValid() || TextureDesc.MipLevels <= 1)
	{
		return;
	}

	if (isDepthStencilFormat(TextureDesc.Format))
	{
		return;
	}

	VkFormatProperties format_properties{};
	vkGetPhysicalDeviceFormatProperties(Device->getVkPhysicalDevice(), toVkFormat(TextureDesc.Format), &format_properties);
	if ((format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0)
	{
		return;
	}

	const VkImageLayout final_layout = getPreferredImageLayout(TextureDesc);
	const VkPipelineStageFlags final_stage = getPipelineStageForLayout(final_layout);
	const VkAccessFlags final_access = getAccessMaskForLayout(final_layout);

	const VkImageLayout previous_layout = ImageResource.Layout;
	VkCommandBuffer command_buffer = Device->beginImmediateCommand();
	transitionImageLayout(
		command_buffer,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		getPipelineStageForLayout(previous_layout),
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		getAccessMaskForLayout(previous_layout),
		VK_ACCESS_TRANSFER_WRITE_BIT);

	const VkImageAspectFlags aspect_mask = getAspectMask();
	for (uint32_t layer = 0; layer < TextureDesc.ArrayLayers; ++layer)
	{
		for (uint32_t mip = 1; mip < TextureDesc.MipLevels; ++mip)
		{
			VkImageMemoryBarrier to_src_barrier{};
			to_src_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			to_src_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			to_src_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			to_src_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			to_src_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			to_src_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			to_src_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			to_src_barrier.image = ImageResource.Image;
			to_src_barrier.subresourceRange.aspectMask = aspect_mask;
			to_src_barrier.subresourceRange.baseMipLevel = mip - 1;
			to_src_barrier.subresourceRange.levelCount = 1;
			to_src_barrier.subresourceRange.baseArrayLayer = layer;
			to_src_barrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(
				command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&to_src_barrier);

			const VkExtent3D src_extent = getMipExtent(TextureDesc, mip - 1);
			const VkExtent3D dst_extent = getMipExtent(TextureDesc, mip);

			VkImageBlit blit{};
			blit.srcSubresource.aspectMask = aspect_mask;
			blit.srcSubresource.mipLevel = mip - 1;
			blit.srcSubresource.baseArrayLayer = layer;
			blit.srcSubresource.layerCount = 1;
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = {
				static_cast<int32_t>(src_extent.width),
				static_cast<int32_t>(src_extent.height),
				static_cast<int32_t>(src_extent.depth)
			};
			blit.dstSubresource.aspectMask = aspect_mask;
			blit.dstSubresource.mipLevel = mip;
			blit.dstSubresource.baseArrayLayer = layer;
			blit.dstSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = {
				static_cast<int32_t>(dst_extent.width),
				static_cast<int32_t>(dst_extent.height),
				static_cast<int32_t>(dst_extent.depth)
			};

			vkCmdBlitImage(
				command_buffer,
				ImageResource.Image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				ImageResource.Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&blit,
				VK_FILTER_LINEAR);

			if (final_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				VkImageMemoryBarrier to_final_barrier{};
				to_final_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				to_final_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				to_final_barrier.newLayout = final_layout;
				to_final_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				to_final_barrier.dstAccessMask = final_access;
				to_final_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				to_final_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				to_final_barrier.image = ImageResource.Image;
				to_final_barrier.subresourceRange.aspectMask = aspect_mask;
				to_final_barrier.subresourceRange.baseMipLevel = mip - 1;
				to_final_barrier.subresourceRange.levelCount = 1;
				to_final_barrier.subresourceRange.baseArrayLayer = layer;
				to_final_barrier.subresourceRange.layerCount = 1;

				vkCmdPipelineBarrier(
					command_buffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					final_stage,
					0,
					0,
					nullptr,
					0,
					nullptr,
					1,
					&to_final_barrier);
			}
		}

		if (final_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			VkImageMemoryBarrier last_level_barrier{};
			last_level_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			last_level_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			last_level_barrier.newLayout = final_layout;
			last_level_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			last_level_barrier.dstAccessMask = final_access;
			last_level_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			last_level_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			last_level_barrier.image = ImageResource.Image;
			last_level_barrier.subresourceRange.aspectMask = aspect_mask;
			last_level_barrier.subresourceRange.baseMipLevel = TextureDesc.MipLevels - 1;
			last_level_barrier.subresourceRange.levelCount = 1;
			last_level_barrier.subresourceRange.baseArrayLayer = layer;
			last_level_barrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(
				command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				final_stage,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&last_level_barrier);
		}
	}

	if (final_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		ImageResource.Layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}
	else
	{
		ImageResource.Layout = final_layout;
	}

	Device->endImmediateCommand(command_buffer);
}

RTextureView* VulkanTexture::createView(const RTextureViewDescriptor& Descriptor)
{
	if (!Device || !isValid())
	{
		return nullptr;
	}

	RTextureViewDescriptor resolved_descriptor = Descriptor;
	resolved_descriptor.Format = (Descriptor.Format == EFormat::Undefined) ? TextureDesc.Format : Descriptor.Format;

	if (resolved_descriptor.MipLevel >= TextureDesc.MipLevels || resolved_descriptor.ArrayLayer >= TextureDesc.ArrayLayers)
	{
		return nullptr;
	}

	const uint32_t remaining_mips = TextureDesc.MipLevels - resolved_descriptor.MipLevel;
	resolved_descriptor.MipLevelCount = (Descriptor.MipLevelCount == 0)
		? remaining_mips
		: std::min<uint32_t>(Descriptor.MipLevelCount, remaining_mips);

	const uint32_t remaining_layers = TextureDesc.ArrayLayers - resolved_descriptor.ArrayLayer;
	resolved_descriptor.ArrayLayerCount = (Descriptor.ArrayLayerCount == 0)
		? remaining_layers
		: std::min<uint32_t>(Descriptor.ArrayLayerCount, remaining_layers);

	if (TextureDesc.Dimension == ETextureDimension::Texture3D)
	{
		resolved_descriptor.ArrayLayer = 0;
		resolved_descriptor.ArrayLayerCount = 1;
	}
	if (resolved_descriptor.MipLevelCount == 0 || resolved_descriptor.ArrayLayerCount == 0)
	{
		return nullptr;
	}

	if (resolved_descriptor.Dimension == ETextureDimension::Texture2D && TextureDesc.Dimension != ETextureDimension::Texture2D)
	{
		resolved_descriptor.Dimension = TextureDesc.Dimension;
	}

	for (RTextureView* existing_view_base : Views)
	{
		auto* existing_view = dynamic_cast<VulkanTextureView*>(existing_view_base);
		if (!existing_view)
		{
			continue;
		}

		const RTextureViewDescriptor existing_descriptor = existing_view->getDescriptor();
		if (existing_descriptor.Type == resolved_descriptor.Type &&
			existing_descriptor.Aspect == resolved_descriptor.Aspect &&
			existing_descriptor.Format == resolved_descriptor.Format &&
			existing_descriptor.MipLevel == resolved_descriptor.MipLevel &&
			existing_descriptor.MipLevelCount == resolved_descriptor.MipLevelCount &&
			existing_descriptor.ArrayLayer == resolved_descriptor.ArrayLayer &&
			existing_descriptor.ArrayLayerCount == resolved_descriptor.ArrayLayerCount &&
			existing_descriptor.Dimension == resolved_descriptor.Dimension)
		{
			return existing_view;
		}
	}

	VkImageViewCreateInfo view_info{};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = ImageResource.Image;
	view_info.viewType = toVkImageViewType(resolved_descriptor.Dimension);
	view_info.format = toVkFormat(resolved_descriptor.Format);
	view_info.subresourceRange.aspectMask = toVkImageAspectMask(resolved_descriptor, resolved_descriptor.Format);
	view_info.subresourceRange.baseMipLevel = resolved_descriptor.MipLevel;
	view_info.subresourceRange.levelCount = resolved_descriptor.MipLevelCount;
	view_info.subresourceRange.baseArrayLayer = resolved_descriptor.ArrayLayer;
	view_info.subresourceRange.layerCount = resolved_descriptor.ArrayLayerCount;
	if (view_info.subresourceRange.aspectMask == 0)
	{
		return nullptr;
	}

	VkImageView image_view = VK_NULL_HANDLE;
	if (vkCreateImageView(Device->getVkDevice(), &view_info, nullptr, &image_view) != VK_SUCCESS)
	{
		return nullptr;
	}

	auto* view = new VulkanTextureView(this, resolved_descriptor, image_view, true);
	Views.push_back(view);
	if (!DefaultView)
	{
		DefaultView = view;
	}
	return view;
}

VkImageAspectFlags VulkanTexture::getAspectMask() const noexcept
{
	return toVkImageAspectMask(ETextureAspect::Auto, TextureDesc.Format);
}

bool VulkanTexture::initializeImage()
{
	if (!Device || !Device->isValid())
	{
		return false;
	}

	TextureDesc.Width = std::max(1u, TextureDesc.Width);
	TextureDesc.Height = std::max(1u, TextureDesc.Height);
	TextureDesc.Depth = std::max(1u, TextureDesc.Depth);
	TextureDesc.MipLevels = std::max(1u, TextureDesc.MipLevels);
	TextureDesc.ArrayLayers = std::max(1u, TextureDesc.ArrayLayers);

	switch (TextureDesc.Dimension)
	{
	case ETextureDimension::Texture1D:
		TextureDesc.Height = 1;
		TextureDesc.Depth = 1;
		TextureDesc.ArrayLayers = 1;
		break;
	case ETextureDimension::Texture1DArray:
		TextureDesc.Height = 1;
		TextureDesc.Depth = 1;
		break;
	case ETextureDimension::Texture3D:
		TextureDesc.ArrayLayers = 1;
		break;
	case ETextureDimension::Cube:
		TextureDesc.Depth = 1;
		TextureDesc.ArrayLayers = 6;
		break;
	case ETextureDimension::CubeArray:
		TextureDesc.Depth = 1;
		if (TextureDesc.ArrayLayers < 6)
		{
			TextureDesc.ArrayLayers = 6;
		}
		if ((TextureDesc.ArrayLayers % 6) != 0)
		{
			TextureDesc.ArrayLayers = Align<uint32_t>(TextureDesc.ArrayLayers, 6);
		}
		break;
	case ETextureDimension::Texture2D:
		TextureDesc.Depth = 1;
		TextureDesc.ArrayLayers = 1;
		break;
	case ETextureDimension::Texture2DArray:
		TextureDesc.Depth = 1;
		break;
	default:
		break;
	}

	if (TextureDesc.Usage == ETextureUsage::None)
	{
		TextureDesc.Usage = isDepthFormat(TextureDesc.Format)
			? ETextureUsage::DepthStencil
			: ETextureUsage::Sampled;
	}

	VkImageUsageFlags usage_flags = toVkImageUsage(TextureDesc.Usage);
	usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (TextureDesc.MipLevels > 1 || TextureDesc.ShouldGenerateMipmaps || hasAnyFlags(TextureDesc.Usage, ETextureUsage::TransferSrc))
	{
		usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.flags = 0;
	image_info.imageType = toVkImageType(TextureDesc.Dimension);
	if (TextureDesc.Dimension == ETextureDimension::Cube || TextureDesc.Dimension == ETextureDimension::CubeArray)
	{
		image_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}
	image_info.format = toVkFormat(TextureDesc.Format);
	image_info.extent = { TextureDesc.Width, TextureDesc.Height, TextureDesc.Depth };
	image_info.mipLevels = TextureDesc.MipLevels;
	image_info.arrayLayers = TextureDesc.ArrayLayers;
	image_info.samples = toVkSampleCount(TextureDesc.SampleCount);
	if (TextureDesc.Dimension == ETextureDimension::Texture3D)
	{
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	}
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.usage = usage_flags;
	image_info.sharingMode = toVkSharingMode(TextureDesc.SharingMode);
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(Device->getVkDevice(), &image_info, nullptr, &ImageResource.Image) != VK_SUCCESS)
	{
		ImageResource.Image = VK_NULL_HANDLE;
		return false;
	}

	ImageResource.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	ImageResource.Ownership = EVulkanImageOwnership::Owned;
	return true;
}

bool VulkanTexture::allocateImageMemory()
{
	if (!Device || ImageResource.Image == VK_NULL_HANDLE)
	{
		return false;
	}

	VkMemoryRequirements memory_requirements{};
	vkGetImageMemoryRequirements(Device->getVkDevice(), ImageResource.Image, &memory_requirements);

	const uint32_t memory_type = Device->findMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (memory_type == UINT32_MAX)
	{
		vkDestroyImage(Device->getVkDevice(), ImageResource.Image, nullptr);
		ImageResource.Image = VK_NULL_HANDLE;
		return false;
	}

	VkMemoryAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = memory_type;

	if (vkAllocateMemory(Device->getVkDevice(), &allocate_info, nullptr, &ImageResource.Memory) != VK_SUCCESS)
	{
		vkDestroyImage(Device->getVkDevice(), ImageResource.Image, nullptr);
		ImageResource.Image = VK_NULL_HANDLE;
		ImageResource.Memory = VK_NULL_HANDLE;
		return false;
	}

	if (vkBindImageMemory(Device->getVkDevice(), ImageResource.Image, ImageResource.Memory, 0) != VK_SUCCESS)
	{
		vkFreeMemory(Device->getVkDevice(), ImageResource.Memory, nullptr);
		vkDestroyImage(Device->getVkDevice(), ImageResource.Image, nullptr);
		ImageResource.Image = VK_NULL_HANDLE;
		ImageResource.Memory = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

bool VulkanTexture::uploadBulkData(const RTextureBulkData& InBulkData)
{
	if (!InBulkData.hasData() || !Device || !isValid())
	{
		return false;
	}

	const uint32_t pixel_size = calPixelSizeFormEFormat(TextureDesc.Format);
	if (pixel_size == 0)
	{
		return false;
	}

	struct UploadChunk
	{
		const uint8_t* Data = nullptr;
		uint32_t RowPitch = 0;
		uint32_t SlicePitch = 0;
		VkExtent3D Extent{};
		VkBufferImageCopy Region{};
	};

	std::vector<UploadChunk> chunks;
	chunks.reserve(static_cast<size_t>(TextureDesc.MipLevels) * TextureDesc.ArrayLayers);
	uint64_t total_size = 0;

	for (uint32_t mip = 0; mip < TextureDesc.MipLevels; ++mip)
	{
		const VkExtent3D mip_extent = getMipExtent(TextureDesc, mip);
		for (uint32_t layer = 0; layer < TextureDesc.ArrayLayers; ++layer)
		{
			TextureHelper::SubresourceInfo subresource = TextureHelper::getSubresourceData(TextureDesc, &InBulkData, mip, layer);
			if (!subresource.Data)
			{
				return false;
			}

			const uint32_t row_pitch = subresource.RowPitch != 0 ? subresource.RowPitch : mip_extent.width * pixel_size;
			const uint32_t slice_pitch = subresource.SlicePitch != 0 ? subresource.SlicePitch : row_pitch * mip_extent.height;
			if (row_pitch < mip_extent.width * pixel_size || (row_pitch % pixel_size) != 0)
			{
				return false;
			}

			UploadChunk chunk{};
			chunk.Data = static_cast<const uint8_t*>(subresource.Data);
			chunk.RowPitch = row_pitch;
			chunk.SlicePitch = slice_pitch;
			chunk.Extent = mip_extent;
			chunk.Region.bufferOffset = total_size;
			chunk.Region.bufferRowLength = row_pitch / pixel_size;
			chunk.Region.bufferImageHeight = mip_extent.height;
			chunk.Region.imageSubresource.aspectMask = getAspectMask();
			chunk.Region.imageSubresource.mipLevel = mip;
			chunk.Region.imageSubresource.baseArrayLayer = layer;
			chunk.Region.imageSubresource.layerCount = 1;
			chunk.Region.imageOffset = { 0, 0, 0 };
			chunk.Region.imageExtent = mip_extent;

			const uint64_t chunk_size = static_cast<uint64_t>(slice_pitch) * mip_extent.depth;
			total_size += chunk_size;
			chunks.push_back(chunk);
		}
	}

	auto* staging_buffer = static_cast<VulkanBuffer*>(Device->createStagingBuffer(nullptr, total_size));
	if (!staging_buffer || !staging_buffer->isValid())
	{
		Device->destroyResource(staging_buffer);
		return false;
	}

	uint8_t* mapped_ptr = static_cast<uint8_t*>(staging_buffer->mapRange(EBufferMapMode::Write, 0, total_size));
	if (!mapped_ptr)
	{
		Device->destroyResource(staging_buffer);
		return false;
	}

	for (const UploadChunk& chunk : chunks)
	{
		for (uint32_t z = 0; z < chunk.Extent.depth; ++z)
		{
			const uint8_t* src_slice = chunk.Data + static_cast<size_t>(z) * chunk.SlicePitch;
			uint8_t* dst_slice = mapped_ptr + chunk.Region.bufferOffset + static_cast<size_t>(z) * chunk.SlicePitch;
			for (uint32_t y = 0; y < chunk.Extent.height; ++y)
			{
				std::memcpy(
					dst_slice + static_cast<size_t>(y) * chunk.RowPitch,
					src_slice + static_cast<size_t>(y) * chunk.RowPitch,
					static_cast<size_t>(chunk.Extent.width) * pixel_size);
			}
		}
	}

	staging_buffer->flushMappedRange(0, total_size);
	staging_buffer->unmap();

	std::vector<VkBufferImageCopy> copy_regions;
	copy_regions.reserve(chunks.size());
	for (const UploadChunk& chunk : chunks)
	{
		copy_regions.push_back(chunk.Region);
	}

	const VkImageLayout preferred_layout = getPreferredImageLayout(TextureDesc);
	const bool will_generate_mips = TextureDesc.ShouldGenerateMipmaps && TextureDesc.MipLevels > 1;

	VkCommandBuffer command_buffer = Device->beginImmediateCommand();
	transitionImageLayout(
		command_buffer,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		getPipelineStageForLayout(ImageResource.Layout),
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		getAccessMaskForLayout(ImageResource.Layout),
		VK_ACCESS_TRANSFER_WRITE_BIT);

	vkCmdCopyBufferToImage(
		command_buffer,
		staging_buffer->getVkBuffer(),
		ImageResource.Image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(copy_regions.size()),
		copy_regions.data());

	if (!will_generate_mips && preferred_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		transitionImageLayout(
			command_buffer,
			preferred_layout,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			getPipelineStageForLayout(preferred_layout),
			VK_ACCESS_TRANSFER_WRITE_BIT,
			getAccessMaskForLayout(preferred_layout));
	}

	Device->endImmediateCommand(command_buffer);
	Device->destroyResource(staging_buffer);
	return true;
}

bool VulkanTexture::transitionImageLayout(
	VkCommandBuffer CommandBuffer,
	VkImageLayout NewLayout,
	VkPipelineStageFlags SrcStage,
	VkPipelineStageFlags DstStage,
	VkAccessFlags SrcAccess,
	VkAccessFlags DstAccess,
	const VkImageSubresourceRange* SubresourceRange)
{
	if (!isValid() || CommandBuffer == VK_NULL_HANDLE)
	{
		return false;
	}
	if (ImageResource.Layout == NewLayout)
	{
		return true;
	}

	const VkImageLayout old_layout = ImageResource.Layout;
	VkImageSubresourceRange range = SubresourceRange
		? *SubresourceRange
		: buildSubresourceRange(0, TextureDesc.MipLevels, 0, TextureDesc.ArrayLayers);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = NewLayout;
	barrier.srcAccessMask = old_layout == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : SrcAccess;
	barrier.dstAccessMask = DstAccess;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = ImageResource.Image;
	barrier.subresourceRange = range;

	vkCmdPipelineBarrier(
		CommandBuffer,
		old_layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : SrcStage,
		DstStage,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier);

	ImageResource.Layout = NewLayout;
	return true;
}

VkImageSubresourceRange VulkanTexture::buildSubresourceRange(
	uint32_t BaseMipLevel,
	uint32_t MipLevelCount,
	uint32_t BaseArrayLayer,
	uint32_t ArrayLayerCount) const
{
	VkImageSubresourceRange range{};
	range.aspectMask = getAspectMask();
	range.baseMipLevel = std::min(BaseMipLevel, TextureDesc.MipLevels - 1);
	range.baseArrayLayer = std::min(BaseArrayLayer, TextureDesc.ArrayLayers - 1);

	const uint32_t available_mips = TextureDesc.MipLevels - range.baseMipLevel;
	const uint32_t available_layers = TextureDesc.ArrayLayers - range.baseArrayLayer;
	range.levelCount = std::min(available_mips, std::max(1u, MipLevelCount));
	range.layerCount = std::min(available_layers, std::max(1u, ArrayLayerCount));
	return range;
}

RTextureViewDescriptor VulkanTexture::buildDefaultViewDescriptor() const
{
	RTextureViewDescriptor descriptor{};
	descriptor.Type = getDefaultViewType(TextureDesc);
	descriptor.Aspect = ETextureAspect::Auto;
	descriptor.Format = TextureDesc.Format;
	descriptor.MipLevel = 0;
	descriptor.MipLevelCount = TextureDesc.MipLevels;
	descriptor.ArrayLayer = 0;
	descriptor.ArrayLayerCount = TextureDesc.ArrayLayers;
	descriptor.Dimension = TextureDesc.Dimension;
	return descriptor;
}

void VulkanTexture::setDebugName(const std::string& Name)
{
	(void)Name;
}

VulkanTextureView::VulkanTextureView(VulkanTexture* InTexture, const RTextureViewDescriptor& InDescriptor, VkImageView InView, bool bInOwnsView)
	: Texture(InTexture)
	, Descriptor(InDescriptor)
	, View(InView)
	, OwnsView(bInOwnsView)
{
}

VulkanTextureView::~VulkanTextureView()
{
	if (!OwnsView || View == VK_NULL_HANDLE || !Texture || !Texture->getDevice())
	{
		return;
	}

	VkDevice vk_device = Texture->getDevice()->getVkDevice();
	if (vk_device == VK_NULL_HANDLE)
	{
		return;
	}

	vkDestroyImageView(vk_device, View, nullptr);
	View = VK_NULL_HANDLE;
}

bool VulkanTextureView::isValid() const
{
	return View != VK_NULL_HANDLE;
}

void VulkanTextureView::setDebugName(const std::string& Name)
{
	(void)Name;
}

} // namespace render::rhi
