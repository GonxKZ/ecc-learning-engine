# CMakeLists.txt for ECScope Plugin Examples
cmake_minimum_required(VERSION 3.16)
project(ecscope_plugin_examples)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ECScope framework (in a real setup, this would find the installed ECScope)
# For now, we'll set up the include paths manually
set(ECSCOPE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../include")
set(ECSCOPE_LIB_DIR "${CMAKE_BINARY_DIR}/../../lib")

# Include directories
include_directories(${ECSCOPE_INCLUDE_DIR})

# Plugin build configuration
function(add_ecscope_plugin PLUGIN_NAME SOURCE_FILE)
    # Create plugin library
    add_library(${PLUGIN_NAME}_plugin SHARED ${SOURCE_FILE})
    
    # Set plugin properties
    set_target_properties(${PLUGIN_NAME}_plugin PROPERTIES
        OUTPUT_NAME "${PLUGIN_NAME}"
        PREFIX ""
        SUFFIX ".ecplugin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    )
    
    # Link with system libraries
    if(WIN32)
        target_link_libraries(${PLUGIN_NAME}_plugin kernel32 user32)
    else()
        target_link_libraries(${PLUGIN_NAME}_plugin dl)
    endif()
    
    # Compiler-specific settings
    if(MSVC)
        target_compile_definitions(${PLUGIN_NAME}_plugin PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
    
    # Create plugin manifest
    set(MANIFEST_CONTENT "{
    \"name\": \"${PLUGIN_NAME}\",
    \"display_name\": \"${PLUGIN_NAME} Plugin\",
    \"version\": \"1.0.0\",
    \"api_version\": \"1.0\",
    \"description\": \"Example plugin for ECScope\",
    \"author\": \"ECScope Team\",
    \"license\": \"MIT\",
    \"sandbox_required\": true,
    \"memory_limit\": 52428800,
    \"cpu_time_limit\": 100
}")
    
    file(WRITE "${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}/plugin.json" "${MANIFEST_CONTENT}")
    
    # Install targets
    install(TARGETS ${PLUGIN_NAME}_plugin
        LIBRARY DESTINATION plugins
        RUNTIME DESTINATION plugins
    )
    
    install(FILES "${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}/plugin.json"
        DESTINATION plugins/${PLUGIN_NAME}
    )
endfunction()

# Build example plugins
add_ecscope_plugin(hello_world hello_world_plugin.cpp)
add_ecscope_plugin(rendering_demo rendering_demo_plugin.cpp)
add_ecscope_plugin(ecs_component_demo ecs_component_plugin.cpp)

# Create plugin directory structure
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/hello_world")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/rendering_demo")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/ecs_component_demo")

# Copy example resources (if they exist)
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_SOURCE_DIR}/resources/" DESTINATION "${CMAKE_BINARY_DIR}/plugins/resources/")
endif()

# Create a test runner executable (optional)
if(EXISTS "${CMAKE_SOURCE_DIR}/plugin_test_runner.cpp")
    add_executable(plugin_test_runner plugin_test_runner.cpp)
    target_include_directories(plugin_test_runner PRIVATE ${ECSCOPE_INCLUDE_DIR})
    
    if(WIN32)
        target_link_libraries(plugin_test_runner kernel32 user32)
    else()
        target_link_libraries(plugin_test_runner dl)
    endif()
    
    # Copy test runner to plugins directory for convenience
    set_target_properties(plugin_test_runner PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    )
endif()

# Print build information
message(STATUS "Building ECScope Plugin Examples")
message(STATUS "Plugin output directory: ${CMAKE_BINARY_DIR}/plugins")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Development helpers
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Add debug flags for plugin development
    add_compile_definitions(ECSCOPE_PLUGIN_DEBUG=1)
    add_compile_definitions(ECSCOPE_ENABLE_LOGGING=1)
    
    # Enable additional warnings
    if(MSVC)
        add_compile_options(/W4)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif()
endif()

# Testing support (optional)
enable_testing()

# Add tests for each plugin (if test framework is available)
# add_test(NAME hello_world_plugin_test 
#          COMMAND plugin_test_runner hello_world
#          WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")

# Documentation generation (if Doxygen is available)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/docs")
    set(DOXYGEN_INPUT "${CMAKE_SOURCE_DIR}")
    
    configure_file(
        "${CMAKE_SOURCE_DIR}/Doxyfile.in"
        "${CMAKE_BINARY_DIR}/Doxyfile"
        @ONLY
    )
    
    add_custom_target(plugin_docs
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Generating plugin documentation"
    )
endif()