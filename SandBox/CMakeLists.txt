
# 如果 SandBox/SDL/ 目录存在且包含 CMakeLists.txt，则直接使用本地源码
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL/CMakeLists.txt")
    message(STATUS "Using local SDL3 sources in SandBox/SDL/")
    add_subdirectory(SDL)
else()
    message(STATUS "SDL3 not found locally - will download via FetchContent")
    include(FetchContent)

    # 声明 SDL3 仓库
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
        GIT_TAG         release-3.2.8   # 或你想用的版本
        GIT_SHALLOW     TRUE            # 只拉取浅克隆，加快下载
    )

    # 配置 SDL3 的构建选项（可选）
    set(SDL_STATIC ON CACHE BOOL "Build SDL as static library")
    set(SDL_SHARED OFF CACHE BOOL "Do not build shared library")
    set(SDL_TEST OFF CACHE BOOL "Do not build tests")

    # 下载并导入
    FetchContent_MakeAvailable(SDL3)
endif()

# 你的可执行文件
add_executable(SandBox main.cpp)

# 链接依赖
target_link_libraries(SandBox PRIVATE Render)

# 根据 SDL3 提供的目标名称链接
if(TARGET SDL3::SDL3)
    target_link_libraries(SandBox PRIVATE SDL3::SDL3)
elseif(TARGET SDL3::SDL3-static)
    target_link_libraries(SandBox PRIVATE SDL3::SDL3-static)
else()
    message(FATAL_ERROR "SDL3 target not found")
endif()

if (WIN32)
	# 确保运行时 SDL3.dll 与可执行文件位于同一目录，避免启动失败。
	add_custom_command(TARGET SandBox POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:SDL3::SDL3>
		$<TARGET_FILE_DIR:SandBox>
	)
endif()