#pragma once

#include "../../Definitions.h"
#include "VulkanImage.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace render::rhi
{

class VulkanDevice;

class VulkanTextureView;

class VulkanTexture : public RImage
{
public:
	VulkanTexture(VulkanDevice* InDevice, const RImageDescriptor& InImageDesc);
	VulkanTexture(VulkanDevice* InDevice, const RImageDescriptor& InImageDesc, const RTextureBulkData& InBulkData);
	VulkanTexture(VulkanDevice* InDevice, const RImageDescriptor& InImageDesc, VkImage InExternalImage, VkImageView InExternalView);
	~VulkanTexture() override;

	bool isValid() const override;
	const RImageDescriptor& getDescriptor() const override { return ImageDescriptor; }

	void updateTexture(const RTextureUpdateRegion& Region, const void* SrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch = 0) override;
	void generateMipmaps() override;
	RTextureView* createView(const RTextureViewDescriptor& Descriptor) override;

	RTextureView* getDefaultView() const noexcept { return DefaultView; }
	VkImage getVkImage() const noexcept;
	VkImageAspectFlags getAspectMask() const noexcept;
	VkImageLayout getVkImageLayout() const noexcept;
	VulkanDevice* getDevice() const noexcept { return Device; }

private:
	bool uploadBulkData(const RTextureBulkData& InBulkData);
	bool transitionImageLayout(
		VkCommandBuffer CommandBuffer,
		VkImageLayout NewLayout,
		VkPipelineStageFlags SrcStage,
		VkPipelineStageFlags DstStage,
		VkAccessFlags SrcAccess,
		VkAccessFlags DstAccess,
		const VkImageSubresourceRange* SubresourceRange = nullptr);
	VkImageSubresourceRange buildSubresourceRange(
		uint32_t BaseMipLevel,
		uint32_t MipLevelCount,
		uint32_t BaseArrayLayer,
		uint32_t ArrayLayerCount) const;
	RTextureViewDescriptor buildDefaultViewDescriptor() const;
	void setDebugName(const std::string& Name) override;

private:
	VulkanDevice* Device = nullptr;
	RImageDescriptor ImageDescriptor{};
	std::unique_ptr<VulkanImage> Image;
	RTextureView* DefaultView = nullptr;
	std::vector<RTextureView*> Views;
};

class VulkanTextureView : public RTextureView
{
public:
	VulkanTextureView(VulkanTexture* InImage, const RTextureViewDescriptor& InDescriptor, VkImageView InView, bool InOwnsView);
	~VulkanTextureView() override;

	bool isValid() const override;
	RImage* getImage() const override { return Image; }
	const RTextureViewDescriptor& getDescriptor() const override { return Descriptor; }
	VkImageView getVkImageView() const noexcept { return View; }

private:
	void setDebugName(const std::string& Name) override;

	VulkanTexture* Image = nullptr;
	RTextureViewDescriptor Descriptor{};
	VkImageView View = VK_NULL_HANDLE;
	bool OwnsView = true;
};

} // namespace render::rhi
