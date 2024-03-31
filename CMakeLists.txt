cmake_minimum_required(VERSION 3.7)
project(heX-Ray CXX)

# CMake project for hexray. It uses SDL2.
# For Windows users: make sure you've unpacked your SDL in either
#   - your user dir, subdir "develop\SDK", e.g. C:\Users\YOUR_USER\develop\SDK\SDL2-2.30.1
#   - in C:\SDK, e.g. C:\SDK\SDL2-2.30.1
#   - or in arbitrary directory provided in CMAKE_PREFIX_PATH
# For Linux and Mac OS X users: you don't need to do anything besides installing SDL2 with your package manager
if (WIN32)
    set(CMAKE_PREFIX_PATH "~/develop/SDK;c:\\SDK;${CMAKE_PREFIX_PATH}")
    if (CMAKE_GENERATOR STREQUAL "NMake Makefiles")
        message("If you get an error about 'nmake' missing, be sure to call CMake with -G 'MinGW Makefiles'")
    endif()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Explicitly require standard conformance to ensure portability.
        add_compile_options("/Zc:__cplusplus" "/permissive-")
    endif()
endif()

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(hexray
	src/bitmap.cpp
	src/bitmap.h
	src/camera.cpp
	src/camera.h
	src/color.h
	src/constants.h
	src/geometry.cpp
	src/geometry.h
	src/main.cpp
	src/main.h
	src/matrix.cpp
	src/matrix.h
	src/sdl.cpp
	src/sdl.h
	src/shading.cpp
	src/shading.h
	src/util.h
	src/vector.h
)
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_link_libraries(hexray stdc++fs)
endif()
target_link_libraries(hexray ${SDL2_LIBRARIES})

set_property(TARGET hexray PROPERTY CXX_STANDARD 17)

if (CMAKE_GENERATOR MATCHES "Visual Studio.*")
    # Set the startup project directly to be ready for running and debugging.
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT hexray)
    # Set the PATH environment variable to include the SDL2 binary directory to load dependencies without manual copies.
    set_target_properties(hexray PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${SDL2_BINDIR};$ENV{PATH}")
endif()
