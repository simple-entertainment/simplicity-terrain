cmake_minimum_required(VERSION 3.2)

include(../simplicity/simplicity-common.cmake)

# Project
project(simplicity-terrain)

# Paths
file(GLOB_RECURSE SRC_FILES src/main/c++/*.cpp src/main/c++/*.h)
include_directories(../simplicity/src/main/c++)

# Output
add_library(simplicity-terrain STATIC ${SRC_FILES})
