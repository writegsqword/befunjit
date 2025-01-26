cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Global properties
# set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Project name
project(befunjit)


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

file(GLOB_RECURSE SOURCES src/*.cpp)


add_executable(befunjit ${SOURCES})
