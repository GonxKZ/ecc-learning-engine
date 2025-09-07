// =============================================================================
// ECScope WebAssembly Performance Monitor Header
// =============================================================================

#pragma once

#include "wasm_core.hpp"
#include <vector>
#include <string>
#include <chrono>
#include <emscripten/val.h>

namespace ecscope::wasm {

// =============================================================================
// SUBSYSTEM TIMING STRUCTURE
// =============================================================================

struct SubsystemTiming {
    std::string name;
    std::chrono::high_resolution_clock::time_point start_time;
};

// Extended WasmPerformanceMonitor class with additional methods
class WasmPerformanceMonitor {
    // ... (existing members from wasm_core.hpp)
    
private:
    std::vector<SubsystemTiming> current_subsystem_timings_;
    std::unordered_map<std::string, std::vector<int64_t>> subsystem_timings_;
    
public:
    // Advanced performance monitoring
    void reportDetailedPerformanceStats();
    double calculateTargetFrameRate(double target_time_us) const;
    
    // Subsystem timing
    void beginSubsystemTiming(const std::string& subsystem);
    void endSubsystemTiming(const std::string& subsystem);
    emscripten::val getSubsystemStatistics() const;
    
    // Web-specific performance monitoring
    void enableWebPerformanceAPI();
    void reportToWebPerformanceAPI(const std::string& name, double duration);
    void checkPerformanceBudget();
    void profileMemoryAllocations();
    
    // Utility classes for RAII timing
    class ScopedSubsystemTimer {
    private:
        std::string name_;
        WasmPerformanceMonitor* monitor_;
        
    public:
        ScopedSubsystemTimer(const std::string& name, WasmPerformanceMonitor* monitor) 
            : name_(name), monitor_(monitor) {
            monitor_->beginSubsystemTiming(name_);
        }
        
        ~ScopedSubsystemTimer() {
            monitor_->endSubsystemTiming(name_);
        }
    };
};

// Convenience macro for subsystem timing
#define WASM_PROFILE_SUBSYSTEM(monitor, name) \
    WasmPerformanceMonitor::ScopedSubsystemTimer timer(name, &(monitor))

}  // namespace ecscope::wasm