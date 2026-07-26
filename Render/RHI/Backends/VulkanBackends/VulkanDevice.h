
#pragma once

#include "../../Definitions.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace render::rhi
{

class VulkanBuffer;
class VulkanTexture;
class VulkanSampler;
class VulkanShader;
class VulkanPipelineState;
class VulkanDescriptorSetLayout;
class VulkanDescriptorSet;
class VulkanCommandList;
class VulkanSwapchain;
class VulkanFence;

class VulkanDevice : public RDevice
{
public:
	VulkanDevice();
	~VulkanDevice() override;

	RBuffer* createBuffer(const RBufferDescriptor& Descriptor) override;
	RTexture* createTexture(const RTextureDescriptor& Descriptor) override;
	RSampler* createSampler(const RSamplerDescriptor& Descriptor) override;
	RShader* createShader(const RShaderDescriptor& Descriptor) override;
	RPipelineState* createGraphicsPipeline(const RGraphicsPipelineDescriptor& Descriptor) override;
	RPipelineState* createComputePipeline(const RComputePipelineDescriptor& Descriptor) override;
	RDescriptorSetLayout* createDescriptorSetLayout(const std::vector<RDescriptorSetLayoutEntry>& Descriptor) override;
	RDescriptorSet* createDescriptorSet(const RDescriptorSetDescriptor& Descriptor) override;
	RCommandList* createCommandList(ECommandQueueType Type) override;
	RSwapchain* createSwapchain(const RSwapchainDescriptor& Descriptor) override;
	RFence* createFence() override;

	void submitCommandLists(ECommandQueueType Type, const QueueSubmitDescriptor& Descriptor) override;
	void waitIdle() override;
	void destroyResource(RResource* Resource) override;

	void* getDeviceHandle() const { return DeviceHandle; }
	void* getAllocatorHandle() const { return AllocatorHandle; }

private:
	void* DeviceHandle = nullptr;
	void* AllocatorHandle = nullptr;
};
}