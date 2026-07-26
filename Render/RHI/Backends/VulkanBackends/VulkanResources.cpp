#include "VulkanResources.h"

#include "VulkanDevice.h"
#include "VulkanRHI.h"

#include <algorithm>
#include <cstring>

namespace render::rhi
{

namespace
{

template <typename EnumT>
bool HasFlag(EnumT value, EnumT flag)
{
	using UIntT = std::underlying_type_t<EnumT>;
	return (static_cast<UIntT>(value) & static_cast<UIntT>(flag)) != 0;
}

VkImageViewType ToVkImageViewType(RTextureViewDescriptor::EViewType)
{
	return VK_IMAGE_VIEW_TYPE_2D;
}

VkImageAspectFlags AspectMaskFromDescriptor(const RTextureViewDescriptor& descriptor, EFormat textureFormat)
{
	if (descriptor.Type == RTextureViewDescriptor::EViewType::DSV)
	{
		if (textureFormat == EFormat::D24_UNorm_S8_UInt)
		{
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	if (IsDepthFormat(textureFormat))
	{
		if (textureFormat == EFormat::D24_UNorm_S8_UInt)
		{
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	return VK_IMAGE_ASPECT_COLOR_BIT;
}

} // namespace

VulkanBuffer::VulkanBuffer(VulkanDevice* InDevice, const RBufferDescriptor& InBufferDesc)
	: Device(InDevice)
	, BufferDesc(InBufferDesc)
{
	if (!Device || !Device->isValid() || BufferDesc.Size == 0)
	{
		return;
	}

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = BufferDesc.Size;
	bufferInfo.usage = ToVkBufferUsage(BufferDesc.Usage);
	if (bufferInfo.usage == 0)
	{
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkDevice vkDevice = Device->getVkDevice();
	if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &Buffer) != VK_SUCCESS)
	{
		Buffer = VK_NULL_HANDLE;
		return;
	}

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vkDevice, Buffer, &memoryRequirements);

	const bool needsHostVisible = BufferDesc.IsCpuVisible || BufferDesc.InitialData != nullptr;
	const VkMemoryPropertyFlags memoryFlags = needsHostVisible
		? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		: VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	const uint32_t memoryType = Device->findMemoryType(memoryRequirements.memoryTypeBits, memoryFlags);
	if (memoryType == UINT32_MAX)
	{
		vkDestroyBuffer(vkDevice, Buffer, nullptr);
		Buffer = VK_NULL_HANDLE;
		return;
	}

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryType;

	if (vkAllocateMemory(vkDevice, &allocateInfo, nullptr, &Memory) != VK_SUCCESS)
	{
		vkDestroyBuffer(vkDevice, Buffer, nullptr);
		Buffer = VK_NULL_HANDLE;
		Memory = VK_NULL_HANDLE;
		return;
	}

	vkBindBufferMemory(vkDevice, Buffer, Memory, 0);

	if (BufferDesc.InitialData && BufferDesc.InitialDataSize > 0)
	{
		void* mappedData = nullptr;
		if (vkMapMemory(vkDevice, Memory, 0, BufferDesc.InitialDataSize, 0, &mappedData) == VK_SUCCESS)
		{
			const uint32_t copySize = std::min(BufferDesc.Size, BufferDesc.InitialDataSize);
			std::memcpy(mappedData, BufferDesc.InitialData, copySize);
			vkUnmapMemory(vkDevice, Memory);
		}
	}
}

VulkanBuffer::~VulkanBuffer()
{
	VkDevice vkDevice = Device ? Device->getVkDevice() : VK_NULL_HANDLE;
	if (vkDevice == VK_NULL_HANDLE)
	{
		return;
	}

	if (Buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(vkDevice, Buffer, nullptr);
		Buffer = VK_NULL_HANDLE;
	}

	if (Memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vkDevice, Memory, nullptr);
		Memory = VK_NULL_HANDLE;
	}
}

bool VulkanBuffer::isValid() const
{
	return Buffer != VK_NULL_HANDLE;
}

uint64_t VulkanBuffer::getSize() const
{
	return BufferDesc.Size;
}

void VulkanBuffer::SetDebugName(const std::string& Name)
{
	(void)Name;
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

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = TextureDesc.Width;
	imageInfo.extent.height = TextureDesc.Height;
	imageInfo.extent.depth = TextureDesc.Depth;
	imageInfo.mipLevels = TextureDesc.MipLevels;
	imageInfo.arrayLayers = TextureDesc.ArrayLayers;
	imageInfo.format = ToVkFormat(TextureDesc.Format);
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = ToVkImageUsage(TextureDesc.Usage);
	if (imageInfo.usage == 0)
	{
		imageInfo.usage = IsDepthFormat(TextureDesc.Format)
			? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
			: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	imageInfo.samples = ToVkSampleCount(TextureDesc.SampleCount);
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkDevice vkDevice = Device->getVkDevice();
	if (vkCreateImage(vkDevice, &imageInfo, nullptr, &Image) != VK_SUCCESS)
	{
		Image = VK_NULL_HANDLE;
		return;
	}

	VkMemoryRequirements memoryRequirements{};
	vkGetImageMemoryRequirements(vkDevice, Image, &memoryRequirements);

	const uint32_t memoryType = Device->findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (memoryType == UINT32_MAX)
	{
		vkDestroyImage(vkDevice, Image, nullptr);
		Image = VK_NULL_HANDLE;
		return;
	}

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryType;

	if (vkAllocateMemory(vkDevice, &allocateInfo, nullptr, &Memory) != VK_SUCCESS)
	{
		vkDestroyImage(vkDevice, Image, nullptr);
		Image = VK_NULL_HANDLE;
		Memory = VK_NULL_HANDLE;
		return;
	}

	vkBindImageMemory(vkDevice, Image, Memory, 0);

	RTextureViewDescriptor viewDescriptor{};
	viewDescriptor.Type = IsDepthFormat(TextureDesc.Format)
		? RTextureViewDescriptor::EViewType::DSV
		: RTextureViewDescriptor::EViewType::RTV;
	viewDescriptor.Format = TextureDesc.Format;
	DefaultView = createView(viewDescriptor);
}

VulkanTexture::VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, VkImage InExternalImage, VkImageView InExternalView)
	: Device(InDevice)
	, TextureDesc(InTextureDesc)
	, Image(InExternalImage)
	, OwnsImage(false)
{
	RTextureViewDescriptor descriptor{};
	descriptor.Type = IsDepthFormat(TextureDesc.Format)
		? RTextureViewDescriptor::EViewType::DSV
		: RTextureViewDescriptor::EViewType::RTV;
	descriptor.Format = TextureDesc.Format;

	DefaultView = new VulkanTextureView(this, descriptor, InExternalView, true);
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

	for (RTextureView* existingViewBase : Views)
	{
		auto* existingView = dynamic_cast<VulkanTextureView*>(existingViewBase);
		if (!existingView)
		{
			continue;
		}

		const RTextureViewDescriptor existingDescriptor = existingView->getDescriptor();
		const EFormat requestFormat = Descriptor.Format == EFormat::Undefined ? TextureDesc.Format : Descriptor.Format;
		const EFormat existingFormat = existingDescriptor.Format == EFormat::Undefined ? TextureDesc.Format : existingDescriptor.Format;

		if (existingDescriptor.Type == Descriptor.Type &&
			existingFormat == requestFormat &&
			existingDescriptor.BaseMipLevel == Descriptor.BaseMipLevel &&
			existingDescriptor.MipLevelCount == Descriptor.MipLevelCount &&
			existingDescriptor.BaseArrayLayer == Descriptor.BaseArrayLayer &&
			existingDescriptor.ArrayLayerCount == Descriptor.ArrayLayerCount)
		{
			return existingView;
		}
	}

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = Image;
	viewInfo.viewType = ToVkImageViewType(Descriptor.Type);
	viewInfo.format = ToVkFormat(Descriptor.Format == EFormat::Undefined ? TextureDesc.Format : Descriptor.Format);
	viewInfo.subresourceRange.aspectMask = AspectMaskFromDescriptor(Descriptor, TextureDesc.Format);
	viewInfo.subresourceRange.baseMipLevel = Descriptor.BaseMipLevel;
	viewInfo.subresourceRange.levelCount = Descriptor.MipLevelCount;
	viewInfo.subresourceRange.baseArrayLayer = Descriptor.BaseArrayLayer;
	viewInfo.subresourceRange.layerCount = Descriptor.ArrayLayerCount;

	VkImageView imageView = VK_NULL_HANDLE;
	if (vkCreateImageView(Device->getVkDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		return nullptr;
	}

	auto* view = new VulkanTextureView(this, Descriptor, imageView, true);
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

void VulkanTexture::SetDebugName(const std::string& Name)
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

void VulkanTextureView::SetDebugName(const std::string& Name)
{
	(void)Name;
}

VulkanSampler::VulkanSampler(VulkanDevice* InDevice, const RSamplerDescriptor& InSamplerDesc)
	: Device(InDevice)
	, SamplerDesc(InSamplerDesc)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.minFilter = SamplerDesc.MinFilter == RSamplerDescriptor::EFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	samplerInfo.magFilter = SamplerDesc.MagFilter == RSamplerDescriptor::EFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = SamplerDesc.MipFilter == RSamplerDescriptor::EFilter::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	vkCreateSampler(Device->getVkDevice(), &samplerInfo, nullptr, &Sampler);
}

VulkanSampler::~VulkanSampler()
{
	if (!Device)
	{
		return;
	}

	VkDevice vkDevice = Device->getVkDevice();
	if (vkDevice != VK_NULL_HANDLE && Sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(vkDevice, Sampler, nullptr);
		Sampler = VK_NULL_HANDLE;
	}
}

bool VulkanSampler::isValid() const
{
	return Sampler != VK_NULL_HANDLE;
}

void VulkanSampler::SetDebugName(const std::string& Name)
{
	(void)Name;
}

} // namespace render::rhi
