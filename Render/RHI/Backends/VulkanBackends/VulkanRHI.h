#pragma once 
#include "../../Definitions.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vector>

namespace render::rhi
{


class VulkanDevice;

class VulkanBuffer : public RBuffer
{
public:
	VulkanBuffer(VulkanDevice* InDevice, const RBufferCopyDescriptor& InBufferDesc, VkBuffer InBuffer, VmaAllocation InAlloc);

	~VulkanBuffer();
	uint64_t getSize() const override { return BufferDesc.Size; }
	VkBuffer getVkBuffer() const { return Buffer; }
	VmaAllocation getAllocation() const { return Alloc; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	RBufferCopyDescriptor BufferDesc;
	VkBuffer Buffer;
	VmaAllocation Alloc;
};

class VulkanTexture : public RTexture
{
public:
	VulkanTexture(VulkanDevice* InDevice, const RTextureCopyDescriptor& InTextureDesc, VkImage InImage, VmaAllocation InAlloc);

	~VulkanTexture();
	uint32_t getWidth() const override { return TextureDesc.Width; }
	uint32_t getHeight() const override { return TextureDesc.Height; }
	uint32_t getDepth() const override { return TextureDesc.Depth; }
	VkImage getVkImage() const { return Image; }
	VmaAllocation getAllocation() const { return Alloc; }

	RTextureView* createView(const ETextureViewDescriptor& Descriptor) override;
	RTextureView* getDefaultView() { return TextureView; }
	EFormat getFormat() const override { return TextureDesc.Format; }
private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	VulkanTextureView* TextureView = nullptr;
	RTextureCopyDescriptor TextureDesc;
	VkImage Image;
	VmaAllocation Alloc;
};


class VulkanTextureView : public RTextureView
{
public:
	virtual EResourceType getType() const override { return EResourceType::Texture; }
	virtual RTexture* getTexture() const override { return Texture; }
	virtual ETextureViewDescriptor getDescriptor() const override { return Descriptor; }
private:
	VulkanTexture* Texture;
	ETextureViewDescriptor Descriptor;
};

class VulkanSampler : public RSampler
{
public:
	virtual EResourceType getType() const override { return EResourceType::Sampler; }

};


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

class VulkanPipelineState : public RPipelineState
{
public:
	VulkanPipelineState(VulkanDevice* InDevice, VkPipeline InPipeline, bool bInIsGraphicsPipeline);
	~VulkanPipelineState();
	virtual EResourceType getType() const override { return EResourceType::Pipeline; }
	virtual bool isGraphicsPipeline() const override { return bIsGraphicsPipeline; }
	VkPipeline getVkPipeline() const { return Pipeline; }
private:
	bool bIsGraphicsPipeline;
	VkPipeline Pipeline;
};

class VulkanBindGroupLayout : public RBindGroupLayout
{
public:
	virtual EResourceType getType() const override { return EResourceType::BindGroupLayout; }

};

class VulkanBindGroup : public RBindGroup
{
public:
	virtual EResourceType getType() const override { return EResourceType::BindGroup; }

};

class VulkanCommandList : public RHICommandList { /* 持有 VkCommandBuffer */ };
class VulkanSwapChain : public RHISwapChain { /* 封装 VkSwapchainKHR */ };
class VulkanFence : public RFence {};

class VulkanDevice : public RDevice
{
public:
	VulkanDevice();
	~VulkanDevice() override;

	RBuffer* createBuffer(const EBufferDescriptor& Descriptor) override;
	RTexture* createTexture(const ETextureDescriptor& Descriptor) override;
	RSampler* createSampler(const ESamplerDescriptor& Descriptor) override;
	RPipelineState* createPipelineState(const EPipelineStateDescriptor& Descriptor) override;
	RBindGroupLayout* createBindGroupLayout(const EBindGroupLayoutDescriptor& Descriptor) override;
	RBindGroup* createBindGroup(const EBindGroupDescriptor& Descriptor) override;
	RFence* createFence(const EFenceDescriptor& Descriptor) override;
	RSwapchain* createSwapchain(const ESwapchainDescriptor& Descriptor) override;
	RCommandList* createCommandList(CommandQueueType Type) override;

	void submitCommandLists(CommandQueueType Type, const QueueSubmitDescriptor& Descriptor) override;
	void waitIdle() override;
	void destroyResource(RResource* Resource) override;
};

} // namespace rhi
