cmake_minimum_required(VERSION 3.10)
project(Sandbox)

file(GLOB SOURCES "src/*.cpp" "src/*.h")

add_executable(Sandbox ${SOURCES})

target_compile_features(Sandbox PRIVATE cxx_std_17)

target_include_directories(Sandbox PRIVATE
    ${CMAKE_SOURCE_DIR}/PieceLib/src
    ${CMAKE_SOURCE_DIR}/Sandbox/src
)

if (MSVC)
	target_compile_options(Sandbox PRIVATE /utf-8)
endif()

target_link_libraries(Sandbox PRIVATE PieceLib)
