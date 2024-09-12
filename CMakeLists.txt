cmake_minimum_required(VERSION 3.10)

project(Piece VERSION 1.0 LANGUAGES CXX)

if (WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
endif()

if (UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
endif()

if (APPLE)
    add_definitions(-DPLATFORM_MAC)
endif()

add_subdirectory(PieceLib)
add_subdirectory(PieceEditor)
