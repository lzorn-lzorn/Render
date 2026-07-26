#pragma once

#include <vulkan/vulkan.h>

#include "../../Definitions.h"

namespace render::rhi
{

class VulkanDevice;

class VulkanShader : public RShader
{
public:
	VulkanShader(VulkanDevice* InDevice, const RShaderDescriptor& InShaderDesc);
	~VulkanShader() override;

	bool isValid() const override;
	EShaderStage getStage() const override { return Stage; }
	VkShaderModule getVkShaderModule() const noexcept { return ShaderModule; }
	const std::string& getEntryPoint() const noexcept { return EntryPoint; }

private:
	void SetDebugName(const std::string& Name) override;

	VulkanDevice* Device = nullptr;
	VkShaderModule ShaderModule = VK_NULL_HANDLE;
	EShaderStage Stage = EShaderStage::Vertex;
	std::string EntryPoint = "main";
};

} // namespace render::rhi