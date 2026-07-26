
#pragma once 

#include "../../Definitions.h"
#include <vulkan/vulkan.h>

namespace render::rhi
{

class VulkanDevice;

class VulkanPipelineState : public RPipelineState
{
public:
	VulkanPipelineState(VulkanDevice* InDevice, VkPipeline InPipeline, VkPipelineLayout InLayout, bool bInIsGraphicsPipeline);
	~VulkanPipelineState() override;

	bool isValid() const override;
	bool isGraphicsPipeline() const override { return bIsGraphicsPipeline; }
	VkPipeline getVkPipeline() const noexcept { return Pipeline; }
	VkPipelineLayout getVkPipelineLayout() const noexcept { return Layout; }
	VkPipelineBindPoint getBindPoint() const noexcept;

private:
	void SetDebugName(const std::string& Name) override;

	VulkanDevice* Device = nullptr;
	bool bIsGraphicsPipeline = true;
	VkPipeline Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout Layout = VK_NULL_HANDLE;
};

VulkanPipelineState* CreateVulkanGraphicsPipeline(VulkanDevice* Device, const RGraphicsPipelineDescriptor& Descriptor);
VulkanPipelineState* CreateVulkanComputePipeline(VulkanDevice* Device, const RComputePipelineDescriptor& Descriptor);
} // namespace render::rhi