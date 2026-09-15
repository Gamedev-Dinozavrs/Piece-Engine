cmake_minimum_required(VERSION 3.10)
project(PieceEditor)

file(GLOB SOURCES "src/*.cpp" "src/*.h")

add_executable(PieceEditor ${SOURCES})

target_compile_features(PieceEditor PRIVATE cxx_std_17)

target_include_directories(PieceEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/PieceLib/src
    ${CMAKE_SOURCE_DIR}/PieceEditor/src
)

if (MSVC)
	target_compile_options(PieceEditor PRIVATE /utf-8)
endif()

target_link_libraries(PieceEditor PRIVATE PieceLib)
