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

	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = Device->getQueueFamilyIndex(QueueType);

	if (vkCreateCommandPool(Device->getVkDevice(), &pool_info, nullptr, &CommandPool) != VK_SUCCESS)
	{
		CommandPool = VK_NULL_HANDLE;
		return;
	}

	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = CommandPool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(Device->getVkDevice(), &alloc_info, &CommandBuffer) != VK_SUCCESS)
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

	VkDevice vk_device = Device->getVkDevice();
	if (vk_device == VK_NULL_HANDLE)
	{
		return;
	}

	if (CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(vk_device, CommandPool, nullptr);
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

	std::vector<VkRenderingAttachmentInfo> color_attachments;
	color_attachments.reserve(Descriptor.ColorAttachmentCount);

	uint32_t render_width = 1;
	uint32_t render_height = 1;

	for (uint32_t i = 0; i < Descriptor.ColorAttachmentCount; ++i)
	{
		const RRenderTargetAttachment& attachment = Descriptor.ColorAttachments[i];
		auto* texture_view = static_cast<VulkanTextureView*>(attachment.TextureView);
		if (!texture_view || !texture_view->isValid())
		{
			continue;
		}

		auto* texture = static_cast<VulkanTexture*>(texture_view->getTexture());
		if (texture)
		{
			render_width = texture->getWidth();
			render_height = texture->getHeight();
		}

		VkRenderingAttachmentInfo color_info{};
		color_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color_info.imageView = texture_view->getVkImageView();
		color_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_info.loadOp = attachment.LoadOp == ELoadOp::Clear
			? VK_ATTACHMENT_LOAD_OP_CLEAR
			: (attachment.LoadOp == ELoadOp::Load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE);
		color_info.storeOp = attachment.StoreOp == EStoreOp::Store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_info.clearValue.color = {
			attachment.ClearValue.Color[0],
			attachment.ClearValue.Color[1],
			attachment.ClearValue.Color[2],
			attachment.ClearValue.Color[3]
		};
		color_attachments.push_back(color_info);
	}

	VkRenderingAttachmentInfo depth_info{};
	VkRenderingAttachmentInfo* depth_info_ptr = nullptr;
	if (Descriptor.DepthStencilAttachment && Descriptor.DepthStencilAttachment->TextureView)
	{
		auto* texture_view = static_cast<VulkanTextureView*>(Descriptor.DepthStencilAttachment->TextureView);
		if (texture_view && texture_view->isValid())
		{
			auto* texture = static_cast<VulkanTexture*>(texture_view->getTexture());
			if (texture)
			{
				render_width = texture->getWidth();
				render_height = texture->getHeight();
			}

			depth_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depth_info.imageView = texture_view->getVkImageView();
			depth_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depth_info.loadOp = Descriptor.DepthStencilAttachment->DepthLoadOp == ELoadOp::Clear
				? VK_ATTACHMENT_LOAD_OP_CLEAR
				: (Descriptor.DepthStencilAttachment->DepthLoadOp == ELoadOp::Load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE);
			depth_info.storeOp = Descriptor.DepthStencilAttachment->DepthStoreOp == EStoreOp::Store
				? VK_ATTACHMENT_STORE_OP_STORE
				: VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depth_info.clearValue.depthStencil = {
				Descriptor.DepthStencilAttachment->ClearValue.Depth,
				Descriptor.DepthStencilAttachment->ClearValue.Stencil
			};
			depth_info_ptr = &depth_info;
		}
	}

	VkRenderingInfo rendering_info{};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	rendering_info.renderArea.offset = { 0, 0 };
	rendering_info.renderArea.extent = { render_width, render_height };
	rendering_info.layerCount = 1;
	rendering_info.colorAttachmentCount = static_cast<uint32_t>(color_attachments.size());
	rendering_info.pColorAttachments = color_attachments.data();
	rendering_info.pDepthAttachment = depth_info_ptr;
	rendering_info.pStencilAttachment = depth_info_ptr;

	vkCmdBeginRendering(CommandBuffer, &rendering_info);
}

void VulkanCommandList::endRenderPass()
{
	if (CommandBuffer != VK_NULL_HANDLE)
	{
		vkCmdEndRendering(CommandBuffer);
	}
}

void VulkanCommandList::setGraphicsPipeline(RPipeline* Pipeline)
{
	auto* vk_pipeline = static_cast<VulkanPipeline*>(Pipeline);
	if (!vk_pipeline || !vk_pipeline->isValid() || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	CurrentPipelineLayout = vk_pipeline->getVkPipelineLayout();
	CurrentBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->getVkPipeline());
}

void VulkanCommandList::setComputePipeline(RPipeline* Pipeline)
{
	auto* vk_pipeline = static_cast<VulkanPipeline*>(Pipeline);
	if (!vk_pipeline || !vk_pipeline->isValid() || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	CurrentPipelineLayout = vk_pipeline->getVkPipelineLayout();
	CurrentBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk_pipeline->getVkPipeline());
}

void VulkanCommandList::setDescriptorSet(uint32_t Index, RDescriptorSet* DescriptorSet)
{
	auto* vk_descriptor_set = static_cast<VulkanDescriptorSet*>(DescriptorSet);
	if (!vk_descriptor_set || CommandBuffer == VK_NULL_HANDLE || CurrentPipelineLayout == VK_NULL_HANDLE)
	{
		return;
	}

	VkDescriptorSet set = vk_descriptor_set->getVkDescriptorSet();
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
	auto* vk_buffer = static_cast<VulkanBuffer*>(Buffer);
	if (!vk_buffer || !vk_buffer->isValid() || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	const VkBuffer buffer = vk_buffer->getVkBuffer();
	const VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(CommandBuffer, Slot, 1, &buffer, &offset);
}

void VulkanCommandList::setIndexBuffer(RBuffer* Buffer, EIndexFormat Format)
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(Buffer);
	if (!vk_buffer || !vk_buffer->isValid() || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	vkCmdBindIndexBuffer(CommandBuffer, vk_buffer->getVkBuffer(), 0, ToVkIndexType(Format));
}

void VulkanCommandList::setViewport(const RViewport& Viewport)
{
	if (CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VkViewport vk_viewport{};
	vk_viewport.x = Viewport.X;
	vk_viewport.y = Viewport.Y;
	vk_viewport.width = Viewport.Width;
	vk_viewport.height = Viewport.Height;
	vk_viewport.minDepth = Viewport.MinDepth;
	vk_viewport.maxDepth = Viewport.MaxDepth;
	vkCmdSetViewport(CommandBuffer, 0, 1, &vk_viewport);
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
	auto* src_buffer = static_cast<VulkanBuffer*>(Src);
	auto* dst_buffer = static_cast<VulkanBuffer*>(Dst);
	if (!src_buffer || !dst_buffer || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VkBufferCopy copy_region{};
	copy_region.srcOffset = Descriptor.SrcOffset;
	copy_region.dstOffset = Descriptor.DstOffset;
	copy_region.size = Descriptor.Size;
	vkCmdCopyBuffer(CommandBuffer, src_buffer->getVkBuffer(), dst_buffer->getVkBuffer(), 1, &copy_region);
}

void VulkanCommandList::copyTexture(RTexture* Src, RTexture* Dst, const RTextureCopyDescriptor& Descriptor)
{
	auto* src_texture = static_cast<VulkanTexture*>(Src);
	auto* dst_texture = static_cast<VulkanTexture*>(Dst);
	if (!src_texture || !dst_texture || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VkImageCopy copy_region{};
	copy_region.srcSubresource.aspectMask = src_texture->getAspectMask();
	copy_region.srcSubresource.mipLevel = Descriptor.SrcMipLevel;
	copy_region.srcSubresource.baseArrayLayer = Descriptor.SrcArrayLayer;
	copy_region.srcSubresource.layerCount = 1;
	copy_region.dstSubresource.aspectMask = dst_texture->getAspectMask();
	copy_region.dstSubresource.mipLevel = Descriptor.DstMipLevel;
	copy_region.dstSubresource.baseArrayLayer = Descriptor.DstArrayLayer;
	copy_region.dstSubresource.layerCount = 1;
	copy_region.extent = { Descriptor.Width, Descriptor.Height, Descriptor.Depth };

	vkCmdCopyImage(
		CommandBuffer,
		src_texture->getVkImage(),
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst_texture->getVkImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&copy_region);
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
		auto* texture = static_cast<VulkanTexture*>(barrier.Texture);
		if (!texture || !texture->isValid())
		{
			continue;
		}

		VkImageMemoryBarrier image_barrier{};
		image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_barrier.srcAccessMask = ToVkAccessMask(barrier.Before);
		image_barrier.dstAccessMask = ToVkAccessMask(barrier.After);
		image_barrier.oldLayout = ToVkImageLayout(barrier.Before);
		image_barrier.newLayout = ToVkImageLayout(barrier.After);
		image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_barrier.image = texture->getVkImage();
		image_barrier.subresourceRange.aspectMask = texture->getAspectMask();
		image_barrier.subresourceRange.baseMipLevel = 0;
		image_barrier.subresourceRange.levelCount = 1;
		image_barrier.subresourceRange.baseArrayLayer = 0;
		image_barrier.subresourceRange.layerCount = 1;

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
			&image_barrier);
	}
}

} // namespace render::rhi
