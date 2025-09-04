#pragma once

#include <cstdint>
#include <cstddef>

namespace ecscope::core {

// Fixed-width integer types for consistent cross-platform behavior
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// Floating point types
using f32 = float;
using f64 = double;

// Size types
using usize = std::size_t;
using isize = std::ptrdiff_t;

// Byte type for memory operations
using byte = std::byte;

// Common aliases for readability
constexpr usize INVALID_INDEX = static_cast<usize>(-1);
constexpr u32   INVALID_ID    = static_cast<u32>(-1);

// Utility constants
constexpr usize KB = 1024;
constexpr usize MB = KB * 1024;
constexpr usize GB = MB * 1024;

// Cache line size for alignment optimizations (typical x86/x64)
constexpr usize CACHE_LINE_SIZE = 64;

// Common alignment values
constexpr usize SIMD_ALIGNMENT = 16;  // For 128-bit SIMD (SSE)
constexpr usize AVX_ALIGNMENT  = 32;  // For 256-bit SIMD (AVX)

// Template for aligned storage (C++20 compatible)
template<typename T, usize Alignment = alignof(T)>
struct alignas(Alignment) aligned_storage {
    byte data[sizeof(T)];
    
    T* as_ptr() noexcept { return reinterpret_cast<T*>(data); }
    const T* as_ptr() const noexcept { return reinterpret_cast<const T*>(data); }
};

} // namespace ecscope::core

// Import common types into global namespace for convenience
using ecscope::core::u8;
using ecscope::core::u16;
using ecscope::core::u32;
using ecscope::core::u64;

using ecscope::core::i8;
using ecscope::core::i16;
using ecscope::core::i32;
using ecscope::core::i64;

using ecscope::core::f32;
using ecscope::core::f64;

using ecscope::core::usize;
using ecscope::core::isize;
using ecscope::core::byte;