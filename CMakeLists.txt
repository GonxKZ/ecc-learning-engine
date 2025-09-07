cmake_minimum_required(VERSION 3.22)
project(ECScope 
    VERSION 1.0.0
    DESCRIPTION "ECScope: World-Class Educational ECS Engine - Advanced Systems Integration"
    HOMEPAGE_URL "https://github.com/ecscope/ecscope"
    LANGUAGES CXX)

# =============================================================================
# PROJECT CONFIGURATION
# =============================================================================

# C++20 configuration with advanced features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# =============================================================================
# ADVANCED BUILD OPTIONS
# =============================================================================

# Core Features
option(ECSCOPE_BUILD_TESTS "Build comprehensive test suite" ON)
option(ECSCOPE_BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(ECSCOPE_BUILD_EXAMPLES "Build educational examples" ON)
option(ECSCOPE_ENABLE_INSTRUMENTATION "Enable advanced tracing & memory hooks" ON)

# Advanced System Features  
option(ECSCOPE_ENABLE_GRAPHICS "Enable 2D graphics and rendering system (requires SDL2, OpenGL)" ON)
option(ECSCOPE_ENABLE_PHYSICS "Enable advanced 2D/3D physics system" ON)
option(ECSCOPE_ENABLE_SCRIPTING "Enable Python/Lua scripting integration" OFF)
option(ECSCOPE_ENABLE_JOB_SYSTEM "Enable work-stealing job system" ON)
option(ECSCOPE_ENABLE_MEMORY_ANALYSIS "Enable advanced memory analysis tools" ON)
option(ECSCOPE_ENABLE_PERFORMANCE_LAB "Enable performance laboratory" ON)
option(ECSCOPE_ENABLE_PLUGIN_SYSTEM "Enable comprehensive plugin system with hot-reload" ON)

# Platform & Performance Options
option(ECSCOPE_ENABLE_SIMD "Enable SIMD optimizations (AVX2/SSE)" ON)
option(ECSCOPE_ENABLE_LOCKFREE "Enable lock-free data structures" ON)
option(ECSCOPE_ENABLE_NUMA "Enable NUMA-aware allocators" OFF)
option(ECSCOPE_ENABLE_HARDWARE_DETECTION "Enable hardware capability detection" ON)

# Development Options
option(ECSCOPE_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ECSCOPE_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ECSCOPE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ECSCOPE_ENABLE_PROFILING "Enable detailed profiling support" OFF)

# =============================================================================
# COMPILER CONFIGURATION
# =============================================================================

# Advanced compiler flags based on build type and options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Base flags for all builds
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wconversion -Wshadow
        -Wno-unused-parameter
        -fno-omit-frame-pointer  # Better profiling
    )
    
    # Debug build optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-O0 -g3)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
        if(ECSCOPE_ENABLE_SIMD)
            add_compile_options(-march=native -mtune=native)
        endif()
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(-O2 -g)
    endif()
    
    # SIMD support
    if(ECSCOPE_ENABLE_SIMD)
        add_compile_options(-msse4.2 -mavx2)
        add_compile_definitions(ECSCOPE_ENABLE_SIMD=1)
    endif()
    
    # Sanitizers
    if(ECSCOPE_ENABLE_ASAN)
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
    
    if(ECSCOPE_ENABLE_TSAN)
        add_compile_options(-fsanitize=thread)
        add_link_options(-fsanitize=thread)
    endif()
    
    if(ECSCOPE_ENABLE_UBSAN)
        add_compile_options(-fsanitize=undefined)
        add_link_options(-fsanitize=undefined)
    endif()
    
    if(ECSCOPE_ENABLE_PROFILING)
        add_compile_options(-pg)
        add_link_options(-pg)
    endif()
endif()

# MSVC specific flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4 /permissive-)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND ECSCOPE_ENABLE_SIMD)
        add_compile_options(/arch:AVX2)
    endif()
endif()

# =============================================================================
# FEATURE CONFIGURATION MACROS  
# =============================================================================

# Configure feature macros
if(ECSCOPE_ENABLE_INSTRUMENTATION)
    add_compile_definitions(ECSCOPE_ENABLE_INSTRUMENTATION=1)
endif()

if(ECSCOPE_ENABLE_PHYSICS)
    add_compile_definitions(ECSCOPE_ENABLE_PHYSICS=1)
endif()

if(ECSCOPE_ENABLE_JOB_SYSTEM)
    add_compile_definitions(ECSCOPE_ENABLE_JOB_SYSTEM=1)
endif()

if(ECSCOPE_ENABLE_MEMORY_ANALYSIS)
    add_compile_definitions(ECSCOPE_ENABLE_MEMORY_ANALYSIS=1)
endif()

if(ECSCOPE_ENABLE_LOCKFREE)
    add_compile_definitions(ECSCOPE_ENABLE_LOCKFREE=1)
endif()

if(ECSCOPE_ENABLE_NUMA)
    add_compile_definitions(ECSCOPE_ENABLE_NUMA=1)
endif()

if(ECSCOPE_ENABLE_HARDWARE_DETECTION)
    add_compile_definitions(ECSCOPE_ENABLE_HARDWARE_DETECTION=1)
endif()

# =============================================================================
# DEPENDENCY MANAGEMENT
# =============================================================================

# Find required packages
find_package(Threads REQUIRED)

# Platform-specific libraries
if(WIN32)
    set(PLATFORM_LIBS)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LIBS dl)
else()
    set(PLATFORM_LIBS)
endif()

# =============================================================================
# ECSCOPE CORE LIBRARY - Modular Architecture
# =============================================================================

# Core ECScope library with minimal dependencies
add_library(ecscope_core STATIC)

target_sources(ecscope_core PRIVATE
    # Core System - Foundation
    src/core/log.cpp
    src/core/time.cpp
    src/core/id.cpp
    
    # ECS System - Entity Component System
    src/ecs/archetype.cpp
    src/ecs/query.cpp
    src/ecs/registry.cpp
    src/ecs/relationships.cpp
    src/ecs/system.cpp
    
    # Platform System - Hardware Detection
    $<$<BOOL:${ECSCOPE_ENABLE_HARDWARE_DETECTION}>:src/platform/hardware_detection.cpp>
    
    # Instrumentation System - Tracing & Hooks
    $<$<BOOL:${ECSCOPE_ENABLE_INSTRUMENTATION}>:src/instrumentation/hooks.cpp>
    $<$<BOOL:${ECSCOPE_ENABLE_INSTRUMENTATION}>:src/instrumentation/trace.cpp>
)

target_include_directories(ecscope_core
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
        $<INSTALL_INTERFACE:include/ecscope>
)

target_link_libraries(ecscope_core 
    PUBLIC 
        Threads::Threads
        ${PLATFORM_LIBS}
)

# =============================================================================
# MEMORY SYSTEM LIBRARY - Advanced Memory Management  
# =============================================================================

add_library(ecscope_memory STATIC)

target_sources(ecscope_memory PRIVATE
    # Basic Memory System
    src/memory/mem_tracker.cpp
    src/memory/memory_tracker_example.cpp
    
    # Allocators
    src/memory/allocators/arena.cpp
    src/memory/allocators/pool_allocator.cpp
    $<$<BOOL:${ECSCOPE_ENABLE_JOB_SYSTEM}>:src/memory/allocators/thread_local_allocator.cpp>
    
    # Memory Pools
    src/memory/pools/pool.cpp
    src/memory/pools/hierarchical_pools.cpp
    
    # Advanced Memory Analysis
    $<$<BOOL:${ECSCOPE_ENABLE_MEMORY_ANALYSIS}>:src/memory/analysis/bandwidth_analyzer.cpp>
    $<$<BOOL:${ECSCOPE_ENABLE_MEMORY_ANALYSIS}>:src/memory/analysis/cache_aware_structures.cpp>
    $<$<BOOL:${ECSCOPE_ENABLE_MEMORY_ANALYSIS}>:src/memory/analysis/memory_benchmark_suite.cpp>
    $<$<BOOL:${ECSCOPE_ENABLE_NUMA}>:src/memory/analysis/numa_manager.cpp>
)

target_include_directories(ecscope_memory
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/memory>
        $<INSTALL_INTERFACE:include/ecscope/memory>
)

target_link_libraries(ecscope_memory 
    PUBLIC 
        ecscope_core
    PRIVATE
        ${PLATFORM_LIBS}
)

# =============================================================================
# JOB SYSTEM LIBRARY - Work-Stealing Concurrency
# =============================================================================

if(ECSCOPE_ENABLE_JOB_SYSTEM)
    add_library(ecscope_jobs STATIC)
    
    target_sources(ecscope_jobs PRIVATE
        src/concurrency/job_system/work_stealing_job_system.cpp
        src/concurrency/job_system/ecs_job_integration.cpp
        src/concurrency/job_system/ecs_parallel_scheduler.cpp
        src/concurrency/job_system/job_profiler.cpp
    )
    
    target_include_directories(ecscope_jobs
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/concurrency>
            $<INSTALL_INTERFACE:include/ecscope/concurrency>
    )
    
    target_link_libraries(ecscope_jobs 
        PUBLIC 
            ecscope_core
            Threads::Threads
    )
endif()

# =============================================================================
# PHYSICS SYSTEM LIBRARY - 2D/3D Physics Engine
# =============================================================================

if(ECSCOPE_ENABLE_PHYSICS)
    add_library(ecscope_physics STATIC)
    
    target_sources(ecscope_physics PRIVATE
        # Core Math
        src/physics/math.cpp
        src/physics/math3d.cpp
        $<$<BOOL:${ECSCOPE_ENABLE_SIMD}>:src/physics/simd_math.cpp>
        $<$<BOOL:${ECSCOPE_ENABLE_SIMD}>:src/physics/simd_math3d.cpp>
        
        # 2D Physics
        src/physics/world.cpp
        src/physics/collision.cpp
        src/physics/broadphase.cpp
        src/physics/narrowphase.cpp
        src/physics/solver.cpp
        src/physics/components.cpp
        src/physics/debug_renderer_2d.cpp
        
        # 3D Physics  
        src/physics/world3d.cpp
        src/physics/collision3d.cpp
        src/physics/collision3d_algorithms.cpp
    )
    
    target_include_directories(ecscope_physics
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/physics>
            $<INSTALL_INTERFACE:include/ecscope/physics>
    )
    
    target_link_libraries(ecscope_physics 
        PUBLIC 
            ecscope_core
            $<$<BOOL:${ECSCOPE_ENABLE_JOB_SYSTEM}>:ecscope_jobs>
    )
endif()

# =============================================================================
# PERFORMANCE LABORATORY - Memory & Performance Analysis
# =============================================================================

if(ECSCOPE_ENABLE_PERFORMANCE_LAB)
    add_library(ecscope_performance STATIC)
    
    target_sources(ecscope_performance PRIVATE
        src/performance/performance_lab.cpp
        src/performance/memory_experiments.cpp
        src/performance/allocation_benchmarks.cpp
    )
    
    target_include_directories(ecscope_performance
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/performance>
            $<INSTALL_INTERFACE:include/ecscope/performance>
    )
    
    target_link_libraries(ecscope_performance 
        PUBLIC 
            ecscope_core
            ecscope_memory
    )
endif()

# =============================================================================
# GRAPHICS SYSTEM LIBRARY - 2D Rendering Engine
# =============================================================================

set(ECSCOPE_HAS_GRAPHICS OFF)
set(ECSCOPE_HAS_IMGUI OFF)

if(ECSCOPE_ENABLE_GRAPHICS)
    # Try to find SDL2
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        set(ECSCOPE_HAS_GRAPHICS ON)
        message(STATUS "SDL2 found via CMake: Enabling graphics support")
    else()
        # Try pkg-config approach
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(SDL2 QUIET sdl2)
            if(SDL2_FOUND)
                set(ECSCOPE_HAS_GRAPHICS ON)
                message(STATUS "SDL2 found via pkg-config: Enabling graphics support")
            endif()
        endif()
    endif()
    
    # Find OpenGL
    find_package(OpenGL QUIET)
    if(NOT OPENGL_FOUND)
        message(WARNING "OpenGL not found - graphics functionality will be limited")
    endif()
    
    # Try to find ImGui (typically as source in external/)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.h")
        set(ECSCOPE_HAS_IMGUI ON)
        message(STATUS "ImGui found: Adding to build")
    else()
        message(STATUS "ImGui not found in external/imgui/ - UI panels will be disabled")
    endif()
    
    if(NOT ECSCOPE_HAS_GRAPHICS)
        message(WARNING "Graphics requested but SDL2 not found. Building console-only version.")
    endif()
endif()

# Graphics library
if(ECSCOPE_HAS_GRAPHICS)
    add_library(ecscope_graphics STATIC)
    
    target_sources(ecscope_graphics PRIVATE
        # Core Graphics
        src/graphics/renderer/window.cpp
        src/graphics/renderer/gl_context.cpp
        
        # 2D Rendering System
        src/graphics/renderer/renderer_2d.cpp
        src/graphics/renderer/batch_renderer.cpp
        src/graphics/renderer/camera2d.cpp
        src/graphics/renderer/draw2d.cpp
        
        # Resources
        src/graphics/renderer/resources/shader.cpp
        src/graphics/renderer/resources/texture.cpp
    )
    
    target_include_directories(ecscope_graphics
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/graphics>
            $<INSTALL_INTERFACE:include/ecscope/graphics>
    )
    
    # Link graphics dependencies
    if(SDL2_FOUND AND TARGET SDL2::SDL2)
        target_link_libraries(ecscope_graphics PUBLIC SDL2::SDL2)
    elseif(SDL2_FOUND)
        target_link_libraries(ecscope_graphics PUBLIC ${SDL2_LIBRARIES})
        target_include_directories(ecscope_graphics PUBLIC ${SDL2_INCLUDE_DIRS})
        target_compile_options(ecscope_graphics PUBLIC ${SDL2_CFLAGS_OTHER})
    endif()
    
    if(OPENGL_FOUND)
        target_link_libraries(ecscope_graphics PUBLIC ${OPENGL_LIBRARIES})
        target_include_directories(ecscope_graphics PUBLIC ${OPENGL_INCLUDE_DIRS})
    endif()
    
    target_link_libraries(ecscope_graphics 
        PUBLIC 
            ecscope_core
            $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:ecscope_physics>
    )
    
    target_compile_definitions(ecscope_graphics PUBLIC ECSCOPE_HAS_GRAPHICS=1)
endif()

# =============================================================================
# UI SYSTEM LIBRARY - ImGui-based User Interface
# =============================================================================

if(ECSCOPE_HAS_GRAPHICS AND ECSCOPE_HAS_IMGUI)
    add_library(ecscope_ui STATIC)
    
    # ImGui source files
    target_sources(ecscope_ui PRIVATE
        external/imgui/imgui.cpp
        external/imgui/imgui_demo.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        
        # ECScope UI System
        src/ui/overlay.cpp
        
        # UI Panels
        src/ui/panels/panel_ecs_inspector.cpp
        src/ui/panels/panel_memory.cpp
        src/ui/panels/panel_stats.cpp
        src/ui/panels/panel_archetypes.cpp
        src/ui/panels/panel_cache_visualizer.cpp
        src/ui/panels/panel_profiler.cpp
        
        # Conditional UI Panels
        $<$<BOOL:${ECSCOPE_ENABLE_PERFORMANCE_LAB}>:src/ui/panels/panel_performance_lab.cpp>
        $<$<BOOL:${ECSCOPE_ENABLE_GRAPHICS}>:src/ui/panels/panel_rendering_debug.cpp>
        $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:src/ui/panels/panel_physics_debug.cpp>
    )
    
    target_include_directories(ecscope_ui
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/imgui>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/ui>
            $<INSTALL_INTERFACE:include/ecscope/ui>
    )
    
    target_link_libraries(ecscope_ui 
        PUBLIC 
            ecscope_graphics
            ecscope_memory
            $<$<BOOL:${ECSCOPE_ENABLE_PERFORMANCE_LAB}>:ecscope_performance>
            $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:ecscope_physics>
            $<$<BOOL:${ECSCOPE_ENABLE_JOB_SYSTEM}>:ecscope_jobs>
    )
    
    target_compile_definitions(ecscope_ui PUBLIC ECSCOPE_HAS_IMGUI=1)
endif()

# =============================================================================
# SCRIPTING SYSTEM LIBRARY - Python/Lua Integration
# =============================================================================

if(ECSCOPE_ENABLE_SCRIPTING)
    add_library(ecscope_scripting STATIC)
    
    target_sources(ecscope_scripting PRIVATE
        src/scripting/script_engine.cpp
        src/scripting/auto_bindings.cpp
    )
    
    # Try to find Python
    find_package(Python3 COMPONENTS Interpreter Development QUIET)
    if(Python3_FOUND)
        target_sources(ecscope_scripting PRIVATE src/scripting/python_engine.cpp)
        target_link_libraries(ecscope_scripting PUBLIC Python3::Python)
        target_compile_definitions(ecscope_scripting PUBLIC ECSCOPE_HAS_PYTHON=1)
        message(STATUS "Python3 found: Enabling Python scripting support")
    else()
        message(STATUS "Python3 not found - Python scripting disabled")
    endif()
    
    # Try to find Lua
    find_package(Lua QUIET)
    if(LUA_FOUND)
        target_sources(ecscope_scripting PRIVATE src/scripting/lua_engine.cpp)
        target_link_libraries(ecscope_scripting PUBLIC ${LUA_LIBRARIES})
        target_include_directories(ecscope_scripting PUBLIC ${LUA_INCLUDE_DIR})
        target_compile_definitions(ecscope_scripting PUBLIC ECSCOPE_HAS_LUA=1)
        message(STATUS "Lua found: Enabling Lua scripting support")
    else()
        message(STATUS "Lua not found - Lua scripting disabled")
    endif()
    
    target_include_directories(ecscope_scripting
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/scripting>
            $<INSTALL_INTERFACE:include/ecscope/scripting>
    )
    
    target_link_libraries(ecscope_scripting 
        PUBLIC 
            ecscope_core
            $<$<BOOL:${ECSCOPE_ENABLE_JOB_SYSTEM}>:ecscope_jobs>
    )
endif()

# =============================================================================
# UNIFIED ECSCOPE LIBRARY - Complete Integration
# =============================================================================

# Main ECScope library that combines all components
add_library(ecscope INTERFACE)

target_link_libraries(ecscope 
    INTERFACE
        ecscope_core
        ecscope_memory
        $<$<BOOL:${ECSCOPE_ENABLE_JOB_SYSTEM}>:ecscope_jobs>
        $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:ecscope_physics>
        $<$<BOOL:${ECSCOPE_ENABLE_PERFORMANCE_LAB}>:ecscope_performance>
        $<$<BOOL:${ECSCOPE_HAS_GRAPHICS}>:ecscope_graphics>
        $<$<AND:$<BOOL:${ECSCOPE_HAS_GRAPHICS}>,$<BOOL:${ECSCOPE_HAS_IMGUI}>>:ecscope_ui>
        $<$<BOOL:${ECSCOPE_ENABLE_SCRIPTING}>:ecscope_scripting>
)

# Convenience alias
add_library(ECScope::ECScope ALIAS ecscope)

# =============================================================================
# APPLICATIONS - Main ECScope Programs
# =============================================================================

# Console Application - Minimal ECScope
add_executable(ecscope_console)
target_sources(ecscope_console PRIVATE
    src/app/main.cpp
)
target_link_libraries(ecscope_console PRIVATE ecscope_core ecscope_memory)

# Minimal Application - Ultra-lightweight  
add_executable(ecscope_minimal)
target_sources(ecscope_minimal PRIVATE
    src/app/main_minimal.cpp
)
target_link_libraries(ecscope_minimal PRIVATE ecscope_core)

# Full Application - Complete ECScope with UI
if(ECSCOPE_HAS_GRAPHICS AND ECSCOPE_HAS_IMGUI)
    add_executable(ecscope_ui)
    target_sources(ecscope_ui PRIVATE
        src/app/main_ui.cpp
    )
    target_link_libraries(ecscope_ui PRIVATE ecscope)
    
    # Main application
    add_executable(ecscope_app)
    target_sources(ecscope_app PRIVATE
        src/app/main_ui.cpp
    )
    target_link_libraries(ecscope_app PRIVATE ECScope::ECScope)
    
    message(STATUS "Building full UI application (ecscope_app)")
else()
    # Fallback to console when graphics not available
    add_executable(ecscope_app)
    target_sources(ecscope_app PRIVATE
        src/app/main.cpp
    )
    target_link_libraries(ecscope_app PRIVATE ECScope::ECScope)
    
    message(STATUS "Building console application (ecscope_app)")
endif()

# =============================================================================
# COMPREHENSIVE EXAMPLES - Educational Showcase
# =============================================================================

if(ECSCOPE_BUILD_EXAMPLES)
    
    # =============================================================================
    # BEGINNER EXAMPLES - Learning Fundamentals
    # =============================================================================
    
    # Beginner Example 1: Memory Basics
    add_executable(example_memory_basics)
    target_sources(example_memory_basics PRIVATE examples/beginner/01-memory-basics.cpp)
    target_link_libraries(example_memory_basics PRIVATE ecscope_core ecscope_memory)
    
    # Beginner Example 2: Basic Physics
    if(ECSCOPE_ENABLE_PHYSICS)
        add_executable(example_basic_physics)
        target_sources(example_basic_physics PRIVATE examples/beginner/02-basic-physics.cpp)
        target_link_libraries(example_basic_physics PRIVATE ecscope_core ecscope_physics)
    endif()
    
    # Beginner Example 3: Basic Rendering
    if(ECSCOPE_HAS_GRAPHICS)
        add_executable(example_basic_rendering)
        target_sources(example_basic_rendering PRIVATE examples/beginner/03-basic-rendering.cpp)
        target_link_libraries(example_basic_rendering PRIVATE ecscope)
    endif()
    
    # =============================================================================
    # INTERMEDIATE EXAMPLES - Advanced Integration
    # =============================================================================
    
    # Intermediate Example 1: Physics Integration
    if(ECSCOPE_ENABLE_PHYSICS AND ECSCOPE_HAS_GRAPHICS)
        add_executable(example_physics_integration)
        target_sources(example_physics_integration PRIVATE examples/intermediate/04-physics-integration.cpp)
        target_link_libraries(example_physics_integration PRIVATE ecscope)
    endif()
    
    # Intermediate Example 2: Performance Analysis
    if(ECSCOPE_ENABLE_PERFORMANCE_LAB)
        add_executable(example_performance_analysis)
        target_sources(example_performance_analysis PRIVATE examples/intermediate/05-performance-analysis.cpp)
        target_link_libraries(example_performance_analysis PRIVATE ecscope)
    endif()
    
    # Intermediate Example 3: Rendering Benchmarks
    if(ECSCOPE_HAS_GRAPHICS)
        add_executable(example_rendering_benchmarks)
        target_sources(example_rendering_benchmarks PRIVATE examples/intermediate/06-rendering-benchmarks.cpp)
        target_link_libraries(example_rendering_benchmarks PRIVATE ecscope)
    endif()
    
    # Job System Examples
    if(ECSCOPE_ENABLE_JOB_SYSTEM)
        add_executable(example_job_system_basics)
        target_sources(example_job_system_basics PRIVATE examples/intermediate/job-system-basics.cpp)
        target_link_libraries(example_job_system_basics PRIVATE ecscope)
        
        add_executable(example_physics_rendering_integration)
        target_sources(example_physics_rendering_integration PRIVATE examples/intermediate/physics-rendering-integration.cpp)
        target_link_libraries(example_physics_rendering_integration PRIVATE ecscope)
    endif()
    
    # =============================================================================
    # ADVANCED EXAMPLES - Expert-Level Demonstrations
    # =============================================================================
    
    # Advanced Example 1: Custom Allocators
    add_executable(example_custom_allocators)
    target_sources(example_custom_allocators PRIVATE examples/advanced/07-custom-allocators.cpp)
    target_link_libraries(example_custom_allocators PRIVATE ecscope)
    
    # Advanced Example 2: Job System Integration
    if(ECSCOPE_ENABLE_JOB_SYSTEM)
        add_executable(example_job_system_integration)
        target_sources(example_job_system_integration PRIVATE examples/advanced/08-job-system-integration.cpp)
        target_link_libraries(example_job_system_integration PRIVATE ecscope)
    endif()
    
    # Advanced Example 3: Performance Laboratory
    if(ECSCOPE_ENABLE_PERFORMANCE_LAB)
        add_executable(example_performance_laboratory)
        target_sources(example_performance_laboratory PRIVATE examples/advanced/09-performance-laboratory.cpp)
        target_link_libraries(example_performance_laboratory PRIVATE ecscope)
    endif()
    
    # Advanced Physics Benchmarks
    if(ECSCOPE_ENABLE_PHYSICS)
        add_executable(example_physics_benchmarks)
        target_sources(example_physics_benchmarks PRIVATE examples/advanced/physics-benchmarks.cpp)
        target_link_libraries(example_physics_benchmarks PRIVATE ecscope)
    endif()
    
    # =============================================================================
    # REFERENCE EXAMPLES - Production-Quality Showcases
    # =============================================================================
    
    # Physics Math Reference
    if(ECSCOPE_ENABLE_PHYSICS)
        add_executable(example_physics_math)
        target_sources(example_physics_math PRIVATE examples/reference/physics-math.cpp)
        target_link_libraries(example_physics_math PRIVATE ecscope)
        
        add_executable(example_3d_physics_integration)
        target_sources(example_3d_physics_integration PRIVATE examples/reference/3d-physics-integration.cpp)
        target_link_libraries(example_3d_physics_integration PRIVATE ecscope)
    endif()
    
    # Hardware Analysis
    if(ECSCOPE_ENABLE_HARDWARE_DETECTION)
        add_executable(example_hardware_analysis)
        target_sources(example_hardware_analysis PRIVATE examples/reference/hardware-analysis.cpp)
        target_link_libraries(example_hardware_analysis PRIVATE ecscope)
    endif()
    
    # Optimization Showcase
    add_executable(example_optimization_showcase)
    target_sources(example_optimization_showcase PRIVATE examples/reference/optimization-showcase.cpp)
    target_link_libraries(example_optimization_showcase PRIVATE ecscope)
    
    # Scripting Showcase
    if(ECSCOPE_ENABLE_SCRIPTING)
        add_executable(example_scripting_showcase)
        target_sources(example_scripting_showcase PRIVATE examples/reference/scripting-showcase.cpp)
        target_link_libraries(example_scripting_showcase PRIVATE ecscope)
    endif()
    
    # =============================================================================
    # RENDERING TUTORIALS - Step-by-Step Learning
    # =============================================================================
    
    if(ECSCOPE_HAS_GRAPHICS)
        # Tutorial 1: Basic Sprite Rendering
        add_executable(tutorial_01_basic_sprites)
        target_sources(tutorial_01_basic_sprites PRIVATE examples/reference/rendering-tutorials/01_basic_sprite_rendering.cpp)
        target_link_libraries(tutorial_01_basic_sprites PRIVATE ecscope)
        
        # Tutorial 2: Sprite Batching
        add_executable(tutorial_02_sprite_batching)
        target_sources(tutorial_02_sprite_batching PRIVATE examples/reference/rendering-tutorials/02_sprite_batching_fundamentals.cpp)
        target_link_libraries(tutorial_02_sprite_batching PRIVATE ecscope)
        
        # Tutorial 3: Camera Systems
        add_executable(tutorial_03_camera_systems)
        target_sources(tutorial_03_camera_systems PRIVATE examples/reference/rendering-tutorials/03_camera_systems_and_viewports.cpp)
        target_link_libraries(tutorial_03_camera_systems PRIVATE ecscope)
        
        # Tutorial 4: Shader Programming
        add_executable(tutorial_04_shader_programming)
        target_sources(tutorial_04_shader_programming PRIVATE examples/reference/rendering-tutorials/04_shader_programming_and_materials.cpp)
        target_link_libraries(tutorial_04_shader_programming PRIVATE ecscope)
        
        # Tutorial 5: Texture Atlasing
        add_executable(tutorial_05_texture_atlasing)
        target_sources(tutorial_05_texture_atlasing PRIVATE examples/reference/rendering-tutorials/05_texture_atlasing_and_optimization.cpp)
        target_link_libraries(tutorial_05_texture_atlasing PRIVATE ecscope)
        
        # Tutorial 6: Particle Systems
        add_executable(tutorial_06_particle_systems)
        target_sources(tutorial_06_particle_systems PRIVATE examples/reference/rendering-tutorials/06_particle_systems_and_effects.cpp)
        target_link_libraries(tutorial_06_particle_systems PRIVATE ecscope)
        
        # Tutorial 7: Multi-layer Rendering
        add_executable(tutorial_07_multilayer_rendering)
        target_sources(tutorial_07_multilayer_rendering PRIVATE examples/reference/rendering-tutorials/07_multi_layer_rendering_and_depth.cpp)
        target_link_libraries(tutorial_07_multilayer_rendering PRIVATE ecscope)
    endif()
    
    # =============================================================================
    # PHYSICS DEBUG EXAMPLES
    # =============================================================================
    
    if(ECSCOPE_ENABLE_PHYSICS AND ECSCOPE_HAS_GRAPHICS)
        add_executable(example_physics_debug_integration_tests)
        target_sources(example_physics_debug_integration_tests PRIVATE examples/reference/physics-debug/physics_debug_integration_tests.cpp)
        target_link_libraries(example_physics_debug_integration_tests PRIVATE ecscope)
        
        add_executable(example_physics_debug_performance_benchmark)
        target_sources(example_physics_debug_performance_benchmark PRIVATE examples/reference/physics-debug/physics_debug_performance_benchmark.cpp)
        target_link_libraries(example_physics_debug_performance_benchmark PRIVATE ecscope)
        
        add_executable(example_physics_debug_rendering_demo)
        target_sources(example_physics_debug_rendering_demo PRIVATE examples/reference/physics-debug/physics_debug_rendering_demo.cpp)
        target_link_libraries(example_physics_debug_rendering_demo PRIVATE ecscope)
    endif()
    
endif() # ECSCOPE_BUILD_EXAMPLES

# =============================================================================
# BENCHMARKS - Performance Measurement
# =============================================================================

if(ECSCOPE_BUILD_BENCHMARKS)
    
    # Core Memory Benchmarks
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/memory_core_benchmarks.cpp")
        add_executable(benchmark_memory_core)
        target_sources(benchmark_memory_core PRIVATE benchmarks/memory_core_benchmarks.cpp)
        target_link_libraries(benchmark_memory_core PRIVATE ecscope)
    endif()
    
    # ECS Performance Benchmarks
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/ecs_performance_benchmarks.cpp")
        add_executable(benchmark_ecs_performance)
        target_sources(benchmark_ecs_performance PRIVATE benchmarks/ecs_performance_benchmarks.cpp)
        target_link_libraries(benchmark_ecs_performance PRIVATE ecscope)
    endif()
    
    # Physics Benchmarks
    if(ECSCOPE_ENABLE_PHYSICS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/physics_benchmarks.cpp")
        add_executable(benchmark_physics)
        target_sources(benchmark_physics PRIVATE benchmarks/physics_benchmarks.cpp)
        target_link_libraries(benchmark_physics PRIVATE ecscope)
    endif()
    
    # Job System Benchmarks
    if(ECSCOPE_ENABLE_JOB_SYSTEM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/job_system_benchmarks.cpp")
        add_executable(benchmark_job_system)
        target_sources(benchmark_job_system PRIVATE benchmarks/job_system_benchmarks.cpp)
        target_link_libraries(benchmark_job_system PRIVATE ecscope)
    endif()
    
    # Rendering Benchmarks
    if(ECSCOPE_HAS_GRAPHICS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/rendering_benchmarks.cpp")
        add_executable(benchmark_rendering)
        target_sources(benchmark_rendering PRIVATE benchmarks/rendering_benchmarks.cpp)
        target_link_libraries(benchmark_rendering PRIVATE ecscope)
    endif()
    
endif() # ECSCOPE_BUILD_BENCHMARKS

# =============================================================================
# TESTING FRAMEWORK
# =============================================================================

if(ECSCOPE_BUILD_TESTS)
    enable_testing()
    
    # Try to find Google Test
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "Google Test found: Building comprehensive test suite")
        
        # Core Tests
        add_executable(test_core)
        target_sources(test_core PRIVATE
            tests/core/test_log.cpp
            tests/core/test_time.cpp
            tests/core/test_id.cpp
        )
        target_link_libraries(test_core PRIVATE ecscope_core GTest::gtest_main)
        add_test(NAME CoreTests COMMAND test_core)
        
        # Memory Tests
        add_executable(test_memory)
        target_sources(test_memory PRIVATE
            tests/memory/test_arena.cpp
            tests/memory/test_pool_allocator.cpp
            tests/memory/test_memory_tracker.cpp
        )
        target_link_libraries(test_memory PRIVATE ecscope_memory GTest::gtest_main)
        add_test(NAME MemoryTests COMMAND test_memory)
        
        # ECS Tests
        add_executable(test_ecs)
        target_sources(test_ecs PRIVATE
            tests/ecs/test_registry.cpp
            tests/ecs/test_archetype.cpp
            tests/ecs/test_query.cpp
        )
        target_link_libraries(test_ecs PRIVATE ecscope_core GTest::gtest_main)
        add_test(NAME ECSTests COMMAND test_ecs)
        
        # Physics Tests
        if(ECSCOPE_ENABLE_PHYSICS)
            add_executable(test_physics)
            target_sources(test_physics PRIVATE
                tests/physics/test_math.cpp
                tests/physics/test_collision.cpp
                tests/physics/test_world.cpp
            )
            target_link_libraries(test_physics PRIVATE ecscope_physics GTest::gtest_main)
            add_test(NAME PhysicsTests COMMAND test_physics)
        endif()
        
        # Job System Tests
        if(ECSCOPE_ENABLE_JOB_SYSTEM)
            add_executable(test_jobs)
            target_sources(test_jobs PRIVATE
                tests/concurrency/test_job_system.cpp
                tests/concurrency/test_work_stealing.cpp
            )
            target_link_libraries(test_jobs PRIVATE ecscope_jobs GTest::gtest_main)
            add_test(NAME JobTests COMMAND test_jobs)
        endif()
        
    else()
        message(STATUS "Google Test not found - building basic tests")
        
        # Simple test without GTest framework (only if file exists)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/simple_tests.cpp")
            add_executable(simple_tests)
            target_sources(simple_tests PRIVATE tests/simple_tests.cpp)
            target_link_libraries(simple_tests PRIVATE ecscope)
        endif()
    endif()
endif()

# =============================================================================
# INSTALLATION & PACKAGING
# =============================================================================

# Installation directories
set(CMAKE_INSTALL_INCLUDEDIR include)
set(CMAKE_INSTALL_LIBDIR lib)
set(CMAKE_INSTALL_BINDIR bin)

# Install libraries
install(TARGETS ecscope_core ecscope_memory
    EXPORT ECScope-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Optional component installations
if(ECSCOPE_ENABLE_JOB_SYSTEM)
    install(TARGETS ecscope_jobs EXPORT ECScope-targets)
endif()

if(ECSCOPE_ENABLE_PHYSICS)
    install(TARGETS ecscope_physics EXPORT ECScope-targets)
endif()

if(ECSCOPE_ENABLE_PERFORMANCE_LAB)
    install(TARGETS ecscope_performance EXPORT ECScope-targets)
endif()

if(ECSCOPE_HAS_GRAPHICS)
    install(TARGETS ecscope_graphics EXPORT ECScope-targets)
endif()

if(ECSCOPE_HAS_GRAPHICS AND ECSCOPE_HAS_IMGUI)
    install(TARGETS ecscope_ui EXPORT ECScope-targets)
endif()

if(ECSCOPE_ENABLE_SCRIPTING)
    install(TARGETS ecscope_scripting EXPORT ECScope-targets)
endif()

# Install main library
install(TARGETS ecscope EXPORT ECScope-targets)

# Install headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ecscope 
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

# Install main executable
install(TARGETS ecscope_app DESTINATION ${CMAKE_INSTALL_BINDIR})

# Export targets for find_package
install(EXPORT ECScope-targets
    FILE ECScope-targets.cmake
    NAMESPACE ECScope::
    DESTINATION lib/cmake/ECScope
)

# Create config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ECScope-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ECScope-config.cmake"
    INSTALL_DESTINATION lib/cmake/ECScope
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ECScope-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ECScope-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ECScope-config-version.cmake"
    DESTINATION lib/cmake/ECScope
)

# =============================================================================
# CONFIGURATION SUMMARY & BUILD INFORMATION
# =============================================================================

message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════════════")
message(STATUS "  ECScope ${PROJECT_VERSION} - Educational ECS Engine Configuration")
message(STATUS "═══════════════════════════════════════════════════════════════")
message(STATUS "Build Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "")

message(STATUS "Core Features:")
message(STATUS "  ECS System: ✓ (Always enabled)")
message(STATUS "  Memory Analysis: ${ECSCOPE_ENABLE_MEMORY_ANALYSIS}")
message(STATUS "  Instrumentation: ${ECSCOPE_ENABLE_INSTRUMENTATION}")
message(STATUS "  Hardware Detection: ${ECSCOPE_ENABLE_HARDWARE_DETECTION}")
message(STATUS "")

message(STATUS "Advanced Systems:")
message(STATUS "  Job System: ${ECSCOPE_ENABLE_JOB_SYSTEM}")
message(STATUS "  Physics Engine: ${ECSCOPE_ENABLE_PHYSICS}")
message(STATUS "  Graphics System: ${ECSCOPE_HAS_GRAPHICS}")
message(STATUS "  UI System: ${ECSCOPE_HAS_IMGUI}")
message(STATUS "  Scripting Support: ${ECSCOPE_ENABLE_SCRIPTING}")
message(STATUS "  Performance Lab: ${ECSCOPE_ENABLE_PERFORMANCE_LAB}")
message(STATUS "")

message(STATUS "Performance Features:")
message(STATUS "  SIMD Optimizations: ${ECSCOPE_ENABLE_SIMD}")
message(STATUS "  Lock-free Structures: ${ECSCOPE_ENABLE_LOCKFREE}")
message(STATUS "  NUMA Support: ${ECSCOPE_ENABLE_NUMA}")
message(STATUS "")

message(STATUS "Development Tools:")
message(STATUS "  Unit Tests: ${ECSCOPE_BUILD_TESTS}")
message(STATUS "  Benchmarks: ${ECSCOPE_BUILD_BENCHMARKS}")
message(STATUS "  Examples: ${ECSCOPE_BUILD_EXAMPLES}")
message(STATUS "  Address Sanitizer: ${ECSCOPE_ENABLE_ASAN}")
message(STATUS "  Thread Sanitizer: ${ECSCOPE_ENABLE_TSAN}")
message(STATUS "")

# Libraries information
set(AVAILABLE_LIBS "ecscope_core ecscope_memory")
if(ECSCOPE_ENABLE_JOB_SYSTEM)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_jobs")
endif()
if(ECSCOPE_ENABLE_PHYSICS)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_physics")
endif()
if(ECSCOPE_ENABLE_PERFORMANCE_LAB)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_performance")
endif()
if(ECSCOPE_HAS_GRAPHICS)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_graphics")
endif()
if(ECSCOPE_HAS_GRAPHICS AND ECSCOPE_HAS_IMGUI)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_ui")
endif()
if(ECSCOPE_ENABLE_SCRIPTING)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_scripting")
endif()

# =============================================================================
# PLUGIN SYSTEM LIBRARY - Complete Plugin Framework
# =============================================================================

if(ECSCOPE_ENABLE_PLUGIN_SYSTEM)
    add_library(ecscope_plugin STATIC)
    
    target_sources(ecscope_plugin PRIVATE
        # Core plugin system
        src/plugin/plugin_core.cpp
        src/plugin/plugin_manager.cpp
        src/plugin/plugin_registry.cpp
        src/plugin/plugin_security.cpp
        src/plugin/plugin_api.cpp
        
        # Dynamic loading system
        src/plugin/dynamic_loader.cpp
        
        # ECS integration
        src/plugin/ecs_plugin_integration.cpp
        
        # Development tools
        src/plugin/plugin_sdk.cpp
        src/plugin/plugin_testing.cpp
        
        # Platform-specific implementations
        $<$<PLATFORM_ID:Windows>:src/plugin/platform/windows_loader.cpp>
        $<$<PLATFORM_ID:Linux>:src/plugin/platform/linux_loader.cpp>
        $<$<PLATFORM_ID:Darwin>:src/plugin/platform/macos_loader.cpp>
    )
    
    target_include_directories(ecscope_plugin
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ecscope/plugin>
            $<INSTALL_INTERFACE:include/ecscope/plugin>
    )
    
    # Platform-specific linking
    if(WIN32)
        target_link_libraries(ecscope_plugin PRIVATE kernel32 psapi)
    elseif(UNIX)
        target_link_libraries(ecscope_plugin PRIVATE ${CMAKE_DL_LIBS} pthread)
    endif()
    
    target_link_libraries(ecscope_plugin 
        PUBLIC 
            ecscope_core
            ecscope_memory
            $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:ecscope_physics>
        PRIVATE
            $<$<BOOL:${ECSCOPE_HAS_GRAPHICS}>:ecscope_graphics>
    )
    
    target_compile_definitions(ecscope_plugin 
        PUBLIC 
            ECSCOPE_HAS_PLUGIN_SYSTEM=1
            PLUGIN_API_VERSION=1
        PRIVATE
            $<$<CONFIG:Debug>:ECSCOPE_PLUGIN_DEBUG=1>
    )
    
    # Add to available libraries
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_plugin")
    
    # Plugin system examples and demos
    if(ECSCOPE_BUILD_EXAMPLES)
        # Plugin system demonstration
        add_executable(plugin_system_demo examples/plugin_system_demo.cpp)
        target_link_libraries(plugin_system_demo 
            PRIVATE 
                ecscope_plugin 
                ecscope_core
                $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:ecscope_physics>
        )
        
        # Example plugins (built as shared libraries)
        add_library(basic_component_plugin SHARED examples/plugins/basic_component_plugin.cpp)
        target_link_libraries(basic_component_plugin PRIVATE ecscope_plugin)
        set_target_properties(basic_component_plugin PROPERTIES
            PREFIX ""
            SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        )
        
        add_library(advanced_system_plugin SHARED examples/plugins/advanced_system_plugin.cpp)
        target_link_libraries(advanced_system_plugin PRIVATE ecscope_plugin)
        set_target_properties(advanced_system_plugin PROPERTIES
            PREFIX ""
            SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        )
        
        # Copy plugin metadata files
        configure_file(
            "${CMAKE_SOURCE_DIR}/examples/plugins/basic_component_plugin.json"
            "${CMAKE_BINARY_DIR}/plugins/basic_component_plugin.json"
            COPYONLY
        )
        configure_file(
            "${CMAKE_SOURCE_DIR}/examples/plugins/advanced_system_plugin.json"
            "${CMAKE_BINARY_DIR}/plugins/advanced_system_plugin.json"
            COPYONLY
        )
    endif()
endif()

if(ECSCOPE_ENABLE_SCRIPTING)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_scripting")
endif()

message(STATUS "Available Libraries:")
message(STATUS "  ${AVAILABLE_LIBS}")
message(STATUS "  ecscope (unified library)")
message(STATUS "")

# Applications information
if(ECSCOPE_HAS_GRAPHICS AND ECSCOPE_HAS_IMGUI)
    message(STATUS "Applications:")
    message(STATUS "  ecscope_app - Full-featured UI application")
    message(STATUS "  ecscope_console - Console-based application")
    message(STATUS "  ecscope_minimal - Ultra-lightweight application")
else()
    message(STATUS "Applications:")
    message(STATUS "  ecscope_app - Console-based application")
    message(STATUS "  ecscope_console - Console-based application")
    message(STATUS "  ecscope_minimal - Ultra-lightweight application")
endif()
message(STATUS "")

# Build suggestions
if(NOT ECSCOPE_HAS_GRAPHICS)
    message(STATUS "💡 Suggestions:")
    message(STATUS "   Install SDL2 and enable graphics: -DECSCOPE_ENABLE_GRAPHICS=ON")
    message(STATUS "   For full UI experience, also install ImGui in external/imgui/")
endif()

if(NOT ECSCOPE_ENABLE_SCRIPTING)
    message(STATUS "   Enable scripting support: -DECSCOPE_ENABLE_SCRIPTING=ON")
    message(STATUS "   Requires Python3 and/or Lua development libraries")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "   For performance testing, use Release build: -DCMAKE_BUILD_TYPE=Release")
endif()

message(STATUS "")
message(STATUS "Build Commands:")
message(STATUS "  cmake --build . --parallel")
message(STATUS "  cmake --build . --target ecscope_app")
if(ECSCOPE_BUILD_TESTS)
    message(STATUS "  cmake --build . --target test")
endif()
message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════════════")