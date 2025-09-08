/**
 * @file simple_profiler.hpp
 * @brief Simple Performance Profiler for ECScope
 * 
 * Provides basic performance monitoring with:
 * - Frame time tracking
 * - Function timing with RAII
 * - Memory usage monitoring
 * - Simple statistics
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#pragma once

#include "core/types.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>

namespace ecscope::profiler {

/**
 * @brief Performance metrics for a specific operation
 */
struct PerformanceMetrics {
    std::atomic<u64> call_count{0};
    std::atomic<f64> total_time_ms{0.0};
    std::atomic<f64> min_time_ms{999999.0};
    std::atomic<f64> max_time_ms{0.0};
    
    f64 average_time_ms() const {
        u64 calls = call_count.load();
        return calls > 0 ? total_time_ms.load() / calls : 0.0;
    }
    
    void add_sample(f64 time_ms) {
        call_count.fetch_add(1);
        total_time_ms.fetch_add(time_ms);
        
        // Update min/max (not perfectly thread-safe but close enough)
        f64 current_min = min_time_ms.load();
        while (time_ms < current_min && !min_time_ms.compare_exchange_weak(current_min, time_ms));
        
        f64 current_max = max_time_ms.load();
        while (time_ms > current_max && !max_time_ms.compare_exchange_weak(current_max, time_ms));
    }
    
    void reset() {
        call_count.store(0);
        total_time_ms.store(0.0);
        min_time_ms.store(999999.0);
        max_time_ms.store(0.0);
    }
};

/**
 * @brief Simple performance profiler
 */
class SimpleProfiler {
public:
    static SimpleProfiler& instance() {
        static SimpleProfiler instance_;
        return instance_;
    }
    
    /**
     * @brief Begin timing a section
     */
    void begin_section(const std::string& name);
    
    /**
     * @brief End timing a section
     */
    void end_section(const std::string& name);
    
    /**
     * @brief Get metrics for a section
     */
    const PerformanceMetrics& get_metrics(const std::string& name) const;
    
    /**
     * @brief Get all metric names
     */
    std::vector<std::string> get_section_names() const;
    
    /**
     * @brief Reset all metrics
     */
    void reset_all();
    
    /**
     * @brief Print performance report to console
     */
    void print_report() const;
    
    /**
     * @brief Get formatted performance report
     */
    std::string get_report() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PerformanceMetrics> metrics_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> start_times_;
    
    SimpleProfiler() = default;
    
    static PerformanceMetrics empty_metrics_;
};

/**
 * @brief RAII timer for automatic section timing
 */
class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& section_name)
        : section_name_(section_name)
        , start_time_(std::chrono::high_resolution_clock::now()) {
    }
    
    ~ScopedTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time_).count();
        f64 time_ms = duration / 1000.0;
        
        SimpleProfiler::instance().get_metrics(section_name_).add_sample(time_ms);
    }

private:
    std::string section_name_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

} // namespace ecscope::profiler

/**
 * @brief Profiling macro for easy use
 */
#define ECSCOPE_PROFILE(name) \
    ecscope::profiler::ScopedTimer _timer(name)

/**
 * @brief Function profiling macro
 */
#define ECSCOPE_PROFILE_FUNCTION() \
    ecscope::profiler::ScopedTimer _timer(__FUNCTION__)