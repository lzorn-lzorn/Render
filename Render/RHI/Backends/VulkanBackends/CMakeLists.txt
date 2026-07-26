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

target_link_libraries(${LibarayName} 
	PUBLIC 
		Vulkan::Vulkan
)