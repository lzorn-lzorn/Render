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
	~VulkanDescriptorSetLayout() override;

	bool isValid() const override;
	VkDescriptorSetLayout getVkDescriptorSetLayout() const noexcept { return Layout; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device = nullptr;
	RDescriptorSetLayoutDescriptor Descriptor{};
	VkDescriptorSetLayout Layout = VK_NULL_HANDLE;

};

class VulkanDescriptorSet : public RDescriptorSet
{
public:
	VulkanDescriptorSet(VulkanDevice* InDevice, const RDescriptorSetDescriptor& InDescriptor);
	~VulkanDescriptorSet() override;

	bool isValid() const override;
	VkDescriptorSet getVkDescriptorSet() const noexcept { return Set; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device = nullptr;
	RDescriptorSetDescriptor Descriptor{};
	VkDescriptorSet Set = VK_NULL_HANDLE;

};


}