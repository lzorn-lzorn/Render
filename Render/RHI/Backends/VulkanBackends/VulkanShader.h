
#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "../../Definitions.h"

namespace render::rhi
{

class VulkanDevice;

class VulkanShader : public RShader
{
public:
	VulkanShader(VulkanDevice* InDevice, const EShaderDescriptor& InShaderDesc, VkShaderModule InShaderModule);
	~VulkanShader();
	virtual EResourceType getType() const override { return EResourceType::Shader; }
	virtual EShaderStage getStage() const override { return Stage; }
private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	EShaderDescriptor ShaderDesc;
	VkShaderModule ShaderModule;
	EShaderStage Stage;
};

} // namespace render::rhi