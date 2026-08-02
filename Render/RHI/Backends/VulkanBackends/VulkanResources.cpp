#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "VulkanRHI.h"

#include <algorithm>
#include <cstring>

namespace render::rhi
{



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

void VulkanBuffer::setDebugName(const std::string& Name)
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

	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.minFilter = SamplerDesc.MinFilter == RSamplerDescriptor::EFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	sampler_info.magFilter = SamplerDesc.MagFilter == RSamplerDescriptor::EFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	sampler_info.mipmapMode = SamplerDesc.MipFilter == RSamplerDescriptor::EFilter::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = VK_LOD_CLAMP_NONE;

	vkCreateSampler(Device->getVkDevice(), &sampler_info, nullptr, &Sampler);
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

void VulkanSampler::setDebugName(const std::string& Name)
{
	(void)Name;
}

} // namespace render::rhi
