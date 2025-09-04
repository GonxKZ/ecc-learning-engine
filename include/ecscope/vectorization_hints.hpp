#pragma once

/**
 * @file core/vectorization_hints.hpp
 * @brief Advanced Auto-Vectorization Detection and Compiler Optimization Hints
 * 
 * This header provides sophisticated tools for detecting, enabling, and optimizing
 * auto-vectorization in modern C++ compilers:
 * 
 * Features:
 * - Compile-time vectorization capability detection
 * - Compiler-specific optimization pragmas and attributes
 * - Loop transformation hints for better vectorization
 * - Memory access pattern optimization
 * - Branch elimination for vectorizable code paths
 * - Performance analysis and feedback tools
 * 
 * Supported Compilers:
 * - GCC 9+ (with vectorization reports)
 * - Clang 10+ (with optimization remarks)
 * - MSVC 2019+ (with auto-vectorization hints)
 * - Intel C++ Compiler (with advanced vectorization)
 * 
 * Educational Value:
 * - Understanding compiler auto-vectorization
 * - Learning vectorization-friendly coding patterns
 * - Performance impact measurement
 * - Optimization strategy selection
 * 
 * @author ECScope Educational ECS Framework - Advanced Optimizations
 * @date 2025
 */

#include "core/types.hpp"
#include <concepts>
#include <span>
#include <array>
#include <type_traits>
#include <cstdlib>
#include <cstring>

namespace ecscope::core::vectorization {

//=============================================================================
// Compiler Detection and Vectorization Support
//=============================================================================

/**
 * @brief Compile-time compiler detection
 */
#if defined(__GNUC__) && !defined(__clang__)
    #define ECSCOPE_COMPILER_GCC
    #define ECSCOPE_COMPILER_NAME "GCC"
    #define ECSCOPE_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(__clang__)
    #define ECSCOPE_COMPILER_CLANG
    #define ECSCOPE_COMPILER_NAME "Clang"
    #define ECSCOPE_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(_MSC_VER)
    #define ECSCOPE_COMPILER_MSVC
    #define ECSCOPE_COMPILER_NAME "MSVC"
    #define ECSCOPE_COMPILER_VERSION _MSC_VER
#elif defined(__INTEL_COMPILER)
    #define ECSCOPE_COMPILER_ICC
    #define ECSCOPE_COMPILER_NAME "Intel C++"
    #define ECSCOPE_COMPILER_VERSION __INTEL_COMPILER
#else
    #define ECSCOPE_COMPILER_UNKNOWN
    #define ECSCOPE_COMPILER_NAME "Unknown"
    #define ECSCOPE_COMPILER_VERSION 0
#endif

/**
 * @brief Vectorization capability detection
 */
struct VectorizationCapability {
    bool supports_auto_vectorization;
    bool supports_vectorization_reports;
    bool supports_pragma_hints;
    bool supports_builtin_assume;
    bool supports_restrict_keyword;
    const char* compiler_name;
    u32 compiler_version;
    
    constexpr VectorizationCapability() noexcept
        : supports_auto_vectorization(false)
        , supports_vectorization_reports(false)
        , supports_pragma_hints(false)
        , supports_builtin_assume(false)
        , supports_restrict_keyword(false)
        , compiler_name(ECSCOPE_COMPILER_NAME)
        , compiler_version(ECSCOPE_COMPILER_VERSION) {
        
        detect_capabilities();
    }
    
private:
    constexpr void detect_capabilities() noexcept {
        #ifdef ECSCOPE_COMPILER_GCC
        if (ECSCOPE_COMPILER_VERSION >= 90000) {  // GCC 9.0+
            supports_auto_vectorization = true;
            supports_vectorization_reports = true;
            supports_pragma_hints = true;
            supports_builtin_assume = true;
            supports_restrict_keyword = true;
        }
        #endif
        
        #ifdef ECSCOPE_COMPILER_CLANG
        if (ECSCOPE_COMPILER_VERSION >= 100000) {  // Clang 10.0+
            supports_auto_vectorization = true;
            supports_vectorization_reports = true;
            supports_pragma_hints = true;
            supports_builtin_assume = true;
            supports_restrict_keyword = true;
        }
        #endif
        
        #ifdef ECSCOPE_COMPILER_MSVC
        if (ECSCOPE_COMPILER_VERSION >= 1920) {  // VS 2019+
            supports_auto_vectorization = true;
            supports_vectorization_reports = false;  // Different mechanism
            supports_pragma_hints = true;
            supports_builtin_assume = true;
            supports_restrict_keyword = true;
        }
        #endif
        
        #ifdef ECSCOPE_COMPILER_ICC
        supports_auto_vectorization = true;
        supports_vectorization_reports = true;
        supports_pragma_hints = true;
        supports_builtin_assume = true;
        supports_restrict_keyword = true;
        #endif
    }
};

constexpr VectorizationCapability vectorization_caps{};

//=============================================================================
// Compiler-Specific Optimization Hints and Pragmas
//=============================================================================

/**
 * @brief Force loop vectorization (compiler-specific)
 */
#define ECSCOPE_VECTORIZE_LOOP \
    ECSCOPE_PRAGMA(GCC ivdep) \
    ECSCOPE_PRAGMA(clang loop vectorize(enable)) \
    ECSCOPE_PRAGMA(loop(ivdep))

/**
 * @brief Disable loop vectorization (for comparison)
 */
#define ECSCOPE_NO_VECTORIZE_LOOP \
    ECSCOPE_PRAGMA(clang loop vectorize(disable)) \
    ECSCOPE_PRAGMA(loop(no_vector))

/**
 * @brief Suggest loop unrolling
 */
#define ECSCOPE_UNROLL_LOOP(factor) \
    ECSCOPE_PRAGMA(GCC unroll factor) \
    ECSCOPE_PRAGMA(clang loop unroll_count(factor)) \
    ECSCOPE_PRAGMA(loop(unroll(factor)))

/**
 * @brief Memory alignment hints
 */
#define ECSCOPE_ASSUME_ALIGNED(ptr, alignment) \
    do { \
        ECSCOPE_BUILTIN_ASSUME_ALIGNED(ptr, alignment); \
    } while(0)

/**
 * @brief Branch probability hints
 */
#define ECSCOPE_LIKELY(condition) \
    ECSCOPE_BUILTIN_LIKELY(condition)

#define ECSCOPE_UNLIKELY(condition) \
    ECSCOPE_BUILTIN_UNLIKELY(condition)

/**
 * @brief Compiler-specific implementations
 */
#ifdef ECSCOPE_COMPILER_GCC
    #define ECSCOPE_PRAGMA(directive) _Pragma(#directive)
    #define ECSCOPE_BUILTIN_ASSUME_ALIGNED(ptr, alignment) \
        (ptr) = (__typeof__(ptr))__builtin_assume_aligned((ptr), (alignment))
    #define ECSCOPE_BUILTIN_LIKELY(x) __builtin_expect(!!(x), 1)
    #define ECSCOPE_BUILTIN_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define ECSCOPE_RESTRICT __restrict__
    #define ECSCOPE_FORCE_INLINE __attribute__((always_inline)) inline
    #define ECSCOPE_PURE __attribute__((pure))
    #define ECSCOPE_CONST __attribute__((const))
    #define ECSCOPE_HOT __attribute__((hot))
    #define ECSCOPE_COLD __attribute__((cold))
#endif

#ifdef ECSCOPE_COMPILER_CLANG
    #define ECSCOPE_PRAGMA(directive) _Pragma(#directive)
    #define ECSCOPE_BUILTIN_ASSUME_ALIGNED(ptr, alignment) \
        (ptr) = (__typeof__(ptr))__builtin_assume_aligned((ptr), (alignment))
    #define ECSCOPE_BUILTIN_LIKELY(x) __builtin_expect(!!(x), 1)
    #define ECSCOPE_BUILTIN_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define ECSCOPE_RESTRICT __restrict__
    #define ECSCOPE_FORCE_INLINE __attribute__((always_inline)) inline
    #define ECSCOPE_PURE __attribute__((pure))
    #define ECSCOPE_CONST __attribute__((const))
    #define ECSCOPE_HOT __attribute__((hot))
    #define ECSCOPE_COLD __attribute__((cold))
#endif

#ifdef ECSCOPE_COMPILER_MSVC
    #define ECSCOPE_PRAGMA(directive) __pragma(directive)
    #define ECSCOPE_BUILTIN_ASSUME_ALIGNED(ptr, alignment) \
        __assume((reinterpret_cast<uintptr_t>(ptr) & ((alignment) - 1)) == 0)
    #define ECSCOPE_BUILTIN_LIKELY(x) (x)
    #define ECSCOPE_BUILTIN_UNLIKELY(x) (x)
    #define ECSCOPE_RESTRICT __restrict
    #define ECSCOPE_FORCE_INLINE __forceinline
    #define ECSCOPE_PURE
    #define ECSCOPE_CONST
    #define ECSCOPE_HOT
    #define ECSCOPE_COLD
#endif

#ifdef ECSCOPE_COMPILER_ICC
    #define ECSCOPE_PRAGMA(directive) _Pragma(#directive)
    #define ECSCOPE_BUILTIN_ASSUME_ALIGNED(ptr, alignment) \
        __assume_aligned((ptr), (alignment))
    #define ECSCOPE_BUILTIN_LIKELY(x) __builtin_expect(!!(x), 1)
    #define ECSCOPE_BUILTIN_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define ECSCOPE_RESTRICT restrict
    #define ECSCOPE_FORCE_INLINE __forceinline
    #define ECSCOPE_PURE __attribute__((pure))
    #define ECSCOPE_CONST __attribute__((const))
    #define ECSCOPE_HOT
    #define ECSCOPE_COLD
#endif

// Fallbacks for unknown compilers
#ifndef ECSCOPE_PRAGMA
    #define ECSCOPE_PRAGMA(directive)
    #define ECSCOPE_BUILTIN_ASSUME_ALIGNED(ptr, alignment) (ptr)
    #define ECSCOPE_BUILTIN_LIKELY(x) (x)
    #define ECSCOPE_BUILTIN_UNLIKELY(x) (x)
    #define ECSCOPE_RESTRICT
    #define ECSCOPE_FORCE_INLINE inline
    #define ECSCOPE_PURE
    #define ECSCOPE_CONST
    #define ECSCOPE_HOT
    #define ECSCOPE_COLD
#endif

//=============================================================================
// Vectorization-Friendly Data Structures
//=============================================================================

/**
 * @brief Aligned array for optimal vectorization
 */
template<typename T, usize N, usize Alignment = 32>
struct alignas(Alignment) VectorizedArray {
    static_assert(N > 0, "Array size must be positive");
    static_assert(Alignment >= alignof(T), "Alignment must be at least type alignment");
    static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be power of 2");
    
    alignas(Alignment) T data[N];
    
    constexpr T* begin() noexcept { return data; }
    constexpr const T* begin() const noexcept { return data; }
    constexpr T* end() noexcept { return data + N; }
    constexpr const T* end() const noexcept { return data + N; }
    
    constexpr T& operator[](usize index) noexcept { return data[index]; }
    constexpr const T& operator[](usize index) const noexcept { return data[index]; }
    
    constexpr usize size() const noexcept { return N; }
    constexpr T* get_aligned_ptr() noexcept {
        ECSCOPE_ASSUME_ALIGNED(data, Alignment);
        return data;
    }
    constexpr const T* get_aligned_ptr() const noexcept {
        ECSCOPE_ASSUME_ALIGNED(data, Alignment);
        return data;
    }
};

/**
 * @brief RAII wrapper for aligned memory allocation
 */
template<typename T>
class AlignedBuffer {
private:
    T* data_;
    usize size_;
    usize alignment_;
    
public:
    explicit AlignedBuffer(usize size, usize alignment = 32)
        : data_(nullptr)
        , size_(size)
        , alignment_(alignment) {
        
        if (size > 0) {
            data_ = static_cast<T*>(std::aligned_alloc(alignment, sizeof(T) * size));
            if (!data_) {
                throw std::bad_alloc{};
            }
        }
    }
    
    ~AlignedBuffer() {
        if (data_) {
            std::free(data_);
        }
    }
    
    // Non-copyable but movable
    AlignedBuffer(const AlignedBuffer&) = delete;
    AlignedBuffer& operator=(const AlignedBuffer&) = delete;
    
    AlignedBuffer(AlignedBuffer&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
        , alignment_(other.alignment_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }
    
    AlignedBuffer& operator=(AlignedBuffer&& other) noexcept {
        if (this != &other) {
            if (data_) {
                std::free(data_);
            }
            
            data_ = other.data_;
            size_ = other.size_;
            alignment_ = other.alignment_;
            
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }
    
    T* data() noexcept {
        ECSCOPE_ASSUME_ALIGNED(data_, alignment_);
        return data_;
    }
    
    const T* data() const noexcept {
        ECSCOPE_ASSUME_ALIGNED(data_, alignment_);
        return data_;
    }
    
    usize size() const noexcept { return size_; }
    usize alignment() const noexcept { return alignment_; }
    
    T& operator[](usize index) noexcept { return data_[index]; }
    const T& operator[](usize index) const noexcept { return data_[index]; }
    
    std::span<T> span() noexcept { return std::span<T>{data(), size()}; }
    std::span<const T> span() const noexcept { return std::span<const T>{data(), size()}; }
};

//=============================================================================
// Vectorization Pattern Templates
//=============================================================================

/**
 * @brief Vectorization-friendly loop patterns
 */
namespace patterns {
    
    /**
     * @brief Element-wise operation with vectorization hints
     */
    template<typename T, typename Op>
    ECSCOPE_FORCE_INLINE void elementwise_operation(
        T* ECSCOPE_RESTRICT output,
        const T* ECSCOPE_RESTRICT input,
        usize count,
        Op&& operation) noexcept {
        
        ECSCOPE_ASSUME_ALIGNED(output, 32);
        ECSCOPE_ASSUME_ALIGNED(input, 32);
        
        ECSCOPE_VECTORIZE_LOOP
        for (usize i = 0; i < count; ++i) {
            output[i] = operation(input[i]);
        }
    }
    
    /**
     * @brief Binary operation with vectorization hints
     */
    template<typename T, typename Op>
    ECSCOPE_FORCE_INLINE void binary_operation(
        T* ECSCOPE_RESTRICT output,
        const T* ECSCOPE_RESTRICT input_a,
        const T* ECSCOPE_RESTRICT input_b,
        usize count,
        Op&& operation) noexcept {
        
        ECSCOPE_ASSUME_ALIGNED(output, 32);
        ECSCOPE_ASSUME_ALIGNED(input_a, 32);
        ECSCOPE_ASSUME_ALIGNED(input_b, 32);
        
        ECSCOPE_VECTORIZE_LOOP
        for (usize i = 0; i < count; ++i) {
            output[i] = operation(input_a[i], input_b[i]);
        }
    }
    
    /**
     * @brief Reduction operation with vectorization
     */
    template<typename T, typename Op, typename Identity>
    ECSCOPE_FORCE_INLINE T reduction_operation(
        const T* ECSCOPE_RESTRICT input,
        usize count,
        Op&& operation,
        Identity&& identity) noexcept {
        
        ECSCOPE_ASSUME_ALIGNED(input, 32);
        
        T result = identity();
        
        ECSCOPE_VECTORIZE_LOOP
        for (usize i = 0; i < count; ++i) {
            result = operation(result, input[i]);
        }
        
        return result;
    }
    
    /**
     * @brief Conditional operation with branch elimination
     */
    template<typename T, typename Predicate, typename TrueOp, typename FalseOp>
    ECSCOPE_FORCE_INLINE void conditional_operation(
        T* ECSCOPE_RESTRICT output,
        const T* ECSCOPE_RESTRICT input,
        usize count,
        Predicate&& predicate,
        TrueOp&& true_op,
        FalseOp&& false_op) noexcept {
        
        ECSCOPE_ASSUME_ALIGNED(output, 32);
        ECSCOPE_ASSUME_ALIGNED(input, 32);
        
        ECSCOPE_VECTORIZE_LOOP
        for (usize i = 0; i < count; ++i) {
            // Use conditional assignment to avoid branches
            const T input_val = input[i];
            const bool condition = predicate(input_val);
            output[i] = condition ? true_op(input_val) : false_op(input_val);
        }
    }
    
    /**
     * @brief Strided access pattern optimization
     */
    template<typename T, typename Op>
    ECSCOPE_FORCE_INLINE void strided_operation(
        T* ECSCOPE_RESTRICT output,
        const T* ECSCOPE_RESTRICT input,
        usize count,
        usize stride,
        Op&& operation) noexcept {
        
        ECSCOPE_ASSUME_ALIGNED(output, 32);
        ECSCOPE_ASSUME_ALIGNED(input, 32);
        
        // Inform compiler about stride properties
        if (ECSCOPE_LIKELY(stride == 1)) {
            // Contiguous access - highly vectorizable
            ECSCOPE_VECTORIZE_LOOP
            for (usize i = 0; i < count; ++i) {
                output[i] = operation(input[i]);
            }
        } else {
            // Strided access - may still vectorize with gather/scatter
            ECSCOPE_VECTORIZE_LOOP
            for (usize i = 0; i < count; ++i) {
                output[i * stride] = operation(input[i * stride]);
            }
        }
    }
    
} // namespace patterns

//=============================================================================
// Vectorization Analysis and Feedback
//=============================================================================

/**
 * @brief Runtime vectorization performance analysis
 */
namespace analysis {
    
    /**
     * @brief Compare vectorized vs non-vectorized performance
     */
    template<typename T, typename Operation>
    struct VectorizationBenchmark {
        f64 vectorized_time_ns;
        f64 scalar_time_ns;
        f64 speedup_factor;
        f64 efficiency_ratio;  // Actual speedup / theoretical maximum
        usize operations_per_second;
        
        const char* analysis_notes;
    };
    
    /**
     * @brief Benchmark vectorization effectiveness
     */
    template<typename T, typename Operation>
    VectorizationBenchmark<T, Operation> benchmark_vectorization(
        usize element_count,
        Operation&& operation,
        u32 iterations = 100) {
        
        using namespace std::chrono;
        
        // Setup test data
        AlignedBuffer<T> input(element_count);
        AlignedBuffer<T> output_vec(element_count);
        AlignedBuffer<T> output_scalar(element_count);
        
        // Initialize with test data
        for (usize i = 0; i < element_count; ++i) {
            input[i] = static_cast<T>(i);
        }
        
        // Benchmark vectorized version
        auto vec_start = high_resolution_clock::now();
        for (u32 iter = 0; iter < iterations; ++iter) {
            patterns::elementwise_operation(
                output_vec.data(), input.data(), element_count, operation);
        }
        auto vec_end = high_resolution_clock::now();
        auto vec_time = duration_cast<nanoseconds>(vec_end - vec_start);
        
        // Benchmark scalar version (no vectorization hints)
        auto scalar_start = high_resolution_clock::now();
        for (u32 iter = 0; iter < iterations; ++iter) {
            ECSCOPE_NO_VECTORIZE_LOOP
            for (usize i = 0; i < element_count; ++i) {
                output_scalar[i] = operation(input[i]);
            }
        }
        auto scalar_end = high_resolution_clock::now();
        auto scalar_time = duration_cast<nanoseconds>(scalar_end - scalar_start);
        
        f64 vec_ns = vec_time.count();
        f64 scalar_ns = scalar_time.count();
        f64 speedup = scalar_ns / std::max(vec_ns, 1.0);
        
        // Estimate theoretical maximum speedup based on data type
        f64 theoretical_max = 32.0 / sizeof(T);  // Assuming 256-bit vectors
        f64 efficiency = speedup / theoretical_max;
        
        const char* notes = "Analysis complete";
        if (speedup < 1.2) {
            notes = "Poor vectorization - check for dependencies or complex operations";
        } else if (speedup > theoretical_max * 0.8) {
            notes = "Excellent vectorization efficiency achieved";
        } else {
            notes = "Good vectorization with room for improvement";
        }
        
        return VectorizationBenchmark<T, Operation>{
            .vectorized_time_ns = vec_ns,
            .scalar_time_ns = scalar_ns,
            .speedup_factor = speedup,
            .efficiency_ratio = efficiency,
            .operations_per_second = static_cast<usize>(
                (element_count * iterations * 1e9) / vec_ns),
            .analysis_notes = notes
        };
    }
    
    /**
     * @brief Analyze memory access patterns for vectorization
     */
    struct MemoryAccessAnalysis {
        bool is_contiguous;
        bool is_aligned;
        usize stride_pattern;
        f64 cache_efficiency;
        const char* vectorization_potential;
    };
    
    template<typename T>
    MemoryAccessAnalysis analyze_memory_access(const T* ptr, usize count, usize stride = 1) {
        MemoryAccessAnalysis analysis{};
        
        // Check alignment
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        analysis.is_aligned = (addr % 32) == 0;  // 256-bit alignment
        
        // Check contiguity
        analysis.is_contiguous = (stride == 1);
        analysis.stride_pattern = stride;
        
        // Estimate cache efficiency
        if (analysis.is_contiguous && analysis.is_aligned) {
            analysis.cache_efficiency = 0.95;
            analysis.vectorization_potential = "Excellent - optimal for vectorization";
        } else if (analysis.is_contiguous) {
            analysis.cache_efficiency = 0.85;
            analysis.vectorization_potential = "Good - contiguous but unaligned";
        } else if (stride <= 4) {
            analysis.cache_efficiency = 0.65;
            analysis.vectorization_potential = "Fair - small stride may vectorize";
        } else {
            analysis.cache_efficiency = 0.35;
            analysis.vectorization_potential = "Poor - large stride reduces efficiency";
        }
        
        return analysis;
    }
    
    /**
     * @brief Generate vectorization report
     */
    struct VectorizationReport {
        VectorizationCapability compiler_caps;
        std::array<MemoryAccessAnalysis, 8> memory_patterns;
        usize total_operations_analyzed;
        f64 average_vectorization_speedup;
        f64 estimated_performance_gain;
        
        std::array<const char*, 5> optimization_recommendations;
        std::array<const char*, 3> potential_issues;
    };
    
    VectorizationReport generate_comprehensive_report() {
        VectorizationReport report{};
        
        report.compiler_caps = vectorization_caps;
        report.total_operations_analyzed = 0;
        report.average_vectorization_speedup = 0.0;
        report.estimated_performance_gain = 0.0;
        
        // Sample recommendations
        report.optimization_recommendations = {{
            "Use aligned memory allocation for better SIMD performance",
            "Prefer contiguous memory access patterns",
            "Avoid complex operations inside vectorized loops", 
            "Consider data layout transformation (AoS to SoA)",
            "Use compiler-specific optimization flags"
        }};
        
        report.potential_issues = {{
            "Function calls prevent vectorization",
            "Data dependencies limit parallel execution",
            "Conditional branches reduce vectorization efficiency"
        }};
        
        return report;
    }
    
} // namespace analysis

//=============================================================================
// Educational Examples and Demonstrations
//=============================================================================

namespace examples {
    
    /**
     * @brief Demonstrate vectorization-friendly vs unfriendly patterns
     */
    namespace good_patterns {
        
        // ✅ Vectorization-friendly: simple arithmetic on contiguous arrays
        ECSCOPE_HOT void vector_add(
            const f32* ECSCOPE_RESTRICT a,
            const f32* ECSCOPE_RESTRICT b,
            f32* ECSCOPE_RESTRICT result,
            usize count) noexcept {
            
            ECSCOPE_ASSUME_ALIGNED(a, 32);
            ECSCOPE_ASSUME_ALIGNED(b, 32);
            ECSCOPE_ASSUME_ALIGNED(result, 32);
            
            ECSCOPE_VECTORIZE_LOOP
            for (usize i = 0; i < count; ++i) {
                result[i] = a[i] + b[i];
            }
        }
        
        // ✅ Vectorization-friendly: reduction with proper accumulator
        ECSCOPE_HOT f32 vector_sum(const f32* ECSCOPE_RESTRICT data, usize count) noexcept {
            ECSCOPE_ASSUME_ALIGNED(data, 32);
            
            f32 sum = 0.0f;
            ECSCOPE_VECTORIZE_LOOP
            for (usize i = 0; i < count; ++i) {
                sum += data[i];
            }
            return sum;
        }
        
        // ✅ Vectorization-friendly: branch-free conditional
        ECSCOPE_HOT void clamp_values(
            const f32* ECSCOPE_RESTRICT input,
            f32* ECSCOPE_RESTRICT output,
            usize count,
            f32 min_val,
            f32 max_val) noexcept {
            
            ECSCOPE_ASSUME_ALIGNED(input, 32);
            ECSCOPE_ASSUME_ALIGNED(output, 32);
            
            ECSCOPE_VECTORIZE_LOOP
            for (usize i = 0; i < count; ++i) {
                f32 val = input[i];
                val = val < min_val ? min_val : val;
                val = val > max_val ? max_val : val;
                output[i] = val;
            }
        }
        
    } // namespace good_patterns
    
    namespace bad_patterns {
        
        // ❌ Vectorization-unfriendly: function calls in loop
        ECSCOPE_COLD void bad_function_calls(
            const f32* input,
            f32* output,
            usize count) noexcept {
            
            for (usize i = 0; i < count; ++i) {
                output[i] = std::sin(input[i]);  // Function call prevents vectorization
            }
        }
        
        // ❌ Vectorization-unfriendly: data dependencies
        ECSCOPE_COLD void bad_dependencies(f32* data, usize count) noexcept {
            for (usize i = 1; i < count; ++i) {
                data[i] = data[i] + data[i-1];  // Dependency on previous iteration
            }
        }
        
        // ❌ Vectorization-unfriendly: complex branching
        ECSCOPE_COLD void bad_branching(
            const f32* input,
            f32* output,
            usize count) noexcept {
            
            for (usize i = 0; i < count; ++i) {
                if (input[i] > 0.5f) {
                    if (input[i] > 0.8f) {
                        output[i] = input[i] * 2.0f;
                    } else {
                        output[i] = input[i] * 1.5f;
                    }
                } else {
                    output[i] = input[i] * 0.5f;
                }
            }
        }
        
    } // namespace bad_patterns
    
    /**
     * @brief Educational comparison framework
     */
    void demonstrate_vectorization_impact() {
        constexpr usize test_size = 100000;
        
        AlignedBuffer<f32> input_a(test_size);
        AlignedBuffer<f32> input_b(test_size);
        AlignedBuffer<f32> output(test_size);
        
        // Initialize test data
        for (usize i = 0; i < test_size; ++i) {
            input_a[i] = static_cast<f32>(i) * 0.01f;
            input_b[i] = static_cast<f32>(i) * 0.02f;
        }
        
        // Benchmark good pattern
        auto good_result = analysis::benchmark_vectorization(
            test_size,
            [](f32 x) { return x * 2.0f + 1.0f; }
        );
        
        // Results would be logged for educational purposes
        // This demonstrates the performance impact of vectorization-friendly code
    }
    
} // namespace examples

} // namespace ecscope::core::vectorization