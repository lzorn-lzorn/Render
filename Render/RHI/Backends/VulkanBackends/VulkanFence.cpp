#include "VulkanFence.h"
#include "VulkanDevice.h"

namespace render::rhi
{

	
VulkanFence::VulkanFence(VulkanDevice* InDevice)
	: Device(InDevice)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	VkFenceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence(Device->getVkDevice(), &create_info, nullptr, &Fence);
}

VulkanFence::~VulkanFence()
{
	if (!Device)
	{
		return;
	}

	const VkDevice vk_device = Device->getVkDevice();
	if (vk_device != VK_NULL_HANDLE && Fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(vk_device, Fence, nullptr);
		Fence = VK_NULL_HANDLE;
	}
}

bool VulkanFence::isValid() const noexcept
{
	return Fence != VK_NULL_HANDLE;
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

	if (Device && Fence != VK_NULL_HANDLE)
	{
		vkWaitForFences(Device->getVkDevice(), 1, &Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(Device->getVkDevice(), 1, &Fence);
	}
}

} // namespace render::rhi