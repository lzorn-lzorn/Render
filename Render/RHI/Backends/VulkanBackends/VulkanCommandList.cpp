#include "VulkanRHI.h"
#include "VulkanCommandList.h"
#include "VulkanDevice.h"

namespace render::rhi
{

VulkanCommandList::VulkanCommandList(VulkanDevice* InDevice, ECommandQueueType InType)
	: Device(InDevice)
	, QueueType(InType)
{
}

VulkanCommandList::~VulkanCommandList() = default;

void VulkanCommandList::begin()
{
}

void VulkanCommandList::end()
{
}

void VulkanCommandList::beginRenderPass(const RRenderPassDescriptor& Descriptor)
{
	(void)Descriptor;
}

void VulkanCommandList::endRenderPass()
{
}

void VulkanCommandList::setGraphicsPipeline(RPipelineState* Pipeline)
{
	(void)Pipeline;
}

void VulkanCommandList::setComputePipeline(RPipelineState* Pipeline)
{
	(void)Pipeline;
}

void VulkanCommandList::setVertexBuffer(uint32_t Slot, RBuffer* Buffer)
{
	(void)Slot;
	(void)Buffer;
}

void VulkanCommandList::setIndexBuffer(RBuffer* Buffer, EIndexFormat Format)
{
	(void)Buffer;
	(void)Format;
}

void VulkanCommandList::setViewport(const RViewport& Viewport)
{
	(void)Viewport;
}

void VulkanCommandList::setScissorRect(const RRect& Rect)
{
	(void)Rect;
}

void VulkanCommandList::draw(uint32_t VertexCount, uint32_t InstanceCount, uint32_t FirstVertex, uint32_t FirstInstance)
{
	(void)VertexCount;
	(void)InstanceCount;
	(void)FirstVertex;
	(void)FirstInstance;
}

void VulkanCommandList::drawIndexed(uint32_t IndexCount, uint32_t InstanceCount, uint32_t FirstIndex, int32_t VertexOffset, uint32_t FirstInstance)
{
	(void)IndexCount;
	(void)InstanceCount;
	(void)FirstIndex;
	(void)VertexOffset;
	(void)FirstInstance;
}

void VulkanCommandList::dispatch(uint32_t GroupX, uint32_t GroupY, uint32_t GroupZ)
{
	(void)GroupX;
	(void)GroupY;
	(void)GroupZ;
}

void VulkanCommandList::copyBuffer(RBuffer* Src, RBuffer* Dst, const RBufferCopyDescriptor& Descriptor)
{
	(void)Src;
	(void)Dst;
	(void)Descriptor;
}

void VulkanCommandList::copyTexture(RTexture* Src, RTexture* Dst, const RTextureCopyDescriptor& Descriptor)
{
	(void)Src;
	(void)Dst;
	(void)Descriptor;
}

void VulkanCommandList::resourceBarrier(const RResourceBarrier& Barriers)
{
	(void)Barriers;
}

void VulkanCommandList::resourceBarriers(std::span<const RResourceBarrier> Barriers)
{
	(void)Barriers;
}

} // namespace render::rhi
