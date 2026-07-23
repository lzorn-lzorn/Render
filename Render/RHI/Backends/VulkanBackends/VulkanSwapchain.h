
#include "../../Definitions.h"
#include "VulkanDevice.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>


namespace render::rhi
{

class VulkanSwapchain : public RSwapchain
{
public:
	VulkanSwapchain(VulkanDevice* InDevice, const RSwapchainDescriptor& InDescriptor);
	~VulkanSwapchain() override;

	RTexture* acquireNextTexture() override;
	void present() override;
	void resize(uint32_t Width, uint32_t Height) override;
	EFormat getFormat() const override { return Desc.Format; }
	uint32_t getWidth() const override { return Desc.Width; }
	uint32_t getHeight() const override { return Desc.Height; }

private:
	VulkanDevice* Device;
	RSwapchainDescriptor Desc;
	VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
};

} // namespace render::rhi