cmake_minimum_required(VERSION 3.16)
project(SandBoxProject LANGUAGES CXX)

# 全局 C++23 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json 文件只是 ninja 和 Unix Makefiles 生成器才会生成的
# 其他的后端生成器不能保证会生成 compile_commands.json 文件, 所以 .clangd 的 
# CompilationDatabase 选项就无法使用, 需要手动 Add 头文件
# 在 cmake -B build -G Ninja 或者 cmake -B build -G "Unix Makefiles" 中手动使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 添加子目录
add_subdirectory(Render)
add_subdirectory(SandBox)