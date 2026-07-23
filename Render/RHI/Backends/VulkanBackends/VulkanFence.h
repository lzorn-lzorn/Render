
#pragma once

#include "../../Definitions.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>


namespace render::rhi
{

class VulkanDevice;
class VulkanFence : public RFence
{
public:
	VulkanFence(VulkanDevice* InDevice);
	~VulkanFence() override;

	void signal(uint64_t Value) override;
	uint64_t getCompletedValue() const override;
	void wait(uint64_t Value) override;

private:
	VulkanDevice* Device;
	uint64_t CompletedValue = 0;
	VkFence Fence = VK_NULL_HANDLE;
};

}