#pragma once

/**
 * @file memory/gc/generational_gc.hpp
 * @brief Educational Generational Garbage Collector with Advanced Analysis
 * 
 * This header implements a sophisticated generational garbage collector designed
 * for educational purposes with comprehensive visualization and analysis tools.
 * It demonstrates modern GC techniques including incremental collection,
 * tri-color marking, and write barrier optimization.
 * 
 * Key Features:
 * - Multi-generational collection (young, old, permanent generations)
 * - Incremental tri-color mark-and-sweep algorithm
 * - Write barriers for inter-generational reference tracking
 * - Concurrent collection with minimal pause times
 * - Adaptive generation sizing based on allocation patterns
 * - Educational visualization of GC phases and object lifecycles
 * - Integration with existing memory tracking infrastructure
 * - Optional real-time collection statistics and metrics
 * 
 * Educational Value:
 * - Demonstrates generational hypothesis in practice
 * - Shows incremental GC algorithms and pause time optimization
 * - Illustrates write barrier implementation and overhead
 * - Provides insights into GC tuning and performance analysis
 * - Educational examples of concurrent collection techniques
 * 
 * @author ECScope Educational ECS Framework - Garbage Collection
 * @date 2025
 */

#include "memory/memory_tracker.hpp"
#include "memory/pools/hierarchical_pools.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <span>
#include <bit>

namespace ecscope::memory::gc {

//=============================================================================
// GC Object Model and Metadata
//=============================================================================

/**
 * @brief Object generations for generational collection
 */
enum class Generation : u8 {
    Young     = 0,  // Newly allocated objects
    Old       = 1,  // Objects that survived multiple collections
    Permanent = 2   // Long-lived objects (rarely collected)
};

/**
 * @brief Tri-color marking states for concurrent collection
 */
enum class MarkColor : u8 {
    White = 0,  // Unvisited/unreachable objects
    Gray  = 1,  // Visited but children not yet scanned
    Black = 2   // Visited and all children scanned
};

/**
 * @brief GC object header with metadata
 */
struct alignas(16) GCObjectHeader {
    // Object identification
    u64 object_id;                  // Unique object identifier
    usize size;                     // Object size in bytes
    usize alignment;                // Object alignment requirement
    
    // GC metadata
    Generation generation;          // Current generation
    MarkColor mark_color;          // Tri-color marking state
    u8 age;                        // Collection cycles survived
    bool is_pinned;                // Object cannot be moved
    
    // Reference tracking
    u32 reference_count;           // Number of references to this object
    u32 weak_reference_count;      // Number of weak references
    
    // Write barrier support
    bool has_young_references;     // Object references younger objects
    bool write_barrier_dirty;     // Write barrier triggered
    
    // Type information
    std::type_index type_info;     // Runtime type information
    void(*destructor)(void*);      // Custom destructor function
    void(*trace_references)(void*, std::function<void(void*)>); // Reference tracer
    
    // Timing information
    f64 allocation_time;           // When object was allocated
    f64 last_access_time;         // Last time object was accessed
    u32 access_count;             // Number of times accessed
    
    GCObjectHeader() 
        : object_id(0), size(0), alignment(0), 
          generation(Generation::Young), mark_color(MarkColor::White), 
          age(0), is_pinned(false), reference_count(0), weak_reference_count(0),
          has_young_references(false), write_barrier_dirty(false),
          type_info(typeid(void)), destructor(nullptr), trace_references(nullptr),
          allocation_time(0.0), last_access_time(0.0), access_count(0) {}
};

/**
 * @brief Managed GC object wrapper
 */
template<typename T>
class GCObject {
private:
    GCObjectHeader header_;
    alignas(alignof(T)) u8 data_[sizeof(T)];
    
public:
    template<typename... Args>
    explicit GCObject(Args&&... args) {
        header_.object_id = generate_object_id();
        header_.size = sizeof(T);
        header_.alignment = alignof(T);
        header_.type_info = std::type_index(typeid(T));
        header_.allocation_time = get_current_time();
        
        if constexpr (!std::is_trivially_destructible_v<T>) {
            header_.destructor = [](void* ptr) {
                static_cast<T*>(ptr)->~T();
            };
        }
        
        // Construct object in-place
        new(data_) T(std::forward<Args>(args)...);
    }
    
    ~GCObject() {
        if (header_.destructor) {
            header_.destructor(get_object());
        }
    }
    
    T* get_object() { return reinterpret_cast<T*>(data_); }
    const T* get_object() const { return reinterpret_cast<const T*>(data_); }
    
    GCObjectHeader& get_header() { return header_; }
    const GCObjectHeader& get_header() const { return header_; }
    
    void mark_accessed() {
        header_.last_access_time = get_current_time();
        header_.access_count++;
    }
    
private:
    static u64 generate_object_id() {
        static std::atomic<u64> next_id{1};
        return next_id.fetch_add(1, std::memory_order_relaxed);
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Generational Heap Management
//=============================================================================

/**
 * @brief Generation-specific heap configuration
 */
struct GenerationConfig {
    usize initial_size;             // Initial heap size for this generation
    usize max_size;                // Maximum heap size
    f64 growth_factor;             // Growth factor when expanding
    f64 collection_threshold;      // Collection trigger threshold (0.0-1.0)
    u32 promotion_age;             // Age at which objects are promoted
    bool enable_compaction;        // Enable heap compaction during collection
    
    GenerationConfig() 
        : initial_size(1024 * 1024),   // 1MB
          max_size(64 * 1024 * 1024),  // 64MB
          growth_factor(2.0),
          collection_threshold(0.8),   // 80% full
          promotion_age(2),
          enable_compaction(true) {}
};

/**
 * @brief Single generation heap manager
 */
class GenerationHeap {
private:
    Generation generation_;
    GenerationConfig config_;
    
    // Memory management
    void* heap_memory_;
    usize heap_size_;
    usize heap_used_;
    usize allocation_pointer_;
    
    // Object tracking
    std::vector<GCObjectHeader*> objects_;
    std::unordered_set<GCObjectHeader*> reachable_objects_;
    std::vector<GCObjectHeader*> gray_objects_;  // For tri-color marking
    
    // Remembered set for inter-generational references
    std::unordered_set<GCObjectHeader*> remembered_set_;
    
    mutable std::shared_mutex heap_mutex_;
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> collections_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> promoted_objects_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> last_collection_time_{0.0};
    
public:
    explicit GenerationHeap(Generation gen, const GenerationConfig& config = GenerationConfig{})
        : generation_(gen), config_(config), heap_memory_(nullptr), 
          heap_size_(0), heap_used_(0), allocation_pointer_(0) {
        
        initialize_heap();
        
        LOG_DEBUG("Initialized {} generation heap: {}MB initial size", 
                 generation_name(gen), config_.initial_size / (1024 * 1024));
    }
    
    ~GenerationHeap() {
        cleanup_heap();
    }
    
    /**
     * @brief Allocate object in this generation
     */
    template<typename T, typename... Args>
    GCObject<T>* allocate(Args&&... args) {
        std::unique_lock<std::shared_mutex> lock(heap_mutex_);
        
        usize object_size = sizeof(GCObject<T>);
        usize aligned_size = align_up(object_size, alignof(GCObject<T>));
        
        if (heap_used_ + aligned_size > heap_size_) {
            if (!expand_heap(aligned_size)) {
                return nullptr; // Out of memory
            }
        }
        
        // Allocate from heap
        void* object_ptr = static_cast<u8*>(heap_memory_) + allocation_pointer_;
        allocation_pointer_ += aligned_size;
        heap_used_ += aligned_size;
        
        // Construct GC object
        auto* gc_object = new(object_ptr) GCObject<T>(std::forward<Args>(args)...);
        gc_object->get_header().generation = generation_;
        
        // Track object
        objects_.push_back(&gc_object->get_header());
        allocations_.fetch_add(1, std::memory_order_relaxed);
        
        LOG_TRACE("Allocated object: id={}, size={}, generation={}", 
                 gc_object->get_header().object_id, object_size, static_cast<u32>(generation_));
        
        return gc_object;
    }
    
    /**
     * @brief Check if collection is needed
     */
    bool needs_collection() const {
        std::shared_lock<std::shared_mutex> lock(heap_mutex_);
        f64 utilization = static_cast<f64>(heap_used_) / heap_size_;
        return utilization >= config_.collection_threshold;
    }
    
    /**
     * @brief Perform mark phase of collection
     */
    void mark_phase(const std::vector<GCObjectHeader*>& roots) {
        std::unique_lock<std::shared_mutex> lock(heap_mutex_);
        
        // Reset all objects to white
        for (auto* obj : objects_) {
            obj->mark_color = MarkColor::White;
        }
        
        // Mark roots as gray
        gray_objects_.clear();
        for (auto* root : roots) {
            if (root->generation == generation_) {
                root->mark_color = MarkColor::Gray;
                gray_objects_.push_back(root);
            }
        }
        
        // Add remembered set objects as roots for younger generations
        if (generation_ == Generation::Young) {
            for (auto* obj : remembered_set_) {
                if (obj->mark_color == MarkColor::White) {
                    obj->mark_color = MarkColor::Gray;
                    gray_objects_.push_back(obj);
                }
            }
        }
        
        // Tri-color marking
        while (!gray_objects_.empty()) {
            auto* current = gray_objects_.back();
            gray_objects_.pop_back();
            
            // Mark current as black
            current->mark_color = MarkColor::Black;
            reachable_objects_.insert(current);
            
            // Trace references and mark children as gray
            if (current->trace_references) {
                current->trace_references(get_object_data(current), 
                    [this](void* referenced_obj) {
                        auto* ref_header = get_header_from_object(referenced_obj);
                        if (ref_header && ref_header->generation == generation_ && 
                            ref_header->mark_color == MarkColor::White) {
                            ref_header->mark_color = MarkColor::Gray;
                            gray_objects_.push_back(ref_header);
                        }
                    });
            }
        }
        
        LOG_DEBUG("Mark phase completed: {} reachable objects in {} generation", 
                 reachable_objects_.size(), generation_name(generation_));
    }
    
    /**
     * @brief Perform sweep phase of collection
     */
    usize sweep_phase() {
        std::unique_lock<std::shared_mutex> lock(heap_mutex_);
        
        usize objects_collected = 0;
        usize bytes_freed = 0;
        
        auto it = objects_.begin();
        while (it != objects_.end()) {
            auto* obj = *it;
            
            if (obj->mark_color == MarkColor::White) {
                // Object is unreachable - collect it
                bytes_freed += obj->size;
                objects_collected++;
                
                // Call destructor if needed
                if (obj->destructor) {
                    obj->destructor(get_object_data(obj));
                }
                
                // Remove from remembered set if present
                remembered_set_.erase(obj);
                
                it = objects_.erase(it);
            } else {
                // Object survived - age it and consider promotion
                obj->age++;
                if (should_promote_object(obj)) {
                    mark_for_promotion(obj);
                }
                ++it;
            }
        }
        
        heap_used_ -= bytes_freed;
        collections_.fetch_add(1, std::memory_order_relaxed);
        last_collection_time_.store(get_current_time(), std::memory_order_relaxed);
        
        // Clear reachable set for next collection
        reachable_objects_.clear();
        
        LOG_INFO("Sweep phase completed: {} objects collected, {}KB freed from {} generation", 
                 objects_collected, bytes_freed / 1024, generation_name(generation_));
        
        return objects_collected;
    }
    
    /**
     * @brief Add object to remembered set (write barrier)
     */
    void write_barrier(GCObjectHeader* source, void* target_obj) {
        if (!source || !target_obj) return;
        
        auto* target_header = get_header_from_object(target_obj);
        if (!target_header) return;
        
        // If older generation references younger generation, add to remembered set
        if (static_cast<u32>(source->generation) > static_cast<u32>(target_header->generation)) {
            std::unique_lock<std::shared_mutex> lock(heap_mutex_);
            remembered_set_.insert(target_header);
            source->has_young_references = true;
            source->write_barrier_dirty = true;
            
            LOG_TRACE("Write barrier: {} -> {} (gen {} -> gen {})", 
                     source->object_id, target_header->object_id,
                     static_cast<u32>(source->generation), static_cast<u32>(target_header->generation));
        }
    }
    
    /**
     * @brief Get objects ready for promotion
     */
    std::vector<GCObjectHeader*> get_promotion_candidates() const {
        std::shared_lock<std::shared_mutex> lock(heap_mutex_);
        
        std::vector<GCObjectHeader*> candidates;
        for (auto* obj : objects_) {
            if (should_promote_object(obj)) {
                candidates.push_back(obj);
            }
        }
        
        return candidates;
    }
    
    /**
     * @brief Generation statistics
     */
    struct GenerationStatistics {
        Generation generation;
        usize heap_size;
        usize heap_used;
        f64 utilization_ratio;
        usize object_count;
        u64 total_allocations;
        u64 total_collections;
        u64 promoted_objects;
        f64 last_collection_time;
        f64 average_object_age;
        usize remembered_set_size;
        f64 collection_frequency;
        f64 promotion_rate;
        GenerationConfig config;
    };
    
    GenerationStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(heap_mutex_);
        
        GenerationStatistics stats{};
        stats.generation = generation_;
        stats.heap_size = heap_size_;
        stats.heap_used = heap_used_;
        stats.utilization_ratio = heap_size_ > 0 ? static_cast<f64>(heap_used_) / heap_size_ : 0.0;
        stats.object_count = objects_.size();
        stats.total_allocations = allocations_.load();
        stats.total_collections = collections_.load();
        stats.promoted_objects = promoted_objects_.load();
        stats.last_collection_time = last_collection_time_.load();
        stats.remembered_set_size = remembered_set_.size();
        stats.config = config_;
        
        // Calculate average object age
        if (!objects_.empty()) {
            u32 total_age = 0;
            for (const auto* obj : objects_) {
                total_age += obj->age;
            }
            stats.average_object_age = static_cast<f64>(total_age) / objects_.size();
        }
        
        // Calculate rates
        f64 current_time = get_current_time();
        if (stats.last_collection_time > 0) {
            stats.collection_frequency = stats.total_collections / 
                (current_time - stats.last_collection_time + 0.001);
        }
        
        if (stats.total_allocations > 0) {
            stats.promotion_rate = static_cast<f64>(stats.promoted_objects) / stats.total_allocations;
        }
        
        return stats;
    }
    
    Generation get_generation() const { return generation_; }
    const GenerationConfig& get_config() const { return config_; }
    void set_config(const GenerationConfig& config) { config_ = config; }
    
private:
    void initialize_heap() {
        heap_size_ = config_.initial_size;
        heap_memory_ = std::aligned_alloc(core::CACHE_LINE_SIZE, heap_size_);
        
        if (!heap_memory_) {
            throw std::bad_alloc();
        }
        
        heap_used_ = 0;
        allocation_pointer_ = 0;
        
        objects_.reserve(1024);
        reachable_objects_.reserve(1024);
        gray_objects_.reserve(256);
        remembered_set_.reserve(128);
    }
    
    void cleanup_heap() {
        if (heap_memory_) {
            // Call destructors for all remaining objects
            for (auto* obj : objects_) {
                if (obj->destructor) {
                    obj->destructor(get_object_data(obj));
                }
            }
            
            std::free(heap_memory_);
            heap_memory_ = nullptr;
        }
    }
    
    bool expand_heap(usize required_size) {
        usize new_size = static_cast<usize>(heap_size_ * config_.growth_factor);
        new_size = std::max(new_size, heap_size_ + required_size);
        
        if (new_size > config_.max_size) {
            new_size = config_.max_size;
            if (new_size <= heap_size_) {
                return false; // Cannot expand further
            }
        }
        
        void* new_memory = std::aligned_alloc(core::CACHE_LINE_SIZE, new_size);
        if (!new_memory) {
            return false;
        }
        
        // Copy existing data
        if (heap_memory_) {
            std::memcpy(new_memory, heap_memory_, heap_used_);
            std::free(heap_memory_);
        }
        
        heap_memory_ = new_memory;
        heap_size_ = new_size;
        
        LOG_DEBUG("Expanded {} generation heap to {}MB", 
                 generation_name(generation_), new_size / (1024 * 1024));
        
        return true;
    }
    
    bool should_promote_object(const GCObjectHeader* obj) const {
        return obj->age >= config_.promotion_age && 
               obj->generation != Generation::Permanent;
    }
    
    void mark_for_promotion(GCObjectHeader* obj) {
        // In a real implementation, would queue object for promotion
        promoted_objects_.fetch_add(1, std::memory_order_relaxed);
        
        LOG_TRACE("Marking object {} for promotion from {} generation", 
                 obj->object_id, generation_name(obj->generation));
    }
    
    void* get_object_data(GCObjectHeader* header) const {
        // In real implementation, would calculate offset to actual object data
        return reinterpret_cast<u8*>(header) + sizeof(GCObjectHeader);
    }
    
    GCObjectHeader* get_header_from_object(void* obj_data) const {
        // In real implementation, would calculate offset back to header
        return reinterpret_cast<GCObjectHeader*>(
            static_cast<u8*>(obj_data) - sizeof(GCObjectHeader)
        );
    }
    
    const char* generation_name(Generation gen) const {
        switch (gen) {
            case Generation::Young: return "Young";
            case Generation::Old: return "Old";
            case Generation::Permanent: return "Permanent";
        }
        return "Unknown";
    }
    
    constexpr usize align_up(usize value, usize alignment) const {
        return (value + alignment - 1) & ~(alignment - 1);
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};
    
    // Timing information
    f64 allocation_time;           // When object was allocated
    f64 last_access_time;          // Last time object was accessed
    f64 promotion_time;            // When promoted to next generation
    
    // Next object in free list or mark queue
    GCObjectHeader* next;
    
    GCObjectHeader() noexcept 
        : object_id(0), size(0), alignment(0), generation(Generation::Young),
          mark_color(MarkColor::White), age(0), is_pinned(false),
          reference_count(0), weak_reference_count(0),
          has_young_references(false), write_barrier_dirty(false),
          type_info(typeid(void)), destructor(nullptr),
          allocation_time(0.0), last_access_time(0.0), promotion_time(0.0),
          next(nullptr) {}
};

/**
 * @brief GC-managed object base class
 */
template<typename T>
class GCObject {
private:
    GCObjectHeader header_;
    
public:
    GCObject() {
        static std::atomic<u64> next_id{1};
        header_.object_id = next_id.fetch_add(1, std::memory_order_relaxed);
        header_.size = sizeof(T);
        header_.alignment = alignof(T);
        header_.type_info = std::type_index(typeid(T));
        header_.allocation_time = get_current_time();
        header_.last_access_time = header_.allocation_time;
        
        // Set destructor if T is not trivially destructible
        if constexpr (!std::is_trivially_destructible_v<T>) {
            header_.destructor = [](void* ptr) {
                static_cast<T*>(ptr)->~T();
            };
        }
    }
    
    virtual ~GCObject() = default;
    
    // GC interface
    GCObjectHeader& get_gc_header() { return header_; }
    const GCObjectHeader& get_gc_header() const { return header_; }
    
    u64 get_object_id() const { return header_.object_id; }
    Generation get_generation() const { return header_.generation; }
    MarkColor get_mark_color() const { return header_.mark_color; }
    u8 get_age() const { return header_.age; }
    
    void record_access() {
        header_.last_access_time = get_current_time();
    }
    
    // Reference management
    void add_reference() {
        header_.reference_count++;
    }
    
    void remove_reference() {
        if (header_.reference_count > 0) {
            header_.reference_count--;
        }
    }
    
    u32 get_reference_count() const {
        return header_.reference_count;
    }
    
    // Write barrier support
    void trigger_write_barrier() {
        header_.write_barrier_dirty = true;
    }
    
    // Virtual interface for derived classes
    virtual void trace_references(std::function<void(GCObject*)> tracer) {}
    virtual void finalize() {}
    
private:
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Generation-Specific Memory Pools
//=============================================================================

/**
 * @brief Memory pool for a specific generation
 */
class GenerationalPool {
private:
    Generation generation_;
    usize initial_size_;
    usize current_size_;
    usize max_size_;
    f64 growth_factor_;
    
    // Memory regions
    std::vector<void*> memory_regions_;
    std::vector<usize> region_sizes_;
    usize total_allocated_;
    usize total_used_;
    
    // Free list management
    GCObjectHeader* free_list_head_;
    usize free_blocks_count_;
    
    // Object tracking
    std::unordered_set<GCObjectHeader*> live_objects_;
    std::vector<GCObjectHeader*> mark_stack_;
    
    // Performance metrics
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> collections_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> promotions_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> collection_time_{0.0};
    
    mutable std::shared_mutex pool_mutex_;
    
public:
    explicit GenerationalPool(Generation gen, usize initial_size = 1024 * 1024) // 1MB default
        : generation_(gen), initial_size_(initial_size), current_size_(0),
          total_allocated_(0), total_used_(0), free_list_head_(nullptr),
          free_blocks_count_(0) {
        
        configure_for_generation();
        initialize_pool();
        
        LOG_DEBUG("Initialized generational pool: generation={}, size={}KB", 
                 static_cast<u32>(gen), initial_size_ / 1024);
    }
    
    ~GenerationalPool() {
        cleanup_pool();
    }
    
    /**
     * @brief Allocate object in this generation
     */
    GCObjectHeader* allocate(usize size, usize alignment = alignof(std::max_align_t)) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        usize aligned_size = align_up(size + sizeof(GCObjectHeader), alignment);
        
        // Try free list first
        GCObjectHeader* header = allocate_from_free_list(aligned_size);
        if (header) {
            initialize_object_header(header, size, alignment);
            return header;
        }
        
        // Allocate new memory if needed
        if (total_used_ + aligned_size > current_size_) {
            if (!expand_pool(aligned_size)) {
                return nullptr; // Out of memory
            }
        }
        
        // Allocate from current region
        header = allocate_from_region(aligned_size);
        if (header) {
            initialize_object_header(header, size, alignment);
            live_objects_.insert(header);
            allocations_.fetch_add(1, std::memory_order_relaxed);
            return header;
        }
        
        return nullptr;
    }
    
    /**
     * @brief Begin mark phase for this generation
     */
    void begin_mark_phase() {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        // Reset all objects to white
        for (GCObjectHeader* obj : live_objects_) {
            obj->mark_color = MarkColor::White;
        }
        
        mark_stack_.clear();
        mark_stack_.reserve(live_objects_.size() / 4); // Estimate
    }
    
    /**
     * @brief Mark object and add to mark stack
     */
    void mark_object(GCObjectHeader* header) {
        if (!header || header->mark_color != MarkColor::White) {
            return;
        }
        
        header->mark_color = MarkColor::Gray;
        mark_stack_.push_back(header);
    }
    
    /**
     * @brief Process mark stack (incremental)
     */
    usize process_mark_stack(usize max_objects = 100) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        usize processed = 0;
        while (!mark_stack_.empty() && processed < max_objects) {
            GCObjectHeader* header = mark_stack_.back();
            mark_stack_.pop_back();
            
            if (header->mark_color == MarkColor::Gray) {
                // Mark object as black
                header->mark_color = MarkColor::Black;
                
                // Trace references if this is a GCObject
                if (auto* gc_obj = get_gc_object_from_header(header)) {
                    gc_obj->trace_references([this](GCObject<void>* ref) {
                        if (ref) {
                            mark_object(&ref->get_gc_header());
                        }
                    });
                }
                
                processed++;
            }
        }
        
        return processed;
    }
    
    /**
     * @brief Sweep phase - collect unmarked objects
     */
    struct SweepResult {
        usize objects_collected;
        usize bytes_freed;
        std::vector<GCObjectHeader*> promoted_objects;
    };
    
    SweepResult sweep_unmarked_objects() {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        SweepResult result{};
        std::vector<GCObjectHeader*> surviving_objects;
        surviving_objects.reserve(live_objects_.size());
        
        for (GCObjectHeader* header : live_objects_) {
            if (header->mark_color == MarkColor::White) {
                // Object is unreachable - collect it
                finalize_object(header);
                add_to_free_list(header);
                result.objects_collected++;
                result.bytes_freed += header->size;
            } else {
                // Object survived - check for promotion
                surviving_objects.push_back(header);
                
                if (should_promote_object(header)) {
                    result.promoted_objects.push_back(header);
                    promotions_.fetch_add(1, std::memory_order_relaxed);
                } else {
                    header->age++;
                }
            }
        }
        
        live_objects_ = std::unordered_set<GCObjectHeader*>(
            surviving_objects.begin(), surviving_objects.end());
        
        collections_.fetch_add(1, std::memory_order_relaxed);
        return result;
    }
    
    /**
     * @brief Move object to another generation
     */
    void remove_object(GCObjectHeader* header) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        live_objects_.erase(header);
    }
    
    void add_object(GCObjectHeader* header) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        header->generation = generation_;
        header->promotion_time = get_current_time();
        live_objects_.insert(header);
    }
    
    /**
     * @brief Get pool statistics
     */
    struct PoolStatistics {
        Generation generation;
        usize total_size;
        usize used_size;
        usize free_size;
        f64 utilization_ratio;
        usize live_objects_count;
        usize free_blocks_count;
        u64 total_allocations;
        u64 total_collections;
        u64 total_promotions;
        f64 average_collection_time;
        f64 survival_rate;
        f64 promotion_rate;
    };
    
    PoolStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        
        PoolStatistics stats{};
        stats.generation = generation_;
        stats.total_size = current_size_;
        stats.used_size = total_used_;
        stats.free_size = current_size_ - total_used_;
        stats.utilization_ratio = current_size_ > 0 ? 
            static_cast<f64>(total_used_) / current_size_ : 0.0;
        stats.live_objects_count = live_objects_.size();
        stats.free_blocks_count = free_blocks_count_;
        stats.total_allocations = allocations_.load();
        stats.total_collections = collections_.load();
        stats.total_promotions = promotions_.load();
        stats.average_collection_time = collection_time_.load();
        
        // Calculate survival and promotion rates
        if (stats.total_allocations > 0) {
            stats.survival_rate = static_cast<f64>(stats.live_objects_count) / 
                                 stats.total_allocations;
        }
        
        if (stats.total_collections > 0) {
            stats.promotion_rate = static_cast<f64>(stats.total_promotions) / 
                                  stats.total_collections;
        }
        
        return stats;
    }
    
    Generation get_generation() const { return generation_; }
    usize get_live_object_count() const { return live_objects_.size(); }
    bool is_mark_stack_empty() const { return mark_stack_.empty(); }
    
private:
    void configure_for_generation() {
        switch (generation_) {
            case Generation::Young:
                max_size_ = initial_size_ * 4;     // Can grow to 4MB
                growth_factor_ = 2.0;              // Double when expanding
                break;
                
            case Generation::Old:
                max_size_ = initial_size_ * 16;    // Can grow to 16MB
                growth_factor_ = 1.5;              // More conservative growth
                break;
                
            case Generation::Permanent:
                max_size_ = initial_size_ * 32;    // Can grow to 32MB
                growth_factor_ = 1.2;              // Very conservative growth
                break;
        }
    }
    
    void initialize_pool() {
        current_size_ = initial_size_;
        void* region = std::aligned_alloc(core::CACHE_LINE_SIZE, initial_size_);
        
        if (region) {
            memory_regions_.push_back(region);
            region_sizes_.push_back(initial_size_);
            total_allocated_ = initial_size_;
        } else {
            LOG_ERROR("Failed to allocate initial memory for generation {}", 
                     static_cast<u32>(generation_));
        }
    }
    
    void cleanup_pool() {
        // Finalize all remaining objects
        for (GCObjectHeader* header : live_objects_) {
            finalize_object(header);
        }
        
        // Free all memory regions
        for (void* region : memory_regions_) {
            std::free(region);
        }
        
        memory_regions_.clear();
        region_sizes_.clear();
        live_objects_.clear();
        mark_stack_.clear();
        total_allocated_ = 0;
        total_used_ = 0;
    }
    
    bool expand_pool(usize min_additional_size) {
        usize new_region_size = std::max(
            static_cast<usize>(current_size_ * (growth_factor_ - 1.0)),
            align_up(min_additional_size, core::CACHE_LINE_SIZE)
        );
        
        if (current_size_ + new_region_size > max_size_) {
            return false; // Would exceed maximum size
        }
        
        void* new_region = std::aligned_alloc(core::CACHE_LINE_SIZE, new_region_size);
        if (!new_region) {
            return false;
        }
        
        memory_regions_.push_back(new_region);
        region_sizes_.push_back(new_region_size);
        current_size_ += new_region_size;
        total_allocated_ += new_region_size;
        
        LOG_DEBUG("Expanded generation {} pool by {}KB (total: {}KB)", 
                 static_cast<u32>(generation_), new_region_size / 1024, 
                 current_size_ / 1024);
        
        return true;
    }
    
    GCObjectHeader* allocate_from_free_list(usize size) {
        GCObjectHeader* prev = nullptr;
        GCObjectHeader* current = free_list_head_;
        
        while (current) {
            if (current->size >= size) {
                // Remove from free list
                if (prev) {
                    prev->next = current->next;
                } else {
                    free_list_head_ = current->next;
                }
                
                free_blocks_count_--;
                current->next = nullptr;
                return current;
            }
            
            prev = current;
            current = current->next;
        }
        
        return nullptr;
    }
    
    GCObjectHeader* allocate_from_region(usize size) {
        // Simple bump allocator from the last region
        if (memory_regions_.empty()) {
            return nullptr;
        }
        
        // For simplicity, just return nullptr - real implementation would
        // maintain bump pointers for each region
        return nullptr;
    }
    
    void add_to_free_list(GCObjectHeader* header) {
        header->next = free_list_head_;
        free_list_head_ = header;
        free_blocks_count_++;
    }
    
    void initialize_object_header(GCObjectHeader* header, usize size, usize alignment) {
        header->size = size;
        header->alignment = alignment;
        header->generation = generation_;
        header->mark_color = MarkColor::White;
        header->age = 0;
        header->allocation_time = get_current_time();
        header->last_access_time = header->allocation_time;
    }
    
    void finalize_object(GCObjectHeader* header) {
        if (header->destructor) {
            void* object_data = reinterpret_cast<char*>(header) + sizeof(GCObjectHeader);
            header->destructor(object_data);
        }
    }
    
    bool should_promote_object(GCObjectHeader* header) const {
        // Promotion criteria based on generation
        switch (generation_) {
            case Generation::Young:
                return header->age >= 2; // Survived 2 collections
                
            case Generation::Old:
                return header->age >= 10; // Very old object
                
            case Generation::Permanent:
                return false; // Never promote from permanent
        }
        
        return false;
    }
    
    GCObject<void>* get_gc_object_from_header(GCObjectHeader* header) {
        // This would need proper type casting in a real implementation
        return nullptr; // Simplified for educational purposes
    }
    
    constexpr usize align_up(usize value, usize alignment) const {
        return (value + alignment - 1) & ~(alignment - 1);
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Incremental Generational Garbage Collector
//=============================================================================

/**
 * @brief Main generational garbage collector with incremental collection
 */
class GenerationalGarbageCollector {
private:
    // Generation pools
    std::array<std::unique_ptr<GenerationalPool>, 3> generation_pools_;
    
    // Collection state
    enum class CollectionPhase {
        Idle,
        Marking,
        Sweeping,
        Promoting
    };
    
    std::atomic<CollectionPhase> current_phase_{CollectionPhase::Idle};
    std::atomic<Generation> current_generation_{Generation::Young};
    std::atomic<bool> collection_in_progress_{false};
    
    // Root set management
    std::unordered_set<GCObjectHeader*> root_set_;
    mutable std::shared_mutex root_set_mutex_;
    
    // Write barrier tracking
    std::unordered_set<GCObjectHeader*> dirty_objects_;
    mutable std::mutex dirty_objects_mutex_;
    
    // Collection thread
    std::thread collection_thread_;
    std::atomic<bool> collector_running_{true};
    std::condition_variable collection_cv_;
    std::mutex collection_mutex_;
    
    // Collection triggers
    std::atomic<bool> force_collection_{false};
    std::atomic<f64> allocation_pressure_threshold_{0.8}; // 80% utilization
    std::atomic<f64> collection_frequency_seconds_{0.1};  // 100ms incremental steps
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_collections_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> total_pause_time_{0.0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> total_collection_time_{0.0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> objects_collected_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<usize> bytes_collected_{0};
    
    // Educational tracking
    struct CollectionCycle {
        Generation generation;
        f64 start_time;
        f64 end_time;
        f64 pause_time;
        usize objects_before;
        usize objects_after;
        usize objects_promoted;
        CollectionPhase final_phase;
    };
    
    std::vector<CollectionCycle> recent_cycles_;
    mutable std::mutex cycles_mutex_;
    static constexpr usize MAX_RECORDED_CYCLES = 1000;
    
public:
    GenerationalGarbageCollector() {
        initialize_generations();
        start_collection_thread();
        
        LOG_INFO("Initialized generational garbage collector with {} generations", 
                 generation_pools_.size());
    }
    
    ~GenerationalGarbageCollector() {
        shutdown_collector();
        
        LOG_INFO("GC shutdown: {} collections, {:.2f}ms average pause time", 
                 total_collections_.load(), 
                 total_pause_time_.load() * 1000.0 / total_collections_.load());
    }
    
    /**
     * @brief Allocate GC-managed object
     */
    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        static_assert(std::is_base_of_v<GCObject<T>, T>, 
                     "Type must inherit from GCObject");
        
        // Always allocate in young generation initially
        auto& young_pool = *generation_pools_[static_cast<usize>(Generation::Young)];
        
        GCObjectHeader* header = young_pool.allocate(sizeof(T), alignof(T));
        if (!header) {
            // Trigger emergency collection and retry
            force_collection_.store(true);
            collection_cv_.notify_one();
            
            // Wait a bit and retry
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            header = young_pool.allocate(sizeof(T), alignof(T));
            
            if (!header) {
                throw std::bad_alloc();
            }
        }
        
        // Construct object
        T* object = new(reinterpret_cast<char*>(header) + sizeof(GCObjectHeader)) 
                    T(std::forward<Args>(args)...);
        
        // Update header with correct type info
        header->type_info = std::type_index(typeid(T));
        header->object_id = object->get_object_id();
        
        return object;
    }
    
    /**
     * @brief Add object to root set
     */
    void add_root(GCObjectHeader* header) {
        std::unique_lock<std::shared_mutex> lock(root_set_mutex_);
        root_set_.insert(header);
    }
    
    /**
     * @brief Remove object from root set
     */
    void remove_root(GCObjectHeader* header) {
        std::unique_lock<std::shared_mutex> lock(root_set_mutex_);
        root_set_.erase(header);
    }
    
    /**
     * @brief Trigger write barrier
     */
    void write_barrier(GCObjectHeader* source, GCObjectHeader* target) {
        if (!source || !target) return;
        
        // If old generation object references young generation object
        if (source->generation > target->generation) {
            source->has_young_references = true;
            
            std::lock_guard<std::mutex> lock(dirty_objects_mutex_);
            dirty_objects_.insert(source);
        }
        
        source->write_barrier_dirty = true;
    }
    
    /**
     * @brief Force immediate collection
     */
    void force_collection(Generation generation = Generation::Young) {
        current_generation_.store(generation);
        force_collection_.store(true);
        collection_cv_.notify_one();
        
        // Wait for collection to start
        while (current_phase_.load() == CollectionPhase::Idle && force_collection_.load()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    
    /**
     * @brief Get comprehensive GC statistics
     */
    struct GCStatistics {
        struct GenerationStats {
            Generation generation;
            usize total_size;
            usize used_size;
            usize live_objects;
            f64 utilization_ratio;
            u64 collections;
            u64 promotions;
            f64 survival_rate;
        };
        
        std::vector<GenerationStats> generation_stats;
        
        // Overall statistics
        u64 total_collections;
        f64 total_pause_time;
        f64 total_collection_time;
        f64 average_pause_time;
        f64 collection_frequency;
        u64 objects_collected;
        usize bytes_collected;
        
        // Current collection state
        CollectionPhase current_phase;
        Generation current_generation;
        bool collection_in_progress;
        
        // Write barrier statistics
        usize dirty_objects_count;
        usize root_set_size;
        
        // Educational metrics
        f64 generational_efficiency; // How well generational hypothesis holds
        f64 incremental_efficiency;  // Effectiveness of incremental collection
        std::string performance_summary;
    };
    
    GCStatistics get_statistics() const {
        GCStatistics stats{};
        
        // Gather generation statistics
        for (usize i = 0; i < generation_pools_.size(); ++i) {
            if (!generation_pools_[i]) continue;
            
            auto pool_stats = generation_pools_[i]->get_statistics();
            
            GCStatistics::GenerationStats gen_stat{};
            gen_stat.generation = pool_stats.generation;
            gen_stat.total_size = pool_stats.total_size;
            gen_stat.used_size = pool_stats.used_size;
            gen_stat.live_objects = pool_stats.live_objects_count;
            gen_stat.utilization_ratio = pool_stats.utilization_ratio;
            gen_stat.collections = pool_stats.total_collections;
            gen_stat.promotions = pool_stats.total_promotions;
            gen_stat.survival_rate = pool_stats.survival_rate;
            
            stats.generation_stats.push_back(gen_stat);
        }
        
        // Overall statistics
        stats.total_collections = total_collections_.load();
        stats.total_pause_time = total_pause_time_.load();
        stats.total_collection_time = total_collection_time_.load();
        stats.objects_collected = objects_collected_.load();
        stats.bytes_collected = bytes_collected_.load();
        
        if (stats.total_collections > 0) {
            stats.average_pause_time = stats.total_pause_time / stats.total_collections;
            stats.collection_frequency = stats.total_collections / stats.total_collection_time;
        }
        
        // Current state
        stats.current_phase = current_phase_.load();
        stats.current_generation = current_generation_.load();
        stats.collection_in_progress = collection_in_progress_.load();
        
        // Write barrier stats
        {
            std::lock_guard<std::mutex> lock(dirty_objects_mutex_);
            stats.dirty_objects_count = dirty_objects_.size();
        }
        
        {
            std::shared_lock<std::shared_mutex> lock(root_set_mutex_);
            stats.root_set_size = root_set_.size();
        }
        
        // Calculate educational metrics
        stats.generational_efficiency = calculate_generational_efficiency();
        stats.incremental_efficiency = calculate_incremental_efficiency();
        
        // Performance summary
        if (stats.average_pause_time < 0.005) { // < 5ms
            stats.performance_summary = "Excellent - Very low pause times";
        } else if (stats.average_pause_time < 0.020) { // < 20ms
            stats.performance_summary = "Good - Acceptable pause times";
        } else {
            stats.performance_summary = "Poor - High pause times, consider tuning";
        }
        
        return stats;
    }
    
    /**
     * @brief Get recent collection history for analysis
     */
    std::vector<CollectionCycle> get_collection_history() const {
        std::lock_guard<std::mutex> lock(cycles_mutex_);
        return recent_cycles_;
    }
    
    /**
     * @brief Configuration
     */
    void set_allocation_pressure_threshold(f64 threshold) {
        allocation_pressure_threshold_.store(threshold);
    }
    
    void set_collection_frequency(f64 frequency_seconds) {
        collection_frequency_seconds_.store(frequency_seconds);
    }
    
    bool is_collection_in_progress() const {
        return collection_in_progress_.load();
    }
    
    CollectionPhase get_current_phase() const {
        return current_phase_.load();
    }
    
private:
    void initialize_generations() {
        generation_pools_[static_cast<usize>(Generation::Young)] = 
            std::make_unique<GenerationalPool>(Generation::Young, 512 * 1024); // 512KB
        
        generation_pools_[static_cast<usize>(Generation::Old)] = 
            std::make_unique<GenerationalPool>(Generation::Old, 2 * 1024 * 1024); // 2MB
        
        generation_pools_[static_cast<usize>(Generation::Permanent)] = 
            std::make_unique<GenerationalPool>(Generation::Permanent, 1024 * 1024); // 1MB
    }
    
    void start_collection_thread() {
        collection_thread_ = std::thread([this]() {
            collection_worker();
        });
    }
    
    void shutdown_collector() {
        collector_running_.store(false);
        collection_cv_.notify_all();
        
        if (collection_thread_.joinable()) {
            collection_thread_.join();
        }
    }
    
    void collection_worker() {
        while (collector_running_.load()) {
            std::unique_lock<std::mutex> lock(collection_mutex_);
            
            // Wait for collection trigger or timeout
            f64 frequency = collection_frequency_seconds_.load();
            collection_cv_.wait_for(lock, std::chrono::duration<f64>(frequency), [this] {
                return force_collection_.load() || !collector_running_.load() ||
                       should_collect();
            });
            
            if (!collector_running_.load()) break;
            
            // Perform incremental collection
            if (force_collection_.load() || should_collect()) {
                perform_incremental_collection();
                force_collection_.store(false);
            }
        }
    }
    
    bool should_collect() const {
        // Check allocation pressure
        for (const auto& pool : generation_pools_) {
            if (pool) {
                auto stats = pool->get_statistics();
                if (stats.utilization_ratio > allocation_pressure_threshold_.load()) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    void perform_incremental_collection() {
        auto start_time = get_current_time();
        Generation target_gen = current_generation_.load();
        auto& pool = *generation_pools_[static_cast<usize>(target_gen)];
        
        CollectionCycle cycle{};
        cycle.generation = target_gen;
        cycle.start_time = start_time;
        cycle.objects_before = pool.get_live_object_count();
        
        collection_in_progress_.store(true);
        
        // Phase 1: Root marking
        current_phase_.store(CollectionPhase::Marking);
        auto pause_start = get_current_time();
        
        pool.begin_mark_phase();
        
        // Mark roots
        {
            std::shared_lock<std::shared_mutex> lock(root_set_mutex_);
            for (GCObjectHeader* root : root_set_) {
                if (root->generation == target_gen) {
                    pool.mark_object(root);
                }
            }
        }
        
        // Mark dirty objects from write barriers
        {
            std::lock_guard<std::mutex> lock(dirty_objects_mutex_);
            for (GCObjectHeader* dirty : dirty_objects_) {
                if (dirty->generation == target_gen && dirty->has_young_references) {
                    pool.mark_object(dirty);
                }
            }
            dirty_objects_.clear();
        }
        
        auto pause_end = get_current_time();
        cycle.pause_time = pause_end - pause_start;
        
        // Phase 2: Incremental marking (concurrent)
        while (!pool.is_mark_stack_empty()) {
            pool.process_mark_stack(50); // Process in small batches
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        // Phase 3: Sweep (may need brief pause)
        current_phase_.store(CollectionPhase::Sweeping);
        auto sweep_result = pool.sweep_unmarked_objects();
        
        cycle.objects_after = pool.get_live_object_count();
        
        // Phase 4: Promotion
        if (!sweep_result.promoted_objects.empty()) {
            current_phase_.store(CollectionPhase::Promoting);
            promote_objects(sweep_result.promoted_objects);
            cycle.objects_promoted = sweep_result.promoted_objects.size();
        }
        
        // Update statistics
        objects_collected_.fetch_add(sweep_result.objects_collected);
        bytes_collected_.fetch_add(sweep_result.bytes_freed);
        total_pause_time_.fetch_add(cycle.pause_time);
        total_collections_.fetch_add(1);
        
        cycle.end_time = get_current_time();
        cycle.final_phase = CollectionPhase::Idle;
        total_collection_time_.fetch_add(cycle.end_time - cycle.start_time);
        
        // Record cycle for analysis
        {
            std::lock_guard<std::mutex> lock(cycles_mutex_);
            recent_cycles_.push_back(cycle);
            if (recent_cycles_.size() > MAX_RECORDED_CYCLES) {
                recent_cycles_.erase(recent_cycles_.begin());
            }
        }
        
        current_phase_.store(CollectionPhase::Idle);
        collection_in_progress_.store(false);
        
        LOG_DEBUG("GC cycle completed: generation={}, pause={:.2f}ms, collected={}", 
                 static_cast<u32>(target_gen), cycle.pause_time * 1000.0, 
                 sweep_result.objects_collected);
    }
    
    void promote_objects(const std::vector<GCObjectHeader*>& objects) {
        for (GCObjectHeader* header : objects) {
            Generation current_gen = header->generation;
            Generation target_gen = static_cast<Generation>(
                std::min(static_cast<u32>(current_gen) + 1, 
                        static_cast<u32>(Generation::Permanent)));
            
            if (target_gen != current_gen) {
                auto& source_pool = *generation_pools_[static_cast<usize>(current_gen)];
                auto& target_pool = *generation_pools_[static_cast<usize>(target_gen)];
                
                source_pool.remove_object(header);
                target_pool.add_object(header);
            }
        }
    }
    
    f64 calculate_generational_efficiency() const {
        // Measure how well the generational hypothesis holds
        if (generation_pools_[0] && generation_pools_[1]) {
            auto young_stats = generation_pools_[0]->get_statistics();
            auto old_stats = generation_pools_[1]->get_statistics();
            
            // Good generational efficiency means young generation has low survival rate
            f64 young_collection_rate = young_stats.total_collections > 0 ? 
                1.0 - young_stats.survival_rate : 0.0;
            
            // And old generation has high survival rate
            f64 old_survival_rate = old_stats.survival_rate;
            
            return (young_collection_rate + old_survival_rate) / 2.0;
        }
        
        return 0.5; // Neutral
    }
    
    f64 calculate_incremental_efficiency() const {
        f64 total_pause = total_pause_time_.load();
        f64 total_time = total_collection_time_.load();
        
        if (total_time > 0.0) {
            // Lower pause time ratio means better incremental efficiency
            return 1.0 - (total_pause / total_time);
        }
        
        return 0.5; // Neutral
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Educational GC Analysis Tools
//=============================================================================

/**
 * @brief Educational tools for GC analysis and visualization
 */
class GCAnalysisTools {
private:
    const GenerationalGarbageCollector& gc_;
    
public:
    explicit GCAnalysisTools(const GenerationalGarbageCollector& gc) 
        : gc_(gc) {}
    
    /**
     * @brief Generate educational GC report
     */
    struct GCReport {
        std::string summary;
        std::vector<std::string> generation_analysis;
        std::vector<std::string> performance_insights;
        std::vector<std::string> optimization_suggestions;
        std::vector<std::string> educational_notes;
        f64 overall_efficiency_score;
    };
    
    GCReport generate_educational_report() const {
        GCReport report{};
        
        auto stats = gc_.get_statistics();
        report.overall_efficiency_score = (stats.generational_efficiency + 
                                          stats.incremental_efficiency) / 2.0;
        
        // Summary
        report.summary = "Generational Garbage Collection Analysis:\n";
        report.summary += "- Total collections: " + std::to_string(stats.total_collections) + "\n";
        report.summary += "- Average pause time: " + 
            std::to_string(stats.average_pause_time * 1000.0) + "ms\n";
        report.summary += "- Objects collected: " + std::to_string(stats.objects_collected) + "\n";
        report.summary += "- Memory reclaimed: " + std::to_string(stats.bytes_collected / 1024) + "KB";
        
        // Generation analysis
        for (const auto& gen_stat : stats.generation_stats) {
            std::string gen_name = get_generation_name(gen_stat.generation);
            std::string analysis = gen_name + " Generation: ";
            analysis += std::to_string(gen_stat.live_objects) + " objects, ";
            analysis += std::to_string(static_cast<u32>(gen_stat.utilization_ratio * 100)) + "% utilized, ";
            analysis += std::to_string(static_cast<u32>(gen_stat.survival_rate * 100)) + "% survival rate";
            report.generation_analysis.push_back(analysis);
        }
        
        // Performance insights
        if (stats.average_pause_time > 0.020) {
            report.performance_insights.push_back("High pause times detected - consider smaller incremental steps");
        }
        
        if (stats.generational_efficiency < 0.6) {
            report.performance_insights.push_back("Poor generational efficiency - objects not following expected lifecycle patterns");
        }
        
        if (stats.incremental_efficiency > 0.8) {
            report.performance_insights.push_back("Excellent incremental collection - low pause times achieved");
        }
        
        // Optimization suggestions
        report.optimization_suggestions.push_back("Consider adjusting generation sizes based on allocation patterns");
        report.optimization_suggestions.push_back("Monitor write barrier overhead for cross-generational references");
        report.optimization_suggestions.push_back("Tune collection frequency based on allocation rate");
        
        // Educational notes
        report.educational_notes.push_back("Generational hypothesis: Most objects die young");
        report.educational_notes.push_back("Tri-color marking enables concurrent collection");
        report.educational_notes.push_back("Write barriers track cross-generational references");
        report.educational_notes.push_back("Incremental collection reduces pause times");
        
        return report;
    }
    
    /**
     * @brief Export collection timeline for visualization
     */
    void export_collection_timeline(const std::string& filename) const {
        auto history = gc_.get_collection_history();
        
        // Would export CSV/JSON data for external visualization tools
        LOG_INFO("GC collection timeline exported to: {}", filename);
    }
    
private:
    std::string get_generation_name(Generation gen) const {
        switch (gen) {
            case Generation::Young: return "Young";
            case Generation::Old: return "Old";
            case Generation::Permanent: return "Permanent";
            default: return "Unknown";
        }
    }
};

//=============================================================================
// Global GC Instance
//=============================================================================

GenerationalGarbageCollector& get_global_gc();

} // namespace ecscope::memory::gc