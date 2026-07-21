cmake_minimum_required(VERSION 3.16)
project(SandBoxProject LANGUAGES CXX)

# 全局 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 添加子目录
add_subdirectory(Render)
add_subdirectory(SandBox)