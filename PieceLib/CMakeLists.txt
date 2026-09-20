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
file(GLOB GUI CONFIGURE_DEPENDS "src/GUI/*.cpp" "src/GUI/*.h")
file(GLOB ASSETS CONFIGURE_DEPENDS "src/assets/*.cpp" "src/assets/*.h")
file(GLOB_RECURSE RENDERER CONFIGURE_DEPENDS "src/renderer/*.cpp" "src/renderer/*.h")
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

set(IMGUIZMO_SOURCES
    ${CMAKE_SOURCE_DIR}/Dependencies/ImGuizmo/src/ImGuizmo.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/ImGuizmo/src/ImGuizmo.h
)

set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/Dependencies/ufbx/ufbx.c
    PROPERTIES LANGUAGE CXX)

add_library(PieceLib STATIC ${CORE} ${WINDOW} ${EVENT} ${LAYER} ${GUI} ${ASSETS} ${RENDERER} ${SCENE} ${UTILS} ${CORE_SOURCES} ${IMGUI_SOURCES} ${IMGUIZMO_SOURCES} ${CMAKE_SOURCE_DIR}/Dependencies/stb_image/stb_image.cpp ${CMAKE_SOURCE_DIR}/Dependencies/ufbx/ufbx.c)

target_precompile_headers(PieceLib PRIVATE src/PiecePCH.h)

target_compile_features(PieceLib PUBLIC cxx_std_17)

if (MSVC)
    target_compile_options(PieceLib PRIVATE /utf-8 /FS)
endif()

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
        ${CMAKE_SOURCE_DIR}/Dependencies/ImGuizmo/src
        ${CMAKE_SOURCE_DIR}/Dependencies/spdlog/include
        ${CMAKE_SOURCE_DIR}/Dependencies/entt/include
        ${CMAKE_SOURCE_DIR}/Dependencies/tinyobjloader
        ${CMAKE_SOURCE_DIR}/Dependencies/yaml-cpp/include
        ${CMAKE_SOURCE_DIR}/Dependencies/ufbx
)

target_link_libraries(PieceLib
    PRIVATE
        glfw
        glm
    fastgltf::fastgltf
        Vulkan::Vulkan
        yaml-cpp::yaml-cpp
)

source_group("Core" FILES ${CORE})
source_group("Window" FILES ${WINDOW})
source_group("Event" FILES ${EVENT})
source_group("Layer" FILES ${LAYER})
source_group("GUI" FILES ${GUI})
source_group("Assets" FILES ${ASSETS})
source_group("Renderer" FILES ${RENDERER})
source_group("Scene" FILES ${SCENE})
source_group("Utils" FILES ${UTILS})
