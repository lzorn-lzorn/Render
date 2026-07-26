#include "VulkanShader.h"

#include "VulkanDevice.h"

#include <cstring>
#include <vector>

namespace render::rhi
{

VulkanShader::VulkanShader(VulkanDevice* InDevice, const RShaderDescriptor& InShaderDesc)
	: Device(InDevice)
	, Stage(InShaderDesc.Stage)
	, EntryPoint(InShaderDesc.EntryPoint.empty() ? "main" : InShaderDesc.EntryPoint)
{
	if (!Device || !Device->isValid() || InShaderDesc.ByteCodes.empty())
	{
		return;
	}

	const size_t byteSize = InShaderDesc.ByteCodes.size();
	const size_t paddedSize = (byteSize + 3u) & ~size_t(3u);

	std::vector<uint32_t> codeWords(paddedSize / 4u, 0u);
	std::memcpy(codeWords.data(), InShaderDesc.ByteCodes.data(), byteSize);

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = paddedSize;
	createInfo.pCode = codeWords.data();

	vkCreateShaderModule(Device->getVkDevice(), &createInfo, nullptr, &ShaderModule);
}

VulkanShader::~VulkanShader()
{
	if (!Device)
	{
		return;
	}

	const VkDevice vkDevice = Device->getVkDevice();
	if (vkDevice != VK_NULL_HANDLE && ShaderModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(vkDevice, ShaderModule, nullptr);
		ShaderModule = VK_NULL_HANDLE;
	}
}

bool VulkanShader::isValid() const
{
	return ShaderModule != VK_NULL_HANDLE;
}

void VulkanShader::SetDebugName(const std::string& Name)
{
	(void)Name;
}

} // namespace render::rhi
