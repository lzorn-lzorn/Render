#include "VulkanSwapchain.h"

#include "VulkanDevice.h"
#include "VulkanRHI.h"
#include "VulkanResources.h"

#include <algorithm>
#include <vector>

namespace render::rhi
{

namespace
{

EFormat ToRhiFormat(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_R8G8B8A8_UNORM: return EFormat::RGBA8_UNorm;
	case VK_FORMAT_R8G8B8A8_SRGB: return EFormat::RGBA8_sRGB;
	case VK_FORMAT_B8G8R8A8_UNORM: return EFormat::BGRA8_UNorm;
	default:
		return EFormat::BGRA8_UNorm;
	}
}

VkPresentModeKHR ChoosePresentMode(bool vsync, const std::vector<VkPresentModeKHR>& modes)
{
	if (vsync)
	{
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	for (VkPresentModeKHR mode : modes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return mode;
		}
	}

	for (VkPresentModeKHR mode : modes)
	{
		if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			return mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

} // namespace

VulkanSwapchain::VulkanSwapchain(VulkanDevice* InDevice, const RSwapchainDescriptor& InDescriptor)
	: Device(InDevice)
	, Desc(InDescriptor)
{
	if (!Device || !Device->isValid())
	{
		return;
	}

	Surface = Device->createSurface(Desc.NativeWindowHandle);
	if (Surface == VK_NULL_HANDLE)
	{
		return;
	}

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(Device->getVkDevice(), &fenceInfo, nullptr, &AcquireFence);

	createSwapchain(VK_NULL_HANDLE);
}

VulkanSwapchain::~VulkanSwapchain()
{
	if (!Device)
	{
		return;
	}

	VkDevice vkDevice = Device->getVkDevice();
	if (vkDevice != VK_NULL_HANDLE && AcquireFence != VK_NULL_HANDLE)
	{
		vkDestroyFence(vkDevice, AcquireFence, nullptr);
		AcquireFence = VK_NULL_HANDLE;
	}

	destroySwapchain();

	if (Surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(Device->getVkInstance(), Surface, nullptr);
		Surface = VK_NULL_HANDLE;
	}
}

RTexture* VulkanSwapchain::acquireNextTexture()
{
	if (!Device || Swapchain == VK_NULL_HANDLE || Textures.empty())
	{
		return nullptr;
	}

	if (vkResetFences(Device->getVkDevice(), 1, &AcquireFence) != VK_SUCCESS)
	{
		return nullptr;
	}

	VkResult result = vkAcquireNextImageKHR(
		Device->getVkDevice(),
		Swapchain,
		UINT64_MAX,
		VK_NULL_HANDLE,
		AcquireFence,
		&CurrentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		resize(Desc.Width, Desc.Height);
		return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		return nullptr;
	}

	if (vkWaitForFences(Device->getVkDevice(), 1, &AcquireFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		return nullptr;
	}

	if (CurrentImageIndex >= Textures.size())
	{
		return nullptr;
	}

	return Textures[CurrentImageIndex].get();
}

void VulkanSwapchain::present()
{
	if (!Device || Swapchain == VK_NULL_HANDLE)
	{
		return;
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &Swapchain;
	presentInfo.pImageIndices = &CurrentImageIndex;

	VkResult result = vkQueuePresentKHR(Device->getVkQueue(ECommandQueueType::Graphics), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		resize(Desc.Width, Desc.Height);
	}
}

void VulkanSwapchain::resize(uint32_t Width, uint32_t Height)
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	Desc.Width = Width;
	Desc.Height = Height;

	if (!Device || !Device->isValid())
	{
		return;
	}

	Device->waitIdle();
	createSwapchain(Swapchain);
}

bool VulkanSwapchain::createSwapchain(VkSwapchainKHR OldSwapchain)
{
	if (!Device || Surface == VK_NULL_HANDLE)
	{
		return false;
	}

	VkPhysicalDevice physicalDevice = Device->getVkPhysicalDevice();
	VkDevice vkDevice = Device->getVkDevice();

	VkBool32 presentSupported = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, Device->getQueueFamilyIndex(ECommandQueueType::Graphics), Surface, &presentSupported);
	if (presentSupported == VK_FALSE)
	{
		return false;
	}

	VkSurfaceCapabilitiesKHR capabilities{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, Surface, &capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, Surface, &formatCount, nullptr);
	if (formatCount == 0)
	{
		return false;
	}

	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, Surface, &formatCount, formats.data());

	VkSurfaceFormatKHR selectedFormat = formats[0];
	VkFormat requested = ToVkFormat(Desc.Format);
	for (const VkSurfaceFormatKHR& format : formats)
	{
		if (format.format == requested)
		{
			selectedFormat = format;
			break;
		}
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, Surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, Surface, &presentModeCount, presentModes.data());

	VkPresentModeKHR presentMode = ChoosePresentMode(Desc.VSync, presentModes);

	VkExtent2D extent{};
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		extent = capabilities.currentExtent;
	}
	else
	{
		extent.width = std::clamp(Desc.Width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = std::clamp(Desc.Height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}

	uint32_t imageCount = std::max(Desc.BufferCount, capabilities.minImageCount);
	if (capabilities.maxImageCount > 0)
	{
		imageCount = std::min(imageCount, capabilities.maxImageCount);
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = Surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = selectedFormat.format;
	createInfo.imageColorSpace = selectedFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = OldSwapchain;

	VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
	if (vkCreateSwapchainKHR(vkDevice, &createInfo, nullptr, &newSwapchain) != VK_SUCCESS)
	{
		return false;
	}

	Textures.clear();
	Images.clear();

	if (OldSwapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(vkDevice, OldSwapchain, nullptr);
	}

	Swapchain = newSwapchain;

	uint32_t swapchainImageCount = 0;
	vkGetSwapchainImagesKHR(vkDevice, Swapchain, &swapchainImageCount, nullptr);
	Images.resize(swapchainImageCount);
	vkGetSwapchainImagesKHR(vkDevice, Swapchain, &swapchainImageCount, Images.data());

	RTextureDescriptor textureDescriptor{};
	textureDescriptor.Usage = ETextureUsage::Target | ETextureUsage::Present;
	textureDescriptor.Format = ToRhiFormat(selectedFormat.format);
	textureDescriptor.Width = extent.width;
	textureDescriptor.Height = extent.height;
	textureDescriptor.Depth = 1;
	textureDescriptor.MipLevels = 1;
	textureDescriptor.ArrayLayers = 1;
	textureDescriptor.SampleCount = ESampleCount::Count1;

	for (VkImage image : Images)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = selectedFormat.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView view = VK_NULL_HANDLE;
		if (vkCreateImageView(vkDevice, &viewInfo, nullptr, &view) != VK_SUCCESS)
		{
			continue;
		}

		Textures.push_back(std::make_unique<VulkanTexture>(Device, textureDescriptor, image, view));
	}

	Desc.Format = textureDescriptor.Format;
	Desc.Width = extent.width;
	Desc.Height = extent.height;
	CurrentImageIndex = 0;
	return !Textures.empty();
}

void VulkanSwapchain::destroySwapchain()
{
	Textures.clear();
	Images.clear();

	if (Device && Swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(Device->getVkDevice(), Swapchain, nullptr);
		Swapchain = VK_NULL_HANDLE;
	}
}

} // namespace render::rhi