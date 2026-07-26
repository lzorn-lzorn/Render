#include "VulkanDescriptorSet.h"

#include "VulkanDevice.h"
#include "VulkanRHI.h"

#include <vector>

namespace render::rhi
{

namespace
{

VkDescriptorType ToVkDescriptorType(EDescriptorType type)
{
	switch (type)
	{
	case EDescriptorType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case EDescriptorType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case EDescriptorType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
	case EDescriptorType::SampledTexture: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case EDescriptorType::StorageTexture: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	default:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}
}

} // namespace

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice* InDevice, const RDescriptorSetLayoutDescriptor& InDescriptor)
	: Device(InDevice)
	, Descriptor(InDescriptor)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.reserve(Descriptor.size());
	for (const RDescriptorSetLayoutEntry& entry : Descriptor)
	{
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = entry.Binding;
		binding.descriptorType = ToVkDescriptorType(entry.Type);
		binding.descriptorCount = entry.Count;
		binding.stageFlags = ToVkShaderStageFlags(entry.Stage);
		binding.pImmutableSamplers = nullptr;
		bindings.push_back(binding);
	}

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	vkCreateDescriptorSetLayout(Device->getVkDevice(), &createInfo, nullptr, &Layout);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
	if (Device && Layout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(Device->getVkDevice(), Layout, nullptr);
		Layout = VK_NULL_HANDLE;
	}
}

bool VulkanDescriptorSetLayout::isValid() const
{
	return Layout != VK_NULL_HANDLE;
}

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

bool VulkanDescriptorSet::isValid() const
{
	if (!Descriptor.Layout)
	{
		return false;
	}

	const auto* layout = dynamic_cast<const VulkanDescriptorSetLayout*>(Descriptor.Layout);
	return layout && layout->isValid();
}

void VulkanDescriptorSet::SetDebugName(const std::string& Name)
{
	(void)Name;
}

} // namespace render::rhi
