cmake_minimum_required(VERSION 3.6)
project(aries)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES src/sourceFiles/main.cpp src/headFiles/main.h src/headFiles/network.h src/headFiles/systemParameter.h src/sourceFiles/systemParameter.cpp src/sourceFiles/network.cpp src/headFiles/machineNode.h src/sourceFiles/machineNode.cpp src/headFiles/machineLink.h src/sourceFiles/machineLink.cpp src/headFiles/struct.h src/sourceFiles/assistFunction.cpp src/headFiles/assistFunction.h)
add_executable(aries ${SOURCE_FILES})