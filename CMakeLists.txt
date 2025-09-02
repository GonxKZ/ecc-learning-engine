cmake_minimum_required(VERSION 3.22)
project(ECScope 
    VERSION 0.1.0
    DESCRIPTION "Educational ECS Engine with Memory Observatory"
    LANGUAGES CXX)

# C++20 configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ECSCOPE_BUILD_TESTS "Build unit tests" ON)
option(ECSCOPE_ENABLE_INSTRUMENTATION "Enable tracing & memory hooks" ON)
option(ECSCOPE_ENABLE_GRAPHICS "Enable graphical UI (requires SDL2, ImGui)" OFF)
option(ECSCOPE_ENABLE_SIMD "Enable SIMD optimizations" ON)
option(ECSCOPE_ENABLE_ADVANCED_OPTIMIZATIONS "Enable advanced compiler optimizations" ON)
option(ECSCOPE_ENABLE_VECTORIZATION_REPORTS "Enable compiler vectorization reports" OFF)

# Advanced optimization configuration
if(ECSCOPE_ENABLE_ADVANCED_OPTIMIZATIONS)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)  # Enable LTO
    message(STATUS "Advanced optimizations enabled (LTO, vectorization, etc.)")
endif()

# Compiler-specific advanced optimization flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC-specific optimizations
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -O3                          # Maximum optimization
        -g                           # Debug symbols
        -fno-omit-frame-pointer      # Better profiling
        -march=native                # Optimize for current CPU
        -mtune=native                # Tune for current CPU
        -ffast-math                  # Aggressive math optimizations
        -funroll-loops               # Loop unrolling
        -fprefetch-loop-arrays       # Software prefetching
        -fvectorize                  # Enable vectorization
        -ftree-vectorize             # Tree vectorization
        -fopt-info-vec-optimized     # Vectorization success reports
        -fopt-info-vec-missed        # Vectorization failure reports
    )
    
    if(ECSCOPE_ENABLE_SIMD)
        add_compile_options(
            -msse2 -msse3 -msse4.1 -msse4.2  # SSE support
            -mavx -mavx2                      # AVX support
            -mfma                             # Fused multiply-add
        )
        # Check for AVX-512 support
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
        if(COMPILER_SUPPORTS_AVX512)
            add_compile_options(-mavx512f -mavx512vl -mavx512dq)
            message(STATUS "AVX-512 support enabled")
        endif()
    endif()
    
    if(ECSCOPE_ENABLE_VECTORIZATION_REPORTS)
        add_compile_options(-fopt-info-vec-all)
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang-specific optimizations
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -O3                          # Maximum optimization
        -g                           # Debug symbols
        -fno-omit-frame-pointer      # Better profiling
        -march=native                # Optimize for current CPU
        -mtune=native                # Tune for current CPU
        -ffast-math                  # Aggressive math optimizations
        -funroll-loops               # Loop unrolling
        -fvectorize                  # Enable vectorization
        -fslp-vectorize              # Superword-level parallelism
    )
    
    if(ECSCOPE_ENABLE_SIMD)
        add_compile_options(
            -msse2 -msse3 -msse4.1 -msse4.2  # SSE support
            -mavx -mavx2                      # AVX support
            -mfma                             # Fused multiply-add
        )
        # Check for AVX-512 support
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
        if(COMPILER_SUPPORTS_AVX512)
            add_compile_options(-mavx512f -mavx512vl -mavx512dq)
            message(STATUS "AVX-512 support enabled")
        endif()
    endif()
    
    if(ECSCOPE_ENABLE_VECTORIZATION_REPORTS)
        add_compile_options(-Rpass=loop-vectorize -Rpass-missed=loop-vectorize)
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC-specific optimizations
    add_compile_options(
        /W4                          # High warning level
        /O2                          # Maximum optimization (speed)
        /Ob2                         # Aggressive inlining
        /Ot                          # Favor fast code
        /GT                          # Fiber-safe thread-local storage
        /GL                          # Whole program optimization
        /arch:AVX2                   # AVX2 support
    )
    
    if(ECSCOPE_ENABLE_SIMD)
        # Check for AVX-512 support
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("/arch:AVX512" COMPILER_SUPPORTS_AVX512)
        if(COMPILER_SUPPORTS_AVX512)
            add_compile_options(/arch:AVX512)
            message(STATUS "AVX-512 support enabled")
        endif()
    endif()
    
    # Enable vectorization reports
    if(ECSCOPE_ENABLE_VECTORIZATION_REPORTS)
        add_compile_options(/Qvec-report:2)
    endif()
    
else()
    # Default flags for unknown compilers
    add_compile_options(
        -O2                          # Basic optimization
        -g                           # Debug symbols
    )
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}. Using basic optimization flags.")
endif()

# Configure SIMD support definitions
if(ECSCOPE_ENABLE_SIMD)
    add_compile_definitions(ECSCOPE_ENABLE_SIMD=1)
    message(STATUS "SIMD optimizations enabled")
else()
    message(STATUS "SIMD optimizations disabled")
endif()

# Configure instrumentation
if(ECSCOPE_ENABLE_INSTRUMENTATION)
    add_compile_definitions(ECSCOPE_ENABLE_INSTRUMENTATION=1)
endif()

# ECScope static library
add_library(ecscope STATIC)

# Source files (Phase 1-3 implementation + Advanced Optimizations)
target_sources(ecscope PRIVATE
    # Core module
    src/core/log.cpp
    src/core/time.cpp
    src/core/id.cpp
    
    # ECS module
    # src/ecs/registry.cpp  # Temporarily disabled due to logging format issues
    
    # Physics module (Phase 5) - Enhanced with SIMD
    src/physics/math.cpp
    src/physics/simd_math.cpp
    
    # Renderer module (Phase 3 & Phase 7)
    src/renderer/window.cpp
    
    # 2D Rendering System (Phase 7)
    src/renderer/renderer_2d.cpp
    src/renderer/batch_renderer.cpp  
    src/renderer/resources/texture.cpp
    src/renderer/resources/shader.cpp
    
    # UI module (Phase 3) 
    src/ui/overlay.cpp
    src/ui/panels/panel_ecs_inspector.cpp
    src/ui/panels/panel_memory.cpp
    src/ui/panels/panel_stats.cpp
    src/ui/panels/panel_performance_lab.cpp
    src/ui/panels/panel_rendering_debug.cpp
    
    # Performance Laboratory (Memory Behavior Analysis)
    src/performance/performance_lab.cpp
    src/performance/memory_experiments.cpp
    src/performance/allocation_benchmarks.cpp
    
    # Memory System (Phase 4) - Enhanced with advanced memory management
    src/memory/arena.cpp
    src/memory/pool.cpp
    src/memory/pool_allocator.cpp
    src/memory/mem_tracker.cpp
    src/memory/numa_manager.cpp
    src/memory/bandwidth_analyzer.cpp
    
    # Advanced optimization headers (header-only implementations)
    # src/ecs/advanced_concepts.hpp
    # src/ecs/soa_storage.hpp
    # src/memory/lockfree_structures.hpp
    # src/memory/lockfree_allocators.hpp
    # src/memory/hierarchical_pools.hpp
    # src/memory/cache_aware_structures.hpp
    # src/memory/thread_local_allocator.hpp
    # src/memory/memory_benchmark_suite.hpp
    # src/core/vectorization_hints.hpp
    # src/physics/simd_math.hpp
)

# Include directories
target_include_directories(ecscope
    PUBLIC 
        include
    PRIVATE 
        src
)

# Dependencies configuration
set(ECSCOPE_HAS_GRAPHICS OFF)

if(ECSCOPE_ENABLE_GRAPHICS)
    # Try to find SDL2
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        target_link_libraries(ecscope PUBLIC SDL2::SDL2)
        set(ECSCOPE_HAS_GRAPHICS ON)
        message(STATUS "SDL2 found: Enabling graphics support")
    else()
        # Try pkg-config approach
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(SDL2 QUIET sdl2)
            if(SDL2_FOUND)
                target_link_libraries(ecscope PUBLIC ${SDL2_LIBRARIES})
                target_include_directories(ecscope PUBLIC ${SDL2_INCLUDE_DIRS})
                target_compile_options(ecscope PUBLIC ${SDL2_CFLAGS_OTHER})
                set(ECSCOPE_HAS_GRAPHICS ON)
                message(STATUS "SDL2 found via pkg-config: Enabling graphics support")
            endif()
        endif()
    endif()
    
    # Try to find ImGui (typically as source in external/)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.h")
        target_include_directories(ecscope PUBLIC external/imgui)
        target_sources(ecscope PRIVATE
            external/imgui/imgui.cpp
            external/imgui/imgui_demo.cpp
            external/imgui/imgui_draw.cpp
            external/imgui/imgui_tables.cpp
            external/imgui/imgui_widgets.cpp
        )
        message(STATUS "ImGui found: Adding to build")
        set(ECSCOPE_HAS_IMGUI ON)
    else()
        message(STATUS "ImGui not found in external/imgui/")
        set(ECSCOPE_HAS_IMGUI OFF)
    endif()
    
    # Find OpenGL
    find_package(OpenGL QUIET)
    if(OPENGL_FOUND)
        target_link_libraries(ecscope PUBLIC ${OPENGL_LIBRARIES})
        message(STATUS "OpenGL found: Adding to build")
    endif()
    
    if(NOT ECSCOPE_HAS_GRAPHICS)
        message(WARNING "Graphics requested but SDL2 not found. Building console-only version.")
    endif()
endif()

# Configure graphics support
if(ECSCOPE_HAS_GRAPHICS)
    target_compile_definitions(ecscope PUBLIC ECSCOPE_HAS_GRAPHICS=1)
endif()

# ECScope console application
add_executable(ecscope_app)
target_sources(ecscope_app PRIVATE
    src/app/main.cpp
)
target_link_libraries(ecscope_app PRIVATE ecscope)

# ECScope UI application (if graphics enabled)
if(ECSCOPE_HAS_GRAPHICS)
    add_executable(ecscope_ui)
    target_sources(ecscope_ui PRIVATE
        src/app/main_ui.cpp
    )
    target_link_libraries(ecscope_ui PRIVATE ecscope)
    
    message(STATUS "Building both console (ecscope_app) and UI (ecscope_ui) applications")
else()
    message(STATUS "Building console-only application (ecscope_app)")
endif()

# Tests
if(ECSCOPE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Rendering Examples and Demos (Phase 7)
if(ECSCOPE_HAS_GRAPHICS)
    # Add option to build rendering examples
    option(ECSCOPE_BUILD_EXAMPLES "Build rendering examples and tutorials" ON)
    
    if(ECSCOPE_BUILD_EXAMPLES)
        # Main rendering demonstration
        add_executable(ecscope_rendering_demo)
        target_sources(ecscope_rendering_demo PRIVATE
            examples/rendering_2d_demo.cpp
        )
        target_link_libraries(ecscope_rendering_demo PRIVATE ecscope)
        
        # Performance benchmarking suite
        add_executable(ecscope_rendering_benchmarks)
        target_sources(ecscope_rendering_benchmarks PRIVATE
            examples/rendering_benchmarks.cpp
        )
        target_link_libraries(ecscope_rendering_benchmarks PRIVATE ecscope)
        
        # Rendering Tutorials
        # Tutorial 1: Basic Sprite Rendering
        add_executable(ecscope_tutorial_01)
        target_sources(ecscope_tutorial_01 PRIVATE
            examples/rendering_tutorials/01_basic_sprite_rendering.cpp
        )
        target_link_libraries(ecscope_tutorial_01 PRIVATE ecscope)
        
        # Tutorial 2: Batching Performance
        add_executable(ecscope_tutorial_02)
        target_sources(ecscope_tutorial_02 PRIVATE
            examples/rendering_tutorials/02_batching_performance.cpp
        )
        target_link_libraries(ecscope_tutorial_02 PRIVATE ecscope)
        
        # Tutorial 3: Advanced Cameras
        add_executable(ecscope_tutorial_03)
        target_sources(ecscope_tutorial_03 PRIVATE
            examples/rendering_tutorials/03_advanced_cameras.cpp
        )
        target_link_libraries(ecscope_tutorial_03 PRIVATE ecscope)
        
        # Performance Laboratory Interactive Demo
        add_executable(ecscope_performance_laboratory)
        target_sources(ecscope_performance_laboratory PRIVATE
            examples/performance_laboratory.cpp
        )
        target_link_libraries(ecscope_performance_laboratory PRIVATE ecscope)
        
        # Advanced C++20 Optimizations Demo
        add_executable(ecscope_advanced_optimizations_demo)
        target_sources(ecscope_advanced_optimizations_demo PRIVATE
            examples/advanced_optimizations_demo.cpp
        )
        target_link_libraries(ecscope_advanced_optimizations_demo PRIVATE ecscope)
        
        # Advanced Memory Management Examples
        add_executable(ecscope_advanced_memory_examples)
        target_sources(ecscope_advanced_memory_examples PRIVATE
            examples/advanced_memory_examples.cpp
        )
        target_link_libraries(ecscope_advanced_memory_examples PRIVATE ecscope)
        
        message(STATUS "Building rendering examples and tutorials:")
        message(STATUS "  - ecscope_rendering_demo (comprehensive demonstration)")
        message(STATUS "  - ecscope_rendering_benchmarks (performance testing)")
        message(STATUS "  - ecscope_performance_laboratory (memory behavior lab)")
        message(STATUS "  - ecscope_advanced_optimizations_demo (C++20 optimizations showcase)")
        message(STATUS "  - ecscope_advanced_memory_examples (advanced memory management examples)")
        message(STATUS "  - ecscope_tutorial_01 (basic sprite rendering)")
        message(STATUS "  - ecscope_tutorial_02 (batching performance)")
        message(STATUS "  - ecscope_tutorial_03 (advanced cameras)")
    else()
        message(STATUS "Rendering examples disabled (ECSCOPE_BUILD_EXAMPLES=OFF)")
    endif()
else()
    message(STATUS "Rendering examples require graphics support (ECSCOPE_ENABLE_GRAPHICS=ON)")
endif()

# Performance Laboratory Core Library (minimal dependencies)
add_library(ecscope_performance_core STATIC)
target_sources(ecscope_performance_core PRIVATE
    # Core module
    src/core/log.cpp
    src/core/time.cpp
    src/core/id.cpp
    
    # Performance Laboratory (Memory Behavior Analysis)
    src/performance/performance_lab.cpp
    src/performance/memory_experiments.cpp
    src/performance/allocation_benchmarks.cpp
    
    # Memory System (Phase 4)
    src/memory/arena.cpp
    src/memory/pool.cpp
    src/memory/pool_allocator.cpp
    src/memory/mem_tracker.cpp
    src/memory/numa_manager.cpp
    src/memory/bandwidth_analyzer.cpp
)

target_include_directories(ecscope_performance_core
    PUBLIC 
        include
    PRIVATE 
        src
)

# Performance Laboratory (console-based, works without graphics)
add_executable(ecscope_performance_lab_console)
target_sources(ecscope_performance_lab_console PRIVATE
    examples/performance_laboratory.cpp
)
target_link_libraries(ecscope_performance_lab_console PRIVATE ecscope_performance_core)
target_compile_definitions(ecscope_performance_lab_console PRIVATE ECSCOPE_CONSOLE_MODE=1)

# Print configuration
message(STATUS "ECScope Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests: ${ECSCOPE_BUILD_TESTS}")
message(STATUS "  Instrumentation: ${ECSCOPE_ENABLE_INSTRUMENTATION}")
message(STATUS "  Graphics Support: ${ECSCOPE_HAS_GRAPHICS}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")

if(NOT ECSCOPE_HAS_GRAPHICS)
    message(STATUS "")
    message(STATUS "Building console-only version (ECS core development).")
    message(STATUS "For graphics support, install SDL2 and run:")
    message(STATUS "  cmake -DECSCOPE_ENABLE_GRAPHICS=ON ..")
    message(STATUS "")
endif()