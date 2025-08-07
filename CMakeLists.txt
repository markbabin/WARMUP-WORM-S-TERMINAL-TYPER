cmake_minimum_required(VERSION 3.10)

# Project name and version
project(tt VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ncurses library
find_library(NCURSES_LIBRARY ncurses REQUIRED)
find_path(NCURSES_INCLUDE_DIR ncurses.h)

# Add compiler flags for better debugging and warnings
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Create executable for the main typing test
add_executable(tt claudetypingtest.cpp)
target_link_libraries(tt ${NCURSES_LIBRARY})
if(NCURSES_INCLUDE_DIR)
    target_include_directories(tt PRIVATE ${NCURSES_INCLUDE_DIR})
endif()

# Create executable for basic ncurses example
add_executable(basic_ncurses ncurses.cpp)
target_link_libraries(basic_ncurses ${NCURSES_LIBRARY})
if(NCURSES_INCLUDE_DIR)
    target_include_directories(basic_ncurses PRIVATE ${NCURSES_INCLUDE_DIR})
endif()

# Create executable for ncurses tutorial
add_executable(ncurses_tutorial ncurses_tut.cpp)
target_link_libraries(ncurses_tutorial ${NCURSES_LIBRARY})
if(NCURSES_INCLUDE_DIR)
    target_include_directories(ncurses_tutorial PRIVATE ${NCURSES_INCLUDE_DIR})
endif()

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
