#pragma once

#include "../../Definitions.h"

#include <vulkan/vulkan.h>

namespace render::rhi
{

class VulkanDevice;

class VulkanSampler : public RSampler
{
public:
	VulkanSampler(VulkanDevice* InDevice, const RSamplerDescriptor& InSamplerDesc);
	~VulkanSampler() override;

	bool isValid() const override;
	VkSampler getVkSampler() const noexcept { return Sampler; }

private:
	void setDebugName(const std::string& Name) override;

	VulkanDevice* Device = nullptr;
	RSamplerDescriptor SamplerDesc{};
	VkSampler Sampler = VK_NULL_HANDLE;
};

} // namespace render::rhi
