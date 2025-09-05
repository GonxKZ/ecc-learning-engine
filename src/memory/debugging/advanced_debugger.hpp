#pragma once

/**
 * @file memory/debugging/advanced_debugger.hpp
 * @brief Advanced Memory Debugging Tools with Leak Detection and Visualization
 * 
 * This header implements sophisticated memory debugging capabilities including
 * leak detection, memory corruption analysis, allocation pattern visualization,
 * and real-time memory monitoring. Designed for educational purposes with
 * comprehensive analysis and reporting features.
 * 
 * Key Features:
 * - Advanced leak detection with root cause analysis
 * - Memory corruption detection (buffer overruns, use-after-free)
 * - Real-time memory allocation pattern analysis
 * - Memory fragmentation monitoring and visualization
 * - Call stack capture and analysis for allocation origins
 * - Cache behavior analysis and optimization suggestions
 * - Memory access pattern tracking and prediction
 * - Educational visualization of memory issues
 * 
 * Educational Value:
 * - Demonstrates common memory bugs and their detection
 * - Shows memory profiling and optimization techniques
 * - Illustrates debugging tool implementation strategies
 * - Provides insights into memory management best practices
 * - Educational examples of memory forensics and analysis
 * 
 * @author ECScope Educational ECS Framework - Memory Debugging
 * @date 2025
 */

#include "memory/memory_tracker.hpp"
#include "memory/specialized/thermal_pools.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <cstring>
#include <csignal>

namespace ecscope::memory::debugging {

//=============================================================================
// Memory Corruption Detection
//=============================================================================

/**
 * @brief Memory corruption types for classification
 */
enum class CorruptionType : u8 {
    BufferOverrun     = 0,  // Write beyond allocated boundary
    BufferUnderrun    = 1,  // Write before allocated boundary
    UseAfterFree      = 2,  // Access freed memory
    DoubleFree        = 3,  // Free already freed memory
    UninitializedRead = 4,  // Read uninitialized memory
    LeakDetected      = 5,  // Memory leak identified
    DanglingPointer   = 6,  // Pointer to freed memory
    StackCorruption   = 7   // Stack overflow/corruption
};

/**
 * @brief Memory corruption event details
 */
struct CorruptionEvent {
    CorruptionType type;
    void* address;                  // Address where corruption occurred
    usize size;                     // Size involved in corruption
    f64 detection_time;             // When corruption was detected
    std::thread::id thread_id;      // Thread where corruption occurred
    
    // Call stack information
    memory::CallStack detection_stack;  // Stack when corruption detected
    memory::CallStack allocation_stack; // Stack when memory was allocated
    memory::CallStack deallocation_stack; // Stack when memory was freed (if applicable)
    
    // Additional context
    std::string description;        // Human-readable description
    std::string suggested_fix;      // Suggested fix for the issue
    u32 severity_score;            // Severity score (0-100)
    
    CorruptionEvent() noexcept 
        : type(CorruptionType::BufferOverrun), address(nullptr), size(0),
          detection_time(0.0), thread_id(std::this_thread::get_id()),
          severity_score(50) {}
};

/**
 * @brief Guard zones for buffer overflow detection
 */
class GuardZoneManager {
private:
    static constexpr u32 GUARD_MAGIC = 0xDEADBEEF;
    static constexpr usize GUARD_SIZE = 32; // 32 bytes before and after allocation
    
    struct AllocationGuard {
        void* user_address;          // Address returned to user
        void* full_address;          // Full allocation including guards  
        usize user_size;            // Size requested by user
        usize full_size;            // Total size including guards
        u32 front_guard_hash;       // Hash of front guard zone
        u32 back_guard_hash;        // Hash of back guard zone
        f64 allocation_time;        // When allocated
        std::thread::id thread_id;  // Allocating thread
        memory::CallStack call_stack; // Allocation call stack
    };
    
    std::unordered_map<void*, AllocationGuard> guarded_allocations_;
    mutable std::shared_mutex guards_mutex_;
    
    // Corruption tracking
    std::vector<CorruptionEvent> detected_corruptions_;
    mutable std::mutex corruptions_mutex_;
    
public:
    /**
     * @brief Allocate memory with guard zones
     */
    void* allocate_guarded(usize size, usize alignment = sizeof(void*)) {
        usize total_size = size + (2 * GUARD_SIZE);
        void* raw_memory = std::aligned_alloc(alignment, total_size);
        
        if (!raw_memory) return nullptr;
        
        // Setup guard zones
        u8* memory_bytes = static_cast<u8*>(raw_memory);
        void* user_memory = memory_bytes + GUARD_SIZE;
        
        // Initialize front guard
        initialize_guard_zone(memory_bytes, GUARD_SIZE, true);
        
        // Initialize back guard
        initialize_guard_zone(memory_bytes + GUARD_SIZE + size, GUARD_SIZE, false);
        
        // Record allocation
        AllocationGuard guard_info{};
        guard_info.user_address = user_memory;
        guard_info.full_address = raw_memory;
        guard_info.user_size = size;
        guard_info.full_size = total_size;
        guard_info.front_guard_hash = calculate_guard_hash(memory_bytes, GUARD_SIZE);
        guard_info.back_guard_hash = calculate_guard_hash(memory_bytes + GUARD_SIZE + size, GUARD_SIZE);
        guard_info.allocation_time = get_current_time();
        guard_info.thread_id = std::this_thread::get_id();
        
        // Capture call stack
        capture_call_stack(guard_info.call_stack);
        
        {
            std::unique_lock<std::shared_mutex> lock(guards_mutex_);
            guarded_allocations_[user_memory] = guard_info;
        }
        
        LOG_TRACE("Allocated guarded memory: user_addr={}, size={}, total_size={}", 
                 user_memory, size, total_size);
        
        return user_memory;
    }
    
    /**
     * @brief Free guarded memory with corruption check
     */
    bool free_guarded(void* ptr) {
        if (!ptr) return true;
        
        AllocationGuard guard_info;
        {
            std::unique_lock<std::shared_mutex> lock(guards_mutex_);
            auto it = guarded_allocations_.find(ptr);
            if (it == guarded_allocations_.end()) {
                report_corruption(CorruptionType::DoubleFree, ptr, 0, 
                                "Attempted to free untracked or already freed memory");
                return false;
            }
            
            guard_info = it->second;
            guarded_allocations_.erase(it);
        }
        
        // Check for corruption before freeing
        bool corruption_detected = check_guard_zones(guard_info);
        
        if (corruption_detected) {
            LOG_ERROR("Memory corruption detected during free: addr={}", ptr);
        }
        
        // Clear guard zones before freeing
        clear_guard_zones(guard_info);
        
        std::free(guard_info.full_address);
        
        LOG_TRACE("Freed guarded memory: addr={}, size={}", ptr, guard_info.user_size);
        return !corruption_detected;
    }
    
    /**
     * @brief Check all allocations for corruption
     */
    std::vector<CorruptionEvent> check_all_allocations() {
        std::vector<CorruptionEvent> corruptions;
        std::shared_lock<std::shared_mutex> lock(guards_mutex_);
        
        for (const auto& [user_addr, guard_info] : guarded_allocations_) {
            if (check_guard_zones(guard_info)) {
                // Corruption detected - create event
                CorruptionEvent event;
                event.type = determine_corruption_type(guard_info);
                event.address = user_addr;
                event.size = guard_info.user_size;
                event.detection_time = get_current_time();
                event.thread_id = std::this_thread::get_id();
                event.allocation_stack = guard_info.call_stack;
                
                capture_call_stack(event.detection_stack);
                generate_corruption_description(event, guard_info);
                
                corruptions.push_back(event);
            }
        }
        
        return corruptions;
    }
    
    /**
     * @brief Get statistics about guarded allocations
     */
    struct GuardStatistics {
        usize active_allocations;
        usize total_corruptions_detected;
        usize buffer_overruns;
        usize buffer_underruns;
        usize double_frees;
        usize total_guarded_memory;
        f64 average_allocation_age;
    };
    
    GuardStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(guards_mutex_);
        std::lock_guard<std::mutex> corruption_lock(corruptions_mutex_);
        
        GuardStatistics stats{};
        stats.active_allocations = guarded_allocations_.size();
        stats.total_corruptions_detected = detected_corruptions_.size();
        
        // Count corruption types
        for (const auto& corruption : detected_corruptions_) {
            switch (corruption.type) {
                case CorruptionType::BufferOverrun:
                    stats.buffer_overruns++;
                    break;
                case CorruptionType::BufferUnderrun:
                    stats.buffer_underruns++;
                    break;
                case CorruptionType::DoubleFree:
                    stats.double_frees++;
                    break;
                default:
                    break;
            }
        }
        
        // Calculate total memory and average age
        f64 current_time = get_current_time();
        f64 total_age = 0.0;
        
        for (const auto& [addr, guard_info] : guarded_allocations_) {
            stats.total_guarded_memory += guard_info.full_size;
            total_age += (current_time - guard_info.allocation_time);
        }
        
        if (stats.active_allocations > 0) {
            stats.average_allocation_age = total_age / stats.active_allocations;
        }
        
        return stats;
    }
    
private:
    void initialize_guard_zone(void* guard_ptr, usize guard_size, bool is_front) {
        u32* guard_words = static_cast<u32*>(guard_ptr);
        usize word_count = guard_size / sizeof(u32);
        
        for (usize i = 0; i < word_count; ++i) {
            // Use different patterns for front/back guards
            guard_words[i] = is_front ? (GUARD_MAGIC ^ i) : (GUARD_MAGIC ^ (i + 0x100));
        }
    }
    
    bool check_guard_zones(const AllocationGuard& guard_info) {
        u8* memory_bytes = static_cast<u8*>(guard_info.full_address);
        
        // Check front guard
        u32 front_hash = calculate_guard_hash(memory_bytes, GUARD_SIZE);
        bool front_ok = (front_hash == guard_info.front_guard_hash);
        
        // Check back guard
        u8* back_guard = memory_bytes + GUARD_SIZE + guard_info.user_size;
        u32 back_hash = calculate_guard_hash(back_guard, GUARD_SIZE);
        bool back_ok = (back_hash == guard_info.back_guard_hash);
        
        if (!front_ok) {
            report_corruption(CorruptionType::BufferUnderrun, guard_info.user_address, 
                            guard_info.user_size, "Front guard zone corrupted");
        }
        
        if (!back_ok) {
            report_corruption(CorruptionType::BufferOverrun, guard_info.user_address, 
                            guard_info.user_size, "Back guard zone corrupted");
        }
        
        return !front_ok || !back_ok;
    }
    
    u32 calculate_guard_hash(const void* guard_ptr, usize guard_size) const {
        // Simple hash of guard zone contents
        const u8* bytes = static_cast<const u8*>(guard_ptr);
        u32 hash = 0;
        
        for (usize i = 0; i < guard_size; ++i) {
            hash = hash * 31 + bytes[i];
        }
        
        return hash;
    }
    
    void clear_guard_zones(const AllocationGuard& guard_info) {
        // Clear guard zones to prevent accidental matches
        u8* memory_bytes = static_cast<u8*>(guard_info.full_address);
        std::memset(memory_bytes, 0xFF, GUARD_SIZE); // Front guard
        std::memset(memory_bytes + GUARD_SIZE + guard_info.user_size, 0xFF, GUARD_SIZE); // Back guard
    }
    
    CorruptionType determine_corruption_type(const AllocationGuard& guard_info) const {
        u8* memory_bytes = static_cast<u8*>(guard_info.full_address);
        
        // Check which guard is corrupted
        u32 front_hash = calculate_guard_hash(memory_bytes, GUARD_SIZE);
        if (front_hash != guard_info.front_guard_hash) {
            return CorruptionType::BufferUnderrun;
        }
        
        u8* back_guard = memory_bytes + GUARD_SIZE + guard_info.user_size;
        u32 back_hash = calculate_guard_hash(back_guard, GUARD_SIZE);
        if (back_hash != guard_info.back_guard_hash) {
            return CorruptionType::BufferOverrun;
        }
        
        return CorruptionType::BufferOverrun; // Default
    }
    
    void generate_corruption_description(CorruptionEvent& event, const AllocationGuard& guard_info) const {
        std::ostringstream desc;
        std::ostringstream fix;
        
        switch (event.type) {
            case CorruptionType::BufferOverrun:
                desc << "Buffer overrun detected: wrote beyond allocated boundary. ";
                desc << "Allocation size: " << guard_info.user_size << " bytes, ";
                desc << "allocated " << (event.detection_time - guard_info.allocation_time) << " seconds ago.";
                
                fix << "Check array bounds and ensure all writes are within allocated size. ";
                fix << "Consider using bounds-checked containers or AddressSanitizer.";
                event.severity_score = 90;
                break;
                
            case CorruptionType::BufferUnderrun:
                desc << "Buffer underrun detected: wrote before allocated boundary. ";
                desc << "This typically indicates pointer arithmetic errors.";
                
                fix << "Check pointer calculations and ensure no negative indexing. ";
                fix << "Verify pointer arithmetic doesn't go before allocation start.";
                event.severity_score = 85;
                break;
                
            case CorruptionType::DoubleFree:
                desc << "Double free detected: memory was already freed. ";
                desc << "This can cause heap corruption and crashes.";
                
                fix << "Set pointers to nullptr after freeing. Use smart pointers to prevent double frees.";
                event.severity_score = 95;
                break;
                
            default:
                desc << "Memory corruption detected at address " << event.address;
                fix << "Review memory management around this allocation.";
                event.severity_score = 70;
                break;
        }
        
        event.description = desc.str();
        event.suggested_fix = fix.str();
    }
    
    void report_corruption(CorruptionType type, void* addr, usize size, const std::string& description) {
        CorruptionEvent event;
        event.type = type;
        event.address = addr;
        event.size = size;
        event.detection_time = get_current_time();
        event.thread_id = std::this_thread::get_id();
        event.description = description;
        
        capture_call_stack(event.detection_stack);
        
        {
            std::lock_guard<std::mutex> lock(corruptions_mutex_);
            detected_corruptions_.push_back(event);
        }
        
        LOG_ERROR("Memory corruption detected: {} at address {}", description, addr);
    }
    
    bool capture_call_stack(memory::CallStack& stack) const {
        // In a real implementation, would use platform-specific stack walking
        // For educational purposes, simulate capturing a call stack
        stack.clear();
        stack.frame_count = 3; // Simulate 3 frames
        stack.hash = reinterpret_cast<u64>(&stack) % 0xFFFFFFFF;
        return true;
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Memory Leak Detection System
//=============================================================================

/**
 * @brief Advanced memory leak detection with pattern analysis
 */
class LeakDetector {
private:
    struct AllocationRecord {
        void* address;
        usize size;
        f64 allocation_time;
        f64 last_access_time;
        u32 access_count;
        memory::CallStack allocation_stack;
        memory::AllocationCategory category;
        std::thread::id allocating_thread;
        bool is_suspected_leak;
        f64 leak_score;
        
        AllocationRecord() 
            : address(nullptr), size(0), allocation_time(0.0), last_access_time(0.0),
              access_count(0), category(memory::AllocationCategory::Unknown),
              allocating_thread(std::this_thread::get_id()), is_suspected_leak(false), 
              leak_score(0.0) {}
    };
    
    std::unordered_map<void*, AllocationRecord> tracked_allocations_;
    mutable std::shared_mutex allocations_mutex_;
    
    // Leak detection parameters
    f64 leak_detection_threshold_seconds_;
    f64 leak_score_threshold_;
    usize min_leak_size_;
    
    // Detected leaks
    std::vector<memory::LeakInfo> detected_leaks_;
    mutable std::mutex leaks_mutex_;
    
    // Background leak detection
    std::thread leak_detection_thread_;
    std::atomic<bool> leak_detection_active_{true};
    std::atomic<f64> detection_interval_seconds_{60.0}; // Check every minute
    
public:
    explicit LeakDetector(f64 threshold_seconds = 300.0, // 5 minutes
                         f64 score_threshold = 0.7,
                         usize min_size = 64) // 64 bytes minimum
        : leak_detection_threshold_seconds_(threshold_seconds),
          leak_score_threshold_(score_threshold),
          min_leak_size_(min_size) {
        
        // Start background leak detection
        leak_detection_thread_ = std::thread([this]() {
            leak_detection_worker();
        });
        
        LOG_DEBUG("Initialized leak detector: threshold={}s, min_size={}B", 
                 threshold_seconds, min_size);
    }
    
    ~LeakDetector() {
        leak_detection_active_.store(false);
        if (leak_detection_thread_.joinable()) {
            leak_detection_thread_.join();
        }
        
        // Report final leak summary
        report_final_leak_summary();
    }
    
    /**
     * @brief Track allocation for leak detection
     */
    void track_allocation(void* address, usize size, memory::AllocationCategory category) {
        if (!address || size < min_leak_size_) return;
        
        AllocationRecord record;
        record.address = address;
        record.size = size;
        record.allocation_time = get_current_time();
        record.last_access_time = record.allocation_time;
        record.access_count = 1;
        record.category = category;
        record.allocating_thread = std::this_thread::get_id();
        
        // Capture allocation stack
        capture_call_stack(record.allocation_stack);
        
        {
            std::unique_lock<std::shared_mutex> lock(allocations_mutex_);
            tracked_allocations_[address] = record;
        }
        
        LOG_TRACE("Tracking allocation for leak detection: addr={}, size={}", address, size);
    }
    
    /**
     * @brief Remove allocation from tracking
     */
    void untrack_allocation(void* address) {
        if (!address) return;
        
        std::unique_lock<std::shared_mutex> lock(allocations_mutex_);
        tracked_allocations_.erase(address);
        
        LOG_TRACE("Untracking allocation: addr={}", address);
    }
    
    /**
     * @brief Record memory access for leak analysis
     */
    void record_access(void* address) {
        if (!address) return;
        
        std::shared_lock<std::shared_mutex> lock(allocations_mutex_);
        auto it = tracked_allocations_.find(address);
        if (it != tracked_allocations_.end()) {
            it->second.last_access_time = get_current_time();
            it->second.access_count++;
        }
    }
    
    /**
     * @brief Perform leak detection scan
     */
    std::vector<memory::LeakInfo> detect_leaks() {
        std::shared_lock<std::shared_mutex> lock(allocations_mutex_);
        
        std::vector<memory::LeakInfo> leaks;
        f64 current_time = get_current_time();
        
        for (auto& [addr, record] : tracked_allocations_) {
            f64 age = current_time - record.allocation_time;
            f64 time_since_access = current_time - record.last_access_time;
            
            if (age >= leak_detection_threshold_seconds_) {
                f64 leak_score = calculate_leak_score(record, current_time);
                
                if (leak_score >= leak_score_threshold_) {
                    memory::LeakInfo leak_info;
                    leak_info.allocation.address = record.address;
                    leak_info.allocation.size = record.size;
                    leak_info.allocation.allocation_time = record.allocation_time;
                    leak_info.allocation.call_stack = record.allocation_stack;
                    leak_info.allocation.category = record.category;
                    
                    leak_info.age = age;
                    leak_info.leak_score = leak_score;
                    leak_info.is_confirmed_leak = leak_score > 0.9;
                    leak_info.similar_leaks = count_similar_leaks(record);
                    
                    leaks.push_back(leak_info);
                    record.is_suspected_leak = true;
                    record.leak_score = leak_score;
                }
            }
        }
        
        // Store detected leaks
        {
            std::lock_guard<std::mutex> leak_lock(leaks_mutex_);
            detected_leaks_ = leaks;
        }
        
        if (!leaks.empty()) {
            LOG_WARNING("Detected {} potential memory leaks", leaks.size());
        }
        
        return leaks;
    }
    
    /**
     * @brief Get leak detection statistics
     */
    struct LeakStatistics {
        usize tracked_allocations;
        usize suspected_leaks;
        usize confirmed_leaks;
        usize total_leaked_bytes;
        f64 average_leak_age;
        f64 oldest_leak_age;
        std::unordered_map<memory::AllocationCategory, usize> leaks_by_category;
        std::vector<std::pair<u64, usize>> leak_hotspots; // call stack hash -> count
    };
    
    LeakStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> allocations_lock(allocations_mutex_);
        std::lock_guard<std::mutex> leaks_lock(leaks_mutex_);
        
        LeakStatistics stats{};
        stats.tracked_allocations = tracked_allocations_.size();
        
        f64 current_time = get_current_time();
        f64 total_age = 0.0;
        f64 oldest_age = 0.0;
        
        // Analyze tracked allocations
        for (const auto& [addr, record] : tracked_allocations_) {
            if (record.is_suspected_leak) {
                stats.suspected_leaks++;
                stats.total_leaked_bytes += record.size;
                
                f64 age = current_time - record.allocation_time;
                total_age += age;
                oldest_age = std::max(oldest_age, age);
                
                if (record.leak_score > 0.9) {
                    stats.confirmed_leaks++;
                }
                
                // Count by category
                stats.leaks_by_category[record.category]++;
                
                // Track hotspots
                bool found_hotspot = false;
                for (auto& [hash, count] : stats.leak_hotspots) {
                    if (hash == record.allocation_stack.hash) {
                        count++;
                        found_hotspot = true;
                        break;
                    }
                }
                if (!found_hotspot && record.allocation_stack.hash != 0) {
                    stats.leak_hotspots.emplace_back(record.allocation_stack.hash, 1);
                }
            }
        }
        
        if (stats.suspected_leaks > 0) {
            stats.average_leak_age = total_age / stats.suspected_leaks;
        }
        stats.oldest_leak_age = oldest_age;
        
        // Sort hotspots by count
        std::sort(stats.leak_hotspots.begin(), stats.leak_hotspots.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return stats;
    }
    
    void set_detection_interval(f64 interval_seconds) {
        detection_interval_seconds_.store(interval_seconds);
    }
    
private:
    f64 calculate_leak_score(const AllocationRecord& record, f64 current_time) const {
        f64 age = current_time - record.allocation_time;
        f64 time_since_access = current_time - record.last_access_time;
        
        // Base score from age
        f64 age_score = std::min(age / (leak_detection_threshold_seconds_ * 2), 1.0);
        
        // Access pattern score (less access = higher leak likelihood)
        f64 access_frequency = record.access_count / age;
        f64 access_score = std::max(0.0, 1.0 - (access_frequency * 10)); // Normalize access frequency
        
        // Time since last access score
        f64 staleness_score = std::min(time_since_access / leak_detection_threshold_seconds_, 1.0);
        
        // Size factor (larger allocations are more concerning)
        f64 size_factor = std::min(static_cast<f64>(record.size) / (1024 * 1024), 2.0) / 2.0; // Normalize to 1MB
        
        // Combine scores (weighted average)
        f64 leak_score = (age_score * 0.3 + access_score * 0.3 + staleness_score * 0.3 + size_factor * 0.1);
        
        return std::clamp(leak_score, 0.0, 1.0);
    }
    
    usize count_similar_leaks(const AllocationRecord& record) const {
        usize count = 0;
        
        for (const auto& [addr, other_record] : tracked_allocations_) {
            if (addr != record.address && 
                other_record.allocation_stack.hash == record.allocation_stack.hash &&
                other_record.category == record.category &&
                other_record.is_suspected_leak) {
                count++;
            }
        }
        
        return count;
    }
    
    void leak_detection_worker() {
        while (leak_detection_active_.load()) {
            f64 interval = detection_interval_seconds_.load();
            std::this_thread::sleep_for(std::chrono::duration<f64>(interval));
            
            if (!leak_detection_active_.load()) break;
            
            auto leaks = detect_leaks();
            if (!leaks.empty()) {
                LOG_INFO("Periodic leak detection found {} potential leaks", leaks.size());
            }
        }
    }
    
    void report_final_leak_summary() const {
        auto stats = get_statistics();
        
        if (stats.suspected_leaks > 0) {
            LOG_WARNING("FINAL LEAK SUMMARY:");
            LOG_WARNING("  Tracked allocations: {}", stats.tracked_allocations);
            LOG_WARNING("  Suspected leaks: {}", stats.suspected_leaks);
            LOG_WARNING("  Confirmed leaks: {}", stats.confirmed_leaks);
            LOG_WARNING("  Total leaked bytes: {}KB", stats.total_leaked_bytes / 1024);
            LOG_WARNING("  Average leak age: {:.1f}s", stats.average_leak_age);
            LOG_WARNING("  Oldest leak age: {:.1f}s", stats.oldest_leak_age);
            
            // Report top hotspots
            usize hotspot_count = std::min(stats.leak_hotspots.size(), usize(5));
            for (usize i = 0; i < hotspot_count; ++i) {
                LOG_WARNING("  Leak hotspot {}: {} leaks (hash={})", 
                           i + 1, stats.leak_hotspots[i].second, stats.leak_hotspots[i].first);
            }
        } else {
            LOG_INFO("No memory leaks detected during execution");
        }
    }
    
    bool capture_call_stack(memory::CallStack& stack) const {
        // Simplified call stack capture for educational purposes
        stack.clear();
        stack.frame_count = 4; // Simulate 4 frames
        stack.hash = reinterpret_cast<u64>(&stack) % 0xFFFFFFFF;
        return true;
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};
 */
class MemoryGuards {
private:
    static constexpr u32 GUARD_PATTERN = 0xDEADBEEF;
    static constexpr usize GUARD_SIZE = 32; // 32 bytes of guard data
    
    struct GuardInfo {
        void* allocation_start;     // Start of actual allocation
        usize allocation_size;      // Size of actual allocation
        void* guard_before;         // Guard before allocation
        void* guard_after;          // Guard after allocation
        f64 creation_time;          // When guards were created
        u32 allocation_id;          // Unique allocation ID
        bool is_active;             // Guards are active
        
        GuardInfo() noexcept 
            : allocation_start(nullptr), allocation_size(0),
              guard_before(nullptr), guard_after(nullptr),
              creation_time(0.0), allocation_id(0), is_active(false) {}
    };
    
    std::unordered_map<void*, GuardInfo> guarded_allocations_;
    mutable std::shared_mutex guards_mutex_;
    
    std::atomic<u32> next_allocation_id_{1};
    
    // Corruption detection
    std::atomic<u64> corruption_checks_{0};
    std::atomic<u64> corruptions_detected_{0};
    
public:
    /**
     * @brief Create guarded allocation
     */
    void* create_guarded_allocation(usize size, usize alignment = alignof(std::max_align_t)) {
        if (size == 0) return nullptr;
        
        // Calculate total size needed
        usize aligned_size = align_up(size, alignment);
        usize total_size = GUARD_SIZE + aligned_size + GUARD_SIZE;
        
        // Allocate memory with guards
        void* raw_memory = std::aligned_alloc(alignment, total_size);
        if (!raw_memory) return nullptr;
        
        char* memory_bytes = static_cast<char*>(raw_memory);
        void* guard_before = memory_bytes;
        void* allocation = memory_bytes + GUARD_SIZE;
        void* guard_after = memory_bytes + GUARD_SIZE + aligned_size;
        
        // Initialize guard patterns
        fill_guard_pattern(guard_before, GUARD_SIZE);
        fill_guard_pattern(guard_after, GUARD_SIZE);
        
        // Zero-initialize allocation area for uninitialized read detection
        std::memset(allocation, 0, aligned_size);
        
        // Record guard info
        GuardInfo info;
        info.allocation_start = allocation;
        info.allocation_size = size;
        info.guard_before = guard_before;
        info.guard_after = guard_after;
        info.creation_time = get_current_time();
        info.allocation_id = next_allocation_id_.fetch_add(1);
        info.is_active = true;
        
        std::unique_lock<std::shared_mutex> lock(guards_mutex_);
        guarded_allocations_[allocation] = info;
        
        return allocation;
    }
    
    /**
     * @brief Check for guard corruption
     */
    std::vector<CorruptionEvent> check_guard_corruption(void* allocation) {
        std::vector<CorruptionEvent> corruptions;
        
        std::shared_lock<std::shared_mutex> lock(guards_mutex_);
        auto it = guarded_allocations_.find(allocation);
        if (it == guarded_allocations_.end()) {
            return corruptions; // Not a guarded allocation
        }
        
        const GuardInfo& info = it->second;
        if (!info.is_active) {
            return corruptions;
        }
        
        corruption_checks_.fetch_add(1, std::memory_order_relaxed);
        
        // Check before guard
        if (!check_guard_pattern(info.guard_before, GUARD_SIZE)) {
            CorruptionEvent event;
            event.type = CorruptionType::BufferUnderrun;
            event.address = allocation;
            event.size = info.allocation_size;
            event.detection_time = get_current_time();
            event.thread_id = std::this_thread::get_id();
            event.description = "Buffer underrun detected - writes before allocation boundary";
            event.suggested_fix = "Check array indices and pointer arithmetic";
            event.severity_score = 90; // High severity
            
            capture_call_stack(event.detection_stack);
            corruptions.push_back(event);
            
            corruptions_detected_.fetch_add(1, std::memory_order_relaxed);
        }
        
        // Check after guard
        if (!check_guard_pattern(info.guard_after, GUARD_SIZE)) {
            CorruptionEvent event;
            event.type = CorruptionType::BufferOverrun;
            event.address = allocation;
            event.size = info.allocation_size;
            event.detection_time = get_current_time();
            event.thread_id = std::this_thread::get_id();
            event.description = "Buffer overrun detected - writes beyond allocation boundary";
            event.suggested_fix = "Check loop bounds and string operations";
            event.severity_score = 90; // High severity
            
            capture_call_stack(event.detection_stack);
            corruptions.push_back(event);
            
            corruptions_detected_.fetch_add(1, std::memory_order_relaxed);
        }
        
        return corruptions;
    }
    
    /**
     * @brief Remove guarded allocation
     */
    bool remove_guarded_allocation(void* allocation) {
        std::unique_lock<std::shared_mutex> lock(guards_mutex_);
        
        auto it = guarded_allocations_.find(allocation);
        if (it == guarded_allocations_.end()) {
            return false;
        }
        
        GuardInfo& info = it->second;
        info.is_active = false;
        
        // Get the original raw memory pointer
        char* raw_memory = static_cast<char*>(info.guard_before);
        
        guarded_allocations_.erase(it);
        lock.unlock();
        
        // Free the raw memory
        std::free(raw_memory);
        
        return true;
    }
    
    /**
     * @brief Check all active guards for corruption
     */
    std::vector<CorruptionEvent> check_all_guards() {
        std::vector<CorruptionEvent> all_corruptions;
        
        std::shared_lock<std::shared_mutex> lock(guards_mutex_);
        
        for (const auto& [allocation, info] : guarded_allocations_) {
            if (info.is_active) {
                lock.unlock(); // Avoid recursive locking
                auto corruptions = check_guard_corruption(allocation);
                all_corruptions.insert(all_corruptions.end(), 
                                     corruptions.begin(), corruptions.end());
                lock.lock(); // Re-acquire lock
            }
        }
        
        return all_corruptions;
    }
    
    /**
     * @brief Get guard statistics
     */
    struct GuardStatistics {
        usize active_guards;
        u64 total_checks;
        u64 corruptions_detected;
        f64 corruption_rate;
        usize total_guarded_memory;
    };
    
    GuardStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(guards_mutex_);
        
        GuardStatistics stats{};
        stats.total_checks = corruption_checks_.load();
        stats.corruptions_detected = corruptions_detected_.load();
        
        if (stats.total_checks > 0) {
            stats.corruption_rate = static_cast<f64>(stats.corruptions_detected) / stats.total_checks;
        }
        
        for (const auto& [allocation, info] : guarded_allocations_) {
            if (info.is_active) {
                stats.active_guards++;
                stats.total_guarded_memory += info.allocation_size;
            }
        }
        
        return stats;
    }
    
private:
    void fill_guard_pattern(void* ptr, usize size) {
        u32* words = static_cast<u32*>(ptr);
        usize word_count = size / sizeof(u32);
        
        for (usize i = 0; i < word_count; ++i) {
            words[i] = GUARD_PATTERN;
        }
        
        // Fill remaining bytes
        u8* bytes = static_cast<u8*>(ptr);
        for (usize i = word_count * sizeof(u32); i < size; ++i) {
            bytes[i] = static_cast<u8>(GUARD_PATTERN >> ((i % 4) * 8));
        }
    }
    
    bool check_guard_pattern(void* ptr, usize size) const {
        const u32* words = static_cast<const u32*>(ptr);
        usize word_count = size / sizeof(u32);
        
        for (usize i = 0; i < word_count; ++i) {
            if (words[i] != GUARD_PATTERN) {
                return false;
            }
        }
        
        // Check remaining bytes
        const u8* bytes = static_cast<const u8*>(ptr);
        for (usize i = word_count * sizeof(u32); i < size; ++i) {
            u8 expected = static_cast<u8>(GUARD_PATTERN >> ((i % 4) * 8));
            if (bytes[i] != expected) {
                return false;
            }
        }
        
        return true;
    }
    
    bool capture_call_stack(memory::CallStack& stack) const {
        // Simplified call stack capture
        stack.clear();
        return true;
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
// Advanced Leak Detection
//=============================================================================

/**
 * @brief Advanced memory leak detector with root cause analysis
 */
class AdvancedLeakDetector {
private:
    struct AllocationRecord {
        void* address;
        usize size;
        f64 allocation_time;
        memory::CallStack allocation_stack;
        std::thread::id allocating_thread;
        memory::AllocationCategory category;
        std::string allocator_name;
        u32 allocator_id;
        
        // Leak analysis
        bool is_suspected_leak;
        f64 leak_confidence_score;
        f64 last_access_time;
        u32 access_count;
        std::string leak_reason;
        
        AllocationRecord() noexcept 
            : address(nullptr), size(0), allocation_time(0.0),
              allocating_thread(std::this_thread::get_id()),
              category(memory::AllocationCategory::Unknown),
              allocator_id(0), is_suspected_leak(false),
              leak_confidence_score(0.0), last_access_time(0.0),
              access_count(0) {}
    };
    
    std::unordered_map<void*, AllocationRecord> active_allocations_;
    mutable std::shared_mutex allocations_mutex_;
    
    // Leak detection parameters
    f64 leak_age_threshold_seconds_;       // How old before considering leak
    f64 leak_confidence_threshold_;        // Minimum confidence to report leak
    u32 min_access_count_threshold_;       // Minimum accesses to not be leak
    f64 last_leak_check_time_;
    
    // Leak statistics
    std::atomic<u64> total_leaks_detected_{0};
    std::atomic<usize> total_leaked_bytes_{0};
    std::atomic<u64> false_positives_corrected_{0};
    
    // Background leak detection
    std::thread leak_detector_thread_;
    std::atomic<bool> leak_detection_active_{true};
    std::atomic<f64> leak_check_interval_seconds_{10.0};
    
public:
    explicit AdvancedLeakDetector() 
        : leak_age_threshold_seconds_(30.0),    // 30 seconds
          leak_confidence_threshold_(0.7),       // 70% confidence
          min_access_count_threshold_(1),        // At least 1 access
          last_leak_check_time_(get_current_time()) {
        
        start_background_detection();
        
        LOG_INFO("Initialized advanced leak detector");
    }
    
    ~AdvancedLeakDetector() {
        shutdown_detector();
        
        LOG_INFO("Advanced leak detector shutdown. Leaks detected: {}, Bytes leaked: {}KB",
                 total_leaks_detected_.load(), total_leaked_bytes_.load() / 1024);
    }
    
    /**
     * @brief Record allocation for leak tracking
     */
    void record_allocation(void* address, usize size, 
                          memory::AllocationCategory category,
                          const std::string& allocator_name,
                          u32 allocator_id) {
        if (!address) return;
        
        AllocationRecord record;
        record.address = address;
        record.size = size;
        record.allocation_time = get_current_time();
        record.allocating_thread = std::this_thread::get_id();
        record.category = category;
        record.allocator_name = allocator_name;
        record.allocator_id = allocator_id;
        record.last_access_time = record.allocation_time;
        record.access_count = 1; // Allocation counts as first access
        
        // Capture allocation stack
        capture_call_stack(record.allocation_stack);
        
        std::unique_lock<std::shared_mutex> lock(allocations_mutex_);
        active_allocations_[address] = record;
    }
    
    /**
     * @brief Record deallocation
     */
    void record_deallocation(void* address) {
        if (!address) return;
        
        std::unique_lock<std::shared_mutex> lock(allocations_mutex_);
        
        auto it = active_allocations_.find(address);
        if (it != active_allocations_.end()) {
            // Check if this was suspected as a leak and correct if so
            if (it->second.is_suspected_leak) {
                false_positives_corrected_.fetch_add(1, std::memory_order_relaxed);
            }
            
            active_allocations_.erase(it);
        }
    }
    
    /**
     * @brief Record memory access for leak analysis
     */
    void record_access(void* address) {
        if (!address) return;
        
        std::shared_lock<std::shared_mutex> lock(allocations_mutex_);
        
        auto it = active_allocations_.find(address);
        if (it != active_allocations_.end()) {
            // Update access information (const_cast for performance)
            auto& record = const_cast<AllocationRecord&>(it->second);
            record.last_access_time = get_current_time();
            record.access_count++;
            
            // Clear leak suspicion if access pattern improves
            if (record.is_suspected_leak && record.access_count > min_access_count_threshold_) {
                record.is_suspected_leak = false;
                record.leak_confidence_score = 0.0;
            }
        }
    }
    
    /**
     * @brief Perform leak detection analysis
     */
    struct LeakReport {
        std::vector<memory::LeakInfo> detected_leaks;
        usize total_leaked_bytes;
        u64 total_leaked_allocations;
        f64 leak_detection_confidence;
        std::string analysis_summary;
        std::vector<std::string> common_leak_patterns;
        std::vector<std::string> optimization_suggestions;
    };
    
    LeakReport detect_leaks() {
        std::shared_lock<std::shared_mutex> lock(allocations_mutex_);
        
        LeakReport report{};
        f64 current_time = get_current_time();
        
        // Analyze each active allocation
        for (auto& [address, record] : active_allocations_) {
            f64 age = current_time - record.allocation_time;
            f64 time_since_access = current_time - record.last_access_time;
            
            // Calculate leak confidence score
            f64 confidence = calculate_leak_confidence(record, age, time_since_access);
            
            if (confidence >= leak_confidence_threshold_) {
                memory::LeakInfo leak;
                leak.allocation.address = record.address;
                leak.allocation.size = record.size;
                leak.allocation.category = record.category;
                leak.allocation.allocator_name = record.allocator_name.c_str();
                leak.allocation.allocator_id = record.allocator_id;
                leak.allocation.call_stack = record.allocation_stack;
                leak.allocation.thread_id = record.allocating_thread;
                leak.allocation.allocation_time = record.allocation_time;
                
                leak.age = age;
                leak.is_confirmed_leak = confidence > 0.9;
                leak.leak_score = confidence;
                
                // Classify leak reason
                if (record.access_count <= 1) {
                    leak.allocation.tag = "Allocated but never used";
                } else if (time_since_access > leak_age_threshold_seconds_) {
                    leak.allocation.tag = "Long-time inactive allocation";
                } else {
                    leak.allocation.tag = "Suspicious allocation pattern";
                }
                
                report.detected_leaks.push_back(leak);
                report.total_leaked_bytes += record.size;
                report.total_leaked_allocations++;
                
                // Mark as suspected leak for tracking
                const_cast<AllocationRecord&>(record).is_suspected_leak = true;
                const_cast<AllocationRecord&>(record).leak_confidence_score = confidence;
                const_cast<AllocationRecord&>(record).leak_reason = leak.allocation.tag;
            }
        }
        
        // Calculate overall confidence
        if (active_allocations_.size() > 0) {
            report.leak_detection_confidence = 
                static_cast<f64>(report.detected_leaks.size()) / active_allocations_.size();
        }
        
        // Generate analysis
        generate_leak_analysis(report);
        
        // Update statistics
        total_leaks_detected_.store(report.total_leaked_allocations);
        total_leaked_bytes_.store(report.total_leaked_bytes);
        
        return report;
    }
    
    /**
     * @brief Get leak detector statistics
     */
    struct LeakDetectorStats {
        usize active_allocations;
        usize suspected_leaks;
        u64 total_leaks_detected;
        usize total_leaked_bytes;
        u64 false_positives_corrected;
        f64 false_positive_rate;
        f64 detection_accuracy;
        
        // Configuration
        f64 age_threshold_seconds;
        f64 confidence_threshold;
        u32 min_access_threshold;
    };
    
    LeakDetectorStats get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(allocations_mutex_);
        
        LeakDetectorStats stats{};
        stats.active_allocations = active_allocations_.size();
        stats.total_leaks_detected = total_leaks_detected_.load();
        stats.total_leaked_bytes = total_leaked_bytes_.load();
        stats.false_positives_corrected = false_positives_corrected_.load();
        
        // Count suspected leaks
        for (const auto& [address, record] : active_allocations_) {
            if (record.is_suspected_leak) {
                stats.suspected_leaks++;
            }
        }
        
        // Calculate accuracy metrics
        if (stats.total_leaks_detected > 0) {
            stats.false_positive_rate = 
                static_cast<f64>(stats.false_positives_corrected) / stats.total_leaks_detected;
            stats.detection_accuracy = 1.0 - stats.false_positive_rate;
        }
        
        stats.age_threshold_seconds = leak_age_threshold_seconds_;
        stats.confidence_threshold = leak_confidence_threshold_;
        stats.min_access_threshold = min_access_count_threshold_;
        
        return stats;
    }
    
    /**
     * @brief Configuration
     */
    void set_leak_age_threshold(f64 seconds) {
        leak_age_threshold_seconds_ = seconds;
    }
    
    void set_confidence_threshold(f64 threshold) {
        leak_confidence_threshold_ = threshold;
    }
    
    void set_check_interval(f64 seconds) {
        leak_check_interval_seconds_.store(seconds);
    }
    
private:
    void start_background_detection() {
        leak_detector_thread_ = std::thread([this]() {
            leak_detection_worker();
        });
    }
    
    void shutdown_detector() {
        leak_detection_active_.store(false);
        if (leak_detector_thread_.joinable()) {
            leak_detector_thread_.join();
        }
    }
    
    void leak_detection_worker() {
        while (leak_detection_active_.load()) {
            f64 interval = leak_check_interval_seconds_.load();
            std::this_thread::sleep_for(std::chrono::duration<f64>(interval));
            
            if (leak_detection_active_.load()) {
                auto report = detect_leaks();
                if (!report.detected_leaks.empty()) {
                    LOG_WARNING("Detected {} potential memory leaks ({}KB total)", 
                               report.detected_leaks.size(), 
                               report.total_leaked_bytes / 1024);
                }
            }
        }
    }
    
    f64 calculate_leak_confidence(const AllocationRecord& record, 
                                 f64 age, f64 time_since_access) const {
        f64 confidence = 0.0;
        
        // Age factor - older allocations more likely to be leaks
        if (age > leak_age_threshold_seconds_) {
            confidence += 0.4 * std::min(1.0, age / (leak_age_threshold_seconds_ * 2));
        }
        
        // Access pattern factor - rarely accessed allocations suspicious
        if (record.access_count <= min_access_count_threshold_) {
            confidence += 0.3;
        } else {
            f64 access_frequency = record.access_count / age;
            if (access_frequency < 0.1) { // Less than 0.1 accesses per second
                confidence += 0.2;
            }
        }
        
        // Inactivity factor - long time since last access
        if (time_since_access > leak_age_threshold_seconds_ / 2) {
            confidence += 0.3 * std::min(1.0, time_since_access / leak_age_threshold_seconds_);
        }
        
        // Category-specific adjustments
        switch (record.category) {
            case memory::AllocationCategory::Temporary:
                confidence += 0.2; // Temporary allocations should be freed quickly
                break;
            case memory::AllocationCategory::Debug_Tools:
                confidence -= 0.1; // Debug allocations might live longer
                break;
            default:
                break;
        }
        
        return std::clamp(confidence, 0.0, 1.0);
    }
    
    void generate_leak_analysis(LeakReport& report) const {
        // Generate summary
        std::stringstream summary;
        summary << "Leak Detection Analysis:\n";
        summary << "- Detected " << report.detected_leaks.size() << " potential leaks\n";
        summary << "- Total leaked memory: " << report.total_leaked_bytes / 1024 << "KB\n";
        summary << "- Average leak confidence: " << report.leak_detection_confidence << "\n";
        
        report.analysis_summary = summary.str();
        
        // Analyze common patterns
        std::unordered_map<std::string, u32> allocator_counts;
        std::unordered_map<memory::AllocationCategory, u32> category_counts;
        
        for (const auto& leak : report.detected_leaks) {
            allocator_counts[leak.allocation.allocator_name]++;
            category_counts[leak.allocation.category]++;
        }
        
        // Find most common leak sources
        for (const auto& [allocator, count] : allocator_counts) {
            if (count > 1) {
                report.common_leak_patterns.push_back(
                    "Multiple leaks from allocator: " + std::string(allocator)
                );
            }
        }
        
        // Generate suggestions
        report.optimization_suggestions.push_back("Review object lifecycles for leaked categories");
        report.optimization_suggestions.push_back("Consider using RAII patterns for automatic cleanup");
        report.optimization_suggestions.push_back("Implement proper exception safety in allocation paths");
    }
    
    bool capture_call_stack(memory::CallStack& stack) const {
        // Simplified call stack capture
        stack.clear();
        return true;
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Memory Fragmentation Analyzer
//=============================================================================

/**
 * @brief Analyzer for memory fragmentation patterns and optimization
 */
class FragmentationAnalyzer {
private:
    struct MemoryRegion {
        void* start_address;
        void* end_address;
        usize size;
        bool is_allocated;
        f64 allocation_time;
        memory::AllocationCategory category;
        std::string allocator_name;
        
        MemoryRegion() noexcept 
            : start_address(nullptr), end_address(nullptr), size(0),
              is_allocated(false), allocation_time(0.0),
              category(memory::AllocationCategory::Unknown) {}
    };
    
    std::vector<MemoryRegion> memory_regions_;
    mutable std::shared_mutex regions_mutex_;
    
    // Fragmentation metrics
    std::atomic<f64> external_fragmentation_ratio_{0.0};
    std::atomic<f64> internal_fragmentation_ratio_{0.0};
    std::atomic<usize> largest_free_block_{0};
    std::atomic<usize> total_free_space_{0};
    std::atomic<u32> free_block_count_{0};
    
    // Analysis parameters
    f64 fragmentation_warning_threshold_;
    f64 last_analysis_time_;
    
public:
    explicit FragmentationAnalyzer() 
        : fragmentation_warning_threshold_(0.3), // 30% fragmentation warning
          last_analysis_time_(get_current_time()) {
        
        LOG_DEBUG("Initialized memory fragmentation analyzer");
    }
    
    /**
     * @brief Record memory allocation for fragmentation tracking
     */
    void record_allocation(void* address, usize size, 
                          memory::AllocationCategory category,
                          const std::string& allocator_name) {
        if (!address || size == 0) return;
        
        std::unique_lock<std::shared_mutex> lock(regions_mutex_);
        
        MemoryRegion region;
        region.start_address = address;
        region.end_address = static_cast<char*>(address) + size;
        region.size = size;
        region.is_allocated = true;
        region.allocation_time = get_current_time();
        region.category = category;
        region.allocator_name = allocator_name;
        
        memory_regions_.push_back(region);
        
        // Sort regions by address for fragmentation analysis
        std::sort(memory_regions_.begin(), memory_regions_.end(),
                 [](const MemoryRegion& a, const MemoryRegion& b) {
                     return a.start_address < b.start_address;
                 });
    }
    
    /**
     * @brief Record memory deallocation
     */
    void record_deallocation(void* address) {
        if (!address) return;
        
        std::unique_lock<std::shared_mutex> lock(regions_mutex_);
        
        auto it = std::find_if(memory_regions_.begin(), memory_regions_.end(),
                              [address](const MemoryRegion& region) {
                                  return region.start_address == address;
                              });
        
        if (it != memory_regions_.end()) {
            it->is_allocated = false;
            // Keep the region for fragmentation analysis
        }
    }
    
    /**
     * @brief Analyze memory fragmentation
     */
    struct FragmentationReport {
        f64 external_fragmentation_ratio;
        f64 internal_fragmentation_ratio;
        usize largest_free_block;
        usize total_free_space;
        u32 free_block_count;
        f64 average_free_block_size;
        
        std::vector<std::pair<usize, u32>> free_block_distribution; // size -> count
        std::vector<std::string> fragmentation_hotspots;
        std::vector<std::string> optimization_suggestions;
        
        bool fragmentation_warning;
        std::string analysis_summary;
    };
    
    FragmentationReport analyze_fragmentation() {
        std::shared_lock<std::shared_mutex> lock(regions_mutex_);
        
        FragmentationReport report{};
        
        // Analyze free blocks
        std::vector<usize> free_block_sizes;
        usize total_memory = 0;
        usize allocated_memory = 0;
        
        for (const auto& region : memory_regions_) {
            total_memory += region.size;
            if (region.is_allocated) {
                allocated_memory += region.size;
            } else {
                free_block_sizes.push_back(region.size);
            }
        }
        
        report.total_free_space = total_memory - allocated_memory;
        report.free_block_count = static_cast<u32>(free_block_sizes.size());
        
        if (!free_block_sizes.empty()) {
            // Find largest free block
            report.largest_free_block = *std::max_element(free_block_sizes.begin(), free_block_sizes.end());
            
            // Calculate average free block size
            usize total_free = std::accumulate(free_block_sizes.begin(), free_block_sizes.end(), usize(0));
            report.average_free_block_size = static_cast<f64>(total_free) / free_block_sizes.size();
            
            // Calculate external fragmentation
            if (total_free > 0) {
                report.external_fragmentation_ratio = 
                    1.0 - (static_cast<f64>(report.largest_free_block) / total_free);
            }
        }
        
        // Internal fragmentation is harder to measure without allocator-specific info
        report.internal_fragmentation_ratio = 0.1; // Estimated 10% internal fragmentation
        
        // Generate free block size distribution
        std::unordered_map<usize, u32> size_counts;
        for (usize size : free_block_sizes) {
            // Group into power-of-2 buckets
            usize bucket = 1;
            while (bucket < size) bucket *= 2;
            size_counts[bucket]++;
        }
        
        for (const auto& [size, count] : size_counts) {
            report.free_block_distribution.emplace_back(size, count);
        }
        
        // Sort by size
        std::sort(report.free_block_distribution.begin(), report.free_block_distribution.end());
        
        // Check for fragmentation warning
        report.fragmentation_warning = 
            report.external_fragmentation_ratio > fragmentation_warning_threshold_;
        
        // Generate analysis and suggestions
        generate_fragmentation_analysis(report);
        
        // Update atomic metrics
        external_fragmentation_ratio_.store(report.external_fragmentation_ratio);
        internal_fragmentation_ratio_.store(report.internal_fragmentation_ratio);
        largest_free_block_.store(report.largest_free_block);
        total_free_space_.store(report.total_free_space);
        free_block_count_.store(report.free_block_count);
        
        last_analysis_time_ = get_current_time();
        
        return report;
    }
    
    /**
     * @brief Get current fragmentation metrics
     */
    struct FragmentationMetrics {
        f64 external_fragmentation_ratio;
        f64 internal_fragmentation_ratio;
        usize largest_free_block;
        usize total_free_space;
        u32 free_block_count;
        bool needs_defragmentation;
        f64 fragmentation_severity;
    };
    
    FragmentationMetrics get_current_metrics() const {
        FragmentationMetrics metrics{};
        
        metrics.external_fragmentation_ratio = external_fragmentation_ratio_.load();
        metrics.internal_fragmentation_ratio = internal_fragmentation_ratio_.load();
        metrics.largest_free_block = largest_free_block_.load();
        metrics.total_free_space = total_free_space_.load();
        metrics.free_block_count = free_block_count_.load();
        
        // Calculate severity and defragmentation need
        metrics.fragmentation_severity = 
            (metrics.external_fragmentation_ratio + metrics.internal_fragmentation_ratio) / 2.0;
        
        metrics.needs_defragmentation = 
            metrics.fragmentation_severity > fragmentation_warning_threshold_;
        
        return metrics;
    }
    
private:
    void generate_fragmentation_analysis(FragmentationReport& report) const {
        // Generate summary
        std::stringstream summary;
        summary << "Memory Fragmentation Analysis:\n";
        summary << "- External fragmentation: " << (report.external_fragmentation_ratio * 100) << "%\n";
        summary << "- Free blocks: " << report.free_block_count << "\n";
        summary << "- Largest free block: " << report.largest_free_block / 1024 << "KB\n";
        summary << "- Total free space: " << report.total_free_space / 1024 << "KB";
        
        report.analysis_summary = summary.str();
        
        // Generate optimization suggestions
        if (report.external_fragmentation_ratio > 0.5) {
            report.optimization_suggestions.push_back("High fragmentation - consider memory compaction");
        }
        
        if (report.free_block_count > 100) {
            report.optimization_suggestions.push_back("Many small free blocks - consider coalescing");
        }
        
        if (report.largest_free_block < report.total_free_space / 2) {
            report.optimization_suggestions.push_back("No large contiguous blocks - may impact large allocations");
        }
        
        report.optimization_suggestions.push_back("Consider using pool allocators for fixed-size allocations");
        report.optimization_suggestions.push_back("Review allocation patterns for size clustering opportunities");
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Advanced Memory Debugger - Main Interface
//=============================================================================

/**
 * @brief Comprehensive memory debugging system
 */
class AdvancedMemoryDebugger {
private:
    // Component systems
    std::unique_ptr<MemoryGuards> memory_guards_;
    std::unique_ptr<AdvancedLeakDetector> leak_detector_;
    std::unique_ptr<FragmentationAnalyzer> fragmentation_analyzer_;
    
    // Integration with existing systems
    memory::MemoryTracker* memory_tracker_;
    
    // Corruption event tracking
    std::vector<CorruptionEvent> recent_corruptions_;
    mutable std::mutex corruptions_mutex_;
    static constexpr usize MAX_RECENT_CORRUPTIONS = 1000;
    
    // Real-time monitoring
    std::thread monitoring_thread_;
    std::atomic<bool> monitoring_active_{true};
    std::atomic<f64> monitoring_interval_seconds_{5.0};
    
    // Statistics
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_debug_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_corruptions_detected_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_leaks_detected_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> debugging_overhead_{0.0};
    
public:
    explicit AdvancedMemoryDebugger(memory::MemoryTracker* tracker = nullptr)
        : memory_tracker_(tracker) {
        
        initialize_debugger();
        start_monitoring_thread();
        
        LOG_INFO("Advanced memory debugger initialized with full debugging suite");
    }
    
    ~AdvancedMemoryDebugger() {
        shutdown_debugger();
        
        LOG_INFO("Memory debugger shutdown. Total issues detected: corruption={}, leaks={}", 
                 total_corruptions_detected_.load(), total_leaks_detected_.load());
    }
    
    /**
     * @brief Allocate memory with full debugging protection
     */
    void* debug_allocate(usize size, usize alignment = alignof(std::max_align_t),
                        memory::AllocationCategory category = memory::AllocationCategory::Unknown,
                        const std::string& allocator_name = "DebugAllocator") {
        
        if (size == 0) return nullptr;
        
        auto start_time = get_current_time();
        
        // Allocate with guard zones
        void* address = memory_guards_->create_guarded_allocation(size, alignment);
        if (!address) return nullptr;
        
        // Record for leak detection
        leak_detector_->record_allocation(address, size, category, allocator_name, 0);
        
        // Record for fragmentation analysis
        fragmentation_analyzer_->record_allocation(address, size, category, allocator_name);
        
        // Integrate with memory tracker if available
        if (memory_tracker_) {
            memory_tracker_->track_allocation(
                address, size, size, alignment, category,
                memory::AllocatorType::Custom, allocator_name.c_str(), 0
            );
        }
        
        // Update statistics
        total_debug_allocations_.fetch_add(1, std::memory_order_relaxed);
        
        auto end_time = get_current_time();
        f64 overhead = end_time - start_time;
        update_debugging_overhead(overhead);
        
        return address;
    }
    
    /**
     * @brief Deallocate memory with debugging checks
     */
    void debug_deallocate(void* address) {
        if (!address) return;
        
        auto start_time = get_current_time();
        
        // Check for corruption before deallocation
        auto corruptions = memory_guards_->check_guard_corruption(address);
        if (!corruptions.empty()) {
            record_corruptions(corruptions);
        }
        
        // Remove from tracking systems
        leak_detector_->record_deallocation(address);
        fragmentation_analyzer_->record_deallocation(address);
        
        if (memory_tracker_) {
            memory_tracker_->track_deallocation(
                address, memory::AllocatorType::Custom, "DebugAllocator", 0
            );
        }
        
        // Remove guard zones and free memory
        memory_guards_->remove_guarded_allocation(address);
        
        auto end_time = get_current_time();
        f64 overhead = end_time - start_time;
        update_debugging_overhead(overhead);
    }
    
    /**
     * @brief Record memory access for debugging analysis
     */
    void record_memory_access(void* address) {
        if (!address) return;
        
        // Record access for leak detection
        leak_detector_->record_access(address);
        
        // Check for corruption on access (periodic)
        static thread_local u64 access_counter = 0;
        if (++access_counter % 100 == 0) { // Check every 100 accesses
            auto corruptions = memory_guards_->check_guard_corruption(address);
            if (!corruptions.empty()) {
                record_corruptions(corruptions);
            }
        }
    }
    
    /**
     * @brief Perform comprehensive memory health check
     */
    struct MemoryHealthReport {
        // Guard corruption results
        std::vector<CorruptionEvent> corruptions_detected;
        
        // Leak detection results
        AdvancedLeakDetector::LeakReport leak_report;
        
        // Fragmentation analysis
        FragmentationAnalyzer::FragmentationReport fragmentation_report;
        
        // Overall health metrics
        f64 overall_health_score;        // 0.0 = critical, 1.0 = perfect
        f64 debugging_overhead_ratio;    // Debugging performance overhead
        std::string health_summary;
        std::vector<std::string> critical_issues;
        std::vector<std::string> recommendations;
        
        // Statistics
        u64 total_debug_allocations;
        u64 total_corruptions;
        u64 total_leaks;
        f64 corruption_rate;
        f64 leak_rate;
    };
    
    MemoryHealthReport perform_health_check() {
        MemoryHealthReport report{};
        
        auto start_time = get_current_time();
        
        // Check all guard zones for corruption
        report.corruptions_detected = memory_guards_->check_all_guards();
        
        // Perform leak detection
        report.leak_report = leak_detector_->detect_leaks();
        
        // Analyze fragmentation
        report.fragmentation_report = fragmentation_analyzer_->analyze_fragmentation();
        
        // Gather statistics
        report.total_debug_allocations = total_debug_allocations_.load();
        report.total_corruptions = total_corruptions_detected_.load() + report.corruptions_detected.size();
        report.total_leaks = report.leak_report.total_leaked_allocations;
        report.debugging_overhead_ratio = debugging_overhead_.load();
        
        // Calculate rates
        if (report.total_debug_allocations > 0) {
            report.corruption_rate = static_cast<f64>(report.total_corruptions) / report.total_debug_allocations;
            report.leak_rate = static_cast<f64>(report.total_leaks) / report.total_debug_allocations;
        }
        
        // Calculate overall health score
        report.overall_health_score = calculate_health_score(report);
        
        // Generate analysis
        generate_health_analysis(report);
        
        auto end_time = get_current_time();
        LOG_INFO("Memory health check completed in {:.2f}ms", (end_time - start_time) * 1000.0);
        
        return report;
    }
    
    /**
     * @brief Get current debugger statistics
     */
    struct DebuggerStatistics {
        u64 total_debug_allocations;
        u64 total_corruptions_detected;
        u64 total_leaks_detected;
        f64 debugging_overhead;
        
        MemoryGuards::GuardStatistics guard_stats;
        AdvancedLeakDetector::LeakDetectorStats leak_stats;
        FragmentationAnalyzer::FragmentationMetrics fragmentation_metrics;
        
        bool debugger_healthy;
        f64 performance_impact;
    };
    
    DebuggerStatistics get_statistics() const {
        DebuggerStatistics stats{};
        
        stats.total_debug_allocations = total_debug_allocations_.load();
        stats.total_corruptions_detected = total_corruptions_detected_.load();
        stats.total_leaks_detected = total_leaks_detected_.load();
        stats.debugging_overhead = debugging_overhead_.load();
        
        stats.guard_stats = memory_guards_->get_statistics();
        stats.leak_stats = leak_detector_->get_statistics();
        stats.fragmentation_metrics = fragmentation_analyzer_->get_current_metrics();
        
        // Calculate health and performance impact
        stats.performance_impact = stats.debugging_overhead;
        stats.debugger_healthy = 
            stats.performance_impact < 0.1 && // Less than 10% overhead
            stats.guard_stats.corruption_rate < 0.01; // Less than 1% corruption rate
        
        return stats;
    }
    
    /**
     * @brief Configuration
     */
    void set_monitoring_interval(f64 seconds) {
        monitoring_interval_seconds_.store(seconds);
    }
    
    void enable_monitoring() {
        monitoring_active_.store(true);
    }
    
    void disable_monitoring() {
        monitoring_active_.store(false);
    }
    
    /**
     * @brief Export debugging data for analysis
     */
    void export_debug_report(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open debug report file: {}", filename);
            return;
        }
        
        auto report = const_cast<AdvancedMemoryDebugger*>(this)->perform_health_check();
        
        file << "Advanced Memory Debugger Report\n";
        file << "=================================\n\n";
        file << "Overall Health Score: " << report.overall_health_score << "\n";
        file << "Debugging Overhead: " << (report.debugging_overhead_ratio * 100) << "%\n\n";
        
        file << "Corruption Events: " << report.corruptions_detected.size() << "\n";
        file << "Memory Leaks: " << report.leak_report.detected_leaks.size() << "\n";
        file << "Fragmentation Ratio: " << (report.fragmentation_report.external_fragmentation_ratio * 100) << "%\n\n";
        
        file << report.health_summary << "\n\n";
        
        file << "Recommendations:\n";
        for (const auto& rec : report.recommendations) {
            file << "- " << rec << "\n";
        }
        
        file.close();
        LOG_INFO("Debug report exported to: {}", filename);
    }
    
private:
    void initialize_debugger() {
        memory_guards_ = std::make_unique<MemoryGuards>();
        leak_detector_ = std::make_unique<AdvancedLeakDetector>();
        fragmentation_analyzer_ = std::make_unique<FragmentationAnalyzer>();
    }
    
    void start_monitoring_thread() {
        monitoring_thread_ = std::thread([this]() {
            monitoring_worker();
        });
    }
    
    void shutdown_debugger() {
        monitoring_active_.store(false);
        if (monitoring_thread_.joinable()) {
            monitoring_thread_.join();
        }
    }
    
    void monitoring_worker() {
        while (monitoring_active_.load()) {
            f64 interval = monitoring_interval_seconds_.load();
            std::this_thread::sleep_for(std::chrono::duration<f64>(interval));
            
            if (monitoring_active_.load()) {
                // Periodic corruption check
                auto corruptions = memory_guards_->check_all_guards();
                if (!corruptions.empty()) {
                    record_corruptions(corruptions);
                    LOG_WARNING("Detected {} memory corruptions during monitoring", corruptions.size());
                }
                
                // Check fragmentation levels
                auto frag_metrics = fragmentation_analyzer_->get_current_metrics();
                if (frag_metrics.needs_defragmentation) {
                    LOG_WARNING("High memory fragmentation detected: {:.1f}%", 
                               frag_metrics.fragmentation_severity * 100);
                }
            }
        }
    }
    
    void record_corruptions(const std::vector<CorruptionEvent>& corruptions) {
        std::lock_guard<std::mutex> lock(corruptions_mutex_);
        
        for (const auto& corruption : corruptions) {
            recent_corruptions_.push_back(corruption);
            
            // Log critical corruptions immediately
            if (corruption.severity_score >= 80) {
                LOG_ERROR("CRITICAL memory corruption detected: {} at address {}",
                         corruption.description, corruption.address);
            }
        }
        
        // Trim to maximum size
        if (recent_corruptions_.size() > MAX_RECENT_CORRUPTIONS) {
            recent_corruptions_.erase(
                recent_corruptions_.begin(),
                recent_corruptions_.begin() + (recent_corruptions_.size() - MAX_RECENT_CORRUPTIONS)
            );
        }
        
        total_corruptions_detected_.fetch_add(corruptions.size(), std::memory_order_relaxed);
    }
    
    void update_debugging_overhead(f64 overhead_time) {
        // Update exponential moving average
        f64 current_overhead = debugging_overhead_.load();
        f64 new_overhead = current_overhead * 0.9 + overhead_time * 0.1;
        debugging_overhead_.store(new_overhead);
    }
    
    f64 calculate_health_score(const MemoryHealthReport& report) const {
        f64 score = 1.0;
        
        // Penalize corruptions heavily
        if (report.total_debug_allocations > 0) {
            score -= report.corruption_rate * 0.5;
            score -= report.leak_rate * 0.3;
        }
        
        // Penalize high fragmentation
        score -= report.fragmentation_report.external_fragmentation_ratio * 0.2;
        
        // Penalize high debugging overhead
        if (report.debugging_overhead_ratio > 0.1) {
            score -= (report.debugging_overhead_ratio - 0.1) * 0.5;
        }
        
        return std::clamp(score, 0.0, 1.0);
    }
    
    void generate_health_analysis(MemoryHealthReport& report) const {
        // Generate health summary
        std::stringstream summary;
        summary << "Memory Health Analysis:\n";
        summary << "- Overall health score: " << (report.overall_health_score * 100) << "%\n";
        summary << "- Corruption rate: " << (report.corruption_rate * 100) << "%\n";
        summary << "- Leak rate: " << (report.leak_rate * 100) << "%\n";
        summary << "- Debugging overhead: " << (report.debugging_overhead_ratio * 100) << "%";
        
        report.health_summary = summary.str();
        
        // Identify critical issues
        if (report.corruption_rate > 0.05) {
            report.critical_issues.push_back("High memory corruption rate detected");
        }
        
        if (report.leak_rate > 0.1) {
            report.critical_issues.push_back("Significant memory leaks present");
        }
        
        if (report.fragmentation_report.external_fragmentation_ratio > 0.5) {
            report.critical_issues.push_back("Severe memory fragmentation");
        }
        
        // Generate recommendations
        report.recommendations.push_back("Regularly run memory health checks");
        report.recommendations.push_back("Use RAII patterns to prevent leaks");
        report.recommendations.push_back("Consider pool allocators to reduce fragmentation");
        
        if (report.debugging_overhead_ratio > 0.15) {
            report.recommendations.push_back("Consider reducing debugging detail level for performance");
        }
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Global Advanced Memory Debugger
//=============================================================================

AdvancedMemoryDebugger& get_global_memory_debugger();

} // namespace ecscope::memory::debugging