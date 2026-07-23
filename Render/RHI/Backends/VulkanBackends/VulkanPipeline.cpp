#include "VulkanPipeline.h"
#include "VulkanDevice.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace render::rhi
{

VulkanPipelineState::VulkanPipelineState(VulkanDevice* InDevice, VkPipeline InPipeline, bool bInIsGraphicsPipeline)
	: Device(InDevice)
	, bIsGraphicsPipeline(bInIsGraphicsPipeline)
	, Pipeline(InPipeline)
{
}

VulkanPipelineState::~VulkanPipelineState()
{
	const auto vkDevice = Device ? reinterpret_cast<VkDevice>(Device->getDeviceHandle()) : VkDevice{};
	if (vkDevice != VkDevice{} && Pipeline != VkPipeline{})
	{
		vkDestroyPipeline(vkDevice, Pipeline, nullptr);
	}
}

void VulkanPipelineState::SetDebugName(const std::string& Name)
{
	(void)Name;
}


} // namespace render::rhi