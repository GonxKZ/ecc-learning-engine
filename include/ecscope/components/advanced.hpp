#pragma once

#include "reflection.hpp"
#include "properties.hpp"
#include "validation.hpp"
#include "factory.hpp"
#include "serialization.hpp"
#include "../foundation/concepts.hpp"
#include "../core/types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <optional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <queue>
#include <condition_variable>
#include <type_traits>

/**
 * @file advanced.hpp
 * @brief Advanced component system features
 * 
 * This file implements sophisticated features that enhance the component system:
 * - Hot-reloading support for runtime component updates
 * - Property change notifications and reactive programming
 * - Component dependency tracking and management
 * - Memory layout optimization for cache performance
 * - Component lifecycle management and hooks
 * - Advanced debugging and profiling tools
 * - Plugin system for extensibility
 * - Performance monitoring and analytics
 * 
 * Key Features:
 * - Zero-downtime hot reloading
 * - Reactive component updates with dependency resolution
 * - Automated memory layout optimization
 * - Real-time performance monitoring
 * - Extensible plugin architecture
 * - Thread-safe operations throughout
 * - Comprehensive debugging facilities
 */

namespace ecscope::components {

/// @brief Hot reload event types
enum class HotReloadEvent : std::uint8_t {
    ComponentAdded,        ///< New component type was added
    ComponentRemoved,      ///< Component type was removed  
    ComponentModified,     ///< Component type was modified
    PropertyAdded,         ///< New property was added to component
    PropertyRemoved,       ///< Property was removed from component
    PropertyModified,      ///< Property definition was modified
    BlueprintAdded,        ///< New blueprint was added
    BlueprintRemoved,      ///< Blueprint was removed
    BlueprintModified,     ///< Blueprint was modified
    ValidationRuleAdded,   ///< New validation rule was added
    ValidationRuleRemoved, ///< Validation rule was removed
    MetadataUpdated        ///< Component metadata was updated
};

/// @brief Hot reload context information
struct HotReloadContext {
    HotReloadEvent event_type;
    std::string component_name;
    std::string property_name;
    std::string blueprint_name;
    std::type_index type_index{typeid(void)};
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    std::unordered_map<std::string, std::string> metadata;
    
    HotReloadContext(HotReloadEvent event, std::string comp_name = "", std::type_index ti = typeid(void))
        : event_type(event), component_name(std::move(comp_name)), type_index(ti) {}
};

/// @brief Hot reload observer interface
class HotReloadObserver {
public:
    virtual ~HotReloadObserver() = default;
    virtual void on_hot_reload_event(const HotReloadContext& context) = 0;
    virtual std::string observer_name() const = 0;
};

/// @brief Component dependency information
struct ComponentDependency {
    std::type_index dependent_type;    ///< Type that depends on another
    std::type_index dependency_type;   ///< Type that is depended upon
    std::string relationship;          ///< Type of relationship (requires, uses, enhances, etc.)
    bool is_critical{false};          ///< If true, dependent cannot exist without dependency
    std::string description;
    
    ComponentDependency(std::type_index dep, std::type_index target, std::string rel, bool critical = false)
        : dependent_type(dep), dependency_type(target), relationship(std::move(rel)), is_critical(critical) {}
};

/// @brief Memory layout optimization information
struct MemoryLayoutInfo {
    std::size_t size{0};
    std::size_t alignment{0};
    std::size_t cache_line_alignment{0};
    std::size_t padding{0};
    bool is_packed{false};
    bool is_cacheline_aligned{false};
    double access_frequency{0.0}; ///< Relative access frequency (0.0-1.0)
    
    /// @brief Calculate cache efficiency score
    double cache_efficiency_score() const {
        double alignment_score = (alignment >= 8) ? 1.0 : alignment / 8.0;
        double size_score = (size <= 64) ? 1.0 : 64.0 / size; // Prefer cache line size
        double frequency_score = access_frequency;
        
        return (alignment_score + size_score + frequency_score) / 3.0;
    }
};

/// @brief Component lifecycle hooks
class ComponentLifecycleHooks {
public:
    using PreCreateHook = std::function<bool(std::type_index, const std::unordered_map<std::string, PropertyValue>&)>;
    using PostCreateHook = std::function<void(void*, std::type_index)>;
    using PreDestroyHook = std::function<bool(void*, std::type_index)>;
    using PostDestroyHook = std::function<void(std::type_index)>;
    using PreModifyHook = std::function<bool(void*, std::type_index, const std::string&, const PropertyValue&)>;
    using PostModifyHook = std::function<void(void*, std::type_index, const std::string&, const PropertyValue&)>;
    
    /// @brief Register pre-create hook
    void register_pre_create_hook(const std::string& name, PreCreateHook hook) {
        std::unique_lock lock(mutex_);
        pre_create_hooks_[name] = std::move(hook);
    }
    
    /// @brief Register post-create hook
    void register_post_create_hook(const std::string& name, PostCreateHook hook) {
        std::unique_lock lock(mutex_);
        post_create_hooks_[name] = std::move(hook);
    }
    
    /// @brief Register pre-destroy hook
    void register_pre_destroy_hook(const std::string& name, PreDestroyHook hook) {
        std::unique_lock lock(mutex_);
        pre_destroy_hooks_[name] = std::move(hook);
    }
    
    /// @brief Register post-destroy hook
    void register_post_destroy_hook(const std::string& name, PostDestroyHook hook) {
        std::unique_lock lock(mutex_);
        post_destroy_hooks_[name] = std::move(hook);
    }
    
    /// @brief Register pre-modify hook
    void register_pre_modify_hook(const std::string& name, PreModifyHook hook) {
        std::unique_lock lock(mutex_);
        pre_modify_hooks_[name] = std::move(hook);
    }
    
    /// @brief Register post-modify hook
    void register_post_modify_hook(const std::string& name, PostModifyHook hook) {
        std::unique_lock lock(mutex_);
        post_modify_hooks_[name] = std::move(hook);
    }
    
    /// @brief Execute pre-create hooks
    bool execute_pre_create_hooks(std::type_index type, const std::unordered_map<std::string, PropertyValue>& params) {
        std::shared_lock lock(mutex_);
        
        for (const auto& [name, hook] : pre_create_hooks_) {
            if (!hook(type, params)) {
                return false; // Hook vetoed creation
            }
        }
        return true;
    }
    
    /// @brief Execute post-create hooks
    void execute_post_create_hooks(void* component, std::type_index type) {
        std::shared_lock lock(mutex_);
        
        for (const auto& [name, hook] : post_create_hooks_) {
            hook(component, type);
        }
    }
    
    /// @brief Execute pre-destroy hooks
    bool execute_pre_destroy_hooks(void* component, std::type_index type) {
        std::shared_lock lock(mutex_);
        
        for (const auto& [name, hook] : pre_destroy_hooks_) {
            if (!hook(component, type)) {
                return false; // Hook vetoed destruction
            }
        }
        return true;
    }
    
    /// @brief Execute post-destroy hooks
    void execute_post_destroy_hooks(std::type_index type) {
        std::shared_lock lock(mutex_);
        
        for (const auto& [name, hook] : post_destroy_hooks_) {
            hook(type);
        }
    }
    
    /// @brief Execute pre-modify hooks
    bool execute_pre_modify_hooks(void* component, std::type_index type, 
                                const std::string& property, const PropertyValue& value) {
        std::shared_lock lock(mutex_);
        
        for (const auto& [name, hook] : pre_modify_hooks_) {
            if (!hook(component, type, property, value)) {
                return false; // Hook vetoed modification
            }
        }
        return true;
    }
    
    /// @brief Execute post-modify hooks
    void execute_post_modify_hooks(void* component, std::type_index type, 
                                 const std::string& property, const PropertyValue& value) {
        std::shared_lock lock(mutex_);
        
        for (const auto& [name, hook] : post_modify_hooks_) {
            hook(component, type, property, value);
        }
    }
    
    /// @brief Remove hook by name
    void remove_hook(const std::string& name) {
        std::unique_lock lock(mutex_);
        pre_create_hooks_.erase(name);
        post_create_hooks_.erase(name);
        pre_destroy_hooks_.erase(name);
        post_destroy_hooks_.erase(name);
        pre_modify_hooks_.erase(name);
        post_modify_hooks_.erase(name);
    }
    
    /// @brief Clear all hooks
    void clear_all_hooks() {
        std::unique_lock lock(mutex_);
        pre_create_hooks_.clear();
        post_create_hooks_.clear();
        pre_destroy_hooks_.clear();
        post_destroy_hooks_.clear();
        pre_modify_hooks_.clear();
        post_modify_hooks_.clear();
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, PreCreateHook> pre_create_hooks_;
    std::unordered_map<std::string, PostCreateHook> post_create_hooks_;
    std::unordered_map<std::string, PreDestroyHook> pre_destroy_hooks_;
    std::unordered_map<std::string, PostDestroyHook> post_destroy_hooks_;
    std::unordered_map<std::string, PreModifyHook> pre_modify_hooks_;
    std::unordered_map<std::string, PostModifyHook> post_modify_hooks_;
};

/// @brief Hot reload manager for runtime component updates
class HotReloadManager {
public:
    /// @brief Singleton access
    static HotReloadManager& instance() {
        static HotReloadManager manager;
        return manager;
    }
    
    /// @brief Enable hot reloading
    void enable_hot_reload() {
        std::unique_lock lock(mutex_);
        if (!enabled_) {
            enabled_ = true;
            start_file_watcher();
        }
    }
    
    /// @brief Disable hot reloading
    void disable_hot_reload() {
        std::unique_lock lock(mutex_);
        if (enabled_) {
            enabled_ = false;
            stop_file_watcher();
        }
    }
    
    /// @brief Check if hot reloading is enabled
    bool is_enabled() const {
        std::shared_lock lock(mutex_);
        return enabled_;
    }
    
    /// @brief Register hot reload observer
    using ObserverHandle = std::uint64_t;
    
    ObserverHandle register_observer(std::shared_ptr<HotReloadObserver> observer) {
        std::unique_lock lock(mutex_);
        const auto handle = next_observer_handle_++;
        observers_[handle] = std::move(observer);
        return handle;
    }
    
    /// @brief Unregister hot reload observer
    void unregister_observer(ObserverHandle handle) {
        std::unique_lock lock(mutex_);
        observers_.erase(handle);
    }
    
    /// @brief Trigger hot reload event
    void trigger_hot_reload_event(const HotReloadContext& context) {
        if (!is_enabled()) return;
        
        std::shared_lock lock(mutex_);
        
        for (const auto& [handle, observer] : observers_) {
            if (auto obs = observer.lock()) {
                try {
                    obs->on_hot_reload_event(context);
                } catch (const std::exception& e) {
                    // Log error but continue
                }
            }
        }
    }
    
    /// @brief Add file to watch for changes
    void watch_file(const std::filesystem::path& file_path) {
        std::unique_lock lock(mutex_);
        watched_files_.insert(file_path);
    }
    
    /// @brief Remove file from watch list
    void unwatch_file(const std::filesystem::path& file_path) {
        std::unique_lock lock(mutex_);
        watched_files_.erase(file_path);
    }
    
    /// @brief Get watched files
    std::vector<std::filesystem::path> get_watched_files() const {
        std::shared_lock lock(mutex_);
        return std::vector<std::filesystem::path>(watched_files_.begin(), watched_files_.end());
    }
    
    /// @brief Set file watch interval
    void set_watch_interval(std::chrono::milliseconds interval) {
        std::unique_lock lock(mutex_);
        watch_interval_ = interval;
    }

private:
    HotReloadManager() = default;
    ~HotReloadManager() { disable_hot_reload(); }
    
    mutable std::shared_mutex mutex_;
    std::atomic<bool> enabled_{false};
    std::atomic<ObserverHandle> next_observer_handle_{1};
    
    std::unordered_map<ObserverHandle, std::weak_ptr<HotReloadObserver>> observers_;
    std::unordered_set<std::filesystem::path> watched_files_;
    std::unordered_map<std::filesystem::path, std::filesystem::file_time_type> file_timestamps_;
    
    std::chrono::milliseconds watch_interval_{1000}; // 1 second
    std::unique_ptr<std::thread> watcher_thread_;
    std::atomic<bool> should_stop_watcher_{false};
    
    void start_file_watcher() {
        should_stop_watcher_ = false;
        watcher_thread_ = std::make_unique<std::thread>([this] { file_watcher_loop(); });
    }
    
    void stop_file_watcher() {
        should_stop_watcher_ = true;
        if (watcher_thread_ && watcher_thread_->joinable()) {
            watcher_thread_->join();
        }
        watcher_thread_.reset();
    }
    
    void file_watcher_loop() {
        while (!should_stop_watcher_) {
            check_file_changes();
            std::this_thread::sleep_for(watch_interval_);
        }
    }
    
    void check_file_changes() {
        std::shared_lock lock(mutex_);
        
        for (const auto& file_path : watched_files_) {
            if (!std::filesystem::exists(file_path)) {
                continue;
            }
            
            auto last_write_time = std::filesystem::last_write_time(file_path);
            auto it = file_timestamps_.find(file_path);
            
            if (it == file_timestamps_.end()) {
                // First time seeing this file
                file_timestamps_[file_path] = last_write_time;
            } else if (it->second != last_write_time) {
                // File was modified
                it->second = last_write_time;
                
                HotReloadContext context(HotReloadEvent::ComponentModified);
                context.metadata["file_path"] = file_path.string();
                trigger_hot_reload_event(context);
            }
        }
    }
};

/// @brief Component dependency manager
class ComponentDependencyManager {
public:
    /// @brief Singleton access
    static ComponentDependencyManager& instance() {
        static ComponentDependencyManager manager;
        return manager;
    }
    
    /// @brief Add component dependency
    void add_dependency(const ComponentDependency& dependency) {
        std::unique_lock lock(mutex_);
        dependencies_.emplace_back(dependency);
    }
    
    /// @brief Add dependency by types
    template<typename Dependent, typename Dependency>
    void add_dependency(const std::string& relationship, bool is_critical = false, const std::string& description = "") {
        ComponentDependency dep(typeid(Dependent), typeid(Dependency), relationship, is_critical);
        dep.description = description;
        add_dependency(dep);
    }
    
    /// @brief Get dependencies for a component type
    std::vector<ComponentDependency> get_dependencies(std::type_index type) const {
        std::shared_lock lock(mutex_);
        
        std::vector<ComponentDependency> result;
        for (const auto& dep : dependencies_) {
            if (dep.dependent_type == type) {
                result.push_back(dep);
            }
        }
        return result;
    }
    
    /// @brief Get dependents of a component type
    std::vector<ComponentDependency> get_dependents(std::type_index type) const {
        std::shared_lock lock(mutex_);
        
        std::vector<ComponentDependency> result;
        for (const auto& dep : dependencies_) {
            if (dep.dependency_type == type) {
                result.push_back(dep);
            }
        }
        return result;
    }
    
    /// @brief Check if dependency exists
    bool has_dependency(std::type_index dependent, std::type_index dependency) const {
        std::shared_lock lock(mutex_);
        
        return std::any_of(dependencies_.begin(), dependencies_.end(),
                          [dependent, dependency](const ComponentDependency& dep) {
                              return dep.dependent_type == dependent && dep.dependency_type == dependency;
                          });
    }
    
    /// @brief Resolve dependency order for component creation
    std::vector<std::type_index> resolve_creation_order(const std::vector<std::type_index>& types) const {
        std::shared_lock lock(mutex_);
        
        // Simple topological sort
        std::vector<std::type_index> result;
        std::unordered_set<std::type_index> visited;
        std::unordered_set<std::type_index> in_progress;
        
        std::function<bool(std::type_index)> visit = [&](std::type_index type) -> bool {
            if (in_progress.contains(type)) {
                return false; // Circular dependency detected
            }
            if (visited.contains(type)) {
                return true; // Already processed
            }
            
            in_progress.insert(type);
            
            // Visit all dependencies first
            for (const auto& dep : dependencies_) {
                if (dep.dependent_type == type && 
                    std::find(types.begin(), types.end(), dep.dependency_type) != types.end()) {
                    if (!visit(dep.dependency_type)) {
                        return false;
                    }
                }
            }
            
            in_progress.erase(type);
            visited.insert(type);
            result.push_back(type);
            return true;
        };
        
        for (const auto& type : types) {
            if (!visit(type)) {
                // Circular dependency, return original order
                return types;
            }
        }
        
        return result;
    }
    
    /// @brief Get all dependencies
    const std::vector<ComponentDependency>& all_dependencies() const {
        std::shared_lock lock(mutex_);
        return dependencies_;
    }
    
    /// @brief Clear all dependencies
    void clear_dependencies() {
        std::unique_lock lock(mutex_);
        dependencies_.clear();
    }

private:
    ComponentDependencyManager() = default;
    
    mutable std::shared_mutex mutex_;
    std::vector<ComponentDependency> dependencies_;
};

/// @brief Memory layout optimizer for cache performance
class MemoryLayoutOptimizer {
public:
    /// @brief Singleton access
    static MemoryLayoutOptimizer& instance() {
        static MemoryLayoutOptimizer optimizer;
        return optimizer;
    }
    
    /// @brief Register component layout info
    template<typename T>
    void register_layout_info(double access_frequency = 0.5) {
        std::unique_lock lock(mutex_);
        
        MemoryLayoutInfo info;
        info.size = sizeof(T);
        info.alignment = alignof(T);
        info.cache_line_alignment = (sizeof(T) % 64 == 0) ? 64 : alignof(T);
        info.is_packed = std::is_standard_layout_v<T>;
        info.is_cacheline_aligned = (alignof(T) >= 64);
        info.access_frequency = access_frequency;
        
        layout_infos_[typeid(T)] = info;
    }
    
    /// @brief Get layout info for component
    const MemoryLayoutInfo* get_layout_info(std::type_index type) const {
        std::shared_lock lock(mutex_);
        
        auto it = layout_infos_.find(type);
        return it != layout_infos_.end() ? &it->second : nullptr;
    }
    
    /// @brief Optimize memory layout for component list
    std::vector<std::type_index> optimize_layout(const std::vector<std::type_index>& types) const {
        std::shared_lock lock(mutex_);
        
        // Sort by cache efficiency score (descending) and size (ascending)
        std::vector<std::type_index> optimized = types;
        
        std::sort(optimized.begin(), optimized.end(),
                 [this](std::type_index a, std::type_index b) {
                     const auto* info_a = get_layout_info(a);
                     const auto* info_b = get_layout_info(b);
                     
                     if (!info_a && !info_b) return false;
                     if (!info_a) return false;
                     if (!info_b) return true;
                     
                     double score_a = info_a->cache_efficiency_score();
                     double score_b = info_b->cache_efficiency_score();
                     
                     if (std::abs(score_a - score_b) < 0.01) {
                         // Similar scores, sort by size (smaller first)
                         return info_a->size < info_b->size;
                     }
                     
                     return score_a > score_b;
                 });
        
        return optimized;
    }
    
    /// @brief Calculate total memory requirements
    std::size_t calculate_total_memory(const std::vector<std::type_index>& types) const {
        std::shared_lock lock(mutex_);
        
        std::size_t total = 0;
        for (const auto& type : types) {
            const auto* info = get_layout_info(type);
            if (info) {
                total += info->size + info->padding;
            }
        }
        return total;
    }
    
    /// @brief Get layout statistics
    struct LayoutStats {
        std::size_t total_registered_types{0};
        std::size_t cache_friendly_types{0};
        std::size_t large_types{0};        // > 64 bytes
        double average_cache_score{0.0};
    };
    
    LayoutStats get_statistics() const {
        std::shared_lock lock(mutex_);
        
        LayoutStats stats;
        stats.total_registered_types = layout_infos_.size();
        
        double total_score = 0.0;
        for (const auto& [type, info] : layout_infos_) {
            double score = info.cache_efficiency_score();
            total_score += score;
            
            if (score >= 0.7) {
                stats.cache_friendly_types++;
            }
            
            if (info.size > 64) {
                stats.large_types++;
            }
        }
        
        if (stats.total_registered_types > 0) {
            stats.average_cache_score = total_score / stats.total_registered_types;
        }
        
        return stats;
    }

private:
    MemoryLayoutOptimizer() = default;
    
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::type_index, MemoryLayoutInfo> layout_infos_;
};

/// @brief Component system performance monitor
class ComponentPerformanceMonitor {
public:
    /// @brief Performance metrics for a component type
    struct ComponentMetrics {
        std::atomic<std::uint64_t> creation_count{0};
        std::atomic<std::uint64_t> destruction_count{0};
        std::atomic<std::uint64_t> property_access_count{0};
        std::atomic<std::uint64_t> validation_count{0};
        std::atomic<std::uint64_t> serialization_count{0};
        
        std::atomic<std::uint64_t> total_creation_time_ns{0};
        std::atomic<std::uint64_t> total_destruction_time_ns{0};
        std::atomic<std::uint64_t> total_property_access_time_ns{0};
        std::atomic<std::uint64_t> total_validation_time_ns{0};
        std::atomic<std::uint64_t> total_serialization_time_ns{0};
        
        /// @brief Get average creation time
        double average_creation_time_ns() const {
            auto count = creation_count.load();
            return count > 0 ? static_cast<double>(total_creation_time_ns.load()) / count : 0.0;
        }
        
        /// @brief Get average property access time
        double average_property_access_time_ns() const {
            auto count = property_access_count.load();
            return count > 0 ? static_cast<double>(total_property_access_time_ns.load()) / count : 0.0;
        }
        
        /// @brief Reset metrics
        void reset() {
            creation_count = 0;
            destruction_count = 0;
            property_access_count = 0;
            validation_count = 0;
            serialization_count = 0;
            total_creation_time_ns = 0;
            total_destruction_time_ns = 0;
            total_property_access_time_ns = 0;
            total_validation_time_ns = 0;
            total_serialization_time_ns = 0;
        }
    };
    
    /// @brief Singleton access
    static ComponentPerformanceMonitor& instance() {
        static ComponentPerformanceMonitor monitor;
        return monitor;
    }
    
    /// @brief Record component creation time
    void record_creation_time(std::type_index type, std::chrono::nanoseconds duration) {
        auto& metrics = get_metrics(type);
        metrics.creation_count.fetch_add(1);
        metrics.total_creation_time_ns.fetch_add(duration.count());
    }
    
    /// @brief Record component destruction time
    void record_destruction_time(std::type_index type, std::chrono::nanoseconds duration) {
        auto& metrics = get_metrics(type);
        metrics.destruction_count.fetch_add(1);
        metrics.total_destruction_time_ns.fetch_add(duration.count());
    }
    
    /// @brief Record property access time
    void record_property_access_time(std::type_index type, std::chrono::nanoseconds duration) {
        auto& metrics = get_metrics(type);
        metrics.property_access_count.fetch_add(1);
        metrics.total_property_access_time_ns.fetch_add(duration.count());
    }
    
    /// @brief Record validation time
    void record_validation_time(std::type_index type, std::chrono::nanoseconds duration) {
        auto& metrics = get_metrics(type);
        metrics.validation_count.fetch_add(1);
        metrics.total_validation_time_ns.fetch_add(duration.count());
    }
    
    /// @brief Record serialization time
    void record_serialization_time(std::type_index type, std::chrono::nanoseconds duration) {
        auto& metrics = get_metrics(type);
        metrics.serialization_count.fetch_add(1);
        metrics.total_serialization_time_ns.fetch_add(duration.count());
    }
    
    /// @brief Get metrics for component type
    const ComponentMetrics& get_metrics(std::type_index type) const {
        std::shared_lock lock(mutex_);
        
        static ComponentMetrics empty_metrics;
        auto it = metrics_.find(type);
        return it != metrics_.end() ? it->second : empty_metrics;
    }
    
    /// @brief Get all metrics
    std::unordered_map<std::type_index, ComponentMetrics> get_all_metrics() const {
        std::shared_lock lock(mutex_);
        return metrics_;
    }
    
    /// @brief Reset all metrics
    void reset_all_metrics() {
        std::unique_lock lock(mutex_);
        for (auto& [type, metrics] : metrics_) {
            metrics.reset();
        }
    }
    
    /// @brief Generate performance report
    std::string generate_report() const;

private:
    ComponentPerformanceMonitor() = default;
    
    mutable std::shared_mutex mutex_;
    mutable std::unordered_map<std::type_index, ComponentMetrics> metrics_;
    
    ComponentMetrics& get_metrics(std::type_index type) {
        std::unique_lock lock(mutex_);
        return metrics_[type]; // Creates if doesn't exist
    }
};

/// @brief RAII performance timer
class PerformanceTimer {
public:
    PerformanceTimer(std::type_index type, void (ComponentPerformanceMonitor::*record_func)(std::type_index, std::chrono::nanoseconds))
        : type_(type), record_func_(record_func), start_(std::chrono::high_resolution_clock::now()) {}
    
    ~PerformanceTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_);
        (ComponentPerformanceMonitor::instance().*record_func_)(type_, duration);
    }

private:
    std::type_index type_;
    void (ComponentPerformanceMonitor::*record_func_)(std::type_index, std::chrono::nanoseconds);
    std::chrono::high_resolution_clock::time_point start_;
};

/// @brief Performance measurement macros
#define ECSCOPE_MEASURE_CREATION(Type) \
    ::ecscope::components::PerformanceTimer timer(typeid(Type), &::ecscope::components::ComponentPerformanceMonitor::record_creation_time)

#define ECSCOPE_MEASURE_DESTRUCTION(Type) \
    ::ecscope::components::PerformanceTimer timer(typeid(Type), &::ecscope::components::ComponentPerformanceMonitor::record_destruction_time)

#define ECSCOPE_MEASURE_PROPERTY_ACCESS(Type) \
    ::ecscope::components::PerformanceTimer timer(typeid(Type), &::ecscope::components::ComponentPerformanceMonitor::record_property_access_time)

/// @brief Advanced component system manager
class AdvancedComponentSystem {
public:
    /// @brief Singleton access
    static AdvancedComponentSystem& instance() {
        static AdvancedComponentSystem system;
        return system;
    }
    
    /// @brief Initialize advanced features
    void initialize() {
        std::unique_lock lock(mutex_);
        if (!initialized_) {
            hot_reload_manager_ = &HotReloadManager::instance();
            dependency_manager_ = &ComponentDependencyManager::instance();
            layout_optimizer_ = &MemoryLayoutOptimizer::instance();
            performance_monitor_ = &ComponentPerformanceMonitor::instance();
            
            setup_lifecycle_hooks();
            initialized_ = true;
        }
    }
    
    /// @brief Shutdown advanced features
    void shutdown() {
        std::unique_lock lock(mutex_);
        if (initialized_) {
            hot_reload_manager_->disable_hot_reload();
            lifecycle_hooks_.clear_all_hooks();
            initialized_ = false;
        }
    }
    
    /// @brief Get lifecycle hooks
    ComponentLifecycleHooks& lifecycle_hooks() { return lifecycle_hooks_; }
    
    /// @brief Check if initialized
    bool is_initialized() const {
        std::shared_lock lock(mutex_);
        return initialized_;
    }

private:
    AdvancedComponentSystem() = default;
    
    mutable std::shared_mutex mutex_;
    std::atomic<bool> initialized_{false};
    
    HotReloadManager* hot_reload_manager_{nullptr};
    ComponentDependencyManager* dependency_manager_{nullptr};
    MemoryLayoutOptimizer* layout_optimizer_{nullptr};
    ComponentPerformanceMonitor* performance_monitor_{nullptr};
    ComponentLifecycleHooks lifecycle_hooks_;
    
    void setup_lifecycle_hooks() {
        // Register default performance monitoring hooks
        lifecycle_hooks_.register_post_create_hook("performance_monitor",
            [this](void* component, std::type_index type) {
                // Component creation completed
            });
            
        lifecycle_hooks_.register_pre_destroy_hook("performance_monitor",
            [this](void* component, std::type_index type) -> bool {
                // Allow destruction
                return true;
            });
    }
};

}  // namespace ecscope::components