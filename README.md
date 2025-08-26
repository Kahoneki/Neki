# Neki - A Cross-Platform Vulkan/DX12/PS5 Render Engine
![Logo](.github/logo.png)
###### Logo Credit: [@jowsey](https://github.com/jowsey)
----------------

<br></br>
# Summary
Neki is an in-development cross-platform render engine built for Vulkan (x64/Linux), DX12 (x64), and AGC (PS5).

This engine primarily serves as a personal learning project of mine in an effort to develop a deep understanding for modern graphics APIs and professional software architecture. In this sense, it's built to prioritise clarity and flexibility over enforcing a rigid structure.

AGC code unavailable due to Sony's NDA.

![Language](https://img.shields.io/badge/Language-C++23-pink.svg)
![CMake](https://img.shields.io/badge/CMake-3.28+-pink.svg)
![License](https://img.shields.io/badge/License-MIT-pink.svg)


<br></br>
# Building
This project is built on top of CMake, and so any compatible CLI/IDE tool will be able to generate the appropriate build files based on the provided CMakeLists.txt.

## Vulkan
Vulkan is the cross platform solution for running Neki on Windows and Linux.
For building on Vulkan:
- Ensure you have the Vulkan SDK package installed
  - Windows / Linux (Debian+Ubuntu) - https://vulkan.lunarg.com/
  - Linux (Arch) - AUR Packages: `vulkan-headers`, `vulkan-validation-layers`, `vulkan-man-pages`, `vulkan-tools`
- Set the NEKI_BUILD_VULKAN setting in CMakeSettings.json to true
- Set the NEKI_BUILD_D3D12 setting in CMakeSettings.json to false
- Configure and generate CMake

## DirectX-12
DX12 is only available for Windows development.
For building on DX12:
- Set the NEKI_BUILD_VULKAN setting in CMakeSettings.json to false
- Set the NEKI_BUILD_D3D12 setting in CMakeSettings.json to true
- Configure and generate CMake

## AGC
AGC development is under strict NDA and hence the source code is maintained in a private repository fork.

## Cross-API
While Vulkan remains the only API compatible for development with Neki on Linux based platforms, developing on Windows allows for both Vulkan and D3D12 simultaneously.
For building on simultaneous DX12 and Vulkan:
- Set the NEKI_BUILD_VULKAN setting in CMakeSettings.json to true
- Set the NEKI_BUILD_D3D12 setting in CMakeSettings.json to false
- Configure and generate CMake
