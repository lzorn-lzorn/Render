#pragma once

#include "../../Definitions.h"
#include "VulkanImage.h"
#include "VulkanRHI.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace render::rhi
{

class VulkanDevice;

class VulkanTextureView;

class VulkanTexture : public RTexture
{
public:
	VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc);
	VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, const RTextureBulkData& InBulkData);
	VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, VkImage InExternalImage, VkImageView InExternalView);
	~VulkanTexture() override;

	bool isValid() const override;
	const RTextureDescriptor& getDescriptor() const override { return TextureDesc; }

	void updateTexture(const RTextureUpdateRegion& Region, const void* SrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch = 0) override;
	void generateMipmaps() override;

	uint32_t getWidth() const override { return TextureDesc.Width; }
	uint32_t getHeight() const override { return TextureDesc.Height; }
	uint32_t getDepth() const override { return TextureDesc.Depth; }
	EFormat getFormat() const override { return TextureDesc.Format; }

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

	VulkanDevice* Device = nullptr;
	RTextureDescriptor TextureDesc{};
	std::unique_ptr<VulkanImage> Image;
	RTextureView* DefaultView = nullptr;
	std::vector<RTextureView*> Views;
};

class VulkanTextureView : public RTextureView
{
public:
	VulkanTextureView(VulkanTexture* InTexture, const RTextureViewDescriptor& InDescriptor, VkImageView InView, bool bInOwnsView);
	~VulkanTextureView() override;

	bool isValid() const override;

	RTexture* getTexture() const override { return Texture; }
	RTextureViewDescriptor getDescriptor() const override { return Descriptor; }
	VkImageView getVkImageView() const noexcept { return View; }

private:
	void setDebugName(const std::string& Name) override;

	VulkanTexture* Texture = nullptr;
	RTextureViewDescriptor Descriptor{};
	VkImageView View = VK_NULL_HANDLE;
	bool OwnsView = true;
};

class VulkanSampler : public RSampler
{
public:
	VulkanSampler(VulkanDevice* InDevice, const RSamplerDescriptor& InSamplerDesc);
	~VulkanSampler() override;

	bool isValid() const override;
	VkSampler getVkSampler() const noexcept { return Sampler; }

private:
	void setDebugName(const std::string& Name) override;

	VulkanDevice* Device = nullptr;
	RSamplerDescriptor SamplerDesc{};
	VkSampler Sampler = VK_NULL_HANDLE;
};

} // namespace render::rhi
