#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "VulkanRHI.h"
#include "VulkanResources.h"
#include "VulkanDevice.h"

namespace render::rhi
{

VulkanBuffer::VulkanBuffer(VulkanDevice* InDevice, const RBufferDescriptor& InBufferDesc, VkBuffer InBuffer, VmaAllocation InAlloc)
	: Device(InDevice)
	, BufferDesc(InBufferDesc)
	, Buffer(InBuffer)
	, Alloc(InAlloc)
{
}

VulkanBuffer::~VulkanBuffer()
{
	const auto allocator = Device ? reinterpret_cast<VmaAllocator>(Device->getAllocatorHandle()) : VmaAllocator{};
	if (allocator != VmaAllocator{} && Buffer != VkBuffer{})
	{
		vmaDestroyBuffer(allocator, Buffer, Alloc);
	}
}

void VulkanBuffer::SetDebugName(const std::string& Name)
{
	(void)Name;
}

VulkanTexture::VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, VkImage InImage, VmaAllocation InAlloc)
	: Device(InDevice)
	, TextureDesc(InTextureDesc)
	, Image(InImage)
	, Alloc(InAlloc)
{
}

VulkanTexture::~VulkanTexture()
{
	delete TextureView;
	TextureView = nullptr;

	const auto allocator = Device ? reinterpret_cast<VmaAllocator>(Device->getAllocatorHandle()) : VmaAllocator{};
	if (allocator != VmaAllocator{} && Image != VkImage{})
	{
		vmaDestroyImage(allocator, Image, Alloc);
	}
}

RTextureView* VulkanTexture::createView(const RTextureViewDescriptor& Descriptor)
{
	if (!TextureView)
	{
		TextureView = new VulkanTextureView(this, Descriptor);
	}
	return TextureView;
}

void VulkanTexture::SetDebugName(const std::string& Name)
{
	(void)Name;
}

VulkanTextureView::VulkanTextureView(VulkanTexture* InTexture, const RTextureViewDescriptor& InDescriptor)
	: Texture(InTexture)
	, Descriptor(InDescriptor)
{
}

VulkanTextureView::~VulkanTextureView() = default;

void VulkanTextureView::SetDebugName(const std::string& Name)
{
	(void)Name;
}

VulkanSampler::VulkanSampler(VulkanDevice* InDevice, const RSamplerDescriptor& InSamplerDesc)
	: Device(InDevice)
	, SamplerDesc(InSamplerDesc)
{
}

VulkanSampler::~VulkanSampler()
{
	const auto vkDevice = Device ? reinterpret_cast<VkDevice>(Device->getDeviceHandle()) : VkDevice{};
	if (vkDevice != VkDevice{} && Sampler != VkSampler{})
	{
		vkDestroySampler(vkDevice, Sampler, nullptr);
	}
}

void VulkanSampler::SetDebugName(const std::string& Name)
{
	(void)Name;
}

} // namespace render::rhi
