project(haywire)
cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_EXTENSIONS       OFF)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_subdirectory(src)
