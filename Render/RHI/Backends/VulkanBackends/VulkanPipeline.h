
#pragma once 

#include "../../Definitions.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace render::rhi
{

class VulkanDevice;

class VulkanPipelineState : public RPipelineState
{
public:
	VulkanPipelineState(VulkanDevice* InDevice, VkPipeline InPipeline, bool bInIsGraphicsPipeline);
	~VulkanPipelineState();
	virtual EResourceType getType() const override { return EResourceType::Pipeline; }
	virtual bool isGraphicsPipeline() const override { return bIsGraphicsPipeline; }
	VkPipeline getVkPipeline() const { return Pipeline; }
private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	bool bIsGraphicsPipeline;
	VkPipeline Pipeline;
};
} // namespace render::rhi