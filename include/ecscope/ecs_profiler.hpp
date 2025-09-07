#pragma once

#include "types.hpp"
#include "id.hpp"
#include "component.hpp"
#include <chrono>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <optional>
#include <array>

namespace ecscope::profiling {

using namespace std::chrono;

// High-resolution timer for microsecond precision
using ProfilerClock = high_resolution_clock;
using ProfilerTimepoint = ProfilerClock::time_point;
using ProfilerDuration = microseconds;

// Performance measurement categories
enum class ProfileCategory : u8 {
    ENTITY_CREATION,
    ENTITY_DESTRUCTION,
    COMPONENT_ADD,
    COMPONENT_REMOVE,
    COMPONENT_ACCESS,
    SYSTEM_EXECUTION,
    MEMORY_ALLOCATION,
    MEMORY_DEALLOCATION,
    ARCHETYPE_CHANGE,
    QUERY_EXECUTION,
    EVENT_PROCESSING,
    SERIALIZATION,
    DESERIALIZATION,
    THREADING_OVERHEAD,
    CACHE_MISS,
    CUSTOM
};

// Memory allocation tracking
struct AllocationInfo {
    usize size;
    usize alignment;
    ProfilerTimepoint timestamp;
    std::string category;
    void* ptr;
    std::string stack_trace;
    u32 thread_id;
};

// Performance event data
struct ProfileEvent {
    ProfileCategory category;
    std::string name;
    ProfilerTimepoint start_time;
    ProfilerDuration duration;
    u32 thread_id;
    usize memory_used;
    usize entity_count;
    usize component_count;
    std::string additional_data;
};

// System performance metrics
struct SystemMetrics {
    std::string system_name;
    ProfilerDuration total_time{0};
    ProfilerDuration min_time{ProfilerDuration::max()};
    ProfilerDuration max_time{0};
    ProfilerDuration avg_time{0};
    u64 execution_count = 0;
    usize memory_peak = 0;
    usize memory_average = 0;
    f64 cpu_percentage = 0.0;
    std::vector<ProfilerDuration> recent_times; // Last N executions
    
    void update(ProfilerDuration execution_time, usize memory_usage) {
        total_time += execution_time;
        min_time = std::min(min_time, execution_time);
        max_time = std::max(max_time, execution_time);
        ++execution_count;
        avg_time = ProfilerDuration(total_time.count() / execution_count);
        
        memory_peak = std::max(memory_peak, memory_usage);
        memory_average = (memory_average * (execution_count - 1) + memory_usage) / execution_count;
        
        recent_times.push_back(execution_time);
        if (recent_times.size() > 100) { // Keep last 100 measurements
            recent_times.erase(recent_times.begin());
        }
    }
};

// Component usage statistics
struct ComponentStats {
    core::ComponentID component_id;
    std::string component_name;
    usize instance_count = 0;
    usize peak_count = 0;
    usize memory_usage = 0;
    usize access_count = 0;
    ProfilerDuration total_access_time{0};
    ProfilerDuration avg_access_time{0};
    std::vector<usize> count_history;
    
    void update_access(ProfilerDuration access_time) {
        ++access_count;
        total_access_time += access_time;
        avg_access_time = ProfilerDuration(total_access_time.count() / access_count);
    }
    
    void update_count(usize new_count) {
        instance_count = new_count;
        peak_count = std::max(peak_count, new_count);
        count_history.push_back(new_count);
        if (count_history.size() > 1000) { // Keep last 1000 samples
            count_history.erase(count_history.begin());
        }
    }
};

// Entity lifecycle tracking
struct EntityStats {
    u64 entities_created = 0;
    u64 entities_destroyed = 0;
    u64 active_entities = 0;
    u64 peak_entities = 0;
    ProfilerDuration avg_creation_time{0};
    ProfilerDuration avg_destruction_time{0};
    std::unordered_map<std::string, u64> archetype_distribution;
    
    void entity_created(ProfilerDuration creation_time) {
        ++entities_created;
        ++active_entities;
        peak_entities = std::max(peak_entities, active_entities);
        
        auto total_time = avg_creation_time * (entities_created - 1) + creation_time;
        avg_creation_time = ProfilerDuration(total_time.count() / entities_created);
    }
    
    void entity_destroyed(ProfilerDuration destruction_time) {
        ++entities_destroyed;
        --active_entities;
        
        auto total_time = avg_destruction_time * (entities_destroyed - 1) + destruction_time;
        avg_destruction_time = ProfilerDuration(total_time.count() / entities_destroyed);
    }
};

// Memory usage tracking
struct MemoryStats {
    usize current_usage = 0;
    usize peak_usage = 0;
    usize total_allocated = 0;
    usize total_deallocated = 0;
    u64 allocation_count = 0;
    u64 deallocation_count = 0;
    f64 fragmentation_ratio = 0.0;
    std::unordered_map<std::string, usize> category_usage;
    std::vector<usize> usage_history;
    
    void allocate(usize size, const std::string& category = "general") {
        current_usage += size;
        peak_usage = std::max(peak_usage, current_usage);
        total_allocated += size;
        ++allocation_count;
        category_usage[category] += size;
        
        usage_history.push_back(current_usage);
        if (usage_history.size() > 10000) { // Keep last 10k samples
            usage_history.erase(usage_history.begin());
        }
    }
    
    void deallocate(usize size, const std::string& category = "general") {
        current_usage = (size > current_usage) ? 0 : current_usage - size;
        total_deallocated += size;
        ++deallocation_count;
        if (category_usage[category] >= size) {
            category_usage[category] -= size;
        }
    }
};

// Thread performance tracking
struct ThreadStats {
    std::thread::id thread_id;
    std::string thread_name;
    ProfilerDuration total_execution_time{0};
    ProfilerDuration active_time{0};
    ProfilerDuration idle_time{0};
    u64 task_count = 0;
    f64 utilization = 0.0;
    std::vector<ProfilerDuration> task_times;
    
    void task_completed(ProfilerDuration task_time) {
        ++task_count;
        total_execution_time += task_time;
        active_time += task_time;
        
        task_times.push_back(task_time);
        if (task_times.size() > 1000) {
            task_times.erase(task_times.begin());
        }
        
        // Calculate utilization (active vs total time)
        auto total_time = active_time + idle_time;
        if (total_time.count() > 0) {
            utilization = static_cast<f64>(active_time.count()) / total_time.count();
        }
    }
};

// RAII profiling scope helper
class ProfileScope {
private:
    ProfilerTimepoint start_time_;
    std::string name_;
    ProfileCategory category_;
    std::function<void(const ProfileEvent&)> callback_;
    
public:
    ProfileScope(const std::string& name, ProfileCategory category = ProfileCategory::CUSTOM);
    ProfileScope(const std::string& name, ProfileCategory category, 
                std::function<void(const ProfileEvent&)> callback);
    ~ProfileScope();
    
    // Non-copyable, moveable
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;
    ProfileScope(ProfileScope&&) = default;
    ProfileScope& operator=(ProfileScope&&) = default;
};

// Main ECS profiler class
class ECSProfiler {
private:
    mutable std::mutex data_mutex_;
    std::atomic<bool> enabled_{true};
    std::atomic<bool> memory_tracking_{true};
    
    // Statistics storage
    std::unordered_map<std::string, SystemMetrics> system_metrics_;
    std::unordered_map<core::ComponentID, ComponentStats> component_stats_;
    EntityStats entity_stats_;
    MemoryStats memory_stats_;
    std::unordered_map<std::thread::id, ThreadStats> thread_stats_;
    
    // Event storage
    std::vector<ProfileEvent> events_;
    std::vector<AllocationInfo> allocations_;
    usize max_events_ = 100000; // Circular buffer size
    usize event_index_ = 0;
    
    // Sampling and filtering
    f32 sampling_rate_ = 1.0f; // 100% by default
    std::unordered_set<ProfileCategory> enabled_categories_;
    
    // Performance thresholds for warnings
    ProfilerDuration slow_system_threshold_{milliseconds(16)}; // 60 FPS frame budget
    usize high_memory_threshold_ = 100 * 1024 * 1024; // 100MB
    
    // Internal helpers
    void record_event_internal(const ProfileEvent& event);
    void record_allocation_internal(const AllocationInfo& allocation);
    std::string get_stack_trace(int max_frames = 10) const;
    u32 get_current_thread_id() const;
    
public:
    ECSProfiler();
    ~ECSProfiler();
    
    // Configuration
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }
    
    void set_memory_tracking(bool enabled) { memory_tracking_ = enabled; }
    bool is_memory_tracking() const { return memory_tracking_; }
    
    void set_sampling_rate(f32 rate) { sampling_rate_ = std::clamp(rate, 0.0f, 1.0f); }
    f32 get_sampling_rate() const { return sampling_rate_; }
    
    void enable_category(ProfileCategory category);
    void disable_category(ProfileCategory category);
    bool is_category_enabled(ProfileCategory category) const;
    
    void set_max_events(usize max_events) { max_events_ = max_events; }
    void set_slow_system_threshold(ProfilerDuration threshold) { slow_system_threshold_ = threshold; }
    void set_high_memory_threshold(usize threshold) { high_memory_threshold_ = threshold; }
    
    // Event recording
    void begin_system(const std::string& system_name);
    void end_system(const std::string& system_name, usize memory_usage = 0);
    
    void record_entity_created(ProfilerDuration creation_time);
    void record_entity_destroyed(ProfilerDuration destruction_time);
    
    void record_component_access(core::ComponentID component_id, 
                               const std::string& component_name,
                               ProfilerDuration access_time);
    
    void record_memory_allocation(usize size, usize alignment, 
                                const std::string& category = "general");
    void record_memory_deallocation(usize size, const std::string& category = "general");
    
    void record_custom_event(const std::string& name, ProfilerDuration duration,
                           const std::string& data = "");
    
    // Query interface
    SystemMetrics get_system_metrics(const std::string& system_name) const;
    ComponentStats get_component_stats(core::ComponentID component_id) const;
    EntityStats get_entity_stats() const;
    MemoryStats get_memory_stats() const;
    ThreadStats get_thread_stats(std::thread::id thread_id) const;
    
    std::vector<SystemMetrics> get_all_system_metrics() const;
    std::vector<ComponentStats> get_all_component_stats() const;
    std::vector<ThreadStats> get_all_thread_stats() const;
    
    // Event history
    std::vector<ProfileEvent> get_recent_events(usize count = 1000) const;
    std::vector<ProfileEvent> get_events_by_category(ProfileCategory category, usize count = 1000) const;
    std::vector<AllocationInfo> get_recent_allocations(usize count = 1000) const;
    
    // Analysis
    std::vector<std::string> detect_performance_issues() const;
    std::vector<std::string> detect_memory_issues() const;
    f64 calculate_overall_performance_score() const;
    
    // Reporting
    std::string generate_performance_report() const;
    std::string generate_memory_report() const;
    void export_to_json(const std::string& filename) const;
    void export_to_csv(const std::string& filename) const;
    
    // Control
    void clear_statistics();
    void reset();
    void flush_events();
    
    // Singleton access
    static ECSProfiler& instance();
    static void cleanup();
};

// Global profiling macros for convenience
#define PROFILE_SCOPE(name) \
    ecscope::profiling::ProfileScope _prof_scope(name)

#define PROFILE_SCOPE_CATEGORY(name, category) \
    ecscope::profiling::ProfileScope _prof_scope(name, category)

#define PROFILE_SYSTEM(system_name) \
    ecscope::profiling::ECSProfiler::instance().begin_system(system_name); \
    auto _system_guard = std::unique_ptr<void, std::function<void(void*)>>( \
        nullptr, [system_name](void*) { \
            ecscope::profiling::ECSProfiler::instance().end_system(system_name); \
        })

#define PROFILE_MEMORY_ALLOC(size, category) \
    if (ecscope::profiling::ECSProfiler::instance().is_memory_tracking()) { \
        ecscope::profiling::ECSProfiler::instance().record_memory_allocation(size, 1, category); \
    }

#define PROFILE_MEMORY_FREE(size, category) \
    if (ecscope::profiling::ECSProfiler::instance().is_memory_tracking()) { \
        ecscope::profiling::ECSProfiler::instance().record_memory_deallocation(size, category); \
    }

// Template helpers for automatic component profiling
template<typename ComponentType>
class ProfiledComponentAccess {
private:
    ProfilerTimepoint start_time_;
    core::ComponentID component_id_;
    std::string component_name_;
    
public:
    ProfiledComponentAccess() 
        : start_time_(ProfilerClock::now())
        , component_id_(ecs::component_id<ComponentType>())
        , component_name_(typeid(ComponentType).name()) {}
    
    ~ProfiledComponentAccess() {
        auto duration = duration_cast<ProfilerDuration>(ProfilerClock::now() - start_time_);
        ECSProfiler::instance().record_component_access(component_id_, component_name_, duration);
    }
};

#define PROFILE_COMPONENT_ACCESS(ComponentType) \
    ecscope::profiling::ProfiledComponentAccess<ComponentType> _comp_prof

} // namespace ecscope::profiling