cmake_minimum_required(VERSION 3.10)
project(PieceEditor)

file(GLOB SOURCES "src/*.cpp" "src/*.h")

add_executable(PieceEditor ${SOURCES})

if (MSVC)
	target_compile_options(PieceEditor PRIVATE /utf-8)
endif()

target_link_libraries(PieceEditor PRIVATE PieceLib)
