#pragma once

#include "../../Definitions.h"
#include "VulkanRHI.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace render::rhi
{

inline VkImageAspectFlags toVkImageAspectMask(ETextureAspect Aspect, EFormat TextureFormat)
{
	switch (Aspect)
	{
	case ETextureAspect::Color:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	case ETextureAspect::Depth:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	case ETextureAspect::Stencil:
		return VK_IMAGE_ASPECT_STENCIL_BIT;
	case ETextureAspect::DepthStencil:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	case ETextureAspect::Auto:
	default:
		if (IsDepthStencilFormat(TextureFormat))
		{
			if (IsDepthOnlyFormat(TextureFormat))
			{
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			if (IsStencilOnlyFormat(TextureFormat))
			{
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

inline VkImageAspectFlags toVkImageAspectMask(const RTextureViewDescriptor& ViewDescriptor, EFormat TextureFormat)
{
	if (ViewDescriptor.Aspect != ETextureAspect::Auto)
	{
		return toVkImageAspectMask(ViewDescriptor.Aspect, TextureFormat);
	}

	switch (ViewDescriptor.Type)
	{
	case RTextureViewDescriptor::EViewType::DSV:
		if (IsDepthOnlyFormat(TextureFormat))
		{
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		if (IsStencilOnlyFormat(TextureFormat))
		{
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		if (IsDepthStencilFormat(TextureFormat))
		{
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return 0;
	case RTextureViewDescriptor::EViewType::UAV:
	case RTextureViewDescriptor::EViewType::SRV:
	case RTextureViewDescriptor::EViewType::RTV:
	default:
		return toVkImageAspectMask(ETextureAspect::Auto, TextureFormat);
	}
}

class VulkanDevice;

enum class EVulkanImageOwnership : uint8_t
{
	Owned,
	External
};

struct VulkanImageResource
{
	VkImage Image = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;
	VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	EVulkanImageOwnership Ownership = EVulkanImageOwnership::Owned;

	bool isValid() const noexcept { return Image != VK_NULL_HANDLE; }
	bool ownsImage() const noexcept { return Ownership == EVulkanImageOwnership::Owned; }
	bool ownsMemory() const noexcept { return Ownership == EVulkanImageOwnership::Owned && Memory != VK_NULL_HANDLE; }
};

class VulkanBuffer : public RBuffer
{
public:
	VulkanBuffer(VulkanDevice* InDevice, const RBufferDescriptor& InBufferDesc);
	~VulkanBuffer() override;

	bool isValid() const override;
	uint64_t getSize() const override;
	bool updateData(uint64_t Offset, const void* Data, uint64_t Size) override;
	void* mapRange(EBufferMapMode MapMode, uint64_t Offset = 0, uint64_t Size = 0) override;
	void unmap() override;
	bool flushMappedRange(uint64_t Offset = 0, uint64_t Size = 0) override;
	bool invalidateMappedRange(uint64_t Offset = 0, uint64_t Size = 0) override;
	bool isCpuAccessible() const override;

	VkBuffer getVkBuffer() const noexcept { return Buffer; }
	VkDeviceMemory getVkDeviceMemory() const noexcept { return Memory; }

private:
	void setDebugName(const std::string& Name) override;

	VulkanDevice* Device = nullptr;
	RBufferDescriptor BufferDesc{};
	VkBuffer Buffer = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;
	void* MappedPtr = nullptr;
	uint64_t MappedOffset = 0;
	uint64_t MappedSize = 0;
};

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

	VkImage getVkImage() const noexcept { return ImageResource.Image; }
	VkImageAspectFlags getAspectMask() const noexcept;
	VkImageLayout getVkImageLayout() const noexcept { return ImageResource.Layout; }
	VulkanDevice* getDevice() const noexcept { return Device; }

private:
	bool initializeImage();
	bool allocateImageMemory();
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
	VulkanImageResource ImageResource{};
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
