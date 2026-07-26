#include "VulkanDescriptorSet.h"

namespace render::rhi
{

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice* InDevice, const RDescriptorSetLayoutDescriptor& InDescriptor)
	: Device(InDevice)
	, Descriptor(InDescriptor)
{
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() = default;

void VulkanDescriptorSetLayout::SetDebugName(const std::string& Name)
{
	(void)Name;
}

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDevice* InDevice, const RDescriptorSetDescriptor& InDescriptor)
	: Device(InDevice)
	, Descriptor(InDescriptor)
{
}

VulkanDescriptorSet::~VulkanDescriptorSet() = default;

void VulkanDescriptorSet::SetDebugName(const std::string& Name)
{
	(void)Name;
}

} // namespace render::rhi
