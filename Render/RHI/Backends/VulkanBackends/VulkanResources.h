
#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "../../Definitions.h"

namespace render::rhi
{

class VulkanDevice;
class VulkanBuffer : public RBuffer
{
public:
	VulkanBuffer(VulkanDevice* InDevice, const EBufferDescriptor& InBufferDesc, VkBuffer InBuffer, VmaAllocation InAlloc);

	~VulkanBuffer();
	uint64_t getSize() const override { return BufferDesc.Size; }
	VkBuffer getVkBuffer() const { return Buffer; }
	VmaAllocation getAllocation() const { return Alloc; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	EBufferDescriptor BufferDesc;
	VkBuffer Buffer;
	VmaAllocation Alloc;
};

class VulkanTexture : public RTexture
{
public:
	VulkanTexture(VulkanDevice* InDevice, const ETextureDescriptor& InTextureDesc, VkImage InImage, VmaAllocation InAlloc);

	~VulkanTexture();
	uint32_t getWidth() const override { return TextureDesc.Width; }
	uint32_t getHeight() const override { return TextureDesc.Height; }
	uint32_t getDepth() const override { return TextureDesc.Depth; }
	VkImage getVkImage() const { return Image; }
	VmaAllocation getAllocation() const { return Alloc; }

	RTextureView* createView(const ETextureViewDescriptor& Descriptor) override;
	RTextureView* getDefaultView() { return TextureView; }
	EFormat getFormat() const override { return TextureDesc.Format; }
private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	RTextureView* TextureView = nullptr;
	ETextureDescriptor TextureDesc;
	VkImage Image;
	VmaAllocation Alloc;
};


class VulkanTextureView : public RTextureView
{
public:
	VulkanTextureView(VulkanTexture* InTexture, const ETextureViewDescriptor& InDescriptor);
	~VulkanTextureView();
	virtual EResourceType getType() const override { return EResourceType::Texture; }
	virtual RTexture* getTexture() const override { return Texture; }
	virtual ETextureViewDescriptor getDescriptor() const override { return Descriptor; }
private:
	void SetDebugName(const std::string& Name) override;
	VulkanTexture* Texture;
	ETextureViewDescriptor Descriptor;
};

class VulkanSampler : public RSampler
{
public:
	VulkanSampler(VulkanDevice* InDevice, const ESamplerDescriptor& InSamplerDesc);
	~VulkanSampler();
	virtual EResourceType getType() const override { return EResourceType::Sampler; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	ESamplerDescriptor SamplerDesc;
	VkSampler Sampler = VK_NULL_HANDLE;

};
}