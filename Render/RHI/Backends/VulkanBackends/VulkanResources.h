#pragma once

#include "../../Definitions.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace render::rhi
{

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
	VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc);
	VulkanTexture(VulkanDevice* InDevice, const RTextureDescriptor& InTextureDesc, VkImage InExternalImage, VkImageView InExternalView);
	~VulkanTexture() override;

	bool isValid() const override;
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