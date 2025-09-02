#include "simd_math.hpp"
#include "core/log.hpp"
#include <chrono>
#include <random>
#include <algorithm>

/**
 * @file physics/simd_math.cpp
 * @brief Implementation of SIMD-optimized vector mathematics
 */

namespace ecscope::physics::simd {

//=============================================================================
// Performance Measurement Implementation
//=============================================================================

namespace performance {
    
    AutoTuner global_tuner{};
    
    SimdBenchmarkResult benchmark_vec2_addition(usize count) noexcept {
        using namespace std::chrono;
        
        // Generate test data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dist(-1000.0f, 1000.0f);
        
        std::vector<Vec2> a_data(count);
        std::vector<Vec2> b_data(count);
        std::vector<Vec2> result_scalar(count);
        std::vector<Vec2> result_simd(count);
        
        // Initialize with random data
        for (usize i = 0; i < count; ++i) {
            a_data[i] = Vec2{dist(gen), dist(gen)};
            b_data[i] = Vec2{dist(gen), dist(gen)};
        }
        
        // Benchmark scalar implementation
        auto start_scalar = high_resolution_clock::now();
        for (usize i = 0; i < count; ++i) {
            result_scalar[i] = a_data[i] + b_data[i];
        }
        auto end_scalar = high_resolution_clock::now();
        auto scalar_time = duration_cast<nanoseconds>(end_scalar - start_scalar);
        
        // Benchmark SIMD implementation  
        auto start_simd = high_resolution_clock::now();
        batch_ops::add_vec2_arrays(a_data.data(), b_data.data(), result_simd.data(), count);
        auto end_simd = high_resolution_clock::now();
        auto simd_time = duration_cast<nanoseconds>(end_simd - start_simd);
        
        // Verify results are equivalent (within epsilon)
        bool results_match = true;
        for (usize i = 0; i < std::min(count, usize{100}); ++i) {
            if (!vec2::approximately_equal(result_scalar[i], result_simd[i], 1e-5f)) {
                results_match = false;
                break;
            }
        }
        
        if (!results_match) {
            LOG_WARN("SIMD and scalar results don't match for Vec2 addition!");
        }
        
        f64 scalar_ns = scalar_time.count();
        f64 simd_ns = simd_time.count();
        f64 speedup = simd_ns > 0.0 ? scalar_ns / simd_ns : 1.0;
        
        return SimdBenchmarkResult{
            .scalar_time_ns = scalar_ns,
            .simd_time_ns = simd_ns,
            .speedup_factor = speedup,
            .operations_count = count,
            .operation_name = "Vec2 Addition",
            .simd_implementation = 
                #ifdef ECSCOPE_HAS_AVX512F
                "AVX-512"
                #elif defined(ECSCOPE_HAS_AVX2)
                "AVX2"
                #elif defined(ECSCOPE_HAS_SSE2)
                "SSE2"
                #elif defined(ECSCOPE_HAS_NEON)
                "ARM NEON"
                #else
                "Scalar"
                #endif
        };
    }
    
    SimdBenchmarkResult benchmark_dot_products(usize count) noexcept {
        using namespace std::chrono;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dist(-1000.0f, 1000.0f);
        
        std::vector<Vec2> a_data(count);
        std::vector<Vec2> b_data(count);
        std::vector<f32> result_scalar(count);
        std::vector<f32> result_simd(count);
        
        for (usize i = 0; i < count; ++i) {
            a_data[i] = Vec2{dist(gen), dist(gen)};
            b_data[i] = Vec2{dist(gen), dist(gen)};
        }
        
        // Scalar benchmark
        auto start_scalar = high_resolution_clock::now();
        for (usize i = 0; i < count; ++i) {
            result_scalar[i] = a_data[i].dot(b_data[i]);
        }
        auto end_scalar = high_resolution_clock::now();
        auto scalar_time = duration_cast<nanoseconds>(end_scalar - start_scalar);
        
        // SIMD benchmark
        auto start_simd = high_resolution_clock::now();
        batch_ops::dot_product_arrays(a_data.data(), b_data.data(), result_simd.data(), count);
        auto end_simd = high_resolution_clock::now();
        auto simd_time = duration_cast<nanoseconds>(end_simd - start_simd);
        
        f64 scalar_ns = scalar_time.count();
        f64 simd_ns = simd_time.count();
        f64 speedup = simd_ns > 0.0 ? scalar_ns / simd_ns : 1.0;
        
        return SimdBenchmarkResult{
            .scalar_time_ns = scalar_ns,
            .simd_time_ns = simd_ns,
            .speedup_factor = speedup,
            .operations_count = count,
            .operation_name = "Vec2 Dot Products",
            .simd_implementation = 
                #ifdef ECSCOPE_HAS_AVX512F
                "AVX-512"
                #elif defined(ECSCOPE_HAS_AVX2)
                "AVX2"
                #elif defined(ECSCOPE_HAS_SSE2)
                "SSE2"
                #elif defined(ECSCOPE_HAS_NEON)
                "ARM NEON"
                #else
                "Scalar"
                #endif
        };
    }
    
    SimdBenchmarkResult benchmark_normalization(usize count) noexcept {
        using namespace std::chrono;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dist(-1000.0f, 1000.0f);
        
        std::vector<Vec2> data_scalar(count);
        std::vector<Vec2> data_simd(count);
        
        // Initialize with same random data
        for (usize i = 0; i < count; ++i) {
            Vec2 vec{dist(gen), dist(gen)};
            data_scalar[i] = vec;
            data_simd[i] = vec;
        }
        
        // Scalar benchmark
        auto start_scalar = high_resolution_clock::now();
        for (usize i = 0; i < count; ++i) {
            data_scalar[i] = data_scalar[i].normalized();
        }
        auto end_scalar = high_resolution_clock::now();
        auto scalar_time = duration_cast<nanoseconds>(end_scalar - start_scalar);
        
        // SIMD benchmark
        auto start_simd = high_resolution_clock::now();
        batch_ops::normalize_vec2_arrays(data_simd.data(), count);
        auto end_simd = high_resolution_clock::now();
        auto simd_time = duration_cast<nanoseconds>(end_simd - start_simd);
        
        f64 scalar_ns = scalar_time.count();
        f64 simd_ns = simd_time.count();
        f64 speedup = simd_ns > 0.0 ? scalar_ns / simd_ns : 1.0;
        
        return SimdBenchmarkResult{
            .scalar_time_ns = scalar_ns,
            .simd_time_ns = simd_ns,
            .speedup_factor = speedup,
            .operations_count = count,
            .operation_name = "Vec2 Normalization",
            .simd_implementation = 
                #ifdef ECSCOPE_HAS_AVX512F
                "AVX-512"
                #elif defined(ECSCOPE_HAS_AVX2)
                "AVX2"
                #elif defined(ECSCOPE_HAS_SSE2)
                "SSE2"
                #elif defined(ECSCOPE_HAS_NEON)
                "ARM NEON"
                #else
                "Scalar"
                #endif
        };
    }
    
    void AutoTuner::calibrate() noexcept {
        LOG_INFO("Auto-tuning SIMD batch sizes...");
        
        f64 best_addition_time = std::numeric_limits<f64>::max();
        f64 best_dot_time = std::numeric_limits<f64>::max();
        f64 best_norm_time = std::numeric_limits<f64>::max();
        
        for (usize batch_size : batch_sizes) {
            // Test Vec2 addition
            auto add_result = benchmark_vec2_addition(batch_size);
            if (add_result.simd_time_ns < best_addition_time) {
                best_addition_time = add_result.simd_time_ns;
                optimal_batch_size_addition = batch_size;
            }
            
            // Test dot products
            auto dot_result = benchmark_dot_products(batch_size);
            if (dot_result.simd_time_ns < best_dot_time) {
                best_dot_time = dot_result.simd_time_ns;
                optimal_batch_size_dot_product = batch_size;
            }
            
            // Test normalization
            auto norm_result = benchmark_normalization(batch_size);
            if (norm_result.simd_time_ns < best_norm_time) {
                best_norm_time = norm_result.simd_time_ns;
                optimal_batch_size_normalization = batch_size;
            }
        }
        
        LOG_INFO("Auto-tuning complete:");
        LOG_INFO("  Optimal addition batch size: " + std::to_string(optimal_batch_size_addition));
        LOG_INFO("  Optimal dot product batch size: " + std::to_string(optimal_batch_size_dot_product));
        LOG_INFO("  Optimal normalization batch size: " + std::to_string(optimal_batch_size_normalization));
    }
    
} // namespace performance

//=============================================================================
// Educational Debug Implementation
//=============================================================================

namespace debug {
    
    SimdCapabilityReport generate_capability_report() noexcept {
        SimdCapabilityReport report;
        
        #ifdef ECSCOPE_ARCH_X86
        report.architecture = "x86/x64";
        
        std::vector<std::string> instruction_sets;
        
        #ifdef ECSCOPE_HAS_SSE2
        instruction_sets.push_back("SSE2");
        #endif
        #ifdef ECSCOPE_HAS_SSE3
        instruction_sets.push_back("SSE3");
        #endif
        #ifdef ECSCOPE_HAS_SSE4_1
        instruction_sets.push_back("SSE4.1");
        #endif
        #ifdef ECSCOPE_HAS_AVX
        instruction_sets.push_back("AVX");
        #endif
        #ifdef ECSCOPE_HAS_AVX2
        instruction_sets.push_back("AVX2");
        #endif
        #ifdef ECSCOPE_HAS_AVX512F
        instruction_sets.push_back("AVX-512F");
        #endif
        #ifdef ECSCOPE_HAS_AVX512VL
        instruction_sets.push_back("AVX-512VL");
        #endif
        
        if (instruction_sets.empty()) {
            report.available_instruction_sets = "None (scalar only)";
        } else {
            report.available_instruction_sets = "";
            for (usize i = 0; i < instruction_sets.size(); ++i) {
                if (i > 0) report.available_instruction_sets += ", ";
                report.available_instruction_sets += instruction_sets[i];
            }
        }
        
        #ifdef ECSCOPE_HAS_AVX512F
        report.vector_register_count = 32;  // ZMM0-ZMM31
        report.vector_width_bits = 512;
        report.preferred_alignment = 64;
        report.theoretical_peak_flops = 32.0; // Rough estimate for modern CPUs
        #elif defined(ECSCOPE_HAS_AVX2)
        report.vector_register_count = 16;  // YMM0-YMM15
        report.vector_width_bits = 256;
        report.preferred_alignment = 32;
        report.theoretical_peak_flops = 16.0;
        #elif defined(ECSCOPE_HAS_SSE2)
        report.vector_register_count = 16;  // XMM0-XMM15
        report.vector_width_bits = 128;
        report.preferred_alignment = 16;
        report.theoretical_peak_flops = 8.0;
        #endif
        
        #elif defined(ECSCOPE_ARCH_ARM)
        report.architecture = "ARM";
        
        std::vector<std::string> instruction_sets;
        
        #ifdef ECSCOPE_HAS_NEON
        instruction_sets.push_back("NEON");
        #endif
        #ifdef ECSCOPE_HAS_SVE
        instruction_sets.push_back("SVE");
        #endif
        
        if (instruction_sets.empty()) {
            report.available_instruction_sets = "None (scalar only)";
        } else {
            report.available_instruction_sets = "";
            for (usize i = 0; i < instruction_sets.size(); ++i) {
                if (i > 0) report.available_instruction_sets += ", ";
                report.available_instruction_sets += instruction_sets[i];
            }
        }
        
        #ifdef ECSCOPE_HAS_NEON
        report.vector_register_count = 32;  // V0-V31
        report.vector_width_bits = 128;
        report.preferred_alignment = 16;
        report.theoretical_peak_flops = 8.0;
        #endif
        
        #else
        report.architecture = "Unknown";
        report.available_instruction_sets = "None (scalar only)";
        report.vector_register_count = 0;
        report.vector_width_bits = 32;  // Scalar float
        report.preferred_alignment = 4;
        report.theoretical_peak_flops = 1.0;
        #endif
        
        return report;
    }
    
    SimdVisualization visualize_simd_operation(const char* op_name, 
                                              std::function<void()> operation) noexcept {
        using namespace std::chrono;
        
        SimdVisualization viz;
        viz.operation_name = op_name;
        
        // Measure execution time
        auto start = high_resolution_clock::now();
        operation();
        auto end = high_resolution_clock::now();
        
        viz.execution_time_ns = duration_cast<nanoseconds>(end - start).count();
        
        // Add example step descriptions (would be customized per operation)
        viz.step_descriptions = {
            "Load input vectors into SIMD registers",
            "Perform vectorized operation",
            "Store results back to memory",
            "Handle remaining scalar elements"
        };
        
        return viz;
    }
    
} // namespace debug

} // namespace ecscope::physics::simd