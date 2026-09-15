cmake_minimum_required(VERSION 3.10)
project(Piece VERSION 1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
else()
    message(FATAL_ERROR "Please Install Vulkan SDK and put VULKAN_SDK env variable to Vulkan version folder!")
endif()

if (WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif (UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
elseif (APPLE)
    add_definitions(-DPLATFORM_MAC)
endif()

set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_DOCS OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(PieceLib)
add_subdirectory(Dependencies/GLFW)
add_subdirectory(Dependencies/GLM)
add_subdirectory(Dependencies/fastgltf)
add_subdirectory(PieceEditor)
