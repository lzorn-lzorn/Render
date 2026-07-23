
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
class VulkanBindGroupLayout;
class VulkanBindGroup;
class VulkanCommandList;
class VulkanSwapchain;
class VulkanFence;

class VulkanDevice : public RDevice
{
public:
	VulkanDevice();
	~VulkanDevice() override;

	RBuffer* createBuffer(const EBufferDescriptor& Descriptor) override;
	RTexture* createTexture(const ETextureDescriptor& Descriptor) override;
	RSampler* createSampler(const ESamplerDescriptor& Descriptor) override;
	RShader* createShader(const EShaderDescriptor& Descriptor) override;
	RPipelineState* createGraphicsPipeline(const RGraphicsPipelineDescriptor& Descriptor) override;
	RPipelineState* createComputePipeline(const RComputePipelineDescriptor& Descriptor) override;
	RBindGroupLayout* createBindGroupLayout(const std::vector<RBindGroupLayoutEntry>& Descriptor) override;
	RBindGroup* createBindGroup(const RBindGroupDescriptor& Descriptor) override;
	RCommandList* createCommandList(ECommandQueueType Type) override;
	RSwapchain* createSwapchain(const ETextureDescriptor& Descriptor) override;
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