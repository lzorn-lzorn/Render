#include "VulkanBuffer.h"
#include "VulkanDevice.h"

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

	if (BufferDesc.InitialData && BufferDesc.InitialDataSize > 0 && !BufferDesc.isCpuAccessible())
	{
		BufferDesc.Usage = BufferDesc.Usage | EBufferUsage::TransferDst;
	}

	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = static_cast<VkDeviceSize>(BufferDesc.Size);
	buffer_info.usage = toVkBufferUsage(BufferDesc.Usage);
	if (buffer_info.usage == 0)
	{
		buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}

	buffer_info.sharingMode = toVkSharingMode(BufferDesc.SharingMode);

	if (vkCreateBuffer(Device->getVkDevice(), &buffer_info, nullptr, &Buffer) != VK_SUCCESS)
	{
		Buffer = VK_NULL_HANDLE;
		return;
	}

	if (!allocateMemory())
	{
		vkDestroyBuffer(Device->getVkDevice(), Buffer, nullptr);
		Buffer = VK_NULL_HANDLE;
		return;
	}

	(void)uploadInitialData();
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
	if (!staging_buffer_base)
	{
		return false;
	}

	auto* staging_buffer = static_cast<VulkanBuffer*>(staging_buffer_base);
	if (!staging_buffer->isValid())
	{
		Device->destroyResource(staging_buffer_base);
		return false;
	}

	VkCommandBuffer command_buffer = Device->beginImmediateCommand();
	if (command_buffer == VK_NULL_HANDLE)
	{
		Device->destroyResource(staging_buffer_base);
		return false;
	}

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
		const uint64_t mapped_end = MappedOffset + MappedSize;
		const uint64_t request_end = Offset + map_size;
		if (Offset >= MappedOffset && request_end <= mapped_end)
		{
			auto* mapped_base = static_cast<uint8_t*>(MappedPtr);
			return mapped_base + (Offset - MappedOffset);
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
	if (!Device || !isValid() || !isCpuAccessible() || MappedPtr == nullptr)
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
	if (!Device || !isValid() || !isCpuAccessible() || MappedPtr == nullptr)
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

bool VulkanBuffer::allocateMemory()
{
	if (!Device || Buffer == VK_NULL_HANDLE)
	{
		return false;
	}

	VkMemoryRequirements memory_requirements{};
	vkGetBufferMemoryRequirements(Device->getVkDevice(), Buffer, &memory_requirements);

	const VkMemoryPropertyFlags memory_flags = toVkMemoryPropertyFlags(BufferDesc.MemoryProperties);
	const uint32_t memory_type = Device->findMemoryType(memory_requirements.memoryTypeBits, memory_flags);
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

	if (vkBindBufferMemory(Device->getVkDevice(), Buffer, Memory, 0) != VK_SUCCESS)
	{
		vkFreeMemory(Device->getVkDevice(), Memory, nullptr);
		Memory = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

bool VulkanBuffer::uploadInitialData()
{
	if (!BufferDesc.InitialData || BufferDesc.InitialDataSize == 0)
	{
		return true;
	}

	const uint64_t copy_size = std::min<uint64_t>(BufferDesc.Size, BufferDesc.InitialDataSize);
	if (copy_size == 0)
	{
		return true;
	}

	return updateData(0, BufferDesc.InitialData, copy_size);
}

void VulkanBuffer::setDebugName(const std::string& Name)
{
	(void)Name;
}



}