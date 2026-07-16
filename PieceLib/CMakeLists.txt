cmake_minimum_required(VERSION 3.10)
project(PieceLib)

if (DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    list(APPEND CMAKE_PREFIX_PATH "${VULKAN_SDK_PATH}")
endif()

find_package(Vulkan REQUIRED)

file(GLOB CORE CONFIGURE_DEPENDS "src/core/*.cpp" "src/core/*.h")
file(GLOB WINDOW CONFIGURE_DEPENDS "src/window/*.cpp" "src/window/*.h")
file(GLOB EVENT CONFIGURE_DEPENDS "src/event/*.cpp" "src/event/*.h")
file(GLOB LAYER CONFIGURE_DEPENDS "src/layer/*.cpp" "src/layer/*.h")
file(GLOB RENDERER CONFIGURE_DEPENDS "src/renderer/*.cpp" "src/renderer/*.h")
file(GLOB SCENE CONFIGURE_DEPENDS "src/scene/*.cpp" "src/scene/*.h")
file(GLOB_RECURSE UTILS CONFIGURE_DEPENDS "src/utils/*.cpp" "src/utils/*.h")

set(CORE_SOURCES
    src/core/Application.cpp
    src/layer/Layer.cpp
    src/layer/LayerStack.cpp
    src/scene/EditorCamera.cpp
)

set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_vulkan.cpp
)

add_library(PieceLib STATIC ${CORE} ${WINDOW} ${EVENT} ${LAYER} ${RENDERER} ${SCENE} ${UTILS} ${CORE_SOURCES} ${IMGUI_SOURCES} ${CMAKE_SOURCE_DIR}/Dependencies/stb_image/stb_image.cpp)

target_precompile_headers(PieceLib PRIVATE src/PiecePCH.h)

target_compile_features(PieceLib PUBLIC cxx_std_17)

target_compile_definitions(PieceLib
    PRIVATE
        PIECE_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/shaders"
)

target_include_directories(PieceLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/Dependencies/glfw/include
        ${CMAKE_SOURCE_DIR}/Dependencies/glm
        ${CMAKE_SOURCE_DIR}/Dependencies/stb_image
        ${CMAKE_SOURCE_DIR}/Dependencies/imgui
        ${CMAKE_SOURCE_DIR}/Dependencies/imgui/backends
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
source_group("Renderer" FILES ${RENDERER})
source_group("Scene" FILES ${SCENE})
source_group("Utils" FILES ${UTILS})
