#include "ecscope/ecs_profiler.hpp"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <unordered_set>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #include <dbghelp.h>
    #pragma comment(lib, "psapi.lib")
    #pragma comment(lib, "dbghelp.lib")
#elif defined(__linux__)
    #include <sys/resource.h>
    #include <unistd.h>
    #include <execinfo.h>
    #include <sys/syscall.h>
#elif defined(__APPLE__)
    #include <sys/resource.h>
    #include <unistd.h>
    #include <execinfo.h>
    #include <mach/mach.h>
#endif

namespace ecscope::profiling {

// Static instance for singleton pattern
static std::unique_ptr<ECSProfiler> g_profiler_instance;
static std::mutex g_instance_mutex;

// Thread-local random number generator for sampling
thread_local std::random_device g_rd;
thread_local std::mt19937 g_gen(g_rd());
thread_local std::uniform_real_distribution<f32> g_dist(0.0f, 1.0f);

// ===== ProfileScope Implementation =====

ProfileScope::ProfileScope(const std::string& name, ProfileCategory category)
    : start_time_(ProfilerClock::now())
    , name_(name)
    , category_(category)
    , callback_(nullptr) {
}

ProfileScope::ProfileScope(const std::string& name, ProfileCategory category,
                         std::function<void(const ProfileEvent&)> callback)
    : start_time_(ProfilerClock::now())
    , name_(name)
    , category_(category)
    , callback_(std::move(callback)) {
}

ProfileScope::~ProfileScope() {
    if (!ECSProfiler::instance().is_enabled()) {
        return;
    }
    
    if (!ECSProfiler::instance().is_category_enabled(category_)) {
        return;
    }
    
    // Check sampling rate
    if (g_dist(g_gen) > ECSProfiler::instance().get_sampling_rate()) {
        return;
    }
    
    auto end_time = ProfilerClock::now();
    auto duration = duration_cast<ProfilerDuration>(end_time - start_time_);
    
    ProfileEvent event{
        .category = category_,
        .name = name_,
        .start_time = start_time_,
        .duration = duration,
        .thread_id = static_cast<u32>(std::hash<std::thread::id>{}(std::this_thread::get_id())),
        .memory_used = 0, // Will be filled if memory tracking is enabled
        .entity_count = 0,
        .component_count = 0,
        .additional_data = ""
    };
    
    if (callback_) {
        callback_(event);
    }
    
    ECSProfiler::instance().record_event_internal(event);
}

// ===== ECSProfiler Implementation =====

ECSProfiler::ECSProfiler() {
    // Enable all categories by default
    enabled_categories_ = {
        ProfileCategory::ENTITY_CREATION,
        ProfileCategory::ENTITY_DESTRUCTION,
        ProfileCategory::COMPONENT_ADD,
        ProfileCategory::COMPONENT_REMOVE,
        ProfileCategory::COMPONENT_ACCESS,
        ProfileCategory::SYSTEM_EXECUTION,
        ProfileCategory::MEMORY_ALLOCATION,
        ProfileCategory::MEMORY_DEALLOCATION,
        ProfileCategory::ARCHETYPE_CHANGE,
        ProfileCategory::QUERY_EXECUTION,
        ProfileCategory::EVENT_PROCESSING,
        ProfileCategory::SERIALIZATION,
        ProfileCategory::DESERIALIZATION,
        ProfileCategory::THREADING_OVERHEAD,
        ProfileCategory::CACHE_MISS,
        ProfileCategory::CUSTOM
    };
    
    // Pre-allocate event storage
    events_.reserve(max_events_);
    allocations_.reserve(max_events_ / 10); // Fewer allocations typically
}

ECSProfiler::~ECSProfiler() = default;

void ECSProfiler::enable_category(ProfileCategory category) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    enabled_categories_.insert(category);
}

void ECSProfiler::disable_category(ProfileCategory category) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    enabled_categories_.erase(category);
}

bool ECSProfiler::is_category_enabled(ProfileCategory category) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return enabled_categories_.find(category) != enabled_categories_.end();
}

void ECSProfiler::begin_system(const std::string& system_name) {
    if (!enabled_) return;
    
    // Store start time for this system (thread-local would be better for MT)
    thread_local std::unordered_map<std::string, ProfilerTimepoint> system_start_times;
    system_start_times[system_name] = ProfilerClock::now();
}

void ECSProfiler::end_system(const std::string& system_name, usize memory_usage) {
    if (!enabled_) return;
    
    thread_local std::unordered_map<std::string, ProfilerTimepoint> system_start_times;
    auto it = system_start_times.find(system_name);
    if (it == system_start_times.end()) {
        return; // begin_system was not called
    }
    
    auto end_time = ProfilerClock::now();
    auto duration = duration_cast<ProfilerDuration>(end_time - it->second);
    
    // Update system metrics
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        system_metrics_[system_name].update(duration, memory_usage);
        
        // Check for performance warnings
        if (duration > slow_system_threshold_) {
            // System is running slow - could log warning
        }
    }
    
    // Record event
    ProfileEvent event{
        .category = ProfileCategory::SYSTEM_EXECUTION,
        .name = system_name,
        .start_time = it->second,
        .duration = duration,
        .thread_id = get_current_thread_id(),
        .memory_used = memory_usage,
        .entity_count = entity_stats_.active_entities,
        .component_count = 0,
        .additional_data = ""
    };
    
    record_event_internal(event);
    system_start_times.erase(it);
}

void ECSProfiler::record_entity_created(ProfilerDuration creation_time) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    entity_stats_.entity_created(creation_time);
    
    ProfileEvent event{
        .category = ProfileCategory::ENTITY_CREATION,
        .name = "entity_created",
        .start_time = ProfilerClock::now() - creation_time,
        .duration = creation_time,
        .thread_id = get_current_thread_id(),
        .memory_used = 0,
        .entity_count = entity_stats_.active_entities,
        .component_count = 0,
        .additional_data = ""
    };
    
    record_event_internal(event);
}

void ECSProfiler::record_entity_destroyed(ProfilerDuration destruction_time) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    entity_stats_.entity_destroyed(destruction_time);
    
    ProfileEvent event{
        .category = ProfileCategory::ENTITY_DESTRUCTION,
        .name = "entity_destroyed",
        .start_time = ProfilerClock::now() - destruction_time,
        .duration = destruction_time,
        .thread_id = get_current_thread_id(),
        .memory_used = 0,
        .entity_count = entity_stats_.active_entities,
        .component_count = 0,
        .additional_data = ""
    };
    
    record_event_internal(event);
}

void ECSProfiler::record_component_access(core::ComponentID component_id,
                                        const std::string& component_name,
                                        ProfilerDuration access_time) {
    if (!enabled_) return;
    if (!is_category_enabled(ProfileCategory::COMPONENT_ACCESS)) return;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto& stats = component_stats_[component_id];
    if (stats.component_name.empty()) {
        stats.component_id = component_id;
        stats.component_name = component_name;
    }
    stats.update_access(access_time);
    
    ProfileEvent event{
        .category = ProfileCategory::COMPONENT_ACCESS,
        .name = component_name + "_access",
        .start_time = ProfilerClock::now() - access_time,
        .duration = access_time,
        .thread_id = get_current_thread_id(),
        .memory_used = 0,
        .entity_count = 0,
        .component_count = stats.instance_count,
        .additional_data = component_name
    };
    
    record_event_internal(event);
}

void ECSProfiler::record_memory_allocation(usize size, usize alignment, const std::string& category) {
    if (!enabled_ || !memory_tracking_) return;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    memory_stats_.allocate(size, category);
    
    AllocationInfo allocation{
        .size = size,
        .alignment = alignment,
        .timestamp = ProfilerClock::now(),
        .category = category,
        .ptr = nullptr, // Could be filled by memory allocator
        .stack_trace = get_stack_trace(),
        .thread_id = get_current_thread_id()
    };
    
    record_allocation_internal(allocation);
    
    // Check for memory warnings
    if (memory_stats_.current_usage > high_memory_threshold_) {
        // High memory usage - could log warning
    }
}

void ECSProfiler::record_memory_deallocation(usize size, const std::string& category) {
    if (!enabled_ || !memory_tracking_) return;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    memory_stats_.deallocate(size, category);
}

void ECSProfiler::record_custom_event(const std::string& name, ProfilerDuration duration,
                                     const std::string& data) {
    if (!enabled_) return;
    if (!is_category_enabled(ProfileCategory::CUSTOM)) return;
    
    ProfileEvent event{
        .category = ProfileCategory::CUSTOM,
        .name = name,
        .start_time = ProfilerClock::now() - duration,
        .duration = duration,
        .thread_id = get_current_thread_id(),
        .memory_used = 0,
        .entity_count = 0,
        .component_count = 0,
        .additional_data = data
    };
    
    record_event_internal(event);
}

void ECSProfiler::record_event_internal(const ProfileEvent& event) {
    // Thread-safe circular buffer insertion
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    if (events_.size() < max_events_) {
        events_.push_back(event);
    } else {
        events_[event_index_ % max_events_] = event;
        ++event_index_;
    }
}

void ECSProfiler::record_allocation_internal(const AllocationInfo& allocation) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    if (allocations_.size() < max_events_ / 10) {
        allocations_.push_back(allocation);
    } else {
        allocations_[(event_index_ / 10) % (max_events_ / 10)] = allocation;
    }
}

std::string ECSProfiler::get_stack_trace(int max_frames) const {
#ifdef _WIN32
    void* stack[32];
    WORD frames = CaptureStackBackTrace(0, std::min(max_frames, 32), stack, nullptr);
    
    std::ostringstream oss;
    for (int i = 0; i < frames; ++i) {
        oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(stack[i]);
        if (i < frames - 1) oss << " -> ";
    }
    return oss.str();
    
#elif defined(__linux__) || defined(__APPLE__)
    void* buffer[32];
    int frames = backtrace(buffer, std::min(max_frames, 32));
    char** symbols = backtrace_symbols(buffer, frames);
    
    std::ostringstream oss;
    for (int i = 0; i < frames; ++i) {
        oss << symbols[i];
        if (i < frames - 1) oss << " -> ";
    }
    
    free(symbols);
    return oss.str();
#else
    return "Stack trace not available on this platform";
#endif
}

u32 ECSProfiler::get_current_thread_id() const {
#ifdef _WIN32
    return static_cast<u32>(GetCurrentThreadId());
#elif defined(__linux__)
    return static_cast<u32>(syscall(SYS_gettid));
#else
    return static_cast<u32>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
}

// Query methods
SystemMetrics ECSProfiler::get_system_metrics(const std::string& system_name) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = system_metrics_.find(system_name);
    return (it != system_metrics_.end()) ? it->second : SystemMetrics{};
}

ComponentStats ECSProfiler::get_component_stats(core::ComponentID component_id) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = component_stats_.find(component_id);
    return (it != component_stats_.end()) ? it->second : ComponentStats{};
}

EntityStats ECSProfiler::get_entity_stats() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return entity_stats_;
}

MemoryStats ECSProfiler::get_memory_stats() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return memory_stats_;
}

std::vector<SystemMetrics> ECSProfiler::get_all_system_metrics() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<SystemMetrics> result;
    result.reserve(system_metrics_.size());
    
    for (const auto& [name, metrics] : system_metrics_) {
        result.push_back(metrics);
    }
    
    return result;
}

std::vector<ComponentStats> ECSProfiler::get_all_component_stats() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<ComponentStats> result;
    result.reserve(component_stats_.size());
    
    for (const auto& [id, stats] : component_stats_) {
        result.push_back(stats);
    }
    
    return result;
}

std::vector<ProfileEvent> ECSProfiler::get_recent_events(usize count) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    std::vector<ProfileEvent> result;
    usize actual_count = std::min(count, events_.size());
    result.reserve(actual_count);
    
    if (events_.size() < max_events_) {
        // Linear case - not wrapped around yet
        usize start_idx = (events_.size() > actual_count) ? events_.size() - actual_count : 0;
        for (usize i = start_idx; i < events_.size(); ++i) {
            result.push_back(events_[i]);
        }
    } else {
        // Circular case - wrapped around
        usize start_idx = (event_index_ > actual_count) ? event_index_ - actual_count : 0;
        for (usize i = 0; i < actual_count; ++i) {
            result.push_back(events_[(start_idx + i) % max_events_]);
        }
    }
    
    return result;
}

std::vector<std::string> ECSProfiler::detect_performance_issues() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<std::string> issues;
    
    // Check for slow systems
    for (const auto& [name, metrics] : system_metrics_) {
        if (metrics.max_time > slow_system_threshold_) {
            std::ostringstream oss;
            oss << "System '" << name << "' exceeded time threshold: "
                << metrics.max_time.count() << "μs (max), "
                << metrics.avg_time.count() << "μs (avg)";
            issues.push_back(oss.str());
        }
        
        // Check for high variance in execution times
        if (!metrics.recent_times.empty()) {
            auto avg = metrics.avg_time.count();
            f64 variance = 0.0;
            for (const auto& time : metrics.recent_times) {
                f64 diff = static_cast<f64>(time.count()) - avg;
                variance += diff * diff;
            }
            variance /= metrics.recent_times.size();
            f64 stddev = std::sqrt(variance);
            
            if (stddev > avg * 0.5) { // High variance
                std::ostringstream oss;
                oss << "System '" << name << "' has inconsistent performance (stddev: "
                    << stddev << "μs, avg: " << avg << "μs)";
                issues.push_back(oss.str());
            }
        }
    }
    
    // Check for memory issues
    if (memory_stats_.current_usage > high_memory_threshold_) {
        std::ostringstream oss;
        oss << "High memory usage: " << (memory_stats_.current_usage / (1024 * 1024))
            << " MB (threshold: " << (high_memory_threshold_ / (1024 * 1024)) << " MB)";
        issues.push_back(oss.str());
    }
    
    // Check for memory leaks (more allocations than deallocations)
    if (memory_stats_.allocation_count > memory_stats_.deallocation_count + 1000) {
        std::ostringstream oss;
        oss << "Potential memory leak detected: "
            << (memory_stats_.allocation_count - memory_stats_.deallocation_count)
            << " unfreed allocations";
        issues.push_back(oss.str());
    }
    
    return issues;
}

f64 ECSProfiler::calculate_overall_performance_score() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    f64 score = 100.0; // Start with perfect score
    
    // Penalize slow systems
    for (const auto& [name, metrics] : system_metrics_) {
        f64 threshold_ratio = static_cast<f64>(metrics.avg_time.count()) / slow_system_threshold_.count();
        if (threshold_ratio > 1.0) {
            score -= (threshold_ratio - 1.0) * 20.0; // Lose 20 points per threshold violation
        }
    }
    
    // Penalize high memory usage
    f64 memory_ratio = static_cast<f64>(memory_stats_.current_usage) / high_memory_threshold_;
    if (memory_ratio > 1.0) {
        score -= (memory_ratio - 1.0) * 30.0; // Lose 30 points for high memory
    }
    
    // Penalize memory leaks
    if (memory_stats_.allocation_count > memory_stats_.deallocation_count) {
        f64 leak_ratio = static_cast<f64>(memory_stats_.allocation_count - memory_stats_.deallocation_count) / 1000.0;
        score -= leak_ratio * 25.0;
    }
    
    return std::max(0.0, std::min(100.0, score));
}

std::string ECSProfiler::generate_performance_report() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::ostringstream report;
    
    report << "=== ECScope Performance Report ===\n\n";
    
    // Overall score
    report << "Overall Performance Score: " << std::fixed << std::setprecision(1)
           << calculate_overall_performance_score() << "/100\n\n";
    
    // System performance
    report << "System Performance:\n";
    report << std::setw(25) << "System" << std::setw(15) << "Avg (μs)" 
           << std::setw(15) << "Min (μs)" << std::setw(15) << "Max (μs)" 
           << std::setw(10) << "Calls" << std::setw(15) << "Memory (KB)\n";
    report << std::string(95, '-') << "\n";
    
    for (const auto& [name, metrics] : system_metrics_) {
        report << std::setw(25) << name
               << std::setw(15) << metrics.avg_time.count()
               << std::setw(15) << metrics.min_time.count()
               << std::setw(15) << metrics.max_time.count()
               << std::setw(10) << metrics.execution_count
               << std::setw(15) << (metrics.memory_average / 1024) << "\n";
    }
    
    // Entity statistics
    report << "\nEntity Statistics:\n";
    report << "  Active Entities: " << entity_stats_.active_entities << "\n";
    report << "  Peak Entities: " << entity_stats_.peak_entities << "\n";
    report << "  Total Created: " << entity_stats_.entities_created << "\n";
    report << "  Total Destroyed: " << entity_stats_.entities_destroyed << "\n";
    report << "  Avg Creation Time: " << entity_stats_.avg_creation_time.count() << " μs\n";
    report << "  Avg Destruction Time: " << entity_stats_.avg_destruction_time.count() << " μs\n";
    
    // Memory statistics
    report << "\nMemory Statistics:\n";
    report << "  Current Usage: " << (memory_stats_.current_usage / (1024 * 1024)) << " MB\n";
    report << "  Peak Usage: " << (memory_stats_.peak_usage / (1024 * 1024)) << " MB\n";
    report << "  Total Allocated: " << (memory_stats_.total_allocated / (1024 * 1024)) << " MB\n";
    report << "  Total Deallocated: " << (memory_stats_.total_deallocated / (1024 * 1024)) << " MB\n";
    report << "  Allocation Count: " << memory_stats_.allocation_count << "\n";
    report << "  Deallocation Count: " << memory_stats_.deallocation_count << "\n";
    
    // Performance issues
    auto issues = detect_performance_issues();
    if (!issues.empty()) {
        report << "\nPerformance Issues:\n";
        for (const auto& issue : issues) {
            report << "  - " << issue << "\n";
        }
    }
    
    return report.str();
}

void ECSProfiler::export_to_json(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::ofstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    file << "{\n";
    file << "  \"performance_score\": " << calculate_overall_performance_score() << ",\n";
    file << "  \"timestamp\": " << duration_cast<seconds>(ProfilerClock::now().time_since_epoch()).count() << ",\n";
    
    // System metrics
    file << "  \"systems\": [\n";
    bool first_system = true;
    for (const auto& [name, metrics] : system_metrics_) {
        if (!first_system) file << ",\n";
        first_system = false;
        
        file << "    {\n";
        file << "      \"name\": \"" << name << "\",\n";
        file << "      \"avg_time_us\": " << metrics.avg_time.count() << ",\n";
        file << "      \"min_time_us\": " << metrics.min_time.count() << ",\n";
        file << "      \"max_time_us\": " << metrics.max_time.count() << ",\n";
        file << "      \"execution_count\": " << metrics.execution_count << ",\n";
        file << "      \"memory_peak\": " << metrics.memory_peak << ",\n";
        file << "      \"memory_average\": " << metrics.memory_average << "\n";
        file << "    }";
    }
    file << "\n  ],\n";
    
    // Memory stats
    file << "  \"memory\": {\n";
    file << "    \"current_usage\": " << memory_stats_.current_usage << ",\n";
    file << "    \"peak_usage\": " << memory_stats_.peak_usage << ",\n";
    file << "    \"total_allocated\": " << memory_stats_.total_allocated << ",\n";
    file << "    \"total_deallocated\": " << memory_stats_.total_deallocated << ",\n";
    file << "    \"allocation_count\": " << memory_stats_.allocation_count << ",\n";
    file << "    \"deallocation_count\": " << memory_stats_.deallocation_count << "\n";
    file << "  },\n";
    
    // Entity stats
    file << "  \"entities\": {\n";
    file << "    \"active\": " << entity_stats_.active_entities << ",\n";
    file << "    \"peak\": " << entity_stats_.peak_entities << ",\n";
    file << "    \"created\": " << entity_stats_.entities_created << ",\n";
    file << "    \"destroyed\": " << entity_stats_.entities_destroyed << ",\n";
    file << "    \"avg_creation_time_us\": " << entity_stats_.avg_creation_time.count() << ",\n";
    file << "    \"avg_destruction_time_us\": " << entity_stats_.avg_destruction_time.count() << "\n";
    file << "  }\n";
    
    file << "}\n";
}

void ECSProfiler::clear_statistics() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    system_metrics_.clear();
    component_stats_.clear();
    entity_stats_ = EntityStats{};
    memory_stats_ = MemoryStats{};
    thread_stats_.clear();
}

void ECSProfiler::reset() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    clear_statistics();
    events_.clear();
    allocations_.clear();
    event_index_ = 0;
}

ECSProfiler& ECSProfiler::instance() {
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (!g_profiler_instance) {
        g_profiler_instance = std::make_unique<ECSProfiler>();
    }
    return *g_profiler_instance;
}

void ECSProfiler::cleanup() {
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    g_profiler_instance.reset();
}

} // namespace ecscope::profiling