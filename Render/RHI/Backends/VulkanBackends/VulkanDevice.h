#pragma once

#include "../../RHI.h"

#include <cstdint>

#if defined(_WIN32) && !defined(VK_USE_PLATFORM_WIN32_KHR)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

namespace render::rhi
{

class VulkanBuffer;
class VulkanTexture;
class VulkanSampler;
class VulkanShader;
class VulkanPipeline;
class VulkanDescriptorSetLayout;
class VulkanDescriptorSet;
class VulkanCommandList;
class VulkanSwapchain;
class VulkanFence;

class VulkanDevice : public RDevice
{
public:
	VulkanDevice();
	~VulkanDevice() override;

	RBuffer* createBuffer(const RBufferDescriptor& Descriptor) override;
	RBuffer* createStagingBuffer(const void* Data, uint64_t Size, uint64_t InitialDataSize = 0) override;
	RImage* createImage(const RImageDescriptor& Descriptor) override;
	RImage* createImage(const RImageDescriptor& Descriptor, const RTextureBulkData& Data) override;

	RSampler* createSampler(const RSamplerDescriptor& Descriptor) override;
	RShader* createShader(const RShaderDescriptor& Descriptor) override;
	RPipeline* createGraphicsPipeline(const RGraphicsPipelineDescriptor& Descriptor) override;
	RPipeline* createComputePipeline(const RComputePipelineDescriptor& Descriptor) override;
	RDescriptorSetLayout* createDescriptorSetLayout(const RDescriptorSetLayoutDescriptor& Descriptor) override;
	RDescriptorSet* createDescriptorSet(const RDescriptorSetDescriptor& Descriptor) override;
	RCommandList* createCommandList(ECommandQueueType Type) override;
	RSwapchain* createSwapchain(const RSwapchainDescriptor& Descriptor) override;
	RFence* createFence() override;
	
	void submitCommandLists(ECommandQueueType Type, const QueueSubmitDescriptor& Descriptor) override;
	void waitIdle() override;
	void destroyResource(RResource* Resource) override;

	bool isValid() const noexcept;

	VkInstance getVkInstance() const noexcept { return Instance; }
	VkPhysicalDevice getVkPhysicalDevice() const noexcept { return PhysicalDevice; }
	VkDevice getVkDevice() const noexcept { return Device; }
	VkQueue getVkQueue(ECommandQueueType Type) const noexcept;
	uint32_t getQueueFamilyIndex(ECommandQueueType Type) const noexcept;
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	VkSurfaceKHR createSurface(void* NativeWindowHandle) const;

public:
	VkCommandBuffer beginImmediateCommand();
	void endImmediateCommand(VkCommandBuffer CommandBuffer);
private:
	bool createInstance();
	bool pickPhysicalDevice();
	bool createLogicalDevice();
	void destroyDebugMessenger();

	VkInstance Instance = VK_NULL_HANDLE;
	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
	VkDevice Device = VK_NULL_HANDLE;

	VkQueue GraphicsQueue = VK_NULL_HANDLE;
	uint32_t GraphicsQueueFamilyIndex = UINT32_MAX;

	VkQueue ComputeQueue = VK_NULL_HANDLE;
	uint32_t ComputeQueueFamilyIndex = UINT32_MAX;

	VkQueue CopyQueue = VK_NULL_HANDLE;
	uint32_t CopyQueueFamilyIndex = UINT32_MAX;
	VkCommandPool ImmediateCommandPool = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
	bool EnableValidation = true;
};

} // namespace render::rhi