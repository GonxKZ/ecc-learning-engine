/**
 * @file pch.hpp
 * @brief Precompiled Header for ECScope
 * 
 * This header contains commonly used standard library headers
 * and frequently included project headers to speed up compilation.
 */

#pragma once

// =============================================================================
// C++ Standard Library Headers
// =============================================================================

// Core language support
#include <memory>
#include <utility>
#include <type_traits>
#include <concepts>
#include <functional>
#include <optional>
#include <variant>
#include <any>
#include <tuple>
#include <expected>

// Containers
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <deque>
#include <list>
#include <forward_list>
#include <queue>
#include <stack>
#include <span>

// Algorithms and ranges
#include <algorithm>
#include <ranges>
#include <execution>
#include <numeric>
#include <iterator>

// I/O and formatting
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <format>

// Threading and concurrency
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include <future>
#include <semaphore>
#include <latch>
#include <barrier>

// Time and chrono
#include <chrono>
#include <ctime>

// Math and random
#include <cmath>
#include <numbers>
#include <random>
#include <complex>

// System and platform
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <exception>
#include <stdexcept>

// =============================================================================
// Platform-Specific Headers
// =============================================================================

#ifdef ECSCOPE_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
    #include <intrin.h>
    #include <pdh.h>
#endif

#ifdef ECSCOPE_PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <pthread.h>
    #include <cpuid.h>
    #ifdef ECSCOPE_HAS_NUMA
        #include <numa.h>
    #endif
#endif

#ifdef ECSCOPE_PLATFORM_MACOS
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <pthread.h>
    #include <mach/mach.h>
    #include <CoreFoundation/CoreFoundation.h>
    #include <IOKit/IOKit.h>
#endif

// =============================================================================
// SIMD Headers
// =============================================================================

#ifdef ECSCOPE_ENABLE_SIMD
    #if defined(ECSCOPE_CPU_X86_64) || defined(ECSCOPE_CPU_X86)
        #include <immintrin.h>
        #include <xmmintrin.h>
        #include <emmintrin.h>
        #include <pmmintrin.h>
        #include <tmmintrin.h>
        #include <smmintrin.h>
        #include <nmmintrin.h>
        
        #ifdef __AVX__
            #include <avxintrin.h>
        #endif
        
        #ifdef __AVX2__
            #include <avx2intrin.h>
        #endif
        
        #ifdef ECSCOPE_HAS_AVX512
            #include <avx512fintrin.h>
            #include <avx512vlintrin.h>
            #include <avx512dqintrin.h>
            #include <avx512bwintrin.h>
        #endif
    #elif defined(ECSCOPE_CPU_ARM64) || defined(ECSCOPE_CPU_ARM32)
        #ifdef ECSCOPE_HAS_NEON
            #include <arm_neon.h>
        #endif
    #endif
#endif

// =============================================================================
// Graphics and UI Headers
// =============================================================================

#ifdef ECSCOPE_HAS_OPENGL
    #ifdef ECSCOPE_PLATFORM_WINDOWS
        #include <GL/gl.h>
        #include <GL/glu.h>
    #elif defined(ECSCOPE_PLATFORM_MACOS)
        #include <OpenGL/gl.h>
        #include <OpenGL/glu.h>
    #else
        #include <GL/gl.h>
        #include <GL/glu.h>
    #endif
#endif

#ifdef ECSCOPE_HAS_VULKAN
    #include <vulkan/vulkan.h>
#endif

#ifdef ECSCOPE_HAS_SDL2
    #include <SDL2/SDL.h>
#endif

#ifdef ECSCOPE_HAS_IMGUI
    #include <imgui.h>
#endif

// =============================================================================
// Physics Headers
// =============================================================================

#ifdef ECSCOPE_HAS_BULLET
    #include <btBulletDynamicsCommon.h>
#endif

#ifdef ECSCOPE_HAS_BOX2D
    #include <box2d/box2d.h>
#endif

// =============================================================================
// Scripting Headers
// =============================================================================

#ifdef ECSCOPE_HAS_PYTHON
    #ifdef ECSCOPE_HAS_PYBIND11
        #include <pybind11/pybind11.h>
        #include <pybind11/stl.h>
        #include <pybind11/numpy.h>
    #endif
#endif

#ifdef ECSCOPE_HAS_LUAJIT
    extern "C" {
        #include <luajit.h>
        #include <lua.h>
        #include <lauxlib.h>
        #include <lualib.h>
    }
#elif defined(ECSCOPE_HAS_LUA)
    extern "C" {
        #include <lua.h>
        #include <lauxlib.h>
        #include <lualib.h>
    }
#endif

// =============================================================================
// Testing and Benchmarking Headers
// =============================================================================

#ifdef ECSCOPE_HAS_CATCH2
    #include <catch2/catch_test_macros.hpp>
    #include <catch2/catch_session.hpp>
    #include <catch2/matchers/catch_matchers.hpp>
    #include <catch2/benchmark/catch_benchmark.hpp>
#endif

#ifdef ECSCOPE_HAS_BENCHMARK
    #include <benchmark/benchmark.h>
#endif

// =============================================================================
// ECScope Core Headers
// =============================================================================

// Core utilities (frequently used)
#include "core/types.hpp"
#include "core/log.hpp"
#include "core/time.hpp"
#include "core/id.hpp"

// Platform configuration
#include "ecscope/platform_config.h"

// Memory management
#include "memory/arena.hpp"
#include "memory/pool.hpp"

// ECS core
#include "ecs/entity.hpp"
#include "ecs/component.hpp"

// Math utilities
#include "physics/math.hpp"

// Common macros and utilities
namespace ecscope {
    // Common type aliases for convenience
    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;
    
    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;
    
    using f32 = float;
    using f64 = double;
    
    using usize = std::size_t;
    using isize = std::ptrdiff_t;
    
    // Common constants
    inline constexpr f32 PI = std::numbers::pi_v<f32>;
    inline constexpr f32 TAU = 2.0f * PI;
    inline constexpr f32 DEG_TO_RAD = PI / 180.0f;
    inline constexpr f32 RAD_TO_DEG = 180.0f / PI;
}