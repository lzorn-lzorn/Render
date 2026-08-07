
#pragma once

#include "../../RHI.h"

#include <vulkan/vulkan.h>


namespace render::rhi
{

class VulkanDevice;
class VulkanFence : public RFence
{
public:
	VulkanFence(VulkanDevice* InDevice);
	~VulkanFence() override;

	bool isValid() const noexcept;
	void signal(uint64_t Value) override;
	uint64_t getCompletedValue() const override;
	void wait(uint64_t Value) override;
	VkFence getVkFence() const noexcept { return Fence; }

private:
	VulkanDevice* Device = nullptr;
	uint64_t CompletedValue = 0;
	VkFence Fence = VK_NULL_HANDLE;
};

}