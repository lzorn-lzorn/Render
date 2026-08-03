#include "VulkanPipeline.h"

#include "VulkanDevice.h"
#include "VulkanRHI.h"
#include "VulkanShader.h"

#include <array>
#include <vector>
#include <type_traits>

namespace render::rhi
{

namespace
{

VkSampleCountFlagBits ToVkSampleCountFromUint(uint32_t sampleCount)
{
	switch (sampleCount)
	{
	case 2: return VK_SAMPLE_COUNT_2_BIT;
	case 4: return VK_SAMPLE_COUNT_4_BIT;
	case 8: return VK_SAMPLE_COUNT_8_BIT;
	case 16: return VK_SAMPLE_COUNT_16_BIT;
	case 32: return VK_SAMPLE_COUNT_32_BIT;
	case 64: return VK_SAMPLE_COUNT_64_BIT;
	case 1:
	default:
		return VK_SAMPLE_COUNT_1_BIT;
	}
}

VkPipelineLayout CreatePipelineLayout(VkDevice device)
{
	VkPushConstantRange pushRange{};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	pushRange.offset = 0;
	pushRange.size = 128;

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushRange;

	VkPipelineLayout layout = VK_NULL_HANDLE;
	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}

	return layout;
}

} // namespace

VulkanPipeline::VulkanPipeline(VulkanDevice* InDevice, VkPipeline InPipeline, VkPipelineLayout InLayout, EPipelineType InPipelineType)
	: Device(InDevice)
	, PipelineType(InPipelineType)
	, Pipeline(InPipeline)
	, Layout(InLayout)
{
}

VulkanPipeline::~VulkanPipeline()
{
	if (!Device)
	{
		return;
	}

	const VkDevice vkDevice = Device->getVkDevice();
	if (vkDevice == VK_NULL_HANDLE)
	{
		return;
	}

	if (Pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(vkDevice, Pipeline, nullptr);
		Pipeline = VK_NULL_HANDLE;
	}

	if (Layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(vkDevice, Layout, nullptr);
		Layout = VK_NULL_HANDLE;
	}
}

bool VulkanPipeline::isValid() const
{
	return Pipeline != VK_NULL_HANDLE && Layout != VK_NULL_HANDLE;
}

VkPipelineBindPoint VulkanPipeline::getBindPoint() const noexcept
{
	return PipelineType == EPipelineType::Graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
}

void VulkanPipeline::setDebugName(const std::string& Name)
{
	(void)Name;
}

VulkanPipeline* CreateVulkanGraphicsPipeline(VulkanDevice* Device, const RGraphicsPipelineDescriptor& Descriptor)
{
	if (!Device || !Device->isValid())
	{
		return new VulkanPipeline(Device, VK_NULL_HANDLE, VK_NULL_HANDLE, EPipelineType::Graphics);
	}

	auto* vertexShader = static_cast<VulkanShader*>(Descriptor.VertexShader);
	auto* pixelShader = static_cast<VulkanShader*>(Descriptor.PixelShader);

	if (!vertexShader || !pixelShader || !vertexShader->isValid() || !pixelShader->isValid() || Descriptor.RenderTargetCount == 0)
	{
		return new VulkanPipeline(Device, VK_NULL_HANDLE, VK_NULL_HANDLE, EPipelineType::Graphics);
	}

	VkDevice vkDevice = Device->getVkDevice();
	VkPipelineLayout layout = CreatePipelineLayout(vkDevice);
	if (layout == VK_NULL_HANDLE)
	{
		return new VulkanPipeline(Device, VK_NULL_HANDLE, VK_NULL_HANDLE, EPipelineType::Graphics);
	}

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.reserve(3);

	VkPipelineShaderStageCreateInfo vsInfo{};
	vsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vsInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vsInfo.module = vertexShader->getVkShaderModule();
	vsInfo.pName = vertexShader->getEntryPoint().c_str();
	shaderStages.push_back(vsInfo);

	VkPipelineShaderStageCreateInfo psInfo{};
	psInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	psInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	psInfo.module = pixelShader->getVkShaderModule();
	psInfo.pName = pixelShader->getEntryPoint().c_str();
	shaderStages.push_back(psInfo);

	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	bindingDescriptions.reserve(Descriptor.VertexInputLayout.Bindings.size());
	for (const RVertexBindingDescriptor& binding : Descriptor.VertexInputLayout.Bindings)
	{
		VkVertexInputBindingDescription vkBinding{};
		vkBinding.binding = binding.Binding;
		vkBinding.stride = binding.Stride;
		vkBinding.inputRate = (binding.Rate == RVertexBindingDescriptor::EVertexInputRate::Instance)
			? VK_VERTEX_INPUT_RATE_INSTANCE
			: VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDescriptions.push_back(vkBinding);
	}

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	attributeDescriptions.reserve(Descriptor.VertexInputLayout.Attributes.size());
	for (const RVertexAttribute& attribute : Descriptor.VertexInputLayout.Attributes)
	{
		VkVertexInputAttributeDescription vkAttribute{};
		vkAttribute.location = attribute.Location;
		vkAttribute.binding = attribute.Binding;
		vkAttribute.format = toVkFormat(attribute.Format);
		vkAttribute.offset = attribute.Offset;
		attributeDescriptions.push_back(vkAttribute);
	}

	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInput.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = toVkPrimitiveTopology(Descriptor.PrimitiveTopology);
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = Descriptor.RasterizerState.DepthClipEnable ? VK_FALSE : VK_TRUE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = toVkPolygonMode(Descriptor.RasterizerState.FillMode);
	rasterizer.cullMode = toVkCullMode(Descriptor.RasterizerState.CullMode);
	rasterizer.frontFace = Descriptor.RasterizerState.FrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = Descriptor.RasterizerState.DepthBias != 0 ? VK_TRUE : VK_FALSE;
	rasterizer.depthBiasConstantFactor = static_cast<float>(Descriptor.RasterizerState.DepthBias);
	rasterizer.depthBiasClamp = Descriptor.RasterizerState.DepthBiasClamp;
	rasterizer.depthBiasSlopeFactor = Descriptor.RasterizerState.DepthBiasSlopeFactor;
	rasterizer.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = ToVkSampleCountFromUint(Descriptor.SampleCount);
	multisample.sampleShadingEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = Descriptor.DepthStencilState.DepthTestEnable ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = Descriptor.DepthStencilState.DepthWriteEnable ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = toVkCompareOp(Descriptor.DepthStencilState.DepthFunc);
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = Descriptor.DepthStencilState.StencilTestEnable ? VK_TRUE : VK_FALSE;

	std::vector<VkPipelineColorBlendAttachmentState> colorAttachments;
	colorAttachments.resize(Descriptor.RenderTargetCount);
	for (uint32_t i = 0; i < Descriptor.RenderTargetCount; ++i)
	{
		VkPipelineColorBlendAttachmentState blend{};
		blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blend.blendEnable = VK_FALSE;

		if (Descriptor.BlendState.BlendEnable && i < Descriptor.BlendState.Attachments.size())
		{
			const RBlendAttachment& attachment = Descriptor.BlendState.Attachments[i];
			blend.blendEnable = attachment.BlendEnable ? VK_TRUE : VK_FALSE;
			blend.srcColorBlendFactor = toVkBlendFactor(attachment.SrcColorBlendFactor);
			blend.dstColorBlendFactor = toVkBlendFactor(attachment.DstColorBlendFactor);
			blend.colorBlendOp = toVkBlendOp(attachment.ColorBlendOp);
			blend.srcAlphaBlendFactor = toVkBlendFactor(attachment.SrcAlphaBlendFactor);
			blend.dstAlphaBlendFactor = toVkBlendFactor(attachment.DstAlphaBlendFactor);
			blend.alphaBlendOp = toVkBlendOp(attachment.AlphaBlendOp);
		}

		colorAttachments[i] = blend;
	}

	VkPipelineColorBlendStateCreateInfo colorBlend{};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.logicOpEnable = VK_FALSE;
	colorBlend.attachmentCount = static_cast<uint32_t>(colorAttachments.size());
	colorBlend.pAttachments = colorAttachments.data();

	std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	std::vector<VkFormat> colorFormats;
	colorFormats.reserve(Descriptor.RenderTargetCount);
	for (uint32_t i = 0; i < Descriptor.RenderTargetCount; ++i)
	{
		colorFormats.push_back(toVkFormat(Descriptor.RenderTargetFormats[i]));
	}

	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
	renderingInfo.pColorAttachmentFormats = colorFormats.data();
	renderingInfo.depthAttachmentFormat = toVkFormat(Descriptor.DepthStencilFormat);
	renderingInfo.stencilAttachmentFormat = (Descriptor.DepthStencilFormat == EFormat::D24_UNorm_S8_UInt)
		? toVkFormat(Descriptor.DepthStencilFormat)
		: VK_FORMAT_UNDEFINED;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &renderingInfo;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisample;
	pipelineInfo.pDepthStencilState = isDepthFormat(Descriptor.DepthStencilFormat) ? &depthStencil : nullptr;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = layout;
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;

	VkPipeline pipeline = VK_NULL_HANDLE;
	if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		vkDestroyPipelineLayout(vkDevice, layout, nullptr);
		layout = VK_NULL_HANDLE;
	}

	return new VulkanPipeline(Device, pipeline, layout, EPipelineType::Graphics);
}

VulkanPipeline* CreateVulkanComputePipeline(VulkanDevice* Device, const RComputePipelineDescriptor& Descriptor)
{
	if (!Device || !Device->isValid())
	{
		return new VulkanPipeline(Device, VK_NULL_HANDLE, VK_NULL_HANDLE, EPipelineType::None);
	}

	auto* computeShader = dynamic_cast<VulkanShader*>(Descriptor.ComputeShader);
	if (!computeShader || !computeShader->isValid())
	{
		return new VulkanPipeline(Device, VK_NULL_HANDLE, VK_NULL_HANDLE, EPipelineType::None);
	}

	VkDevice vkDevice = Device->getVkDevice();
	VkPipelineLayout layout = CreatePipelineLayout(vkDevice);
	if (layout == VK_NULL_HANDLE)
	{
		return new VulkanPipeline(Device, VK_NULL_HANDLE, VK_NULL_HANDLE, EPipelineType::None);
	}

	VkPipelineShaderStageCreateInfo shaderStage{};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStage.module = computeShader->getVkShaderModule();
	shaderStage.pName = computeShader->getEntryPoint().c_str();

	VkComputePipelineCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.stage = shaderStage;
	createInfo.layout = layout;

	VkPipeline pipeline = VK_NULL_HANDLE;
	if (vkCreateComputePipelines(vkDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		vkDestroyPipelineLayout(vkDevice, layout, nullptr);
		layout = VK_NULL_HANDLE;
	}

	return new VulkanPipeline(Device, pipeline, layout, EPipelineType::Compute);
}

} // namespace render::rhi