#pragma once

/**
 * @file memory/cache_aware_structures.hpp
 * @brief Cache-Line Aware Data Structures and Memory Layout Optimization
 * 
 * This header provides advanced cache-line aware data structures that optimize
 * memory layouts for modern CPU cache hierarchies. These structures demonstrate
 * cache optimization principles while providing educational insights into how
 * memory layout affects performance.
 * 
 * Key Features:
 * - Cache-line aligned and padded data structures
 * - False sharing prevention through intelligent padding
 * - Cache-friendly array layouts with prefetching hints
 * - Hot/cold data separation for optimal cache utilization
 * - Memory access pattern optimization
 * - Cache miss prediction and optimization
 * - Educational visualization of cache behavior
 * 
 * Educational Value:
 * - Demonstrates cache line boundaries and alignment effects
 * - Shows false sharing prevention techniques
 * - Illustrates hot/cold data separation strategies
 * - Provides cache miss profiling and analysis
 * - Examples of cache-friendly algorithm design
 * - Real-time cache behavior visualization
 * 
 * @author ECScope Educational ECS Framework - Cache Optimization
 * @date 2025
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <array>
#include <span>
#include <bit>
#include <immintrin.h> // For prefetch intrinsics

namespace ecscope::memory::cache {

//=============================================================================
// Cache Line Detection and Optimization
//=============================================================================

/**
 * @brief Runtime cache line detection and system analysis
 */
class CacheTopologyAnalyzer {
private:
    struct CacheLevel {
        usize size_bytes;           // Total cache size
        usize line_size_bytes;      // Cache line size
        usize associativity;        // N-way associativity
        usize sets_count;           // Number of cache sets
        f64 access_latency_ns;      // Average access latency
        std::string cache_type;     // L1D, L1I, L2, L3, etc.
    };
    
    std::vector<CacheLevel> cache_levels_;
    usize detected_line_size_;
    bool topology_detected_;
    
public:
    CacheTopologyAnalyzer();
    
    /**
     * @brief Detect cache topology through benchmarking
     */
    bool detect_cache_topology();
    
    /**
     * @brief Get optimal alignment for data structures
     */
    usize get_optimal_alignment() const { return detected_line_size_; }
    
    /**
     * @brief Get cache line size
     */
    usize get_cache_line_size() const { return detected_line_size_; }
    
    /**
     * @brief Calculate optimal stride for array access
     */
    usize calculate_optimal_stride(usize element_size) const;
    
    /**
     * @brief Predict cache misses for access pattern
     */
    f64 predict_miss_rate(const std::vector<usize>& access_pattern, usize data_size) const;
    
    /**
     * @brief Get cache topology information
     */
    const std::vector<CacheLevel>& get_cache_levels() const { return cache_levels_; }
    
    /**
     * @brief Generate topology report
     */
    std::string generate_topology_report() const;
    
private:
    usize detect_cache_line_size_benchmark();
    void detect_cache_sizes();
    f64 measure_memory_latency(usize buffer_size, usize stride, usize iterations = 10000);
};

//=============================================================================
// Cache-Aligned Memory Management
//=============================================================================

/**
 * @brief Cache-line aligned memory allocator
 */
class CacheAlignedAllocator {
private:
    usize cache_line_size_;
    std::atomic<usize> total_allocated_{0};
    std::atomic<usize> alignment_waste_{0};
    
    // Track allocations for analysis
    struct AllocationRecord {
        void* ptr;
        usize size;
        usize requested_size;
        usize alignment;
        f64 allocation_time;
    };
    
    std::vector<AllocationRecord> allocations_;
    mutable std::mutex allocations_mutex_;
    
public:
    explicit CacheAlignedAllocator(usize cache_line_size = 64) 
        : cache_line_size_(cache_line_size) {}
    
    ~CacheAlignedAllocator();
    
    /**
     * @brief Allocate cache-line aligned memory
     */
    void* allocate(usize size, usize alignment = 0);
    
    /**
     * @brief Deallocate memory
     */
    void deallocate(void* ptr);
    
    /**
     * @brief Allocate with specific cache line padding
     */
    void* allocate_with_padding(usize size, usize padding_lines = 1);
    
    /**
     * @brief Type-safe allocation
     */
    template<typename T>
    T* allocate(usize count = 1) {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }
    
    template<typename T>
    void deallocate(T* ptr) {
        deallocate(static_cast<void*>(ptr));
    }
    
    /**
     * @brief Get allocation statistics
     */
    struct Statistics {
        usize total_allocated_bytes;
        usize alignment_waste_bytes;
        f64 waste_ratio;
        usize active_allocations;
        f64 average_allocation_time_ns;
    };
    
    Statistics get_statistics() const;
    
private:
    void record_allocation(void* ptr, usize size, usize requested_size, usize alignment);
    void remove_allocation(void* ptr);
    f64 get_current_time() const;
};

//=============================================================================
// Cache-Aware Data Structures
//=============================================================================

/**
 * @brief Cache-line aligned atomic counter with false sharing prevention
 */
template<typename T>
class alignas(core::CACHE_LINE_SIZE) CacheAlignedAtomic {
private:
    alignas(core::CACHE_LINE_SIZE) std::atomic<T> value_;
    
    // Padding to prevent false sharing
    byte padding_[core::CACHE_LINE_SIZE - sizeof(std::atomic<T>)];
    
public:
    CacheAlignedAtomic() : value_(T{}) {}
    explicit CacheAlignedAtomic(T initial) : value_(initial) {}
    
    // Standard atomic operations
    T load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
        return value_.load(order);
    }
    
    void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        value_.store(desired, order);
    }
    
    T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.exchange(desired, order);
    }
    
    bool compare_exchange_weak(T& expected, T desired,
                              std::memory_order success = std::memory_order_seq_cst,
                              std::memory_order failure = std::memory_order_seq_cst) noexcept {
        return value_.compare_exchange_weak(expected, desired, success, failure);
    }
    
    bool compare_exchange_strong(T& expected, T desired,
                                std::memory_order success = std::memory_order_seq_cst,
                                std::memory_order failure = std::memory_order_seq_cst) noexcept {
        return value_.compare_exchange_strong(expected, desired, success, failure);
    }
    
    T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.fetch_add(arg, order);
    }
    
    T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.fetch_sub(arg, order);
    }
    
    T operator++(int) noexcept { return fetch_add(1); }
    T operator--(int) noexcept { return fetch_sub(1); }
    T operator++() noexcept { return fetch_add(1) + 1; }
    T operator--() noexcept { return fetch_sub(1) - 1; }
    
    operator T() const noexcept { return load(); }
    T operator=(T desired) noexcept { store(desired); return desired; }
};

/**
 * @brief Cache-friendly array with intelligent prefetching
 */
template<typename T>
class CacheFriendlyArray {
private:
    T* data_;
    usize size_;
    usize capacity_;
    usize cache_line_size_;
    
    // Prefetching configuration
    bool prefetch_enabled_;
    usize prefetch_distance_;
    
    // Performance monitoring
    mutable std::atomic<u64> access_count_{0};
    mutable std::atomic<u64> sequential_accesses_{0};
    mutable std::atomic<u64> random_accesses_{0};
    mutable usize last_accessed_index_{USIZE_MAX};
    
    CacheAlignedAllocator allocator_;
    
public:
    explicit CacheFriendlyArray(usize initial_capacity = 0, bool enable_prefetch = true)
        : data_(nullptr), size_(0), capacity_(0), 
          cache_line_size_(64), prefetch_enabled_(enable_prefetch), prefetch_distance_(2) {
        
        if (initial_capacity > 0) {
            reserve(initial_capacity);
        }
    }
    
    ~CacheFriendlyArray() {
        clear();
        if (data_) {
            allocator_.deallocate(data_);
        }
    }
    
    // Non-copyable for simplicity (could be implemented)
    CacheFriendlyArray(const CacheFriendlyArray&) = delete;
    CacheFriendlyArray& operator=(const CacheFriendlyArray&) = delete;
    
    // Movable
    CacheFriendlyArray(CacheFriendlyArray&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_),
          cache_line_size_(other.cache_line_size_), prefetch_enabled_(other.prefetch_enabled_),
          prefetch_distance_(other.prefetch_distance_) {
        
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    
    CacheFriendlyArray& operator=(CacheFriendlyArray&& other) noexcept {
        if (this != &other) {
            if (data_) {
                clear();
                allocator_.deallocate(data_);
            }
            
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            cache_line_size_ = other.cache_line_size_;
            prefetch_enabled_ = other.prefetch_enabled_;
            prefetch_distance_ = other.prefetch_distance_;
            
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }
    
    /**
     * @brief Element access with intelligent prefetching
     */
    T& operator[](usize index) {
        track_access(index);
        
        if (prefetch_enabled_ && index + prefetch_distance_ < size_) {
            prefetch_cache_line(&data_[index + prefetch_distance_]);
        }
        
        return data_[index];
    }
    
    const T& operator[](usize index) const {
        track_access(index);
        
        if (prefetch_enabled_ && index + prefetch_distance_ < size_) {
            prefetch_cache_line(&data_[index + prefetch_distance_]);
        }
        
        return data_[index];
    }
    
    T& at(usize index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return (*this)[index];
    }
    
    const T& at(usize index) const {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return (*this)[index];
    }
    
    /**
     * @brief Add element with cache-friendly growth
     */
    void push_back(const T& value) {
        if (size_ >= capacity_) {
            grow_capacity();
        }
        
        new(&data_[size_]) T(value);
        ++size_;
    }
    
    void push_back(T&& value) {
        if (size_ >= capacity_) {
            grow_capacity();
        }
        
        new(&data_[size_]) T(std::move(value));
        ++size_;
    }
    
    template<typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ >= capacity_) {
            grow_capacity();
        }
        
        new(&data_[size_]) T(std::forward<Args>(args)...);
        return data_[size_++];
    }
    
    /**
     * @brief Remove last element
     */
    void pop_back() {
        if (size_ > 0) {
            --size_;
            data_[size_].~T();
        }
    }
    
    /**
     * @brief Reserve capacity with cache-line alignment
     */
    void reserve(usize new_capacity) {
        if (new_capacity <= capacity_) return;
        
        // Round up to cache line boundary for optimal access
        usize elements_per_line = cache_line_size_ / sizeof(T);
        if (elements_per_line > 0) {
            new_capacity = ((new_capacity + elements_per_line - 1) / elements_per_line) * elements_per_line;
        }
        
        T* new_data = allocator_.allocate<T>(new_capacity);
        if (!new_data) {
            throw std::bad_alloc();
        }
        
        // Move existing elements
        for (usize i = 0; i < size_; ++i) {
            new(&new_data[i]) T(std::move(data_[i]));
            data_[i].~T();
        }
        
        if (data_) {
            allocator_.deallocate(data_);
        }
        
        data_ = new_data;
        capacity_ = new_capacity;
    }
    
    /**
     * @brief Resize array
     */
    void resize(usize new_size) {
        if (new_size > capacity_) {
            reserve(new_size);
        }
        
        if (new_size > size_) {
            // Construct new elements
            for (usize i = size_; i < new_size; ++i) {
                new(&data_[i]) T();
            }
        } else if (new_size < size_) {
            // Destruct excess elements
            for (usize i = new_size; i < size_; ++i) {
                data_[i].~T();
            }
        }
        
        size_ = new_size;
    }
    
    /**
     * @brief Clear all elements
     */
    void clear() {
        for (usize i = 0; i < size_; ++i) {
            data_[i].~T();
        }
        size_ = 0;
    }
    
    // Properties
    usize size() const noexcept { return size_; }
    usize capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size_ == 0; }
    
    T* data() noexcept { return data_; }
    const T* data() const noexcept { return data_; }
    
    // Iterators
    T* begin() noexcept { return data_; }
    T* end() noexcept { return data_ + size_; }
    const T* begin() const noexcept { return data_; }
    const T* end() const noexcept { return data_ + size_; }
    const T* cbegin() const noexcept { return data_; }
    const T* cend() const noexcept { return data_ + size_; }
    
    /**
     * @brief Sequential access iterator with prefetching
     */
    class SequentialIterator {
    private:
        T* ptr_;
        T* end_;
        usize prefetch_distance_;
        
    public:
        SequentialIterator(T* ptr, T* end, usize prefetch_dist = 2)
            : ptr_(ptr), end_(end), prefetch_distance_(prefetch_dist) {
            if (ptr_ + prefetch_distance_ < end_) {
                prefetch_cache_line(ptr_ + prefetch_distance_);
            }
        }
        
        T& operator*() const { return *ptr_; }
        T* operator->() const { return ptr_; }
        
        SequentialIterator& operator++() {
            ++ptr_;
            if (ptr_ + prefetch_distance_ < end_) {
                prefetch_cache_line(ptr_ + prefetch_distance_);
            }
            return *this;
        }
        
        SequentialIterator operator++(int) {
            SequentialIterator tmp = *this;
            ++(*this);
            return tmp;
        }
        
        bool operator==(const SequentialIterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const SequentialIterator& other) const { return ptr_ != other.ptr_; }
    };
    
    SequentialIterator sequential_begin() {
        return SequentialIterator(data_, data_ + size_, prefetch_distance_);
    }
    
    SequentialIterator sequential_end() {
        return SequentialIterator(data_ + size_, data_ + size_, prefetch_distance_);
    }
    
    /**
     * @brief Configure prefetching
     */
    void set_prefetch_enabled(bool enabled) { prefetch_enabled_ = enabled; }
    void set_prefetch_distance(usize distance) { prefetch_distance_ = distance; }
    
    /**
     * @brief Get access pattern statistics
     */
    struct AccessStatistics {
        u64 total_accesses;
        u64 sequential_accesses;
        u64 random_accesses;
        f64 sequential_ratio;
        f64 cache_efficiency_estimate;
    };
    
    AccessStatistics get_access_statistics() const {
        AccessStatistics stats{};
        stats.total_accesses = access_count_.load();
        stats.sequential_accesses = sequential_accesses_.load();
        stats.random_accesses = random_accesses_.load();
        
        if (stats.total_accesses > 0) {
            stats.sequential_ratio = static_cast<f64>(stats.sequential_accesses) / stats.total_accesses;
        }
        
        // Estimate cache efficiency based on access patterns
        stats.cache_efficiency_estimate = 0.9 * stats.sequential_ratio + 0.1 * (1.0 - stats.sequential_ratio);
        
        return stats;
    }
    
private:
    void grow_capacity() {
        usize new_capacity = capacity_ == 0 ? 8 : capacity_ * 2;
        reserve(new_capacity);
    }
    
    void track_access(usize index) const {
        access_count_.fetch_add(1, std::memory_order_relaxed);
        
        usize last_index = last_accessed_index_;
        if (last_index != USIZE_MAX) {
            if (index == last_index + 1 || (last_index > 0 && index == last_index - 1)) {
                sequential_accesses_.fetch_add(1, std::memory_order_relaxed);
            } else {
                random_accesses_.fetch_add(1, std::memory_order_relaxed);
            }
        }
        
        last_accessed_index_ = index;
    }
    
    void prefetch_cache_line(const void* addr) const {
#ifdef __x86_64__
        _mm_prefetch(static_cast<const char*>(addr), _MM_HINT_T0);
#elif defined(__aarch64__)
        __builtin_prefetch(addr, 0, 3);
#else
        // No prefetch available
        (void)addr;
#endif
    }
};

//=============================================================================
// Hot/Cold Data Separation
//=============================================================================

/**
 * @brief Data structure with hot/cold separation for optimal cache usage
 */
template<typename THot, typename TCold>
class HotColdSeparatedData {
private:
    struct alignas(core::CACHE_LINE_SIZE) HotData {
        THot data;
        std::atomic<u64> access_count{0};
        f64 last_access_time{0.0};
        
        // Padding to fill cache line
        static constexpr usize padding_size = 
            core::CACHE_LINE_SIZE > sizeof(THot) + sizeof(std::atomic<u64>) + sizeof(f64) ?
            core::CACHE_LINE_SIZE - sizeof(THot) - sizeof(std::atomic<u64>) - sizeof(f64) : 0;
        
        byte padding[padding_size > 0 ? padding_size : 1];
        
        template<typename... Args>
        HotData(Args&&... args) : data(std::forward<Args>(args)...) {}
    };
    
    struct ColdData {
        TCold data;
        std::atomic<u64> access_count{0};
        f64 last_access_time{0.0};
        f64 creation_time;
        
        template<typename... Args>
        ColdData(Args&&... args) : data(std::forward<Args>(args)...), creation_time(get_current_time()) {}
        
    private:
        f64 get_current_time() const {
            using namespace std::chrono;
            return duration<f64>(steady_clock::now().time_since_epoch()).count();
        }
    };
    
    // Hot data is cache-line aligned for fast access
    alignas(core::CACHE_LINE_SIZE) HotData hot_data_;
    
    // Cold data is stored separately to avoid cache pollution
    std::unique_ptr<ColdData> cold_data_;
    
    // Access pattern tracking
    std::atomic<u64> hot_accesses_{0};
    std::atomic<u64> cold_accesses_{0};
    
    // Thresholds for hot/cold classification
    static constexpr u64 HOT_ACCESS_THRESHOLD = 100;
    static constexpr f64 COLD_ACCESS_TIME_THRESHOLD = 1.0; // seconds
    
public:
    template<typename... HotArgs, typename... ColdArgs>
    HotColdSeparatedData(std::tuple<HotArgs...> hot_args, std::tuple<ColdArgs...> cold_args)
        : hot_data_(std::apply([](auto&&... args) { return HotData(std::forward<decltype(args)>(args)...); }, hot_args)),
          cold_data_(std::make_unique<ColdData>(std::apply([](auto&&... args) { return ColdData(std::forward<decltype(args)>(args)...); }, cold_args))) {
    }
    
    template<typename... HotArgs>
    explicit HotColdSeparatedData(HotArgs&&... hot_args)
        : hot_data_(std::forward<HotArgs>(hot_args)...),
          cold_data_(std::make_unique<ColdData>()) {
    }
    
    /**
     * @brief Access hot data (frequent access, cache-friendly)
     */
    THot& hot() {
        hot_data_.access_count.fetch_add(1, std::memory_order_relaxed);
        hot_data_.last_access_time = get_current_time();
        hot_accesses_.fetch_add(1, std::memory_order_relaxed);
        return hot_data_.data;
    }
    
    const THot& hot() const {
        hot_data_.access_count.fetch_add(1, std::memory_order_relaxed);
        hot_data_.last_access_time = get_current_time();
        hot_accesses_.fetch_add(1, std::memory_order_relaxed);
        return hot_data_.data;
    }
    
    /**
     * @brief Access cold data (infrequent access, may cause cache miss)
     */
    TCold& cold() {
        if (!cold_data_) {
            cold_data_ = std::make_unique<ColdData>();
        }
        
        cold_data_->access_count.fetch_add(1, std::memory_order_relaxed);
        cold_data_->last_access_time = get_current_time();
        cold_accesses_.fetch_add(1, std::memory_order_relaxed);
        return cold_data_->data;
    }
    
    const TCold& cold() const {
        if (!cold_data_) {
            cold_data_ = std::make_unique<ColdData>();
        }
        
        cold_data_->access_count.fetch_add(1, std::memory_order_relaxed);
        cold_data_->last_access_time = get_current_time();
        cold_accesses_.fetch_add(1, std::memory_order_relaxed);
        return cold_data_->data;
    }
    
    /**
     * @brief Get access pattern analysis
     */
    struct AccessAnalysis {
        u64 hot_accesses;
        u64 cold_accesses;
        u64 hot_data_accesses;
        u64 cold_data_accesses;
        f64 hot_access_ratio;
        f64 cache_efficiency_estimate;
        bool is_hot_data_truly_hot;
        bool is_cold_data_truly_cold;
    };
    
    AccessAnalysis analyze_access_pattern() const {
        AccessAnalysis analysis{};
        
        analysis.hot_accesses = hot_accesses_.load();
        analysis.cold_accesses = cold_accesses_.load();
        analysis.hot_data_accesses = hot_data_.access_count.load();
        if (cold_data_) {
            analysis.cold_data_accesses = cold_data_->access_count.load();
        }
        
        u64 total_accesses = analysis.hot_accesses + analysis.cold_accesses;
        if (total_accesses > 0) {
            analysis.hot_access_ratio = static_cast<f64>(analysis.hot_accesses) / total_accesses;
        }
        
        // Determine if hot data is truly hot
        analysis.is_hot_data_truly_hot = analysis.hot_data_accesses >= HOT_ACCESS_THRESHOLD;
        
        // Determine if cold data is truly cold
        f64 current_time = get_current_time();
        if (cold_data_) {
            f64 time_since_last_access = current_time - cold_data_->last_access_time;
            analysis.is_cold_data_truly_cold = time_since_last_access >= COLD_ACCESS_TIME_THRESHOLD;
        } else {
            analysis.is_cold_data_truly_cold = true;
        }
        
        // Estimate cache efficiency
        analysis.cache_efficiency_estimate = analysis.hot_access_ratio * 0.95 + 
                                           (1.0 - analysis.hot_access_ratio) * 0.3;
        
        return analysis;
    }
    
    /**
     * @brief Get memory layout information
     */
    struct MemoryLayoutInfo {
        usize hot_data_size;
        usize cold_data_size;
        usize hot_data_alignment;
        void* hot_data_address;
        void* cold_data_address;
        usize cache_lines_used;
        bool hot_cold_on_same_cache_line;
    };
    
    MemoryLayoutInfo get_memory_layout_info() const {
        MemoryLayoutInfo info{};
        
        info.hot_data_size = sizeof(HotData);
        info.cold_data_size = cold_data_ ? sizeof(ColdData) : 0;
        info.hot_data_alignment = alignof(HotData);
        info.hot_data_address = const_cast<HotData*>(&hot_data_);
        info.cold_data_address = cold_data_.get();
        
        // Calculate cache lines used
        info.cache_lines_used = (info.hot_data_size + core::CACHE_LINE_SIZE - 1) / core::CACHE_LINE_SIZE;
        if (cold_data_) {
            info.cache_lines_used += (info.cold_data_size + core::CACHE_LINE_SIZE - 1) / core::CACHE_LINE_SIZE;
        }
        
        // Check if hot and cold data share cache lines (should not happen with proper alignment)
        if (cold_data_) {
            uintptr_t hot_addr = reinterpret_cast<uintptr_t>(info.hot_data_address);
            uintptr_t cold_addr = reinterpret_cast<uintptr_t>(info.cold_data_address);
            uintptr_t hot_cache_line = hot_addr / core::CACHE_LINE_SIZE;
            uintptr_t cold_cache_line = cold_addr / core::CACHE_LINE_SIZE;
            info.hot_cold_on_same_cache_line = (hot_cache_line == cold_cache_line);
        } else {
            info.hot_cold_on_same_cache_line = false;
        }
        
        return info;
    }
    
private:
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Cache-Aware Thread Pool Data Structure
//=============================================================================

/**
 * @brief Thread-local data with cache-line isolation to prevent false sharing
 */
template<typename T, usize MaxThreads = 64>
class CacheIsolatedThreadData {
private:
    struct alignas(core::CACHE_LINE_SIZE) ThreadEntry {
        T data;
        std::atomic<bool> active{false};
        std::thread::id thread_id{};
        std::atomic<u64> access_count{0};
        
        // Padding to ensure each entry occupies exactly one cache line
        static constexpr usize data_size = sizeof(T) + sizeof(std::atomic<bool>) + 
                                          sizeof(std::thread::id) + sizeof(std::atomic<u64>);
        static constexpr usize padding_size = 
            data_size < core::CACHE_LINE_SIZE ? core::CACHE_LINE_SIZE - data_size : 0;
        
        byte padding[padding_size > 0 ? padding_size : 1];
        
        template<typename... Args>
        ThreadEntry(Args&&... args) : data(std::forward<Args>(args)...) {}
    };
    
    alignas(core::CACHE_LINE_SIZE) std::array<ThreadEntry, MaxThreads> thread_entries_;
    std::atomic<usize> active_threads_{0};
    std::unordered_map<std::thread::id, usize> thread_to_index_;
    mutable std::shared_mutex thread_map_mutex_;
    
    // Performance monitoring
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_accesses_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> cache_line_conflicts_{0};
    
public:
    template<typename... Args>
    CacheIsolatedThreadData(Args&&... default_args) {
        // Initialize all entries with default construction
        for (auto& entry : thread_entries_) {
            entry = ThreadEntry(args...);
        }
    }
    
    /**
     * @brief Get thread-local data with cache isolation
     */
    T& get_local() {
        std::thread::id current_thread = std::this_thread::get_id();
        
        // Try to find existing entry
        {
            std::shared_lock<std::shared_mutex> lock(thread_map_mutex_);
            auto it = thread_to_index_.find(current_thread);
            if (it != thread_to_index_.end()) {
                auto& entry = thread_entries_[it->second];
                entry.access_count.fetch_add(1, std::memory_order_relaxed);
                total_accesses_.fetch_add(1, std::memory_order_relaxed);
                return entry.data;
            }
        }
        
        // Need to allocate new entry
        {
            std::unique_lock<std::shared_mutex> lock(thread_map_mutex_);
            
            // Double-check in case another thread allocated it
            auto it = thread_to_index_.find(current_thread);
            if (it != thread_to_index_.end()) {
                auto& entry = thread_entries_[it->second];
                entry.access_count.fetch_add(1, std::memory_order_relaxed);
                total_accesses_.fetch_add(1, std::memory_order_relaxed);
                return entry.data;
            }
            
            // Find free slot
            usize free_index = USIZE_MAX;
            for (usize i = 0; i < MaxThreads; ++i) {
                if (!thread_entries_[i].active.load()) {
                    if (thread_entries_[i].active.compare_exchange_strong(
                            bool{false}, true, std::memory_order_acquire)) {
                        free_index = i;
                        break;
                    }
                }
            }
            
            if (free_index == USIZE_MAX) {
                throw std::runtime_error("Maximum number of threads exceeded");
            }
            
            // Initialize entry for this thread
            auto& entry = thread_entries_[free_index];
            entry.thread_id = current_thread;
            thread_to_index_[current_thread] = free_index;
            active_threads_.fetch_add(1, std::memory_order_relaxed);
            
            entry.access_count.fetch_add(1, std::memory_order_relaxed);
            total_accesses_.fetch_add(1, std::memory_order_relaxed);
            return entry.data;
        }
    }
    
    /**
     * @brief Apply function to all active thread data
     */
    template<typename Func>
    void for_each_active(Func&& func) {
        std::shared_lock<std::shared_mutex> lock(thread_map_mutex_);
        
        for (usize i = 0; i < MaxThreads; ++i) {
            if (thread_entries_[i].active.load()) {
                func(thread_entries_[i].data);
            }
        }
    }
    
    /**
     * @brief Apply function to all active thread data with thread ID
     */
    template<typename Func>
    void for_each_active_with_id(Func&& func) {
        std::shared_lock<std::shared_mutex> lock(thread_map_mutex_);
        
        for (usize i = 0; i < MaxThreads; ++i) {
            if (thread_entries_[i].active.load()) {
                func(thread_entries_[i].thread_id, thread_entries_[i].data);
            }
        }
    }
    
    /**
     * @brief Aggregate data from all threads
     */
    template<typename Aggregator, typename Result = T>
    Result aggregate(Aggregator&& agg, Result initial = Result{}) {
        std::shared_lock<std::shared_mutex> lock(thread_map_mutex_);
        
        Result result = initial;
        for (usize i = 0; i < MaxThreads; ++i) {
            if (thread_entries_[i].active.load()) {
                result = agg(result, thread_entries_[i].data);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Remove thread data when thread exits
     */
    void cleanup_thread(std::thread::id thread_id = std::this_thread::get_id()) {
        std::unique_lock<std::shared_mutex> lock(thread_map_mutex_);
        
        auto it = thread_to_index_.find(thread_id);
        if (it != thread_to_index_.end()) {
            usize index = it->second;
            thread_entries_[index].active.store(false);
            thread_to_index_.erase(it);
            active_threads_.fetch_sub(1, std::memory_order_relaxed);
        }
    }
    
    /**
     * @brief Get cache performance statistics
     */
    struct CacheStatistics {
        usize active_threads;
        usize max_threads;
        u64 total_accesses;
        u64 cache_line_conflicts;
        f64 cache_efficiency_estimate;
        std::vector<std::pair<std::thread::id, u64>> per_thread_access_counts;
    };
    
    CacheStatistics get_cache_statistics() const {
        std::shared_lock<std::shared_mutex> lock(thread_map_mutex_);
        
        CacheStatistics stats{};
        stats.active_threads = active_threads_.load();
        stats.max_threads = MaxThreads;
        stats.total_accesses = total_accesses_.load();
        stats.cache_line_conflicts = cache_line_conflicts_.load();
        
        // Calculate cache efficiency (simplified)
        if (stats.total_accesses > 0) {
            f64 conflict_ratio = static_cast<f64>(stats.cache_line_conflicts) / stats.total_accesses;
            stats.cache_efficiency_estimate = 1.0 - conflict_ratio;
        } else {
            stats.cache_efficiency_estimate = 1.0;
        }
        
        // Collect per-thread access counts
        for (usize i = 0; i < MaxThreads; ++i) {
            if (thread_entries_[i].active.load()) {
                u64 accesses = thread_entries_[i].access_count.load();
                stats.per_thread_access_counts.emplace_back(thread_entries_[i].thread_id, accesses);
            }
        }
        
        return stats;
    }
    
    usize get_active_thread_count() const {
        return active_threads_.load();
    }
    
    static constexpr usize get_cache_line_size() {
        return core::CACHE_LINE_SIZE;
    }
    
    static constexpr usize get_entry_size() {
        return sizeof(ThreadEntry);
    }
};

//=============================================================================
// Cache Performance Measurement and Visualization
//=============================================================================

/**
 * @brief Cache behavior analyzer and visualizer
 */
class CacheBehaviorAnalyzer {
private:
    struct AccessPattern {
        std::vector<usize> addresses;
        std::vector<f64> timestamps;
        std::string pattern_name;
        f64 measured_performance;
    };
    
    std::vector<AccessPattern> recorded_patterns_;
    CacheTopologyAnalyzer& cache_analyzer_;
    
    // Performance counters (would use hardware counters in real implementation)
    std::atomic<u64> estimated_l1_misses_{0};
    std::atomic<u64> estimated_l2_misses_{0};
    std::atomic<u64> estimated_l3_misses_{0};
    std::atomic<u64> total_memory_accesses_{0};
    
public:
    explicit CacheBehaviorAnalyzer(CacheTopologyAnalyzer& analyzer) 
        : cache_analyzer_(analyzer) {}
    
    /**
     * @brief Record memory access pattern
     */
    void record_access_pattern(const std::string& name, 
                              const std::vector<usize>& addresses,
                              f64 measured_time) {
        AccessPattern pattern;
        pattern.pattern_name = name;
        pattern.addresses = addresses;
        pattern.measured_performance = measured_time;
        
        // Generate timestamps (simplified)
        pattern.timestamps.resize(addresses.size());
        f64 time_step = measured_time / addresses.size();
        for (usize i = 0; i < addresses.size(); ++i) {
            pattern.timestamps[i] = i * time_step;
        }
        
        recorded_patterns_.push_back(std::move(pattern));
        
        // Update performance counters
        analyze_pattern(recorded_patterns_.back());
    }
    
    /**
     * @brief Benchmark different access patterns
     */
    void benchmark_access_patterns() {
        const usize buffer_size = 16 * 1024 * 1024; // 16MB
        const usize iterations = 1000000;
        
        // Allocate test buffer
        CacheAlignedAllocator allocator;
        u64* buffer = allocator.allocate<u64>(buffer_size / sizeof(u64));
        
        // Pattern 1: Sequential access
        benchmark_sequential_access(buffer, buffer_size, iterations);
        
        // Pattern 2: Random access
        benchmark_random_access(buffer, buffer_size, iterations);
        
        // Pattern 3: Strided access
        benchmark_strided_access(buffer, buffer_size, iterations, 64);
        
        // Pattern 4: Cache-friendly chunks
        benchmark_chunked_access(buffer, buffer_size, iterations);
        
        allocator.deallocate(buffer);
    }
    
    /**
     * @brief Generate cache behavior visualization data
     */
    struct VisualizationData {
        std::vector<std::string> pattern_names;
        std::vector<f64> performance_scores;
        std::vector<f64> cache_miss_rates;
        std::vector<std::vector<std::pair<usize, f64>>> access_heatmaps;
        std::string optimization_recommendations;
    };
    
    VisualizationData generate_visualization_data() const {
        VisualizationData data;
        
        for (const auto& pattern : recorded_patterns_) {
            data.pattern_names.push_back(pattern.pattern_name);
            data.performance_scores.push_back(pattern.measured_performance);
            
            // Calculate miss rate (simplified)
            f64 miss_rate = cache_analyzer_.predict_miss_rate(pattern.addresses, 
                                                            pattern.addresses.size() * sizeof(usize));
            data.cache_miss_rates.push_back(miss_rate);
            
            // Generate heatmap data
            std::vector<std::pair<usize, f64>> heatmap;
            for (usize i = 0; i < pattern.addresses.size(); ++i) {
                heatmap.emplace_back(pattern.addresses[i], pattern.timestamps[i]);
            }
            data.access_heatmaps.push_back(std::move(heatmap));
        }
        
        // Generate recommendations
        data.optimization_recommendations = generate_optimization_recommendations();
        
        return data;
    }
    
    /**
     * @brief Get cache performance statistics
     */
    struct CachePerformanceStats {
        u64 estimated_l1_misses;
        u64 estimated_l2_misses;
        u64 estimated_l3_misses;
        u64 total_memory_accesses;
        f64 l1_miss_rate;
        f64 l2_miss_rate;
        f64 l3_miss_rate;
        f64 overall_cache_efficiency;
    };
    
    CachePerformanceStats get_performance_stats() const {
        CachePerformanceStats stats{};
        
        stats.estimated_l1_misses = estimated_l1_misses_.load();
        stats.estimated_l2_misses = estimated_l2_misses_.load();
        stats.estimated_l3_misses = estimated_l3_misses_.load();
        stats.total_memory_accesses = total_memory_accesses_.load();
        
        if (stats.total_memory_accesses > 0) {
            stats.l1_miss_rate = static_cast<f64>(stats.estimated_l1_misses) / stats.total_memory_accesses;
            stats.l2_miss_rate = static_cast<f64>(stats.estimated_l2_misses) / stats.total_memory_accesses;
            stats.l3_miss_rate = static_cast<f64>(stats.estimated_l3_misses) / stats.total_memory_accesses;
            
            u64 total_hits = stats.total_memory_accesses - stats.estimated_l1_misses;
            stats.overall_cache_efficiency = static_cast<f64>(total_hits) / stats.total_memory_accesses;
        }
        
        return stats;
    }
    
private:
    void benchmark_sequential_access(u64* buffer, usize buffer_size, usize iterations);
    void benchmark_random_access(u64* buffer, usize buffer_size, usize iterations);
    void benchmark_strided_access(u64* buffer, usize buffer_size, usize iterations, usize stride);
    void benchmark_chunked_access(u64* buffer, usize buffer_size, usize iterations);
    
    void analyze_pattern(const AccessPattern& pattern);
    std::string generate_optimization_recommendations() const;
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Global Cache Management
//=============================================================================

/**
 * @brief Global cache topology analyzer instance
 */
CacheTopologyAnalyzer& get_global_cache_analyzer();

/**
 * @brief Global cache-aligned allocator instance
 */
CacheAlignedAllocator& get_global_cache_aligned_allocator();

/**
 * @brief Global cache behavior analyzer instance
 */
CacheBehaviorAnalyzer& get_global_cache_behavior_analyzer();

} // namespace ecscope::memory::cache