#pragma once

#include "../../Definitions.h"
#include <vulkan/vulkan.h>
namespace render::rhi
{
class VulkanDevice;
class VulkanTextureView;

class VulkanDescriptorSetLayout : public RDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout(VulkanDevice* InDevice, const RDescriptorSetLayoutDescriptor& InDescriptor);
	~VulkanDescriptorSetLayout();
	virtual EResourceType getType() const override { return EResourceType::DescriptorSetLayout; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	RDescriptorSetLayoutDescriptor Descriptor;

};

class VulkanDescriptorSet : public RDescriptorSet
{
public:
	VulkanDescriptorSet(VulkanDevice* InDevice, const RDescriptorSetDescriptor& InDescriptor);
	~VulkanDescriptorSet();
	virtual EResourceType getType() const override { return EResourceType::DescriptorSet; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	RDescriptorSetDescriptor Descriptor;

};


}