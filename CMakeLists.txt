cmake_minimum_required(VERSION 3.16)

if (APPLE)
	set(CMAKE_C_COMPILER "/usr/bin/clang" CACHE FILEPATH "C compiler" FORCE)
	set(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE FILEPATH "CXX compiler" FORCE)
	set(CMAKE_OBJC_COMPILER "/usr/bin/clang" CACHE FILEPATH "Objective-C compiler" FORCE)
	set(CMAKE_OBJCXX_COMPILER "/usr/bin/clang++" CACHE FILEPATH "Objective-CXX compiler" FORCE)
	project(SandBoxProject LANGUAGES C CXX OBJC OBJCXX)
else()
	project(SandBoxProject LANGUAGES CXX)
endif()

# 全局 C++23 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json 文件只是 ninja 和 Unix Makefiles 生成器才会生成的
# 其他的后端生成器不能保证会生成 compile_commands.json 文件, 所以 .clangd 的 
# CompilationDatabase 选项就无法使用, 需要手动 Add 头文件
# 在 cmake -B build -G Ninja 或者 cmake -B build -G "Unix Makefiles" 中手动使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (WIN32)
	set(VULKAN_SDK_ROOT "$ENV{VULKAN_SDK}")

	if (NOT VULKAN_SDK_ROOT)
		find_package(Vulkan QUIET)
		if (Vulkan_INCLUDE_DIR)
			get_filename_component(VULKAN_SDK_ROOT "${Vulkan_INCLUDE_DIR}" DIRECTORY)
		elseif (Vulkan_INCLUDE_DIRS)
			list(GET Vulkan_INCLUDE_DIRS 0 _first_vulkan_include)
			get_filename_component(VULKAN_SDK_ROOT "${_first_vulkan_include}" DIRECTORY)
		endif()
	endif()

	set(DXC_IMPLIB "")
	set(DXC_DLL "")
	if (VULKAN_SDK_ROOT)
		if (EXISTS "${VULKAN_SDK_ROOT}/Lib/dxcompiler.lib")
			set(DXC_IMPLIB "${VULKAN_SDK_ROOT}/Lib/dxcompiler.lib")
		endif()
		if (EXISTS "${VULKAN_SDK_ROOT}/Bin/dxcompiler.dll")
			set(DXC_DLL "${VULKAN_SDK_ROOT}/Bin/dxcompiler.dll")
		elseif (EXISTS "${VULKAN_SDK_ROOT}/Bin32/dxcompiler.dll")
			set(DXC_DLL "${VULKAN_SDK_ROOT}/Bin32/dxcompiler.dll")
		endif()
	endif()

	if (NOT DXC_IMPLIB)
		find_library(DXC_IMPLIB NAMES dxcompiler)
	endif()
	if (NOT DXC_DLL)
		find_file(DXC_DLL NAMES dxcompiler.dll)
	endif()

	if (NOT DXC_IMPLIB OR NOT DXC_DLL)
		message(FATAL_ERROR "Failed to locate dxcompiler.lib/dxcompiler.dll. Set VULKAN_SDK or install a DXC runtime.")
	endif()

	add_library(DXC::Compiler SHARED IMPORTED GLOBAL)
	set_target_properties(DXC::Compiler PROPERTIES
		IMPORTED_IMPLIB "${DXC_IMPLIB}"
		IMPORTED_LOCATION "${DXC_DLL}"
	)

	message(STATUS "Using DXC implib: ${DXC_IMPLIB}")
	message(STATUS "Using DXC runtime: ${DXC_DLL}")
endif()

# 添加子目录
add_subdirectory(Render)
add_subdirectory(SandBox)