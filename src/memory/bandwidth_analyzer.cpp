#include "bandwidth_analyzer.hpp"
#include "core/log.hpp"

namespace ecscope::memory::bandwidth {

// Global bandwidth profiler instance (simplified implementation)
MemoryBandwidthProfiler& get_global_bandwidth_profiler() {
    static auto numa_manager = numa::get_global_numa_manager();
    static auto cache_analyzer = cache::get_global_cache_analyzer();
    static MemoryBandwidthProfiler instance(numa_manager, cache_analyzer);
    return instance;
}

// Global bottleneck detector instance (simplified implementation)
MemoryBottleneckDetector& get_global_bottleneck_detector() {
    static auto& profiler = get_global_bandwidth_profiler();
    static auto numa_manager = numa::get_global_numa_manager();
    static auto cache_analyzer = cache::get_global_cache_analyzer();
    static MemoryBottleneckDetector instance(profiler, numa_manager, cache_analyzer);
    return instance;
}

} // namespace ecscope::memory::bandwidth