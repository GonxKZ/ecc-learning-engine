/**
 * @file system.cpp
 * @brief Implementation of the Advanced ECS System Architecture
 * 
 * Contains the runtime implementations for system management, scheduling,
 * and execution. This demonstrates advanced system architecture patterns
 * and performance optimization techniques.
 */

#include "system.hpp"
#include "registry.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <future>

namespace ecscope::ecs {

// Static member initialization
std::atomic<u32> System::allocator_id_counter_{1};

// System implementation
System::System(const std::string& name, SystemPhase phase, SystemExecutionType execution)
    : name_(name)
    , primary_phase_(phase)
    , execution_type_(execution)
    , is_enabled_(true)
    , is_initialized_(false)
    , time_budget_(0.016)  // 16ms default (60 FPS)
    , allocator_id_(next_allocator_id()) {
    
    // Create system-specific arena allocator
    system_arena_ = std::make_unique<memory::ArenaAllocator>(
        1024 * 1024,  // 1MB default
        name + "_Arena",
        true  // Enable tracking
    );
    
    LOG_INFO("Created system '{}' (phase: {}, execution: {})",
             name_, static_cast<int>(phase), static_cast<int>(execution));
}

System::~System() {
    if (is_initialized_) {
        LOG_WARN("System '{}' destroyed without proper shutdown", name_);
    }
    
    LOG_INFO("Destroyed system '{}' - {} executions, {:.2f}ms average",
             name_, stats_.total_executions, stats_.average_execution_time * 1000.0);
}

bool System::initialize(const SystemContext& context) {
    if (is_initialized_) {
        LOG_WARN("System '{}' already initialized", name_);
        return true;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Default initialization - subclasses can override
    is_initialized_ = true;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 init_time = std::chrono::duration<f64>(end_time - start_time).count();
    
    LOG_INFO("System '{}' initialized in {:.2f}ms", name_, init_time * 1000.0);
    return true;
}

void System::shutdown(const SystemContext& context) {
    if (!is_initialized_) {
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Default shutdown - subclasses can override
    is_initialized_ = false;
    
    // Reset arena allocator
    system_arena_->reset();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 shutdown_time = std::chrono::duration<f64>(end_time - start_time).count();
    
    LOG_INFO("System '{}' shutdown in {:.2f}ms", name_, shutdown_time * 1000.0);
}

void System::execute_internal(const SystemContext& context) {
    if (!is_enabled_ || !is_initialized_) {
        stats_.total_skipped++;
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Track memory usage before execution
    usize memory_before = system_arena_->used_size();
    
    try {
        // Execute the system
        update(context);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 execution_time = std::chrono::duration<f64>(end_time - start_time).count();
        
        // Track memory usage after execution
        usize memory_after = system_arena_->used_size();
        usize memory_used = memory_after - memory_before;
        
        // Update statistics
        record_execution_end(execution_time);
        
        // Track memory allocation
        if (memory_used > 0) {
            stats_.memory_allocated += memory_used;
            stats_.memory_allocations++;
            
            if (memory_after > stats_.peak_memory_usage) {
                stats_.peak_memory_usage = memory_after;
            }
        }
        
        // Check for budget violations
        if (execution_time > time_budget_) {
            stats_.exceeded_budget = true;
            LOG_WARN("System '{}' exceeded time budget: {:.2f}ms vs {:.2f}ms budget",
                    name_, execution_time * 1000.0, time_budget_ * 1000.0);
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("System '{}' threw exception during execution: {}", name_, e.what());
        throw;
    }
}

System& System::depends_on(const std::string& system_name, bool hard_dependency, f64 max_wait) {
    dependencies_.emplace_back(system_name, hard_dependency, max_wait);
    return *this;
}

System& System::reads_component(std::type_index component_type) {
    resource_info_.read_components.push_back(component_type);
    return *this;
}

System& System::writes_component(std::type_index component_type) {
    resource_info_.write_components.push_back(component_type);
    return *this;
}

System& System::reads_resource(const std::string& resource_name) {
    resource_info_.read_resources.push_back(resource_name);
    return *this;
}

System& System::writes_resource(const std::string& resource_name) {
    resource_info_.write_resources.push_back(resource_name);
    return *this;
}

System& System::exclusive_resource(const std::string& resource_name) {
    resource_info_.exclusive_resources.push_back(resource_name);
    return *this;
}

void System::record_execution_start() {
    stats_.total_scheduled++;
}

void System::record_execution_end(f64 execution_time) {
    stats_.record_execution(execution_time);
}

u32 System::next_allocator_id() {
    return allocator_id_counter_.fetch_add(1, std::memory_order_relaxed);
}

// SystemGroup implementation
void SystemGroup::add_system(std::unique_ptr<System> system) {
    LOG_INFO("Adding system '{}' to group '{}'", system->name(), name_);
    systems_.push_back(std::move(system));
}

void SystemGroup::remove_system(const std::string& system_name) {
    auto it = std::find_if(systems_.begin(), systems_.end(),
                          [&](const auto& system) { 
                              return system->name() == system_name; 
                          });
    
    if (it != systems_.end()) {
        LOG_INFO("Removing system '{}' from group '{}'", system_name, name_);
        systems_.erase(it);
    }
}

System* SystemGroup::get_system(const std::string& system_name) {
    auto it = std::find_if(systems_.begin(), systems_.end(),
                          [&](const auto& system) { 
                              return system->name() == system_name; 
                          });
    
    return it != systems_.end() ? it->get() : nullptr;
}

void SystemGroup::execute_all(const SystemContext& context) {
    if (is_parallel_) {
        execute_parallel(context);
    } else {
        execute_sequential(context);
    }
}

void SystemGroup::execute_parallel(const SystemContext& context) {
    std::vector<std::future<void>> futures;
    futures.reserve(systems_.size());
    
    for (auto& system : systems_) {
        if (system->execution_type() == SystemExecutionType::Parallel) {
            futures.emplace_back(std::async(std::launch::async, [&]() {
                system->execute_internal(context);
            }));
        } else {
            // Sequential systems run immediately
            system->execute_internal(context);
        }
    }
    
    // Wait for all parallel systems to complete
    for (auto& future : futures) {
        future.wait();
    }
}

void SystemGroup::execute_sequential(const SystemContext& context) {
    for (auto& system : systems_) {
        system->execute_internal(context);
    }
}

SystemStats SystemGroup::get_combined_stats() const {
    SystemStats combined;
    combined.reset();
    
    for (const auto& system : systems_) {
        const auto& stats = system->statistics();
        combined.total_executions += stats.total_executions;
        combined.total_execution_time += stats.total_execution_time;
        combined.total_scheduled += stats.total_scheduled;
        combined.total_skipped += stats.total_skipped;
        combined.memory_allocated += stats.memory_allocated;
        combined.memory_allocations += stats.memory_allocations;
        
        if (stats.peak_memory_usage > combined.peak_memory_usage) {
            combined.peak_memory_usage = stats.peak_memory_usage;
        }
        
        if (stats.max_execution_time > combined.max_execution_time) {
            combined.max_execution_time = stats.max_execution_time;
        }
    }
    
    combined.update_averages();
    return combined;
}

std::vector<std::pair<std::string, SystemStats>> SystemGroup::get_individual_stats() const {
    std::vector<std::pair<std::string, SystemStats>> stats;
    stats.reserve(systems_.size());
    
    for (const auto& system : systems_) {
        stats.emplace_back(system->name(), system->statistics());
    }
    
    return stats;
}

// SystemThreadPool implementation
SystemThreadPool::SystemThreadPool(usize thread_count) 
    : thread_count_(thread_count), stop_flag_(false) {
    
    workers_.reserve(thread_count_);
    
    for (usize i = 0; i < thread_count_; ++i) {
        workers_.emplace_back(&SystemThreadPool::worker_thread, this);
    }
    
    LOG_INFO("SystemThreadPool initialized with {} threads", thread_count_);
}

SystemThreadPool::~SystemThreadPool() {
    stop_flag_.store(true);
    condition_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    LOG_INFO("SystemThreadPool shutdown");
}

void SystemThreadPool::wait_for_all() {
    std::unique_lock<std::mutex> lock(tasks_mutex_);
    condition_.wait(lock, [this] { return tasks_.empty(); });
}

usize SystemThreadPool::pending_tasks() const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    return tasks_.size();
}

void SystemThreadPool::worker_thread() {
    while (!stop_flag_.load()) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(tasks_mutex_);
            condition_.wait(lock, [this] { return !tasks_.empty() || stop_flag_.load(); });
            
            if (stop_flag_.load()) {
                break;
            }
            
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }
        
        if (task) {
            task();
        }
    }
}

// SystemScheduler implementation
SystemScheduler::SystemScheduler(usize thread_count)
    : thread_pool_(std::make_unique<SystemThreadPool>(thread_count))
    , total_scheduling_time_(0.0)
    , scheduling_iterations_(0) {
    
    LOG_INFO("SystemScheduler initialized with {} threads", thread_count);
}

SystemScheduler::~SystemScheduler() {
    LOG_INFO("SystemScheduler shutdown - average scheduling time: {:.2f}ms",
             get_average_scheduling_time() * 1000.0);
}

void SystemScheduler::add_system(System* system) {
    std::lock_guard<std::mutex> lock(scheduling_mutex_);
    
    scheduled_systems_.emplace_back(system);
    
    // Extract dependencies
    auto& scheduled = scheduled_systems_.back();
    for (const auto& dep : system->dependencies()) {
        scheduled.dependencies.push_back(dep.system_name);
    }
    
    LOG_INFO("Added system '{}' to scheduler", system->name());
}

void SystemScheduler::remove_system(const std::string& system_name) {
    std::lock_guard<std::mutex> lock(scheduling_mutex_);
    
    auto it = std::find_if(scheduled_systems_.begin(), scheduled_systems_.end(),
                          [&](const ScheduledSystem& scheduled) {
                              return scheduled.system->name() == system_name;
                          });
    
    if (it != scheduled_systems_.end()) {
        LOG_INFO("Removed system '{}' from scheduler", system_name);
        scheduled_systems_.erase(it);
    }
}

void SystemScheduler::execute_phase(SystemPhase phase, const SystemContext& context) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Get systems for this phase
    auto systems = get_systems_for_phase(phase);
    if (systems.empty()) {
        return;
    }
    
    // Perform topological sort to resolve dependencies
    topological_sort(systems);
    
    // Check for circular dependencies
    if (has_circular_dependency(systems)) {
        LOG_ERROR("Circular dependency detected in phase {}", static_cast<int>(phase));
        return;
    }
    
    // Execute systems in dependency order
    std::vector<std::future<void>> parallel_futures;
    
    for (System* system : systems) {
        if (system->execution_type() == SystemExecutionType::Parallel) {
            // Submit to thread pool
            parallel_futures.emplace_back(
                thread_pool_->submit([system, &context]() {
                    system->execute_internal(context);
                })
            );
        } else {
            // Execute sequentially
            system->execute_internal(context);
        }
    }
    
    // Wait for parallel systems to complete
    for (auto& future : parallel_futures) {
        future.wait();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 scheduling_time = std::chrono::duration<f64>(end_time - start_time).count();
    
    total_scheduling_time_ += scheduling_time;
    scheduling_iterations_++;
}

std::vector<System*> SystemScheduler::get_systems_for_phase(SystemPhase phase) const {
    std::vector<System*> systems;
    
    for (const auto& scheduled : scheduled_systems_) {
        if (scheduled.system->phase() == phase && scheduled.system->is_enabled()) {
            systems.push_back(scheduled.system);
        }
    }
    
    return systems;
}

void SystemScheduler::topological_sort(std::vector<System*>& systems) const {
    // Simple topological sort implementation
    // In a full implementation, this would use Kahn's algorithm or DFS
    
    std::stable_sort(systems.begin(), systems.end(),
                    [](const System* a, const System* b) {
                        // Systems with fewer dependencies come first
                        return a->dependencies().size() < b->dependencies().size();
                    });
}

bool SystemScheduler::has_circular_dependency(const std::vector<System*>& systems) const {
    // Simple cycle detection - in practice, this would use a proper graph algorithm
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursion_stack;
    
    // For this simplified implementation, just check for obvious cycles
    for (const System* system : systems) {
        for (const auto& dep : system->dependencies()) {
            // Check if any dependency depends on this system (simplified check)
            for (const System* other : systems) {
                if (other->name() == dep.system_name) {
                    for (const auto& other_dep : other->dependencies()) {
                        if (other_dep.system_name == system->name()) {
                            LOG_ERROR("Circular dependency: {} <-> {}", system->name(), other->name());
                            return true;
                        }
                    }
                }
            }
        }
    }
    
    return false;
}

std::vector<std::vector<std::string>> SystemScheduler::get_execution_order(SystemPhase phase) const {
    auto systems = get_systems_for_phase(phase);
    
    // Group systems by dependency level
    std::vector<std::vector<std::string>> levels;
    std::unordered_set<std::string> completed;
    
    while (completed.size() < systems.size()) {
        std::vector<std::string> current_level;
        
        for (const System* system : systems) {
            if (completed.find(system->name()) != completed.end()) {
                continue;  // Already processed
            }
            
            // Check if all dependencies are completed
            bool all_deps_ready = true;
            for (const auto& dep : system->dependencies()) {
                if (completed.find(dep.system_name) == completed.end()) {
                    all_deps_ready = false;
                    break;
                }
            }
            
            if (all_deps_ready) {
                current_level.push_back(system->name());
            }
        }
        
        if (current_level.empty()) {
            // Circular dependency or other issue
            break;
        }
        
        levels.push_back(current_level);
        for (const auto& name : current_level) {
            completed.insert(name);
        }
    }
    
    return levels;
}

f64 SystemScheduler::estimate_phase_execution_time(SystemPhase phase) const {
    f64 total_time = 0.0;
    
    for (const auto& scheduled : scheduled_systems_) {
        if (scheduled.system->phase() == phase) {
            total_time += scheduled.system->get_average_execution_time();
        }
    }
    
    return total_time;
}

void SystemScheduler::set_thread_count(usize count) {
    thread_pool_ = std::make_unique<SystemThreadPool>(count);
    LOG_INFO("SystemScheduler thread pool resized to {} threads", count);
}

f64 SystemScheduler::get_average_scheduling_time() const {
    return scheduling_iterations_ > 0 ? 
        (total_scheduling_time_ / scheduling_iterations_) : 0.0;
}

// SystemManager implementation
SystemManager::SystemManager(Registry* registry)
    : registry_(registry)
    , scheduler_(std::make_unique<SystemScheduler>())
    , query_manager_(std::make_unique<QueryManager>())
    , is_running_(false)
    , current_frame_(0)
    , total_time_(0.0)
    , enable_parallel_execution_(true)
    , enable_performance_monitoring_(true)
    , max_systems_per_phase_(100) {
    
    // Initialize phase time budgets (60 FPS = 16.67ms per frame)
    phase_time_budgets_[static_cast<usize>(SystemPhase::Initialize)] = 1.0;      // 1 second for init
    phase_time_budgets_[static_cast<usize>(SystemPhase::PreUpdate)] = 0.001;     // 1ms
    phase_time_budgets_[static_cast<usize>(SystemPhase::Update)] = 0.010;        // 10ms
    phase_time_budgets_[static_cast<usize>(SystemPhase::PostUpdate)] = 0.001;    // 1ms
    phase_time_budgets_[static_cast<usize>(SystemPhase::PreRender)] = 0.001;     // 1ms
    phase_time_budgets_[static_cast<usize>(SystemPhase::Render)] = 0.004;        // 4ms
    phase_time_budgets_[static_cast<usize>(SystemPhase::PostRender)] = 0.001;    // 1ms
    phase_time_budgets_[static_cast<usize>(SystemPhase::Cleanup)] = 1.0;         // 1 second for cleanup
    
    LOG_INFO("SystemManager initialized");
}

SystemManager::~SystemManager() {
    if (is_running_.load()) {
        shutdown_all_systems();
    }
    
    LOG_INFO("SystemManager destroyed - {} systems, {} frames processed",
             system_count(), current_frame_.load());
}

void SystemManager::remove_system(const std::string& system_name) {
    auto it = systems_by_name_.find(system_name);
    if (it == systems_by_name_.end()) {
        return;
    }
    
    System* system = it->second;
    SystemPhase phase = system->phase();
    
    // Remove from scheduler
    scheduler_->remove_system(system_name);
    
    // Remove from phase storage
    auto& phase_systems = systems_by_phase_[static_cast<usize>(phase)];
    auto phase_it = std::find_if(phase_systems.begin(), phase_systems.end(),
                                [system_name](const auto& sys) {
                                    return sys->name() == system_name;
                                });
    
    if (phase_it != phase_systems.end()) {
        // Shutdown system before removal
        if (system->is_initialized()) {
            SystemContext context = create_system_context(phase, 0.0);
            system->shutdown(context);
        }
        
        phase_systems.erase(phase_it);
    }
    
    // Remove from name map
    systems_by_name_.erase(it);
    
    LOG_INFO("Removed system '{}'", system_name);
}

System* SystemManager::get_system(const std::string& system_name) {
    auto it = systems_by_name_.find(system_name);
    return it != systems_by_name_.end() ? it->second : nullptr;
}

SystemGroup* SystemManager::create_system_group(const std::string& name, SystemPhase phase, bool parallel) {
    auto group = std::make_unique<SystemGroup>(name, phase, parallel);
    SystemGroup* group_ptr = group.get();
    system_groups_.push_back(std::move(group));
    
    LOG_INFO("Created system group '{}' (phase: {}, parallel: {})", 
             name, static_cast<int>(phase), parallel);
    
    return group_ptr;
}

SystemGroup* SystemManager::get_system_group(const std::string& name) {
    auto it = std::find_if(system_groups_.begin(), system_groups_.end(),
                          [&](const auto& group) {
                              return group->name() == name;
                          });
    
    return it != system_groups_.end() ? it->get() : nullptr;
}

void SystemManager::initialize_all_systems() {
    LOG_INFO("Initializing all systems...");
    
    is_running_.store(true);
    total_time_ = core::get_time_seconds();
    
    // Initialize systems in dependency order
    execute_phase(SystemPhase::PreInitialize, 0.0);
    execute_phase(SystemPhase::Initialize, 0.0);
    execute_phase(SystemPhase::PostInitialize, 0.0);
    
    LOG_INFO("All systems initialized successfully");
}

void SystemManager::execute_phase(SystemPhase phase, f64 delta_time) {
    if (!is_running_.load()) {
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Update total time
    total_time_ += delta_time;
    
    // Create system context
    SystemContext context = create_system_context(phase, delta_time);
    
    // Execute systems through scheduler
    scheduler_->execute_phase(phase, context);
    
    // Execute system groups for this phase
    for (auto& group : system_groups_) {
        if (group->phase() == phase) {
            group->execute_all(context);
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 phase_time = std::chrono::duration<f64>(end_time - start_time).count();
    
    // Update phase statistics
    if (enable_performance_monitoring_) {
        auto& phase_stats = phase_stats_[static_cast<usize>(phase)];
        phase_stats.record_execution(phase_time);
        
        // Check budget violations
        check_budget_violations(phase);
    }
}

void SystemManager::shutdown_all_systems() {
    LOG_INFO("Shutting down all systems...");
    
    // Shutdown in reverse order
    execute_phase(SystemPhase::PreCleanup, 0.0);
    execute_phase(SystemPhase::Cleanup, 0.0);
    execute_phase(SystemPhase::PostCleanup, 0.0);
    
    is_running_.store(false);
    
    LOG_INFO("All systems shutdown successfully");
}

void SystemManager::execute_frame(f64 delta_time) {
    if (!is_running_.load()) {
        return;
    }
    
    current_frame_.fetch_add(1, std::memory_order_relaxed);
    
    // Execute frame phases
    execute_phase(SystemPhase::PreUpdate, delta_time);
    execute_phase(SystemPhase::Update, delta_time);
    execute_phase(SystemPhase::PostUpdate, delta_time);
    execute_phase(SystemPhase::PreRender, delta_time);
    execute_phase(SystemPhase::Render, delta_time);
    execute_phase(SystemPhase::PostRender, delta_time);
    
    // Update frame statistics
    if (enable_performance_monitoring_) {
        update_frame_statistics();
    }
}

void SystemManager::execute_fixed_update(f64 fixed_delta_time) {
    // Fixed update systems would be handled separately
    // For now, just execute update phase with fixed delta
    execute_phase(SystemPhase::Update, fixed_delta_time);
}

void SystemManager::set_phase_time_budget(SystemPhase phase, f64 budget) {
    phase_time_budgets_[static_cast<usize>(phase)] = budget;
    LOG_INFO("Set time budget for phase {} to {:.2f}ms", 
             static_cast<int>(phase), budget * 1000.0);
}

SystemContext SystemManager::create_system_context(SystemPhase phase, f64 delta_time) {
    // TODO: Create proper EventBus and ResourceManager instances
    return SystemContext(registry_, nullptr, nullptr, query_manager_.get(),
                        delta_time, total_time_, current_frame_.load(), phase);
}

void SystemManager::update_frame_statistics() {
    // Update global frame statistics
    // This would include overall frame timing, memory usage, etc.
}

void SystemManager::check_budget_violations(SystemPhase phase) {
    const auto& phase_stats = phase_stats_[static_cast<usize>(phase)];
    f64 budget = phase_time_budgets_[static_cast<usize>(phase)];
    
    if (phase_stats.last_execution_time > budget) {
        LOG_WARN("Phase {} exceeded budget: {:.2f}ms vs {:.2f}ms",
                static_cast<int>(phase), 
                phase_stats.last_execution_time * 1000.0,
                budget * 1000.0);
    }
}

SystemStats SystemManager::get_phase_stats(SystemPhase phase) const {
    return phase_stats_[static_cast<usize>(phase)];
}

std::vector<std::pair<std::string, SystemStats>> SystemManager::get_all_system_stats() const {
    std::vector<std::pair<std::string, SystemStats>> all_stats;
    
    for (const auto& [name, system] : systems_by_name_) {
        all_stats.emplace_back(name, system->statistics());
    }
    
    return all_stats;
}

std::vector<std::string> SystemManager::get_slowest_systems(usize count) const {
    auto all_stats = get_all_system_stats();
    
    std::sort(all_stats.begin(), all_stats.end(),
             [](const auto& a, const auto& b) {
                 return a.second.average_execution_time > b.second.average_execution_time;
             });
    
    std::vector<std::string> slowest;
    usize limit = std::min(count, all_stats.size());
    
    for (usize i = 0; i < limit; ++i) {
        slowest.push_back(all_stats[i].first);
    }
    
    return slowest;
}

std::vector<std::string> SystemManager::get_systems_over_budget() const {
    std::vector<std::string> over_budget;
    
    for (const auto& [name, system] : systems_by_name_) {
        if (system->is_over_budget()) {
            over_budget.push_back(name);
        }
    }
    
    return over_budget;
}

f64 SystemManager::get_total_system_time() const {
    f64 total = 0.0;
    
    for (const auto& stats : phase_stats_) {
        total += stats.total_execution_time;
    }
    
    return total;
}

f64 SystemManager::get_frame_budget_utilization() const {
    f64 total_budget = 0.0;
    f64 total_used = 0.0;
    
    for (usize i = 0; i < static_cast<usize>(SystemPhase::COUNT); ++i) {
        total_budget += phase_time_budgets_[i];
        total_used += phase_stats_[i].last_execution_time;
    }
    
    return total_budget > 0.0 ? (total_used / total_budget) : 0.0;
}

void SystemManager::print_system_execution_order() const {
    LOG_INFO("=== System Execution Order ===");
    
    for (usize i = 0; i < static_cast<usize>(SystemPhase::COUNT); ++i) {
        SystemPhase phase = static_cast<SystemPhase>(i);
        auto execution_order = scheduler_->get_execution_order(phase);
        
        if (!execution_order.empty()) {
            LOG_INFO("Phase {}: {}", static_cast<int>(phase), phase_name(phase));
            
            for (usize level = 0; level < execution_order.size(); ++level) {
                std::string systems_str;
                for (const auto& system_name : execution_order[level]) {
                    if (!systems_str.empty()) systems_str += ", ";
                    systems_str += system_name;
                }
                LOG_INFO("  Level {}: [{}]", level, systems_str);
            }
        }
    }
}

void SystemManager::print_system_performance_report() const {
    LOG_INFO("=== System Performance Report ===");
    
    auto all_stats = get_all_system_stats();
    
    // Sort by execution time
    std::sort(all_stats.begin(), all_stats.end(),
             [](const auto& a, const auto& b) {
                 return a.second.average_execution_time > b.second.average_execution_time;
             });
    
    LOG_INFO("Top 10 slowest systems:");
    for (usize i = 0; i < std::min<usize>(10, all_stats.size()); ++i) {
        const auto& [name, stats] = all_stats[i];
        LOG_INFO("  {}: {:.2f}ms avg, {} executions, {:.1f}% budget",
                name, stats.average_execution_time * 1000.0,
                stats.total_executions, stats.budget_utilization * 100.0);
    }
    
    LOG_INFO("Overall frame budget utilization: {:.1f}%", 
             get_frame_budget_utilization() * 100.0);
}

const char* SystemManager::phase_name(SystemPhase phase) {
    switch (phase) {
        case SystemPhase::PreInitialize: return "PreInitialize";
        case SystemPhase::Initialize: return "Initialize";
        case SystemPhase::PostInitialize: return "PostInitialize";
        case SystemPhase::PreUpdate: return "PreUpdate";
        case SystemPhase::Update: return "Update";
        case SystemPhase::PostUpdate: return "PostUpdate";
        case SystemPhase::PreRender: return "PreRender";
        case SystemPhase::Render: return "Render";
        case SystemPhase::PostRender: return "PostRender";
        case SystemPhase::PreCleanup: return "PreCleanup";
        case SystemPhase::Cleanup: return "Cleanup";
        case SystemPhase::PostCleanup: return "PostCleanup";
        default: return "Unknown";
    }
}

} // namespace ecscope::ecs