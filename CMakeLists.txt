cmake_minimum_required(VERSION 3.28)
project(Neki LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#Define library target
file(GLOB_RECURSE NEKI_SOURCES "src/*.cpp")
add_library(Neki STATIC ${NEKI_SOURCES})
target_include_directories(Neki PUBLIC src)



#Propagate public dependencies
include(FetchContent)

#Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(Neki PUBLIC Vulkan::Vulkan)

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








#Test executables - check only passes if building the Neki library directly, not when it's being included by another project via FetchContent
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    #Shader compilation
    find_program(
            GLSLC_EXECUTABLE
            NAMES glslc
            HINTS "$ENV{VULKAN_SDK}/bin"
            REQUIRED
            DOC "Path to the GLSL to SPIR-V compiler"
    )
    message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")

    #Define shader files
    file(GLOB_RECURSE SHADER_SOURCES
            "${CMAKE_SOURCE_DIR}/Samples/*/Shaders/*.vert"
            "${CMAKE_SOURCE_DIR}/Samples/*/Shaders/*.frag"
    )
    message(STATUS "---- Found Shader Files ----")
    foreach(FILE ${SHADER_SOURCES})
        message(STATUS "  ${FILE}")
    endforeach()
    message(STATUS "--------------------------------")

    #Get list of all the full paths of all the compiled .spv files and compile them
    set(SPIRV_OUTPUTS "")
    foreach(SHADER_SRC ${SHADER_SOURCES})
        get_filename_component(SHADER_FILENAME ${SHADER_SRC} NAME)
        set(SHADER_SPV "${PROJECT_BINARY_DIR}/Samples/*/Shaders/${SHADER_FILENAME}.spv")
        list(APPEND SPIRV_OUTPUTS ${SHADER_SPV})

        #Compile shader
        add_custom_command(
                OUTPUT ${SHADER_SPV}
                COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SRC} -o ${SHADER_SPV}
                DEPENDS ${SHADER_SRC}
                COMMENT "Compiling ${SHADER_SRC} to ${SHADER_SPV}"
        )
    endforeach()

    #Create phony target that depends on all the compiled shaders
    add_custom_target(
            Shaders
            DEPENDS ${SPIRV_OUTPUTS}
    )



    #Create executables
    add_executable(NKSample_Library "Samples/Library/Library.cpp")
    target_include_directories(NKSample_Library PUBLIC "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(NKSample_Library PRIVATE Neki)
    add_dependencies(NKSample_Library Shaders)

    # add_executable(Neki_ModelTest "/Samples/ModelTest/ModelTest.h" "/Samples/Model/Model.cpp")
    # target_link_libraries(Neki_ModelTest PRIVATE Neki)
    # add_dependencies(Neki_ModelTest Shaders)


endif()
