#include "VulkanRHI.h"

namespace render::rhi
{

VulkanBindGroupLayout::VulkanBindGroupLayout(VulkanDevice* InDevice, const RBindGroupLayoutDescriptor& InDescriptor)
	: Device(InDevice)
	, Descriptor(InDescriptor)
{
}

VulkanBindGroupLayout::~VulkanBindGroupLayout() = default;

void VulkanBindGroupLayout::SetDebugName(const std::string& Name)
{
	(void)Name;
}

VulkanBindGroup::VulkanBindGroup(VulkanDevice* InDevice, const RBindGroupDescriptor& InDescriptor)
	: Device(InDevice)
	, Descriptor(InDescriptor)
{
}

VulkanBindGroup::~VulkanBindGroup() = default;

void VulkanBindGroup::SetDebugName(const std::string& Name)
{
	(void)Name;
}

} // namespace render::rhi
