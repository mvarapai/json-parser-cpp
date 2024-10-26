cmake_minimum_required(VERSION 3.10)

# Project name and version
project(json-parser-cpp VERSION 1.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add include and source directories
include_directories(src/include)

add_executable(json-parser-cpp src/main.cpp  "src/utilstr.cpp" "src/include/utilstr.h")