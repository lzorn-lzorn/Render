#include "VulkanSampler.h"

#include "VulkanDevice.h"

namespace render::rhi
{

VulkanSampler::VulkanSampler(VulkanDevice* InDevice, const RSamplerDescriptor& InSamplerDesc)
	: Device(InDevice)
	, SamplerDesc(InSamplerDesc)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.minFilter = SamplerDesc.MinFilter == RSamplerDescriptor::EFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	sampler_info.magFilter = SamplerDesc.MagFilter == RSamplerDescriptor::EFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	sampler_info.mipmapMode = SamplerDesc.MipFilter == RSamplerDescriptor::EFilter::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = VK_LOD_CLAMP_NONE;

	vkCreateSampler(Device->getVkDevice(), &sampler_info, nullptr, &Sampler);
}

VulkanSampler::~VulkanSampler()
{
	if (!Device)
	{
		return;
	}

	VkDevice vk_device = Device->getVkDevice();
	if (vk_device != VK_NULL_HANDLE && Sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(vk_device, Sampler, nullptr);
		Sampler = VK_NULL_HANDLE;
	}
}

bool VulkanSampler::isValid() const
{
	return Sampler != VK_NULL_HANDLE;
}

void VulkanSampler::setDebugName(const std::string& Name)
{
	(void)Name;
}

} // namespace render::rhi
