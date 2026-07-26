# Render 库
set(LibarayName "Renderer")
add_library(${LibarayName} STATIC
    Renderer.cpp
)

# 公开头文件目录（使得其他目标可以直接 #include "render.h"）
target_include_directories(${LibarayName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(RHI)

if (WIN32)
    target_compile_definitions(${LibarayName} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()