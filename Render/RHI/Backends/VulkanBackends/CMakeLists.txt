find_package(Vulkan REQUIRED)

target_sources(Render PRIVATE
	VulkanRHI.cpp
)

target_include_directories(Render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Render PUBLIC Vulkan::Vulkan)
