# Render 库
set(LibarayName "Renderer")
add_library(${LibarayName} STATIC
    Renderer.cpp
)

add_subdirectory(RHI)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Reflect/CMakeLists.txt")
    message(STATUS "Using local SPIRV-Reflect sources in SandBox/SPIRV-Reflect/")
    add_subdirectory(SPIRV-Reflect)
else()
    message(STATUS "SPIRV-Reflect not found locally - will download via FetchContent")
    include(FetchContent)

    # 声明 SPIRV-Reflect 仓库
    FetchContent_Declare(
        SPIRV-Reflect
        GIT_REPOSITORY  https://github.com/KhronosGroup/SPIRV-Reflect.git
        GIT_TAG         vulkan-sdl-1.4.350.0  # TODO: 要检测当前 vulkan sdk 的版本   
        GIT_SHALLOW     TRUE            # 只拉取浅克隆，加快下载
    )

    # 配置 SPIRV-Reflect 的构建选项
	set(VULKAN_SDK $ENV{VULKAN_SDK})

    FetchContent_MakeAvailable(SPIRV-Reflect)

	if (${VULKAN_DIR_FOUND})
		target_compile_definitions(${LibarayName} PRIVATE SPIRV_REFLECT_HAS_VULKAN_H)
		target_include_directories(${LibarayName} PRIVATE ${VULKAN_DIR}/include)
	endif()

endif()

if (WIN32)
    target_compile_definitions(${LibarayName} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_link_libraries(${LibarayName} 
    PRIVATE 
		dxcompiler
)

target_include_directories(${LibarayName} 
	PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/include
)