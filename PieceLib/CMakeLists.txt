cmake_minimum_required(VERSION 3.10)
project(PieceLib)

if (DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    list(APPEND CMAKE_PREFIX_PATH "${VULKAN_SDK_PATH}")
endif()

find_package(Vulkan REQUIRED)

file(GLOB CORE "src/core/*.cpp" "src/core/*.h")
file(GLOB WINDOW "src/window/*.cpp" "src/window/*.h")
file(GLOB EVENT "src/event/*.cpp" "src/event/*.h")
file(GLOB LAYER "src/layer/*.cpp" "src/layer/*.h")

add_library(PieceLib STATIC ${CORE} ${WINDOW} ${EVENT} ${LAYER})

target_include_directories(PieceLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/Dependencies/glfw/include
        ${CMAKE_SOURCE_DIR}/Dependencies/glm
)

target_link_libraries(PieceLib
    PRIVATE
        glfw
        glm
        Vulkan::Vulkan
)

source_group("Core" FILES ${CORE})
source_group("Window" FILES ${WINDOW})
source_group("Event" FILES ${EVENT})
source_group("Layer" FILES ${LAYER})
