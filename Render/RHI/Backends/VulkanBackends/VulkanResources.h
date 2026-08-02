#pragma once

#include "../../Definitions.h"
#include "VulkanRHI.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace render::rhi
{

inline VkImageAspectFlags AspectMask(const RTextureViewDescriptor::EViewType& ViewType, const EFormat TextureFormat)
{
	// 根据视图类型决定默认 aspect 策略
	switch (ViewType) {
		case RTextureViewDescriptor::EViewType::DSV:
		{
			// 深度模板视图：必须考虑深度/模板分量
			if (IsDepthStencilFormat(TextureFormat)) {
				// 混合格式默认同时包含深度和模板
				return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			if (IsDepthOnlyFormat(TextureFormat)) {
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			if (IsStencilOnlyFormat(TextureFormat)) {
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			// 其他格式不能作为 DSV，这里可记录错误并返回 0
			return 0;
		}

		case RTextureViewDescriptor::EViewType::SRV:
		{
			// 着色器资源视图：可以读取深度或模板，也可能读取整个纹理
			if (IsDepthStencilFormat(TextureFormat)) {
				// 默认：深度+模板都读取（用于阴影贴图比较、纹理采样等）
				// 但如果视图希望只读取深度（如以 R32 格式读取深度），可以
				// 通过 TextureFormat 进一步细化。例如若 TextureFormat 为 D32_Float
				// 则已经是 depth-only，下面的 IsDepthOnlyFormat 会捕获。
				if (IsDepthOnlyFormat(TextureFormat)) {
					return VK_IMAGE_ASPECT_DEPTH_BIT;
				}
				// 如果 TextureFormat 仍然是 D24_S8 混合格式，说明要完整采样
				return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			if (IsDepthOnlyFormat(TextureFormat)) {
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			if (IsStencilOnlyFormat(TextureFormat)) {
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			// 颜色或其它格式
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}

		case RTextureViewDescriptor::EViewType::UAV:
		{
			// 无序访问视图：一般只有颜色或深度/模板单独分量可写入
			if (IsDepthOnlyFormat(TextureFormat) || IsDepthStencilFormat(TextureFormat)) {
				// 某些平台允许以 UAV 写入深度（如 R32_UINT 映射），通常返回深度 aspect
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			if (IsStencilOnlyFormat(TextureFormat)) {
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}

		default:
			// 未知视图类型，回退到格式驱动的默认 mask
			if (IsDepthStencilFormat(TextureFormat)) 
				return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			if (IsDepthOnlyFormat(TextureFormat)) 
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			if (IsStencilOnlyFormat(TextureFormat)) 
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}


class VulkanDevice;

class VulkanBuffer : public RBuffer
{
public:
	VulkanBuffer(VulkanDevice* InDevice, const RBufferDescriptor& InBufferDesc);
	~VulkanBuffer() override;

	bool isValid() const override;
	uint64_t getSize() const override;
	VkBuffer getVkBuffer() const noexcept { return Buffer; }

private:
	void setDebugName(const std::string& Name) override;

	VulkanDevice* Device = nullptr;
	RBufferDescriptor BufferDesc{};
	VkBuffer Buffer = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;
};

class VulkanTextureView;

class VulkanTexture : public RTexture
{
public:
	VkImageAspectFlags GetAspectMask(EFormat format) {
        if (IsDepthStencilFormat(format)) {
            // 包含深度和模板两个分量
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        } 
        else if (IsDepthOnlyFormat(format)) {
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else if (IsStencilOnlyFormat(format)) {
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else {
            // 所有颜色格式，包括压缩格式、YUV 等
            return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }
public:
	VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc);
	VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, const RTextureBulkData& InBulkData);
	VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, VkImage InExternalImage, VkImageView InExternalView);
	~VulkanTexture() override;

	bool isValid() const override;

	void updateTexture(const RTextureUpdateRegion& Region, const void* SrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch = 0) override;

	void generateMipmaps() override;
	uint32_t getWidth() const override { return TextureDesc.Width; }
	uint32_t getHeight() const override { return TextureDesc.Height; }
	uint32_t getDepth() const override { return TextureDesc.Depth; }
	EFormat getFormat() const override { return TextureDesc.Format; }

	RTextureView* createView(const RTextureViewDescriptor& Descriptor) override;
	RTextureView* getDefaultView() const noexcept { return DefaultView; }

	VkImage getVkImage() const noexcept { return Image; }
	VkImageAspectFlags getAspectMask() const noexcept;
	VulkanDevice* getDevice() const noexcept { return Device; }

private:
	void setDebugName(const std::string& Name) override;

	VulkanDevice* Device = nullptr;
	RTextureDescriptor TextureDesc{};
	RTextureBulkData BulkData{};
	VkImage Image = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;
	bool OwnsImage = true;

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