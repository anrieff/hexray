# heX-Ray
The C++ raytracer for the v6 FMI raytracing course

## Setup on Windows

Recommanded setup on Windows if you don't have Visual Studio (e.g. you only have Visual Studio Code)

1. Use the [following](https://code.visualstudio.com/docs/cpp/config-mingw) instructions. It boils down to:
    - Installing [MSYS2](https://github.com/msys2/msys2-installer/releases/download/2024-01-13/msys2-x86_64-20240113.exe)
    - Installing the MinGW-w64 toolchain using `pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain`
    - Adding the path to gcc.exe to the system PATH variable. Likely you'd want to modify it from Environment variables editor in Windows Settings, so that it points to `C:\msys64\ucrt64\bin`
2. Download the libSDL2 development package for MinGW from [here](https://github.com/libsdl-org/SDL/releases/).
    - E.g., use [SDL2-devel-2.30.1-mingw.zip](https://github.com/libsdl-org/SDL/releases/download/release-2.30.1/SDL2-devel-2.30.1-mingw.zip)
3. Install CMake — any version later than 3.7 is OK.
4. Make a "SDK" directory, either in a "develop" subfolder in your user directory (e.g. `C:\Users\«USERNAME»\develop\SDK`) or in C:\
5. Unarchive the SDL2-devel there, e.g. so that CMake can see `C:\Users\«USERNAME»\develop\SDK\SDL2-2.30.1\cmake`

To compile `hexray` manually:

1. Create a "build" directory: `mkdir build`
2. Change into it: `cd build`
3. Run CMake, targeting MinGW: `cmake -G "MinGW Makefiles" ..`
4. Run `make` to build your code

To run it:

1. Copy SDL2.dll from SDL2\x86_64-w64-mingw32\bin\ to your "build" directory
2. You're all set - run `hexray.exe`
