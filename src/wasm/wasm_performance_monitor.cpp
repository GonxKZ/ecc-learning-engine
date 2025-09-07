// =============================================================================
// ECScope WebAssembly Performance Monitor Implementation
// =============================================================================

#include "wasm_performance_monitor.hpp"
#include <emscripten.h>
#include <algorithm>
#include <numeric>

namespace ecscope::wasm {

// =============================================================================
// ADVANCED PERFORMANCE MONITORING
// =============================================================================

void WasmPerformanceMonitor::reportDetailedPerformanceStats() {
    if (frame_times_.empty()) {
        return;
    }
    
    // Calculate advanced statistics
    double sum = std::accumulate(frame_times_.begin(), frame_times_.end(), 0.0);
    double mean = sum / frame_times_.size();
    
    // Calculate standard deviation
    double sq_sum = std::inner_product(frame_times_.begin(), frame_times_.end(), 
                                       frame_times_.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / frame_times_.size() - mean * mean);
    
    // Calculate percentiles
    auto sorted_times = frame_times_;
    std::sort(sorted_times.begin(), sorted_times.end());
    
    size_t p50_idx = sorted_times.size() / 2;
    size_t p95_idx = static_cast<size_t>(sorted_times.size() * 0.95);
    size_t p99_idx = static_cast<size_t>(sorted_times.size() * 0.99);
    
    double p50 = sorted_times[p50_idx] / 1000.0;  // Convert to ms
    double p95 = sorted_times[p95_idx] / 1000.0;
    double p99 = sorted_times[p99_idx] / 1000.0;
    
    EM_ASM({
        const stats = {
            frameCount: $0,
            averageFrameTime: $1,
            standardDeviation: $2,
            percentiles: {
                p50: $3,
                p95: $4,
                p99: $5
            },
            fps: {
                average: $6,
                min: $7,
                max: $8
            },
            targetPerformance: {
                fps60Rate: $9,
                fps30Rate: $10
            }
        };
        
        console.log('Detailed ECScope Performance Statistics:');
        console.table(stats);
        
        if (window.ECScope && window.ECScope.onDetailedPerformanceReport) {
            window.ECScope.onDetailedPerformanceReport(stats);
        }
    }, 
    frame_times_.size(),
    mean / 1000.0,  // Convert to ms
    stdev / 1000.0,  // Convert to ms
    p50, p95, p99,
    1000000.0 / mean,  // Average FPS
    1000000.0 / *std::max_element(frame_times_.begin(), frame_times_.end()),  // Min FPS
    1000000.0 / *std::min_element(frame_times_.begin(), frame_times_.end()),  // Max FPS
    calculateTargetFrameRate(16666.67),  // 60 FPS target (16.67ms)
    calculateTargetFrameRate(33333.33)   // 30 FPS target (33.33ms)
    );
}

double WasmPerformanceMonitor::calculateTargetFrameRate(double target_time_us) const {
    if (frame_times_.empty()) {
        return 0.0;
    }
    
    int frames_at_target = 0;
    for (auto time : frame_times_) {
        if (time <= target_time_us) {
            frames_at_target++;
        }
    }
    
    return static_cast<double>(frames_at_target) / frame_times_.size();
}

void WasmPerformanceMonitor::beginSubsystemTiming(const std::string& subsystem) {
    SubsystemTiming timing;
    timing.name = subsystem;
    timing.start_time = std::chrono::high_resolution_clock::now();
    
    current_subsystem_timings_.push_back(timing);
}

void WasmPerformanceMonitor::endSubsystemTiming(const std::string& subsystem) {
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Find the matching subsystem timing
    for (auto it = current_subsystem_timings_.rbegin(); it != current_subsystem_timings_.rend(); ++it) {
        if (it->name == subsystem) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - it->start_time).count();
            
            // Store the timing
            subsystem_timings_[subsystem].push_back(duration);
            
            // Limit stored results
            auto& results = subsystem_timings_[subsystem];
            if (results.size() > config_.max_timing_samples) {
                results.erase(results.begin());
            }
            
            // Remove from current timings
            current_subsystem_timings_.erase((it + 1).base());
            break;
        }
    }
}

val WasmPerformanceMonitor::getSubsystemStatistics() const {
    val subsystems = val::object();
    
    for (const auto& pair : subsystem_timings_) {
        const std::string& name = pair.first;
        const std::vector<int64_t>& times = pair.second;
        
        if (!times.empty()) {
            double sum = 0.0;
            int64_t min_time = times[0];
            int64_t max_time = times[0];
            
            for (auto time : times) {
                sum += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }
            
            val stats = val::object();
            stats.set("averageTime", (sum / times.size()) / 1000.0);  // Convert to ms
            stats.set("minTime", min_time / 1000.0);  // Convert to ms
            stats.set("maxTime", max_time / 1000.0);  // Convert to ms
            stats.set("sampleCount", times.size());
            
            subsystems.set(name, stats);
        }
    }
    
    return subsystems;
}

// =============================================================================
// WEB-SPECIFIC PERFORMANCE MONITORING
// =============================================================================

void WasmPerformanceMonitor::enableWebPerformanceAPI() {
    EM_ASM({
        // Use the Performance API if available
        if (window.performance && window.performance.mark) {
            window.ECScope = window.ECScope || {};
            window.ECScope.usePerformanceAPI = true;
            
            // Create custom performance marks for ECScope
            window.ECScope.markFrame = function() {
                window.performance.mark('ecscope-frame');
            };
            
            window.ECScope.markSubsystem = function(name) {
                window.performance.mark('ecscope-' + name + '-start');
            };
            
            window.ECScope.measureSubsystem = function(name) {
                window.performance.mark('ecscope-' + name + '-end');
                window.performance.measure('ecscope-' + name, 
                    'ecscope-' + name + '-start', 
                    'ecscope-' + name + '-end');
            };
            
            console.log('ECScope Performance API integration enabled');
        }
    });
}

void WasmPerformanceMonitor::reportToWebPerformanceAPI(const std::string& name, double duration) {
    EM_ASM({
        if (window.ECScope && window.ECScope.usePerformanceAPI) {
            // Report custom timing to browser's performance timeline
            if (window.performance && window.performance.measure) {
                // This would require the Performance API to support custom measures
                // For now, we'll just log it
                console.debug('ECScope Performance: ' + UTF8ToString($0) + ' took ' + $1 + 'ms');
            }
        }
    }, name.c_str(), duration);
}

void WasmPerformanceMonitor::checkPerformanceBudget() {
    if (frame_times_.empty()) {
        return;
    }
    
    double current_fps = getCurrentFPS();
    double target_fps = config_.target_fps;
    
    bool budget_exceeded = current_fps < target_fps * 0.8;  // 80% of target
    
    EM_ASM({
        const budgetInfo = {
            currentFPS: $0,
            targetFPS: $1,
            budgetExceeded: $2,
            performanceRatio: $3
        };
        
        if (budgetInfo.budgetExceeded) {
            console.warn('ECScope Performance Budget Exceeded:', budgetInfo);
        }
        
        if (window.ECScope && window.ECScope.onPerformanceBudgetCheck) {
            window.ECScope.onPerformanceBudgetCheck(budgetInfo);
        }
    }, current_fps, target_fps, budget_exceeded, current_fps / target_fps);
}

void WasmPerformanceMonitor::profileMemoryAllocations() {
    EM_ASM({
        // Use browser's memory profiling if available
        if (window.performance && window.performance.memory) {
            const memInfo = {
                usedJSHeapSize: window.performance.memory.usedJSHeapSize,
                totalJSHeapSize: window.performance.memory.totalJSHeapSize,
                jsHeapSizeLimit: window.performance.memory.jsHeapSizeLimit
            };
            
            if (window.ECScope && window.ECScope.onMemoryProfileUpdate) {
                window.ECScope.onMemoryProfileUpdate(memInfo);
            }
        }
    });
}

}  // namespace ecscope::wasm