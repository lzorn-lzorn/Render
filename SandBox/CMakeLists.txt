if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL/CMakeLists.txt")
    message(STATUS "Using local SDL3 sources in SandBox/SDL/")
    add_subdirectory(SDL)
else()
    message(STATUS "SDL3 not found locally - will download via FetchContent")
    include(FetchContent)

    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
        GIT_TAG         release-3.2.8
        GIT_SHALLOW     TRUE
    )

    set(SDL_STATIC ON CACHE BOOL "Build SDL as static library")
    set(SDL_SHARED OFF CACHE BOOL "Do not build shared library")
    set(SDL_TEST OFF CACHE BOOL "Do not build tests")

    FetchContent_MakeAvailable(SDL3)
endif()

# ---------- 处理 dxcompiler ----------
# 1. 找到 dxcompiler 的导入库和动态库
find_library(DXCOMPILER_LIB dxcompiler REQUIRED)
find_file(DXCOMPILER_DLL dxcompiler.dll REQUIRED)

# 2. 创建 IMPORTED 共享库目标，让 CMake 知道它是一个带 DLL 的外部库
add_library(dxcompiler SHARED IMPORTED)
set_target_properties(dxcompiler PROPERTIES
    IMPORTED_LOCATION "${DXCOMPILER_DLL}"
    IMPORTED_IMPLIB   "${DXCOMPILER_LIB}"
)

# 3. (可选) 如果你的 DXC 版本还需要 dxil.dll，同样处理并建立依赖关系
# find_file(DXIL_DLL dxil.dll REQUIRED)
# add_library(dxil SHARED IMPORTED)
# set_target_properties(dxil PROPERTIES IMPORTED_LOCATION "${DXIL_DLL}")
# target_link_libraries(dxcompiler INTERFACE dxil)   # 告知 cmake dxcompiler 依赖 dxil

# ---------- 主可执行程序 ----------
add_executable(SandBox main.cpp)

target_link_libraries(SandBox PRIVATE Renderer)
target_link_libraries(SandBox PRIVATE dxcompiler)   # 现在 dxcompiler 是一个目标了

target_include_directories(SandBox PRIVATE
    ${CMAKE_SOURCE_DIR}/glm
)

# 链接 SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(SandBox PRIVATE SDL3::SDL3)
elseif(TARGET SDL3::SDL3-static)
    target_link_libraries(SandBox PRIVATE SDL3::SDL3-static)
else()
    message(FATAL_ERROR "SDL3 target not found")
endif()

# ---------- Windows 运行时自动化 ----------
if (WIN32)
    # 一行命令自动复制 SandBox 所有直接/间接依赖的运行时 DLL
    # 包括 SDL3::SDL3 (如果是动态库)、dxcompiler.dll、dxil.dll(如果前面配置了) 等
    add_custom_command(TARGET SandBox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:SandBox>
            $<TARGET_FILE_DIR:SandBox>
        COMMAND_EXPAND_LISTS
    )

    # 保持 Assets 资源复制（这个仍需手动处理，因为与链接无关）
    add_custom_command(TARGET SandBox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:SandBox>/Assets
    )
endif()

# TODO:
# 对于 dxcompiler.dll 没有被复制到 SandBox.exe 所在目录的情况, 可以考虑使用 
# 直接 clone https://github.com/microsoft/DirectXShaderCompiler.git 编译, 链接成静态库