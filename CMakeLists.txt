cmake_minimum_required(VERSION 3.13)  # CMake version check
project(strdist)                           # Create project "simple_example"
set(CMAKE_CXX_STANDARD 20)            # Enable c++20 standard
set(CMAKE_C_STANDARD 20)              # Enable c20 standard

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES strdist.c)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(strdist ${SOURCE_FILES})

# Create the run command for cmake
add_custom_target(run
  COMMAND strdist
  DEPENDS strdist
  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
