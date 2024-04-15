cmake_minimum_required(VERSION 3.7)
project(heX-Ray CXX)

# CMake project for hexray. It uses SDL2 and OpenEXR (with its dependencies).
# For Windows users: make sure you've unpacked your SDL and OpenEXR in either
#   - your user dir, subdir "develop\SDK", e.g. C:\Users\YOUR_USER\develop\SDK\SDL2-2.30.1
#   - in C:\SDK, e.g. C:\SDK\SDL2-2.30.1
#   - or in arbitrary directory provided in CMAKE_PREFIX_PATH
# For Linux and Mac OS X users: you don't need to do anything besides installing SDL2 with your package manager
if (WIN32)
    option(HEXRAY_BUILD_OPENEXR "Build OpenEXR as part of the CMake configuration" OFF)

    set(CMAKE_PREFIX_PATH "~/develop/SDK;c:\\SDK;${CMAKE_PREFIX_PATH}")
    if (CMAKE_GENERATOR STREQUAL "NMake Makefiles")
        message("If you get an error about 'nmake' missing, be sure to call CMake with -G 'MinGW Makefiles'")
    endif()
    if (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
        set(HEXRAY_MSYS_ROOT "C:\\msys64" CACHE STRING "Root directory to MSYS installation to find installed packages automatically")
        set(CMAKE_PREFIX_PATH "${HEXRAY_MSYS_ROOT}/ucrt64/lib/cmake;${CMAKE_PREFIX_PATH}")
    endif()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Additional search directories for vcpkg
        set(CMAKE_PREFIX_PATH "~/develop/SDK/x64-windows;c:\\SDK\\x64-windows;${CMAKE_PREFIX_PATH}")
        # Explicitly require standard conformance to ensure portability.
        add_compile_options("/Zc:__cplusplus" "/permissive-")
    endif()
    if (HEXRAY_BUILD_OPENEXR)
        set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/openexr;${CMAKE_PREFIX_PATH}")
        # First check if it was already built, or available in the prefix path.
        find_package(OpenEXR QUIET)
        if (NOT OpenEXR_FOUND)
            include(scripts/build_openexr.cmake)
        endif()
    endif()
endif()

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
if (NOT TARGET OpenEXR::OpenEXR)
	find_package(OpenEXR REQUIRED)
endif()

add_executable(hexray
	src/bitmap.cpp
	src/bitmap.h
	src/camera.cpp
	src/camera.h
	src/color.h
	src/constants.h
	src/environment.cpp
	src/environment.h
	src/geometry.cpp
	src/geometry.h
	src/main.cpp
	src/main.h
	src/mesh.cpp
	src/mesh.h
	src/matrix.cpp
	src/matrix.h
	src/sdl.cpp
	src/sdl.h
	src/shading.cpp
	src/shading.h
	src/util.cpp
	src/util.h
	src/vector.h
)
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_link_libraries(hexray stdc++fs)
endif()
target_link_libraries(hexray ${SDL2_LIBRARIES} OpenEXR::OpenEXR)

set_property(TARGET hexray PROPERTY CXX_STANDARD 17)

# Additional configuration for easy debugging in Visual Studio
if (CMAKE_GENERATOR MATCHES "Visual Studio.*")
	# Set the startup project directly to be ready for running and debugging.
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT hexray)
	set (_debug_path "${SDL2_BINDIR};$<TARGET_FILE_DIR:OpenEXR::OpenEXR>;$<TARGET_FILE_DIR:Imath::Imath>")
	if (TARGET libdeflate::libdeflate_shared)
		set (_debug_path "${_debug_path};$<TARGET_FILE_DIR:libdeflate::libdeflate_shared>")
	endif()
	# Set the PATH environment variable to include the SDL2 binary directory to load dependencies without manual copies.
	set_target_properties(hexray PROPERTIES VS_DEBUGGER_ENVIRONMENT
		"PATH=${_debug_path};$ENV{PATH}"
	)
endif()

# Configure the VSCode launch file
if (WIN32 AND CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
	# Backslashes need to be escaped additionally for the json generation.
	string(REPLACE "\\" "\\\\" _msys_root_escaped "${HEXRAY_MSYS_ROOT}")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/.vscode/launch.json.in.mingw ${CMAKE_CURRENT_SOURCE_DIR}/.vscode/launch.json @ONLY)
else()
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/.vscode/launch.json.in.linux ${CMAKE_CURRENT_SOURCE_DIR}/.vscode/launch.json @ONLY)
endif()
