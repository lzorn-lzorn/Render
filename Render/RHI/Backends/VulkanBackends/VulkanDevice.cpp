#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "VulkanRHI.h"
#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "VulkanShader.h"
#include "VulkanPipeline.h"
#include "VulkanCommandList.h"
#include "VulkanSwapchain.h"

namespace render::rhi
{

VulkanDevice::VulkanDevice() = default;

VulkanDevice::~VulkanDevice() = default;

RBuffer* VulkanDevice::createBuffer(const RBufferDescriptor& Descriptor)
{
	return new VulkanBuffer(this, Descriptor, VkBuffer{}, VmaAllocation{});
}

RTexture* VulkanDevice::createTexture(const RTextureDescriptor& Descriptor)
{
	return new VulkanTexture(this, Descriptor, VkImage{}, VmaAllocation{});
}

RSampler* VulkanDevice::createSampler(const RSamplerDescriptor& Descriptor)
{
	return new VulkanSampler(this, Descriptor);
}

RShader* VulkanDevice::createShader(const RShaderDescriptor& Descriptor)
{
	return new VulkanShader(this, Descriptor, VkShaderModule{});
}

RPipelineState* VulkanDevice::createGraphicsPipeline(const RGraphicsPipelineDescriptor& Descriptor)
{
	(void)Descriptor;
	return new VulkanPipelineState(this, VkPipeline{}, true);
}

RPipelineState* VulkanDevice::createComputePipeline(const RComputePipelineDescriptor& Descriptor)
{
	(void)Descriptor;
	return new VulkanPipelineState(this, VkPipeline{}, false);
}

RBindGroupLayout* VulkanDevice::createBindGroupLayout(const std::vector<RBindGroupLayoutEntry>& Descriptor)
{
	return new VulkanBindGroupLayout(this, Descriptor);
}

RBindGroup* VulkanDevice::createBindGroup(const RBindGroupDescriptor& Descriptor)
{
	return new VulkanBindGroup(this, Descriptor);
}

RCommandList* VulkanDevice::createCommandList(ECommandQueueType Type)
{
	return new VulkanCommandList(this, Type);
}

RSwapchain* VulkanDevice::createSwapchain(const RTextureDescriptor& Descriptor)
{
	RSwapchainDescriptor SwapchainDescriptor;
	SwapchainDescriptor.Width = Descriptor.Width;
	SwapchainDescriptor.Height = Descriptor.Height;
	SwapchainDescriptor.Format = Descriptor.Format;
	SwapchainDescriptor.Name = Descriptor.Name;
	return new VulkanSwapchain(this, SwapchainDescriptor);
}

RFence* VulkanDevice::createFence()
{
	return new VulkanFence(this);
}

void VulkanDevice::submitCommandLists(ECommandQueueType Type, const QueueSubmitDescriptor& Descriptor)
{
	(void)Type;
	if (Descriptor.SignalFence)
	{
		Descriptor.SignalFence->signal(Descriptor.SignalValue);
	}
}

void VulkanDevice::waitIdle()
{
}

void VulkanDevice::destroyResource(RResource* Resource)
{
	delete Resource;
}

} // namespace render::rhi
