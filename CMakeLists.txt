cmake_minimum_required(VERSION 3.28)
project(Neki LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)


#Backend selection
option(NEKI_BUILD_VULKAN "Enable the Vulkan RHI backend" OFF)
option(NEKI_BUILD_D3D12 "Enable the D3D12 RHI backend" OFF)
option(NEKI_ENABLE_EDITOR "Enable the engine editor tools" OFF)


#Define library target
file(GLOB_RECURSE NEKI_COMMON_SOURCES
        "src/Components/*.cpp"
        "src/Core/*.cpp"
        "src/Core-ECS/*.cpp"
        "src/Platform/*.cpp"
        "src/Graphics/*.cpp"
        "src/Managers/*.cpp"
        "src/Networking/*.cpp"
        "src/Physics/*.cpp"
        "src/Types/*.cpp"
        "src/RHI/*.cpp"
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
if(NEKI_ENABLE_EDITOR)
    target_compile_definitions(Neki PUBLIC NEKI_EDITOR=1)
endif()

target_compile_definitions(Neki PUBLIC NEKI_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")
target_compile_definitions(Neki PUBLIC NEKI_BUILD_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\")



#Dependencies
include(FetchContent)

#Vulkan
if(NEKI_BUILD_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(Neki PUBLIC Vulkan::Vulkan)

    #VMA
    FetchContent_Declare(
            vma
            GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
            GIT_TAG v3.3.0
    )
    FetchContent_MakeAvailable(vma)
    target_link_libraries(Neki PUBLIC VulkanMemoryAllocator)
    #todo: this is sick i just learnt about this, use it for stb and allat too
    set_source_files_properties(src/RHI-Vulkan/VulkanDevice.cpp PROPERTIES COMPILE_DEFINITIONS VMA_IMPLEMENTATION)
endif()

if(NEKI_BUILD_D3D12)
    #D3D12 handled automatically

    #D3D12MA
    FetchContent_Declare(
            d3d12ma
            GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git
            GIT_TAG v3.0.1
    )
    FetchContent_MakeAvailable(d3d12ma)
    target_link_libraries(Neki PUBLIC D3D12MemoryAllocator)
endif()

#DXC
if(WIN32)
    FetchContent_Declare(
            dxc_artifacts
            URL "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2505.1/dxc_2025_07_14.zip"
    )
elseif(UNIX AND NOT APPLE)
    FetchContent_Declare(
            dxc_artifacts
            URL "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2505.1/linux_dxc_2025_07_14.x86_64.tar.gz"
    )
else()
    message(FATAL_ERROR "Platform not supported. Supported platforms = Windows, Linux")
endif ()
FetchContent_MakeAvailable(dxc_artifacts)
if(WIN32)
    set(DXC_EXECUTABLE "${dxc_artifacts_SOURCE_DIR}/bin/x64/dxc.exe")
else()
    set(DXC_EXECUTABLE "${dxc_artifacts_SOURCE_DIR}/bin/dxc")
endif()
if(NOT DXC_EXECUTABLE)
    message(FATAL_ERROR "Could not find dxc.exe")
endif()
message(STATUS "Found DXC: ${DXC_EXECUTABLE}")
if(UNIX AND NOT APPLE)
    execute_process(COMMAND chmod +x "${DXC_EXECUTABLE}")
endif()
#Slang
if(WIN32)
    FetchContent_Declare(
            slang_artifacts
            URL "https://github.com/shader-slang/slang/releases/download/v2026.5.2/slang-2026.5.2-windows-x86_64.zip"
    )
elseif(UNIX AND NOT APPLE)
    FetchContent_Declare(
            slang_artifacts
            URL "https://github.com/shader-slang/slang/releases/download/v2026.5.2/slang-2026.5.2-linux-x86_64.tar.gz"
    )
else()
    message(FATAL_ERROR "Slang: Platform not supported.")
endif()
FetchContent_MakeAvailable(slang_artifacts)
if(WIN32)
    set(SLANGC_EXECUTABLE "${slang_artifacts_SOURCE_DIR}/bin/slangc.exe")
else()
    set(SLANGC_EXECUTABLE "${slang_artifacts_SOURCE_DIR}/bin/slangc")
    execute_process(COMMAND chmod +x "${SLANGC_EXECUTABLE}")
endif()
message(STATUS "Found Slang: ${SLANGC_EXECUTABLE}")

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

#SFML
FetchContent_Declare(sfml GIT_REPOSITORY https://github.com/SFML/SFML.git GIT_TAG 3.0.2)
FetchContent_MakeAvailable(sfml)
target_link_libraries(Neki PUBLIC sfml-network)

#Cereal
FetchContent_Declare(cereal GIT_REPOSITORY https://github.com/USCiLab/cereal.git GIT_TAG v1.3.2)
set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "" FORCE) #disables boost requirement - i dont want to talk about how long this took to find
FetchContent_MakeAvailable(cereal)
target_link_libraries(Neki PUBLIC cereal)

#stb_image
FetchContent_Declare(stb_image GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)
FetchContent_MakeAvailable(stb_image)
target_include_directories(Neki PUBLIC ${stb_image_SOURCE_DIR})

#bcdec
FetchContent_Declare(bcdec GIT_REPOSITORY https://github.com/iOrange/bcdec.git GIT_TAG main)
FetchContent_MakeAvailable(bcdec)
target_include_directories(Neki PUBLIC ${bcdec_SOURCE_DIR})

#KTX
FetchContent_Declare(ktx GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git GIT_TAG v4.4.2)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ktx)
target_link_libraries(Neki PUBLIC ktx)

#Assimp
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(assimp GIT_REPOSITORY "https://github.com/assimp/assimp.git" GIT_TAG v5.4.1)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)
target_link_libraries(Neki PUBLIC assimp)

#Jolt
set(USE_STATIC_MSVC_RUNTIME OFF CACHE BOOL "" FORCE)
FetchContent_Declare(jolt GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git GIT_TAG v5.5.0 SOURCE_SUBDIR Build)
FetchContent_MakeAvailable(jolt)
target_link_libraries(Neki PUBLIC Jolt)
target_include_directories(Neki PUBLIC ${jolt_SOURCE_DIR})

#Portable File Dialogs
FetchContent_Declare(portable_file_dialogs GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs.git GIT_TAG main)
FetchContent_GetProperties(portable_file_dialogs)
if(NOT portable_file_dialogs_POPULATED)
    FetchContent_Populate(portable_file_dialogs)
    add_library(portable_file_dialogs INTERFACE)
    target_include_directories(portable_file_dialogs INTERFACE ${portable_file_dialogs_SOURCE_DIR})
endif()
target_link_libraries(Neki PUBLIC portable_file_dialogs)


#ImGui
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.92.5-docking)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

if(NEKI_BUILD_VULKAN)
    list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp)
endif()

if(NEKI_BUILD_D3D12)
    list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_dx12.cpp)
endif()


#ImGuizmo
FetchContent_Declare(imguizmo GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git GIT_TAG 1.9)
FetchContent_GetProperties(imguizmo)
if(NOT imguizmo_POPULATED)
    FetchContent_Populate(imguizmo)
endif()
list(APPEND IMGUI_SOURCES ${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp)


#ImGui library
add_library(ImGui STATIC ${IMGUI_SOURCES})
target_include_directories(ImGui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_include_directories(ImGui PUBLIC ${imguizmo_SOURCE_DIR}/src)
target_link_libraries(ImGui PUBLIC glfw)

if(NEKI_BUILD_VULKAN)
    target_link_libraries(ImGui PUBLIC Vulkan::Vulkan)
endif()

if(NEKI_BUILD_D3D12)
    target_link_libraries(ImGui PUBLIC d3d12 dxgi d3dcompiler)
endif()

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(ImGui PUBLIC X11::X11)
endif()

target_link_libraries(Neki PUBLIC ImGui)



# ==========================================
# NTC Python Virtual Environment Setup
# ==========================================
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Define the venv directory inside the build folder
set(NTC_VENV_DIR "${CMAKE_BINARY_DIR}/ntc_venv")

# Determine the correct Python executable path based on the OS
if(WIN32)
    set(NTC_PYTHON_EXE "${NTC_VENV_DIR}/Scripts/python.exe")
else()
    set(NTC_PYTHON_EXE "${NTC_VENV_DIR}/bin/python")
endif()

# Convert to forward slashes so it doesn't break C++ string escaping
file(TO_CMAKE_PATH "${NTC_PYTHON_EXE}" NTC_PYTHON_EXE_FWD)

# Define the custom command to create the venv and install requirements.
# This uses a marker file so it only runs once, unless requirements.txt changes.
set(NTC_REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/util/ntc/requirements.txt")
set(NTC_VENV_MARKER "${NTC_VENV_DIR}/venv.marker")

add_custom_command(
        OUTPUT "${NTC_VENV_MARKER}"
        COMMAND ${Python3_EXECUTABLE} -m venv "${NTC_VENV_DIR}"
        COMMAND "${NTC_PYTHON_EXE}" -m pip install --upgrade pip
        COMMAND "${NTC_PYTHON_EXE}" -m pip install -r "${NTC_REQUIREMENTS}"
        COMMAND ${CMAKE_COMMAND} -E touch "${NTC_VENV_MARKER}"
        DEPENDS "${NTC_REQUIREMENTS}"
        COMMENT "Setting up Python venv for NTC and installing dependencies (PyTorch is large, this may take a few minutes)..."
)

# Create a target for the venv setup
add_custom_target(NTCVenvSetup ALL DEPENDS "${NTC_VENV_MARKER}")

# Make the Neki library depend on this setup, so the venv is built before the engine runs
add_dependencies(Neki NTCVenvSetup)

# Pass the venv Python executable path to C++
target_compile_definitions(Neki PUBLIC NEKI_PYTHON_EXECUTABLE=\"${NTC_PYTHON_EXE_FWD}\")



#Compiles a given HLSL shader source file to SPIR-V and/or DXIL
#Usage:
#- SOURCE_FILE: Path to the HLSL shader file
#- SHADER_TYPE: Shader model profile (e.g.: "vs_6_6")
#- ENTRY_POINT: Name of the main function in the shader
#- OUTPUT_LIST: Name of the list to contain the returned shader paths - will contain with the .spv and/or .dxil paths from this function
function(CompileShader SHADER_SOURCE SHADER_TYPE ENTRY_POINT OUTPUT_LIST)

    #Get relative path from project directory (maintain subdirectory structure)
    file(RELATIVE_PATH SHADER_RELATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${SHADER_SOURCE})

    #Get directory and filename without extension
    get_filename_component(SHADER_DIRECTORY ${SHADER_RELATIVE_PATH} PATH)
    get_filename_component(SHADER_NAME_WE ${SHADER_RELATIVE_PATH} NAME_WE)

    # Recombine them to get the base for the output filename.
    if(SHADER_DIRECTORY)
        set(SHADER_OUTPUT_PATH "${SHADER_DIRECTORY}/${SHADER_NAME_WE}")
    else()
        set(SHADER_OUTPUT_PATH "${SHADER_NAME_WE}")
    endif()

    set(SPV_OUTPUT "${PROJECT_BINARY_DIR}/${SHADER_OUTPUT_PATH}.spv")
    set(DXIL_OUTPUT "${PROJECT_BINARY_DIR}/${SHADER_OUTPUT_PATH}.dxil")

    #Ensure output directory exists before trying to write to it
    get_filename_component(OUTPUT_DIRECTORY ${SPV_OUTPUT} DIRECTORY)
    file(MAKE_DIRECTORY ${OUTPUT_DIRECTORY})


    #Add include directory to shader files for the root directory of source tree
    set(SHADER_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src")

    set(LOCAL_SHADER_OUTPUTS "")

    if(NEKI_BUILD_VULKAN)
        add_custom_command(
                OUTPUT ${SPV_OUTPUT}
                COMMAND ${DXC_EXECUTABLE}
                ${SHADER_SOURCE}
                -I ${SHADER_INCLUDE_PATH}
                -T ${SHADER_TYPE}
                -E ${ENTRY_POINT}
                -spirv
                -Fo ${SPV_OUTPUT}
                DEPENDS ${SHADER_SOURCE}
                COMMENT "Compiling ${SHADER_SOURCE} to SPIR-V"
        )
        list(APPEND LOCAL_SHADER_OUTPUTS ${SPV_OUTPUT})
    endif()

    if(NEKI_BUILD_D3D12)
        add_custom_command(
                OUTPUT ${DXIL_OUTPUT}
                COMMAND ${DXC_EXECUTABLE}
                ${SHADER_SOURCE}
                -I ${SHADER_INCLUDE_PATH}
                -T ${SHADER_TYPE}
                -E ${ENTRY_POINT}
                -Fo ${DXIL_OUTPUT}
                DEPENDS ${SHADER_SOURCE}
                COMMENT "Compiling ${SHADER_SOURCE} to DXIL"
        )
        list(APPEND LOCAL_SHADER_OUTPUTS ${DXIL_OUTPUT})
    endif()


    #Append to output list
    set(${OUTPUT_LIST} ${LOCAL_SHADER_OUTPUTS} PARENT_SCOPE)


endfunction()



function(CompileSlangShader SHADER_SOURCE SHADER_STAGE ENTRY_POINT OUTPUT_LIST)

    file(RELATIVE_PATH SHADER_RELATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${SHADER_SOURCE})

    get_filename_component(SHADER_DIRECTORY ${SHADER_RELATIVE_PATH} PATH)
    get_filename_component(SHADER_NAME_WE ${SHADER_RELATIVE_PATH} NAME_WE)

    if(SHADER_DIRECTORY)
        set(SHADER_OUTPUT_PATH "${SHADER_DIRECTORY}/${SHADER_NAME_WE}")
    else()
        set(SHADER_OUTPUT_PATH "${SHADER_NAME_WE}")
    endif()

    set(SPV_OUTPUT "${PROJECT_BINARY_DIR}/${SHADER_OUTPUT_PATH}.spv")
    set(DXIL_OUTPUT "${PROJECT_BINARY_DIR}/${SHADER_OUTPUT_PATH}.dxil")

    get_filename_component(OUTPUT_DIRECTORY ${SPV_OUTPUT} DIRECTORY)
    file(MAKE_DIRECTORY ${OUTPUT_DIRECTORY})

    set(SHADER_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src")
    set(LOCAL_SHADER_OUTPUTS "")

    if(NEKI_BUILD_VULKAN)
        add_custom_command(
                OUTPUT ${SPV_OUTPUT}
                COMMAND ${SLANGC_EXECUTABLE}
                ${SHADER_SOURCE}
                -I ${SHADER_INCLUDE_PATH}
                -target spirv
                -stage ${SHADER_STAGE}
                -entry ${ENTRY_POINT}
                -o ${SPV_OUTPUT}
                DEPENDS ${SHADER_SOURCE}
                COMMENT "Compiling ${SHADER_SOURCE} to SPIR-V (Slang)"
        )
        list(APPEND LOCAL_SHADER_OUTPUTS ${SPV_OUTPUT})
    endif()

    if(NEKI_BUILD_D3D12)
        add_custom_command(
                OUTPUT ${DXIL_OUTPUT}
                COMMAND ${SLANGC_EXECUTABLE}
                ${SHADER_SOURCE}
                -I ${SHADER_INCLUDE_PATH}
                -target dxil
                -stage ${SHADER_STAGE}
                -entry ${ENTRY_POINT}
                -o ${DXIL_OUTPUT}
                DEPENDS ${SHADER_SOURCE}
                COMMENT "Compiling ${SHADER_SOURCE} to DXIL (Slang)"
        )
        list(APPEND LOCAL_SHADER_OUTPUTS ${DXIL_OUTPUT})
    endif()

    set(${OUTPUT_LIST} ${LOCAL_SHADER_OUTPUTS} PARENT_SCOPE)

endfunction()



#Shader compilation

#Define shader files
file(GLOB_RECURSE SHADER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.hlsl" "${CMAKE_CURRENT_SOURCE_DIR}/Samples/Graphics/Shaders/*.hlsl")
message(STATUS "---- Found Shader Files ----")
foreach(FILE ${SHADER_SOURCES})
    message(STATUS "  ${FILE}")
endforeach()
message(STATUS "--------------------------------")

#Compile all shaders
set(ALL_SHADER_OUTPUTS "")
foreach(SHADER_SRC ${SHADER_SOURCES})
    set(CURRENT_SHADER_OUTPUTS "")
    if(SHADER_SRC MATCHES ".*_vs\\.hlsl$")
        CompileShader(${SHADER_SRC} "vs_6_9" "VSMain" CURRENT_SHADER_OUTPUTS)
    elseif(SHADER_SRC MATCHES ".*_fs\\.hlsl$")
        CompileShader(${SHADER_SRC} "ps_6_9" "FSMain" CURRENT_SHADER_OUTPUTS)
    elseif(SHADER_SRC MATCHES ".*_cs\\.hlsl$")
        CompileShader(${SHADER_SRC} "cs_6_9" "CSMain" CURRENT_SHADER_OUTPUTS)
    endif()

    list(APPEND ALL_SHADER_OUTPUTS ${CURRENT_SHADER_OUTPUTS})
endforeach()


#Compile all Slang shaders
file(GLOB_RECURSE SLANG_SHADER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.slang"
        "${CMAKE_CURRENT_SOURCE_DIR}/Samples/Graphics/Shaders/*.slang"
)
if(SLANG_SHADER_SOURCES)
    message(STATUS "---- Found Slang Shader Files ----")
    foreach(FILE ${SLANG_SHADER_SOURCES})
        message(STATUS "  ${FILE}")
    endforeach()
    message(STATUS "-----------------------------------")
endif()

foreach(SHADER_SRC ${SLANG_SHADER_SOURCES})
    set(CURRENT_SHADER_OUTPUTS "")
    if(SHADER_SRC MATCHES ".*_vs\\.slang$")
        CompileSlangShader(${SHADER_SRC} "vertex" "main" CURRENT_SHADER_OUTPUTS)
    elseif(SHADER_SRC MATCHES ".*_fs\\.slang$")
        CompileSlangShader(${SHADER_SRC} "fragment" "main" CURRENT_SHADER_OUTPUTS)
    elseif(SHADER_SRC MATCHES ".*_cs\\.slang$")
        CompileSlangShader(${SHADER_SRC} "compute" "main" CURRENT_SHADER_OUTPUTS)
    endif()
    list(APPEND ALL_SHADER_OUTPUTS ${CURRENT_SHADER_OUTPUTS})
endforeach()


#Create phony target that depends on all the compiled shaders
add_custom_target(
        Shaders
        DEPENDS ${ALL_SHADER_OUTPUTS}
)

add_dependencies(Neki Shaders)




#Test executables - check only passes if building the Neki library directly, not when it's being included by another project via FetchContent
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)


    #Copy all resource files to the output directory, maintaining subdirectory structure
    add_custom_target(CopyNekiResources ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Samples/Resource-Files"
            "${CMAKE_BINARY_DIR}/Samples/Resource-Files"
            COMMENT "Copying engine resource files..."
    )



    #Create executables

    #Graphics
    if(NEKI_BUILD_VULKAN)
        add_executable(NKVulkanSample_Library "Samples/Graphics/Vulkan/Library/Library.cpp")
        target_include_directories(NKVulkanSample_Library PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKVulkanSample_Library PRIVATE Neki)
        add_dependencies(NKVulkanSample_Library Shaders)
    endif()
    if(NEKI_BUILD_D3D12)
        add_executable(NKD3D12Sample_Library "Samples/Graphics/D3D12/Library/Library.cpp")
        target_include_directories(NKD3D12Sample_Library PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKD3D12Sample_Library PRIVATE Neki)
        add_dependencies(NKD3D12Sample_Library Shaders)
    endif()

    if(NEKI_BUILD_VULKAN)
        add_executable(NKVulkanSample_Triangle "Samples/Graphics/Vulkan/Triangle/Triangle.cpp")
        target_include_directories(NKVulkanSample_Triangle PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKVulkanSample_Triangle PRIVATE Neki)
        add_dependencies(NKVulkanSample_Triangle Shaders)
    endif()
    if(NEKI_BUILD_D3D12)
        add_executable(NKD3D12Sample_Triangle "Samples/Graphics/D3D12/Triangle/Triangle.cpp")
        target_include_directories(NKD3D12Sample_Triangle PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKD3D12Sample_Triangle PRIVATE Neki)
        add_dependencies(NKD3D12Sample_Triangle Shaders)
    endif()

    if(NEKI_BUILD_VULKAN)
        add_executable(NKVulkanSample_Texture "Samples/Graphics/Vulkan/Texture/Texture.cpp")
        target_include_directories(NKVulkanSample_Texture PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKVulkanSample_Texture PRIVATE Neki)
        add_dependencies(NKVulkanSample_Texture Shaders)
    endif()
    if(NEKI_BUILD_D3D12)
        add_executable(NKD3D12Sample_Texture "Samples/Graphics/D3D12/Texture/Texture.cpp")
        target_include_directories(NKD3D12Sample_Texture PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKD3D12Sample_Texture PRIVATE Neki)
        add_dependencies(NKD3D12Sample_Texture Shaders)
    endif()

    if(NEKI_BUILD_VULKAN)
        add_executable(NKVulkanSample_Cube "Samples/Graphics/Vulkan/Cube/Cube.cpp")
        target_include_directories(NKVulkanSample_Cube PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKVulkanSample_Cube PRIVATE Neki)
        add_dependencies(NKVulkanSample_Cube Shaders)
    endif()
    if(NEKI_BUILD_D3D12)
        add_executable(NKD3D12Sample_Cube "Samples/Graphics/D3D12/Cube/Cube.cpp")
        target_include_directories(NKD3D12Sample_Cube PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKD3D12Sample_Cube PRIVATE Neki)
        add_dependencies(NKD3D12Sample_Cube Shaders)
    endif()

    if(NEKI_BUILD_VULKAN)
        add_executable(NKVulkanSample_Camera "Samples/Graphics/Vulkan/Camera/Camera.cpp")
        target_include_directories(NKVulkanSample_Camera PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKVulkanSample_Camera PRIVATE Neki)
        add_dependencies(NKVulkanSample_Camera Shaders)
    endif()
    if(NEKI_BUILD_D3D12)
        add_executable(NKD3D12Sample_Camera "Samples/Graphics/D3D12/Camera/Camera.cpp")
        target_include_directories(NKD3D12Sample_Camera PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKD3D12Sample_Camera PRIVATE Neki)
        add_dependencies(NKD3D12Sample_Camera Shaders)
    endif()

    if(NEKI_BUILD_VULKAN)
        add_executable(NKVulkanSample_Model "Samples/Graphics/Vulkan/Model/Model.cpp")
        target_include_directories(NKVulkanSample_Model PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKVulkanSample_Model PRIVATE Neki)
        add_dependencies(NKVulkanSample_Model Shaders)
    endif()
    if(NEKI_BUILD_D3D12)
        add_executable(NKD3D12Sample_Model "Samples/Graphics/D3D12/Model/Model.cpp")
        target_include_directories(NKD3D12Sample_Model PUBLIC "${CMAKE_SOURCE_DIR}/src")
        target_link_libraries(NKD3D12Sample_Model PRIVATE Neki)
        add_dependencies(NKD3D12Sample_Model Shaders)
    endif()



    #Engine
    add_executable(NKEngineSample_ECS "Samples/Engine/ECS/ECS.cpp")
    target_include_directories(NKEngineSample_ECS PUBLIC "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(NKEngineSample_ECS PRIVATE Neki)
    add_dependencies(NKEngineSample_ECS Shaders)

    add_executable(NKEngineSample_Rendering "Samples/Engine/Rendering/Rendering.cpp")
    target_include_directories(NKEngineSample_Rendering PUBLIC "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(NKEngineSample_Rendering PRIVATE Neki)
    add_dependencies(NKEngineSample_Rendering Shaders)

    add_executable(NKEngineSample_Events "Samples/Engine/Events/Events.cpp")
    target_include_directories(NKEngineSample_Events PUBLIC "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(NKEngineSample_Events PRIVATE Neki)
    add_dependencies(NKEngineSample_Events Shaders)

    add_executable(NKEngineSample_Networking_Server "Samples/Engine/Networking/Networking_Server.cpp" "Samples/Engine/Networking/PlayerLayer.cpp")
    target_include_directories(NKEngineSample_Networking_Server PUBLIC "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(NKEngineSample_Networking_Server PRIVATE Neki)
    add_dependencies(NKEngineSample_Networking_Server Shaders)

    add_executable(NKEngineSample_Networking_Client "Samples/Engine/Networking/Networking_Client.cpp" "Samples/Engine/Networking/PlayerLayer.cpp")
    target_include_directories(NKEngineSample_Networking_Client PUBLIC "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(NKEngineSample_Networking_Client PRIVATE Neki)
    add_dependencies(NKEngineSample_Networking_Client Shaders)

    add_executable(NKEngineSample_Physics "Samples/Engine/Physics/Physics.cpp")
    target_include_directories(NKEngineSample_Physics PUBLIC "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(NKEngineSample_Physics PRIVATE Neki)
    add_dependencies(NKEngineSample_Physics Shaders)
    
    
    
    add_subdirectory(NTCDemo)
#    add_subdirectory(AssetCompiler)

endif()