
#include "VulkanRHI.h"
#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "VulkanShader.h"

namespace render::rhi
{


VulkanShader::VulkanShader(VulkanDevice* InDevice, const EShaderDescriptor& InShaderDesc, VkShaderModule InShaderModule)
	: Device(InDevice)
	, ShaderDesc(InShaderDesc)
	, ShaderModule(InShaderModule)
	, Stage(InShaderDesc.Stage)
{
}

VulkanShader::~VulkanShader()
{
	const auto vkDevice = Device ? reinterpret_cast<VkDevice>(Device->getDeviceHandle()) : VkDevice{};
	if (vkDevice != VkDevice{} && ShaderModule != VkShaderModule{})
	{
		vkDestroyShaderModule(vkDevice, ShaderModule, nullptr);
	}
}

void VulkanShader::SetDebugName(const std::string& Name)
{
	(void)Name;
}


}