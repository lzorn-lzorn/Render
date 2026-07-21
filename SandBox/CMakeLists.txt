
# 将 SDL3 源码目录添加为子项目
# 它会提供 SDL3::SDL3 这个 CMake 目标
add_subdirectory(SDL)

# 你的可执行文件
add_executable(SandBox main.cpp)

# 链接 SDL3 与 Render 库
# Render 的 PUBLIC include 目录会自动传递给 SandBox。
target_link_libraries(SandBox PRIVATE SDL3::SDL3 Render)

if (WIN32)
	# 确保运行时 SDL3.dll 与可执行文件位于同一目录，避免启动失败。
	add_custom_command(TARGET SandBox POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:SDL3::SDL3>
		$<TARGET_FILE_DIR:SandBox>
	)
endif()