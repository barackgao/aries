cmake_minimum_required(VERSION 3.6)
project(aries)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES src/sourceFiles/main.cpp src/headFiles/main.h)
add_executable(aries ${SOURCE_FILES})