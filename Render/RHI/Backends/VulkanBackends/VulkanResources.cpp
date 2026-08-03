#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "VulkanRHI.h"

#include <algorithm>
#include <cstring>

namespace render::rhi
{

namespace
{

uint64_t clampCopySize(uint64_t Offset, uint64_t RequestedSize, uint64_t MaxSize)
{
	if (Offset >= MaxSize)
	{
		return 0;
	}

	const uint64_t available_size = MaxSize - Offset;
	if (RequestedSize == 0)
	{
		return available_size;
	}
	return std::min<uint64_t>(RequestedSize, available_size);
}

bool buildMappedMemoryRange(
	VulkanDevice* Device,
	VkDeviceMemory Memory,
	uint64_t BufferSize,
	uint64_t Offset,
	uint64_t Size,
	VkMappedMemoryRange& OutRange)
{
	if (!Device || Memory == VK_NULL_HANDLE)
	{
		return false;
	}

	const uint64_t range_size = clampCopySize(Offset, Size, BufferSize);
	if (range_size == 0)
	{
		return false;
	}

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(Device->getVkPhysicalDevice(), &properties);

	const uint64_t atom_size = std::max<uint64_t>(1, properties.limits.nonCoherentAtomSize);
	const uint64_t aligned_offset = Offset - (Offset % atom_size);
	const uint64_t aligned_end = Align<uint64_t>(Offset + range_size, atom_size);
	const uint64_t clamped_end = std::min<uint64_t>(aligned_end, BufferSize);
	if (clamped_end <= aligned_offset)
	{
		return false;
	}

	OutRange = {};
	OutRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	OutRange.memory = Memory;
	OutRange.offset = static_cast<VkDeviceSize>(aligned_offset);
	OutRange.size = static_cast<VkDeviceSize>(clamped_end - aligned_offset);
	return true;
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

	if (BufferDesc.InitialData && BufferDesc.InitialDataSize > 0 && !BufferDesc.isCpuAccessible())
	{
		BufferDesc.Usage = BufferDesc.Usage | EBufferUsage::TransferDst;
	}

	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = static_cast<VkDeviceSize>(BufferDesc.Size);
	buffer_info.usage = ToVkBufferUsage(BufferDesc.Usage);
	if (buffer_info.usage == 0)
	{
		buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}

	buffer_info.sharingMode = ToVkSharingMode(InBufferDesc.SharingMode);

	VkDevice vk_device = Device->getVkDevice();
	if (vkCreateBuffer(vk_device, &buffer_info, nullptr, &Buffer) != VK_SUCCESS)
	{
		Buffer = VK_NULL_HANDLE;
		return;
	}

	VkMemoryRequirements memory_requirements{};
	vkGetBufferMemoryRequirements(vk_device, Buffer, &memory_requirements);

	const VkMemoryPropertyFlags memory_flags = ToVkMemoryPropertyFlags(BufferDesc.MemoryProperties);
	const uint32_t memory_type = Device->findMemoryType(memory_requirements.memoryTypeBits, memory_flags);
	if (memory_type == UINT32_MAX)
	{
		vkDestroyBuffer(vk_device, Buffer, nullptr);
		Buffer = VK_NULL_HANDLE;
		return;
	}

	VkMemoryAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = memory_type;

	if (vkAllocateMemory(vk_device, &allocate_info, nullptr, &Memory) != VK_SUCCESS)
	{
		vkDestroyBuffer(vk_device, Buffer, nullptr);
		Buffer = VK_NULL_HANDLE;
		Memory = VK_NULL_HANDLE;
		return;
	}

	if (vkBindBufferMemory(vk_device, Buffer, Memory, 0) != VK_SUCCESS)
	{
		vkDestroyBuffer(vk_device, Buffer, nullptr);
		vkFreeMemory(vk_device, Memory, nullptr);
		Buffer = VK_NULL_HANDLE;
		Memory = VK_NULL_HANDLE;
		return;
	}

	if (BufferDesc.InitialData && BufferDesc.InitialDataSize > 0)
	{
		const uint64_t copy_size = std::min<uint64_t>(BufferDesc.Size, BufferDesc.InitialDataSize);
		updateData(0, BufferDesc.InitialData, copy_size);
	}
}

VulkanBuffer::~VulkanBuffer()
{
	VkDevice vk_device = Device ? Device->getVkDevice() : VK_NULL_HANDLE;
	if (vk_device == VK_NULL_HANDLE)
	{
		return;
	}

	if (MappedPtr != nullptr && Memory != VK_NULL_HANDLE)
	{
		vkUnmapMemory(vk_device, Memory);
		MappedPtr = nullptr;
		MappedOffset = 0;
		MappedSize = 0;
	}

	if (Buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(vk_device, Buffer, nullptr);
		Buffer = VK_NULL_HANDLE;
	}

	if (Memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vk_device, Memory, nullptr);
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

bool VulkanBuffer::updateData(uint64_t Offset, const void* Data, uint64_t Size)
{
	if (!Data || Size == 0 || !isValid())
	{
		return false;
	}

	const uint64_t copy_size = clampCopySize(Offset, Size, BufferDesc.Size);
	if (copy_size == 0)
	{
		return false;
	}

	if (isCpuAccessible())
	{
		void* mapped_ptr = mapRange(EBufferMapMode::Write, Offset, copy_size);
		if (!mapped_ptr)
		{
			return false;
		}

		std::memcpy(mapped_ptr, Data, static_cast<size_t>(copy_size));
		if (!hasAnyFlags(BufferDesc.MemoryProperties, EMemoryProperty::HostCoherent))
		{
			flushMappedRange(Offset, copy_size);
		}
		unmap();
		return true;
	}

	if (!Device || !hasAnyFlags(BufferDesc.Usage, EBufferUsage::TransferDst))
	{
		return false;
	}

	RBuffer* staging_buffer_base = Device->createStagingBuffer(Data, copy_size, copy_size);
	auto* staging_buffer = static_cast<VulkanBuffer*>(staging_buffer_base);
	if (!staging_buffer || !staging_buffer->isValid())
	{
		Device->destroyResource(staging_buffer_base);
		return false;
	}

	VkCommandBuffer command_buffer = Device->beginImmediateCommand();
	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = static_cast<VkDeviceSize>(Offset);
	copy_region.size = static_cast<VkDeviceSize>(copy_size);
	vkCmdCopyBuffer(command_buffer, staging_buffer->getVkBuffer(), Buffer, 1, &copy_region);
	Device->endImmediateCommand(command_buffer);

	Device->destroyResource(staging_buffer_base);
	return true;
}

void* VulkanBuffer::mapRange(EBufferMapMode MapMode, uint64_t Offset, uint64_t Size)
{
	if (!Device || !isValid() || !isCpuAccessible())
	{
		return nullptr;
	}

	const uint64_t map_size = clampCopySize(Offset, Size, BufferDesc.Size);
	if (map_size == 0)
	{
		return nullptr;
	}

	if (MappedPtr != nullptr)
	{
		if (Offset == MappedOffset && map_size <= MappedSize)
		{
			return MappedPtr;
		}
		return nullptr;
	}

	VkDevice vk_device = Device->getVkDevice();
	if (vk_device == VK_NULL_HANDLE)
	{
		return nullptr;
	}

	void* mapped_ptr = nullptr;
	if (vkMapMemory(vk_device, Memory, static_cast<VkDeviceSize>(Offset), static_cast<VkDeviceSize>(map_size), 0, &mapped_ptr) != VK_SUCCESS)
	{
		return nullptr;
	}

	MappedPtr = mapped_ptr;
	MappedOffset = Offset;
	MappedSize = map_size;

	if (!hasAnyFlags(BufferDesc.MemoryProperties, EMemoryProperty::HostCoherent) &&
		(MapMode == EBufferMapMode::Read || MapMode == EBufferMapMode::ReadWrite))
	{
		invalidateMappedRange(Offset, map_size);
	}

	return MappedPtr;
}

void VulkanBuffer::unmap()
{
	if (!Device || Memory == VK_NULL_HANDLE || MappedPtr == nullptr)
	{
		return;
	}

	VkDevice vk_device = Device->getVkDevice();
	if (vk_device == VK_NULL_HANDLE)
	{
		return;
	}

	vkUnmapMemory(vk_device, Memory);
	MappedPtr = nullptr;
	MappedOffset = 0;
	MappedSize = 0;
}

bool VulkanBuffer::flushMappedRange(uint64_t Offset, uint64_t Size)
{
	if (!Device || !isValid() || !isCpuAccessible())
	{
		return false;
	}

	if (hasAnyFlags(BufferDesc.MemoryProperties, EMemoryProperty::HostCoherent))
	{
		return true;
	}

	VkMappedMemoryRange range{};
	if (!buildMappedMemoryRange(Device, Memory, BufferDesc.Size, Offset, Size, range))
	{
		return false;
	}

	return vkFlushMappedMemoryRanges(Device->getVkDevice(), 1, &range) == VK_SUCCESS;
}

bool VulkanBuffer::invalidateMappedRange(uint64_t Offset, uint64_t Size)
{
	if (!Device || !isValid() || !isCpuAccessible())
	{
		return false;
	}

	if (hasAnyFlags(BufferDesc.MemoryProperties, EMemoryProperty::HostCoherent))
	{
		return true;
	}

	VkMappedMemoryRange range{};
	if (!buildMappedMemoryRange(Device, Memory, BufferDesc.Size, Offset, Size, range))
	{
		return false;
	}

	return vkInvalidateMappedMemoryRanges(Device->getVkDevice(), 1, &range) == VK_SUCCESS;
}

bool VulkanBuffer::isCpuAccessible() const
{
	return hasAnyFlags(BufferDesc.MemoryProperties, EMemoryProperty::HostVisible);
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

	VkDevice vk_device = Device->getVkDevice();
	if (vk_device != VK_NULL_HANDLE && Sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(vk_device, Sampler, nullptr);
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
