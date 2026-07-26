find_package(Vulkan REQUIRED)

target_sources(${LibarayName} PRIVATE
	VulkanDevice.cpp
	VulkanRHI.cpp
	VulkanResources.cpp
	VulkanFence.cpp
	VulkanPipeline.cpp
	VulkanShader.cpp
	VulkanDescriptorSet.cpp
	VulkanCommandList.cpp
	VulkanSwapchain.cpp
)

target_include_directories(${LibarayName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
	target_compile_definitions(${LibarayName} PRIVATE VK_USE_PLATFORM_WIN32_KHR NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

target_link_libraries(${LibarayName} 
	PUBLIC 
		Vulkan::Vulkan
)