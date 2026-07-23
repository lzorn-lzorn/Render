
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "../../Definitions.h"

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

	void setGraphicsPipeline(class RPipelineState* Pipeline) override;
	void setComputePipeline(class RPipelineState* Pipeline) override;
	void setBindGroup(uint32_t Index, class RBindGroup* BindGroup) override;
	void setVertexBuffer(uint32_t Slot, class RBuffer* Buffer) override;
	void setIndexBuffer(class RBuffer* Buffer, EIndexFormat Format) override;
	void setViewport(const RViewport& Viewport) override;
	void setScissorRect(const RRect& Rect) override;

	void draw(uint32_t VertexCount, uint32_t InstanceCount = 1, uint32_t FirstVertex = 0, uint32_t FirstInstance = 0) override;
	void drawIndexed(uint32_t IndexCount, uint32_t InstanceCount = 1, uint32_t FirstIndex = 0, int32_t VertexOffset = 0, uint32_t FirstInstance = 0) override;

	void dispatch(uint32_t GroupX, uint32_t GroupY = 1, uint32_t GroupZ = 1) override;

	void copyBuffer(RBuffer* Src, RBuffer* Dst, const RBufferCopyDescriptor& Descriptor) override;
	void copyTexture(RTexture* Src, RTexture* Dst, const RTextureCopyDescriptor& Descriptor) override;

	void resourceBarrier(const RResourceBarrier& Barriers) override;
	void resourceBarriers(std::span<const RResourceBarrier> Barriers) override;

private:
	VulkanDevice* Device;
	ECommandQueueType QueueType;
	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
};

}