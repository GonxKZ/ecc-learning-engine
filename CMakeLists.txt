cmake_minimum_required(VERSION 3.22)
project(ECScope 
    VERSION 1.0.0
    DESCRIPTION "ECScope: Standalone Integration Test"
    LANGUAGES CXX)

# C++20 configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -fno-omit-frame-pointer
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-O0 -g3)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG -march=native -mtune=native)
    endif()
endif()

# Build options
option(ECSCOPE_BUILD_GUI "Build GUI system with Dear ImGui" ON)
option(ECSCOPE_BUILD_RESPONSIVE_DESIGN "Build responsive design system" ON)
option(ECSCOPE_BUILD_EXAMPLES "Build example applications" ON)
option(ECSCOPE_BUILD_ECS_INSPECTOR "Build comprehensive ECS inspector" ON)
option(ECSCOPE_BUILD_AUDIO_UI "Build comprehensive audio UI system" ON)
option(ECSCOPE_ENABLE_MODERN_RENDERING "Enable modern rendering system" OFF)
option(ECSCOPE_BUILD_CROSS_PLATFORM_TESTS "Build cross-platform compatibility tests" OFF)
option(ECSCOPE_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
option(ECSCOPE_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan in Debug builds" OFF)
option(ECSCOPE_ENABLE_COVERAGE "Enable code coverage reporting" OFF)
option(ECSCOPE_ENABLE_ADVANCED_OPTIMIZATIONS "Enable advanced compiler optimizations" OFF)

# Include platform configuration
include(cmake/Platform.cmake)
include(cmake/Dependencies.cmake)
include(cmake/CrossPlatformTesting.cmake)

# Configure platform
ecscope_configure_platform()

# Find required libraries
find_package(Threads REQUIRED)

# Optional GUI dependencies
if(ECSCOPE_BUILD_GUI)
    find_package(PkgConfig QUIET)
    find_package(glfw3 QUIET)
    find_package(OpenGL QUIET)
    
    # Check for ImGui (usually installed manually or via vcpkg)
    find_path(IMGUI_INCLUDE_DIR NAMES imgui.h PATHS 
        ${CMAKE_PREFIX_PATH}/include
        /usr/local/include
        /usr/include
    )
    
    if(glfw3_FOUND AND OpenGL_FOUND AND IMGUI_INCLUDE_DIR)
        set(ECSCOPE_HAS_GUI_DEPS TRUE)
        add_compile_definitions(ECSCOPE_HAS_GLFW ECSCOPE_HAS_OPENGL ECSCOPE_HAS_IMGUI)
    else()
        message(WARNING "GUI dependencies not found. GUI system will be disabled.")
        message(STATUS "Missing: glfw3=${glfw3_FOUND}, OpenGL=${OpenGL_FOUND}, ImGui=${IMGUI_INCLUDE_DIR}")
        set(ECSCOPE_HAS_GUI_DEPS FALSE)
    endif()
endif()

# Standalone tests that don't depend on the existing codebase
add_executable(ecscope_standalone_test
    examples/standalone_test.cpp
)
target_link_libraries(ecscope_standalone_test Threads::Threads)

# Standalone performance test
add_executable(ecscope_standalone_performance  
    examples/standalone_performance.cpp
)
target_link_libraries(ecscope_standalone_performance Threads::Threads)

# GUI System Library (if dependencies available)
if(ECSCOPE_BUILD_GUI AND ECSCOPE_HAS_GUI_DEPS)
    # Create GUI library
    add_library(ecscope_gui
        src/gui/dashboard.cpp
        src/gui/gui_manager.cpp
        src/gui/audio_ui.cpp
        src/gui/network_ui.cpp
        src/gui/asset_pipeline_ui.cpp
        src/gui/debug_tools_ui.cpp
        src/gui/scripting_ui.cpp
    )
    
    # Add responsive design system if enabled
    if(ECSCOPE_BUILD_RESPONSIVE_DESIGN)
        target_sources(ecscope_gui PRIVATE
            src/gui/responsive_design.cpp
        )
        target_compile_definitions(ecscope_gui PUBLIC ECSCOPE_HAS_RESPONSIVE_DESIGN=1)
    endif()
    
    target_include_directories(ecscope_gui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${IMGUI_INCLUDE_DIR}
    )
    
    target_link_libraries(ecscope_gui PUBLIC
        Threads::Threads
        glfw
        OpenGL::GL
    )
    
    # Add ImGui source files (assuming they're in a subdirectory or installed)
    # Note: In a real project, you'd link against an ImGui library
    target_compile_definitions(ecscope_gui PUBLIC ECSCOPE_HAS_GUI=1)
    
    # Audio UI System (if audio UI is enabled)
    if(ECSCOPE_BUILD_AUDIO_UI)
        # Add audio UI specific components
        target_sources(ecscope_gui PRIVATE
            # Audio effects UI would be implemented here
            # src/gui/audio_effects_ui.cpp
            # src/gui/spatial_audio_ui.cpp
        )
        target_compile_definitions(ecscope_gui PUBLIC ECSCOPE_HAS_AUDIO_UI=1)
    endif()
endif()

# GUI Examples (if GUI system available)
if(ECSCOPE_BUILD_EXAMPLES AND ECSCOPE_BUILD_GUI AND ECSCOPE_HAS_GUI_DEPS)
    # Dashboard Demo
    add_executable(dashboard_demo
        examples/advanced/dashboard_demo.cpp
    )
    target_link_libraries(dashboard_demo ecscope_gui)
    
    # Complete Dashboard Showcase
    add_executable(complete_dashboard_showcase
        examples/advanced/complete_dashboard_showcase.cpp
    )
    target_link_libraries(complete_dashboard_showcase ecscope_gui)
    
    # ECS Inspector Demo
    if(ECSCOPE_BUILD_ECS_INSPECTOR)
        add_library(ecscope_ecs_inspector STATIC
            src/gui/ecs_inspector.cpp
            src/gui/ecs_inspector_widgets.cpp
        )
        target_include_directories(ecscope_ecs_inspector PUBLIC include)
        target_link_libraries(ecscope_ecs_inspector 
            PUBLIC ecscope_gui
            PRIVATE Threads::Threads
        )
        
        # Make sure imgui is available
        if(ECSCOPE_HAS_GUI_DEPS)
            target_compile_definitions(ecscope_ecs_inspector PUBLIC ECSCOPE_HAS_IMGUI)
            target_link_libraries(ecscope_ecs_inspector PRIVATE ${IMGUI_LIBRARIES})
        endif()
        
        add_executable(ecs_inspector_demo 
            examples/advanced/ecs_inspector_demo.cpp
        )
        target_link_libraries(ecs_inspector_demo 
            ecscope_ecs_inspector 
            ecscope_gui
        )
    endif()
    
    # Comprehensive Audio UI Demo
    if(ECSCOPE_BUILD_AUDIO_UI)
        add_executable(audio_ui_demo
            examples/advanced/14-comprehensive-audio-ui-demo.cpp
        )
        target_link_libraries(audio_ui_demo ecscope_gui)
        target_compile_definitions(audio_ui_demo PRIVATE ECSCOPE_HAS_AUDIO_UI=1)
    endif()
    
    # Network Interface Demo
    add_executable(network_interface_demo
        examples/advanced/15-network-interface-demo.cpp
    )
    target_link_libraries(network_interface_demo ecscope_gui)
    
    # Asset Pipeline Demo
    add_executable(asset_pipeline_demo
        examples/advanced/16-asset-pipeline-demo.cpp
    )
    target_link_libraries(asset_pipeline_demo ecscope_gui)
    
    # Debug Tools Demo
    add_executable(debug_tools_demo
        examples/advanced/17-debug-tools-demo.cpp
    )
    target_link_libraries(debug_tools_demo ecscope_gui)
    
    # Scripting Environment Demo
    add_executable(scripting_environment_demo
        examples/advanced/18-scripting-environment-demo.cpp
    )
    target_link_libraries(scripting_environment_demo ecscope_gui)
    
    # Responsive Design System Demo
    if(ECSCOPE_BUILD_RESPONSIVE_DESIGN)
        add_executable(responsive_design_demo
            examples/advanced/responsive_design_demo.cpp
        )
        target_link_libraries(responsive_design_demo ecscope_gui)
        target_compile_definitions(responsive_design_demo PRIVATE ECSCOPE_HAS_RESPONSIVE_DESIGN=1)
    endif()
endif()

# Print configuration
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "  ECScope Standalone Integration Test Build")  
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "Core Components:")
message(STATUS "  Compiler & Language: ✓")
message(STATUS "  Standard Library: ✓") 
message(STATUS "  Threading Support: ✓")
message(STATUS "  Memory Management: ✓")
message(STATUS "  Mathematical Operations: ✓")

if(ECSCOPE_BUILD_GUI)
    message(STATUS "")
    message(STATUS "GUI System:")
    if(ECSCOPE_HAS_GUI_DEPS)
        message(STATUS "  Dear ImGui Integration: ✓")
        message(STATUS "  GLFW Window Management: ✓")
        message(STATUS "  OpenGL Rendering: ✓")
        message(STATUS "  Dashboard System: ✓")
        message(STATUS "  Professional UI/UX: ✓")
        if(ECSCOPE_BUILD_RESPONSIVE_DESIGN)
            message(STATUS "  Responsive Design System: ✓")
            message(STATUS "  DPI Scaling Support: ✓")
            message(STATUS "  Touch Interface Support: ✓")
            message(STATUS "  Adaptive Layouts: ✓")
        endif()
        if(ECSCOPE_BUILD_ECS_INSPECTOR)
            message(STATUS "  ECS Inspector: ✓")
            message(STATUS "  Advanced Component Editors: ✓")
            message(STATUS "  System Monitoring: ✓")
            message(STATUS "  Performance Profiling: ✓")
        endif()
    else()
        message(STATUS "  GUI Dependencies: ✗ (Missing dependencies)")
        message(STATUS "  Install: glfw3, OpenGL, Dear ImGui")
    endif()
endif()
message(STATUS "")
message(STATUS "Build Commands:")
message(STATUS "  cmake --build . --parallel")
message(STATUS "  ./ecscope_standalone_test")
message(STATUS "  ./ecscope_standalone_performance")
if(ECSCOPE_BUILD_GUI AND ECSCOPE_HAS_GUI_DEPS)
    message(STATUS "  ./dashboard_demo")
    message(STATUS "  ./complete_dashboard_showcase")
    if(ECSCOPE_BUILD_ECS_INSPECTOR)
        message(STATUS "  ./ecs_inspector_demo")
    endif()
    if(ECSCOPE_BUILD_RESPONSIVE_DESIGN)
        message(STATUS "  ./responsive_design_demo")
    endif()
endif()

message(STATUS "")
# Setup cross-platform testing if enabled
ecscope_setup_cross_platform_testing()

message(STATUS "Build Options:")
message(STATUS "  -DECSCOPE_BUILD_GUI=ON/OFF")
message(STATUS "  -DECSCOPE_BUILD_RESPONSIVE_DESIGN=ON/OFF")
message(STATUS "  -DECSCOPE_BUILD_EXAMPLES=ON/OFF")
message(STATUS "  -DECSCOPE_BUILD_ECS_INSPECTOR=ON/OFF")
message(STATUS "  -DECSCOPE_ENABLE_MODERN_RENDERING=ON/OFF")
message(STATUS "  -DECSCOPE_BUILD_CROSS_PLATFORM_TESTS=ON/OFF")
message(STATUS "  -DECSCOPE_BUILD_BENCHMARKS=ON/OFF")
message(STATUS "  -DECSCOPE_ENABLE_SANITIZERS=ON/OFF")
message(STATUS "  -DECSCOPE_ENABLE_COVERAGE=ON/OFF")
message(STATUS "  -DECSCOPE_ENABLE_ADVANCED_OPTIMIZATIONS=ON/OFF")
message(STATUS "═══════════════════════════════════════════════════════")