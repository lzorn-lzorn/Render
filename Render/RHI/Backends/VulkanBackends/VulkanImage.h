#pragma once

#include "../../Definitions.h"
#include "VulkanRHI.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace render::rhi
{

class VulkanDevice;

enum class EVulkanImageOwnership : uint8_t
{
	Owned,
	External
};

class VulkanImage : public RImage
{
public:
	VulkanImage(VulkanDevice* InDevice, const RImageDescriptor& InDescriptor);
	VulkanImage(VulkanDevice* InDevice, VkImage InExternalImage, VkImageLayout InInitialLayout);
	~VulkanImage();

	bool isValid() const noexcept;
	bool ownsImage() const noexcept;
	bool ownsMemory() const noexcept;

	VkImage getVkImage() const noexcept { return Image; }
	VkDeviceMemory getVkDeviceMemory() const noexcept { return Memory; }
	VkImageLayout getLayout() const noexcept { return Layout; }
	void setLayout(VkImageLayout InLayout) noexcept { Layout = InLayout; }

private:
	virtual void updateTexture(const RTextureUpdateRegion& Region, const void* SrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch = 0) {}

	virtual void generateMipmaps() {}


	bool createImage(const RImageDescriptor& Descriptor);
	bool allocateImageMemory();

	VulkanDevice* Device = nullptr;
	VkImage Image = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;
	VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	EVulkanImageOwnership Ownership = EVulkanImageOwnership::Owned;
};

} // namespace render::rhi
