cmake_minimum_required(VERSION 3.28)
project(SubmissionGame LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NEKI_BUILD_VULKAN ON CACHE BOOL "" FORCE)
set(NEKI_BUILD_D3D12 OFF CACHE BOOL "" FORCE)
set(NEKI_ENABLE_EDITOR ON CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
        Neki
        #Note: in an actual project, you would point to the git repo here, but for submission purposes (keeping everything in the same repo) we point to the parent directory
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

FetchContent_MakeAvailable(Neki)
add_executable(SubmissionGame "src/main.cpp")
target_link_libraries(SubmissionGame PRIVATE Neki)
add_custom_command(TARGET SubmissionGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${neki_SOURCE_DIR}/Samples/Resource-Files"
        "$<TARGET_FILE_DIR:SubmissionGame>/Samples/Resource-Files"
        COMMENT "Copying Engine Resources..."
)
add_custom_command(TARGET SubmissionGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${neki_BINARY_DIR}/Shaders"
        "$<TARGET_FILE_DIR:SubmissionGame>/Shaders"
        COMMENT "Copying Compiled Shaders..."
)


add_custom_command(TARGET SubmissionGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Scenes"
        "$<TARGET_FILE_DIR:SubmissionGame>/Scenes"
)