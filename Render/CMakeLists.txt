# Render 库
add_library(Render STATIC
    render.cpp
)

# 公开头文件目录（使得其他目标可以直接 #include "render.h"）
target_include_directories(Render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(RHI)