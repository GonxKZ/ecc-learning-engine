#include "arena.hpp"
#include "core/time.hpp"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <mutex>

namespace ecscope::memory {

// Global arena registry
static std::vector<ArenaAllocator*> g_registered_arenas;
static std::mutex g_registry_mutex;

ArenaAllocator::ArenaAllocator(usize size, const std::string& name, bool enable_tracking)
    : memory_(nullptr)
    , total_size_(size)
    , current_offset_(0)
    , peak_offset_(0)
    , enable_tracking_(enable_tracking)
    , enable_debug_fill_(true)
    , debug_alloc_pattern_(0xCD) // "Clean memory" pattern
    , debug_free_pattern_(0xDD)  // "Dead memory" pattern
    , name_(name) {
    
    // Initialize statistics
    stats_ = ArenaStats{};
    stats_.total_size = total_size_;
    
    // Allocate memory
    initialize_memory();
    
    // Register with global registry
    arena_registry::register_arena(this);
    
    LOG_INFO("Arena '" + name_ + "' created: " + 
             std::to_string(total_size_ / 1024) + " KB");
}

ArenaAllocator::~ArenaAllocator() {
    arena_registry::unregister_arena(this);
    cleanup_memory();
    
    LOG_INFO("Arena '" + name_ + "' destroyed");
}

ArenaAllocator::ArenaAllocator(ArenaAllocator&& other) noexcept
    : memory_(other.memory_)
    , total_size_(other.total_size_)
    , current_offset_(other.current_offset_)
    , peak_offset_(other.peak_offset_)
    , allocations_(std::move(other.allocations_))
    , stats_(other.stats_)
    , enable_tracking_(other.enable_tracking_)
    , enable_debug_fill_(other.enable_debug_fill_)
    , debug_alloc_pattern_(other.debug_alloc_pattern_)
    , debug_free_pattern_(other.debug_free_pattern_)
    , name_(std::move(other.name_)) {
    
    // Nullify moved-from object
    other.memory_ = nullptr;
    other.total_size_ = 0;
    other.current_offset_ = 0;
    other.peak_offset_ = 0;
    
    // Update registry
    arena_registry::unregister_arena(&other);
    arena_registry::register_arena(this);
}

ArenaAllocator& ArenaAllocator::operator=(ArenaAllocator&& other) noexcept {
    if (this != &other) {
        // Clean up current state
        arena_registry::unregister_arena(this);
        cleanup_memory();
        
        // Move from other
        memory_ = other.memory_;
        total_size_ = other.total_size_;
        current_offset_ = other.current_offset_;
        peak_offset_ = other.peak_offset_;
        allocations_ = std::move(other.allocations_);
        stats_ = other.stats_;
        enable_tracking_ = other.enable_tracking_;
        enable_debug_fill_ = other.enable_debug_fill_;
        debug_alloc_pattern_ = other.debug_alloc_pattern_;
        debug_free_pattern_ = other.debug_free_pattern_;
        name_ = std::move(other.name_);
        
        // Nullify moved-from object
        other.memory_ = nullptr;
        other.total_size_ = 0;
        other.current_offset_ = 0;
        other.peak_offset_ = 0;
        
        // Update registry
        arena_registry::unregister_arena(&other);
        arena_registry::register_arena(this);
    }
    return *this;
}

void* ArenaAllocator::allocate(usize size, usize alignment, const char* category) {
    return allocate_debug(size, alignment, category, nullptr, 0, nullptr);
}

void* ArenaAllocator::allocate_debug(usize size, usize alignment, const char* category, 
                                    const char* file, int line, const char* function) {
    if (size == 0) return nullptr;
    
    core::Timer alloc_timer;
    
    // Align the current offset
    usize aligned_offset = align_size(current_offset_, alignment);
    usize aligned_size = align_size(size, alignment);
    
    // Check if we have enough space
    if (aligned_offset + aligned_size > total_size_) {
        LOG_WARN("Arena '" + name_ + "' out of memory: requested " + 
                std::to_string(aligned_size) + " bytes, available " + 
                std::to_string(total_size_ - aligned_offset) + " bytes");
        return nullptr;
    }
    
    // Calculate wasted bytes due to alignment
    usize wasted = aligned_offset - current_offset_;
    
    // Get pointer to allocated memory
    void* ptr = memory_ + aligned_offset;
    
    // Fill with debug pattern if enabled
    if (enable_debug_fill_) {
        fill_memory(ptr, aligned_size, debug_alloc_pattern_);
    }
    
    // Update offset
    current_offset_ = aligned_offset + aligned_size;
    peak_offset_ = std::max(peak_offset_, current_offset_);
    
    // Record allocation for tracking
    if (enable_tracking_) {
        record_allocation(ptr, aligned_size, alignment, category, file, line, function);
    }
    
    // Update statistics
    stats_.used_size = current_offset_;
    stats_.peak_usage = peak_offset_;
    stats_.wasted_bytes += wasted;
    stats_.allocation_count++;
    stats_.active_allocations++;
    
    f64 alloc_time = alloc_timer.elapsed_microseconds();
    stats_.total_alloc_time += alloc_time;
    stats_.average_alloc_time = stats_.total_alloc_time / stats_.allocation_count;
    
    return ptr;
}

void ArenaAllocator::reset() {
    // Mark all allocations as inactive
    if (enable_tracking_) {
        std::lock_guard<std::mutex> lock(tracking_mutex_);
        for (auto& alloc : allocations_) {
            alloc.active = false;
        }
    }
    
    // Fill memory with debug pattern
    if (enable_debug_fill_) {
        fill_memory(memory_, current_offset_, debug_free_pattern_);
    }
    
    // Reset offset
    current_offset_ = 0;
    stats_.used_size = 0;
    stats_.active_allocations = 0;
    
    update_stats();
    
    LOG_DEBUG("Arena '" + name_ + "' reset");
}

void ArenaAllocator::clear() {
    reset();
    
    // Clear tracking data
    if (enable_tracking_) {
        std::lock_guard<std::mutex> lock(tracking_mutex_);
        allocations_.clear();
    }
    
    // Reset statistics (but keep peak usage for analysis)
    stats_.allocation_count = 0;
    stats_.wasted_bytes = 0;
    stats_.total_alloc_time = 0.0;
    stats_.average_alloc_time = 0.0;
    
    LOG_DEBUG("Arena '" + name_ + "' cleared");
}

bool ArenaAllocator::owns(const void* ptr) const noexcept {
    if (!ptr || !memory_) return false;
    
    const byte* byte_ptr = static_cast<const byte*>(ptr);
    return byte_ptr >= memory_ && byte_ptr < (memory_ + total_size_);
}

usize ArenaAllocator::get_allocation_size(const void* ptr) const {
    if (!enable_tracking_ || !owns(ptr)) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    auto it = std::find_if(allocations_.begin(), allocations_.end(),
        [ptr](const AllocationInfo& info) {
            return info.ptr == ptr && info.active;
        });
    
    return it != allocations_.end() ? it->size : 0;
}

void ArenaAllocator::update_stats() {
    stats_.efficiency_ratio = total_size_ > 0 ? static_cast<f64>(current_offset_) / total_size_ : 0.0;
    
    // Simple fragmentation calculation
    // In a linear arena, fragmentation is primarily from alignment waste
    stats_.fragmentation_ratio = current_offset_ > 0 ? 
        static_cast<f64>(stats_.wasted_bytes) / current_offset_ : 0.0;
    
    update_fragmentation_stats();
}

void ArenaAllocator::set_debug_patterns(byte alloc_pattern, byte free_pattern) {
    debug_alloc_pattern_ = alloc_pattern;
    debug_free_pattern_ = free_pattern;
}

std::vector<ArenaAllocator::MemoryRegion> ArenaAllocator::get_memory_layout() const {
    std::vector<MemoryRegion> regions;
    
    if (!enable_tracking_) {
        // Without tracking, we can only show used vs unused
        if (current_offset_ > 0) {
            regions.push_back({0, current_offset_, true, "Used", 0.0});
        }
        if (current_offset_ < total_size_) {
            regions.push_back({current_offset_, total_size_ - current_offset_, false, "Free", 0.0});
        }
        return regions;
    }
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    
    // Sort allocations by offset for layout visualization
    std::vector<AllocationInfo> sorted_allocs = allocations_;
    std::sort(sorted_allocs.begin(), sorted_allocs.end(),
        [this](const AllocationInfo& a, const AllocationInfo& b) {
            usize offset_a = static_cast<const byte*>(a.ptr) - memory_;
            usize offset_b = static_cast<const byte*>(b.ptr) - memory_;
            return offset_a < offset_b;
        });
    
    f64 current_time = core::get_time_seconds();
    usize last_end = 0;
    
    for (const auto& alloc : sorted_allocs) {
        if (!alloc.active) continue;
        
        usize offset = static_cast<const byte*>(alloc.ptr) - memory_;
        
        // Add gap if there's space between allocations
        if (offset > last_end) {
            regions.push_back({last_end, offset - last_end, false, "Gap", 0.0});
        }
        
        // Add the allocation
        f64 age = current_time - alloc.timestamp;
        regions.push_back({offset, alloc.size, true, alloc.category ? alloc.category : "Unknown", age});
        
        last_end = offset + alloc.size;
    }
    
    // Add remaining free space
    if (last_end < total_size_) {
        regions.push_back({last_end, total_size_ - last_end, false, "Free", 0.0});
    }
    
    return regions;
}

std::vector<AllocationInfo> ArenaAllocator::get_active_allocations() const {
    std::vector<AllocationInfo> active_allocs;
    
    if (!enable_tracking_) return active_allocs;
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    std::copy_if(allocations_.begin(), allocations_.end(), 
                std::back_inserter(active_allocs),
                [](const AllocationInfo& info) { return info.active; });
    
    return active_allocs;
}

std::vector<AllocationInfo> ArenaAllocator::get_all_allocations() const {
    if (!enable_tracking_) return {};
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    return allocations_;
}

void* ArenaAllocator::try_allocate(usize size, usize alignment) {
    if (!can_allocate(size, alignment)) {
        return nullptr;
    }
    return allocate(size, alignment);
}

bool ArenaAllocator::can_allocate(usize size, usize alignment) const noexcept {
    if (size == 0) return true;
    
    usize aligned_offset = align_size(current_offset_, alignment);
    usize aligned_size = align_size(size, alignment);
    
    return aligned_offset + aligned_size <= total_size_;
}

ArenaAllocator::Checkpoint ArenaAllocator::create_checkpoint() const {
    return Checkpoint{
        current_offset_,
        stats_.allocation_count,
        core::get_time_seconds()
    };
}

void ArenaAllocator::restore_checkpoint(const Checkpoint& checkpoint) {
    // This is a simplified implementation
    // In a full implementation, you'd need to track which allocations to invalidate
    
    if (checkpoint.offset <= current_offset_) {
        // Mark allocations after checkpoint as inactive
        if (enable_tracking_) {
            std::lock_guard<std::mutex> lock(tracking_mutex_);
            for (auto& alloc : allocations_) {
                usize alloc_offset = static_cast<const byte*>(alloc.ptr) - memory_;
                if (alloc_offset >= checkpoint.offset) {
                    alloc.active = false;
                    if (stats_.active_allocations > 0) {
                        stats_.active_allocations--;
                    }
                }
            }
        }
        
        // Fill freed memory with debug pattern
        if (enable_debug_fill_ && checkpoint.offset < current_offset_) {
            fill_memory(memory_ + checkpoint.offset, 
                       current_offset_ - checkpoint.offset, 
                       debug_free_pattern_);
        }
        
        current_offset_ = checkpoint.offset;
        stats_.used_size = current_offset_;
        update_stats();
        
        LOG_DEBUG("Arena '" + name_ + "' restored to checkpoint");
    }
}

void ArenaAllocator::initialize_memory() {
    // Allocate aligned memory
    memory_ = static_cast<byte*>(std::aligned_alloc(64, total_size_)); // 64-byte aligned for cache lines
    
    if (!memory_) {
        throw std::bad_alloc();
    }
    
    // Fill with debug pattern
    if (enable_debug_fill_) {
        fill_memory(memory_, total_size_, debug_free_pattern_);
    }
}

void ArenaAllocator::cleanup_memory() {
    if (memory_) {
        std::free(memory_);
        memory_ = nullptr;
    }
}

usize ArenaAllocator::align_size(usize size, usize alignment) const noexcept {
    return (size + alignment - 1) & ~(alignment - 1);
}

void ArenaAllocator::record_allocation(void* ptr, usize size, usize alignment, const char* category, 
                                      const char* file, int line, const char* function) {
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    
    allocations_.push_back(AllocationInfo{
        ptr,
        size,
        alignment,
        category,
        core::get_time_seconds(),
        true,
        file,
        line,
        function
    });
}

void ArenaAllocator::fill_memory(void* ptr, usize size, byte pattern) {
    std::memset(ptr, pattern, size);
}

void ArenaAllocator::update_fragmentation_stats() {
    // Estimate cache misses based on allocation patterns
    // This is a simplified heuristic
    
    if (!enable_tracking_ || allocations_.empty()) {
        stats_.cache_misses = 0;
        return;
    }
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    
    // Count allocations that likely cause cache misses
    // (allocations that cross cache line boundaries)
    constexpr usize CACHE_LINE_SIZE = 64;
    u64 potential_misses = 0;
    
    for (const auto& alloc : allocations_) {
        if (!alloc.active) continue;
        
        usize start_line = reinterpret_cast<usize>(alloc.ptr) / CACHE_LINE_SIZE;
        usize end_line = (reinterpret_cast<usize>(alloc.ptr) + alloc.size - 1) / CACHE_LINE_SIZE;
        
        potential_misses += (end_line - start_line + 1);
    }
    
    stats_.cache_misses = potential_misses;
}

// Global arena registry implementation
namespace arena_registry {
    
    void register_arena(ArenaAllocator* arena) {
        if (!arena) return;
        
        std::lock_guard<std::mutex> lock(g_registry_mutex);
        auto it = std::find(g_registered_arenas.begin(), g_registered_arenas.end(), arena);
        if (it == g_registered_arenas.end()) {
            g_registered_arenas.push_back(arena);
        }
    }
    
    void unregister_arena(ArenaAllocator* arena) {
        if (!arena) return;
        
        std::lock_guard<std::mutex> lock(g_registry_mutex);
        auto it = std::find(g_registered_arenas.begin(), g_registered_arenas.end(), arena);
        if (it != g_registered_arenas.end()) {
            g_registered_arenas.erase(it);
        }
    }
    
    std::vector<ArenaAllocator*> get_all_arenas() {
        std::lock_guard<std::mutex> lock(g_registry_mutex);
        return g_registered_arenas;
    }
    
    ArenaStats get_combined_stats() {
        ArenaStats combined{};
        
        std::lock_guard<std::mutex> lock(g_registry_mutex);
        for (const auto* arena : g_registered_arenas) {
            if (!arena) continue;
            
            const auto& stats = arena->stats();
            combined.total_size += stats.total_size;
            combined.used_size += stats.used_size;
            combined.peak_usage += stats.peak_usage;
            combined.wasted_bytes += stats.wasted_bytes;
            combined.allocation_count += stats.allocation_count;
            combined.active_allocations += stats.active_allocations;
            combined.total_alloc_time += stats.total_alloc_time;
            combined.cache_misses += stats.cache_misses;
        }
        
        if (!g_registered_arenas.empty()) {
            combined.average_alloc_time = combined.total_alloc_time / combined.allocation_count;
            combined.efficiency_ratio = combined.total_size > 0 ? 
                static_cast<f64>(combined.used_size) / combined.total_size : 0.0;
            combined.fragmentation_ratio = combined.used_size > 0 ?
                static_cast<f64>(combined.wasted_bytes) / combined.used_size : 0.0;
        }
        
        return combined;
    }
    
} // namespace arena_registry

} // namespace ecscope::memory