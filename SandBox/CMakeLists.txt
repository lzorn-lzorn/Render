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

# ---------- 主可执行程序 ----------
add_executable(SandBox main.cpp)

target_link_libraries(SandBox PRIVATE Renderer)
if (NOT TARGET DXC::Compiler)
    message(FATAL_ERROR "DXC::Compiler target not found. Ensure top-level CMake config sets up dxcompiler first.")
endif()
target_link_libraries(SandBox PRIVATE DXC::Compiler)

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
    # 复制 SandBox 的直接/间接运行时 DLL 依赖
    add_custom_command(TARGET SandBox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:SandBox>
            $<TARGET_FILE_DIR:SandBox>
        COMMAND_EXPAND_LISTS
    )

    # 显式复制 DXC runtime，防止被 PATH 中其他版本抢占
    add_custom_command(TARGET SandBox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_PROPERTY:DXC::Compiler,IMPORTED_LOCATION>
            $<TARGET_FILE_DIR:SandBox>
    )

    # 保持 Assets 资源复制（这个仍需手动处理，因为与链接无关）
    add_custom_command(TARGET SandBox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:SandBox>/Assets
    )
endif()
