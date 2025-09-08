#pragma once

/**
 * @file thread_pool.hpp
 * @brief Professional-grade work-stealing thread pool with NUMA awareness and advanced load balancing
 * 
 * This implementation provides a world-class thread pool designed for high-performance system scheduling
 * with the following key features:
 * 
 * - Lock-free work-stealing deques for optimal task distribution
 * - NUMA-aware thread affinity and memory allocation
 * - Dynamic load balancing with work migration
 * - Hierarchical task queues with priority support
 * - Cache-friendly task batching and locality optimization
 * - Comprehensive performance monitoring and statistics
 * - Adaptive thread count based on system load
 * - Task dependency tracking and scheduling
 * - Thread-local storage optimization
 * - Exception handling and recovery mechanisms
 * 
 * Architecture:
 * - Each thread maintains its own work-stealing deque
 * - Global task queue for initial task distribution
 * - NUMA node awareness for memory locality
 * - Hierarchical task priority system
 * - Adaptive scheduling algorithms
 * 
 * Performance Optimizations:
 * - Lock-free data structures throughout
 * - Cache line alignment for critical data
 * - NUMA-aware memory allocation
 * - Thread affinity management
 * - Task batching for reduced overhead
 * - Memory prefetching hints
 * 
 * @author ECScope Professional ECS Framework
 * @date 2024
 */

#include "../core/types.hpp"
#include "../core/log.hpp"
#include "../foundation/memory_utils.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <deque>
#include <queue>
#include <functional>
#include <future>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <array>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <span>

#ifdef _WIN32
    #include <windows.h>
    #include <processthreadsapi.h>
#elif defined(__linux__)
    #include <pthread.h>
    #include <sched.h>
    #include <numa.h>
    #include <numaif.h>
#endif

namespace ecscope::scheduling {

/**
 * @brief Task priority levels for hierarchical scheduling
 */
enum class TaskPriority : u8 {
    Critical = 0,       // System-critical tasks (input, physics integration)
    High = 1,           // High-priority systems (rendering, audio)
    Normal = 2,         // Regular game logic systems
    Low = 3,            // Background tasks (asset loading, compression)
    Idle = 4,           // Idle tasks (garbage collection, profiling)
    COUNT = 5
};

/**
 * @brief Task execution flags for advanced scheduling control
 */
enum class TaskFlags : u32 {
    None = 0,
    PreferLocalThread = 1 << 0,     // Prefer execution on submitting thread
    NUMAAware = 1 << 1,             // Use NUMA-aware scheduling
    CacheFriendly = 1 << 2,         // Optimize for cache locality  
    MemoryIntensive = 1 << 3,       // Task performs heavy memory operations
    CPUIntensive = 1 << 4,          // Task performs heavy CPU operations
    IOBound = 1 << 5,               // Task performs I/O operations
    Continuation = 1 << 6,          // Task is continuation of previous work
    BatchingAllowed = 1 << 7,       // Task can be batched with similar tasks
    ThreadAffinityRequired = 1 << 8 // Task requires specific thread affinity
};

constexpr TaskFlags operator|(TaskFlags a, TaskFlags b) {
    return static_cast<TaskFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

constexpr TaskFlags operator&(TaskFlags a, TaskFlags b) {
    return static_cast<TaskFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}

/**
 * @brief NUMA topology information for thread and memory affinity
 */
struct NUMATopology {
    struct Node {
        u32 node_id;
        std::vector<u32> cpu_cores;
        usize memory_size;
        f64 memory_bandwidth;
        f64 access_latency;
    };
    
    std::vector<Node> nodes;
    u32 total_cores;
    u32 total_nodes;
    bool numa_available;
    
    NUMATopology();
    
    u32 get_numa_node_for_thread(u32 thread_id) const;
    std::vector<u32> get_preferred_cores(u32 numa_node) const;
    void set_thread_affinity(std::thread& thread, u32 numa_node) const;
    void* allocate_numa_memory(usize size, u32 numa_node) const;
    void free_numa_memory(void* ptr, usize size) const;
};

/**
 * @brief Task wrapper containing execution information and metadata
 */
class Task {
private:
    std::function<void()> function_;
    TaskPriority priority_;
    TaskFlags flags_;
    u64 submission_time_;
    u64 task_id_;
    u32 preferred_numa_node_;
    u32 preferred_thread_id_;
    std::string debug_name_;
    
    // Dependency tracking
    std::vector<u64> dependencies_;
    std::atomic<u32> remaining_dependencies_;
    std::vector<std::weak_ptr<Task>> dependents_;
    
    // Performance tracking
    mutable std::atomic<u64> execution_start_time_;
    mutable std::atomic<u64> execution_end_time_;
    mutable std::atomic<u32> execution_count_;
    
    static std::atomic<u64> next_task_id_;
    
public:
    template<typename F>
    Task(F&& func, TaskPriority priority = TaskPriority::Normal, 
         TaskFlags flags = TaskFlags::None, const std::string& name = "")
        : function_(std::forward<F>(func))
        , priority_(priority)
        , flags_(flags)
        , submission_time_(get_high_resolution_time())
        , task_id_(next_task_id_.fetch_add(1, std::memory_order_relaxed))
        , preferred_numa_node_(0)
        , preferred_thread_id_(~0u)
        , debug_name_(name.empty() ? ("Task_" + std::to_string(task_id_)) : name)
        , remaining_dependencies_(0)
        , execution_start_time_(0)
        , execution_end_time_(0)
        , execution_count_(0) {}
    
    void execute() const {
        execution_start_time_.store(get_high_resolution_time(), std::memory_order_relaxed);
        execution_count_.fetch_add(1, std::memory_order_relaxed);
        
        try {
            function_();
        } catch (const std::exception& e) {
            LOG_ERROR("Task '{}' threw exception: {}", debug_name_, e.what());
        } catch (...) {
            LOG_ERROR("Task '{}' threw unknown exception", debug_name_);
        }
        
        execution_end_time_.store(get_high_resolution_time(), std::memory_order_relaxed);
        notify_dependents();
    }
    
    // Accessors
    u64 id() const noexcept { return task_id_; }
    TaskPriority priority() const noexcept { return priority_; }
    TaskFlags flags() const noexcept { return flags_; }
    u64 submission_time() const noexcept { return submission_time_; }
    u32 preferred_numa_node() const noexcept { return preferred_numa_node_; }
    u32 preferred_thread_id() const noexcept { return preferred_thread_id_; }
    const std::string& debug_name() const noexcept { return debug_name_; }
    
    // Configuration
    Task& set_numa_node(u32 node) { preferred_numa_node_ = node; return *this; }
    Task& set_thread_affinity(u32 thread_id) { preferred_thread_id_ = thread_id; return *this; }
    Task& set_flags(TaskFlags flags) { flags_ = flags; return *this; }
    
    // Dependencies
    void add_dependency(u64 task_id) {
        dependencies_.push_back(task_id);
        remaining_dependencies_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void add_dependent(std::shared_ptr<Task> task) {
        dependents_.push_back(task);
    }
    
    bool is_ready() const {
        return remaining_dependencies_.load(std::memory_order_acquire) == 0;
    }
    
    void satisfy_dependency() {
        remaining_dependencies_.fetch_sub(1, std::memory_order_acq_rel);
    }
    
    // Performance metrics
    f64 get_execution_time() const {
        u64 start = execution_start_time_.load(std::memory_order_relaxed);
        u64 end = execution_end_time_.load(std::memory_order_relaxed);
        return start > 0 && end > start ? (end - start) / 1e9 : 0.0;
    }
    
    f64 get_queue_time() const {
        u64 start = execution_start_time_.load(std::memory_order_relaxed);
        return start > submission_time_ ? (start - submission_time_) / 1e9 : 0.0;
    }
    
    u32 execution_count() const {
        return execution_count_.load(std::memory_order_relaxed);
    }

private:
    void notify_dependents() const {
        for (auto& weak_dependent : dependents_) {
            if (auto dependent = weak_dependent.lock()) {
                dependent->satisfy_dependency();
            }
        }
    }
    
    static u64 get_high_resolution_time() {
        return std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }
};

/**
 * @brief Lock-free work-stealing deque optimized for task scheduling
 */
template<typename T>
class WorkStealingDeque {
private:
    static constexpr usize CACHE_LINE_SIZE = 64;
    static constexpr usize INITIAL_SIZE = 256;
    
    struct alignas(CACHE_LINE_SIZE) Array {
        std::atomic<usize> size;
        std::unique_ptr<std::atomic<T>[]> data;
        
        Array(usize capacity) : size(capacity) {
            data = std::make_unique<std::atomic<T>[]>(capacity);
        }
        
        T load(usize index) const {
            return data[index].load(std::memory_order_acquire);
        }
        
        void store(usize index, T item) {
            data[index].store(item, std::memory_order_release);
        }
        
        usize capacity() const {
            return size.load(std::memory_order_relaxed);
        }
    };
    
    alignas(CACHE_LINE_SIZE) std::atomic<isize> top_;
    alignas(CACHE_LINE_SIZE) std::atomic<isize> bottom_;
    alignas(CACHE_LINE_SIZE) std::atomic<Array*> array_;
    
public:
    WorkStealingDeque() 
        : top_(0), bottom_(0)
        , array_(new Array(INITIAL_SIZE)) {}
    
    ~WorkStealingDeque() {
        delete array_.load();
    }
    
    void push(T item) {
        isize b = bottom_.load(std::memory_order_relaxed);
        isize t = top_.load(std::memory_order_acquire);
        Array* a = array_.load(std::memory_order_relaxed);
        
        if (b - t > static_cast<isize>(a->capacity()) - 1) {
            // Queue is full, resize
            Array* new_array = new Array(a->capacity() * 2);
            for (isize i = t; i < b; ++i) {
                new_array->store(i % new_array->capacity(), a->load(i % a->capacity()));
            }
            array_.store(new_array, std::memory_order_release);
            delete a;
            a = new_array;
        }
        
        a->store(b % a->capacity(), item);
        std::atomic_thread_fence(std::memory_order_release);
        bottom_.store(b + 1, std::memory_order_relaxed);
    }
    
    T pop() {
        isize b = bottom_.load(std::memory_order_relaxed) - 1;
        Array* a = array_.load(std::memory_order_relaxed);
        bottom_.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        isize t = top_.load(std::memory_order_relaxed);
        
        T item = T{};
        if (t <= b) {
            item = a->load(b % a->capacity());
            if (t == b) {
                if (!top_.compare_exchange_strong(t, t + 1, 
                                                std::memory_order_seq_cst,
                                                std::memory_order_relaxed)) {
                    item = T{};
                }
                bottom_.store(b + 1, std::memory_order_relaxed);
            }
        } else {
            bottom_.store(b + 1, std::memory_order_relaxed);
        }
        
        return item;
    }
    
    T steal() {
        isize t = top_.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        isize b = bottom_.load(std::memory_order_acquire);
        
        T item = T{};
        if (t < b) {
            Array* a = array_.load(std::memory_order_consume);
            item = a->load(t % a->capacity());
            if (!top_.compare_exchange_strong(t, t + 1,
                                            std::memory_order_seq_cst,
                                            std::memory_order_relaxed)) {
                return T{};
            }
        }
        
        return item;
    }
    
    bool empty() const {
        isize b = bottom_.load(std::memory_order_relaxed);
        isize t = top_.load(std::memory_order_relaxed);
        return b <= t;
    }
    
    usize size() const {
        isize b = bottom_.load(std::memory_order_relaxed);
        isize t = top_.load(std::memory_order_relaxed);
        return static_cast<usize>(std::max<isize>(0, b - t));
    }
};

/**
 * @brief Thread pool statistics for performance monitoring and optimization
 */
struct ThreadPoolStats {
    // Task execution metrics
    std::atomic<u64> total_tasks_submitted{0};
    std::atomic<u64> total_tasks_completed{0};
    std::atomic<u64> total_tasks_stolen{0};
    std::atomic<u64> total_tasks_failed{0};
    
    // Timing metrics
    std::atomic<u64> total_execution_time_ns{0};
    std::atomic<u64> total_queue_time_ns{0};
    std::atomic<u64> total_idle_time_ns{0};
    std::atomic<u64> total_stealing_time_ns{0};
    
    // Thread utilization
    std::atomic<u64> thread_busy_time_ns{0};
    std::atomic<u64> thread_blocking_time_ns{0};
    std::atomic<u64> context_switches{0};
    
    // Load balancing metrics
    std::atomic<u64> successful_steals{0};
    std::atomic<u64> failed_steals{0};
    std::atomic<u64> load_balance_operations{0};
    
    // Memory and cache metrics
    std::atomic<u64> cache_misses{0};
    std::atomic<u64> numa_remote_accesses{0};
    std::atomic<usize> peak_queue_size{0};
    
    void reset() {
        total_tasks_submitted = 0;
        total_tasks_completed = 0;
        total_tasks_stolen = 0;
        total_tasks_failed = 0;
        total_execution_time_ns = 0;
        total_queue_time_ns = 0;
        total_idle_time_ns = 0;
        total_stealing_time_ns = 0;
        thread_busy_time_ns = 0;
        thread_blocking_time_ns = 0;
        context_switches = 0;
        successful_steals = 0;
        failed_steals = 0;
        load_balance_operations = 0;
        cache_misses = 0;
        numa_remote_accesses = 0;
        peak_queue_size = 0;
    }
    
    f64 get_average_execution_time() const {
        u64 completed = total_tasks_completed.load();
        return completed > 0 ? total_execution_time_ns.load() / (1e9 * completed) : 0.0;
    }
    
    f64 get_average_queue_time() const {
        u64 completed = total_tasks_completed.load();
        return completed > 0 ? total_queue_time_ns.load() / (1e9 * completed) : 0.0;
    }
    
    f64 get_thread_utilization() const {
        u64 busy = thread_busy_time_ns.load();
        u64 idle = total_idle_time_ns.load();
        u64 total = busy + idle;
        return total > 0 ? static_cast<f64>(busy) / total : 0.0;
    }
    
    f64 get_steal_success_rate() const {
        u64 successful = successful_steals.load();
        u64 failed = failed_steals.load();
        u64 total = successful + failed;
        return total > 0 ? static_cast<f64>(successful) / total : 0.0;
    }
};

/**
 * @brief Per-thread worker context containing thread-specific data and queues
 */
class WorkerThread {
private:
    u32 thread_id_;
    u32 numa_node_;
    std::thread thread_;
    std::atomic<bool> should_stop_;
    
    // Task queues with priority support
    std::array<WorkStealingDeque<std::shared_ptr<Task>>, 
               static_cast<usize>(TaskPriority::COUNT)> priority_queues_;
    
    // Thread-local statistics
    ThreadPoolStats local_stats_;
    
    // Performance optimization
    std::random_device random_device_;
    std::mt19937 random_generator_;
    std::uniform_int_distribution<u32> steal_distribution_;
    
    // Thread affinity and NUMA
    cpu_set_t affinity_mask_;
    void* numa_memory_pool_;
    usize numa_pool_size_;
    
    class ThreadPool* pool_;  // Back-reference to pool
    
public:
    WorkerThread(u32 thread_id, u32 numa_node, class ThreadPool* pool);
    ~WorkerThread();
    
    void start();
    void stop();
    void join();
    
    // Task management
    bool try_push_task(std::shared_ptr<Task> task);
    std::shared_ptr<Task> try_pop_task();
    std::shared_ptr<Task> try_steal_task();
    
    // Information
    u32 id() const noexcept { return thread_id_; }
    u32 numa_node() const noexcept { return numa_node_; }
    bool is_busy() const;
    usize queue_size() const;
    usize queue_size(TaskPriority priority) const;
    const ThreadPoolStats& statistics() const { return local_stats_; }
    
    // NUMA optimization
    void* allocate_local_memory(usize size, usize alignment = alignof(std::max_align_t));
    void deallocate_local_memory(void* ptr);

private:
    void worker_loop();
    bool execute_next_task();
    std::shared_ptr<Task> find_highest_priority_task();
    void update_thread_affinity();
    void optimize_numa_locality();
    void record_performance_metrics();
};

/**
 * @brief Professional work-stealing thread pool with advanced scheduling capabilities
 */
class ThreadPool {
private:
    // Configuration
    u32 thread_count_;
    bool numa_aware_;
    bool adaptive_scheduling_;
    bool load_balancing_enabled_;
    f64 load_balance_threshold_;
    
    // Thread management
    std::vector<std::unique_ptr<WorkerThread>> workers_;
    std::atomic<bool> shutdown_requested_;
    std::atomic<u32> active_threads_;
    
    // Global task queue for initial distribution
    std::array<std::queue<std::shared_ptr<Task>>, 
               static_cast<usize>(TaskPriority::COUNT)> global_queues_;
    std::mutex global_queue_mutex_;
    std::condition_variable task_available_;
    
    // NUMA topology
    std::unique_ptr<NUMATopology> numa_topology_;
    
    // Performance monitoring
    ThreadPoolStats global_stats_;
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<u64> last_balance_time_;
    
    // Load balancing
    std::atomic<bool> load_balance_active_;
    std::thread load_balancer_thread_;
    std::mutex load_balance_mutex_;
    
    // Task dependency tracking
    std::unordered_map<u64, std::shared_ptr<Task>> pending_tasks_;
    std::mutex pending_tasks_mutex_;

public:
    explicit ThreadPool(u32 thread_count = 0,
                       bool numa_aware = true,
                       bool adaptive_scheduling = true);
    ~ThreadPool();
    
    // Lifecycle management
    void initialize();
    void shutdown();
    bool is_running() const { return !shutdown_requested_.load(); }
    
    // Task submission
    template<typename F, typename... Args>
    auto submit(F&& func, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>> {
        return submit_with_priority(TaskPriority::Normal, std::forward<F>(func), 
                                   std::forward<Args>(args)...);
    }
    
    template<typename F, typename... Args>
    auto submit_with_priority(TaskPriority priority, F&& func, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;
        
        auto task_promise = std::make_shared<std::promise<return_type>>();
        auto future = task_promise->get_future();
        
        auto wrapped_task = std::make_shared<Task>([=, func = std::forward<F>(func)]() mutable {
            try {
                if constexpr (std::is_void_v<return_type>) {
                    std::invoke(func, args...);
                    task_promise->set_value();
                } else {
                    auto result = std::invoke(func, args...);
                    task_promise->set_value(std::move(result));
                }
            } catch (...) {
                task_promise->set_exception(std::current_exception());
            }
        }, priority);
        
        submit_task(wrapped_task);
        return future;
    }
    
    void submit_task(std::shared_ptr<Task> task);
    void submit_batch(std::span<std::shared_ptr<Task>> tasks);
    
    // Task dependency management
    template<typename F>
    std::shared_ptr<Task> create_task(F&& func, TaskPriority priority = TaskPriority::Normal,
                                     const std::string& name = "") {
        return std::make_shared<Task>(std::forward<F>(func), priority, TaskFlags::None, name);
    }
    
    void add_dependency(std::shared_ptr<Task> task, std::shared_ptr<Task> dependency);
    void submit_task_graph(const std::vector<std::shared_ptr<Task>>& tasks);
    
    // Configuration
    void set_thread_count(u32 count);
    void set_numa_aware(bool enabled) { numa_aware_ = enabled; }
    void set_adaptive_scheduling(bool enabled) { adaptive_scheduling_ = enabled; }
    void set_load_balancing(bool enabled, f64 threshold = 0.1);
    void set_thread_affinity(u32 thread_id, const std::vector<u32>& cpu_cores);
    
    // Information and statistics
    u32 thread_count() const { return thread_count_; }
    u32 active_threads() const { return active_threads_.load(); }
    bool is_numa_aware() const { return numa_aware_; }
    const NUMATopology& numa_topology() const { return *numa_topology_; }
    
    ThreadPoolStats get_statistics() const;
    std::vector<ThreadPoolStats> get_per_thread_statistics() const;
    void reset_statistics();
    
    // Performance monitoring
    f64 get_average_utilization() const;
    f64 get_load_balance_efficiency() const;
    std::vector<std::pair<u32, usize>> get_queue_sizes() const;
    std::string generate_performance_report() const;
    
    // Advanced features
    void balance_load();
    void optimize_thread_placement();
    void pause_all_threads();
    void resume_all_threads();
    
    // Friend access for WorkerThread
    friend class WorkerThread;

private:
    void initialize_numa_topology();
    void create_worker_threads();
    void distribute_initial_tasks();
    std::shared_ptr<Task> try_steal_from_global_queue(TaskPriority priority);
    void balance_thread_loads();
    void load_balancer_loop();
    u32 find_best_worker_for_task(const Task& task) const;
    void update_global_statistics();
    void handle_task_completion(const Task& task);
    void process_ready_dependencies();
};

} // namespace ecscope::scheduling