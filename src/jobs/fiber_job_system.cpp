#include "jobs/fiber_job_system.hpp"
#include "jobs/job_dependency_graph.hpp"
#include "jobs/job_profiler.hpp"
#include <algorithm>
#include <thread>
#include <random>

namespace ecscope::jobs {

//=============================================================================
// Fiber Job Implementation
//=============================================================================

FiberJob::FiberJob(JobID id, std::string name, JobFunction function,
                   JobPriority priority, JobAffinity affinity,
                   const FiberStackConfig& stack_config)
    : id_(id)
    , name_(std::move(name))
    , function_(std::move(function))
    , state_(JobState::Created)
    , priority_(priority)
    , affinity_(affinity)
    , fiber_(nullptr)
    , stack_config_(stack_config)
    , requires_large_stack_(false)
    , pending_dependencies_(0)
    , preferred_worker_(0)
    , preferred_core_(0)
    , preferred_numa_node_(0)
    , estimated_duration_(1000)
    , memory_requirement_(0)
    , completion_promise_()
    , completion_future_(completion_promise_.get_future())
    , completion_callback_(nullptr)
    , can_be_stolen_(true)
    , steal_resistance_(0) {
    
    stats_.creation_time = std::chrono::high_resolution_clock::now();
}

FiberJob::~FiberJob() {
    cleanup_fiber();
}

void FiberJob::execute_in_fiber() {
    if (state() != JobState::Ready) {
        return;
    }
    
    set_state(JobState::Running);
    stats_.start_time = std::chrono::high_resolution_clock::now();
    
    try {
        if (!fiber_) {
            initialize_fiber();
        }
        
        if (fiber_) {
            fiber_->start();
            
            if (fiber_->is_finished()) {
                set_state(JobState::Completed);
            }
        } else {
            // Fallback to direct execution
            function_();
            set_state(JobState::Completed);
        }
    } catch (...) {
        set_state(JobState::Failed);
    }
    
    stats_.end_time = std::chrono::high_resolution_clock::now();
    update_stats();
    
    // Complete the promise
    completion_promise_.set_value();
    
    // Call completion callback if set
    if (completion_callback_) {
        completion_callback_(stats_);
    }
}

void FiberJob::suspend() {
    if (state() == JobState::Running && fiber_) {
        set_state(JobState::Suspended);
        fiber_->yield();
    }
}

void FiberJob::resume() {
    if (state() == JobState::Suspended && fiber_) {
        set_state(JobState::Running);
        fiber_->resume();
    }
}

void FiberJob::cancel() {
    JobState expected = JobState::Created;
    if (state_.compare_exchange_strong(expected, JobState::Cancelled)) {
        completion_promise_.set_value();
    }
    
    expected = JobState::Ready;
    if (state_.compare_exchange_strong(expected, JobState::Cancelled)) {
        completion_promise_.set_value();
    }
}

bool FiberJob::is_ready() const noexcept {
    return state() == JobState::Ready && pending_dependencies_.load(std::memory_order_acquire) == 0;
}

bool FiberJob::is_complete() const noexcept {
    JobState current_state = state();
    return current_state == JobState::Completed || 
           current_state == JobState::Failed || 
           current_state == JobState::Cancelled;
}

void FiberJob::add_dependency(JobID dependency) {
    dependencies_.push_back(dependency);
    pending_dependencies_.fetch_add(1, std::memory_order_relaxed);
}

void FiberJob::notify_dependency_completed(JobID dependency) {
    auto it = std::find(dependencies_.begin(), dependencies_.end(), dependency);
    if (it != dependencies_.end()) {
        u32 prev_count = pending_dependencies_.fetch_sub(1, std::memory_order_acq_rel);
        if (prev_count == 1) {
            // Last dependency completed - job is now ready
            set_state(JobState::Ready);
        }
    }
}

bool FiberJob::has_pending_dependencies() const noexcept {
    return pending_dependencies_.load(std::memory_order_acquire) > 0;
}

void FiberJob::set_state(JobState new_state) noexcept {
    state_.store(new_state, std::memory_order_release);
}

void FiberJob::update_stats() {
    // Update execution statistics
    if (stats_.start_time != std::chrono::high_resolution_clock::time_point{} &&
        stats_.end_time != std::chrono::high_resolution_clock::time_point{}) {
        // Statistics are calculated on-demand via accessor methods
    }
}

void FiberJob::initialize_fiber() {
    if (fiber_) return;
    
    // Create fiber with the job function
    try {
        fiber_ = std::make_unique<Fiber>(
            FiberID{id_.index, id_.generation}, 
            name_, 
            function_, 
            stack_config_, 
            static_cast<FiberPriority>(priority_)
        );
    } catch (...) {
        // Fiber creation failed - will fall back to direct execution
    }
}

void FiberJob::cleanup_fiber() {
    fiber_.reset();
}

//=============================================================================
// Fiber Work-Stealing Queue Implementation
//=============================================================================

FiberWorkStealingQueue::Buffer::Buffer(usize cap) : capacity(cap), mask(cap - 1) {
    // Ensure capacity is power of 2
    assert((capacity & (capacity - 1)) == 0);
    
    jobs = new std::atomic<FiberJob*>[capacity];
    for (usize i = 0; i < capacity; ++i) {
        jobs[i].store(nullptr, std::memory_order_relaxed);
    }
}

FiberWorkStealingQueue::Buffer::~Buffer() {
    delete[] jobs;
}

FiberJob* FiberWorkStealingQueue::Buffer::get(usize index) const noexcept {
    return jobs[index & mask].load(std::memory_order_acquire);
}

void FiberWorkStealingQueue::Buffer::put(usize index, FiberJob* job) noexcept {
    jobs[index & mask].store(job, std::memory_order_release);
}

FiberWorkStealingQueue::FiberWorkStealingQueue(u32 owner_id, const std::string& name)
    : buffer_(new Buffer(DEFAULT_CAPACITY))
    , top_(0)
    , bottom_(0)
    , priority_mutex_("WorkQueue_Priority")
    , pushes_(0)
    , pops_(0)
    , steals_(0)
    , steal_attempts_(0)
    , failed_steals_(0)
    , aba_failures_(0)
    , owner_worker_id_(owner_id)
    , debug_name_(name.empty() ? ("WorkQueue_" + std::to_string(owner_id)) : name) {
}

FiberWorkStealingQueue::~FiberWorkStealingQueue() {
    delete buffer_.load(std::memory_order_relaxed);
}

bool FiberWorkStealingQueue::push(FiberJob* job) noexcept {
    if (!job) return false;
    
    u64 b = bottom_.load(std::memory_order_relaxed);
    u64 t = top_.load(std::memory_order_acquire);
    
    Buffer* buffer = buffer_.load(std::memory_order_relaxed);
    
    if (b - t >= buffer->capacity) {
        // Queue is full - try to grow
        grow_buffer();
        buffer = buffer_.load(std::memory_order_relaxed);
        
        if (b - t >= buffer->capacity) {
            return false; // Still full after growth attempt
        }
    }
    
    buffer->put(b, job);
    bottom_.store(b + 1, std::memory_order_release);
    
    pushes_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

FiberJob* FiberWorkStealingQueue::pop() noexcept {
    u64 b = bottom_.load(std::memory_order_relaxed);
    
    if (b == 0) {
        return nullptr; // Queue is empty
    }
    
    b--;
    bottom_.store(b, std::memory_order_relaxed);
    
    u64 t = top_.load(std::memory_order_acquire);
    
    if (t <= b) {
        // Non-empty queue
        Buffer* buffer = buffer_.load(std::memory_order_relaxed);
        FiberJob* job = buffer->get(b);
        
        if (t == b) {
            // Last element - compete with stealers
            if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst)) {
                // Lost race with stealer
                job = nullptr;
            }
            bottom_.store(b + 1, std::memory_order_relaxed);
        }
        
        if (job) {
            pops_.fetch_add(1, std::memory_order_relaxed);
        }
        
        return job;
    } else {
        // Empty queue
        bottom_.store(b + 1, std::memory_order_relaxed);
        return nullptr;
    }
}

FiberJob* FiberWorkStealingQueue::steal() noexcept {
    steal_attempts_.fetch_add(1, std::memory_order_relaxed);
    
    u64 t = top_.load(std::memory_order_acquire);
    u64 b = bottom_.load(std::memory_order_acquire);
    
    if (t >= b) {
        failed_steals_.fetch_add(1, std::memory_order_relaxed);
        return nullptr; // Empty queue
    }
    
    Buffer* buffer = buffer_.load(std::memory_order_relaxed);
    FiberJob* job = buffer->get(t);
    
    if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst)) {
        // Lost race
        failed_steals_.fetch_add(1, std::memory_order_relaxed);
        return nullptr;
    }
    
    if (job && job->can_be_stolen()) {
        steals_.fetch_add(1, std::memory_order_relaxed);
        return job;
    } else {
        failed_steals_.fetch_add(1, std::memory_order_relaxed);
        return nullptr;
    }
}

bool FiberWorkStealingQueue::empty() const noexcept {
    u64 b = bottom_.load(std::memory_order_relaxed);
    u64 t = top_.load(std::memory_order_relaxed);
    return b <= t;
}

usize FiberWorkStealingQueue::size() const noexcept {
    u64 b = bottom_.load(std::memory_order_relaxed);
    u64 t = top_.load(std::memory_order_relaxed);
    return (b > t) ? (b - t) : 0;
}

void FiberWorkStealingQueue::grow_buffer() {
    Buffer* old_buffer = buffer_.load(std::memory_order_relaxed);
    usize new_capacity = std::min(old_buffer->capacity * 2, MAX_CAPACITY);
    
    if (new_capacity == old_buffer->capacity) {
        return; // Already at max capacity
    }
    
    auto new_buffer = std::make_unique<Buffer>(new_capacity);
    
    // Copy existing jobs
    u64 t = top_.load(std::memory_order_relaxed);
    u64 b = bottom_.load(std::memory_order_relaxed);
    
    for (u64 i = t; i < b; ++i) {
        new_buffer->put(i, old_buffer->get(i));
    }
    
    // Atomically replace buffer
    buffer_.store(new_buffer.release(), std::memory_order_release);
    
    // Schedule old buffer for deletion (in real implementation, would use hazard pointers)
    delete old_buffer;
}

//=============================================================================
// Adaptive Scheduler Implementation
//=============================================================================

AdaptiveScheduler::AdaptiveScheduler(StealStrategy initial_strategy)
    : current_strategy_(initial_strategy)
    , strategy_switch_counter_(0)
    , last_strategy_change_(std::chrono::steady_clock::now())
    , stats_mutex_("AdaptiveScheduler_Stats")
    , strategy_evaluation_interval_(5000)
    , adaptation_threshold_(0.1)
    , enable_adaptation_(true) {
    
    // Initialize strategy stats
    for (auto& stats : strategy_stats_) {
        stats = StrategyStats{};
    }
}

u32 AdaptiveScheduler::select_steal_target(u32 current_worker, u32 worker_count, 
                                          const std::vector<usize>& worker_loads) const {
    if (worker_count <= 1) {
        return current_worker;
    }
    
    switch (current_strategy_) {
        case StealStrategy::Random:
            return select_random_target(current_worker, worker_count);
            
        case StealStrategy::RoundRobin:
            return select_round_robin_target(current_worker, worker_count);
            
        case StealStrategy::LoadBased:
            return select_load_based_target(current_worker, worker_loads);
            
        case StealStrategy::LocalityAware:
            return select_locality_aware_target(current_worker, worker_count);
            
        case StealStrategy::PriorityAware:
            return select_priority_aware_target(current_worker, worker_count);
            
        case StealStrategy::Adaptive:
            // Use the best performing strategy
            return select_steal_target(current_worker, worker_count, worker_loads);
            
        default:
            return select_random_target(current_worker, worker_count);
    }
}

u32 AdaptiveScheduler::select_random_target(u32 current_worker, u32 worker_count) const {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<u32> dist(0, worker_count - 1);
    
    u32 target;
    do {
        target = dist(rng);
    } while (target == current_worker && worker_count > 1);
    
    return target;
}

u32 AdaptiveScheduler::select_load_based_target(u32 current_worker, 
                                               const std::vector<usize>& worker_loads) const {
    if (worker_loads.size() <= 1) {
        return current_worker;
    }
    
    // Find worker with highest load
    u32 best_target = 0;
    usize max_load = 0;
    
    for (usize i = 0; i < worker_loads.size(); ++i) {
        if (i != current_worker && worker_loads[i] > max_load) {
            max_load = worker_loads[i];
            best_target = static_cast<u32>(i);
        }
    }
    
    return best_target;
}

void AdaptiveScheduler::record_steal_attempt(u32 target_worker, bool success, 
                                           std::chrono::microseconds latency) {
    usize strategy_index = static_cast<usize>(current_strategy_);
    if (strategy_index < strategy_stats_.size()) {
        FiberLockGuard lock(stats_mutex_);
        
        StrategyStats& stats = strategy_stats_[strategy_index];
        stats.steals_attempted++;
        
        if (success) {
            stats.steals_succeeded++;
        }
        
        // Update average latency with exponential moving average
        f64 alpha = 0.1;
        stats.average_latency_us = alpha * static_cast<f64>(latency.count()) + 
                                  (1.0 - alpha) * stats.average_latency_us;
    }
}

//=============================================================================
// Fiber Worker Implementation
//=============================================================================

FiberWorker::FiberWorker(u32 worker_id, u32 cpu_core, u32 numa_node, FiberJobSystem* job_system)
    : worker_id_(worker_id)
    , cpu_core_(cpu_core)
    , numa_node_(numa_node)
    , is_running_(false)
    , should_stop_(false)
    , local_queue_(std::make_unique<FiberWorkStealingQueue>(worker_id))
    , fiber_pool_(std::make_unique<FiberPool>())
    , job_system_(job_system)
    , current_job_(nullptr)
    , main_fiber_(nullptr)
    , current_fiber_(nullptr)
    , scheduler_(std::make_unique<AdaptiveScheduler>())
    , last_steal_attempt_(std::chrono::steady_clock::now())
    , consecutive_failed_steals_(0)
    , jobs_executed_(0)
    , jobs_stolen_(0)
    , jobs_donated_(0)
    , fiber_switches_(0)
    , idle_cycles_(0)
    , steal_attempts_(0)
    , successful_steals_(0)
    , worker_start_time_(std::chrono::steady_clock::now())
    , last_activity_time_(std::chrono::steady_clock::now())
    , total_execution_time_us_(0)
    , total_idle_time_us_(0)
    , idle_sleep_duration_(100)
    , max_steal_attempts_before_yield_(1000)
    , enable_work_stealing_(true)
    , enable_fiber_switching_(true) {
}

FiberWorker::~FiberWorker() {
    stop();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void FiberWorker::start() {
    if (is_running_.load(std::memory_order_acquire)) {
        return; // Already running
    }
    
    should_stop_.store(false, std::memory_order_release);
    worker_thread_ = std::thread([this]() { worker_main_loop(); });
}

void FiberWorker::stop() {
    should_stop_.store(true, std::memory_order_release);
}

void FiberWorker::join() {
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

bool FiberWorker::submit_job(FiberJob* job) {
    if (!job) return false;
    
    return local_queue_->push(job);
}

FiberJob* FiberWorker::try_get_work() {
    // Try local queue first
    if (FiberJob* job = local_queue_->pop()) {
        return job;
    }
    
    // Try work stealing if enabled
    if (enable_work_stealing_) {
        return steal_work();
    }
    
    return nullptr;
}

void FiberWorker::worker_main_loop() {
    setup_worker_thread();
    is_running_.store(true, std::memory_order_release);
    
    while (!should_stop_.load(std::memory_order_acquire)) {
        FiberJob* job = find_work_internal();
        
        if (job) {
            execute_job(job);
            last_activity_time_ = std::chrono::steady_clock::now();
            consecutive_failed_steals_ = 0;
        } else {
            handle_idle_period();
        }
        
        update_performance_counters();
    }
    
    cleanup_worker_thread();
    is_running_.store(false, std::memory_order_release);
}

void FiberWorker::execute_job(FiberJob* job) {
    if (!job) return;
    
    current_job_.store(job, std::memory_order_release);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        job->execute_in_fiber();
        jobs_executed_.fetch_add(1, std::memory_order_relaxed);
    } catch (...) {
        // Job execution failed - mark as failed
        job->set_state(JobState::Failed);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    total_execution_time_us_.fetch_add(duration.count(), std::memory_order_relaxed);
    
    current_job_.store(nullptr, std::memory_order_release);
}

FiberJob* FiberWorker::steal_work() {
    if (!job_system_ || !enable_work_stealing_) {
        return nullptr;
    }
    
    steal_attempts_.fetch_add(1, std::memory_order_relaxed);
    
    // Simple round-robin stealing for now
    u32 worker_count = job_system_->worker_count();
    if (worker_count <= 1) {
        return nullptr;
    }
    
    u32 target = (worker_id_ + 1) % worker_count;
    for (u32 attempts = 0; attempts < worker_count - 1; ++attempts) {
        if (target == worker_id_) {
            target = (target + 1) % worker_count;
            continue;
        }
        
        FiberWorker* target_worker = job_system_->get_worker(target);
        if (target_worker && target_worker->local_queue_) {
            if (FiberJob* stolen_job = target_worker->local_queue_->steal()) {
                jobs_stolen_.fetch_add(1, std::memory_order_relaxed);
                successful_steals_.fetch_add(1, std::memory_order_relaxed);
                return stolen_job;
            }
        }
        
        target = (target + 1) % worker_count;
    }
    
    consecutive_failed_steals_++;
    return nullptr;
}

FiberJob* FiberWorker::find_work_internal() {
    // Try local work first
    if (FiberJob* job = local_queue_->pop()) {
        return job;
    }
    
    // Try stealing from other workers
    return steal_work();
}

void FiberWorker::handle_idle_period() {
    idle_cycles_.fetch_add(1, std::memory_order_relaxed);
    
    if (consecutive_failed_steals_ >= max_steal_attempts_before_yield_) {
        // Sleep briefly to avoid busy waiting
        FiberUtils::sleep_for(idle_sleep_duration_);
        consecutive_failed_steals_ = 0;
    } else {
        // Just yield
        FiberUtils::yield();
    }
    
    auto idle_start = std::chrono::high_resolution_clock::now();
    auto idle_end = idle_start + idle_sleep_duration_;
    auto idle_duration = std::chrono::duration_cast<std::chrono::microseconds>(idle_end - idle_start);
    total_idle_time_us_.fetch_add(idle_duration.count(), std::memory_order_relaxed);
}

void FiberWorker::setup_worker_thread() {
    set_thread_affinity();
    set_thread_numa_policy();
    
    // Create main fiber for this worker
    // In a full implementation, this would be a proper fiber
    worker_start_time_ = std::chrono::steady_clock::now();
}

void FiberWorker::cleanup_worker_thread() {
    // Cleanup resources
}

void FiberWorker::set_thread_affinity() {
    // Platform-specific CPU affinity setting would go here
}

void FiberWorker::set_thread_numa_policy() {
    // Platform-specific NUMA policy setting would go here
}

void FiberWorker::update_performance_counters() {
    // Update performance statistics periodically
}

FiberWorker::WorkerStats FiberWorker::get_statistics() const {
    WorkerStats stats;
    stats.worker_id = worker_id_;
    stats.cpu_core = cpu_core_;
    stats.numa_node = numa_node_;
    
    stats.jobs_executed = jobs_executed_.load(std::memory_order_relaxed);
    stats.jobs_stolen = jobs_stolen_.load(std::memory_order_relaxed);
    stats.jobs_donated = jobs_donated_.load(std::memory_order_relaxed);
    stats.fiber_switches = fiber_switches_.load(std::memory_order_relaxed);
    stats.idle_cycles = idle_cycles_.load(std::memory_order_relaxed);
    stats.steal_attempts = steal_attempts_.load(std::memory_order_relaxed);
    stats.successful_steals = successful_steals_.load(std::memory_order_relaxed);
    
    u64 total_execution = total_execution_time_us_.load(std::memory_order_relaxed);
    u64 total_idle = total_idle_time_us_.load(std::memory_order_relaxed);
    u64 total_time = total_execution + total_idle;
    
    if (total_time > 0) {
        stats.utilization_percent = static_cast<f64>(total_execution) / static_cast<f64>(total_time) * 100.0;
    }
    
    if (stats.steal_attempts > 0) {
        stats.steal_success_rate = static_cast<f64>(stats.successful_steals) / 
                                  static_cast<f64>(stats.steal_attempts) * 100.0;
    }
    
    if (stats.jobs_executed > 0 && total_execution > 0) {
        stats.average_job_time_us = static_cast<f64>(total_execution) / static_cast<f64>(stats.jobs_executed);
    }
    
    stats.current_queue_size = local_queue_->size();
    stats.is_running = is_running();
    stats.is_idle = is_idle();
    stats.last_activity = last_activity_time_;
    stats.total_execution_time = std::chrono::microseconds{total_execution};
    stats.total_idle_time = std::chrono::microseconds{total_idle};
    
    return stats;
}

//=============================================================================
// Main Fiber Job System Implementation
//=============================================================================

FiberJobSystem::FiberJobSystem(const SystemConfig& config)
    : config_(config)
    , is_initialized_(false)
    , is_shutting_down_(false)
    , main_thread_worker_(nullptr)
    , total_worker_count_(0)
    , next_job_index_(1)
    , job_generation_(1)
    , job_pool_mutex_("JobSystem_JobPool")
    , dependency_graph_(std::make_unique<JobDependencyGraph>())
    , dependency_mutex_("JobSystem_Dependencies")
    , global_queue_(std::make_unique<FiberWorkStealingQueue>(0, "GlobalQueue"))
    , work_available_("JobSystem_WorkAvailable")
    , work_mutex_("JobSystem_Work")
    , profiler_(nullptr)
    , total_jobs_submitted_(0)
    , total_jobs_completed_(0)
    , total_jobs_failed_(0)
    , total_fiber_switches_(0)
    , system_start_time_(std::chrono::steady_clock::now())
    , system_uptime_us_(0) {
    
    if (config_.enable_performance_monitoring) {
        ProfilerConfig profiler_config = ProfilerConfig::create_development();
        profiler_ = std::make_unique<JobProfiler>(profiler_config);
    }
}

FiberJobSystem::~FiberJobSystem() {
    shutdown();
}

bool FiberJobSystem::initialize() {
    if (is_initialized_.load(std::memory_order_acquire)) {
        return true; // Already initialized
    }
    
    if (!initialize_workers() || !initialize_job_pools() || !initialize_monitoring()) {
        cleanup_system();
        return false;
    }
    
    is_initialized_.store(true, std::memory_order_release);
    system_start_time_ = std::chrono::steady_clock::now();
    
    if (profiler_) {
        profiler_->initialize(total_worker_count_);
        profiler_->start_profiling_session("JobSystem_Main");
    }
    
    return true;
}

void FiberJobSystem::shutdown() {
    if (is_shutting_down_.exchange(true, std::memory_order_acq_rel)) {
        return; // Already shutting down
    }
    
    // Stop all workers
    for (auto& worker : workers_) {
        if (worker) {
            worker->stop();
        }
    }
    
    // Wait for workers to finish
    for (auto& worker : workers_) {
        if (worker) {
            worker->join();
        }
    }
    
    if (profiler_) {
        profiler_->end_profiling_session();
        profiler_->shutdown();
    }
    
    cleanup_system();
    is_initialized_.store(false, std::memory_order_release);
}

u32 FiberJobSystem::active_job_count() const noexcept {
    u32 count = 0;
    for (const auto& worker : workers_) {
        if (worker && !worker->is_idle()) {
            count++;
        }
    }
    return count;
}

bool FiberJobSystem::all_workers_idle() const noexcept {
    for (const auto& worker : workers_) {
        if (worker && !worker->is_idle()) {
            return false;
        }
    }
    return true;
}

FiberWorker* FiberJobSystem::get_worker(u32 worker_id) const {
    if (worker_id >= workers_.size()) {
        return nullptr;
    }
    return workers_[worker_id].get();
}

FiberJobSystem::SystemStats FiberJobSystem::get_system_statistics() const {
    SystemStats stats;
    
    stats.total_jobs_submitted = total_jobs_submitted_.load(std::memory_order_relaxed);
    stats.total_jobs_completed = total_jobs_completed_.load(std::memory_order_relaxed);
    stats.total_jobs_failed = total_jobs_failed_.load(std::memory_order_relaxed);
    
    auto current_time = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::microseconds>(
        current_time - system_start_time_);
    stats.system_uptime = uptime;
    
    // Calculate jobs per second
    if (uptime.count() > 0) {
        stats.jobs_per_second = static_cast<f64>(stats.total_jobs_completed) / 
                               (static_cast<f64>(uptime.count()) / 1000000.0);
    }
    
    // Collect worker statistics
    stats.per_worker_utilization.reserve(workers_.size());
    f64 total_utilization = 0.0;
    
    for (const auto& worker : workers_) {
        if (worker) {
            auto worker_stats = worker->get_statistics();
            stats.per_worker_utilization.push_back(worker_stats.utilization_percent);
            total_utilization += worker_stats.utilization_percent;
        }
    }
    
    if (!workers_.empty()) {
        stats.overall_worker_utilization = total_utilization / workers_.size();
    }
    
    stats.measurement_start = system_start_time_;
    stats.measurement_end = current_time;
    
    return stats;
}

//=============================================================================
// Private Implementation Methods
//=============================================================================

bool FiberJobSystem::initialize_workers() {
    u32 worker_count = config_.worker_count;
    if (worker_count == 0) {
        worker_count = std::max(1u, std::thread::hardware_concurrency() - 1);
    }
    
    workers_.reserve(worker_count);
    
    for (u32 i = 0; i < worker_count; ++i) {
        u32 cpu_core = config_.enable_cpu_affinity ? i : 0;
        u32 numa_node = config_.enable_numa_awareness ? (i % 2) : 0; // Simplified
        
        auto worker = std::make_unique<FiberWorker>(i, cpu_core, numa_node, this);
        
        workers_.push_back(std::move(worker));
    }
    
    // Start all workers
    for (auto& worker : workers_) {
        worker->start();
    }
    
    total_worker_count_ = static_cast<u32>(workers_.size());
    return true;
}

bool FiberJobSystem::initialize_job_pools() {
    // Initialize job pool
    job_pool_.reserve(config_.job_pool_initial_size);
    
    for (usize i = 0; i < config_.job_pool_initial_size; ++i) {
        free_job_indices_.push(i);
    }
    
    return true;
}

bool FiberJobSystem::initialize_monitoring() {
    // Monitoring is initialized in constructor
    return true;
}

void FiberJobSystem::cleanup_system() {
    workers_.clear();
    job_pool_.clear();
    while (!free_job_indices_.empty()) {
        free_job_indices_.pop();
    }
}

JobID FiberJobSystem::allocate_job_id() {
    u32 index = next_job_index_.fetch_add(1, std::memory_order_relaxed);
    u16 generation = job_generation_.load(std::memory_order_relaxed);
    
    // Handle overflow
    if (index == 0) {
        generation = job_generation_.fetch_add(1, std::memory_order_relaxed);
    }
    
    return JobID{index, generation};
}

FiberWorker* FiberJobSystem::select_worker_for_job(const FiberJob& job) {
    if (workers_.empty()) {
        return nullptr;
    }
    
    // Simple load balancing - select worker with smallest queue
    FiberWorker* best_worker = nullptr;
    usize min_queue_size = std::numeric_limits<usize>::max();
    
    for (auto& worker : workers_) {
        if (worker) {
            usize queue_size = worker->queue_size();
            if (queue_size < min_queue_size) {
                min_queue_size = queue_size;
                best_worker = worker.get();
            }
        }
    }
    
    return best_worker;
}

} // namespace ecscope::jobs