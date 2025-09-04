/**
 * @file work_stealing_job_system.cpp
 * @brief Implementation of Work-Stealing Job System
 */

#include "work_stealing_job_system.hpp"
#include "memory/numa_manager.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <cassert>

namespace ecscope::job_system {

//=============================================================================
// Job Implementation
//=============================================================================

Job::Job(JobID id, std::string name, JobFunction function, 
         JobPriority priority, JobAffinity affinity)
    : id_(id), name_(std::move(name)), function_(std::move(function))
    , priority_(priority), affinity_(affinity), state_(JobState::Pending)
    , pending_dependencies_(0), preferred_core_(0), preferred_numa_node_(0)
    , estimated_duration_us_(1000), memory_requirement_bytes_(0)
    , completion_future_(completion_promise_.get_future()) {
    
    stats_.creation_time = std::chrono::high_resolution_clock::now();
}

void Job::execute() {
    auto start_time = std::chrono::high_resolution_clock::now();
    stats_.start_time = start_time;
    set_state(JobState::Running);
    
    try {
        if (function_) {
            function_();
        }
        set_state(JobState::Completed);
    } catch (const std::exception& e) {
        LOG_ERROR("Job '{}' failed with exception: {}", name_, e.what());
        set_state(JobState::Failed);
    } catch (...) {
        LOG_ERROR("Job '{}' failed with unknown exception", name_);
        set_state(JobState::Failed);
    }
    
    stats_.end_time = std::chrono::high_resolution_clock::now();
    update_stats();
    
    // Notify completion
    completion_promise_.set_value();
}

void Job::cancel() {
    JobState expected = JobState::Pending;
    if (state_.compare_exchange_strong(expected, JobState::Cancelled, std::memory_order_release)) {
        completion_promise_.set_value();
        LOG_DEBUG("Job '{}' cancelled", name_);
    }
}

bool Job::is_ready() const noexcept {
    return pending_dependencies_.load(std::memory_order_acquire) == 0 &&
           state_.load(std::memory_order_acquire) == JobState::Pending;
}

bool Job::is_complete() const noexcept {
    JobState state = state_.load(std::memory_order_acquire);
    return state == JobState::Completed || state == JobState::Failed || state == JobState::Cancelled;
}

void Job::wait() const {
    completion_future_.wait();
}

bool Job::wait_for(std::chrono::milliseconds timeout) const {
    return completion_future_.wait_for(timeout) == std::future_status::ready;
}

void Job::add_dependency(JobID dependency) {
    dependencies_.push_back(dependency);
    pending_dependencies_.fetch_add(1, std::memory_order_acq_rel);
}

void Job::remove_dependency(JobID dependency) {
    auto it = std::find(dependencies_.begin(), dependencies_.end(), dependency);
    if (it != dependencies_.end()) {
        dependencies_.erase(it);
        pending_dependencies_.fetch_sub(1, std::memory_order_acq_rel);
    }
}

void Job::notify_dependency_complete(JobID dependency) {
    auto it = std::find(dependencies_.begin(), dependencies_.end(), dependency);
    if (it != dependencies_.end()) {
        u32 old_count = pending_dependencies_.fetch_sub(1, std::memory_order_acq_rel);
        if (old_count == 1) {
            // This was the last dependency - job is now ready
            set_state(JobState::Ready);
        }
    }
}

bool Job::has_dependencies() const noexcept {
    return pending_dependencies_.load(std::memory_order_acquire) > 0;
}

Job& Job::set_estimated_duration(std::chrono::microseconds duration) {
    estimated_duration_us_ = duration.count();
    return *this;
}

void Job::update_stats() {
    // Update CPU and NUMA info
    #ifdef __linux__
    stats_.cpu_core = sched_getcpu();
    stats_.numa_node = numa_node_of_cpu(stats_.cpu_core);
    #endif
}

//=============================================================================
// WorkStealingQueue Implementation
//=============================================================================

WorkStealingQueue::WorkStealingQueue(u32 owner_id, const std::string& name, usize initial_capacity)
    : buffer_(std::make_unique<Buffer>(initial_capacity))
    , top_(0), bottom_(0), owner_thread_id_(owner_id), queue_name_(name) {
    
    LOG_DEBUG("Created work-stealing queue '{}' for thread {} with capacity {}", 
             name, owner_id, initial_capacity);
}

WorkStealingQueue::~WorkStealingQueue() {
    LOG_DEBUG("Destroyed work-stealing queue '{}' - Stats: {} pushes, {} pops, {} steals, {:.2f}% steal success",
             queue_name_, pushes_.load(), pops_.load(), steals_.load(), steal_success_rate() * 100.0);
}

bool WorkStealingQueue::push(Job* job) noexcept {
    if (!job) return false;
    
    isize b = bottom_.load(std::memory_order_relaxed);
    isize t = top_.load(std::memory_order_acquire);
    
    // Check if queue is full (need to grow)
    if (b - t >= static_cast<isize>(buffer_->capacity - 1)) {
        grow_buffer();
    }
    
    buffer_->put(b, job);
    
    // Ensure the job is written before we update bottom
    std::atomic_thread_fence(std::memory_order_release);
    bottom_.store(b + 1, std::memory_order_relaxed);
    
    pushes_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

Job* WorkStealingQueue::pop() noexcept {
    isize b = bottom_.load(std::memory_order_relaxed) - 1;
    bottom_.store(b, std::memory_order_relaxed);
    
    std::atomic_thread_fence(std::memory_order_seq_cst);
    
    isize t = top_.load(std::memory_order_relaxed);
    Job* job = nullptr;
    
    if (t <= b) {
        // Non-empty queue
        job = buffer_->get(b);
        
        if (t == b) {
            // This is the last element, compete with thieves
            if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                // A thief got it
                job = nullptr;
            }
            bottom_.store(b + 1, std::memory_order_relaxed);
        }
    } else {
        // Empty queue
        bottom_.store(b + 1, std::memory_order_relaxed);
    }
    
    if (job) {
        pops_.fetch_add(1, std::memory_order_relaxed);
    }
    
    return job;
}

Job* WorkStealingQueue::steal() noexcept {
    steal_attempts_.fetch_add(1, std::memory_order_relaxed);
    
    isize t = top_.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    isize b = bottom_.load(std::memory_order_acquire);
    
    Job* job = nullptr;
    if (t < b) {
        // Non-empty, try to steal
        job = buffer_->get(t);
        if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
            // Failed to steal (another thief got it, or owner popped it)
            job = nullptr;
        } else {
            steals_.fetch_add(1, std::memory_order_relaxed);
        }
    }
    
    return job;
}

bool WorkStealingQueue::empty() const noexcept {
    isize t = top_.load(std::memory_order_acquire);
    isize b = bottom_.load(std::memory_order_acquire);
    return t >= b;
}

usize WorkStealingQueue::size() const noexcept {
    isize t = top_.load(std::memory_order_acquire);
    isize b = bottom_.load(std::memory_order_acquire);
    return std::max(static_cast<isize>(0), b - t);
}

f64 WorkStealingQueue::steal_success_rate() const noexcept {
    u64 attempts = steal_attempts_.load(std::memory_order_relaxed);
    u64 successes = steals_.load(std::memory_order_relaxed);
    return attempts > 0 ? static_cast<f64>(successes) / attempts : 0.0;
}

void WorkStealingQueue::grow_buffer() {
    auto new_buffer = buffer_->grow();
    buffer_ = std::move(new_buffer);
    LOG_DEBUG("Grew work-stealing queue '{}' to capacity {}", queue_name_, buffer_->capacity);
}

//=============================================================================
// TaskDependencyGraph Implementation
//=============================================================================

usize TaskDependencyGraph::add_job(JobID job_id, const std::string& name, 
                                  JobPriority priority, usize estimated_duration_us,
                                  usize memory_requirement) {
    usize index = nodes_.size();
    nodes_.emplace_back(job_id, name, priority);
    
    Node& node = nodes_.back();
    node.estimated_duration_us = estimated_duration_us;
    node.memory_requirement = memory_requirement;
    
    job_to_node_[job_id] = index;
    total_estimated_time_us_ += estimated_duration_us;
    
    return index;
}

bool TaskDependencyGraph::add_dependency(JobID dependent, JobID dependency) {
    auto dependent_it = job_to_node_.find(dependent);
    auto dependency_it = job_to_node_.find(dependency);
    
    if (dependent_it == job_to_node_.end() || dependency_it == job_to_node_.end()) {
        return false;
    }
    
    usize dependent_idx = dependent_it->second;
    usize dependency_idx = dependency_it->second;
    
    // Add dependency relationship
    nodes_[dependent_idx].dependencies.push_back(dependency_idx);
    nodes_[dependency_idx].dependents.push_back(dependent_idx);
    nodes_[dependent_idx].pending_dependencies.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool TaskDependencyGraph::remove_dependency(JobID dependent, JobID dependency) {
    auto dependent_it = job_to_node_.find(dependent);
    auto dependency_it = job_to_node_.find(dependency);
    
    if (dependent_it == job_to_node_.end() || dependency_it == job_to_node_.end()) {
        return false;
    }
    
    usize dependent_idx = dependent_it->second;
    usize dependency_idx = dependency_it->second;
    
    // Remove from dependencies
    auto& deps = nodes_[dependent_idx].dependencies;
    auto dep_it = std::find(deps.begin(), deps.end(), dependency_idx);
    if (dep_it != deps.end()) {
        deps.erase(dep_it);
        nodes_[dependent_idx].pending_dependencies.fetch_sub(1, std::memory_order_relaxed);
    }
    
    // Remove from dependents
    auto& dependents = nodes_[dependency_idx].dependents;
    auto dependent_dep_it = std::find(dependents.begin(), dependents.end(), dependent_idx);
    if (dependent_dep_it != dependents.end()) {
        dependents.erase(dependent_dep_it);
    }
    
    return true;
}

void TaskDependencyGraph::clear() {
    nodes_.clear();
    job_to_node_.clear();
    levels_.clear();
    while (!ready_queue_.empty()) ready_queue_.pop();
    max_depth_ = 0;
    total_estimated_time_us_ = 0;
    critical_path_time_us_ = 0;
    parallelism_factor_ = 1.0;
}

bool TaskDependencyGraph::build_schedule() {
    if (nodes_.empty()) return true;
    
    // Check for cycles
    auto cycles = detect_cycles();
    if (!cycles.empty()) {
        LOG_ERROR("Dependency cycles detected: {}", fmt::join(cycles, ", "));
        return false;
    }
    
    // Calculate dependency levels and critical path
    calculate_dependency_levels();
    calculate_critical_path();
    update_parallelism_metrics();
    
    // Build initial ready queue
    for (usize i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].pending_dependencies.load() == 0) {
            ready_queue_.push(i);
            nodes_[i].is_ready = true;
        }
    }
    
    return true;
}

std::vector<std::string> TaskDependencyGraph::detect_cycles() const {
    std::vector<std::string> cycles;
    std::vector<bool> visited(nodes_.size(), false);
    std::vector<bool> rec_stack(nodes_.size(), false);
    
    for (usize i = 0; i < nodes_.size(); ++i) {
        if (!visited[i] && has_cycle_util(i, visited, rec_stack)) {
            cycles.push_back(nodes_[i].job_name);
        }
    }
    
    return cycles;
}

std::vector<usize> TaskDependencyGraph::get_ready_jobs() {
    std::vector<usize> ready_jobs;
    
    while (!ready_queue_.empty()) {
        usize job_idx = ready_queue_.front();
        ready_queue_.pop();
        
        if (!nodes_[job_idx].is_scheduled) {
            ready_jobs.push_back(job_idx);
            nodes_[job_idx].is_scheduled = true;
        }
    }
    
    return ready_jobs;
}

void TaskDependencyGraph::mark_job_complete(JobID job_id) {
    auto it = job_to_node_.find(job_id);
    if (it == job_to_node_.end()) return;
    
    usize job_idx = it->second;
    Node& node = nodes_[job_idx];
    node.is_complete = true;
    
    // Notify dependents
    for (usize dependent_idx : node.dependents) {
        Node& dependent = nodes_[dependent_idx];
        u32 old_deps = dependent.pending_dependencies.fetch_sub(1, std::memory_order_acq_rel);
        
        if (old_deps == 1) {
            // This dependent is now ready
            ready_queue_.push(dependent_idx);
            dependent.is_ready = true;
        }
    }
}

bool TaskDependencyGraph::all_jobs_complete() const {
    return std::all_of(nodes_.begin(), nodes_.end(),
                      [](const Node& node) { return node.is_complete; });
}

void TaskDependencyGraph::calculate_dependency_levels() {
    levels_.clear();
    
    // Topological sort to assign levels
    std::vector<usize> in_degree(nodes_.size());
    for (usize i = 0; i < nodes_.size(); ++i) {
        in_degree[i] = nodes_[i].dependencies.size();
    }
    
    std::queue<usize> level_queue;
    for (usize i = 0; i < nodes_.size(); ++i) {
        if (in_degree[i] == 0) {
            level_queue.push(i);
            nodes_[i].depth_level = 0;
        }
    }
    
    max_depth_ = 0;
    while (!level_queue.empty()) {
        usize current = level_queue.front();
        level_queue.pop();
        
        u32 current_level = nodes_[current].depth_level;
        
        // Ensure we have enough levels
        while (levels_.size() <= current_level) {
            levels_.emplace_back();
        }
        levels_[current_level].push_back(current);
        max_depth_ = std::max(max_depth_, current_level);
        
        // Process dependents
        for (usize dependent_idx : nodes_[current].dependents) {
            nodes_[dependent_idx].depth_level = std::max(nodes_[dependent_idx].depth_level, 
                                                        current_level + 1);
            in_degree[dependent_idx]--;
            if (in_degree[dependent_idx] == 0) {
                level_queue.push(dependent_idx);
            }
        }
    }
}

void TaskDependencyGraph::calculate_critical_path() {
    critical_path_time_us_ = 0;
    
    // Calculate critical path using dynamic programming
    std::vector<usize> critical_path_length(nodes_.size(), 0);
    
    // Process nodes in reverse topological order (from leaves to roots)
    for (int level = static_cast<int>(levels_.size()) - 1; level >= 0; --level) {
        for (usize node_idx : levels_[level]) {
            Node& node = nodes_[node_idx];
            
            // Calculate the longest path from this node to a leaf
            usize max_dependent_path = 0;
            for (usize dependent_idx : node.dependents) {
                max_dependent_path = std::max(max_dependent_path, critical_path_length[dependent_idx]);
            }
            
            critical_path_length[node_idx] = node.estimated_duration_us + max_dependent_path;
            node.critical_path_length = critical_path_length[node_idx];
            
            // Update global critical path if this is a root node
            if (node.dependencies.empty()) {
                critical_path_time_us_ = std::max(critical_path_time_us_, critical_path_length[node_idx]);
            }
        }
    }
}

bool TaskDependencyGraph::has_cycle_util(usize node, std::vector<bool>& visited, 
                                        std::vector<bool>& rec_stack) const {
    visited[node] = true;
    rec_stack[node] = true;
    
    for (usize dependent_idx : nodes_[node].dependents) {
        if (!visited[dependent_idx] && has_cycle_util(dependent_idx, visited, rec_stack)) {
            return true;
        } else if (rec_stack[dependent_idx]) {
            return true;
        }
    }
    
    rec_stack[node] = false;
    return false;
}

void TaskDependencyGraph::update_parallelism_metrics() {
    if (critical_path_time_us_ == 0 || total_estimated_time_us_ == 0) {
        parallelism_factor_ = 1.0;
        return;
    }
    
    parallelism_factor_ = static_cast<f64>(total_estimated_time_us_) / critical_path_time_us_;
}

f64 TaskDependencyGraph::calculate_parallelism_potential() const {
    return parallelism_factor_;
}

usize TaskDependencyGraph::calculate_critical_path_length() const {
    return critical_path_time_us_;
}

const TaskDependencyGraph::Node* TaskDependencyGraph::find_node(JobID job_id) const {
    auto it = job_to_node_.find(job_id);
    return it != job_to_node_.end() ? &nodes_[it->second] : nullptr;
}

std::string TaskDependencyGraph::export_graphviz() const {
    std::ostringstream oss;
    oss << "digraph TaskDependencyGraph {\n";
    oss << "  rankdir=TB;\n";
    oss << "  node [shape=box, style=filled];\n\n";
    
    // Nodes
    for (usize i = 0; i < nodes_.size(); ++i) {
        const Node& node = nodes_[i];
        std::string color = "lightblue";
        
        if (node.is_complete) color = "lightgreen";
        else if (node.is_ready) color = "yellow";
        else if (node.pending_dependencies.load() > 0) color = "lightcoral";
        
        oss << "  node_" << i << " [label=\"" << node.job_name << "\\n"
            << node.estimated_duration_us << "μs\", fillcolor=" << color << "];\n";
    }
    
    oss << "\n";
    
    // Edges
    for (usize i = 0; i < nodes_.size(); ++i) {
        for (usize dep_idx : nodes_[i].dependencies) {
            oss << "  node_" << dep_idx << " -> node_" << i << ";\n";
        }
    }
    
    oss << "}\n";
    return oss.str();
}

//=============================================================================
// WorkerThread Implementation
//=============================================================================

WorkerThread::WorkerThread(u32 worker_id, u32 cpu_core, u32 numa_node, JobSystem* job_system)
    : worker_id_(worker_id), cpu_core_(cpu_core), numa_node_(numa_node), job_system_(job_system)
    , last_activity_(std::chrono::high_resolution_clock::now())
    , rng_(std::random_device{}()) {
    
    std::string queue_name = "Worker_" + std::to_string(worker_id) + "_Queue";
    local_queue_ = std::make_unique<WorkStealingQueue>(worker_id, queue_name);
    
    // Initialize steal distribution for other workers
    if (job_system_->worker_count_ > 1) {
        steal_distribution_ = std::uniform_int_distribution<u32>(0, job_system_->worker_count_ - 1);
    }
}

WorkerThread::~WorkerThread() {
    if (thread_.joinable()) {
        stop();
        join();
    }
}

void WorkerThread::start() {
    if (is_running_.load(std::memory_order_acquire)) {
        return;
    }
    
    should_stop_.store(false, std::memory_order_release);
    thread_ = std::thread(&WorkerThread::worker_main, this);
    
    LOG_DEBUG("Started worker thread {} on CPU core {} (NUMA node {})", 
             worker_id_, cpu_core_, numa_node_);
}

void WorkerThread::stop() {
    should_stop_.store(true, std::memory_order_release);
    LOG_DEBUG("Stopping worker thread {}", worker_id_);
}

void WorkerThread::join() {
    if (thread_.joinable()) {
        thread_.join();
        LOG_DEBUG("Worker thread {} joined", worker_id_);
    }
}

bool WorkerThread::submit_job(Job* job) {
    if (!job || should_stop_.load(std::memory_order_acquire)) {
        return false;
    }
    
    return local_queue_->push(job);
}

f64 WorkerThread::steal_success_rate() const noexcept {
    u64 attempts = steal_attempts_.load(std::memory_order_relaxed);
    u64 successes = jobs_stolen_.load(std::memory_order_relaxed);
    return attempts > 0 ? static_cast<f64>(successes) / attempts : 0.0;
}

void WorkerThread::worker_main() {
    is_running_.store(true, std::memory_order_release);
    
    // Set CPU affinity and NUMA policy
    set_cpu_affinity();
    update_numa_policy();
    
    LOG_DEBUG("Worker thread {} started main loop", worker_id_);
    
    u32 idle_count = 0;
    const u32 max_idle_before_sleep = 1000;
    
    while (!should_stop_.load(std::memory_order_acquire)) {
        Job* job = find_work();
        
        if (job) {
            execute_job(job);
            idle_count = 0;
            last_activity_ = std::chrono::high_resolution_clock::now();
        } else {
            idle_count++;
            idle_cycles_.fetch_add(1, std::memory_order_relaxed);
            
            if (idle_count > max_idle_before_sleep) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                idle_count = 0;
            } else {
                std::this_thread::yield();
            }
        }
    }
    
    is_running_.store(false, std::memory_order_release);
    LOG_DEBUG("Worker thread {} exiting - executed {} jobs, stole {} jobs", 
             worker_id_, jobs_executed_.load(), jobs_stolen_.load());
}

Job* WorkerThread::find_work() {
    // First, try to get work from local queue
    Job* job = local_queue_->pop();
    if (job) {
        return job;
    }
    
    // If no local work, try to steal from other workers
    job = steal_work();
    if (job) {
        jobs_stolen_.fetch_add(1, std::memory_order_relaxed);
        return job;
    }
    
    // Finally, try the global queue
    return job_system_->global_queue_->steal();
}

Job* WorkerThread::steal_work() {
    if (job_system_->worker_count_ <= 1) {
        return nullptr;
    }
    
    const u32 max_steal_attempts = job_system_->worker_count_ * 2;
    
    for (u32 attempt = 0; attempt < max_steal_attempts; ++attempt) {
        steal_attempts_.fetch_add(1, std::memory_order_relaxed);
        
        u32 target_worker = select_steal_target();
        if (target_worker == worker_id_) {
            continue; // Don't steal from ourselves
        }
        
        Job* stolen_job = job_system_->workers_[target_worker]->local_queue_->steal();
        if (stolen_job) {
            return stolen_job;
        }
    }
    
    return nullptr;
}

void WorkerThread::execute_job(Job* job) {
    if (!job) return;
    
    current_job_.store(job, std::memory_order_release);
    
    // Update job statistics
    job->stats_.worker_id = worker_id_;
    job->stats_.cpu_core = cpu_core_;
    job->stats_.numa_node = numa_node_;
    
    // Execute the job
    job->execute();
    
    jobs_executed_.fetch_add(1, std::memory_order_relaxed);
    current_job_.store(nullptr, std::memory_order_release);
    
    // Notify job system of completion for dependency tracking
    // This would need to be implemented in JobSystem
}

void WorkerThread::set_cpu_affinity() {
    #ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_core_, &cpuset);
    
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        LOG_WARN("Failed to set CPU affinity for worker thread {} to core {}", 
                worker_id_, cpu_core_);
    } else {
        LOG_DEBUG("Set CPU affinity for worker thread {} to core {}", 
                 worker_id_, cpu_core_);
    }
    #endif
}

void WorkerThread::update_numa_policy() {
    #ifdef __linux__
    // Set NUMA memory policy to prefer local node
    if (numa_available() == 0) {
        struct bitmask* nodemask = numa_allocate_nodemask();
        numa_bitmask_setbit(nodemask, numa_node_);
        
        if (numa_set_membind(nodemask) != 0) {
            LOG_WARN("Failed to set NUMA memory policy for worker thread {} to node {}", 
                    worker_id_, numa_node_);
        } else {
            LOG_DEBUG("Set NUMA memory policy for worker thread {} to node {}", 
                     worker_id_, numa_node_);
        }
        
        numa_free_nodemask(nodemask);
    }
    #endif
}

u32 WorkerThread::select_steal_target() const {
    if (job_system_->worker_count_ <= 1) {
        return 0;
    }
    
    // Use random selection for now
    // Could be improved with topology-aware selection
    u32 target;
    do {
        target = steal_distribution_(rng_);
    } while (target == worker_id_);
    
    return target;
}

//=============================================================================
// JobSystem Implementation
//=============================================================================

JobSystem::JobSystem(const Config& config)
    : worker_count_(config.worker_count == 0 ? 
                   std::max(1u, std::thread::hardware_concurrency() - 1) : config.worker_count)
    , enable_work_stealing_(config.enable_work_stealing)
    , enable_numa_awareness_(config.enable_numa_awareness)
    , enable_cpu_affinity_(config.enable_cpu_affinity)
    , enable_profiling_(config.enable_profiling)
    , enable_visualization_(config.enable_visualization)
    , steal_attempts_before_yield_(config.steal_attempts_before_yield)
    , idle_sleep_duration_(config.idle_sleep_duration) {
    
    // Initialize job pool
    job_pool_.reserve(config.initial_job_pool_size);
    for (usize i = 0; i < config.initial_job_pool_size; ++i) {
        job_pool_.push_back(nullptr);
        free_job_slots_.push(i);
    }
    
    // Initialize dependency graph
    dependency_graph_ = std::make_unique<TaskDependencyGraph>();
    
    // Create global queue
    global_queue_ = std::make_unique<WorkStealingQueue>(0, "GlobalQueue");
    
    LOG_INFO("Created JobSystem with {} workers", worker_count_);
}

JobSystem::~JobSystem() {
    if (is_initialized()) {
        shutdown();
    }
}

bool JobSystem::initialize() {
    if (is_initialized()) {
        LOG_WARN("JobSystem already initialized");
        return true;
    }
    
    LOG_INFO("Initializing JobSystem...");
    
    // Initialize NUMA if available
    if (enable_numa_awareness_) {
        #ifdef __linux__
        if (numa_available() == 0) {
            LOG_INFO("NUMA support available with {} nodes", numa_num_configured_nodes());
        }
        #endif
    }
    
    // Create worker threads
    workers_.reserve(worker_count_);
    for (u32 i = 0; i < worker_count_; ++i) {
        u32 cpu_core = i % std::thread::hardware_concurrency();
        u32 numa_node = 0;
        
        #ifdef __linux__
        if (enable_numa_awareness_ && numa_available() == 0) {
            numa_node = numa_node_of_cpu(cpu_core);
        }
        #endif
        
        auto worker = std::make_unique<WorkerThread>(i, cpu_core, numa_node, this);
        workers_.push_back(std::move(worker));
    }
    
    // Start all workers
    for (auto& worker : workers_) {
        worker->start();
    }
    
    is_initialized_.store(true, std::memory_order_release);
    LOG_INFO("JobSystem initialized successfully with {} workers", worker_count_);
    
    return true;
}

void JobSystem::shutdown() {
    if (!is_initialized()) {
        return;
    }
    
    LOG_INFO("Shutting down JobSystem...");
    is_shutting_down_.store(true, std::memory_order_release);
    
    // Wait for all jobs to complete
    wait_for_all();
    
    // Stop and join all workers
    for (auto& worker : workers_) {
        worker->stop();
    }
    
    for (auto& worker : workers_) {
        worker->join();
    }
    
    workers_.clear();
    
    // Clean up job pool
    {
        std::lock_guard<std::mutex> lock(job_pool_mutex_);
        for (auto& job : job_pool_) {
            if (job) {
                job.reset();
            }
        }
        job_pool_.clear();
        while (!free_job_slots_.empty()) {
            free_job_slots_.pop();
        }
    }
    
    is_initialized_.store(false, std::memory_order_release);
    is_shutting_down_.store(false, std::memory_order_release);
    
    LOG_INFO("JobSystem shutdown complete");
}

JobID JobSystem::allocate_job_id() {
    static std::atomic<u32> job_counter{1};
    u32 index = job_counter.fetch_add(1, std::memory_order_relaxed);
    u16 generation = job_generation_counter_.fetch_add(1, std::memory_order_relaxed);
    if (generation == 0) {
        generation = job_generation_counter_.fetch_add(1, std::memory_order_relaxed);
    }
    return JobID{index, generation};
}

Job* JobSystem::allocate_job(JobID id, const std::string& name, JobFunction function,
                            JobPriority priority, JobAffinity affinity) {
    std::lock_guard<std::mutex> lock(job_pool_mutex_);
    
    if (free_job_slots_.empty()) {
        // Expand job pool
        usize old_size = job_pool_.size();
        job_pool_.resize(old_size * 2);
        
        for (usize i = old_size; i < job_pool_.size(); ++i) {
            job_pool_[i] = nullptr;
            free_job_slots_.push(i);
        }
    }
    
    usize slot = free_job_slots_.front();
    free_job_slots_.pop();
    
    job_pool_[slot] = std::make_unique<Job>(id, name, std::move(function), priority, affinity);
    return job_pool_[slot].get();
}

void JobSystem::deallocate_job(Job* job) {
    if (!job) return;
    
    std::lock_guard<std::mutex> lock(job_pool_mutex_);
    
    // Find the job in the pool and free its slot
    for (usize i = 0; i < job_pool_.size(); ++i) {
        if (job_pool_[i].get() == job) {
            job_pool_[i].reset();
            free_job_slots_.push(i);
            break;
        }
    }
}

WorkerThread* JobSystem::select_worker_for_job(const Job& job) {
    if (workers_.empty()) {
        return nullptr;
    }
    
    // Simple round-robin selection for now
    // Could be improved with load balancing and affinity consideration
    static std::atomic<u32> worker_counter{0};
    u32 worker_index = worker_counter.fetch_add(1, std::memory_order_relaxed) % workers_.size();
    return workers_[worker_index].get();
}

void JobSystem::notify_workers() {
    // Workers will naturally pick up work through their polling loops
    // Could be optimized with condition variables for immediate wakeup
}

u32 JobSystem::active_job_count() const noexcept {
    u32 active = 0;
    for (const auto& worker : workers_) {
        if (!worker->is_idle()) {
            active++;
        }
    }
    return active;
}

u32 JobSystem::pending_job_count() const noexcept {
    u32 pending = 0;
    
    // Count jobs in global queue
    pending += global_queue_->size();
    
    // Count jobs in worker queues
    for (const auto& worker : workers_) {
        pending += worker->queue().size();
    }
    
    return pending;
}

bool JobSystem::all_workers_idle() const noexcept {
    return std::all_of(workers_.begin(), workers_.end(),
                      [](const auto& worker) { return worker->is_idle(); });
}

void JobSystem::wait_for_all() {
    // Wait until all queues are empty and all workers are idle
    while (pending_job_count() > 0 || !all_workers_idle()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

JobSystem::SystemStats JobSystem::get_system_statistics() const {
    SystemStats stats;
    stats.measurement_start = std::chrono::high_resolution_clock::now();
    
    // Aggregate worker statistics
    for (const auto& worker : workers_) {
        stats.total_jobs_completed += worker->jobs_executed();
        stats.total_steals += worker->jobs_stolen();
        stats.total_steal_attempts += worker->steal_attempts();
    }
    
    // Calculate rates and utilization
    if (stats.total_steal_attempts > 0) {
        stats.overall_steal_success_rate = static_cast<f64>(stats.total_steals) / stats.total_steal_attempts;
    }
    
    // Worker utilization calculation would require more detailed timing
    stats.worker_utilization_percent = 50.0; // Placeholder
    
    stats.measurement_end = std::chrono::high_resolution_clock::now();
    return stats;
}

void JobSystem::reset_statistics() {
    // Reset statistics in all workers
    // This would require additional methods in WorkerThread
}

std::string JobSystem::generate_performance_report() const {
    auto stats = get_system_statistics();
    
    std::ostringstream report;
    report << "=== JobSystem Performance Report ===\n";
    report << "Workers: " << worker_count_ << "\n";
    report << "Configuration:\n";
    report << "  Work Stealing: " << (enable_work_stealing_ ? "Enabled" : "Disabled") << "\n";
    report << "  NUMA Awareness: " << (enable_numa_awareness_ ? "Enabled" : "Disabled") << "\n";
    report << "  CPU Affinity: " << (enable_cpu_affinity_ ? "Enabled" : "Disabled") << "\n";
    report << "\nStatistics:\n";
    report << "  Jobs Completed: " << stats.total_jobs_completed << "\n";
    report << "  Jobs Stolen: " << stats.total_steals << "\n";
    report << "  Steal Success Rate: " << (stats.overall_steal_success_rate * 100.0) << "%\n";
    report << "  Worker Utilization: " << stats.worker_utilization_percent << "%\n";
    
    return report.str();
}

} // namespace ecscope::job_system