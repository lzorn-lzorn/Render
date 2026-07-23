
#include "../../Definitions.h"
#include "VulkanFence.h"
#include "VulkanDevice.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace render::rhi
{

	
VulkanFence::VulkanFence(VulkanDevice* InDevice)
	: Device(InDevice)
{
}

VulkanFence::~VulkanFence()
{
	const auto vkDevice = Device ? reinterpret_cast<VkDevice>(Device->getDeviceHandle()) : VkDevice{};
	if (vkDevice != VkDevice{} && Fence != VkFence{})
	{
		vkDestroyFence(vkDevice, Fence, nullptr);
	}
}

void VulkanFence::signal(uint64_t Value)
{
	CompletedValue = Value;
}

uint64_t VulkanFence::getCompletedValue() const
{
	return CompletedValue;
}

void VulkanFence::wait(uint64_t Value)
{
	if (CompletedValue < Value)
	{
		CompletedValue = Value;
	}
}

} // namespace render::rhi