/**
 * @file hardware_analysis_educational.cpp
 * @brief Educational Hardware Analysis and Optimization Demonstrations
 * 
 * This comprehensive example demonstrates the power of hardware-aware programming
 * and shows how ECScope's hardware detection system can be used to create
 * adaptive, high-performance applications that work optimally across different
 * platforms and hardware configurations.
 * 
 * Educational Objectives:
 * - Understand hardware impact on software performance
 * - Learn hardware-aware optimization techniques
 * - Compare performance across different architectures
 * - Demonstrate thermal and power management
 * - Show cross-platform compatibility strategies
 * 
 * @author ECScope Educational ECS Framework
 * @date 2025
 */

#include "../src/platform/hardware_detection.hpp"
#include "../src/platform/optimization_engine.hpp"
#include "../src/platform/thermal_power_manager.hpp"
#include "../src/platform/graphics_detection.hpp"
#include "../src/platform/performance_benchmark.hpp"
#include "../src/platform/system_integration.hpp"
#include "../src/physics/simd_math.hpp"
#include "../src/memory/numa_manager.hpp"
#include "core/log.hpp"

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>

using namespace ecscope;
using namespace ecscope::platform;
using namespace ecscope::platform::benchmark;
using namespace ecscope::platform::thermal;
using namespace ecscope::platform::graphics;
using namespace ecscope::platform::integration;

//=============================================================================
// Educational Demonstration Classes
//=============================================================================

/**
 * @brief Interactive hardware analysis demonstration
 */
class HardwareAnalysisDemo {
private:
    HardwareDetector& detector_;
    
public:
    explicit HardwareAnalysisDemo(HardwareDetector& detector) : detector_(detector) {}
    
    void run_complete_demonstration() {
        log::info("=== ECScope Hardware Analysis Educational Demo ===\n");
        
        demonstrate_hardware_detection();
        demonstrate_simd_impact();
        demonstrate_memory_hierarchy();
        demonstrate_threading_scalability();
        demonstrate_thermal_management();
        demonstrate_cross_platform_compatibility();
        demonstrate_optimization_recommendations();
        
        log::info("=== Hardware Analysis Demo Complete ===\n");
    }
    
private:
    void demonstrate_hardware_detection() {
        log::info("--- Hardware Detection Demonstration ---");
        
        const auto& cpu_info = detector_.get_cpu_info();
        const auto& memory_info = detector_.get_memory_info();
        const auto& os_info = detector_.get_os_info();
        
        std::cout << "\n🔍 Detected Hardware Configuration:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // CPU Information
        std::cout << "🖥️  CPU: " << cpu_info.get_detailed_description() << "\n";
        std::cout << "⚡ Performance Score: " << std::fixed << std::setprecision(1) 
                  << cpu_info.get_overall_performance_score() << "/10\n";
        
        // Memory Information  
        std::cout << "💾 Memory: " << memory_info.get_memory_description() << "\n";
        std::cout << "⚡ Memory Score: " << memory_info.get_memory_performance_score() << "/10\n";
        
        // Platform Information
        std::cout << "🖧  Platform: " << os_info.get_platform_description() << "\n";
        
        // SIMD Capabilities
        if (cpu_info.simd_caps.get_performance_score() > 1.0f) {
            std::cout << "🚀 SIMD: " << cpu_info.simd_caps.get_description() 
                      << " (Score: " << cpu_info.simd_caps.get_performance_score() << ")\n";
        }
        
        // Educational insights
        std::cout << "\n💡 Educational Insights:\n";
        if (cpu_info.topology.hyperthreading_enabled) {
            std::cout << "   • Hyperthreading is enabled - consider workload characteristics\n";
        }
        if (memory_info.numa_available) {
            std::cout << "   • NUMA system detected - memory placement matters for performance\n";
        }
        if (cpu_info.simd_caps.avx2) {
            std::cout << "   • AVX2 available - vectorized operations can be 8x faster\n";
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
    
    void demonstrate_simd_impact() {
        log::info("--- SIMD Performance Impact Demonstration ---");
        
        const auto& cpu_info = detector_.get_cpu_info();
        
        if (!cpu_info.simd_caps.sse2) {
            log::info("SIMD not available on this system - skipping demonstration");
            return;
        }
        
        constexpr usize VECTOR_SIZE = 1000000;
        std::vector<f32> a(VECTOR_SIZE);
        std::vector<f32> b(VECTOR_SIZE);
        std::vector<f32> result(VECTOR_SIZE);
        
        // Initialize data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dist(0.0f, 100.0f);
        
        std::generate(a.begin(), a.end(), [&]() { return dist(gen); });
        std::generate(b.begin(), b.end(), [&]() { return dist(gen); });
        
        std::cout << "\n🧮 SIMD Performance Comparison:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // Scalar implementation
        auto scalar_time = HighResolutionTimer::measure([&]() {
            for (usize i = 0; i < VECTOR_SIZE; ++i) {
                result[i] = a[i] + b[i];
            }
        });
        
        f64 scalar_ms = std::chrono::duration<f64, std::milli>(scalar_time).count();
        std::cout << "📊 Scalar Addition:    " << std::setw(8) << std::fixed << std::setprecision(2) 
                  << scalar_ms << " ms\n";
        
        // SIMD implementation (if available)
        if (cpu_info.simd_caps.sse2 || cpu_info.simd_caps.avx2) {
            auto simd_time = HighResolutionTimer::measure([&]() {
                physics::simd::batch_ops::add_vec2_arrays(
                    reinterpret_cast<const physics::math::Vec2*>(a.data()),
                    reinterpret_cast<const physics::math::Vec2*>(b.data()),
                    reinterpret_cast<physics::math::Vec2*>(result.data()),
                    VECTOR_SIZE / 2
                );
            });
            
            f64 simd_ms = std::chrono::duration<f64, std::milli>(simd_time).count();
            f64 speedup = scalar_ms / simd_ms;
            
            std::string simd_type = cpu_info.simd_caps.avx512f ? "AVX-512" :
                                   cpu_info.simd_caps.avx2 ? "AVX2" :
                                   cpu_info.simd_caps.avx ? "AVX" : "SSE2";
            
            std::cout << "🚀 " << simd_type << " Addition: " << std::setw(8) << std::fixed << std::setprecision(2) 
                      << simd_ms << " ms (";
            
            if (speedup > 1.0) {
                std::cout << "✅ " << std::setprecision(1) << speedup << "x faster)\n";
            } else {
                std::cout << "❌ " << std::setprecision(1) << speedup << "x slower)\n";
            }
        }
        
        std::cout << "\n💡 Educational Insights:\n";
        std::cout << "   • SIMD operations process multiple data elements simultaneously\n";
        std::cout << "   • Performance gains depend on data alignment and access patterns\n";
        std::cout << "   • Modern CPUs can process 4-16 floats in a single instruction\n";
        
        // Show compiler auto-vectorization potential
        if (detector_.get_compiler_info().supports_vectorization) {
            std::cout << "   • Your compiler supports auto-vectorization optimizations\n";
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
    
    void demonstrate_memory_hierarchy() {
        log::info("--- Memory Hierarchy Impact Demonstration ---");
        
        const auto& memory_info = detector_.get_memory_info();
        const auto& cpu_info = detector_.get_cpu_info();
        
        std::cout << "\n💾 Memory Hierarchy Performance Analysis:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // Cache hierarchy information
        if (!cpu_info.cache_info.levels.empty()) {
            std::cout << "📊 Cache Hierarchy: " << cpu_info.cache_info.get_hierarchy_description() << "\n";
        }
        
        // Memory access pattern benchmark
        std::vector<usize> test_sizes = {
            1024,       // L1 cache
            32768,      // L2 cache  
            1048576,    // L3 cache
            16777216,   // Main memory
            268435456   // Large memory
        };
        
        std::vector<u8> test_data(test_sizes.back());
        std::random_device rd;
        std::mt19937 gen(rd());
        std::iota(test_data.begin(), test_data.end(), 0);
        std::shuffle(test_data.begin(), test_data.end(), gen);
        
        for (usize size : test_sizes) {
            auto access_time = HighResolutionTimer::measure([&]() {
                volatile u64 sum = 0;
                for (usize i = 0; i < size; i += 64) { // Cache line size
                    sum += test_data[i];
                }
            });
            
            f64 ns_per_access = std::chrono::duration<f64, std::nano>(access_time).count() / (size / 64);
            std::string size_label = (size >= 1048576) ? std::to_string(size / 1048576) + " MB" :
                                    (size >= 1024) ? std::to_string(size / 1024) + " KB" :
                                    std::to_string(size) + " B";
            
            std::cout << "⏱️  " << std::setw(8) << size_label << ": " 
                      << std::setw(6) << std::fixed << std::setprecision(1) 
                      << ns_per_access << " ns/access\n";
        }
        
        std::cout << "\n💡 Educational Insights:\n";
        std::cout << "   • Cache access is 10-100x faster than main memory\n";
        std::cout << "   • Data locality is crucial for performance\n";
        std::cout << "   • Cache-friendly algorithms can provide massive speedups\n";
        
        if (memory_info.numa_available) {
            std::cout << "   • NUMA-aware memory allocation can improve performance by 2-4x\n";
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
    
    void demonstrate_threading_scalability() {
        log::info("--- Threading Scalability Demonstration ---");
        
        const auto& cpu_info = detector_.get_cpu_info();
        
        std::cout << "\n🧵 Threading Performance Scalability:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        constexpr usize WORK_SIZE = 10000000;
        std::vector<f64> work_data(WORK_SIZE);
        std::iota(work_data.begin(), work_data.end(), 1.0);
        
        auto compute_work = [&work_data]() {
            return std::accumulate(work_data.begin(), work_data.end(), 0.0,
                [](f64 sum, f64 val) { return sum + std::sqrt(val); });
        };
        
        // Single-threaded baseline
        auto single_thread_time = HighResolutionTimer::measure([&]() {
            volatile f64 result = compute_work();
        });
        
        f64 baseline_ms = std::chrono::duration<f64, std::milli>(single_thread_time).count();
        std::cout << "📊 Single Thread:     " << std::setw(8) << std::fixed << std::setprecision(2) 
                  << baseline_ms << " ms\n";
        
        // Test different thread counts
        std::vector<u32> thread_counts = {2, 4, 8, 16};
        if (cpu_info.topology.logical_cores < 16) {
            thread_counts.erase(
                std::remove_if(thread_counts.begin(), thread_counts.end(),
                    [&](u32 count) { return count > cpu_info.topology.logical_cores; }),
                thread_counts.end());
        }
        
        for (u32 num_threads : thread_counts) {
            if (num_threads > cpu_info.topology.logical_cores) continue;
            
            auto multi_thread_time = HighResolutionTimer::measure([&]() {
                std::vector<std::thread> threads;
                std::vector<f64> results(num_threads);
                
                usize chunk_size = WORK_SIZE / num_threads;
                
                for (u32 i = 0; i < num_threads; ++i) {
                    threads.emplace_back([&, i]() {
                        usize start = i * chunk_size;
                        usize end = (i == num_threads - 1) ? WORK_SIZE : start + chunk_size;
                        
                        results[i] = std::accumulate(work_data.begin() + start, work_data.begin() + end, 0.0,
                            [](f64 sum, f64 val) { return sum + std::sqrt(val); });
                    });
                }
                
                for (auto& thread : threads) {
                    thread.join();
                }
                
                volatile f64 total = std::accumulate(results.begin(), results.end(), 0.0);
            });
            
            f64 multi_ms = std::chrono::duration<f64, std::milli>(multi_thread_time).count();
            f64 speedup = baseline_ms / multi_ms;
            f64 efficiency = speedup / num_threads * 100.0;
            
            std::cout << "🚀 " << std::setw(2) << num_threads << " Threads:      " 
                      << std::setw(8) << std::fixed << std::setprecision(2) << multi_ms 
                      << " ms (";
            
            if (speedup > 1.0) {
                std::cout << "✅ " << std::setprecision(1) << speedup << "x, " 
                          << std::setprecision(0) << efficiency << "% efficiency)\n";
            } else {
                std::cout << "❌ " << std::setprecision(1) << speedup << "x slower)\n";
            }
        }
        
        std::cout << "\n💡 Educational Insights:\n";
        std::cout << "   • Parallel efficiency depends on workload characteristics\n";
        std::cout << "   • Amdahl's law limits theoretical speedup\n";
        std::cout << "   • Context switching overhead increases with thread count\n";
        
        if (cpu_info.topology.hyperthreading_enabled) {
            std::cout << "   • Hyperthreading may help or hurt depending on workload\n";
        }
        
        if (cpu_info.topology.numa_nodes > 1) {
            std::cout << "   • NUMA-aware thread placement can improve scalability\n";
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
    
    void demonstrate_thermal_management() {
        log::info("--- Thermal Management Demonstration ---");
        
        // This would integrate with the thermal monitoring system
        std::cout << "\n🌡️  Thermal Management Analysis:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // Simulate thermal data (in real implementation, would read from sensors)
        f32 cpu_temp = 65.0f + (rand() % 20); // 65-85°C
        f32 gpu_temp = 70.0f + (rand() % 15); // 70-85°C
        
        std::cout << "🖥️  CPU Temperature: " << cpu_temp << "°C\n";
        std::cout << "🎮 GPU Temperature: " << gpu_temp << "°C\n";
        
        // Thermal state analysis
        ThermalState thermal_state = ThermalState::Nominal;
        if (cpu_temp > 80.0f || gpu_temp > 80.0f) {
            thermal_state = ThermalState::Hot;
        }
        if (cpu_temp > 85.0f || gpu_temp > 85.0f) {
            thermal_state = ThermalState::Critical;
        }
        
        std::cout << "🌡️  Thermal State: ";
        switch (thermal_state) {
            case ThermalState::Cool:
                std::cout << "❄️  Cool - Optimal performance available\n";
                break;
            case ThermalState::Nominal:
                std::cout << "✅ Nominal - Normal operating temperature\n";
                break;
            case ThermalState::Hot:
                std::cout << "🔥 Hot - Consider reducing performance to prevent throttling\n";
                break;
            case ThermalState::Critical:
                std::cout << "⚠️  Critical - Thermal throttling likely\n";
                break;
            default:
                std::cout << "❓ Unknown\n";
                break;
        }
        
        std::cout << "\n💡 Educational Insights:\n";
        std::cout << "   • Modern CPUs/GPUs throttle performance when overheating\n";
        std::cout << "   • Sustained workloads may need thermal management\n";
        std::cout << "   • Mobile devices are especially thermal-constrained\n";
        std::cout << "   • Adaptive performance scaling can prevent throttling\n";
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
    
    void demonstrate_cross_platform_compatibility() {
        log::info("--- Cross-Platform Compatibility Analysis ---");
        
        const auto& os_info = detector_.get_os_info();
        const auto& cpu_info = detector_.get_cpu_info();
        
        std::cout << "\n🌐 Cross-Platform Compatibility Report:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // Platform analysis
        std::cout << "🖧  Current Platform: " << os_info.name << " " << os_info.version << "\n";
        std::cout << "🏗️  Architecture: ";
        
        switch (cpu_info.architecture) {
            case CpuArchitecture::X86_64:
                std::cout << "x86-64 (Excellent compatibility)\n";
                break;
            case CpuArchitecture::X86_32:
                std::cout << "x86-32 (Good compatibility, limited performance)\n";
                break;
            case CpuArchitecture::ARM_64:
                std::cout << "ARM64 (Good compatibility, excellent efficiency)\n";
                break;
            case CpuArchitecture::ARM_32:
                std::cout << "ARM32 (Limited compatibility)\n";
                break;
            default:
                std::cout << "Unknown/Specialized architecture\n";
                break;
        }
        
        // Feature compatibility matrix
        std::cout << "\n📊 Feature Compatibility Matrix:\n";
        std::cout << "   • SIMD Support: " << (cpu_info.simd_caps.get_performance_score() > 1.0f ? "✅ Yes" : "❌ Limited") << "\n";
        std::cout << "   • Multi-threading: ✅ Yes (" << cpu_info.topology.logical_cores << " threads)\n";
        std::cout << "   • 64-bit Support: " << (cpu_info.supports_64bit() ? "✅ Yes" : "❌ No") << "\n";
        std::cout << "   • Large Pages: " << (detector_.supports_large_pages() ? "✅ Yes" : "❓ Unknown") << "\n";
        std::cout << "   • NUMA Awareness: " << (detector_.supports_numa() ? "✅ Yes" : "❌ No") << "\n";
        
        std::cout << "\n💡 Cross-Platform Design Principles:\n";
        std::cout << "   • Always provide scalar fallbacks for SIMD operations\n";
        std::cout << "   • Use runtime detection rather than compile-time assumptions\n";
        std::cout << "   • Design for the lowest common denominator, optimize upward\n";
        std::cout << "   • Test on multiple architectures and platforms\n";
        std::cout << "   • Consider mobile and embedded constraints\n";
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
    
    void demonstrate_optimization_recommendations() {
        log::info("--- Hardware-Specific Optimization Recommendations ---");
        
        std::cout << "\n🚀 Optimization Recommendations for Your System:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        const auto& cpu_info = detector_.get_cpu_info();
        const auto& memory_info = detector_.get_memory_info();
        
        // CPU Optimizations
        std::cout << "🖥️  CPU Optimizations:\n";
        if (cpu_info.simd_caps.avx512f) {
            std::cout << "   • ✅ Use AVX-512 for vectorized operations (up to 16x faster)\n";
            std::cout << "   • ⚠️  Monitor thermal throttling with heavy AVX-512 usage\n";
        } else if (cpu_info.simd_caps.avx2) {
            std::cout << "   • ✅ Use AVX2 for vectorized operations (up to 8x faster)\n";
        } else if (cpu_info.simd_caps.sse4_1) {
            std::cout << "   • ✅ Use SSE4.1 for vectorized operations (up to 4x faster)\n";
        }
        
        if (cpu_info.topology.logical_cores > 8) {
            std::cout << "   • ✅ Parallelize workloads across " << cpu_info.topology.logical_cores << " threads\n";
        } else if (cpu_info.topology.logical_cores > 2) {
            std::cout << "   • ✅ Use " << (cpu_info.topology.logical_cores - 1) << " worker threads (leave 1 for system)\n";
        }
        
        if (cpu_info.topology.hyperthreading_enabled) {
            std::cout << "   • 💡 Hyperthreading may help I/O-bound tasks\n";
        }
        
        // Memory Optimizations
        std::cout << "\n💾 Memory Optimizations:\n";
        if (memory_info.numa_available) {
            std::cout << "   • ✅ Use NUMA-aware memory allocation\n";
            std::cout << "   • ✅ Bind threads to NUMA nodes for better locality\n";
        }
        
        if (memory_info.supports_large_pages) {
            std::cout << "   • ✅ Enable large pages for reduced TLB misses\n";
        }
        
        f64 memory_gb = memory_info.total_physical_memory_bytes / (1024.0 * 1024.0 * 1024.0);
        if (memory_gb < 8.0) {
            std::cout << "   • ⚠️  Limited memory (" << std::fixed << std::setprecision(1) 
                      << memory_gb << " GB) - optimize for memory efficiency\n";
        } else if (memory_gb > 32.0) {
            std::cout << "   • ✅ Abundant memory (" << std::fixed << std::setprecision(0) 
                      << memory_gb << " GB) - can use memory-intensive optimizations\n";
        }
        
        // Compiler Optimizations
        std::cout << "\n🛠️  Recommended Compiler Flags:\n";
        auto compiler_flags = detector_.get_recommended_compiler_flags();
        for (const auto& flag : compiler_flags) {
            std::cout << "   • " << flag << "\n";
        }
        
        // Runtime Optimizations
        std::cout << "\n⚡ Runtime Optimizations:\n";
        auto runtime_opts = detector_.get_recommended_runtime_optimizations();
        for (const auto& opt : runtime_opts) {
            std::cout << "   • " << opt << "\n";
        }
        
        std::cout << "\n🎯 Priority Recommendations:\n";
        f32 perf_score = cpu_info.get_overall_performance_score();
        if (perf_score < 5.0f) {
            std::cout << "   • 🔴 Focus on algorithmic optimizations first\n";
            std::cout << "   • 🔴 Minimize memory allocations\n";
            std::cout << "   • 🔴 Use cache-friendly data structures\n";
        } else if (perf_score < 8.0f) {
            std::cout << "   • 🟡 Implement SIMD optimizations for hot paths\n";
            std::cout << "   • 🟡 Optimize memory access patterns\n";
            std::cout << "   • 🟡 Consider multi-threading for CPU-bound tasks\n";
        } else {
            std::cout << "   • 🟢 High-performance system - focus on advanced optimizations\n";
            std::cout << "   • 🟢 Implement GPU compute for parallel workloads\n";
            std::cout << "   • 🟢 Use lock-free data structures\n";
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
};

/**
 * @brief Interactive benchmarking demonstration
 */
class InteractiveBenchmarkDemo {
private:
    BenchmarkExecutor& executor_;
    HardwareDetector& detector_;
    
public:
    InteractiveBenchmarkDemo(BenchmarkExecutor& executor, HardwareDetector& detector)
        : executor_(executor), detector_(detector) {}
    
    void run_interactive_benchmarks() {
        log::info("=== Interactive Hardware Benchmarking ===\n");
        
        std::cout << "🏃 Running comprehensive hardware benchmarks...\n";
        std::cout << "This may take a few minutes. Please wait.\n\n";
        
        // Register all standard benchmarks
        executor_.register_all_standard_benchmarks();
        
        // Run CPU benchmarks
        auto cpu_results = run_cpu_benchmark_suite();
        display_cpu_results(cpu_results);
        
        // Run memory benchmarks
        auto memory_results = run_memory_benchmark_suite();
        display_memory_results(memory_results);
        
        // Run comparative analysis
        perform_comparative_analysis(cpu_results, memory_results);
        
        std::cout << "✅ Benchmarking complete!\n\n";
    }
    
private:
    std::vector<BenchmarkResult> run_cpu_benchmark_suite() {
        std::vector<std::string> cpu_benchmarks = {
            "integer_arithmetic",
            "floating_point",
            "simd_sse2",
            "simd_avx2",
            "branch_prediction"
        };
        
        std::vector<BenchmarkResult> results;
        
        for (const auto& benchmark_name : cpu_benchmarks) {
            try {
                auto result = executor_.run_benchmark(benchmark_name);
                results.push_back(result);
            } catch (const std::exception& e) {
                log::warn("Benchmark '{}' failed: {}", benchmark_name, e.what());
            }
        }
        
        return results;
    }
    
    std::vector<BenchmarkResult> run_memory_benchmark_suite() {
        std::vector<std::string> memory_benchmarks = {
            "memory_bandwidth_sequential",
            "memory_bandwidth_random",
            "memory_latency",
            "cache_hierarchy"
        };
        
        std::vector<BenchmarkResult> results;
        
        for (const auto& benchmark_name : memory_benchmarks) {
            try {
                auto result = executor_.run_benchmark(benchmark_name);
                results.push_back(result);
            } catch (const std::exception& e) {
                log::warn("Benchmark '{}' failed: {}", benchmark_name, e.what());
            }
        }
        
        return results;
    }
    
    void display_cpu_results(const std::vector<BenchmarkResult>& results) {
        std::cout << "🖥️  CPU Benchmark Results:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        for (const auto& result : results) {
            std::cout << "📊 " << std::setw(25) << result.benchmark_name << ": "
                      << std::setw(8) << std::fixed << std::setprecision(2)
                      << result.timing_stats.mean / 1000000.0 << " ms "
                      << "(Score: " << std::setprecision(1) << result.calculate_performance_score() << ")\n";
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
    
    void display_memory_results(const std::vector<BenchmarkResult>& results) {
        std::cout << "💾 Memory Benchmark Results:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        for (const auto& result : results) {
            if (result.throughput_mbps > 0.0) {
                std::cout << "📊 " << std::setw(25) << result.benchmark_name << ": "
                          << std::setw(8) << std::fixed << std::setprecision(1)
                          << result.throughput_mbps << " MB/s "
                          << "(Score: " << std::setprecision(1) << result.calculate_performance_score() << ")\n";
            } else {
                std::cout << "📊 " << std::setw(25) << result.benchmark_name << ": "
                          << std::setw(8) << std::fixed << std::setprecision(2)
                          << result.timing_stats.mean / 1000000.0 << " ms "
                          << "(Score: " << std::setprecision(1) << result.calculate_performance_score() << ")\n";
            }
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
    
    void perform_comparative_analysis(const std::vector<BenchmarkResult>& cpu_results,
                                     const std::vector<BenchmarkResult>& memory_results) {
        std::cout << "🔍 Performance Analysis & Recommendations:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // Calculate overall scores
        f64 cpu_score = 0.0;
        f64 memory_score = 0.0;
        
        for (const auto& result : cpu_results) {
            cpu_score += result.calculate_performance_score();
        }
        cpu_score /= cpu_results.size();
        
        for (const auto& result : memory_results) {
            memory_score += result.calculate_performance_score();
        }
        memory_score /= memory_results.size();
        
        std::cout << "🎯 Overall CPU Score:    " << std::fixed << std::setprecision(1) << cpu_score << "/10\n";
        std::cout << "🎯 Overall Memory Score: " << memory_score << "/10\n";
        
        // Performance characteristics
        std::cout << "\n📈 Performance Characteristics:\n";
        if (cpu_score > 7.0) {
            std::cout << "   ✅ High CPU Performance - Suitable for compute-intensive tasks\n";
        } else if (cpu_score > 5.0) {
            std::cout << "   🟡 Moderate CPU Performance - Good for general-purpose computing\n";
        } else {
            std::cout << "   🔴 Limited CPU Performance - Focus on algorithmic optimizations\n";
        }
        
        if (memory_score > 7.0) {
            std::cout << "   ✅ High Memory Performance - Can use memory-intensive algorithms\n";
        } else if (memory_score > 5.0) {
            std::cout << "   🟡 Moderate Memory Performance - Be mindful of memory access patterns\n";
        } else {
            std::cout << "   🔴 Limited Memory Performance - Cache optimization is critical\n";
        }
        
        // Bottleneck analysis
        std::cout << "\n🔍 Bottleneck Analysis:\n";
        if (cpu_score > memory_score + 2.0) {
            std::cout << "   • Memory bandwidth is likely the primary bottleneck\n";
            std::cout << "   • Focus on cache-friendly algorithms and data structures\n";
            std::cout << "   • Consider memory prefetching and NUMA optimizations\n";
        } else if (memory_score > cpu_score + 2.0) {
            std::cout << "   • CPU compute capacity is the primary bottleneck\n";
            std::cout << "   • Focus on SIMD optimizations and parallelization\n";
            std::cout << "   • Consider offloading to GPU for suitable workloads\n";
        } else {
            std::cout << "   • Balanced system - no obvious bottleneck\n";
            std::cout << "   • Optimize both CPU and memory usage for best results\n";
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
};

//=============================================================================
// Main Educational Demo Function
//=============================================================================

int main() {
    try {
        log::info("Starting ECScope Hardware Analysis Educational Demo");
        
        // Initialize hardware detection
        auto& detector = get_hardware_detector();
        
        // Initialize benchmarking system
        initialize_benchmark_system();
        auto& executor = get_benchmark_executor();
        
        // Run hardware analysis demonstration
        HardwareAnalysisDemo analysis_demo(detector);
        analysis_demo.run_complete_demonstration();
        
        // Ask user if they want to run benchmarks
        std::cout << "Would you like to run comprehensive benchmarks? (y/n): ";
        std::string response;
        std::getline(std::cin, response);
        
        if (!response.empty() && (response[0] == 'y' || response[0] == 'Y')) {
            InteractiveBenchmarkDemo benchmark_demo(executor, detector);
            benchmark_demo.run_interactive_benchmarks();
        }
        
        // Integration demonstration (if system integration is available)
        try {
            auto& integration_manager = get_system_integration_manager();
            
            std::cout << "\n🔗 ECScope System Integration Status:\n";
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            std::cout << integration_manager.generate_system_report();
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
            
        } catch (const std::exception& e) {
            log::info("System integration not available: {}", e.what());
        }
        
        // Final summary
        std::cout << "🎓 Educational Summary:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "• Hardware-aware programming can provide significant performance benefits\n";
        std::cout << "• Runtime detection enables adaptive optimization across platforms\n";
        std::cout << "• Understanding your hardware characteristics is key to optimization\n";
        std::cout << "• Modern systems are complex - thermal and power management matter\n";
        std::cout << "• Benchmarking validates optimization effectiveness\n";
        std::cout << "• ECScope provides comprehensive hardware analysis for game engines\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        std::cout << "\n✨ Thank you for exploring ECScope's Hardware Analysis System! ✨\n";
        
    } catch (const std::exception& e) {
        log::error("Demo failed: {}", e.what());
        return 1;
    }
    
    return 0;
}