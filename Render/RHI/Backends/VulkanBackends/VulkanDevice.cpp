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
		if (ImmediateCommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(Device, ImmediateCommandPool, nullptr);
			ImmediateCommandPool = VK_NULL_HANDLE;
		}

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

RBuffer* VulkanDevice::createStagingBuffer(const void* Data, uint64_t Size, uint64_t InitialDataSize)
{
	RBufferDescriptor Descriptor{};
	Descriptor.Size = Size;
	Descriptor.Usage = EBufferUsage::TransferSrc;
	Descriptor.MemoryProperties = EMemoryProperty::HostVisible | EMemoryProperty::HostCoherent;
	Descriptor.InitialData = Data;
	Descriptor.InitialDataSize = InitialDataSize;
	Descriptor.Name = "StagingBuffer";
	return new VulkanBuffer(this, Descriptor);
}

RTexture* VulkanDevice::createTexture(const RTextureDescriptor& Descriptor)
{
	return new VulkanTexture(this, Descriptor);
}

RTexture* VulkanDevice::createTexture(const RTextureDescriptor& Descriptor, RTextureBulkData Data)
{
	return new VulkanTexture(this, Descriptor, Data);
}

RSampler* VulkanDevice::createSampler(const RSamplerDescriptor& Descriptor)
{
	return new VulkanSampler(this, Descriptor);
}

RShader* VulkanDevice::createShader(const RShaderDescriptor& Descriptor)
{
	return new VulkanShader(this, Descriptor);
}

RPipeline* VulkanDevice::createGraphicsPipeline(const RGraphicsPipelineDescriptor& Descriptor)
{
	return CreateVulkanGraphicsPipeline(this, Descriptor);
}

RPipeline* VulkanDevice::createComputePipeline(const RComputePipelineDescriptor& Descriptor)
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

	std::vector<VkCommandBuffer> command_buffers;
	command_buffers.reserve(Descriptor.CommandListCount);

	for (uint32_t i = 0; i < Descriptor.CommandListCount; ++i)
	{
		auto* command_list = static_cast<VulkanCommandList*>(Descriptor.CommandLists[i]);
		if (command_list)
		{
			command_buffers.push_back(command_list->getVkCommandBuffer());
		}
	}

	if (command_buffers.empty())
	{
		return;
	}

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
	submit_info.pCommandBuffers = command_buffers.data();

	VkQueue queue = getVkQueue(Type);
	if (queue == VK_NULL_HANDLE)
	{
		return;
	}

	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
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

VkCommandBuffer VulkanDevice::beginImmediateCommand()
{
	if (Device == VK_NULL_HANDLE || ImmediateCommandPool == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	VkCommandBufferAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool = ImmediateCommandPool;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	if (vkAllocateCommandBuffers(Device, &allocate_info, &command_buffer) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(Device, ImmediateCommandPool, 1, &command_buffer);
		return VK_NULL_HANDLE;
	}

	return command_buffer;
}

void VulkanDevice::endImmediateCommand(VkCommandBuffer CommandBuffer)
{
	if (Device == VK_NULL_HANDLE || ImmediateCommandPool == VK_NULL_HANDLE || CommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	if (vkEndCommandBuffer(CommandBuffer) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(Device, ImmediateCommandPool, 1, &CommandBuffer);
		return;
	}

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &CommandBuffer;

	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence submit_fence = VK_NULL_HANDLE;
	if (vkCreateFence(Device, &fence_info, nullptr, &submit_fence) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(Device, ImmediateCommandPool, 1, &CommandBuffer);
		return;
	}

	const VkQueue submit_queue = GraphicsQueue != VK_NULL_HANDLE ? GraphicsQueue : getVkQueue(ECommandQueueType::Graphics);
	if (submit_queue != VK_NULL_HANDLE)
	{
		vkQueueSubmit(submit_queue, 1, &submit_info, submit_fence);
		vkWaitForFences(Device, 1, &submit_fence, VK_TRUE, UINT64_MAX);
	}

	vkDestroyFence(Device, submit_fence, nullptr);
	vkFreeCommandBuffers(Device, ImmediateCommandPool, 1, &CommandBuffer);
}

bool VulkanDevice::createInstance()
{
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "RenderSandbox";
	app_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	app_info.pEngineName = "Render";
	app_info.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

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

	VkDebugUtilsMessengerCreateInfoEXT debug_info{};
	debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_info.pfnUserCallback = DebugCallback;

	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();
	create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
	create_info.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();
	create_info.pNext = EnableValidation ? &debug_info : nullptr;

	VkResult create_result = vkCreateInstance(&create_info, nullptr, &Instance);
	if (create_result != VK_SUCCESS && EnableValidation)
	{
		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = nullptr;
		create_info.pNext = nullptr;
		create_result = vkCreateInstance(&create_info, nullptr, &Instance);
		if (create_result == VK_SUCCESS)
		{
			EnableValidation = false;
		}
	}

	if (create_result != VK_SUCCESS)
	{
		return false;
	}

	if (EnableValidation)
	{
		auto* create_debug_utils_messenger_EXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT"));
		if (create_debug_utils_messenger_EXT)
		{
			create_debug_utils_messenger_EXT(Instance, &debug_info, nullptr, &DebugMessenger);
		}
	}

	return true;
}

bool VulkanDevice::pickPhysicalDevice()
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(Instance, &device_count, nullptr);
	if (device_count == 0)
	{
		return false;
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(Instance, &device_count, devices.data());

	for (VkPhysicalDevice device : devices)
	{
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
		if (queue_family_count == 0)
		{
			continue;
		}

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

		uint32_t graphics = UINT32_MAX;
		uint32_t compute = UINT32_MAX;
		uint32_t copy = UINT32_MAX;

		for (uint32_t i = 0; i < queue_family_count; ++i)
		{
			const auto& props = queue_families[i];
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

	std::set<uint32_t> unique_families = {
		GraphicsQueueFamilyIndex,  // 图形队列
		ComputeQueueFamilyIndex,   // 计算队列
		CopyQueueFamilyIndex       // 传输队列
	};

	const float queuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	queue_create_infos.reserve(unique_families.size());

	for (uint32_t family : unique_families)
	{
		VkDeviceQueueCreateInfo queue_info{};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info.queueFamilyIndex = family;
		queue_info.queueCount = 1;
		queue_info.pQueuePriorities = &queuePriority;
		queue_create_infos.push_back(queue_info);
	}

	std::array<const char*, 1> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering{};
	dynamic_rendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynamic_rendering.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceFeatures device_features{};

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pNext = &dynamic_rendering;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	if (vkCreateDevice(PhysicalDevice, &create_info, nullptr, &Device) != VK_SUCCESS)
	{
		return false;
	}

	vkGetDeviceQueue(Device, GraphicsQueueFamilyIndex, 0, &GraphicsQueue);
	vkGetDeviceQueue(Device, ComputeQueueFamilyIndex, 0, &ComputeQueue);
	vkGetDeviceQueue(Device, CopyQueueFamilyIndex, 0, &CopyQueue);

	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = GraphicsQueueFamilyIndex;
	if (vkCreateCommandPool(Device, &pool_info, nullptr, &ImmediateCommandPool) != VK_SUCCESS)
	{
		vkDestroyDevice(Device, nullptr);
		Device = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void VulkanDevice::destroyDebugMessenger()
{
	if (Instance == VK_NULL_HANDLE || DebugMessenger == VK_NULL_HANDLE)
	{
		return;
	}

	auto* destroy_debug_utils_messenger_EXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (destroy_debug_utils_messenger_EXT)
	{
		destroy_debug_utils_messenger_EXT(Instance, DebugMessenger, nullptr);
	}

	DebugMessenger = VK_NULL_HANDLE;
}

} // namespace render::rhi
