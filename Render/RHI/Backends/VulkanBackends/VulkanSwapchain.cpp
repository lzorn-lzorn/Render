
#include "../../Definitions.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace render::rhi
{

VulkanSwapchain::VulkanSwapchain(VulkanDevice* InDevice, const RSwapchainDescriptor& InDescriptor)
	: Device(InDevice)
	, Desc(InDescriptor)
{
}

VulkanSwapchain::~VulkanSwapchain()
{
	const auto vkDevice = Device ? reinterpret_cast<VkDevice>(Device->getDeviceHandle()) : VkDevice{};
	if (vkDevice != VkDevice{} && Swapchain != VkSwapchainKHR{})
	{
		vkDestroySwapchainKHR(vkDevice, Swapchain, nullptr);
	}
}

RTexture* VulkanSwapchain::acquireNextTexture()
{
	return nullptr;
}

void VulkanSwapchain::present()
{
}

void VulkanSwapchain::resize(uint32_t Width, uint32_t Height)
{
	Desc.Width = Width;
	Desc.Height = Height;
}


}