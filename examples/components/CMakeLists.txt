cmake_minimum_required(VERSION 3.20)
project(ECScope_Component_Examples)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(../../include)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Example executable
add_executable(comprehensive_example comprehensive_example.cpp)

# Link libraries
target_link_libraries(comprehensive_example 
    Threads::Threads
)

# Additional include directories for core types
target_include_directories(comprehensive_example PRIVATE
    ../../include/ecscope
    ../../include/ecscope/components
    ../../include/ecscope/foundation
    ../../include/ecscope/core
)

# Set output directory
set_target_properties(comprehensive_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)