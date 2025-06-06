#
# criado pelo GEMINI 2.5 -> "https://g.co/gemini/share/904662306c45"


cmake_minimum_required(VERSION 3.10)

# Setting project first to enable languages
project(hello VERSION 1.0 LANGUAGES CXX)

# Setting default cpp and compiler behaviour
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++)

# Setting default paths for cmake
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# --- NEW: Option to control benchmark builds ---
# This will be OFF by default, so app-main and app-run don't try to build benchmarks.
option(BUILD_BENCHMARKS "Build the Google Benchmark tests" OFF)


# Find ZeroMQ using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq>=4.2.5)

# Find Threads package (for pthreads)
find_package(Threads REQUIRED)

# Adds testing features
enable_testing()

# get all CPP files, except main.cpp
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")
file(GLOB_RECURSE MAIN_SOURCE "src/main.cpp") 
list(REMOVE_ITEM ALL_SOURCES ${MAIN_SOURCE})

# main project
add_executable(${PROJECT_NAME} ${MAIN_SOURCE})

# Link against the 'core' static library, ZeroMQ, and Threads (pthreads)
target_link_libraries(${PROJECT_NAME} PRIVATE core ${ZMQ_LIBRARIES} Threads::Threads)

# Include directories for the main project
target_include_directories(${PROJECT_NAME} PUBLIC "include" PRIVATE ${ZMQ_INCLUDE_DIRS})

# Create static library with all non-main sources
add_library(core STATIC ${ALL_SOURCES})
target_include_directories(core PUBLIC "include") 

# CTest functional tests (your existing tests)
file(GLOB TEST_SOURCES "test/*.cpp")
# Exclude the new benchmark file from the functional tests
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*benchmark.cpp$")
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE core ${ZMQ_LIBRARIES} Threads::Threads)
    target_include_directories(${test_name} PUBLIC "include" PRIVATE ${ZMQ_INCLUDE_DIRS})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()


# --- NEW: Conditional Benchmark Section ---
# This entire section will only be processed if BUILD_BENCHMARKS is set to ON
if(BUILD_BENCHMARKS)
    message(STATUS "Benchmark build enabled. Looking for Google Benchmark library...")
    find_package(benchmark REQUIRED)
    
    add_executable(benchmarks test/benchmark.cpp)
    
    # Link the benchmark executable against your 'core' library and Google Benchmark
    target_link_libraries(benchmarks PRIVATE core benchmark::benchmark Threads::Threads)
    target_include_directories(benchmarks PUBLIC "include")
    
    message(STATUS "Benchmark target 'benchmarks' configured.")
endif()

