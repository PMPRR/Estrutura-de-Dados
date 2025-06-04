# cmake_minimum_required(VERSION 3.10)
# 
# # Setting default cpp and compiler behaviour
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_COMPILER g++)
# 
# # Setting default paths for cmake
# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
# 
# # Find ZeroMQ using pkg-config
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(ZMQ REQUIRED libzmq>=4.2.5)
# 
# # Check if ZeroMQ components were found
# if (NOT ZMQ_LIBRARY)
#     message(FATAL_ERROR "ZeroMQ library not found. Please ensure libzmq3-dev is installed.")
# endif()
# #if (NOT ZMQ_INCLUDE_DIR)
# #    message(FATAL_ERROR "ZeroMQ include directory (zmq.hpp) not found. Please ensure libzmq3-dev is installed.")
# #endif()
# 
# # Adds testing features
# enable_testing()
# 
# # Setting project
# project(hello VERSION 1.0 LANGUAGES CXX)
# 
# # get all CPP files, except main.cpp
# file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")
# file(GLOB_RECURSE MAIN_SOURCE "src/main.cpp")
# list(REMOVE_ITEM ALL_SOURCES ${MAIN_SOURCE})
# 
# # main project
# add_executable(${PROJECT_NAME} ${MAIN_SOURCE})
# 
# target_link_libraries(${PROJECT_NAME} PRIVATE core)
# #target_link_libraries(${PROJECT_NAME} PRIVATE ${ZMQ_LIBRARY})
# target_link_libraries(${PROJECT_NAME} PRIVATE ${ZMQ_LIBRARIES})
# 
# target_include_directories(${PROJECT_NAME} PUBLIC "include")
# target_include_directories(${PROJECT_NAME} PRIVATE ${ZMQ_INCLUDE_DIRS})
# # target_link_libraries(${PROJECT_NAME} PRIVATE ${ZeroMQ_LIBRARIES})
# 
# 
# # Create static library with all non-main sources
# add_library(core STATIC ${ALL_SOURCES})
# target_include_directories(core PUBLIC "include")
# 
# # tests
# file(GLOB TEST_SOURCES "test/*.cpp")
# foreach(test_src ${TEST_SOURCES})
#     get_filename_component(test_name ${test_src} NAME_WE)
#     add_executable(${test_name} ${test_src})
#     target_link_libraries(${test_name} PRIVATE core)
#     #target_link_libraries(${test_name} PRIVATE ${ZMQ_LIBRARY})
#     target_link_libraries(${test_name} PRIVATE ${ZMQ_LIBRARIES})
# 
#     # target_link_libraries(${test_name} PRIVATE ${ZeroMQ_LIBRARIES})
#     target_include_directories(${test_name} PUBLIC "include")  # ⬅️ Optional: if tests need the headers
#     target_include_directories(${test_name} PRIVATE ${ZMQ_INCLUDE_DIRS})
#     add_test(NAME ${test_name} COMMAND ${test_name})
# endforeach()
#

cmake_minimum_required(VERSION 3.10)

# Setting default cpp and compiler behaviour
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++)

# Setting default paths for cmake
# It's generally better to use CMAKE_BINARY_DIR for build artifacts
# rather than CMAKE_SOURCE_DIR to keep the source tree clean.
# However, retaining user's preference for now.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}) # Or consider ${CMAKE_BINARY_DIR}/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin) # Or consider ${CMAKE_BINARY_DIR}/bin

# Find ZeroMQ using pkg-config
find_package(PkgConfig REQUIRED)
# This command will halt if libzmq>=4.2.5 is not found by pkg-config
# and will populate ZMQ_LIBRARIES and ZMQ_INCLUDE_DIRS if found.
pkg_check_modules(ZMQ REQUIRED libzmq>=4.2.5)

# The custom check for ZMQ_LIBRARY has been removed as it's redundant
# with `pkg_check_modules(ZMQ REQUIRED ...)` and was using a non-standard variable.
# If libzmq was not found, CMake would have already issued a FATAL_ERROR.

# Adds testing features
enable_testing()

# Setting project
project(hello VERSION 1.0 LANGUAGES CXX)

# get all CPP files, except main.cpp
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")
file(GLOB_RECURSE MAIN_SOURCE "src/main.cpp") # Assumes only one main.cpp
list(REMOVE_ITEM ALL_SOURCES ${MAIN_SOURCE})

# main project
add_executable(${PROJECT_NAME} ${MAIN_SOURCE})

# Link against the 'core' static library and ZeroMQ
target_link_libraries(${PROJECT_NAME} PRIVATE core)
target_link_libraries(${PROJECT_NAME} PRIVATE ${ZMQ_LIBRARIES}) # Correct variable from pkg_check_modules

# Include directories for the main project
target_include_directories(${PROJECT_NAME} PUBLIC "include") # Project's own include directory
target_include_directories(${PROJECT_NAME} PRIVATE ${ZMQ_INCLUDE_DIRS}) # ZeroMQ include directories

# Create static library with all non-main sources
add_library(core STATIC ${ALL_SOURCES})
target_include_directories(core PUBLIC "include") # 'core' library's own include directory

# tests
file(GLOB TEST_SOURCES "test/*.cpp")
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    # Link tests against 'core' and ZeroMQ
    target_link_libraries(${test_name} PRIVATE core)
    target_link_libraries(${test_name} PRIVATE ${ZMQ_LIBRARIES}) # Correct variable

    # Include directories for tests
    target_include_directories(${test_name} PUBLIC "include") # Project's own include directory
    target_include_directories(${test_name} PRIVATE ${ZMQ_INCLUDE_DIRS}) # ZeroMQ include directories
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

