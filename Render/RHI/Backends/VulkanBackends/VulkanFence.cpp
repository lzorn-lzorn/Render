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

	VkFenceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence(Device->getVkDevice(), &createInfo, nullptr, &Fence);
}

VulkanFence::~VulkanFence()
{
	if (!Device)
	{
		return;
	}

	const VkDevice vkDevice = Device->getVkDevice();
	if (vkDevice != VK_NULL_HANDLE && Fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(vkDevice, Fence, nullptr);
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