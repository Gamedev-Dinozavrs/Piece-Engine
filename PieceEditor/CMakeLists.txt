cmake_minimum_required(VERSION 3.10)
project(PieceEditor)

if (NOT DEFINED VULKAN_SDK_PATH)
    message(FATAL_ERROR "VULKAN_SDK_PATH is not defined. Make sure the parent CMakeLists.txt is correctly setting this variable.")
endif()

file(GLOB SOURCES "src/*.cpp" "src/*.h")

include_directories(${VULKAN_SDK_PATH}/Include)
link_directories(${VULKAN_SDK_PATH}/Lib)

add_executable(PieceEditor ${SOURCES})

target_link_libraries(PieceEditor PRIVATE PieceLib)
