#include "VulkanCommandList.h"
#include "VulkanDescriptorSet.h"
#include "VulkanFence.h"
#include "VulkanPipeline.h"
#include "VulkanRHI.h"
#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "VulkanShader.h"
#include "VulkanSwapchain.h"

#include <algorithm>
#include <array>
#include <set>
#include <vector>


namespace render::rhi
{

namespace
{

VkImageType ToVkImageTypeFormDescriptor(const RTextureDescriptor& InTextureDesc)
{
	if (InTextureDesc.Depth > 1)
	{
		return VK_IMAGE_TYPE_3D;
	}
	else if (InTextureDesc.Height > 1)
	{
		return VK_IMAGE_TYPE_2D;
	}
	else
	{
		return VK_IMAGE_TYPE_1D;
	}
}
VkImageViewType ToVkImageViewType(RTextureViewDescriptor::EViewType)
{
	return VK_IMAGE_VIEW_TYPE_2D;
}



} // namespace


VulkanTexture::VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, const RTextureBulkData& InBulkData)
	: Device(InDevice)
	, TextureDesc(InTextureDesc)
	, OwnsImage(true)
	, BulkData(InBulkData)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	VkDevice vkDevice = Device->getVkDevice();
	VkPhysicalDevice vkPhysicalDevice = Device->getVkPhysicalDevice();

	VkImageCreateInfo image_info {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.flags = 0;
	image_info.imageType = ToVkImageTypeFormDescriptor(InTextureDesc);
	image_info.format = ToVkFormat(InTextureDesc.Format);
	image_info.extent = {
		InTextureDesc.Width,
		InTextureDesc.Height,
		InTextureDesc.Depth
	};

	image_info.mipLevels = InTextureDesc.MipLevels;
	image_info.arrayLayers = InTextureDesc.ArrayLayers;
	image_info.samples = ToVkSampleCount(InTextureDesc.SampleCount);
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.sharingMode = (InTextureDesc.SharingMode == ESharingMode::Exclusive) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	image_info.usage = ToVkImageUsage(InTextureDesc.Usage);
	if (InBulkData.hasData())
	{
		image_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	VkResult result = vkCreateImage(vkDevice, &image_info, nullptr, &Image);
	if (result != VK_SUCCESS)
	{
		// TODO: 错误处理
		Image = VK_NULL_HANDLE;
		return;
	}

	VkMemoryRequirements memory_requirements{};
	vkGetImageMemoryRequirements(vkDevice, Image, &memory_requirements);

	uint32_t memory_type_index = Device->findMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = memory_type_index;

	result = vkAllocateMemory(vkDevice, &allocate_info, nullptr, &Memory);
	if (result != VK_SUCCESS)
	{
		// TODO: 错误处理
		vkDestroyImage(vkDevice, Image, nullptr);
		Image = VK_NULL_HANDLE;
		return;
	}

	if (InBulkData.hasData())
	{
		// 计算 staging buffer 大小和子资源布局
		std::vector<VkBufferImageCopy> copy_regions;
		VkDeviceSize total_buffer_size = 0;
		uint32_t pixel_size = CalPixelSizeFormEFormat(InTextureDesc.Format);

		// 遍历所有 mip 和 array layer，计算每个子资源的偏移和复制区域
		for (uint32_t mip = 0; mip < InTextureDesc.MipLevels; ++mip)
		{
			uint32_t mip_width = std::max(1u, InTextureDesc.Width >> mip);
			uint32_t mip_height = std::max(1u, InTextureDesc.Height >> mip);
			uint32_t mip_depth = (InTextureDesc.Depth > 0) ? std::max(1u, InTextureDesc.Depth >> mip) : 1;

			for (uint32_t layer = 0; layer < InTextureDesc.ArrayLayers; ++layer)
			{
				TextureHelper::SubresourceInfo subresource = TextureHelper::getSubresourceData(InTextureDesc, &InBulkData, mip, layer);

				VkBufferImageCopy region{};
				region.bufferOffset = total_buffer_size;
				region.bufferRowLength = subresource.RowPitch / pixel_size;
				region.bufferImageHeight = mip_height;
				region.imageSubresource.aspectMask = AspectMask(RTextureViewDescriptor::EViewType::SRV, InTextureDesc.Format);
				region.imageSubresource.mipLevel = mip;
				region.imageSubresource.baseArrayLayer = layer;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { mip_width, mip_height, mip_depth };

				copy_regions.push_back(region);

				uint32_t sub_size = (mip_depth > 0 ? mip_depth : 1) * subresource.SlicePitch;
				total_buffer_size += sub_size;
			}
		}
		// 创建 staging buffer 并上传数据
		VkBuffer staging_buffer;
		VkDeviceMemory staging_memory;

		Device->createStagingBuffer(staging_buffer, staging_memory, total_buffer_size);

		void* mapped_data = nullptr;
		vkMapMemory(vkDevice, staging_memory, 0, total_buffer_size, 0, &mapped_data);
		uint8_t* dst_ptr = static_cast<uint8_t*>(mapped_data);

		for (size_t i = 0; i < copy_regions.size(); ++i)
		{
			const VkBufferImageCopy& region = copy_regions[i];
			TextureHelper::SubresourceInfo subresource = TextureHelper::getSubresourceData(
				InTextureDesc, 
				&InBulkData, 
				region.imageSubresource.mipLevel, 
				region.imageSubresource.baseArrayLayer
			);
			
			const uint8_t* src_ptr = static_cast<const uint8_t*>(subresource.Data);
			uint32_t row_pitch = subresource.RowPitch;
			uint32_t slice_pitch = subresource.SlicePitch;
			uint32_t mip_width = region.imageExtent.width;
			uint32_t mip_height = region.imageExtent.height;
			uint32_t mip_depth = region.imageExtent.depth;

			for (uint32_t z = 0; z < mip_depth; ++z)
			{
				uint8_t* dst_slice = dst_ptr + region.bufferOffset + z * slice_pitch;
				const uint8_t* src_slice = src_ptr + z * slice_pitch;
				for (uint32_t y = 0; y < mip_height; ++y)
				{
					std::memcpy(dst_slice + y * row_pitch, src_slice + y * row_pitch, mip_width * pixel_size);
				}
			}
		}

		vkUnmapMemory(vkDevice, staging_memory);

		// 记录一次立即执行的拷贝命令
		VkCommandBuffer cmd_buffer = Device->beginImmediateCommand();

        // 图像布局：UNDEFINED -> TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = Image;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange.aspectMask = GetAspectMask(TextureDesc.Format);
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = TextureDesc.MipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = TextureDesc.ArrayLayers;

        vkCmdPipelineBarrier(cmd_buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        vkCmdCopyBufferToImage(cmd_buffer, staging_buffer, Image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               static_cast<uint32_t>(copy_regions.size()), copy_regions.data());

        // 转换布局为最终的着色器只读（假设纹理主要用于采样）
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(cmd_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        Device->endImmediateCommand(cmd_buffer);   // 提交并等待

        // 销毁 staging buffer
        vkDestroyBuffer(vkDevice, staging_buffer, nullptr);
        vkFreeMemory(vkDevice, staging_memory, nullptr);
	} 
	else 
	{
		// 无目标原始数据
	}

	RTextureViewDescriptor default_view_desc;
    default_view_desc.Format = TextureDesc.Format;
    default_view_desc.Dimension = ETextureDimension::Texture2D; // 根据实际调整
    default_view_desc.MipLevel = TextureDesc.MipLevels;
    default_view_desc.ArrayLayer = TextureDesc.ArrayLayers;
    DefaultView = static_cast<VulkanTextureView*>(createView(default_view_desc));

	if (!TextureDesc.Name.empty()) {
        setDebugName(TextureDesc.Name);
    }
}

VulkanTexture::VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc)
	: Device(InDevice)
	, TextureDesc(InTextureDesc)
	, OwnsImage(true)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent = {
		InTextureDesc.Width,
		InTextureDesc.Height,
		InTextureDesc.Depth
	};
	image_info.mipLevels = TextureDesc.MipLevels;
	image_info.arrayLayers = TextureDesc.ArrayLayers;
	image_info.format = ToVkFormat(TextureDesc.Format);
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = ToVkImageUsage(TextureDesc.Usage);
	if (image_info.usage == 0)
	{
		image_info.usage = IsDepthFormat(TextureDesc.Format)
			? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
			: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	image_info.samples = ToVkSampleCount(TextureDesc.SampleCount);
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkDevice vkDevice = Device->getVkDevice();
	if (vkCreateImage(vkDevice, &image_info, nullptr, &Image) != VK_SUCCESS)
	{
		Image = VK_NULL_HANDLE;
		return;
	}

	VkMemoryRequirements memory_requirements{};
	vkGetImageMemoryRequirements(vkDevice, Image, &memory_requirements);

	const uint32_t memoryType = Device->findMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (memoryType == UINT32_MAX)
	{
		vkDestroyImage(vkDevice, Image, nullptr);
		Image = VK_NULL_HANDLE;
		return;
	}

	VkMemoryAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = memoryType;

	if (vkAllocateMemory(vkDevice, &allocate_info, nullptr, &Memory) != VK_SUCCESS)
	{
		vkDestroyImage(vkDevice, Image, nullptr);
		Image = VK_NULL_HANDLE;
		Memory = VK_NULL_HANDLE;
		return;
	}

	vkBindImageMemory(vkDevice, Image, Memory, 0);

	RTextureViewDescriptor view_descriptor{};
	view_descriptor.Type = IsDepthFormat(TextureDesc.Format)
		? RTextureViewDescriptor::EViewType::DSV
		: RTextureViewDescriptor::EViewType::RTV;
	view_descriptor.Format = TextureDesc.Format;
	DefaultView = createView(view_descriptor);
}

VulkanTexture::VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, VkImage InExternalImage, VkImageView InExternalView)
	: Device(InDevice)
	, TextureDesc(InTextureDesc)
	, Image(InExternalImage)
	, OwnsImage(false)
{
	RTextureViewDescriptor Descriptor{};
	Descriptor.Type = IsDepthFormat(TextureDesc.Format)
		? RTextureViewDescriptor::EViewType::DSV
		: RTextureViewDescriptor::EViewType::RTV;
	Descriptor.Format = TextureDesc.Format;

	DefaultView = new VulkanTextureView(this, Descriptor, InExternalView, true);
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

	VkDevice vkDevice = Device->getVkDevice();
	if (vkDevice == VK_NULL_HANDLE)
	{
		return;
	}

	if (OwnsImage && Image != VK_NULL_HANDLE)
	{
		vkDestroyImage(vkDevice, Image, nullptr);
		Image = VK_NULL_HANDLE;
	}

	if (OwnsImage && Memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vkDevice, Memory, nullptr);
		Memory = VK_NULL_HANDLE;
	}
}

bool VulkanTexture::isValid() const
{
	return Image != VK_NULL_HANDLE;
}

RTextureView* VulkanTexture::createView(const RTextureViewDescriptor& Descriptor)
{
	if (!Device || !isValid())
	{
		return nullptr;
	}

	for (RTextureView* existing_view_base : Views)
	{
		auto* existing_view = dynamic_cast<VulkanTextureView*>(existing_view_base);
		if (!existing_view)
		{
			continue;
		}

		const RTextureViewDescriptor existing_descriptor = existing_view->getDescriptor();
		const EFormat request_format = Descriptor.Format == EFormat::Undefined ? TextureDesc.Format : Descriptor.Format;
		const EFormat existing_format = existing_descriptor.Format == EFormat::Undefined ? TextureDesc.Format : existing_descriptor.Format;

		if (existing_descriptor.Type == Descriptor.Type &&
			existing_format == request_format &&
			existing_descriptor.MipLevel == Descriptor.MipLevel &&
			existing_descriptor.MipLevelCount == Descriptor.MipLevelCount &&
			existing_descriptor.ArrayLayer == Descriptor.ArrayLayer &&
			existing_descriptor.ArrayLayerCount == Descriptor.ArrayLayerCount)
		{
			return existing_view;
		}
	}

	VkImageViewCreateInfo view_info{};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = Image;
	view_info.viewType = ToVkImageViewType(Descriptor.Type);
	view_info.format = ToVkFormat(Descriptor.Format == EFormat::Undefined ? TextureDesc.Format : Descriptor.Format);
	view_info.subresourceRange.aspectMask = AspectMask(Descriptor.Type, TextureDesc.Format);
	view_info.subresourceRange.baseMipLevel = Descriptor.MipLevel;
	view_info.subresourceRange.levelCount = Descriptor.MipLevelCount;
	view_info.subresourceRange.baseArrayLayer = Descriptor.ArrayLayer;
	view_info.subresourceRange.layerCount = Descriptor.ArrayLayerCount;

	VkImageView image_view = VK_NULL_HANDLE;
	if (vkCreateImageView(Device->getVkDevice(), &view_info, nullptr, &image_view) != VK_SUCCESS)
	{
		return nullptr;
	}

	auto* view = new VulkanTextureView(this, Descriptor, image_view, true);
	Views.push_back(view);
	if (!DefaultView)
	{
		DefaultView = view;
	}
	return view;
}

VkImageAspectFlags VulkanTexture::getAspectMask() const noexcept
{
	if (TextureDesc.Format == EFormat::D24_UNorm_S8_UInt)
	{
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	if (TextureDesc.Format == EFormat::D32_Float)
	{
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	return VK_IMAGE_ASPECT_COLOR_BIT;
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

	vkDestroyImageView(Texture->getDevice()->getVkDevice(), View, nullptr);
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

}