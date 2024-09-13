cmake_minimum_required(VERSION 3.10)
project(PieceLib)

if (DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
else()
    message(FATAL_ERROR "Please set the VULKAN_SDK environment variable.")
endif()

file(GLOB CORE "src/core/*.cpp" "src/core/*.h")

add_library(PieceLib STATIC ${CORE})

target_include_directories(PieceLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${VULKAN_SDK_PATH}/Include
	${CMAKE_SOURCE_DIR}/Dependencies/glfw/include
        ${CMAKE_SOURCE_DIR}/Dependencies/glm/
)

target_link_libraries(PieceLib
    PRIVATE
        glfw
        glm
        ${VULKAN_SDK_PATH}/Lib/vulkan-1.lib
)
