cmake_minimum_required(VERSION 3.28)
project(AssetCompiler LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

if(NOT TARGET Neki)
    FetchContent_Declare(
            Neki
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.."
    )
    FetchContent_MakeAvailable(Neki)
    set(NEKI_ROOT_SOURCE_DIR "${neki_SOURCE_DIR}")
    set(NEKI_ROOT_BINARY_DIR "${neki_BINARY_DIR}")
else()
    set(NEKI_ROOT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
    set(NEKI_ROOT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/..")
endif()

add_executable(AssetCompiler "src/main.cpp")
target_link_libraries(AssetCompiler PRIVATE Neki)

function(copy_directory_on_change NAME SRC_DIR DST_DIR)
    add_custom_target(${NAME} ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_DIR}" "${DST_DIR}"
            COMMENT "Copying ${NAME}..."
    )
    add_dependencies(AssetCompiler ${NAME})
endfunction()

copy_directory_on_change(CopyEngineResources
        "${NEKI_ROOT_SOURCE_DIR}/Samples/Resource-Files"
        "$<TARGET_FILE_DIR:AssetCompiler>/Samples/Resource-Files"
)

copy_directory_on_change(CopyEngineShaders
        "${NEKI_ROOT_BINARY_DIR}/Shaders"
        "$<TARGET_FILE_DIR:AssetCompiler>/Shaders"
)

copy_directory_on_change(CopyAppResources
        "${CMAKE_CURRENT_SOURCE_DIR}/Resource-Files"
        "$<TARGET_FILE_DIR:AssetCompiler>/Resource-Files"
)

copy_directory_on_change(CopyAppShaders
        "${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
        "$<TARGET_FILE_DIR:AssetCompiler>/Shaders"
)
add_dependencies(CopyEngineShaders Shaders)

copy_directory_on_change(CopyScenes
        "${CMAKE_CURRENT_SOURCE_DIR}/Scenes"
        "$<TARGET_FILE_DIR:AssetCompiler>/Scenes"
)