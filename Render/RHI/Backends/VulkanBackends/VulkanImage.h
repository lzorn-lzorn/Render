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

class VulkanImage
{
public:
	VulkanImage(VulkanDevice* InDevice, const RTextureDescriptor& InDescriptor);
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
	bool createImage(const RTextureDescriptor& Descriptor);
	bool allocateImageMemory();

	VulkanDevice* Device = nullptr;
	VkImage Image = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;
	VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	EVulkanImageOwnership Ownership = EVulkanImageOwnership::Owned;
};

} // namespace render::rhi
