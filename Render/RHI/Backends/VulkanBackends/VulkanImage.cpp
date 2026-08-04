#include "VulkanImage.h"

#include "VulkanDevice.h"

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

} // namespace

VulkanImage::VulkanImage(VulkanDevice* InDevice, const RTextureDescriptor& InDescriptor)
	: Device(InDevice)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	if (!createImage(InDescriptor) || !allocateImageMemory())
	{
		if (Image != VK_NULL_HANDLE)
		{
			vkDestroyImage(Device->getVkDevice(), Image, nullptr);
			Image = VK_NULL_HANDLE;
		}
		if (Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(Device->getVkDevice(), Memory, nullptr);
			Memory = VK_NULL_HANDLE;
		}
		return;
	}

	Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	Ownership = EVulkanImageOwnership::Owned;
}

VulkanImage::VulkanImage(VulkanDevice* InDevice, VkImage InExternalImage, VkImageLayout InInitialLayout)
	: Device(InDevice)
	, Image(InExternalImage)
	, Layout(InInitialLayout)
	, Ownership(EVulkanImageOwnership::External)
{
}

VulkanImage::~VulkanImage()
{
	VkDevice vk_device = Device ? Device->getVkDevice() : VK_NULL_HANDLE;
	if (vk_device == VK_NULL_HANDLE)
	{
		return;
	}

	if (ownsImage() && Image != VK_NULL_HANDLE)
	{
		vkDestroyImage(vk_device, Image, nullptr);
		Image = VK_NULL_HANDLE;
	}

	if (ownsMemory() && Memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vk_device, Memory, nullptr);
		Memory = VK_NULL_HANDLE;
	}
}

bool VulkanImage::isValid() const noexcept
{
	return Image != VK_NULL_HANDLE;
}

bool VulkanImage::ownsImage() const noexcept
{
	return Ownership == EVulkanImageOwnership::Owned;
}

bool VulkanImage::ownsMemory() const noexcept
{
	return Ownership == EVulkanImageOwnership::Owned && Memory != VK_NULL_HANDLE;
}

bool VulkanImage::createImage(const RTextureDescriptor& Descriptor)
{
	VkImageUsageFlags usage_flags = toVkImageUsage(Descriptor.Usage);
	usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (Descriptor.MipLevels > 1 || Descriptor.ShouldGenerateMipmaps || hasAnyFlags(Descriptor.Usage, ETextureUsage::TransferSrc))
	{
		usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.flags = 0;
	image_info.imageType = toVkImageType(Descriptor.Dimension);
	if (Descriptor.Dimension == ETextureDimension::Cube || Descriptor.Dimension == ETextureDimension::CubeArray)
	{
		image_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}
	image_info.format = toVkFormat(Descriptor.Format);
	image_info.extent = { Descriptor.Width, Descriptor.Height, Descriptor.Depth };
	image_info.mipLevels = Descriptor.MipLevels;
	image_info.arrayLayers = Descriptor.ArrayLayers;
	image_info.samples = toVkSampleCount(Descriptor.SampleCount);
	if (Descriptor.Dimension == ETextureDimension::Texture3D)
	{
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	}
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.usage = usage_flags;
	image_info.sharingMode = toVkSharingMode(Descriptor.SharingMode);
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(Device->getVkDevice(), &image_info, nullptr, &Image) != VK_SUCCESS)
	{
		Image = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

bool VulkanImage::allocateImageMemory()
{
	if (Image == VK_NULL_HANDLE)
	{
		return false;
	}

	VkMemoryRequirements memory_requirements{};
	vkGetImageMemoryRequirements(Device->getVkDevice(), Image, &memory_requirements);

	const uint32_t memory_type = Device->findMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (memory_type == UINT32_MAX)
	{
		return false;
	}

	VkMemoryAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = memory_type;

	if (vkAllocateMemory(Device->getVkDevice(), &allocate_info, nullptr, &Memory) != VK_SUCCESS)
	{
		Memory = VK_NULL_HANDLE;
		return false;
	}

	if (vkBindImageMemory(Device->getVkDevice(), Image, Memory, 0) != VK_SUCCESS)
	{
		vkFreeMemory(Device->getVkDevice(), Memory, nullptr);
		Memory = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

} // namespace render::rhi
