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
option(ECSCOPE_ENABLE_MODERN_RENDERING "Enable professional 3D rendering engine (Vulkan/OpenGL, Deferred, PBR)" ON)
option(ECSCOPE_ENABLE_PHYSICS "Enable advanced 2D/3D physics system" ON)
option(ECSCOPE_ENABLE_NETWORKING "Enable complete networking system with ECS replication" ON)
option(ECSCOPE_ENABLE_SCRIPTING "Enable Python/Lua scripting integration" ON)
option(ECSCOPE_ENABLE_JOB_SYSTEM "Enable work-stealing job system" ON)
option(ECSCOPE_ENABLE_MEMORY_ANALYSIS "Enable advanced memory analysis tools" ON)
option(ECSCOPE_ENABLE_PERFORMANCE_LAB "Enable performance laboratory" ON)
option(ECSCOPE_ENABLE_PLUGIN_SYSTEM "Enable comprehensive plugin system with hot-reload" ON)
option(ECSCOPE_ENABLE_AUDIO "Enable comprehensive 3D spatial audio system" ON)
option(ECSCOPE_ENABLE_AI_ML "Enable comprehensive AI/ML framework with behavior systems, neural networks, and PCG" ON)

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
option(ECSCOPE_ENABLE_DEBUG_SYSTEM "Enable comprehensive debugging and profiling system" ON)

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

if(ECSCOPE_ENABLE_NETWORKING)
    add_compile_definitions(ECSCOPE_ENABLE_NETWORKING=1)
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

if(ECSCOPE_ENABLE_MODERN_RENDERING)
    add_compile_definitions(ECSCOPE_ENABLE_MODERN_RENDERING=1)
endif()

if(ECSCOPE_ENABLE_DEBUG_SYSTEM)
    add_compile_definitions(ECSCOPE_ENABLE_DEBUG_SYSTEM=1)
endif()

if(ECSCOPE_ENABLE_AUDIO)
    add_compile_definitions(ECSCOPE_ENABLE_AUDIO=1)
endif()

# AI/ML Framework Configuration
if(ECSCOPE_ENABLE_AI_ML)
    add_compile_definitions(ECSCOPE_ENABLE_AI_ML=1)
endif()

# =============================================================================
# DEPENDENCY MANAGEMENT
# =============================================================================

# Find required packages
find_package(Threads REQUIRED)

# Platform-specific libraries
if(WIN32)
    set(PLATFORM_LIBS ws2_32 wsock32)
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
        # Fiber-based job system core
        src/jobs/fiber.cpp
        src/jobs/fiber_sync.cpp
        src/jobs/fiber_job_system.cpp
        src/jobs/job_dependency_graph.cpp
        src/jobs/job_profiler.cpp
        src/jobs/ecs_integration.cpp
    )
    
    target_include_directories(ecscope_jobs
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ecscope/jobs>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/concurrency>
            $<INSTALL_INTERFACE:include/ecscope/jobs>
            $<INSTALL_INTERFACE:include/ecscope/concurrency>
    )
    
    # Platform-specific libraries for fiber context switching
    set(FIBER_PLATFORM_LIBS)
    if(WIN32)
        set(FIBER_PLATFORM_LIBS kernel32)
    elseif(UNIX AND NOT APPLE)
        set(FIBER_PLATFORM_LIBS rt)
        # Check for NUMA libraries
        find_library(NUMA_LIBRARY NAMES numa)
        if(NUMA_LIBRARY)
            list(APPEND FIBER_PLATFORM_LIBS ${NUMA_LIBRARY})
            target_compile_definitions(ecscope_jobs PRIVATE ECSCOPE_HAS_NUMA=1)
        endif()
    elseif(APPLE)
        # macOS specific libraries if needed
    endif()
    
    target_link_libraries(ecscope_jobs 
        PUBLIC 
            ecscope_core
            ecscope_memory
            Threads::Threads
        PRIVATE
            ${FIBER_PLATFORM_LIBS}
    )
    
    # Add profiling support macro
    if(ECSCOPE_ENABLE_PROFILING)
        target_compile_definitions(ecscope_jobs PRIVATE ECSCOPE_ENABLE_PROFILING=1)
    endif()
    
    # Enable lock-free structures if available
    if(ECSCOPE_ENABLE_LOCKFREE)
        target_compile_definitions(ecscope_jobs PRIVATE ECSCOPE_ENABLE_LOCKFREE=1)
    endif()
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
# NETWORKING SYSTEM LIBRARY - Complete Networking with ECS Replication
# =============================================================================

if(ECSCOPE_ENABLE_NETWORKING)
    add_library(ecscope_networking STATIC)
    
    target_sources(ecscope_networking PRIVATE
        # Core networking
        src/networking/network_buffer.cpp
        
        # Socket abstractions would go here when implemented
        # src/networking/tcp_socket.cpp
        # src/networking/udp_socket.cpp
        # src/networking/reliable_udp_socket.cpp
        
        # Connection management would go here when implemented
        # src/networking/connection.cpp
        # src/networking/connection_pool.cpp
        # src/networking/connection_manager.cpp
        
        # Message system would go here when implemented
        # src/networking/network_message.cpp
        # src/networking/message_factory.cpp
        # src/networking/message_queue.cpp
        
        # ECS replication would go here when implemented
        # src/networking/ecs_replication.cpp
        # src/networking/component_replication_registry.cpp
        # src/networking/replication_manager.cpp
        
        # State synchronization would go here when implemented
        # src/networking/world_snapshot.cpp
        # src/networking/snapshot_history.cpp
        # src/networking/delta_compression_engine.cpp
        # src/networking/state_synchronization_manager.cpp
        
        # Compression and encryption would go here when implemented
        # src/networking/compression_factory.cpp
        # src/networking/null_compressor.cpp
        # src/networking/lz4_compressor.cpp
        # src/networking/zstd_compressor.cpp
        # src/networking/encryption_factory.cpp
        # src/networking/null_encryptor.cpp
        # src/networking/aes_gcm_encryptor.cpp
        # src/networking/chacha20_poly1305_encryptor.cpp
        
        # Network monitoring would go here when implemented
        # src/networking/connection_monitor.cpp
        # src/networking/network_monitor_manager.cpp
        # src/networking/network_profiler.cpp
        
        # Main network registry would go here when implemented
        # src/networking/network_registry.cpp
    )
    
    target_include_directories(ecscope_networking
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ecscope/networking>
            $<INSTALL_INTERFACE:include/ecscope/networking>
    )
    
    # Platform-specific networking libraries
    if(WIN32)
        target_link_libraries(ecscope_networking PRIVATE ws2_32 wsock32)
    endif()
    
    target_link_libraries(ecscope_networking 
        PUBLIC 
            ecscope_core
            Threads::Threads
            $<$<BOOL:${ECSCOPE_ENABLE_JOB_SYSTEM}>:ecscope_jobs>
    )
    
    target_compile_definitions(ecscope_networking PUBLIC ECSCOPE_HAS_NETWORKING=1)
    
    # Optional LZ4 support for compression
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LZ4 QUIET liblz4)
        if(LZ4_FOUND)
            target_link_libraries(ecscope_networking PRIVATE ${LZ4_LIBRARIES})
            target_include_directories(ecscope_networking PRIVATE ${LZ4_INCLUDE_DIRS})
            target_compile_definitions(ecscope_networking PRIVATE ECSCOPE_HAS_LZ4=1)
            message(STATUS "LZ4 found: Enabling LZ4 compression support")
        endif()
    endif()
    
    # Optional zstd support for compression
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(ZSTD QUIET libzstd)
        if(ZSTD_FOUND)
            target_link_libraries(ecscope_networking PRIVATE ${ZSTD_LIBRARIES})
            target_include_directories(ecscope_networking PRIVATE ${ZSTD_INCLUDE_DIRS})
            target_compile_definitions(ecscope_networking PRIVATE ECSCOPE_HAS_ZSTD=1)
            message(STATUS "zstd found: Enabling Zstandard compression support")
        endif()
    endif()
    
    # Optional OpenSSL support for encryption
    find_package(OpenSSL QUIET)
    if(OPENSSL_FOUND)
        target_link_libraries(ecscope_networking PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(ecscope_networking PRIVATE ECSCOPE_HAS_OPENSSL=1)
        message(STATUS "OpenSSL found: Enabling encryption support")
    else()
        message(STATUS "OpenSSL not found - encryption will use internal implementations")
    endif()
endif()

# =============================================================================
# AI/ML FRAMEWORK - Behavior AI, Neural Networks, and Procedural Content Generation
# =============================================================================

if(ECSCOPE_ENABLE_AI_ML)
    add_library(ecscope_ai STATIC)
    
    target_sources(ecscope_ai PRIVATE
        # Core AI infrastructure
        src/ai/core/ai_memory.cpp
        src/ai/core/ai_systems.cpp
        src/ai/core/blackboard.cpp
        
        # Behavior AI systems
        src/ai/behavior/finite_state_machine.cpp
        src/ai/behavior/behavior_tree.cpp
        # src/ai/behavior/goap.cpp
        # src/ai/behavior/utility_ai.cpp
        # src/ai/behavior/flocking.cpp
        # src/ai/behavior/steering.cpp
        
        # Machine Learning
        src/ai/ml/neural_network.cpp
        # src/ai/ml/reinforcement_learning.cpp
        # src/ai/ml/genetic_algorithm.cpp
        # src/ai/ml/decision_tree.cpp
        # src/ai/ml/clustering.cpp
        # src/ai/ml/feature_extraction.cpp
        
        # Procedural Content Generation
        src/ai/pcg/noise.cpp
        # src/ai/pcg/l_systems.cpp
        # src/ai/pcg/wave_function_collapse.cpp
        # src/ai/pcg/cellular_automata.cpp
        # src/ai/pcg/grammar_generation.cpp
        # src/ai/pcg/markov_chains.cpp
        
        # Performance Optimization
        # src/ai/optimization/ecs_optimizer.cpp
        # src/ai/optimization/load_balancer.cpp
        # src/ai/optimization/predictive_cache.cpp
        # src/ai/optimization/parameter_tuner.cpp
        # src/ai/optimization/anomaly_detector.cpp
        # src/ai/optimization/memory_optimizer.cpp
    )
    
    target_include_directories(ecscope_ai
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ecscope/ai>
            $<INSTALL_INTERFACE:include/ecscope/ai>
    )
    
    target_link_libraries(ecscope_ai 
        PUBLIC 
            ecscope_core
            Threads::Threads
            $<$<BOOL:${ECSCOPE_ENABLE_JOB_SYSTEM}>:ecscope_jobs>
            $<$<BOOL:${ECSCOPE_ENABLE_MEMORY_ANALYSIS}>:ecscope_memory>
    )
    
    target_compile_definitions(ecscope_ai PUBLIC ECSCOPE_HAS_AI_ML=1)
    
    # AI/ML specific compiler optimizations
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(ecscope_ai PRIVATE -ffast-math -funroll-loops)
        if(ECSCOPE_ENABLE_SIMD)
            target_compile_options(ecscope_ai PRIVATE -mavx -mavx2)
        endif()
    endif()
    
    message(STATUS "AI/ML Framework configured successfully")
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
# MODERN RENDERING SYSTEM LIBRARY - Professional 3D Rendering Engine
# =============================================================================

set(ECSCOPE_HAS_VULKAN OFF)
set(ECSCOPE_HAS_MODERN_RENDERING OFF)

if(ECSCOPE_ENABLE_MODERN_RENDERING)
    # Try to find Vulkan
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        set(ECSCOPE_HAS_VULKAN ON)
        message(STATUS "Vulkan found: Enabling Vulkan backend")
        
        # Check for VMA (Vulkan Memory Allocator)
        find_path(VMA_INCLUDE_DIR vk_mem_alloc.h
            HINTS
                ${Vulkan_INCLUDE_DIRS}
                /usr/include
                /usr/local/include
                ${CMAKE_SOURCE_DIR}/external/VulkanMemoryAllocator/include
            PATH_SUFFIXES vma
        )
        
        if(VMA_INCLUDE_DIR)
            message(STATUS "VMA found at: ${VMA_INCLUDE_DIR}")
            set(ECSCOPE_HAS_VMA ON)
        else()
            message(STATUS "VMA not found - using fallback memory allocation")
            set(ECSCOPE_HAS_VMA OFF)
        endif()
    else()
        message(STATUS "Vulkan not found - Vulkan backend will be disabled")
    endif()
    
    # Try to find GLEW for OpenGL backend
    find_package(GLEW QUIET)
    if(GLEW_FOUND OR TARGET GLEW::GLEW)
        set(ECSCOPE_HAS_OPENGL_BACKEND ON)
        message(STATUS "GLEW found: Enabling OpenGL backend")
    else()
        # Try pkg-config approach
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(GLEW QUIET glew)
            if(GLEW_FOUND)
                set(ECSCOPE_HAS_OPENGL_BACKEND ON)
                message(STATUS "GLEW found via pkg-config: Enabling OpenGL backend")
            endif()
        endif()
    endif()
    
    if(NOT ECSCOPE_HAS_OPENGL_BACKEND)
        message(STATUS "GLEW not found - OpenGL backend will be disabled")
    endif()
    
    # Require at least one backend
    if(ECSCOPE_HAS_VULKAN OR ECSCOPE_HAS_OPENGL_BACKEND)
        set(ECSCOPE_HAS_MODERN_RENDERING ON)
        message(STATUS "Modern rendering system enabled")
    else()
        message(WARNING "Modern rendering requested but no supported backends found. Install Vulkan SDK and/or GLEW.")
    endif()
endif()

# Modern rendering library
if(ECSCOPE_HAS_MODERN_RENDERING)
    add_library(ecscope_modern_rendering STATIC)
    
    target_sources(ecscope_modern_rendering PRIVATE
        # Core rendering system implementation
        src/rendering/renderer_factory.cpp
        src/rendering/deferred_renderer.cpp
        
        # Vulkan backend implementation
        $<$<BOOL:${ECSCOPE_HAS_VULKAN}>:src/rendering/vulkan_renderer.cpp>
        $<$<BOOL:${ECSCOPE_HAS_VULKAN}>:src/rendering/vulkan_buffer.cpp>
        $<$<BOOL:${ECSCOPE_HAS_VULKAN}>:src/rendering/vulkan_texture.cpp>
        $<$<BOOL:${ECSCOPE_HAS_VULKAN}>:src/rendering/vulkan_shader.cpp>
        $<$<BOOL:${ECSCOPE_HAS_VULKAN}>:src/rendering/vulkan_command_buffer.cpp>
        $<$<BOOL:${ECSCOPE_HAS_VULKAN}>:src/rendering/vulkan_memory.cpp>
        
        # OpenGL backend implementation
        $<$<BOOL:${ECSCOPE_HAS_OPENGL_BACKEND}>:src/rendering/opengl_renderer.cpp>
        
        # Common utilities
        src/rendering/shader_compiler.cpp
        src/rendering/materials.cpp
    )
    
    target_include_directories(ecscope_modern_rendering
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ecscope/rendering>
            $<INSTALL_INTERFACE:include/ecscope/rendering>
    )
    
    # Link rendering dependencies
    target_link_libraries(ecscope_modern_rendering 
        PUBLIC 
            ecscope_core
            ecscope_memory
            $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:ecscope_physics>
    )
    
    # Vulkan support
    if(ECSCOPE_HAS_VULKAN)
        target_link_libraries(ecscope_modern_rendering PUBLIC Vulkan::Vulkan)
        target_compile_definitions(ecscope_modern_rendering PUBLIC ECSCOPE_HAS_VULKAN=1)
        
        # VMA support
        if(ECSCOPE_HAS_VMA)
            target_include_directories(ecscope_modern_rendering PRIVATE ${VMA_INCLUDE_DIR})
            target_compile_definitions(ecscope_modern_rendering PRIVATE ECSCOPE_HAS_VMA=1)
        endif()
    endif()
    
    # OpenGL support
    if(ECSCOPE_HAS_OPENGL_BACKEND)
        if(TARGET GLEW::GLEW)
            target_link_libraries(ecscope_modern_rendering PUBLIC GLEW::GLEW)
        else()
            target_link_libraries(ecscope_modern_rendering PUBLIC ${GLEW_LIBRARIES})
            target_include_directories(ecscope_modern_rendering PUBLIC ${GLEW_INCLUDE_DIRS})
            target_compile_options(ecscope_modern_rendering PUBLIC ${GLEW_CFLAGS_OTHER})
        endif()
        
        # Also link OpenGL
        find_package(OpenGL REQUIRED)
        target_link_libraries(ecscope_modern_rendering PUBLIC ${OPENGL_LIBRARIES})
        target_compile_definitions(ecscope_modern_rendering PUBLIC ECSCOPE_HAS_OPENGL_BACKEND=1)
    endif()
    
    # GLFW for window management (optional)
    find_package(glfw3 QUIET)
    if(glfw3_FOUND OR TARGET glfw)
        target_link_libraries(ecscope_modern_rendering PUBLIC glfw)
        target_compile_definitions(ecscope_modern_rendering PUBLIC ECSCOPE_HAS_GLFW=1)
        message(STATUS "GLFW found: Window management support enabled")
    else()
        message(STATUS "GLFW not found - window management will be limited")
    endif()
    
    target_compile_definitions(ecscope_modern_rendering PUBLIC ECSCOPE_HAS_MODERN_RENDERING=1)
    
    # Set C++20 requirement for modern rendering
    target_compile_features(ecscope_modern_rendering PUBLIC cxx_std_20)
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
        # Core scripting system
        src/scripting/script_engine.cpp
        src/scripting/script_manager.cpp
        src/scripting/auto_bindings.cpp
        
        # Language-specific engines
        src/scripting/lua_engine.cpp
        src/scripting/python_engine.cpp
        
        # Advanced features
        src/scripting/script_hot_reloader.cpp
        src/scripting/script_debugger.cpp
        src/scripting/multi_language_repl.cpp
        
        # Engine system bindings
        src/scripting/ecs_bindings.cpp
        src/scripting/engine_bindings.cpp
    )
    
    # Try to find Python 3.11+
    find_package(Python3 3.11 COMPONENTS Interpreter Development QUIET)
    if(Python3_FOUND)
        target_link_libraries(ecscope_scripting PUBLIC Python3::Python)
        target_compile_definitions(ecscope_scripting PUBLIC ECSCOPE_HAS_PYTHON=1)
        
        # Try to find NumPy for advanced array support
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import numpy; print(numpy.get_include())"
            OUTPUT_VARIABLE NUMPY_INCLUDE_PATH
            RESULT_VARIABLE NUMPY_NOT_FOUND
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(NOT NUMPY_NOT_FOUND)
            target_include_directories(ecscope_scripting PRIVATE ${NUMPY_INCLUDE_PATH})
            target_compile_definitions(ecscope_scripting PUBLIC ECSCOPE_HAS_NUMPY=1)
            message(STATUS "NumPy found: Enabling advanced array support")
        endif()
        
        message(STATUS "Python ${Python3_VERSION} found: Enabling comprehensive Python scripting support")
    else()
        message(STATUS "Python 3.11+ not found - Python scripting disabled")
    endif()
    
    # Try to find Lua 5.4+
    find_package(Lua 5.4 QUIET)
    if(NOT LUA_FOUND)
        find_package(Lua 5.3 QUIET)
    endif()
    if(LUA_FOUND)
        target_link_libraries(ecscope_scripting PUBLIC ${LUA_LIBRARIES})
        target_include_directories(ecscope_scripting PUBLIC ${LUA_INCLUDE_DIR})
        target_compile_definitions(ecscope_scripting PUBLIC ECSCOPE_HAS_LUA=1)
        message(STATUS "Lua ${LUA_VERSION_STRING} found: Enabling comprehensive Lua scripting support")
    else()
        message(STATUS "Lua 5.3+ not found - Lua scripting disabled")
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
# GUI SYSTEM LIBRARY - Professional Immediate Mode GUI Framework
# =============================================================================

if(ECSCOPE_HAS_MODERN_RENDERING)
    add_library(ecscope_gui STATIC)
    
    # Try to find FreeType for font rendering
    find_package(Freetype QUIET)
    if(Freetype_FOUND OR FREETYPE_FOUND)
        set(ECSCOPE_HAS_FREETYPE ON)
        message(STATUS "FreeType found: Advanced text rendering enabled")
    else()
        set(ECSCOPE_HAS_FREETYPE OFF)
        message(STATUS "FreeType not found - using basic font rendering")
    endif()
    
    target_sources(ecscope_gui PRIVATE
        # Core GUI system
        src/gui/gui_core.cpp
        src/gui/gui_context.cpp
        
        # Memory management
        src/gui/gui_memory.cpp
        src/gui/memory/linear_allocator.cpp
        src/gui/memory/stack_allocator.cpp
        src/gui/memory/freelist_allocator.cpp
        src/gui/memory/memory_manager.cpp
        
        # Input system
        src/gui/gui_input.cpp
        src/gui/input/input_system.cpp
        src/gui/input/keyboard_navigator.cpp
        
        # Text rendering
        src/gui/gui_text.cpp
        src/gui/text/font_atlas.cpp
        src/gui/text/text_layout_engine.cpp
        src/gui/text/text_renderer.cpp
        $<$<BOOL:${ECSCOPE_HAS_FREETYPE}>:src/gui/text/freetype_integration.cpp>
        
        # Widget system
        src/gui/gui_widgets.cpp
        src/gui/widgets/button.cpp
        src/gui/widgets/input_text.cpp
        src/gui/widgets/slider.cpp
        src/gui/widgets/checkbox.cpp
        src/gui/widgets/combo.cpp
        src/gui/widgets/listbox.cpp
        src/gui/widgets/selectable.cpp
        src/gui/widgets/color_edit.cpp
        src/gui/widgets/tree_node.cpp
        src/gui/widgets/table.cpp
        src/gui/widgets/progress_bar.cpp
        
        # Layout system
        src/gui/gui_layout.cpp
        src/gui/layout/window.cpp
        src/gui/layout/layout_container.cpp
        src/gui/layout/grid_layout.cpp
        src/gui/layout/splitter.cpp
        src/gui/layout/tabs.cpp
        src/gui/layout/docking.cpp
        src/gui/layout/popup.cpp
        src/gui/layout/menu.cpp
        src/gui/layout/tooltip.cpp
        src/gui/layout/table.cpp
        
        # Theme system
        src/gui/gui_theme.cpp
        src/gui/theme/color_palette.cpp
        src/gui/theme/gui_style.cpp
        src/gui/theme/font_manager.cpp
        src/gui/theme/animation.cpp
        src/gui/theme/theme_manager.cpp
        
        # Renderer integration
        src/gui/gui_renderer.cpp
        src/gui/renderer/draw_list.cpp
        src/gui/renderer/render_command_processor.cpp
        src/gui/renderer/gui_vertex.cpp
        
        # Advanced features
        src/gui/gui_advanced.cpp
        src/gui/advanced/drag_drop.cpp
        src/gui/advanced/context_menu.cpp
        src/gui/advanced/modal_dialog.cpp
        src/gui/advanced/notification.cpp
        src/gui/advanced/input_dialog.cpp
        src/gui/advanced/wizard.cpp
        src/gui/advanced/progress_dialog.cpp
        src/gui/advanced/property_grid.cpp
        src/gui/advanced/shortcut_manager.cpp
        
        # Platform integration
        src/gui/platform/glfw_integration.cpp
        
        # Demo and utilities
        src/gui/demo/gui_demo.cpp
        src/gui/utilities/gui_utilities.cpp
    )
    
    target_include_directories(ecscope_gui
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ecscope/gui>
            $<INSTALL_INTERFACE:include/ecscope/gui>
    )
    
    # Link dependencies
    target_link_libraries(ecscope_gui 
        PUBLIC 
            ecscope_core
            ecscope_memory
            ecscope_modern_rendering
        PRIVATE
            $<$<BOOL:${ECSCOPE_HAS_FREETYPE}>:${FREETYPE_LIBRARIES}>
    )
    
    # FreeType support
    if(ECSCOPE_HAS_FREETYPE)
        target_include_directories(ecscope_gui PRIVATE ${FREETYPE_INCLUDE_DIRS})
        target_compile_definitions(ecscope_gui PRIVATE ECSCOPE_HAS_FREETYPE=1)
    endif()
    
    # GLFW support (for input integration)
    if(glfw3_FOUND OR TARGET glfw)
        target_compile_definitions(ecscope_gui PRIVATE ECSCOPE_HAS_GLFW=1)
    endif()
    
    # OpenGL/Vulkan backend integration
    if(ECSCOPE_HAS_VULKAN)
        target_compile_definitions(ecscope_gui PRIVATE GUI_ENABLE_VULKAN_INTEGRATION=1)
    endif()
    
    if(ECSCOPE_HAS_OPENGL_BACKEND)
        target_compile_definitions(ecscope_gui PRIVATE GUI_ENABLE_OPENGL_INTEGRATION=1)
    endif()
    
    # Debug features
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(ecscope_gui PRIVATE GUI_DEBUG_MEMORY=1)
    endif()
    
    target_compile_definitions(ecscope_gui PUBLIC ECSCOPE_HAS_GUI=1)
    
    # Add to available libraries
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_gui")
    
    message(STATUS "GUI System enabled with modern rendering backend")
endif()

# =============================================================================
# ASSET SYSTEM LIBRARY - Professional Asset Management
# =============================================================================

# Asset system configuration
option(ECSCOPE_ENABLE_ASSETS "Enable comprehensive asset management system" ON)

if(ECSCOPE_ENABLE_ASSETS)
    add_library(ecscope_assets STATIC)
    
    target_sources(ecscope_assets PRIVATE
        # Core asset system
        src/assets/asset_types.cpp
        src/assets/asset.cpp
        src/assets/asset_manager.cpp
        src/assets/asset_registry.cpp
        src/assets/asset_loader.cpp
        src/assets/asset_cache.cpp
        src/assets/asset_database.cpp
        src/assets/asset_streaming.cpp
        src/assets/hot_reload.cpp
        src/assets/asset_tools.cpp
        src/assets/compression.cpp
        
        # Concrete asset implementations
        src/assets/concrete_assets.cpp
        
        # Asset processors
        src/assets/processors/asset_processor.cpp
        src/assets/processors/texture_processor.cpp
        src/assets/processors/mesh_processor.cpp
        src/assets/processors/audio_processor.cpp
        src/assets/processors/shader_processor.cpp
        
        # Engine integrations
        src/assets/integrations/rendering_integration.cpp
        src/assets/integrations/ecs_integration.cpp
        
        # Platform-specific file watching
        $<$<PLATFORM_ID:Windows>:src/assets/platform/windows_file_watcher.cpp>
        $<$<PLATFORM_ID:Linux>:src/assets/platform/linux_file_watcher.cpp>
        $<$<PLATFORM_ID:Darwin>:src/assets/platform/macos_file_watcher.cpp>
    )
    
    target_include_directories(ecscope_assets
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ecscope/assets>
            $<INSTALL_INTERFACE:include/ecscope/assets>
    )
    
    # Link dependencies
    target_link_libraries(ecscope_assets 
        PUBLIC 
            ecscope_core
            ecscope_memory
            Threads::Threads
        PRIVATE
            ${PLATFORM_LIBS}
            $<$<BOOL:${ECSCOPE_ENABLE_JOB_SYSTEM}>:ecscope_jobs>
            $<$<BOOL:${ECSCOPE_HAS_MODERN_RENDERING}>:ecscope_modern_rendering>
    )
    
    # Optional compression support
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LZ4 QUIET liblz4)
        if(LZ4_FOUND)
            target_link_libraries(ecscope_assets PRIVATE ${LZ4_LIBRARIES})
            target_include_directories(ecscope_assets PRIVATE ${LZ4_INCLUDE_DIRS})
            target_compile_definitions(ecscope_assets PRIVATE ECSCOPE_HAS_LZ4=1)
            message(STATUS "LZ4 found: Enabling LZ4 asset compression")
        endif()
        
        pkg_check_modules(ZSTD QUIET libzstd)
        if(ZSTD_FOUND)
            target_link_libraries(ecscope_assets PRIVATE ${ZSTD_LIBRARIES})
            target_include_directories(ecscope_assets PRIVATE ${ZSTD_INCLUDE_DIRS})
            target_compile_definitions(ecscope_assets PRIVATE ECSCOPE_HAS_ZSTD=1)
            message(STATUS "zstd found: Enabling Zstandard asset compression")
        endif()
    endif()
    
    # SQLite for asset database
    find_library(SQLITE3_LIBRARY NAMES sqlite3)
    if(SQLITE3_LIBRARY)
        target_link_libraries(ecscope_assets PRIVATE ${SQLITE3_LIBRARY})
        target_compile_definitions(ecscope_assets PRIVATE ECSCOPE_HAS_SQLITE3=1)
        message(STATUS "SQLite3 found: Enabling asset database support")
    else()
        message(STATUS "SQLite3 not found - asset database will use in-memory storage")
    endif()
    
    target_compile_definitions(ecscope_assets PUBLIC ECSCOPE_HAS_ASSETS=1)
    
    message(STATUS "Asset System enabled with comprehensive features")
endif()

# =============================================================================
# AUDIO SYSTEM LIBRARY - Professional 3D Spatial Audio Engine
# =============================================================================

if(ECSCOPE_ENABLE_AUDIO)
    add_library(ecscope_audio STATIC)
    
    # Try to find audio dependencies
    set(ECSCOPE_HAS_OPENAL OFF)
    set(ECSCOPE_HAS_PORTAUDIO OFF)
    
    # Try to find OpenAL for 3D audio
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(OPENAL QUIET openal)
        if(OPENAL_FOUND)
            set(ECSCOPE_HAS_OPENAL ON)
            message(STATUS "OpenAL found: Enabling OpenAL backend")
        endif()
    endif()
    
    # Try to find PortAudio for cross-platform audio
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)
        if(PORTAUDIO_FOUND)
            set(ECSCOPE_HAS_PORTAUDIO ON)
            message(STATUS "PortAudio found: Enabling PortAudio backend")
        endif()
    endif()
    
    # Try to find FFTW for FFT operations
    set(ECSCOPE_HAS_FFTW OFF)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FFTW3 QUIET fftw3)
        if(FFTW3_FOUND)
            set(ECSCOPE_HAS_FFTW ON)
            message(STATUS "FFTW3 found: Enabling advanced DSP operations")
        endif()
    endif()
    
    target_sources(ecscope_audio PRIVATE
        # Core audio system
        src/audio/core/audio_types.cpp
        src/audio/core/audio_device.cpp
        src/audio/core/audio_pipeline.cpp
        src/audio/core/audio_system.cpp
        
        # 3D audio processing
        src/audio/3d/audio_3d_engine.cpp
        src/audio/3d/hrtf_processor.cpp
        src/audio/3d/doppler_processor.cpp
        src/audio/3d/distance_processor.cpp
        
        # Audio effects
        src/audio/effects/reverb_effect.cpp
        src/audio/effects/compressor_effect.cpp
        src/audio/effects/equalizer_effect.cpp
        src/audio/effects/delay_effect.cpp
        src/audio/effects/chorus_effect.cpp
        src/audio/effects/distortion_effect.cpp
        src/audio/effects/effects_chain.cpp
        src/audio/effects/environmental_processor.cpp
        src/audio/effects/occlusion_processor.cpp
        
        # Ambisonics support
        src/audio/ambisonics/ambisonics_encoder.cpp
        src/audio/ambisonics/ambisonics_decoder.cpp
        src/audio/ambisonics/ambisonics_rotator.cpp
        src/audio/ambisonics/ambisonics_processor.cpp
        
        # Ray tracing audio
        src/audio/raytracing/audio_ray_tracer.cpp
        src/audio/raytracing/acoustic_geometry.cpp
        src/audio/raytracing/audio_bvh.cpp
        src/audio/raytracing/audio_octree.cpp
        src/audio/raytracing/convolution_room_acoustics.cpp
        
        # Audio processing and optimization
        src/audio/processing/audio_simd.cpp
        src/audio/processing/audio_thread_pool.cpp
        src/audio/processing/audio_processor.cpp
        src/audio/processing/audio_dsp.cpp
        src/audio/processing/audio_memory_manager.cpp
        src/audio/processing/audio_job_system.cpp
        
        # ECS integration
        src/audio/ecs/audio_system_ecs.cpp
        src/audio/ecs/audio_zone_system.cpp
        src/audio/ecs/audio_streaming_system.cpp
        src/audio/ecs/audio_event_system.cpp
        
        # Debug and development tools
        src/audio/debug/audio_logger.cpp
        src/audio/debug/audio_visualizer.cpp
        src/audio/debug/audio_performance_monitor.cpp
        src/audio/debug/audio_analyzer.cpp
        src/audio/debug/audio_debugger.cpp
        
        # Platform-specific device implementations
        $<$<PLATFORM_ID:Windows>:src/audio/platform/wasapi_device.cpp>
        $<$<PLATFORM_ID:Linux>:src/audio/platform/alsa_device.cpp>
        $<$<PLATFORM_ID:Darwin>:src/audio/platform/coreaudio_device.cpp>
        
        # Optional backend implementations
        $<$<BOOL:${ECSCOPE_HAS_OPENAL}>:src/audio/backends/openal_backend.cpp>
        $<$<BOOL:${ECSCOPE_HAS_PORTAUDIO}>:src/audio/backends/portaudio_backend.cpp>
        
        # File format support
        src/audio/formats/wav_decoder.cpp
        src/audio/formats/audio_stream_manager.cpp
        
        # Optional format decoders (when libraries available)
        # src/audio/formats/mp3_decoder.cpp
        # src/audio/formats/ogg_decoder.cpp
        # src/audio/formats/flac_decoder.cpp
    )
    
    target_include_directories(ecscope_audio
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ecscope/audio>
            $<INSTALL_INTERFACE:include/ecscope/audio>
    )
    
    # Link dependencies
    target_link_libraries(ecscope_audio 
        PUBLIC 
            ecscope_core
            ecscope_memory
            Threads::Threads
        PRIVATE
            $<$<BOOL:${ECSCOPE_ENABLE_JOB_SYSTEM}>:ecscope_jobs>
            $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:ecscope_physics>  # For collision detection in occlusion
            ${PLATFORM_LIBS}
    )
    
    # Platform-specific audio libraries
    if(WIN32)
        target_link_libraries(ecscope_audio PRIVATE 
            winmm      # Windows Multimedia API
            ole32      # For WASAPI COM interfaces  
            oleaut32   # For WASAPI automation
        )
    elseif(UNIX AND NOT APPLE)
        # Linux audio libraries
        find_library(ASOUND_LIBRARY NAMES asound)
        if(ASOUND_LIBRARY)
            target_link_libraries(ecscope_audio PRIVATE ${ASOUND_LIBRARY})
            target_compile_definitions(ecscope_audio PRIVATE ECSCOPE_HAS_ALSA=1)
            message(STATUS "ALSA found: Enabling ALSA audio support")
        endif()
        
        # PulseAudio support
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(PULSE QUIET libpulse-simple)
            if(PULSE_FOUND)
                target_link_libraries(ecscope_audio PRIVATE ${PULSE_LIBRARIES})
                target_include_directories(ecscope_audio PRIVATE ${PULSE_INCLUDE_DIRS})
                target_compile_definitions(ecscope_audio PRIVATE ECSCOPE_HAS_PULSE=1)
                message(STATUS "PulseAudio found: Enabling PulseAudio support")
            endif()
        endif()
    elseif(APPLE)
        # macOS audio frameworks
        find_library(COREAUDIO_FRAMEWORK CoreAudio)
        find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
        find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
        
        target_link_libraries(ecscope_audio PRIVATE
            ${COREAUDIO_FRAMEWORK}
            ${AUDIOUNIT_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
        )
        target_compile_definitions(ecscope_audio PRIVATE ECSCOPE_HAS_COREAUDIO=1)
        message(STATUS "CoreAudio frameworks found: Enabling CoreAudio support")
    endif()
    
    # OpenAL support
    if(ECSCOPE_HAS_OPENAL)
        target_link_libraries(ecscope_audio PRIVATE ${OPENAL_LIBRARIES})
        target_include_directories(ecscope_audio PRIVATE ${OPENAL_INCLUDE_DIRS})
        target_compile_definitions(ecscope_audio PRIVATE ECSCOPE_HAS_OPENAL=1)
    endif()
    
    # PortAudio support
    if(ECSCOPE_HAS_PORTAUDIO)
        target_link_libraries(ecscope_audio PRIVATE ${PORTAUDIO_LIBRARIES})
        target_include_directories(ecscope_audio PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
        target_compile_definitions(ecscope_audio PRIVATE ECSCOPE_HAS_PORTAUDIO=1)
    endif()
    
    # FFTW support for advanced DSP
    if(ECSCOPE_HAS_FFTW)
        target_link_libraries(ecscope_audio PRIVATE ${FFTW3_LIBRARIES})
        target_include_directories(ecscope_audio PRIVATE ${FFTW3_INCLUDE_DIRS})
        target_compile_definitions(ecscope_audio PRIVATE ECSCOPE_HAS_FFTW=1)
    endif()
    
    # Optional audio codec support
    if(PKG_CONFIG_FOUND)
        # libsndfile for multiple format support
        pkg_check_modules(SNDFILE QUIET sndfile)
        if(SNDFILE_FOUND)
            target_link_libraries(ecscope_audio PRIVATE ${SNDFILE_LIBRARIES})
            target_include_directories(ecscope_audio PRIVATE ${SNDFILE_INCLUDE_DIRS})
            target_compile_definitions(ecscope_audio PRIVATE ECSCOPE_HAS_SNDFILE=1)
            message(STATUS "libsndfile found: Enabling multi-format audio file support")
        endif()
        
        # Vorbis/OGG support
        pkg_check_modules(VORBIS QUIET vorbisfile)
        if(VORBIS_FOUND)
            target_link_libraries(ecscope_audio PRIVATE ${VORBIS_LIBRARIES})
            target_include_directories(ecscope_audio PRIVATE ${VORBIS_INCLUDE_DIRS})
            target_compile_definitions(ecscope_audio PRIVATE ECSCOPE_HAS_VORBIS=1)
            message(STATUS "Vorbis found: Enabling OGG/Vorbis support")
        endif()
        
        # FLAC support
        pkg_check_modules(FLAC QUIET flac)
        if(FLAC_FOUND)
            target_link_libraries(ecscope_audio PRIVATE ${FLAC_LIBRARIES})
            target_include_directories(ecscope_audio PRIVATE ${FLAC_INCLUDE_DIRS})
            target_compile_definitions(ecscope_audio PRIVATE ECSCOPE_HAS_FLAC=1)
            message(STATUS "FLAC found: Enabling FLAC support")
        endif()
    endif()
    
    # Math libraries for audio processing
    if(UNIX AND NOT APPLE)
        target_link_libraries(ecscope_audio PRIVATE m)  # Math library
    endif()
    
    # SIMD optimizations for audio DSP
    if(ECSCOPE_ENABLE_SIMD)
        target_compile_definitions(ecscope_audio PRIVATE ECSCOPE_AUDIO_ENABLE_SIMD=1)
    endif()
    
    # Debug features
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(ecscope_audio PRIVATE 
            ECSCOPE_AUDIO_DEBUG=1
            ECSCOPE_AUDIO_ENABLE_PROFILING=1
        )
    endif()
    
    target_compile_definitions(ecscope_audio PUBLIC ECSCOPE_HAS_AUDIO=1)
    
    message(STATUS "Audio System enabled with professional 3D spatial audio")
    message(STATUS "  - HRTF processing: Enabled")
    message(STATUS "  - Ambisonics support: Enabled") 
    message(STATUS "  - Ray tracing acoustics: Enabled")
    message(STATUS "  - Multi-threaded processing: Enabled")
    message(STATUS "  - SIMD optimization: ${ECSCOPE_ENABLE_SIMD}")
    message(STATUS "  - OpenAL backend: ${ECSCOPE_HAS_OPENAL}")
    message(STATUS "  - PortAudio backend: ${ECSCOPE_HAS_PORTAUDIO}")
    message(STATUS "  - FFTW support: ${ECSCOPE_HAS_FFTW}")
endif()

# =============================================================================
# DEBUG SYSTEM LIBRARY - Comprehensive Debugging and Profiling
# =============================================================================

if(ECSCOPE_ENABLE_DEBUG_SYSTEM)
    add_library(ecscope_debug STATIC)
    
    target_sources(ecscope_debug PRIVATE
        # Core debug system
        src/debug/debug_system.cpp
        src/debug/debug_renderer.cpp
        src/debug/performance_monitor.cpp
        
        # Profilers
        src/debug/profilers/cpu_profiler.cpp
        src/debug/profilers/memory_profiler.cpp
        src/debug/profilers/gpu_profiler.cpp
        src/debug/profilers/network_profiler.cpp
        src/debug/profilers/asset_profiler.cpp
        src/debug/profilers/custom_event_profiler.cpp
        
        # Visualizers
        src/debug/visualizers/performance_graph_visualizer.cpp
        src/debug/visualizers/memory_visualizer.cpp
        src/debug/visualizers/ecs_visualizer.cpp
        src/debug/visualizers/physics_debug_visualizer.cpp
        src/debug/visualizers/rendering_debug_visualizer.cpp
        src/debug/visualizers/network_visualizer.cpp
        src/debug/visualizers/chart_visualizer.cpp
        
        # Inspectors
        src/debug/inspectors/entity_inspector.cpp
        src/debug/inspectors/system_inspector.cpp
        src/debug/inspectors/asset_inspector.cpp
        src/debug/inspectors/memory_inspector.cpp
        src/debug/inspectors/shader_inspector.cpp
        src/debug/inspectors/job_system_inspector.cpp
        
        # Console system
        src/debug/console/console.cpp
        src/debug/console/remote_debug_server.cpp
        src/debug/console/script_engine.cpp
        src/debug/console/crash_analyzer.cpp
        src/debug/console/command_registry.cpp
        
        # ImGui integration
        src/debug/imgui/imgui_debug_renderer.cpp
        src/debug/imgui/imgui_integration.cpp
        
        # Utilities
        src/debug/utils/debug_utils.cpp
        src/debug/utils/global_debug_system.cpp
        src/debug/utils/debug_system_builder.cpp
        src/debug/utils/debug_presets.cpp
    )
    
    target_include_directories(ecscope_debug
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ecscope/debug>
            $<INSTALL_INTERFACE:include/ecscope/debug>
    )
    
    # Link dependencies
    target_link_libraries(ecscope_debug 
        PUBLIC 
            ecscope_core
            ecscope_memory
            Threads::Threads
        PRIVATE
            $<$<BOOL:${ECSCOPE_ENABLE_JOB_SYSTEM}>:ecscope_jobs>
            $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:ecscope_physics>
            $<$<BOOL:${ECSCOPE_HAS_GRAPHICS}>:ecscope_graphics>
            $<$<BOOL:${ECSCOPE_HAS_MODERN_RENDERING}>:ecscope_modern_rendering>
            $<$<BOOL:${ECSCOPE_ENABLE_NETWORKING}>:ecscope_networking>
            $<$<BOOL:${ECSCOPE_ENABLE_ASSETS}>:ecscope_assets>
            ${PLATFORM_LIBS}
    )
    
    # Platform-specific networking libraries for remote debugging
    if(WIN32)
        target_link_libraries(ecscope_debug PRIVATE ws2_32 wsock32)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(ecscope_debug PRIVATE rt)
    endif()
    
    # Optional ImGui support for debug UI
    if(ECSCOPE_HAS_IMGUI)
        target_compile_definitions(ecscope_debug PRIVATE ECSCOPE_HAS_IMGUI=1)
        # ImGui sources would be linked here if available
    endif()
    
    # Optional scripting support for console
    if(ECSCOPE_ENABLE_SCRIPTING)
        target_link_libraries(ecscope_debug PRIVATE ecscope_scripting)
        target_compile_definitions(ecscope_debug PRIVATE ECSCOPE_DEBUG_HAS_SCRIPTING=1)
    endif()
    
    # Debug features configuration
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(ecscope_debug PRIVATE 
            ECSCOPE_DEBUG_VERBOSE=1
            ECSCOPE_DEBUG_MEMORY_TRACKING=1
        )
    endif()
    
    # Release optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_definitions(ecscope_debug PRIVATE 
            ECSCOPE_DEBUG_MINIMAL_IMPACT=1
            ECSCOPE_DEBUG_RELEASE_BUILD=1
        )
    endif()
    
    target_compile_definitions(ecscope_debug PUBLIC ECSCOPE_HAS_DEBUG_SYSTEM=1)
    
    message(STATUS "Debug System enabled with comprehensive debugging and profiling tools")
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
        $<$<BOOL:${ECSCOPE_ENABLE_NETWORKING}>:ecscope_networking>
        $<$<BOOL:${ECSCOPE_ENABLE_PERFORMANCE_LAB}>:ecscope_performance>
        $<$<BOOL:${ECSCOPE_HAS_GRAPHICS}>:ecscope_graphics>
        $<$<BOOL:${ECSCOPE_HAS_MODERN_RENDERING}>:ecscope_modern_rendering>
        $<$<BOOL:${ECSCOPE_HAS_MODERN_RENDERING}>:ecscope_gui>
        $<$<AND:$<BOOL:${ECSCOPE_HAS_GRAPHICS}>,$<BOOL:${ECSCOPE_HAS_IMGUI}>>:ecscope_ui>
        $<$<BOOL:${ECSCOPE_ENABLE_SCRIPTING}>:ecscope_scripting>
        $<$<BOOL:${ECSCOPE_ENABLE_ASSETS}>:ecscope_assets>
        $<$<BOOL:${ECSCOPE_ENABLE_DEBUG_SYSTEM}>:ecscope_debug>
        $<$<BOOL:${ECSCOPE_ENABLE_PLUGIN_SYSTEM}>:ecscope_plugins>
        $<$<BOOL:${ECSCOPE_ENABLE_AUDIO}>:ecscope_audio>
        $<$<BOOL:${ECSCOPE_ENABLE_AI_ML}>:ecscope_ai>
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
    
    # Advanced Debug System Demo
    if(ECSCOPE_ENABLE_DEBUG_SYSTEM)
        add_executable(example_comprehensive_debug_system)
        target_sources(example_comprehensive_debug_system PRIVATE examples/advanced/comprehensive_debug_system_demo.cpp)
        target_link_libraries(example_comprehensive_debug_system PRIVATE ecscope)
        
        message(STATUS "Building comprehensive debug system demo")
    endif()
    
    # =============================================================================
    # NETWORKING EXAMPLES - Client-Server and P2P Demonstrations
    # =============================================================================
    
    # Basic Client-Server Networking
    if(ECSCOPE_ENABLE_NETWORKING)
        add_executable(example_basic_client_server)
        target_sources(example_basic_client_server PRIVATE examples/networking/basic_client_server.cpp)
        target_link_libraries(example_basic_client_server PRIVATE ecscope)
        
        # Peer-to-Peer Networking Demo
        add_executable(example_peer_to_peer_demo)
        target_sources(example_peer_to_peer_demo PRIVATE examples/networking/peer_to_peer_demo.cpp)
        target_link_libraries(example_peer_to_peer_demo PRIVATE ecscope)
        
        message(STATUS "Building networking examples")
    endif()
    
    # =============================================================================
    # GUI FRAMEWORK EXAMPLES - Professional GUI Demonstrations
    # =============================================================================
    
    # GUI Comprehensive Demo
    if(ECSCOPE_HAS_MODERN_RENDERING)
        add_executable(gui_comprehensive_demo)
        target_sources(gui_comprehensive_demo PRIVATE examples/gui_comprehensive_demo.cpp)
        target_link_libraries(gui_comprehensive_demo PRIVATE ecscope)
        
        message(STATUS "Building GUI comprehensive demo")
    endif()
    
    # =============================================================================
    # ASSET SYSTEM EXAMPLES - Professional Asset Management
    # =============================================================================
    
    if(ECSCOPE_ENABLE_ASSETS)
        # Basic Asset Loading Demo
        add_executable(example_01_basic_asset_loading)
        target_sources(example_01_basic_asset_loading PRIVATE examples/assets/01_basic_asset_loading.cpp)
        target_link_libraries(example_01_basic_asset_loading PRIVATE ecscope)
        
        # Streaming and LOD Demo
        add_executable(example_02_streaming_and_lod_demo)
        target_sources(example_02_streaming_and_lod_demo PRIVATE examples/assets/02_streaming_and_lod_demo.cpp)
        target_link_libraries(example_02_streaming_and_lod_demo PRIVATE ecscope)
        
        # Hot Reload Demo
        add_executable(example_03_hot_reload_demo)
        target_sources(example_03_hot_reload_demo PRIVATE examples/assets/03_hot_reload_demo.cpp)
        target_link_libraries(example_03_hot_reload_demo PRIVATE ecscope)
        
        message(STATUS "Building asset system examples")
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
    
    # =============================================================================
    # AUDIO SYSTEM EXAMPLES - 3D Spatial Audio Demonstrations
    # =============================================================================
    
    if(ECSCOPE_ENABLE_AUDIO)
        # 3D Spatial Audio Demo
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/audio/3d_spatial_audio_demo.cpp")
            add_executable(example_3d_spatial_audio_demo)
            target_sources(example_3d_spatial_audio_demo PRIVATE examples/audio/3d_spatial_audio_demo.cpp)
            target_link_libraries(example_3d_spatial_audio_demo PRIVATE ecscope)
            
            message(STATUS "Building 3D spatial audio demo")
        endif()
        
        # VR Spatial Audio Demo
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/audio/vr_spatial_audio_demo.cpp")
            add_executable(example_vr_spatial_audio_demo)
            target_sources(example_vr_spatial_audio_demo PRIVATE examples/audio/vr_spatial_audio_demo.cpp)
            target_link_libraries(example_vr_spatial_audio_demo PRIVATE ecscope)
            
            message(STATUS "Building VR spatial audio demo")
        endif()
    endif()
    
endif() # ECSCOPE_BUILD_EXAMPLES

# =============================================================================
# BENCHMARKS - Performance Measurement
# =============================================================================

if(ECSCOPE_BUILD_BENCHMARKS)
    
    # Audio System Benchmarks
    if(ECSCOPE_ENABLE_AUDIO)
        # Audio Ray Tracing Benchmark
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/audio/audio_raytracing_benchmark.cpp")
            add_executable(benchmark_audio_raytracing)
            target_sources(benchmark_audio_raytracing PRIVATE benchmarks/audio/audio_raytracing_benchmark.cpp)
            target_link_libraries(benchmark_audio_raytracing PRIVATE ecscope)
            
            message(STATUS "Building audio ray tracing benchmark")
        endif()
        
        # Audio Processing Performance Benchmark
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/audio/audio_performance_benchmark.cpp")
            add_executable(benchmark_audio_performance)
            target_sources(benchmark_audio_performance PRIVATE benchmarks/audio/audio_performance_benchmark.cpp)
            target_link_libraries(benchmark_audio_performance PRIVATE ecscope)
        endif()
    endif()
    
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
    if(ECSCOPE_ENABLE_JOB_SYSTEM)
        # Fiber-based job system comprehensive benchmark
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/fiber_job_system_benchmark.cpp")
            add_executable(benchmark_fiber_job_system)
            target_sources(benchmark_fiber_job_system PRIVATE benchmarks/fiber_job_system_benchmark.cpp)
            target_link_libraries(benchmark_fiber_job_system PRIVATE ecscope)
        endif()
        
        # Legacy job system benchmark (backward compatibility)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/job_system_benchmarks.cpp")
            add_executable(benchmark_job_system)
            target_sources(benchmark_job_system PRIVATE benchmarks/job_system_benchmarks.cpp)
            target_link_libraries(benchmark_job_system PRIVATE ecscope)
        endif()
    endif()
    
    # Rendering Benchmarks
    if(ECSCOPE_HAS_GRAPHICS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/rendering_benchmarks.cpp")
        add_executable(benchmark_rendering)
        target_sources(benchmark_rendering PRIVATE benchmarks/rendering_benchmarks.cpp)
        target_link_libraries(benchmark_rendering PRIVATE ecscope)
    endif()
    
endif() # ECSCOPE_BUILD_BENCHMARKS

# =============================================================================
# COMPREHENSIVE TESTING FRAMEWORK
# =============================================================================

# Testing configuration options
option(ECSCOPE_ENABLE_TESTING_FRAMEWORK "Enable comprehensive testing framework" ON)
option(ECSCOPE_ENABLE_MEMORY_TESTING "Enable memory leak detection and profiling" ON)
option(ECSCOPE_ENABLE_PERFORMANCE_TESTING "Enable performance testing and benchmarking" ON)
option(ECSCOPE_ENABLE_STRESS_TESTING "Enable stress testing" ON)
option(ECSCOPE_ENABLE_COVERAGE "Enable code coverage analysis" OFF)
option(ECSCOPE_ENABLE_SANITIZERS "Enable AddressSanitizer and other sanitizers" OFF)
option(ECSCOPE_ENABLE_FUZZING "Enable fuzz testing" OFF)

if(ECSCOPE_BUILD_TESTS)
    enable_testing()
    
    # Create testing framework library
    if(ECSCOPE_ENABLE_TESTING_FRAMEWORK)
        add_library(ecscope_testing_framework INTERFACE)
        target_include_directories(ecscope_testing_framework INTERFACE 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )
        target_compile_features(ecscope_testing_framework INTERFACE cxx_std_20)
        
        # Add platform-specific libraries for stack traces and memory monitoring
        if(WIN32)
            target_link_libraries(ecscope_testing_framework INTERFACE dbghelp psapi)
        elseif(UNIX AND NOT APPLE)
            target_link_libraries(ecscope_testing_framework INTERFACE dl)
        endif()
        
        # Enable sanitizers if requested
        if(ECSCOPE_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(ecscope_testing_framework INTERFACE
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=leak
                -fno-omit-frame-pointer
            )
            target_link_options(ecscope_testing_framework INTERFACE
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=leak
            )
        endif()
        
        # Enable coverage if requested
        if(ECSCOPE_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            target_compile_options(ecscope_testing_framework INTERFACE --coverage)
            target_link_options(ecscope_testing_framework INTERFACE --coverage)
        endif()
    endif()
    
    # Main test runner executable
    add_executable(ecscope_test_runner)
    target_sources(ecscope_test_runner PRIVATE
        tests/main.cpp
        tests/unit/ecs_component_tests.cpp
        tests/integration/ecs_system_integration_tests.cpp
        tests/performance/ecs_performance_tests.cpp
    )
    target_link_libraries(ecscope_test_runner PRIVATE 
        ecscope 
        ecscope_testing_framework
    )
    
    # Add tests for existing components if they exist
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/memory/memory_tests.cpp")
        target_sources(ecscope_test_runner PRIVATE tests/memory/memory_tests.cpp)
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/physics_engine_tests.cpp")
        target_sources(ecscope_test_runner PRIVATE tests/physics_engine_tests.cpp)
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/rendering/")
        file(GLOB RENDERING_TESTS tests/rendering/*.cpp)
        target_sources(ecscope_test_runner PRIVATE ${RENDERING_TESTS})
    endif()
    
    # Memory testing executable
    if(ECSCOPE_ENABLE_MEMORY_TESTING)
        add_executable(ecscope_memory_tests)
        target_sources(ecscope_memory_tests PRIVATE
            tests/main.cpp
            tests/memory/memory_tests.cpp
        )
        target_link_libraries(ecscope_memory_tests PRIVATE 
            ecscope 
            ecscope_testing_framework
        )
        target_compile_definitions(ecscope_memory_tests PRIVATE ECSCOPE_MEMORY_TESTING_ONLY)
    endif()
    
    # Performance testing executable
    if(ECSCOPE_ENABLE_PERFORMANCE_TESTING)
        add_executable(ecscope_performance_tests)
        target_sources(ecscope_performance_tests PRIVATE
            tests/main.cpp
            tests/performance/ecs_performance_tests.cpp
        )
        target_link_libraries(ecscope_performance_tests PRIVATE 
            ecscope 
            ecscope_testing_framework
        )
        target_compile_definitions(ecscope_performance_tests PRIVATE ECSCOPE_PERFORMANCE_TESTING_ONLY)
    endif()
    
    # Stress testing executable
    if(ECSCOPE_ENABLE_STRESS_TESTING)
        add_executable(ecscope_stress_tests)
        target_sources(ecscope_stress_tests PRIVATE
            tests/main.cpp
            tests/stress/stress_tests.cpp
        )
        target_link_libraries(ecscope_stress_tests PRIVATE 
            ecscope 
            ecscope_testing_framework
        )
        target_compile_definitions(ecscope_stress_tests PRIVATE ECSCOPE_STRESS_TESTING_ONLY)
    endif()
    
    # Threading tests executable
    add_executable(ecscope_threading_tests)
    target_sources(ecscope_threading_tests PRIVATE
        tests/main.cpp
        tests/integration/ecs_system_integration_tests.cpp
    )
    target_link_libraries(ecscope_threading_tests PRIVATE 
        ecscope 
        ecscope_testing_framework
    )
    target_compile_definitions(ecscope_threading_tests PRIVATE ECSCOPE_THREADING_TESTING_ONLY)
    
    # Fuzz testing if enabled
    if(ECSCOPE_ENABLE_FUZZING AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_executable(ecscope_fuzz_tests)
        target_sources(ecscope_fuzz_tests PRIVATE
            tests/fuzz/fuzz_tests.cpp
        )
        target_link_libraries(ecscope_fuzz_tests PRIVATE 
            ecscope 
            ecscope_testing_framework
        )
        target_compile_options(ecscope_fuzz_tests PRIVATE -fsanitize=fuzzer)
        target_link_options(ecscope_fuzz_tests PRIVATE -fsanitize=fuzzer)
    endif()
    
    # Register CTest targets
    add_test(NAME ECScope_Unit_Tests 
             COMMAND ecscope_test_runner --include-category=unit --verbose)
    add_test(NAME ECScope_Integration_Tests 
             COMMAND ecscope_test_runner --include-category=integration --verbose)
    add_test(NAME ECScope_ECS_Tests 
             COMMAND ecscope_test_runner --include-category=ecs --verbose)
    
    if(ECSCOPE_ENABLE_MEMORY_TESTING)
        add_test(NAME ECScope_Memory_Tests 
                 COMMAND ecscope_memory_tests --verbose)
    endif()
    
    if(ECSCOPE_ENABLE_PERFORMANCE_TESTING)
        add_test(NAME ECScope_Performance_Tests 
                 COMMAND ecscope_performance_tests --verbose)
    endif()
    
    # Set test properties
    set_tests_properties(ECScope_Unit_Tests PROPERTIES
        TIMEOUT 300
        LABELS "unit;quick"
    )
    
    set_tests_properties(ECScope_Integration_Tests PROPERTIES
        TIMEOUT 600
        LABELS "integration;slow"
    )
    
    set_tests_properties(ECScope_ECS_Tests PROPERTIES
        TIMEOUT 300
        LABELS "ecs;core"
    )
    
    # Create test output directory
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_output)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_data/references)
    
    # Copy test data if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_data/")
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_data/ 
             DESTINATION ${CMAKE_BINARY_DIR}/test_data/)
    endif()
    
    # Custom targets for different test categories
    add_custom_target(test_unit
        COMMAND ecscope_test_runner --include-category=unit --verbose
        DEPENDS ecscope_test_runner
        COMMENT "Running unit tests"
    )
    
    add_custom_target(test_integration
        COMMAND ecscope_test_runner --include-category=integration --verbose
        DEPENDS ecscope_test_runner
        COMMENT "Running integration tests"
    )
    
    add_custom_target(test_performance
        COMMAND ecscope_performance_tests --verbose
        DEPENDS ecscope_performance_tests
        COMMENT "Running performance tests"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    
    add_custom_target(test_all
        COMMAND ecscope_test_runner --verbose --output-format=html --output-file=test_report.html
        DEPENDS ecscope_test_runner
        COMMENT "Running all tests with HTML report"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    
    # Coverage target
    if(ECSCOPE_ENABLE_COVERAGE)
        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)
        
        if(LCOV_PATH AND GENHTML_PATH)
            add_custom_target(coverage
                COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
                COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/vcpkg/*' '*/tests/*' --output-file coverage_filtered.info
                COMMAND ${GENHTML_PATH} coverage_filtered.info --output-directory coverage_html
                DEPENDS test_all
                COMMENT "Generating code coverage report"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            )
        endif()
    endif()
    
    message(STATUS "ECScope Testing Framework configured:")
    message(STATUS "  Test Runner: ecscope_test_runner")
    message(STATUS "  Memory Testing: ${ECSCOPE_ENABLE_MEMORY_TESTING}")
    message(STATUS "  Performance Testing: ${ECSCOPE_ENABLE_PERFORMANCE_TESTING}")
    message(STATUS "  Stress Testing: ${ECSCOPE_ENABLE_STRESS_TESTING}")
    message(STATUS "  Sanitizers: ${ECSCOPE_ENABLE_SANITIZERS}")
    message(STATUS "  Coverage: ${ECSCOPE_ENABLE_COVERAGE}")
    message(STATUS "  Fuzzing: ${ECSCOPE_ENABLE_FUZZING}")
    
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

if(ECSCOPE_ENABLE_NETWORKING)
    install(TARGETS ecscope_networking EXPORT ECScope-targets)
endif()

if(ECSCOPE_ENABLE_PERFORMANCE_LAB)
    install(TARGETS ecscope_performance EXPORT ECScope-targets)
endif()

if(ECSCOPE_HAS_GRAPHICS)
    install(TARGETS ecscope_graphics EXPORT ECScope-targets)
endif()

if(ECSCOPE_HAS_MODERN_RENDERING)
    install(TARGETS ecscope_modern_rendering EXPORT ECScope-targets)
endif()

if(ECSCOPE_HAS_MODERN_RENDERING)
    install(TARGETS ecscope_gui EXPORT ECScope-targets)
endif()

if(ECSCOPE_HAS_GRAPHICS AND ECSCOPE_HAS_IMGUI)
    install(TARGETS ecscope_ui EXPORT ECScope-targets)
endif()

if(ECSCOPE_ENABLE_SCRIPTING)
    install(TARGETS ecscope_scripting EXPORT ECScope-targets)
endif()

if(ECSCOPE_ENABLE_AUDIO)
    install(TARGETS ecscope_audio EXPORT ECScope-targets)
endif()

if(ECSCOPE_ENABLE_AI_ML)
    install(TARGETS ecscope_ai EXPORT ECScope-targets)
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
message(STATUS "  Networking System: ${ECSCOPE_ENABLE_NETWORKING}")
message(STATUS "  Graphics System: ${ECSCOPE_HAS_GRAPHICS}")
message(STATUS "  Modern Rendering: ${ECSCOPE_HAS_MODERN_RENDERING}")
message(STATUS "  Plugin System: ${ECSCOPE_ENABLE_PLUGIN_SYSTEM}")
if(ECSCOPE_HAS_MODERN_RENDERING)
    message(STATUS "    - Vulkan Backend: ${ECSCOPE_HAS_VULKAN}")
    message(STATUS "    - OpenGL Backend: ${ECSCOPE_HAS_OPENGL_BACKEND}")
endif()
message(STATUS "  Professional GUI: ${ECSCOPE_HAS_MODERN_RENDERING}")
if(ECSCOPE_HAS_MODERN_RENDERING)
    message(STATUS "    - FreeType Support: ${ECSCOPE_HAS_FREETYPE}")
endif()
message(STATUS "  ImGui UI System: ${ECSCOPE_HAS_IMGUI}")
message(STATUS "  Scripting Support: ${ECSCOPE_ENABLE_SCRIPTING}")
message(STATUS "  Performance Lab: ${ECSCOPE_ENABLE_PERFORMANCE_LAB}")
message(STATUS "  Debug System: ${ECSCOPE_ENABLE_DEBUG_SYSTEM}")
message(STATUS "  Audio System: ${ECSCOPE_ENABLE_AUDIO}")
message(STATUS "  AI/ML Framework: ${ECSCOPE_ENABLE_AI_ML}")
if(ECSCOPE_ENABLE_DEBUG_SYSTEM)
    message(STATUS "    - CPU Profiler: Enabled")
    message(STATUS "    - Memory Profiler: Enabled") 
    message(STATUS "    - GPU Profiler: Enabled")
    message(STATUS "    - Network Profiler: Enabled")
    message(STATUS "    - Asset Profiler: Enabled")
    message(STATUS "    - Visual Debuggers: Enabled")
    message(STATUS "    - Runtime Inspectors: Enabled")
    message(STATUS "    - Debug Console: Enabled")
    message(STATUS "    - Remote Debugging: Enabled")
endif()
if(ECSCOPE_ENABLE_AUDIO)
    message(STATUS "    - 3D Spatial Audio: Enabled")
    message(STATUS "    - HRTF Processing: Enabled")
    message(STATUS "    - Ambisonics Support: Enabled")
    message(STATUS "    - Ray Tracing Audio: Enabled")
    message(STATUS "    - Audio Effects Chain: Enabled")
    message(STATUS "    - Multi-threaded DSP: Enabled")
endif()

if(ECSCOPE_ENABLE_AI_ML)
    message(STATUS "    - Behavior AI Systems: Enabled (FSM, Behavior Trees, GOAP, Utility AI)")
    message(STATUS "    - Machine Learning: Enabled (Neural Networks, Reinforcement Learning)")
    message(STATUS "    - Procedural Generation: Enabled (Noise, L-systems, WFC)")
    message(STATUS "    - AI Performance Optimization: Enabled")
    message(STATUS "    - Multi-threaded AI Processing: Enabled")
    message(STATUS "    - AI Memory Management: Enabled")
    message(STATUS "    - Blackboard System: Enabled")
endif()
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
if(ECSCOPE_ENABLE_NETWORKING)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_networking")
endif()
if(ECSCOPE_ENABLE_PERFORMANCE_LAB)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_performance")
endif()
if(ECSCOPE_HAS_GRAPHICS)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_graphics")
endif()
if(ECSCOPE_HAS_MODERN_RENDERING)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_modern_rendering")
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
    add_library(ecscope_plugins STATIC)
    
    target_sources(ecscope_plugins PRIVATE
        # Core plugin system
        src/plugins/plugin_interface.cpp
        src/plugins/plugin_context.cpp
        src/plugins/plugin_loader.cpp
        src/plugins/plugin_discovery.cpp
        
        # Plugin registry and management (when implemented)
        # src/plugins/plugin_registry.cpp
        # src/plugins/plugin_security.cpp
        # src/plugins/plugin_messaging.cpp
        
        # ECS integration (when implemented)
        # src/plugins/ecs_integration.cpp
        
        # Rendering integration (when implemented)
        # src/plugins/rendering_integration.cpp
        
        # SDK implementations (when implemented)
        # src/plugins/sdk/plugin_base.cpp
        # src/plugins/sdk/plugin_builder.cpp
        # src/plugins/sdk/plugin_utils.cpp
        # src/plugins/sdk/cmake_utils.cpp
        # src/plugins/sdk/testing_framework.cpp
    )
    
    target_include_directories(ecscope_plugins
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ecscope/plugins>
            $<INSTALL_INTERFACE:include>
            $<INSTALL_INTERFACE:include/ecscope/plugins>
    )
    
    # Platform-specific linking
    if(WIN32)
        target_link_libraries(ecscope_plugins PRIVATE kernel32 psapi)
    elseif(UNIX)
        target_link_libraries(ecscope_plugins PRIVATE ${CMAKE_DL_LIBS} pthread)
    endif()
    
    target_link_libraries(ecscope_plugins 
        PUBLIC 
            ecscope_core
            ecscope_memory
            $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:ecscope_physics>
            $<$<BOOL:${ECSCOPE_HAS_MODERN_RENDERING}>:ecscope_modern_rendering>
            $<$<BOOL:${ECSCOPE_HAS_MODERN_RENDERING}>:ecscope_gui>
            $<$<BOOL:${ECSCOPE_ENABLE_ASSETS}>:ecscope_assets>
        PRIVATE
            $<$<BOOL:${ECSCOPE_HAS_GRAPHICS}>:ecscope_graphics>
    )
    
    target_compile_definitions(ecscope_plugins 
        PUBLIC 
            ECSCOPE_HAS_PLUGIN_SYSTEM=1
            ECSCOPE_PLUGIN_API_VERSION=1
        PRIVATE
            $<$<CONFIG:Debug>:ECSCOPE_PLUGIN_DEBUG=1>
    )
    
    # Add plugin search paths
    if(WIN32)
        target_compile_definitions(ecscope_plugins PRIVATE 
            ECSCOPE_PLUGIN_EXTENSION=".dll"
            ECSCOPE_PLUGIN_PREFIX=""
        )
    elseif(APPLE)
        target_compile_definitions(ecscope_plugins PRIVATE 
            ECSCOPE_PLUGIN_EXTENSION=".dylib"
            ECSCOPE_PLUGIN_PREFIX="lib"
        )
    else()
        target_compile_definitions(ecscope_plugins PRIVATE 
            ECSCOPE_PLUGIN_EXTENSION=".so"
            ECSCOPE_PLUGIN_PREFIX="lib"
        )
    endif()
    
    # Add to available libraries
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_plugins")
    
    # Plugin system examples and demos
    if(ECSCOPE_BUILD_EXAMPLES)
        # Create plugins output directory
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
        
        # Build example plugins as shared libraries
        add_subdirectory(examples/plugins)
        
        # Plugin template generator utility
        add_executable(plugin_template_generator 
            examples/plugins/plugin_template_generator.cpp
        )
        target_link_libraries(plugin_template_generator PRIVATE ecscope_plugins)
        set_target_properties(plugin_template_generator PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
        )
        
        # Plugin system demonstration
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/plugin_system_demo.cpp")
            add_executable(plugin_system_demo examples/plugin_system_demo.cpp)
            target_link_libraries(plugin_system_demo 
                PRIVATE 
                    ecscope_plugins 
                    ecscope_core
                    $<$<BOOL:${ECSCOPE_ENABLE_PHYSICS}>:ecscope_physics>
            )
        endif()
        
        message(STATUS "Plugin System: Example plugins and tools will be built")
    endif()
    
    # Install plugin system
    install(TARGETS ecscope_plugins EXPORT ECScope-targets)
    install(DIRECTORY include/ecscope/plugins DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ecscope)
    
    message(STATUS "Plugin System enabled with comprehensive features")
endif()

if(ECSCOPE_ENABLE_SCRIPTING)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_scripting")
endif()

if(ECSCOPE_ENABLE_DEBUG_SYSTEM)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_debug")
endif()

if(ECSCOPE_ENABLE_AUDIO)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_audio")
endif()

if(ECSCOPE_ENABLE_AI_ML)
    set(AVAILABLE_LIBS "${AVAILABLE_LIBS} ecscope_ai")
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