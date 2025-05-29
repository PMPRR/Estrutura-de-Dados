cmake_minimum_required(VERSION 3.10)

# Setting default cpp and compiler behaviour
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++)

# Setting default paths for cmake
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Adds testing features
enable_testing()

# Setting project
project(hello VERSION 1.0 LANGUAGES CXX)

# get all CPP files, except main.cpp
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")
file(GLOB_RECURSE MAIN_SOURCE "src/main.cpp")
list(REMOVE_ITEM ALL_SOURCES ${MAIN_SOURCE})

# main project
add_executable(${PROJECT_NAME} ${MAIN_SOURCE})
target_link_libraries(${PROJECT_NAME} PRIVATE core)
target_include_directories(${PROJECT_NAME} PUBLIC "include")


# Create static library with all non-main sources
add_library(core STATIC ${ALL_SOURCES})
target_include_directories(core PUBLIC "include")

# tests
file(GLOB TEST_SOURCES "test/*.cpp")
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE core)
    target_include_directories(${test_name} PUBLIC "include")  # ⬅️ Optional: if tests need the headers
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

