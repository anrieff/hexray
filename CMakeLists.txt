cmake_minimum_required(VERSION 3.7)
# CMake project for hexray. It uses SDL2.
# For Windows users: make sure you've unpacked your SDL in either
#   - your user dir, subdir "develop\SDK", e.g. C:\Users\YOUR_USER\develop\SDK\SDL2-2.30.1
#   - in C:\SDK, e.g. C:\SDK\SDL2-2.30.1
# For Linux and Mac OS X users: you don't need to do anything besides installing SDL2 with your package manager
IF (WIN32)
    set(CMAKE_PREFIX_PATH "~/develop/SDK;c:\\SDK")
    if (CMAKE_GENERATOR STREQUAL "NMake Makefiles")
        message("If you get an error about 'nmake' missing, be sure to call CMake with -G 'MinGW Makefiles'")
    ENDIF()
ENDIF()
project(heX-Ray)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(hexray
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
target_link_libraries(hexray ${SDL2_LIBRARIES})
set_property(TARGET hexray PROPERTY CXX_STANDARD 17)
