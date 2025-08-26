cmake_minimum_required(VERSION 3.28)
project(Neki LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#Backend selection
option(NEKI_BUILD_VULKAN "Enable the Vulkan RHI backend" ON)
if(WIN32)
    option(NEKI_BUILD_D3D12 "Enable the D3D12 RHI backend" ON)
endif()


#Define library target
file(GLOB_RECURSE NEKI_COMMON_SOURCES
    "src/Core/*.cpp"
    "src/Platform/*.cpp"
    "src/Graphics/*.cpp"
)
set(NEKI_SOURCES ${NEKI_COMMON_SOURCES})

if(NEKI_BUILD_VULKAN)
    file(GLOB_RECURSE NEKI_VULKAN_SOURCES "src/RHI-Vulkan/*.cpp")
    list(APPEND NEKI_SOURCES ${NEKI_VULKAN_SOURCES})
endif()

if(NEKI_BUILD_D3D12)
    file(GLOB_RECURSE NEKI_D3D12_SOURCES "src/RHI-D3D12/*.cpp")
    list(APPEND NEKI_SOURCES ${NEKI_D3D12_SOURCES})
endif()

add_library(Neki STATIC ${NEKI_SOURCES})
target_include_directories(Neki PUBLIC src)


#Add compile definitions for macro usage
if(NEKI_BUILD_VULKAN)
    target_compile_definitions(Neki PUBLIC NEKI_VULKAN_SUPPORTED=1)
endif()
if(NEKI_BUILD_D3D12)
    target_compile_definitions(Neki PUBLIC NEKI_D3D12_SUPPORTED=1)
    target_link_libraries(Neki PUBLIC d3d12 dxgi)
endif()



#Propagate public dependencies
include(FetchContent)

#Vulkan
if(NEKI_BUILD_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(Neki PUBLIC Vulkan::Vulkan)
endif()

#D3D12 handled automatically

#DXC
FetchContent_Declare(
    dxc_artifacts
    URL "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2212/dxc_2022_12_16.zip"
)
FetchContent_MakeAvailable(dxc_artifacts)
find_program(DXC_EXECUTABLE NAMES dxc HINTS "${dxc_artifacts_SOURCE_DIR}/bin/x64")
if(NOT DXC_EXECUTABLE)
    message(FATAL_ERROR "Could not find dxc.exe")
endif()
message(STATUS "Found DXC: ${DXC_EXECUTABLE}")

#GLM
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)
target_link_libraries(Neki PUBLIC glm)

#GLFW
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
target_link_libraries(Neki PUBLIC glfw)



#Handle private dependencies

#stb_image
FetchContent_Declare(stb_image GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)
FetchContent_MakeAvailable(stb_image)
target_include_directories(Neki PRIVATE ${stb_image_SOURCE_DIR})

#Assimp
FetchContent_Declare(assimp GIT_REPOSITORY "https://github.com/assimp/assimp.git" GIT_TAG v5.4.1)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)
target_link_libraries(Neki PRIVATE assimp)


#Copy all Neki resource files to the output directory, maintaining subdirectory structure
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Resource Files/" DESTINATION "${CMAKE_BINARY_DIR}/Resource Files/")



#Function to compile a shader
function(CompileShader SHADER_SOURCE)

    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME_WE)
    set(SPIRV_OUTPUT "${PROJECT_BINARY_DIR}/Shaders/${SHADER_NAME}.spirv")
    set(DXIL_OUTPUT "${PROJECT_BINARY_DIR}/Shaders/${SHADER_NAME}.dxil")

    if(NEKI_BUILD_VULKAN)
        add_custom_command(
            OUTPUT ${SPIRV_OUTPUT}
            COMMAND ${DXC_EXECUTABLE} -T vs_6_6 -E VSMain -spirv -Fo ${SPIRV_OUTPUT} ${SHADER_SOURCE}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling ${SHADER_SOURCE} to SPIR-V"
        )
        list(APPEND SHADER_OUTPUTS ${SPIRV_OUTPUT})
    endif()

     if(NEKI_BUILD_D3D12)
        add_custom_command(
            OUTPUT ${DXIL_OUTPUT}
            COMMAND ${DXC_EXECUTABLE} -T vs_6_6 -E VSMain -Fo ${DXIL_OUTPUT} ${SHADER_SOURCE}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling ${SHADER_SOURCE} to DXIL"
        )
        list(APPEND SHADER_OUTPUTS ${DXIL_OUTPUT})
    endif()

endfunction()








#Test executables - check only passes if building the Neki library directly, not when it's being included by another project via FetchContent
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    #Shader compilation

    #Define shader files
    file(GLOB_RECURSE SHADER_SOURCES "${CMAKE_SOURCE_DIR}/Samples/*/Shaders/*.hlsl")
    message(STATUS "---- Found Shader Files ----")
    foreach(FILE ${SHADER_SOURCES})
        message(STATUS "  ${FILE}")
    endforeach()
    message(STATUS "--------------------------------")

    #Compile all shaders
    set(SHADER_OUTPUTS "")
    foreach(SHADER_SRC ${SHADER_SOURCES})
        CompileShader(${SHADER_SRC})
    endforeach()

    #Create phony target that depends on all the compiled shaders
    add_custom_target(
            Shaders
            DEPENDS ${SHADER_OUTPUTS}
    )



    #Create executables
    if(NEKI_BUILD_VULKAN)
        add_executable(NKVulkanSample_Library "Samples/Vulkan/Library/Library.cpp")
        target_include_directories(NKVulkanSample_Library PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKVulkanSample_Library PRIVATE Neki)
        add_dependencies(NKVulkanSample_Library Shaders)
    endif()
    if(NEKI_BUILD_D3D12)
        add_executable(NKD3D12Sample_Library "Samples/D3D12/Library/Library.cpp")
        target_include_directories(NKD3D12Sample_Library PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKD3D12Sample_Library PRIVATE Neki)
        add_dependencies(NKD3D12Sample_Library Shaders)
    endif()

endif()
