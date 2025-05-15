cmake_minimum_required(VERSION 3.10)

# Setting default cpp and compiler behaviour
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++)

# Setting default paths for cmake
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)

# Setting project
project(hello VERSION 1.0)

add_executable("${PROJECT_NAME}" "src/main.cpp")

target_include_directories(hello PUBLIC "include")


