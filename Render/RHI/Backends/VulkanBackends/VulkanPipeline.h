
#pragma once 

#include "../../RHI.h"
#include <vulkan/vulkan.h>

namespace render::rhi
{

class VulkanDevice;

class VulkanPipeline : public RPipeline
{
public:
	VulkanPipeline(VulkanDevice* InDevice, VkPipeline InPipeline, VkPipelineLayout InLayout, EPipelineType InPipelineType);
	~VulkanPipeline() override;

	bool isValid() const override;
	bool isGraphicsPipeline() const override { return PipelineType == EPipelineType::Graphics; }
	virtual bool isComputePipeline() const override { return PipelineType == EPipelineType::Compute; }
	virtual EPipelineType getPipelineType() const override { return PipelineType; }
	VkPipeline getVkPipeline() const noexcept { return Pipeline; }
	VkPipelineLayout getVkPipelineLayout() const noexcept { return Layout; }
	VkPipelineBindPoint getBindPoint() const noexcept;

private:
	void setDebugName(const std::string& Name) override;

	VulkanDevice* Device = nullptr;
	VkPipeline Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout Layout = VK_NULL_HANDLE;
	EPipelineType PipelineType = EPipelineType::None;
};

/**
 * @brief 创建 Vulkan 图形管线对象
 * @param Device Vulkan 设备对象
 * @param Descriptor 图形管线描述符
 * @return VulkanPipeline* Vulkan 图形管线对象指针, 如果创建失败则返回 nullptr
 */
VulkanPipeline* CreateVulkanGraphicsPipeline(VulkanDevice* Device, const RGraphicsPipelineDescriptor& Descriptor);
VulkanPipeline* CreateVulkanComputePipeline(VulkanDevice* Device, const RComputePipelineDescriptor& Descriptor);
} // namespace render::rhi