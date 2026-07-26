#include "VulkanRHI.h"
#include "VulkanCommandList.h"

#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanResources.h"

#include <vector>

namespace render::rhi
{

VulkanCommandList::VulkanCommandList(VulkanDevice* InDevice, ECommandQueueType InType)
	: Device(InDevice)
	, QueueType(InType)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = Device->getQueueFamilyIndex(QueueType);

	if (vkCreateCommandPool(Device->getVkDevice(), &poolInfo, nullptr, &CommandPool) != VK_SUCCESS)
	{
		CommandPool = VK_NULL_HANDLE;
		return;
	}

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(Device->getVkDevice(), &allocInfo, &CommandBuffer) != VK_SUCCESS)
	{
		CommandBuffer = VK_NULL_HANDLE;
	}
}

VulkanCommandList::~VulkanCommandList()
{
	if (!Device)
	{
		return;
	}

	VkDevice vkDevice = Device->getVkDevice();
	if (vkDevice == VK_NULL_HANDLE)
	{
		return;
	}

	if (CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(vkDevice, CommandPool, nullptr);
		CommandPool = VK_NULL_HANDLE;
		CommandBuffer = VK_NULL_HANDLE;
	}
}

void VulkanCommandList::begin()
{
	if (CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	vkResetCommandBuffer(CommandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(CommandBuffer, &beginInfo);
}

void VulkanCommandList::end()
{
	if (CommandBuffer != VK_NULL_HANDLE)
	{
		vkEndCommandBuffer(CommandBuffer);
	}
}

void VulkanCommandList::beginRenderPass(const RRenderPassDescriptor& Descriptor)
{
	if (CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	std::vector<VkRenderingAttachmentInfo> colorAttachments;
	colorAttachments.reserve(Descriptor.ColorAttachmentCount);

	uint32_t renderWidth = 1;
	uint32_t renderHeight = 1;

	for (uint32_t i = 0; i < Descriptor.ColorAttachmentCount; ++i)
	{
		const RRenderTargetAttachment& attachment = Descriptor.ColorAttachments[i];
		auto* textureView = dynamic_cast<VulkanTextureView*>(attachment.TextureView);
		if (!textureView || !textureView->isValid())
		{
			continue;
		}

		auto* texture = dynamic_cast<VulkanTexture*>(textureView->getTexture());
		if (texture)
		{
			renderWidth = texture->getWidth();
			renderHeight = texture->getHeight();
		}

		VkRenderingAttachmentInfo colorInfo{};
		colorInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorInfo.imageView = textureView->getVkImageView();
		colorInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorInfo.loadOp = attachment.LoadOp == ELoadOp::Clear
			? VK_ATTACHMENT_LOAD_OP_CLEAR
			: (attachment.LoadOp == ELoadOp::Load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE);
		colorInfo.storeOp = attachment.StoreOp == EStoreOp::Store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorInfo.clearValue.color = {
			attachment.ClearValue.Color[0],
			attachment.ClearValue.Color[1],
			attachment.ClearValue.Color[2],
			attachment.ClearValue.Color[3]
		};
		colorAttachments.push_back(colorInfo);
	}

	VkRenderingAttachmentInfo depthInfo{};
	VkRenderingAttachmentInfo* depthInfoPtr = nullptr;
	if (Descriptor.DepthStencilAttachment && Descriptor.DepthStencilAttachment->TextureView)
	{
		auto* textureView = dynamic_cast<VulkanTextureView*>(Descriptor.DepthStencilAttachment->TextureView);
		if (textureView && textureView->isValid())
		{
			auto* texture = dynamic_cast<VulkanTexture*>(textureView->getTexture());
			if (texture)
			{
				renderWidth = texture->getWidth();
				renderHeight = texture->getHeight();
			}

			depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthInfo.imageView = textureView->getVkImageView();
			depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthInfo.loadOp = Descriptor.DepthStencilAttachment->DepthLoadOp == ELoadOp::Clear
				? VK_ATTACHMENT_LOAD_OP_CLEAR
				: (Descriptor.DepthStencilAttachment->DepthLoadOp == ELoadOp::Load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE);
			depthInfo.storeOp = Descriptor.DepthStencilAttachment->DepthStoreOp == EStoreOp::Store
				? VK_ATTACHMENT_STORE_OP_STORE
				: VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthInfo.clearValue.depthStencil = {
				Descriptor.DepthStencilAttachment->ClearValue.Depth,
				Descriptor.DepthStencilAttachment->ClearValue.Stencil
			};
			depthInfoPtr = &depthInfo;
		}
	}

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = { renderWidth, renderHeight };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
	renderingInfo.pColorAttachments = colorAttachments.data();
	renderingInfo.pDepthAttachment = depthInfoPtr;
	renderingInfo.pStencilAttachment = depthInfoPtr;

	vkCmdBeginRendering(CommandBuffer, &renderingInfo);
}

void VulkanCommandList::endRenderPass()
{
	if (CommandBuffer != VK_NULL_HANDLE)
	{
		vkCmdEndRendering(CommandBuffer);
	}
}

void VulkanCommandList::setGraphicsPipeline(RPipelineState* Pipeline)
{
	auto* vkPipeline = dynamic_cast<VulkanPipelineState*>(Pipeline);
	if (!vkPipeline || !vkPipeline->isValid() || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	CurrentPipelineLayout = vkPipeline->getVkPipelineLayout();
	CurrentBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->getVkPipeline());
}

void VulkanCommandList::setComputePipeline(RPipelineState* Pipeline)
{
	auto* vkPipeline = dynamic_cast<VulkanPipelineState*>(Pipeline);
	if (!vkPipeline || !vkPipeline->isValid() || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	CurrentPipelineLayout = vkPipeline->getVkPipelineLayout();
	CurrentBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->getVkPipeline());
}

void VulkanCommandList::setDescriptorSet(uint32_t Index, RDescriptorSet* DescriptorSet)
{
	auto* vkDescriptorSet = dynamic_cast<VulkanDescriptorSet*>(DescriptorSet);
	if (!vkDescriptorSet || CommandBuffer == VK_NULL_HANDLE || CurrentPipelineLayout == VK_NULL_HANDLE)
	{
		return;
	}

	VkDescriptorSet set = vkDescriptorSet->getVkDescriptorSet();
	if (set == VK_NULL_HANDLE)
	{
		return;
	}

	vkCmdBindDescriptorSets(
		CommandBuffer,
		CurrentBindPoint,
		CurrentPipelineLayout,
		Index,
		1,
		&set,
		0,
		nullptr);
}

void VulkanCommandList::setVertexBuffer(uint32_t Slot, RBuffer* Buffer)
{
	auto* vkBuffer = dynamic_cast<VulkanBuffer*>(Buffer);
	if (!vkBuffer || !vkBuffer->isValid() || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	const VkBuffer buffer = vkBuffer->getVkBuffer();
	const VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(CommandBuffer, Slot, 1, &buffer, &offset);
}

void VulkanCommandList::setIndexBuffer(RBuffer* Buffer, EIndexFormat Format)
{
	auto* vkBuffer = dynamic_cast<VulkanBuffer*>(Buffer);
	if (!vkBuffer || !vkBuffer->isValid() || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	vkCmdBindIndexBuffer(CommandBuffer, vkBuffer->getVkBuffer(), 0, ToVkIndexType(Format));
}

void VulkanCommandList::setViewport(const RViewport& Viewport)
{
	if (CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VkViewport vkViewport{};
	vkViewport.x = Viewport.X;
	vkViewport.y = Viewport.Y;
	vkViewport.width = Viewport.Width;
	vkViewport.height = Viewport.Height;
	vkViewport.minDepth = Viewport.MinDepth;
	vkViewport.maxDepth = Viewport.MaxDepth;
	vkCmdSetViewport(CommandBuffer, 0, 1, &vkViewport);
}

void VulkanCommandList::setScissorRect(const RRect& Rect)
{
	if (CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VkRect2D scissor{};
	scissor.offset = { Rect.X, Rect.Y };
	scissor.extent = { Rect.Width, Rect.Height };
	vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);
}

void VulkanCommandList::setPushConstants(EShaderStage stage, uint32_t offset, uint32_t size, const void* data)
{
	if (CommandBuffer == VK_NULL_HANDLE || CurrentPipelineLayout == VK_NULL_HANDLE || data == nullptr || size == 0)
	{
		return;
	}

	VkShaderStageFlags flags = ToVkShaderStageFlags(stage);
	if (flags == 0)
	{
		flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	}

	vkCmdPushConstants(CommandBuffer, CurrentPipelineLayout, flags, offset, size, data);
}

void VulkanCommandList::draw(uint32_t VertexCount, uint32_t InstanceCount, uint32_t FirstVertex, uint32_t FirstInstance)
{
	if (CommandBuffer != VK_NULL_HANDLE)
	{
		vkCmdDraw(CommandBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
	}
}

void VulkanCommandList::drawIndexed(uint32_t IndexCount, uint32_t InstanceCount, uint32_t FirstIndex, int32_t VertexOffset, uint32_t FirstInstance)
{
	if (CommandBuffer != VK_NULL_HANDLE)
	{
		vkCmdDrawIndexed(CommandBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
	}
}

void VulkanCommandList::dispatch(uint32_t GroupX, uint32_t GroupY, uint32_t GroupZ)
{
	if (CommandBuffer != VK_NULL_HANDLE)
	{
		vkCmdDispatch(CommandBuffer, GroupX, GroupY, GroupZ);
	}
}

void VulkanCommandList::copyBuffer(RBuffer* Src, RBuffer* Dst, const RBufferCopyDescriptor& Descriptor)
{
	auto* srcBuffer = dynamic_cast<VulkanBuffer*>(Src);
	auto* dstBuffer = dynamic_cast<VulkanBuffer*>(Dst);
	if (!srcBuffer || !dstBuffer || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = Descriptor.SrcOffset;
	copyRegion.dstOffset = Descriptor.DstOffset;
	copyRegion.size = Descriptor.Size;
	vkCmdCopyBuffer(CommandBuffer, srcBuffer->getVkBuffer(), dstBuffer->getVkBuffer(), 1, &copyRegion);
}

void VulkanCommandList::copyTexture(RTexture* Src, RTexture* Dst, const RTextureCopyDescriptor& Descriptor)
{
	auto* srcTexture = dynamic_cast<VulkanTexture*>(Src);
	auto* dstTexture = dynamic_cast<VulkanTexture*>(Dst);
	if (!srcTexture || !dstTexture || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VkImageCopy copyRegion{};
	copyRegion.srcSubresource.aspectMask = srcTexture->getAspectMask();
	copyRegion.srcSubresource.mipLevel = Descriptor.SrcMipLevel;
	copyRegion.srcSubresource.baseArrayLayer = Descriptor.SrcArrayLayer;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.dstSubresource.aspectMask = dstTexture->getAspectMask();
	copyRegion.dstSubresource.mipLevel = Descriptor.DstMipLevel;
	copyRegion.dstSubresource.baseArrayLayer = Descriptor.DstArrayLayer;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.extent = { Descriptor.Width, Descriptor.Height, Descriptor.Depth };

	vkCmdCopyImage(
		CommandBuffer,
		srcTexture->getVkImage(),
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstTexture->getVkImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&copyRegion);
}

void VulkanCommandList::resourceBarrier(const RResourceBarrier& Barriers)
{
	resourceBarriers(std::span<const RResourceBarrier>(&Barriers, 1));
}

void VulkanCommandList::resourceBarriers(std::span<const RResourceBarrier> Barriers)
{
	if (CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	for (const RResourceBarrier& barrier : Barriers)
	{
		auto* texture = dynamic_cast<VulkanTexture*>(barrier.Texture);
		if (!texture || !texture->isValid())
		{
			continue;
		}

		VkImageMemoryBarrier imageBarrier{};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.srcAccessMask = ToVkAccessMask(barrier.Before);
		imageBarrier.dstAccessMask = ToVkAccessMask(barrier.After);
		imageBarrier.oldLayout = ToVkImageLayout(barrier.Before);
		imageBarrier.newLayout = ToVkImageLayout(barrier.After);
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = texture->getVkImage();
		imageBarrier.subresourceRange.aspectMask = texture->getAspectMask();
		imageBarrier.subresourceRange.baseMipLevel = 0;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			CommandBuffer,
			ToVkPipelineStage(barrier.Before),
			ToVkPipelineStage(barrier.After),
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&imageBarrier);
	}
}

} // namespace render::rhi
