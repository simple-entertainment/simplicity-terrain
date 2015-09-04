cmake_minimum_required(VERSION 3.2)

include(../simplicity/simplicity-common.cmake)

# Project
#########################
project(simplicity-terrain)

# Source
#########################
file(GLOB_RECURSE SRC_FILES src/main/c++/*.cpp src/main/c++/*.h)

# Target
#########################
add_library(simplicity-terrain ${SIMPLE_LINK_TYPE} ${SRC_FILES})
target_include_directories(simplicity-terrain PUBLIC src/main/c++)

# Target Dependencies
#########################

# Simplicity
target_link_libraries(simplicity-terrain simplicity)
