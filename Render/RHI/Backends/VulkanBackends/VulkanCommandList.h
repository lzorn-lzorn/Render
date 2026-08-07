
#pragma once

#include "../../RHI.h"

#include <vulkan/vulkan.h>

namespace render::rhi
{

class VulkanDevice;

class VulkanCommandList : public RCommandList
{
public:
	VulkanCommandList(VulkanDevice* InDevice, ECommandQueueType InType);
	~VulkanCommandList() override;

	void begin() override;
	void end() override;

	void beginRenderPass(const RRenderPassDescriptor& Descriptor) override;
	void endRenderPass() override;

	void setGraphicsPipeline(class RPipeline* Pipeline) override;
	void setComputePipeline(class RPipeline* Pipeline) override;
	void setDescriptorSet(uint32_t Index, class RDescriptorSet* DescriptorSet) override;
	void setVertexBuffer(uint32_t Slot, class RBuffer* Buffer) override;
	void setIndexBuffer(class RBuffer* Buffer, EIndexFormat Format) override;
	void setViewport(const RViewport& Viewport) override;
	void setScissorRect(const RRect& Rect) override;
	void setPushConstants(EShaderStage stage, uint32_t offset, uint32_t size, const void* data) override;

	void draw(uint32_t VertexCount, uint32_t InstanceCount = 1, uint32_t FirstVertex = 0, uint32_t FirstInstance = 0) override;
	void drawIndexed(uint32_t IndexCount, uint32_t InstanceCount = 1, uint32_t FirstIndex = 0, int32_t VertexOffset = 0, uint32_t FirstInstance = 0) override;

	void dispatch(uint32_t GroupX, uint32_t GroupY = 1, uint32_t GroupZ = 1) override;

	void copyBuffer(RBuffer* Src, RBuffer* Dst, const RBufferCopyDescriptor& Descriptor) override;
	void copyTexture(RImage* Src, RImage* Dst, const RTextureCopyDescriptor& Descriptor) override;

	void resourceBarrier(const RResourceBarrier& Barriers) override;
	void resourceBarriers(std::span<const RResourceBarrier> Barriers) override;

	VkCommandBuffer getVkCommandBuffer() const noexcept { return CommandBuffer; }

private:
	VulkanDevice* Device = nullptr;
	ECommandQueueType QueueType = ECommandQueueType::Graphics;
	VkCommandPool CommandPool = VK_NULL_HANDLE;
	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
	VkPipelineLayout CurrentPipelineLayout = VK_NULL_HANDLE;
	VkPipelineBindPoint CurrentBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

}