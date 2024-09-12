cmake_minimum_required(VERSION 3.10)
project(PieceEditor)

file(GLOB SOURCES "src/*.cpp" "src/*.h")

add_executable(PieceEditor ${SOURCES})

target_link_libraries(PieceEditor PRIVATE PieceLib)
