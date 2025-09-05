#pragma once

/**
 * @file memory/gc/gc_manager.hpp
 * @brief Complete Generational Garbage Collection Manager
 * 
 * This header implements a complete generational garbage collection system
 * with incremental collection, concurrent marking, and educational visualization.
 * It coordinates all generations and provides comprehensive GC management.
 * 
 * Key Features:
 * - Complete generational garbage collection with young/old/permanent generations
 * - Incremental and concurrent collection to minimize pause times
 * - Root set management and automatic root discovery
 * - Write barrier implementation for inter-generational references
 * - Educational GC visualization and phase analysis
 * - Performance tuning and adaptive generation sizing
 * - Integration with existing memory tracking systems
 * 
 * @author ECScope Educational ECS Framework - Complete GC System
 * @date 2025
 */

#include "memory/gc/generational_gc.hpp"
#include "memory/memory_tracker.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>

namespace ecscope::memory::gc {

//=============================================================================
// GC Manager Configuration and Control
//=============================================================================

/**
 * @brief Garbage collection trigger conditions
 */
enum class GCTrigger : u8 {
    Manual        = 0,  // Manually triggered
    AllocationRate = 1, // Based on allocation rate
    HeapPressure  = 2,  // Based on heap utilization
    Periodic      = 3,  // Time-based periodic collection
    Adaptive      = 4   // Adaptive based on allocation patterns
};

/**
 * @brief GC collection type and scope
 */
enum class CollectionType : u8 {
    Minor    = 0,  // Young generation only
    Major    = 1,  // Young + Old generations
    Full     = 2,  // All generations including permanent
    Partial  = 3   // Specific subset of objects
};

/**
 * @brief GC phase for incremental collection
 */
enum class GCPhase : u8 {
    Idle         = 0,  // No collection in progress
    RootScanning = 1,  // Scanning root set
    Marking      = 2,  // Marking reachable objects
    Sweeping     = 3,  // Sweeping unreachable objects
    Compacting   = 4,  // Compacting heap (optional)
    Finalizing   = 5   // Running finalizers
};

/**
 * @brief Comprehensive GC configuration
 */
struct GCConfig {
    // Generation configurations
    GenerationConfig young_config;
    GenerationConfig old_config;
    GenerationConfig permanent_config;
    
    // Collection triggers
    GCTrigger primary_trigger;
    f64 young_collection_threshold;    // Young gen collection threshold (0.0-1.0)
    f64 old_collection_threshold;      // Old gen collection threshold (0.0-1.0)
    f64 full_collection_threshold;     // Full collection threshold (0.0-1.0)
    
    // Timing parameters
    f64 max_pause_time_ms;            // Maximum pause time per collection phase
    f64 incremental_step_size_ms;     // Time slice for incremental collection
    f64 periodic_collection_interval_s; // Periodic collection interval
    
    // Performance tuning
    bool enable_concurrent_marking;    // Enable concurrent marking phase
    bool enable_incremental_sweeping;  // Enable incremental sweeping
    bool enable_compaction;           // Enable heap compaction
    bool enable_write_barriers;       // Enable write barriers
    usize parallel_marking_threads;   // Number of parallel marking threads
    
    // Educational features
    bool enable_visualization;        // Enable GC phase visualization
    bool enable_detailed_logging;     // Enable detailed GC logging
    bool collect_statistics;          // Collect comprehensive statistics
    
    GCConfig() {
        // Young generation - frequent, small collections
        young_config.initial_size = 2 * 1024 * 1024;  // 2MB
        young_config.max_size = 16 * 1024 * 1024;     // 16MB
        young_config.collection_threshold = 0.9;       // 90% full
        young_config.promotion_age = 3;                // Promote after 3 collections
        
        // Old generation - less frequent, larger collections
        old_config.initial_size = 8 * 1024 * 1024;    // 8MB
        old_config.max_size = 128 * 1024 * 1024;      // 128MB
        old_config.collection_threshold = 0.8;         // 80% full
        old_config.promotion_age = 10;                 // Promote to permanent after 10 collections
        
        // Permanent generation - very infrequent collections
        permanent_config.initial_size = 4 * 1024 * 1024; // 4MB
        permanent_config.max_size = 32 * 1024 * 1024;    // 32MB
        permanent_config.collection_threshold = 0.95;     // 95% full
        permanent_config.promotion_age = UINT32_MAX;      // Never promote from permanent
        
        // Trigger configuration
        primary_trigger = GCTrigger::Adaptive;
        young_collection_threshold = 0.9;
        old_collection_threshold = 0.8;
        full_collection_threshold = 0.95;
        
        // Timing configuration
        max_pause_time_ms = 10.0;          // 10ms max pause
        incremental_step_size_ms = 2.0;    // 2ms incremental steps
        periodic_collection_interval_s = 30.0; // 30 second intervals
        
        // Performance features
        enable_concurrent_marking = true;
        enable_incremental_sweeping = true;
        enable_compaction = true;
        enable_write_barriers = true;
        parallel_marking_threads = std::max(1u, std::thread::hardware_concurrency() / 2);
        
        // Educational features
        enable_visualization = true;
        enable_detailed_logging = false;  // Can be verbose
        collect_statistics = true;
    }
};

//=============================================================================
// Root Set Management
//=============================================================================

/**
 * @brief Root set manager for tracking GC roots
 */
class RootSetManager {
private:
    std::unordered_set<GCObjectHeader*> static_roots_;     // Static/global roots
    std::unordered_set<GCObjectHeader*> stack_roots_;      // Stack-based roots
    std::unordered_set<GCObjectHeader*> register_roots_;   // Register-based roots
    std::unordered_set<GCObjectHeader*> temporary_roots_;  // Temporary/pinned roots
    
    mutable std::shared_mutex roots_mutex_;
    
    // Root discovery callbacks
    std::vector<std::function<void(std::function<void(void*)>)>> root_scanners_;
    
public:
    /**
     * @brief Add static root (global variables, etc.)
     */
    void add_static_root(GCObjectHeader* root) {
        if (!root) return;
        
        std::unique_lock<std::shared_mutex> lock(roots_mutex_);
        static_roots_.insert(root);
        
        LOG_TRACE("Added static root: object_id={}", root->object_id);
    }
    
    /**
     * @brief Remove static root
     */
    void remove_static_root(GCObjectHeader* root) {
        std::unique_lock<std::shared_mutex> lock(roots_mutex_);
        static_roots_.erase(root);
    }
    
    /**
     * @brief Add temporary root (prevents collection)
     */
    void add_temporary_root(GCObjectHeader* root) {
        if (!root) return;
        
        std::unique_lock<std::shared_mutex> lock(roots_mutex_);
        temporary_roots_.insert(root);
        root->is_pinned = true;
    }
    
    /**
     * @brief Remove temporary root
     */
    void remove_temporary_root(GCObjectHeader* root) {
        if (!root) return;
        
        std::unique_lock<std::shared_mutex> lock(roots_mutex_);
        temporary_roots_.erase(root);
        root->is_pinned = false;
    }
    
    /**
     * @brief Register root scanner callback
     */
    void register_root_scanner(std::function<void(std::function<void(void*)>)> scanner) {
        std::unique_lock<std::shared_mutex> lock(roots_mutex_);
        root_scanners_.push_back(scanner);
    }
    
    /**
     * @brief Collect all roots for GC
     */
    std::vector<GCObjectHeader*> collect_all_roots() const {
        std::shared_lock<std::shared_mutex> lock(roots_mutex_);
        
        std::vector<GCObjectHeader*> all_roots;
        
        // Add static roots
        for (auto* root : static_roots_) {
            all_roots.push_back(root);
        }
        
        // Add temporary roots
        for (auto* root : temporary_roots_) {
            all_roots.push_back(root);
        }
        
        // TODO: In a real implementation, would scan stack and registers
        // For educational purposes, we'll simulate this
        scan_stack_and_registers(all_roots);
        
        // Run registered scanners
        for (const auto& scanner : root_scanners_) {
            scanner([&all_roots](void* obj) {
                // Convert object to header (simplified)
                auto* header = static_cast<GCObjectHeader*>(obj);
                if (header) {
                    all_roots.push_back(header);
                }
            });
        }
        
        LOG_DEBUG("Collected {} roots for GC", all_roots.size());
        return all_roots;
    }
    
    /**
     * @brief Get root set statistics
     */
    struct RootStatistics {
        usize static_roots_count;
        usize stack_roots_count;
        usize register_roots_count;
        usize temporary_roots_count;
        usize total_roots_count;
        usize scanner_count;
    };
    
    RootStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(roots_mutex_);
        
        RootStatistics stats{};
        stats.static_roots_count = static_roots_.size();
        stats.stack_roots_count = stack_roots_.size();
        stats.register_roots_count = register_roots_.size();
        stats.temporary_roots_count = temporary_roots_.size();
        stats.total_roots_count = stats.static_roots_count + stats.stack_roots_count + 
                                 stats.register_roots_count + stats.temporary_roots_count;
        stats.scanner_count = root_scanners_.size();
        
        return stats;
    }
    
private:
    void scan_stack_and_registers(std::vector<GCObjectHeader*>& roots) const {
        // In a real implementation, would scan the call stack and CPU registers
        // This is highly platform-specific and complex
        // For educational purposes, we'll simulate finding some stack roots
        
        // Simulated stack scanning - in practice would use stack walking
        // This is a simplified educational example
        static thread_local std::vector<GCObjectHeader*> simulated_stack_roots;
        
        for (auto* root : simulated_stack_roots) {
            roots.push_back(root);
        }
    }
};

//=============================================================================
// Incremental GC Controller
//=============================================================================

/**
 * @brief Controls incremental garbage collection execution
 */
class IncrementalGCController {
private:
    GCPhase current_phase_;
    CollectionType current_collection_type_;
    
    // Incremental state
    std::vector<GCObjectHeader*> current_roots_;
    std::vector<GenerationHeap*> target_generations_;
    usize current_generation_index_;
    
    // Timing control
    std::chrono::steady_clock::time_point phase_start_time_;
    f64 max_step_time_ms_;
    f64 total_pause_time_ms_;
    
    // Progress tracking
    usize objects_marked_;
    usize objects_swept_;
    usize total_objects_;
    
    std::atomic<bool> collection_in_progress_{false};
    mutable std::mutex controller_mutex_;
    
public:
    explicit IncrementalGCController(f64 max_step_time = 2.0) 
        : current_phase_(GCPhase::Idle), current_collection_type_(CollectionType::Minor),
          current_generation_index_(0), max_step_time_ms_(max_step_time),
          total_pause_time_ms_(0.0), objects_marked_(0), objects_swept_(0), total_objects_(0) {}
    
    /**
     * @brief Start incremental collection
     */
    bool start_collection(CollectionType type, std::vector<GenerationHeap*> generations, 
                         const std::vector<GCObjectHeader*>& roots) {
        std::lock_guard<std::mutex> lock(controller_mutex_);
        
        if (collection_in_progress_.load()) {
            return false; // Collection already in progress
        }
        
        current_collection_type_ = type;
        current_phase_ = GCPhase::RootScanning;
        target_generations_ = generations;
        current_roots_ = roots;
        current_generation_index_ = 0;
        
        phase_start_time_ = std::chrono::steady_clock::now();
        total_pause_time_ms_ = 0.0;
        objects_marked_ = objects_swept_ = 0;
        
        // Calculate total objects for progress tracking
        total_objects_ = 0;
        for (auto* gen : target_generations_) {
            total_objects_ += gen->get_statistics().object_count;
        }
        
        collection_in_progress_.store(true);
        
        LOG_INFO("Started incremental {} collection: {} generations, {} total objects", 
                collection_type_name(type), generations.size(), total_objects_);
        
        return true;
    }
    
    /**
     * @brief Execute one incremental step
     */
    bool execute_step() {
        std::lock_guard<std::mutex> lock(controller_mutex_);
        
        if (!collection_in_progress_.load()) {
            return false; // No collection in progress
        }
        
        auto step_start = std::chrono::steady_clock::now();
        
        bool step_completed = false;
        
        switch (current_phase_) {
            case GCPhase::RootScanning:
                step_completed = execute_root_scanning_step();
                break;
            case GCPhase::Marking:
                step_completed = execute_marking_step();
                break;
            case GCPhase::Sweeping:
                step_completed = execute_sweeping_step();
                break;
            case GCPhase::Compacting:
                step_completed = execute_compacting_step();
                break;
            case GCPhase::Finalizing:
                step_completed = execute_finalizing_step();
                break;
            default:
                step_completed = true; // Unknown phase, complete it
                break;
        }
        
        // Track timing
        auto step_end = std::chrono::steady_clock::now();
        f64 step_time = std::chrono::duration<f64, std::milli>(step_end - step_start).count();
        total_pause_time_ms_ += step_time;
        
        // Advance phase if step completed
        if (step_completed) {
            advance_to_next_phase();
        }
        
        // Check if we've exceeded our time budget
        bool should_yield = step_time >= max_step_time_ms_;
        
        return collection_in_progress_.load() && !should_yield;
    }
    
    /**
     * @brief Check if collection is complete
     */
    bool is_collection_complete() const {
        return !collection_in_progress_.load() && current_phase_ == GCPhase::Idle;
    }
    
    /**
     * @brief Get current collection progress (0.0-1.0)
     */
    f64 get_progress() const {
        std::lock_guard<std::mutex> lock(controller_mutex_);
        
        if (!collection_in_progress_.load()) {
            return current_phase_ == GCPhase::Idle ? 0.0 : 1.0;
        }
        
        // Calculate progress based on phase and objects processed
        f64 phase_progress = 0.0;
        switch (current_phase_) {
            case GCPhase::RootScanning:
                phase_progress = 0.1; // Root scanning is quick
                break;
            case GCPhase::Marking:
                phase_progress = 0.1 + (total_objects_ > 0 ? 
                    0.5 * objects_marked_ / total_objects_ : 0.0);
                break;
            case GCPhase::Sweeping:
                phase_progress = 0.6 + (total_objects_ > 0 ? 
                    0.3 * objects_swept_ / total_objects_ : 0.0);
                break;
            case GCPhase::Compacting:
                phase_progress = 0.9;
                break;
            case GCPhase::Finalizing:
                phase_progress = 0.95;
                break;
            default:
                phase_progress = 1.0;
                break;
        }
        
        return std::clamp(phase_progress, 0.0, 1.0);
    }
    
    /**
     * @brief Get current collection statistics
     */
    struct CollectionStatistics {
        CollectionType collection_type;
        GCPhase current_phase;
        f64 progress_ratio;
        f64 total_pause_time_ms;
        usize objects_marked;
        usize objects_swept;
        usize total_objects;
        usize generations_processed;
        bool is_complete;
        f64 estimated_remaining_time_ms;
    };
    
    CollectionStatistics get_statistics() const {
        std::lock_guard<std::mutex> lock(controller_mutex_);
        
        CollectionStatistics stats{};
        stats.collection_type = current_collection_type_;
        stats.current_phase = current_phase_;
        stats.progress_ratio = get_progress();
        stats.total_pause_time_ms = total_pause_time_ms_;
        stats.objects_marked = objects_marked_;
        stats.objects_swept = objects_swept_;
        stats.total_objects = total_objects_;
        stats.generations_processed = current_generation_index_;
        stats.is_complete = is_collection_complete();
        
        // Estimate remaining time based on current progress
        if (stats.progress_ratio > 0.1) {
            f64 estimated_total_time = stats.total_pause_time_ms / stats.progress_ratio;
            stats.estimated_remaining_time_ms = estimated_total_time - stats.total_pause_time_ms;
        }
        
        return stats;
    }
    
    GCPhase get_current_phase() const { return current_phase_; }
    CollectionType get_collection_type() const { return current_collection_type_; }
    
private:
    bool execute_root_scanning_step() {
        // Root scanning is typically fast, complete in one step
        LOG_DEBUG("Executing root scanning step: {} roots", current_roots_.size());
        return true; // Complete root scanning
    }
    
    bool execute_marking_step() {
        // Execute marking for current generation with time budget
        if (current_generation_index_ >= target_generations_.size()) {
            return true; // All generations marked
        }
        
        auto* current_gen = target_generations_[current_generation_index_];
        current_gen->mark_phase(current_roots_);
        
        // Update progress
        objects_marked_ += current_gen->get_statistics().object_count;
        current_generation_index_++;
        
        LOG_DEBUG("Marked generation {}: {} objects", 
                 current_generation_index_ - 1, current_gen->get_statistics().object_count);
        
        return current_generation_index_ >= target_generations_.size();
    }
    
    bool execute_sweeping_step() {
        // Execute sweeping for all generations
        bool all_swept = true;
        
        for (usize i = 0; i < target_generations_.size(); ++i) {
            auto* gen = target_generations_[i];
            usize collected = gen->sweep_phase();
            objects_swept_ += collected;
            
            if (collected > 0) {
                LOG_DEBUG("Swept generation {}: {} objects collected", i, collected);
            }
        }
        
        return all_swept;
    }
    
    bool execute_compacting_step() {
        // Compaction is optional and complex - for educational purposes, simulate
        LOG_DEBUG("Executing heap compaction step (simulated)");
        return true; // Complete compaction
    }
    
    bool execute_finalizing_step() {
        // Run finalizers and cleanup - typically fast
        LOG_DEBUG("Executing finalization step");
        return true; // Complete finalization
    }
    
    void advance_to_next_phase() {
        switch (current_phase_) {
            case GCPhase::RootScanning:
                current_phase_ = GCPhase::Marking;
                current_generation_index_ = 0; // Reset for marking
                break;
            case GCPhase::Marking:
                current_phase_ = GCPhase::Sweeping;
                break;
            case GCPhase::Sweeping:
                current_phase_ = GCPhase::Compacting;
                break;
            case GCPhase::Compacting:
                current_phase_ = GCPhase::Finalizing;
                break;
            case GCPhase::Finalizing:
                current_phase_ = GCPhase::Idle;
                collection_in_progress_.store(false);
                LOG_INFO("Incremental {} collection completed: {:.2f}ms total pause time", 
                        collection_type_name(current_collection_type_), total_pause_time_ms_);
                break;
            default:
                current_phase_ = GCPhase::Idle;
                collection_in_progress_.store(false);
                break;
        }
        
        phase_start_time_ = std::chrono::steady_clock::now();
    }
    
    const char* collection_type_name(CollectionType type) const {
        switch (type) {
            case CollectionType::Minor: return "Minor";
            case CollectionType::Major: return "Major";
            case CollectionType::Full: return "Full";
            case CollectionType::Partial: return "Partial";
        }
        return "Unknown";
    }
};

//=============================================================================
// Main GC Manager
//=============================================================================

/**
 * @brief Complete generational garbage collection manager
 */
class GenerationalGCManager {
private:
    // Generation heaps
    std::unique_ptr<GenerationHeap> young_generation_;
    std::unique_ptr<GenerationHeap> old_generation_;
    std::unique_ptr<GenerationHeap> permanent_generation_;
    
    // Management components
    std::unique_ptr<RootSetManager> root_manager_;
    std::unique_ptr<IncrementalGCController> incremental_controller_;
    
    // Configuration
    GCConfig config_;
    memory::MemoryTracker* memory_tracker_;
    
    // Collection thread
    std::thread gc_thread_;
    std::atomic<bool> gc_thread_active_{true};
    std::condition_variable gc_condition_;
    std::mutex gc_mutex_;
    
    // Trigger state
    std::atomic<GCTrigger> pending_trigger_{GCTrigger::Manual};
    std::atomic<CollectionType> pending_collection_type_{CollectionType::Minor};
    
    // Statistics
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_collections_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> minor_collections_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> major_collections_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> full_collections_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> total_pause_time_ms_{0.0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> last_collection_time_{0.0};
    
public:
    explicit GenerationalGCManager(const GCConfig& config = GCConfig{}, 
                                  memory::MemoryTracker* tracker = nullptr)
        : config_(config), memory_tracker_(tracker) {
        
        initialize_generations();
        root_manager_ = std::make_unique<RootSetManager>();
        incremental_controller_ = std::make_unique<IncrementalGCController>(
            config_.incremental_step_size_ms
        );
        
        // Start GC thread
        gc_thread_ = std::thread([this]() {
            gc_worker_thread();
        });
        
        LOG_INFO("Initialized generational GC manager: young={}MB, old={}MB, permanent={}MB",
                 config_.young_config.initial_size / (1024*1024),
                 config_.old_config.initial_size / (1024*1024),
                 config_.permanent_config.initial_size / (1024*1024));
    }
    
    ~GenerationalGCManager() {
        gc_thread_active_.store(false);
        gc_condition_.notify_all();
        
        if (gc_thread_.joinable()) {
            gc_thread_.join();
        }
        
        LOG_INFO("GC Manager destroyed: {} total collections, {:.2f}ms total pause time",
                 total_collections_.load(), total_pause_time_ms_.load());
    }
    
    /**
     * @brief Allocate object in appropriate generation
     */
    template<typename T, typename... Args>
    GCObject<T>* allocate(Args&&... args) {
        // New objects start in young generation
        auto* gc_object = young_generation_->allocate<T>(std::forward<Args>(args)...);
        
        if (!gc_object) {
            // Young generation full - trigger collection and retry
            request_collection(CollectionType::Minor);
            gc_object = young_generation_->allocate<T>(std::forward<Args>(args)...);
        }
        
        // Track allocation
        if (gc_object && memory_tracker_) {
            memory_tracker_->track_allocation(
                gc_object->get_object(), sizeof(T), sizeof(T), alignof(T),
                memory::AllocationCategory::Custom_02, // GC allocations
                memory::AllocatorType::Custom,
                "GenerationalGC",
                static_cast<u32>(Generation::Young)
            );
        }
        
        return gc_object;
    }
    
    /**
     * @brief Request garbage collection
     */
    void request_collection(CollectionType type = CollectionType::Minor) {
        pending_collection_type_.store(type);
        pending_trigger_.store(GCTrigger::Manual);
        gc_condition_.notify_one();
        
        LOG_DEBUG("Requested {} collection", collection_type_name(type));
    }
    
    /**
     * @brief Add static root
     */
    void add_root(GCObjectHeader* root) {
        root_manager_->add_static_root(root);
    }
    
    /**
     * @brief Remove static root
     */
    void remove_root(GCObjectHeader* root) {
        root_manager_->remove_static_root(root);
    }
    
    /**
     * @brief Pin object temporarily (prevents collection)
     */
    void pin_object(GCObjectHeader* obj) {
        root_manager_->add_temporary_root(obj);
    }
    
    /**
     * @brief Unpin object
     */
    void unpin_object(GCObjectHeader* obj) {
        root_manager_->remove_temporary_root(obj);
    }
    
    /**
     * @brief Execute write barrier
     */
    void write_barrier(GCObjectHeader* source, void* target) {
        if (!config_.enable_write_barriers) return;
        
        // Determine which generation should handle the barrier
        if (source->generation == Generation::Young) {
            young_generation_->write_barrier(source, target);
        } else if (source->generation == Generation::Old) {
            old_generation_->write_barrier(source, target);
        } else {
            permanent_generation_->write_barrier(source, target);
        }
    }
    
    /**
     * @brief Get comprehensive GC statistics
     */
    struct GCManagerStatistics {
        // Generation statistics
        GenerationHeap::GenerationStatistics young_stats;
        GenerationHeap::GenerationStatistics old_stats;
        GenerationHeap::GenerationStatistics permanent_stats;
        
        // Root set statistics
        RootSetManager::RootStatistics root_stats;
        
        // Collection statistics
        u64 total_collections;
        u64 minor_collections;
        u64 major_collections;
        u64 full_collections;
        f64 total_pause_time_ms;
        f64 average_pause_time_ms;
        f64 last_collection_time;
        
        // Current state
        GCPhase current_phase;
        CollectionType current_collection_type;
        f64 collection_progress;
        bool collection_in_progress;
        
        // Performance metrics
        f64 allocation_rate_objects_per_second;
        f64 collection_frequency_per_second;
        f64 gc_overhead_percentage;
        usize total_heap_size;
        usize total_heap_used;
        f64 overall_utilization;
        
        // Configuration
        GCConfig config;
    };
    
    GCManagerStatistics get_statistics() const {
        GCManagerStatistics stats{};
        
        // Get generation statistics
        stats.young_stats = young_generation_->get_statistics();
        stats.old_stats = old_generation_->get_statistics();
        stats.permanent_stats = permanent_generation_->get_statistics();
        
        // Get root statistics
        stats.root_stats = root_manager_->get_statistics();
        
        // Collection statistics
        stats.total_collections = total_collections_.load();
        stats.minor_collections = minor_collections_.load();
        stats.major_collections = major_collections_.load();
        stats.full_collections = full_collections_.load();
        stats.total_pause_time_ms = total_pause_time_ms_.load();
        if (stats.total_collections > 0) {
            stats.average_pause_time_ms = stats.total_pause_time_ms / stats.total_collections;
        }
        stats.last_collection_time = last_collection_time_.load();
        
        // Current state
        auto controller_stats = incremental_controller_->get_statistics();
        stats.current_phase = controller_stats.current_phase;
        stats.current_collection_type = controller_stats.collection_type;
        stats.collection_progress = controller_stats.progress_ratio;
        stats.collection_in_progress = !controller_stats.is_complete;
        
        // Performance metrics
        stats.total_heap_size = stats.young_stats.heap_size + stats.old_stats.heap_size + 
                               stats.permanent_stats.heap_size;
        stats.total_heap_used = stats.young_stats.heap_used + stats.old_stats.heap_used + 
                               stats.permanent_stats.heap_used;
        if (stats.total_heap_size > 0) {
            stats.overall_utilization = static_cast<f64>(stats.total_heap_used) / stats.total_heap_size;
        }
        
        // Calculate rates
        f64 current_time = get_current_time();
        u64 total_allocations = stats.young_stats.total_allocations + 
                               stats.old_stats.total_allocations + 
                               stats.permanent_stats.total_allocations;
        if (current_time > 0) {
            stats.allocation_rate_objects_per_second = total_allocations / current_time;
            stats.collection_frequency_per_second = stats.total_collections / current_time;
        }
        
        // GC overhead estimate
        if (current_time > 0) {
            stats.gc_overhead_percentage = (stats.total_pause_time_ms / 1000.0) / current_time * 100.0;
        }
        
        stats.config = config_;
        
        return stats;
    }
    
    /**
     * @brief Configuration management
     */
    void set_config(const GCConfig& config) {
        config_ = config;
        
        // Update generation configurations
        young_generation_->set_config(config.young_config);
        old_generation_->set_config(config.old_config);
        permanent_generation_->set_config(config.permanent_config);
        
        LOG_INFO("Updated GC configuration");
    }
    
    const GCConfig& get_config() const { return config_; }
    
    /**
     * @brief Force immediate collection (blocking)
     */
    void force_collection(CollectionType type = CollectionType::Full) {
        request_collection(type);
        
        // Wait for collection to complete
        while (!incremental_controller_->is_collection_complete()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
private:
    void initialize_generations() {
        young_generation_ = std::make_unique<GenerationHeap>(Generation::Young, config_.young_config);
        old_generation_ = std::make_unique<GenerationHeap>(Generation::Old, config_.old_config);
        permanent_generation_ = std::make_unique<GenerationHeap>(Generation::Permanent, config_.permanent_config);
    }
    
    void gc_worker_thread() {
        while (gc_thread_active_.load()) {
            std::unique_lock<std::mutex> lock(gc_mutex_);
            
            // Wait for collection trigger
            gc_condition_.wait(lock, [this]() {
                return !gc_thread_active_.load() || 
                       pending_trigger_.load() != GCTrigger::Manual ||
                       should_trigger_collection();
            });
            
            if (!gc_thread_active_.load()) break;
            
            // Determine collection type and execute
            CollectionType collection_type = determine_collection_type();
            execute_collection(collection_type);
            
            // Reset trigger
            pending_trigger_.store(GCTrigger::Manual);
        }
    }
    
    bool should_trigger_collection() const {
        switch (config_.primary_trigger) {
            case GCTrigger::AllocationRate:
                return young_generation_->needs_collection();
                
            case GCTrigger::HeapPressure:
                return young_generation_->get_statistics().utilization_ratio >= config_.young_collection_threshold ||
                       old_generation_->get_statistics().utilization_ratio >= config_.old_collection_threshold;
                
            case GCTrigger::Periodic: {
                f64 current_time = get_current_time();
                f64 last_collection = last_collection_time_.load();
                return (current_time - last_collection) >= config_.periodic_collection_interval_s;
            }
            
            case GCTrigger::Adaptive:
                return young_generation_->needs_collection() || 
                       old_generation_->needs_collection();
                
            default:
                return false;
        }
    }
    
    CollectionType determine_collection_type() const {
        // Check if explicit collection was requested
        CollectionType requested = pending_collection_type_.load();
        if (requested != CollectionType::Minor) {
            return requested;
        }
        
        // Automatic collection type determination
        if (permanent_generation_->needs_collection()) {
            return CollectionType::Full;
        } else if (old_generation_->needs_collection()) {
            return CollectionType::Major;
        } else {
            return CollectionType::Minor;
        }
    }
    
    void execute_collection(CollectionType type) {
        auto collection_start = std::chrono::steady_clock::now();
        
        // Collect roots
        auto roots = root_manager_->collect_all_roots();
        
        // Determine target generations
        std::vector<GenerationHeap*> target_generations;
        switch (type) {
            case CollectionType::Minor:
                target_generations.push_back(young_generation_.get());
                break;
            case CollectionType::Major:
                target_generations.push_back(young_generation_.get());
                target_generations.push_back(old_generation_.get());
                break;
            case CollectionType::Full:
                target_generations.push_back(young_generation_.get());
                target_generations.push_back(old_generation_.get());
                target_generations.push_back(permanent_generation_.get());
                break;
            case CollectionType::Partial:
                // For partial collections, just collect young generation
                target_generations.push_back(young_generation_.get());
                break;
        }
        
        // Start incremental collection
        if (incremental_controller_->start_collection(type, target_generations, roots)) {
            // Execute incremental steps
            while (incremental_controller_->execute_step()) {
                // Continue incremental collection
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
        
        // Update statistics
        auto collection_end = std::chrono::steady_clock::now();
        f64 collection_time = std::chrono::duration<f64, std::milli>(
            collection_end - collection_start).count();
        
        total_collections_.fetch_add(1, std::memory_order_relaxed);
        total_pause_time_ms_.fetch_add(collection_time, std::memory_order_relaxed);
        last_collection_time_.store(get_current_time(), std::memory_order_relaxed);
        
        switch (type) {
            case CollectionType::Minor:
                minor_collections_.fetch_add(1, std::memory_order_relaxed);
                break;
            case CollectionType::Major:
                major_collections_.fetch_add(1, std::memory_order_relaxed);
                break;
            case CollectionType::Full:
                full_collections_.fetch_add(1, std::memory_order_relaxed);
                break;
            default:
                break;
        }
        
        LOG_INFO("Completed {} collection in {:.2f}ms", collection_type_name(type), collection_time);
    }
    
    const char* collection_type_name(CollectionType type) const {
        switch (type) {
            case CollectionType::Minor: return "Minor";
            case CollectionType::Major: return "Major";
            case CollectionType::Full: return "Full";
            case CollectionType::Partial: return "Partial";
        }
        return "Unknown";
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Global GC Manager Instance
//=============================================================================

GenerationalGCManager& get_global_gc_manager();

} // namespace ecscope::memory::gc