cmake_minimum_required(VERSION 3.28)
project(NTCDemo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NEKI_BUILD_VULKAN ON CACHE BOOL "" FORCE)
set(NEKI_BUILD_D3D12 OFF CACHE BOOL "" FORCE)
set(NEKI_ENABLE_EDITOR ON CACHE BOOL "" FORCE)

include(FetchContent)

#Only fetch Neki when building standalone — skip when already a subdirectory of Neki
if(NOT TARGET Neki)
    set(NEKI_BUILD_VULKAN ON CACHE BOOL "" FORCE)
    set(NEKI_BUILD_D3D12 OFF CACHE BOOL "" FORCE)
    set(NEKI_ENABLE_EDITOR ON CACHE BOOL "" FORCE)

    include(FetchContent)

    FetchContent_Declare(
            Neki
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.."
    )
    FetchContent_MakeAvailable(Neki)
    set(NEKI_ROOT_SOURCE_DIR "${neki_SOURCE_DIR}")
    set(NEKI_ROOT_BINARY_DIR "${neki_BINARY_DIR}")
else()
    #Built as add_subdirectory() from the Neki root — derive paths from parent
    set(NEKI_ROOT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
    set(NEKI_ROOT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/..")
endif()

add_executable(NTCDemo "src/main.cpp")
target_link_libraries(NTCDemo PRIVATE Neki)

function(copy_directory_on_change NAME SRC_DIR DST_DIR)
    file(GLOB_RECURSE _files "${SRC_DIR}/*")
    set(_stamp "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.stamp")

    add_custom_command(
            OUTPUT  "${_stamp}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_DIR}" "${DST_DIR}"
            COMMAND ${CMAKE_COMMAND} -E touch "${_stamp}"
            DEPENDS ${_files}
            COMMENT "Copying ${NAME}..."
    )
    add_custom_target(${NAME} DEPENDS "${_stamp}")
    add_dependencies(NTCDemo ${NAME})
endfunction()

copy_directory_on_change(CopyEngineResources
        "${Neki_BINARY_DIR}/Samples/Resource-Files"
        "$<TARGET_FILE_DIR:NTCDemo>/Samples/Resource-Files"
)

copy_directory_on_change(CopyEngineShaders
        "${Neki_BINARY_DIR}/Shaders"
        "$<TARGET_FILE_DIR:NTCDemo>/Shaders"
)

copy_directory_on_change(CopyAppResources
        "${CMAKE_CURRENT_SOURCE_DIR}/Resource-Files"
        "$<TARGET_FILE_DIR:NTCDemo>/Resource-Files"
)

copy_directory_on_change(CopyAppShaders
        "${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
        "$<TARGET_FILE_DIR:NTCDemo>/Shaders"
)

copy_directory_on_change(CopyScenes
        "${CMAKE_CURRENT_SOURCE_DIR}/Scenes"
        "$<TARGET_FILE_DIR:NTCDemo>/Scenes"
)