#pragma once

#include "../../Definitions.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>


namespace render::rhi
{

class VulkanDevice;
class VulkanTexture;

class VulkanSwapchain : public RSwapchain
{
public:
	VulkanSwapchain(VulkanDevice* InDevice, const RSwapchainDescriptor& InDescriptor);
	~VulkanSwapchain() override;

	RTexture* acquireNextTexture() override;
	void present() override;
	void resize(uint32_t Width, uint32_t Height) override;
	uint32_t getCurrentTextureIndex() const override { return CurrentImageIndex; }
	uint32_t getTextureCount() const override { return static_cast<uint32_t>(Textures.size()); }
	EFormat getFormat() const override { return Desc.Format; }
	uint32_t getWidth() const override { return Desc.Width; }
	uint32_t getHeight() const override { return Desc.Height; }

private:
	bool createSwapchain(VkSwapchainKHR OldSwapchain);
	void destroySwapchain();

	VulkanDevice* Device = nullptr;
	RSwapchainDescriptor Desc{};
	VkSurfaceKHR Surface = VK_NULL_HANDLE;
	VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
	VkFence AcquireFence = VK_NULL_HANDLE;
	uint32_t CurrentImageIndex = 0;

	std::vector<VkImage> Images;
	std::vector<std::unique_ptr<VulkanTexture>> Textures;

};

} // namespace render::rhi