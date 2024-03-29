# heX-Ray
The C++ raytracer for the v6 FMI raytracing course.
[raytracing-bg.net](http://raytracing-bg.net/)

## Setup on Windows

Recommended setup on Windows if you don't have Visual Studio (e.g. you only have Visual Studio Code)

1. Use the [following](https://code.visualstudio.com/docs/cpp/config-mingw) instructions. It boils down to:
    - Installing [MSYS2](https://github.com/msys2/msys2-installer/releases/download/2024-01-13/msys2-x86_64-20240113.exe)
    - Installing the MinGW-w64 toolchain using `pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain`
    - Adding the path to gcc.exe to the system PATH variable. Likely you'd want to modify it from Environment variables editor in Windows Settings, so that it points to `C:\msys64\ucrt64\bin`
2. Download the libSDL2 development package for MinGW from [here](https://github.com/libsdl-org/SDL/releases/).
    - E.g., use [SDL2-devel-2.30.1-mingw.zip](https://github.com/libsdl-org/SDL/releases/download/release-2.30.1/SDL2-devel-2.30.1-mingw.zip)
3. Install CMake — any version later than 3.7 is OK.
4. Make a "SDK" directory, either in a "develop" subfolder in your user directory (e.g. `C:\Users\«USERNAME»\develop\SDK`) or in `C:\`
5. Unarchive the SDL2-devel there, e.g. so that CMake can see `C:\Users\«USERNAME»\develop\SDK\SDL2-2.30.1\cmake`

### To compile `hexray` manually:

1. Create a "build" sub-directory: `mkdir build`
2. Change into it: `cd build`
3. Run CMake, targeting MinGW: `cmake -G "MinGW Makefiles" ..`
4. Run `make` to build your code

### To run it:

1. Copy SDL2.dll from SDL2\x86_64-w64-mingw32\bin\ to your "build" directory
2. You're all set - run `hexray.exe`

## Setup on Windows **with Visual Studio**

1. Install Visual Studio Community edition (free for personal use). It can be downloaded from [here](https://visualstudio.microsoft.com/vs/community/).
    - Only the `Desktop Development with C++` is necessary for building the project. There are additional instructions [here](https://learn.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170#visual-studio-2022-installation).
2. Download the SDL2 development package for **VC** from [here](https://github.com/libsdl-org/SDL/releases/)
    - E.g., use [SDL-devel-2.30.1-VC.zip](https://github.com/libsdl-org/SDL/releases/download/release-2.30.1/SDL2-devel-2.30.1-VC.zip)
3. Install CMake — version **3.13** or newer is OK.
4. \(_Same as regular [setup](#setup-on-windows)_\) Make a "SDK" directory, either in a "develop" subfolder in your user directory (e.g. `C:\Users\«USERNAME»\develop\SDK`) or in `C:\`
5. \(_Same as regular [setup](#setup-on-windows)_\) Unarchive the SDL2-devel there, e.g. so that CMake can see `C:\Users\«USERNAME»\develop\SDK\SDL2-2.30.1\cmake`

### To create a Visual Studio solution for `hexray`

1. Create a "build" sub-directory: `mkdir build`
2. Change into it: `cd build`
3. Run CMake, targeting a supported Visual Studio version:
    - Visual Studio 2022 \(recommended\): `cmake -G "Visual Studio 17 2022" -A x64 ..`
    - Visual Studio 2019: `cmake -G "Visual Studio 16 2019" -A x64 ..`
    - Visual Studio 2017: `cmake -G "Visual Studio 15 2017 Win64" ..`

### Build and run inside Visual Studio:

1. Open the generated `heX-Ray.sln` solution file.
2. Build the solution \(`Build` -> `Build Solution` OR `[Ctrl]+[Shift]+[B]` \(some VS may have `[F7]` as shortcut for building\)\).
3. Start the build \(`Debug` -> `Start Debugging` OR `[F5]`\).
