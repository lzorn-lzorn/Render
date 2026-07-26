#include "VulkanDevice.h"

#include "VulkanCommandList.h"
#include "VulkanDescriptorSet.h"
#include "VulkanFence.h"
#include "VulkanPipeline.h"
#include "VulkanRHI.h"
#include "VulkanResources.h"
#include "VulkanShader.h"
#include "VulkanSwapchain.h"

#include <algorithm>
#include <array>
#include <set>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace render::rhi
{

namespace
{

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT,
	VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
	void*)
{
#if defined(_WIN32)
	if (CallbackData && CallbackData->pMessage)
	{
		OutputDebugStringA(CallbackData->pMessage);
		OutputDebugStringA("\n");
	}
#endif
	return VK_FALSE;
}

bool IsQueueFamilyUsableForCompute(const VkQueueFamilyProperties& properties)
{
	return (properties.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
}

bool IsQueueFamilyUsableForCopy(const VkQueueFamilyProperties& properties)
{
	return (properties.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
}

} // namespace

VulkanDevice::VulkanDevice()
{
	if (!createInstance())
	{
		return;
	}

	if (!pickPhysicalDevice())
	{
		return;
	}

	createLogicalDevice();
}

VulkanDevice::~VulkanDevice()
{
	waitIdle();

	if (Device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(Device, nullptr);
		Device = VK_NULL_HANDLE;
	}

	destroyDebugMessenger();

	if (Instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(Instance, nullptr);
		Instance = VK_NULL_HANDLE;
	}
}

RBuffer* VulkanDevice::createBuffer(const RBufferDescriptor& Descriptor)
{
	return new VulkanBuffer(this, Descriptor);
}

RTexture* VulkanDevice::createTexture(const RTextureDescriptor& Descriptor)
{
	return new VulkanTexture(this, Descriptor);
}

RSampler* VulkanDevice::createSampler(const RSamplerDescriptor& Descriptor)
{
	return new VulkanSampler(this, Descriptor);
}

RShader* VulkanDevice::createShader(const RShaderDescriptor& Descriptor)
{
	return new VulkanShader(this, Descriptor);
}

RPipelineState* VulkanDevice::createGraphicsPipeline(const RGraphicsPipelineDescriptor& Descriptor)
{
	return CreateVulkanGraphicsPipeline(this, Descriptor);
}

RPipelineState* VulkanDevice::createComputePipeline(const RComputePipelineDescriptor& Descriptor)
{
	return CreateVulkanComputePipeline(this, Descriptor);
}

RDescriptorSetLayout* VulkanDevice::createDescriptorSetLayout(const RDescriptorSetLayoutDescriptor& Descriptor)
{
	return new VulkanDescriptorSetLayout(this, Descriptor);
}

RDescriptorSet* VulkanDevice::createDescriptorSet(const RDescriptorSetDescriptor& Descriptor)
{
	return new VulkanDescriptorSet(this, Descriptor);
}

RCommandList* VulkanDevice::createCommandList(ECommandQueueType Type)
{
	return new VulkanCommandList(this, Type);
}

RSwapchain* VulkanDevice::createSwapchain(const RSwapchainDescriptor& Descriptor)
{
	return new VulkanSwapchain(this, Descriptor);
}

RFence* VulkanDevice::createFence()
{
	return new VulkanFence(this);
}

void VulkanDevice::submitCommandLists(ECommandQueueType Type, const QueueSubmitDescriptor& Descriptor)
{
	if (!Descriptor.CommandLists || Descriptor.CommandListCount == 0)
	{
		return;
	}

	std::vector<VkCommandBuffer> commandBuffers;
	commandBuffers.reserve(Descriptor.CommandListCount);

	for (uint32_t i = 0; i < Descriptor.CommandListCount; ++i)
	{
		auto* commandList = dynamic_cast<VulkanCommandList*>(Descriptor.CommandLists[i]);
		if (commandList)
		{
			commandBuffers.push_back(commandList->getVkCommandBuffer());
		}
	}

	if (commandBuffers.empty())
	{
		return;
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
	submitInfo.pCommandBuffers = commandBuffers.data();

	VkQueue queue = getVkQueue(Type);
	if (queue == VK_NULL_HANDLE)
	{
		return;
	}

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	if (Descriptor.SignalFence)
	{
		Descriptor.SignalFence->signal(Descriptor.SignalValue);
	}
}

void VulkanDevice::waitIdle()
{
	if (Device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(Device);
	}
}

void VulkanDevice::destroyResource(RResource* Resource)
{
	delete Resource;
}

bool VulkanDevice::isValid() const noexcept
{
	return Instance != VK_NULL_HANDLE && PhysicalDevice != VK_NULL_HANDLE && Device != VK_NULL_HANDLE;
}

VkQueue VulkanDevice::getVkQueue(ECommandQueueType Type) const noexcept
{
	switch (Type)
	{
	case ECommandQueueType::Compute:
		return ComputeQueue != VK_NULL_HANDLE ? ComputeQueue : GraphicsQueue;
	case ECommandQueueType::Copy:
		return CopyQueue != VK_NULL_HANDLE ? CopyQueue : GraphicsQueue;
	case ECommandQueueType::Graphics:
	default:
		return GraphicsQueue;
	}
}

uint32_t VulkanDevice::getQueueFamilyIndex(ECommandQueueType Type) const noexcept
{
	switch (Type)
	{
	case ECommandQueueType::Compute:
		return ComputeQueueFamilyIndex != UINT32_MAX ? ComputeQueueFamilyIndex : GraphicsQueueFamilyIndex;
	case ECommandQueueType::Copy:
		return CopyQueueFamilyIndex != UINT32_MAX ? CopyQueueFamilyIndex : GraphicsQueueFamilyIndex;
	case ECommandQueueType::Graphics:
	default:
		return GraphicsQueueFamilyIndex;
	}
}

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1u << i)) != 0 && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	return UINT32_MAX;
}

VkSurfaceKHR VulkanDevice::createSurface(void* NativeWindowHandle) const
{
	if (Instance == VK_NULL_HANDLE || NativeWindowHandle == nullptr)
	{
		return VK_NULL_HANDLE;
	}

#if defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = reinterpret_cast<HWND>(NativeWindowHandle);
	createInfo.hinstance = GetModuleHandleW(nullptr);

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (vkCreateWin32SurfaceKHR(Instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}

	return surface;
#else
	(void)NativeWindowHandle;
	return VK_NULL_HANDLE;
#endif
}

bool VulkanDevice::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "RenderSandbox";
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	appInfo.pEngineName = "Render";
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	std::vector<const char*> extensions;
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(_WIN32)
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	std::vector<const char*> layers;
	if (EnableValidation)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		layers.push_back("VK_LAYER_KHRONOS_validation");
	}

	VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = DebugCallback;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	createInfo.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();
	createInfo.pNext = EnableValidation ? &debugInfo : nullptr;

	VkResult createResult = vkCreateInstance(&createInfo, nullptr, &Instance);
	if (createResult != VK_SUCCESS && EnableValidation)
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.pNext = nullptr;
		createResult = vkCreateInstance(&createInfo, nullptr, &Instance);
		if (createResult == VK_SUCCESS)
		{
			EnableValidation = false;
		}
	}

	if (createResult != VK_SUCCESS)
	{
		return false;
	}

	if (EnableValidation)
	{
		auto* createDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT"));
		if (createDebugUtilsMessengerEXT)
		{
			createDebugUtilsMessengerEXT(Instance, &debugInfo, nullptr, &DebugMessenger);
		}
	}

	return true;
}

bool VulkanDevice::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data());

	for (VkPhysicalDevice device : devices)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		if (queueFamilyCount == 0)
		{
			continue;
		}

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		uint32_t graphics = UINT32_MAX;
		uint32_t compute = UINT32_MAX;
		uint32_t copy = UINT32_MAX;

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			const auto& props = queueFamilies[i];
			if (graphics == UINT32_MAX && (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				graphics = i;
			}

			if (compute == UINT32_MAX && IsQueueFamilyUsableForCompute(props))
			{
				compute = i;
			}

			if (copy == UINT32_MAX && IsQueueFamilyUsableForCopy(props))
			{
				copy = i;
			}
		}

		if (graphics == UINT32_MAX)
		{
			continue;
		}

		PhysicalDevice = device;
		GraphicsQueueFamilyIndex = graphics;
		ComputeQueueFamilyIndex = (compute == UINT32_MAX) ? graphics : compute;
		CopyQueueFamilyIndex = (copy == UINT32_MAX) ? graphics : copy;
		return true;
	}

	return false;
}

bool VulkanDevice::createLogicalDevice()
{
	if (PhysicalDevice == VK_NULL_HANDLE)
	{
		return false;
	}

	std::set<uint32_t> uniqueFamilies = {
		GraphicsQueueFamilyIndex,
		ComputeQueueFamilyIndex,
		CopyQueueFamilyIndex
	};

	const float queuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.reserve(uniqueFamilies.size());

	for (uint32_t family : uniqueFamilies)
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = family;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	std::array<const char*, 1> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
	dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynamicRendering.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &dynamicRendering;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &Device) != VK_SUCCESS)
	{
		return false;
	}

	vkGetDeviceQueue(Device, GraphicsQueueFamilyIndex, 0, &GraphicsQueue);
	vkGetDeviceQueue(Device, ComputeQueueFamilyIndex, 0, &ComputeQueue);
	vkGetDeviceQueue(Device, CopyQueueFamilyIndex, 0, &CopyQueue);

	return true;
}

void VulkanDevice::destroyDebugMessenger()
{
	if (Instance == VK_NULL_HANDLE || DebugMessenger == VK_NULL_HANDLE)
	{
		return;
	}

	auto* destroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (destroyDebugUtilsMessengerEXT)
	{
		destroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
	}

	DebugMessenger = VK_NULL_HANDLE;
}

} // namespace render::rhi
