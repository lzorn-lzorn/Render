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

	VkDevice vk_device = Device->getVkDevice();
	if (vk_device != VK_NULL_HANDLE && AcquireFence != VK_NULL_HANDLE)
	{
		vkDestroyFence(vk_device, AcquireFence, nullptr);
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

	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &Swapchain;
	present_info.pImageIndices = &CurrentImageIndex;

	VkResult result = vkQueuePresentKHR(Device->getVkQueue(ECommandQueueType::Graphics), &present_info);
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

	VkPhysicalDevice physical_device = Device->getVkPhysicalDevice();
	VkDevice vk_device = Device->getVkDevice();

	VkBool32 presentSupported = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, Device->getQueueFamilyIndex(ECommandQueueType::Graphics), Surface, &presentSupported);
	if (presentSupported == VK_FALSE)
	{
		return false;
	}

	VkSurfaceCapabilitiesKHR capabilities{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, Surface, &capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, Surface, &formatCount, nullptr);
	if (formatCount == 0)
	{
		return false;
	}

	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, Surface, &formatCount, formats.data());

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
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, Surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, Surface, &presentModeCount, presentModes.data());

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

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = Surface;
	create_info.minImageCount = imageCount;
	create_info.imageFormat = selectedFormat.format;
	create_info.imageColorSpace = selectedFormat.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.preTransform = capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = presentMode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = OldSwapchain;

	VkSwapchainKHR new_swapchain = VK_NULL_HANDLE;
	if (vkCreateSwapchainKHR(vk_device, &create_info, nullptr, &new_swapchain) != VK_SUCCESS)
	{
		return false;
	}

	Textures.clear();
	Images.clear();

	if (OldSwapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(vk_device, OldSwapchain, nullptr);
	}

	Swapchain = new_swapchain;

	uint32_t swapchain_image_count = 0;
	vkGetSwapchainImagesKHR(vk_device, Swapchain, &swapchain_image_count, nullptr);
	Images.resize(swapchain_image_count);
	vkGetSwapchainImagesKHR(vk_device, Swapchain, &swapchain_image_count, Images.data());

	RTextureDescriptor texture_descriptor{};
	texture_descriptor.Usage = ETextureUsage::Target | ETextureUsage::Present;
	texture_descriptor.Format = ToRhiFormat(selectedFormat.format);
	texture_descriptor.Width = extent.width;
	texture_descriptor.Height = extent.height;
	texture_descriptor.Depth = 1;
	texture_descriptor.MipLevels = 1;
	texture_descriptor.ArrayLayers = 1;
	texture_descriptor.SampleCount = ESampleCount::Count1;

	for (VkImage image : Images)
	{
		VkImageViewCreateInfo view_info{};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = image;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = selectedFormat.format;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;

		VkImageView view = VK_NULL_HANDLE;
		if (vkCreateImageView(vk_device, &view_info, nullptr, &view) != VK_SUCCESS)
		{
			continue;
		}

		Textures.push_back(std::make_unique<VulkanTexture>(Device, texture_descriptor, image, view));
	}

	Desc.Format = texture_descriptor.Format;
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