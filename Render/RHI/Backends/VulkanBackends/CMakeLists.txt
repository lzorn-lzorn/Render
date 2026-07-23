find_package(Vulkan REQUIRED)

target_sources(Render PRIVATE
	VulkanDevice.cpp
	VulkanRHI.cpp
	VulkanResources.cpp
	VulkanShaderPipeline.cpp
	VulkanBindGroup.cpp
	VulkanCommandList.cpp
	VulkanSwapchainFence.cpp
)

target_include_directories(Render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Render PUBLIC Vulkan::Vulkan)
