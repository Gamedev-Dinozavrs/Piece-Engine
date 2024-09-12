cmake_minimum_required(VERSION 3.10)
project(PieceLib)

file(GLOB CORE "src/core/*.cpp" "src/core/*.h")
# file(GLOB UTILS_SOURCES "src/utils/*.cpp")

set(SOURCES ${CORE})

add_library(PieceLib STATIC ${SOURCES})

target_include_directories(PieceLib PUBLIC include)
