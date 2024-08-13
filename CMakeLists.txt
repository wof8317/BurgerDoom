cmake_minimum_required(VERSION 3.15)
project(BurgerDoom)

# Where CMake will search for .cmake module files
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Causes CMake projects to go into their own folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Include common build scripts and do basic build setup
include(HelperFunctions)
include(BuildSetup)

build_setup()

# Global identifiers for each project/target
set(GAME_NAME       BurgerDoom)

# Adding individual projects and libraries
set(GAME_SRC_DIR            "${PROJECT_SOURCE_DIR}/source")

add_subdirectory("${GAME_SRC_DIR}")
