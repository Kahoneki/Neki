# Neki - A Cross-Platform Vulkan/DX12 Game Engine
![Logo](.github/logo.png)
###### Logo Credit: [@jowsey](https://github.com/jowsey)
----------------

<br></br>
# Summary
Neki is an in-development cross-platform game engine built for Vulkan (x64/Linux) and DirectX 12 (x64).

This engine primarily serves as a personal learning project of mine in an effort to develop a deep understanding for modern graphics APIs and professional software architecture. In this sense, it's built to prioritise clarity and flexibility over enforcing a rigid structure.

![Language](https://img.shields.io/badge/Language-C++23-pink.svg)
![CMake](https://img.shields.io/badge/CMake-3.28+-pink.svg)
![License](https://img.shields.io/badge/License-MIT-pink.svg)


<br></br>
# Building
This project is built on top of CMake, and so any compatible CLI/IDE tool will be able to generate the appropriate build files based on the provided CMakeLists.txt and CMakePresets.json.

## Prerequisites
### Vulkan
Vulkan is the cross platform solution for running Neki on Windows and Linux.
For building on Vulkan:
- Ensure you have the Vulkan SDK package installed
  - Windows / Linux (Debian+Ubuntu) - https://vulkan.lunarg.com/
  - Linux (Arch) - AUR Packages: `vulkan-headers`, `vulkan-validation-layers`, `vulkan-man-pages`, `vulkan-tools`

### D3D12
DX12 is only available for Windows development. If building on a Windows platform, the DX12 SDK should already be installed for you.


## Instructions
After cloning the repository:
1) Open the project folder in your IDE of choice.
2) Select the configuration you want to build for - Vulkan (Debug) or D3D12 (Debug)
   - On **Visual Studio** / **VS Code**, these configurations should load automatically and appear at the top of your window.
   - On **CLion**, the desired configuration(s) needs to be manually added to the UI. When the project is first opened, the profiles settings menu should be opened automatically. From there, you can select the desired configuration(s) and check "Enable Profile" to add them. Thereafter, the selected profiles will appear at the top of your window.
     - To access this menu otherwise, navigate to `File -> Settings -> Build, Execution, Deployment -> CMake`
     - If this menu doesn't appear and there is no `CMake` option under `Build, Execution, Deployment` as described above, it is likely you are experiencing a known issue with CLion which can be solved by fully closing the IDE, navigating to the project folder, deleting the `.idea` folder, then reopening the project.
   - If using the **CMake CLI**, the desired build can be configured with `cmake --preset "Vulkan (Debug)"` or `cmake --preset "D3D12 (Debug)"`.
4) Once the CMake files have been generated, the desired executable can be selected (e.g.: NKVulkanSample_Library) and built and ran as normal.
   - If using the CMake CLI, the project can instead be built with `cmake --build --preset "Vulkan (Debug)"` or `cmake --build --preset "D3D12 (Debug)"` whereafter the executables will be under `build/vulkan-debug/` and `build/d3d12-debug/` respectively

## Cross-API
While Vulkan remains the only API compatible for development with Neki on Linux based platforms, developing on Windows allows for both Vulkan and D3D12 simultaneously.
For building on simultaneous DX12 and Vulkan, there exists a `Vulkan/D3D12 (Debug)` preset which can be selected using the same instructions as above
- For CMake CLI, the cross-API build is configured with `cmake --preset "Vulkan/D3D12 (Debug)"` and built with `cmake --build --preset "Vulkan/D3D12 (Debug)"`. The executables for both platforms can thereafter be found under `build/cross-api-debug/`.
