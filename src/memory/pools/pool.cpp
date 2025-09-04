#include "pool.hpp"
#include "core/time.hpp"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <sstream>
#include <iomanip>

namespace ecscope::memory {

// Global pool registry
static std::vector<PoolAllocator*> g_registered_pools;
static std::mutex g_pool_registry_mutex;

// ===========================================
// PoolChunk Implementation
// ===========================================

PoolChunk::PoolChunk(byte* mem, usize block_sz, usize block_cnt, bool owns_mem) noexcept
    : memory(mem)
    , block_size(block_sz)
    , block_count(block_cnt)
    , blocks_allocated(0)
    , free_head(nullptr)
    , owns_memory(owns_mem)
    , creation_time(core::get_time_seconds()) {
    
    if (memory && block_size > 0 && block_count > 0) {
        initialize_free_list();
    }
}

PoolChunk::~PoolChunk() {
    if (owns_memory && memory) {
        std::free(memory);
        memory = nullptr;
    }
}

PoolChunk::PoolChunk(PoolChunk&& other) noexcept
    : memory(other.memory)
    , block_size(other.block_size)
    , block_count(other.block_count)
    , blocks_allocated(other.blocks_allocated)
    , free_head(other.free_head)
    , owns_memory(other.owns_memory)
    , creation_time(other.creation_time) {
    
    // Nullify moved-from object
    other.memory = nullptr;
    other.block_size = 0;
    other.block_count = 0;
    other.blocks_allocated = 0;
    other.free_head = nullptr;
    other.owns_memory = false;
    other.creation_time = 0.0;
}

PoolChunk& PoolChunk::operator=(PoolChunk&& other) noexcept {
    if (this != &other) {
        // Clean up current state
        if (owns_memory && memory) {
            std::free(memory);
        }
        
        // Move from other
        memory = other.memory;
        block_size = other.block_size;
        block_count = other.block_count;
        blocks_allocated = other.blocks_allocated;
        free_head = other.free_head;
        owns_memory = other.owns_memory;
        creation_time = other.creation_time;
        
        // Nullify moved-from object
        other.memory = nullptr;
        other.block_size = 0;
        other.block_count = 0;
        other.blocks_allocated = 0;
        other.free_head = nullptr;
        other.owns_memory = false;
        other.creation_time = 0.0;
    }
    return *this;
}

void PoolChunk::initialize_free_list() noexcept {
    if (!memory || block_size == 0 || block_count == 0) {
        free_head = nullptr;
        return;
    }
    
    // Initialize the free list by linking all blocks together
    // Each block stores a pointer to the next free block at its beginning
    FreeBlock* current = nullptr;
    FreeBlock* prev = nullptr;
    
    // Start from the last block and work backwards to maintain order
    for (usize i = 0; i < block_count; ++i) {
        current = reinterpret_cast<FreeBlock*>(memory + i * block_size);
        current->next = prev;
        prev = current;
    }
    
    free_head = current; // Points to first block
    blocks_allocated = 0;
}

bool PoolChunk::contains(const void* ptr) const noexcept {
    if (!ptr || !memory) return false;
    
    const byte* byte_ptr = static_cast<const byte*>(ptr);
    const byte* chunk_end = memory + (block_count * block_size);
    
    return byte_ptr >= memory && byte_ptr < chunk_end;
}

usize PoolChunk::get_block_index(const void* ptr) const noexcept {
    if (!contains(ptr) || block_size == 0) {
        return SIZE_MAX; // Invalid index
    }
    
    const byte* byte_ptr = static_cast<const byte*>(ptr);
    usize offset = byte_ptr - memory;
    return offset / block_size;
}

// ===========================================
// PoolAllocator Implementation
// ===========================================

PoolAllocator::PoolAllocator(
    usize block_size,
    usize initial_capacity,
    usize alignment,
    const std::string& name,
    bool enable_tracking)
    : block_size_(std::max(block_size, sizeof(void*))) // Ensure minimum size for free list
    , alignment_(alignment)
    , initial_capacity_(initial_capacity)
    , max_chunks_(0) // Unlimited by default
    , allow_expansion_(true)
    , free_head_(nullptr)
    , total_free_blocks_(0)
    , enable_tracking_(enable_tracking)
    , enable_debug_fill_(true)
    , enable_thread_safety_(false)
    , debug_alloc_pattern_(0xAB) // "Allocated Block" pattern
    , debug_free_pattern_(0xFE)  // "Freed Element" pattern
    , name_(name)
    , type_hash_(0)
    , last_stats_update_(0.0)
    , timing_index_(0) {
    
    // Ensure alignment is power of 2
    if (alignment_ == 0 || (alignment_ & (alignment_ - 1)) != 0) {
        alignment_ = alignof(std::max_align_t);
    }
    
    // Align block size to alignment requirement
    block_size_ = (block_size_ + alignment_ - 1) & ~(alignment_ - 1);
    
    // Initialize statistics
    stats_.reset();
    stats_.block_size = block_size_;
    
    // Initialize timing buffer
    recent_alloc_times_.fill(0.0);
    
    // Create initial chunk
    initialize_pool();
    
    // Register with global registry
    pool_registry::register_pool(this);
    
    LOG_INFO("Pool '" + name_ + "' created: block_size=" + std::to_string(block_size_) + 
             ", capacity=" + std::to_string(initial_capacity_) + 
             ", alignment=" + std::to_string(alignment_));
}

PoolAllocator::~PoolAllocator() {
    pool_registry::unregister_pool(this);
    cleanup_pool();
    
    LOG_INFO("Pool '" + name_ + "' destroyed - lifetime stats: " +
             std::to_string(stats_.total_allocations) + " allocations, " +
             std::to_string(stats_.peak_allocated) + " peak usage");
}

PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
    : block_size_(other.block_size_)
    , alignment_(other.alignment_)
    , initial_capacity_(other.initial_capacity_)
    , max_chunks_(other.max_chunks_)
    , allow_expansion_(other.allow_expansion_)
    , chunks_(std::move(other.chunks_))
    , free_head_(other.free_head_)
    , total_free_blocks_(other.total_free_blocks_)
    , allocated_blocks_(std::move(other.allocated_blocks_))
    , allocations_(std::move(other.allocations_))
    , stats_(other.stats_)
    , enable_tracking_(other.enable_tracking_)
    , enable_debug_fill_(other.enable_debug_fill_)
    , enable_thread_safety_(other.enable_thread_safety_)
    , debug_alloc_pattern_(other.debug_alloc_pattern_)
    , debug_free_pattern_(other.debug_free_pattern_)
    , name_(std::move(other.name_))
    , type_hash_(other.type_hash_)
    , last_stats_update_(other.last_stats_update_)
    , recent_alloc_times_(other.recent_alloc_times_)
    , timing_index_(other.timing_index_) {
    
    // Transfer atomic counter
    allocation_counter_.store(other.allocation_counter_.load());
    
    // Nullify moved-from object
    other.free_head_ = nullptr;
    other.total_free_blocks_ = 0;
    other.allocation_counter_.store(0);
    
    // Update registry
    pool_registry::unregister_pool(&other);
    pool_registry::register_pool(this);
}

PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept {
    if (this != &other) {
        // Clean up current state
        pool_registry::unregister_pool(this);
        cleanup_pool();
        
        // Move from other
        block_size_ = other.block_size_;
        alignment_ = other.alignment_;
        initial_capacity_ = other.initial_capacity_;
        max_chunks_ = other.max_chunks_;
        allow_expansion_ = other.allow_expansion_;
        chunks_ = std::move(other.chunks_);
        free_head_ = other.free_head_;
        total_free_blocks_ = other.total_free_blocks_;
        allocated_blocks_ = std::move(other.allocated_blocks_);
        allocations_ = std::move(other.allocations_);
        stats_ = other.stats_;
        enable_tracking_ = other.enable_tracking_;
        enable_debug_fill_ = other.enable_debug_fill_;
        enable_thread_safety_ = other.enable_thread_safety_;
        debug_alloc_pattern_ = other.debug_alloc_pattern_;
        debug_free_pattern_ = other.debug_free_pattern_;
        name_ = std::move(other.name_);
        type_hash_ = other.type_hash_;
        last_stats_update_ = other.last_stats_update_;
        recent_alloc_times_ = other.recent_alloc_times_;
        timing_index_ = other.timing_index_;
        
        // Transfer atomic counter
        allocation_counter_.store(other.allocation_counter_.load());
        
        // Nullify moved-from object
        other.free_head_ = nullptr;
        other.total_free_blocks_ = 0;
        other.allocation_counter_.store(0);
        
        // Update registry
        pool_registry::unregister_pool(&other);
        pool_registry::register_pool(this);
    }
    return *this;
}

// ===========================================
// Core Allocation Interface
// ===========================================

void* PoolAllocator::allocate(const char* category) {
    return allocate_debug(category, nullptr, 0, nullptr);
}

void* PoolAllocator::allocate_debug(const char* category, const char* file, int line, const char* function) {
    core::Timer alloc_timer;
    
    return with_lock([&]() -> void* {
        // Check if we have any free blocks available
        if (free_head_ == nullptr) {
            // Try to expand the pool if allowed
            if (allow_expansion_) {
                if (!expand_pool()) {
                    LOG_WARN("Pool '" + name_ + "' allocation failed: no free blocks and expansion failed");
                    return nullptr;
                }
            } else {
                LOG_WARN("Pool '" + name_ + "' allocation failed: no free blocks and expansion disabled");
                return nullptr;
            }
        }
        
        // Get block from free list (O(1) operation)
        void* ptr = free_head_;
        FreeBlock* free_block = static_cast<FreeBlock*>(ptr);
        free_head_ = free_block->next;
        total_free_blocks_--;
        
        // Fill with debug pattern if enabled
        if (enable_debug_fill_) {
            poison_block(ptr, true);
        }
        
        // Record allocation for tracking
        if (enable_tracking_) {
            record_allocation(ptr, category, file, line, function);
        }
        
        // Update statistics
        stats_.total_allocated++;
        stats_.total_allocations++;
        stats_.peak_allocated = std::max(stats_.peak_allocated, stats_.total_allocated);
        
        // Update the chunk's allocation count
        PoolChunk* chunk = find_chunk_for_ptr(ptr);
        if (chunk) {
            chunk->blocks_allocated++;
        }
        
        // Record timing
        f64 alloc_time = alloc_timer.elapsed_microseconds();
        stats_.total_alloc_time += alloc_time;
        recent_alloc_times_[timing_index_] = alloc_time * 1000.0; // Convert to nanoseconds
        timing_index_ = (timing_index_ + 1) % recent_alloc_times_.size();
        
        // Update performance metrics
        allocation_counter_.fetch_add(1);
        estimate_cache_behavior(ptr);
        
        return ptr;
    });
}

void PoolAllocator::deallocate(void* ptr) {
    if (!ptr) return;
    
    core::Timer dealloc_timer;
    
    with_lock([&]() {
        // Validate that pointer belongs to this pool
        if (!owns(ptr)) {
            LOG_ERROR("Pool '" + name_ + "' deallocate: pointer " + 
                     std::to_string(reinterpret_cast<uintptr_t>(ptr)) + " does not belong to this pool");
            return;
        }
        
        // Validate alignment
        if (!is_aligned(ptr)) {
            LOG_ERROR("Pool '" + name_ + "' deallocate: pointer " + 
                     std::to_string(reinterpret_cast<uintptr_t>(ptr)) + " is not properly aligned");
            return;
        }
        
        // Record deallocation for tracking
        if (enable_tracking_) {
            record_deallocation(ptr);
        }
        
        // Fill with debug pattern before returning to free list
        if (enable_debug_fill_) {
            poison_block(ptr, false);
        }
        
        // Add block back to free list (O(1) operation)
        FreeBlock* free_block = static_cast<FreeBlock*>(ptr);
        free_block->next = free_head_;
        free_head_ = free_block;
        total_free_blocks_++;
        
        // Update statistics
        stats_.total_allocated--;
        stats_.total_deallocations++;
        
        // Update the chunk's allocation count
        PoolChunk* chunk = find_chunk_for_ptr(ptr);
        if (chunk && chunk->blocks_allocated > 0) {
            chunk->blocks_allocated--;
        }
        
        // Record timing
        f64 dealloc_time = dealloc_timer.elapsed_microseconds();
        stats_.total_dealloc_time += dealloc_time;
    });
}

void* PoolAllocator::try_allocate(const char* category) {
    if (free_head_ == nullptr) {
        return nullptr; // No free blocks available
    }
    return allocate(category);
}

// ===========================================
// Pool Management
// ===========================================

bool PoolAllocator::expand_pool(usize capacity) {
    // Check if we've reached the maximum number of chunks
    if (max_chunks_ > 0 && chunks_.size() >= max_chunks_) {
        LOG_WARN("Pool '" + name_ + "' cannot expand: reached maximum chunks limit (" + 
                std::to_string(max_chunks_) + ")");
        return false;
    }
    
    // Use default capacity if not specified
    if (capacity == 0) {
        capacity = initial_capacity_;
    }
    
    // Calculate memory needed for the new chunk
    usize memory_needed = capacity * block_size_;
    
    // Allocate aligned memory
    byte* chunk_memory = static_cast<byte*>(std::aligned_alloc(alignment_, memory_needed));
    if (!chunk_memory) {
        LOG_ERROR("Pool '" + name_ + "' expansion failed: could not allocate " + 
                 std::to_string(memory_needed) + " bytes");
        return false;
    }
    
    // Create and initialize new chunk
    chunks_.emplace_back(chunk_memory, block_size_, capacity, true);
    PoolChunk& new_chunk = chunks_.back();
    
    // Link the new chunk's free list to the global free list
    if (new_chunk.free_head) {
        // Find the end of the new chunk's free list
        FreeBlock* tail = new_chunk.free_head;
        usize count = 1;
        while (tail->next && count < capacity) {
            tail = tail->next;
            count++;
        }
        
        // Connect to existing free list
        tail->next = free_head_;
        free_head_ = new_chunk.free_head;
        total_free_blocks_ += capacity;
        
        // Update statistics
        stats_.total_capacity += capacity;
        stats_.chunk_count++;
        stats_.chunk_expansions++;
        stats_.total_memory_used += memory_needed;
        stats_.overhead_bytes += sizeof(PoolChunk);
        
        LOG_DEBUG("Pool '" + name_ + "' expanded: added chunk with " + 
                 std::to_string(capacity) + " blocks (" + 
                 std::to_string(memory_needed / 1024) + " KB)");
        
        return true;
    }
    
    // If chunk initialization failed, clean up
    chunks_.pop_back();
    return false;
}

usize PoolAllocator::shrink_pool() {
    usize removed_chunks = 0;
    
    with_lock([&]() {
        // Remove empty chunks (except the first one to maintain minimum capacity)
        auto it = chunks_.begin();
        if (it != chunks_.end()) ++it; // Skip first chunk
        
        while (it != chunks_.end()) {
            if (it->is_empty()) {
                // Remove blocks from global free list
                // This is expensive but shrinking should be rare
                rebuild_free_list();
                
                // Update statistics
                stats_.total_capacity -= it->block_count;
                stats_.chunk_count--;
                stats_.total_memory_used -= (it->block_count * block_size_);
                stats_.overhead_bytes -= sizeof(PoolChunk);
                
                LOG_DEBUG("Pool '" + name_ + "' shrunk: removed empty chunk with " + 
                         std::to_string(it->block_count) + " blocks");
                
                it = chunks_.erase(it);
                removed_chunks++;
            } else {
                ++it;
            }
        }
    });
    
    return removed_chunks;
}

void PoolAllocator::reset() {
    with_lock([&]() {
        // Mark all allocations as inactive
        if (enable_tracking_) {
            std::lock_guard<std::mutex> lock(tracking_mutex_);
            for (auto& alloc : allocations_) {
                alloc.active = false;
            }
            allocated_blocks_.clear();
        }
        
        // Rebuild free lists for all chunks
        free_head_ = nullptr;
        total_free_blocks_ = 0;
        
        for (auto& chunk : chunks_) {
            // Fill chunk memory with debug pattern
            if (enable_debug_fill_) {
                fill_memory(chunk.memory, chunk.block_count * chunk.block_size, debug_free_pattern_);
            }
            
            // Reinitialize chunk's free list
            chunk.initialize_free_list();
            
            // Link to global free list
            if (chunk.free_head) {
                // Find tail of chunk's free list
                FreeBlock* tail = chunk.free_head;
                usize count = 1;
                while (tail->next && count < chunk.block_count) {
                    tail = tail->next;
                    count++;
                }
                
                // Connect to global free list
                tail->next = free_head_;
                free_head_ = chunk.free_head;
                total_free_blocks_ += chunk.block_count;
            }
            
            chunk.blocks_allocated = 0;
        }
        
        // Reset statistics
        stats_.total_allocated = 0;
        
        LOG_DEBUG("Pool '" + name_ + "' reset: all blocks returned to free list");
    });
}

void PoolAllocator::clear() {
    reset();
    
    // Clear tracking data
    if (enable_tracking_) {
        std::lock_guard<std::mutex> lock(tracking_mutex_);
        allocations_.clear();
        allocated_blocks_.clear();
    }
    
    // Reset cumulative statistics (but keep configuration)
    usize total_capacity = stats_.total_capacity;
    usize block_size = stats_.block_size;
    usize chunk_count = stats_.chunk_count;
    usize total_memory_used = stats_.total_memory_used;
    usize overhead_bytes = stats_.overhead_bytes;
    
    stats_.reset();
    stats_.total_capacity = total_capacity;
    stats_.block_size = block_size;
    stats_.chunk_count = chunk_count;
    stats_.total_memory_used = total_memory_used;
    stats_.overhead_bytes = overhead_bytes;
    
    // Reset timing data
    recent_alloc_times_.fill(0.0);
    timing_index_ = 0;
    allocation_counter_.store(0);
    
    LOG_DEBUG("Pool '" + name_ + "' cleared: all tracking data and statistics reset");
}

// ===========================================
// Ownership and Validation
// ===========================================

bool PoolAllocator::owns(const void* ptr) const noexcept {
    if (!ptr) return false;
    
    // Check each chunk to see if it contains the pointer
    for (const auto& chunk : chunks_) {
        if (chunk.contains(ptr)) {
            // Additional validation: ensure pointer is at block boundary
            const byte* byte_ptr = static_cast<const byte*>(ptr);
            usize offset = byte_ptr - chunk.memory;
            return (offset % chunk.block_size) == 0;
        }
    }
    
    return false;
}

bool PoolAllocator::is_aligned(const void* ptr) const noexcept {
    if (!ptr) return false;
    
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    return (addr % alignment_) == 0;
}

bool PoolAllocator::is_valid_block(const void* ptr) const noexcept {
    if (!ptr) return false;
    
    // Check if pointer is owned by this pool
    if (!owns(ptr)) return false;
    
    // Check alignment
    if (!is_aligned(ptr)) return false;
    
    // Find the chunk containing this pointer
    PoolChunk* chunk = find_chunk_for_ptr(ptr);
    if (!chunk) return false;
    
    // Calculate block index
    usize block_index = chunk->get_block_index(ptr);
    return block_index < chunk->block_count;
}

// ===========================================
// Statistics and Performance Monitoring
// ===========================================

void PoolAllocator::update_stats() {
    f64 current_time = get_current_time();
    
    // Update timing-based statistics
    if (last_stats_update_ > 0.0) {
        f64 time_delta = current_time - last_stats_update_;
        if (time_delta > 0.0) {
            stats_.allocation_frequency = stats_.total_allocations / time_delta;
            stats_.deallocation_frequency = stats_.total_deallocations / time_delta;
        }
    }
    
    // Calculate average timings
    if (stats_.total_allocations > 0) {
        stats_.average_alloc_time = (stats_.total_alloc_time * 1000.0) / stats_.total_allocations; // Convert to ns
    }
    
    if (stats_.total_deallocations > 0) {
        stats_.average_dealloc_time = (stats_.total_dealloc_time * 1000.0) / stats_.total_deallocations; // Convert to ns
    }
    
    // Update fragmentation analysis
    update_fragmentation_stats();
    
    // Update chunk utilization
    stats_.chunk_count = chunks_.size();
    if (stats_.chunk_count > 0) {
        f64 total_utilization = 0.0;
        usize free_count = 0;
        
        for (const auto& chunk : chunks_) {
            total_utilization += chunk.utilization();
            free_count += (chunk.block_count - chunk.blocks_allocated);
        }
        
        stats_.average_chunk_usage = total_utilization / stats_.chunk_count;
        stats_.free_list_length = free_count;
        stats_.max_free_list_length = std::max(stats_.max_free_list_length, stats_.free_list_length);
    }
    
    // Calculate fragmentation metrics
    if (stats_.total_capacity > 0) {
        stats_.external_fragmentation = static_cast<f64>(stats_.free_list_length) / stats_.total_capacity;
    }
    
    // Update performance metrics
    update_performance_stats();
    
    last_stats_update_ = current_time;
}

void PoolAllocator::reset_stats() {
    stats_.reset();
    stats_.block_size = block_size_;
    stats_.total_capacity = total_capacity();
    stats_.chunk_count = chunks_.size();
    
    // Recalculate memory usage
    stats_.total_memory_used = 0;
    for (const auto& chunk : chunks_) {
        stats_.total_memory_used += chunk.block_count * chunk.block_size;
    }
    stats_.overhead_bytes = chunks_.size() * sizeof(PoolChunk);
    
    // Reset timing data
    recent_alloc_times_.fill(0.0);
    timing_index_ = 0;
    last_stats_update_ = get_current_time();
}

// ===========================================
// Configuration and Introspection
// ===========================================

void PoolAllocator::set_debug_patterns(byte alloc_pattern, byte free_pattern) {
    debug_alloc_pattern_ = alloc_pattern;
    debug_free_pattern_ = free_pattern;
}

usize PoolAllocator::total_capacity() const noexcept {
    usize capacity = 0;
    for (const auto& chunk : chunks_) {
        capacity += chunk.block_count;
    }
    return capacity;
}

usize PoolAllocator::allocated_count() const noexcept {
    return stats_.total_allocated;
}

// ===========================================
// Memory Layout Visualization
// ===========================================

std::vector<PoolAllocator::BlockInfo> PoolAllocator::get_memory_layout() const {
    std::vector<BlockInfo> blocks;
    
    if (!enable_tracking_) {
        // Without tracking, we can only show basic allocation status
        for (usize chunk_idx = 0; chunk_idx < chunks_.size(); ++chunk_idx) {
            const auto& chunk = chunks_[chunk_idx];
            for (usize block_idx = 0; block_idx < chunk.block_count; ++block_idx) {
                void* ptr = chunk.memory + (block_idx * chunk.block_size);
                blocks.push_back(BlockInfo{
                    ptr,
                    block_idx,
                    chunk_idx,
                    false, // We don't know allocation status without tracking
                    nullptr,
                    0.0,
                    0.0
                });
            }
        }
        return blocks;
    }
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    f64 current_time = get_current_time();
    
    // Create map of allocated blocks
    std::unordered_set<void*> allocated_set = allocated_blocks_;
    
    // Generate block info for all blocks
    for (usize chunk_idx = 0; chunk_idx < chunks_.size(); ++chunk_idx) {
        const auto& chunk = chunks_[chunk_idx];
        for (usize block_idx = 0; block_idx < chunk.block_count; ++block_idx) {
            void* ptr = chunk.memory + (block_idx * chunk.block_size);
            bool is_allocated = allocated_set.count(ptr) > 0;
            
            const char* category = nullptr;
            f64 allocation_time = 0.0;
            
            // Find allocation info if available
            if (is_allocated) {
                auto alloc_it = std::find_if(allocations_.begin(), allocations_.end(),
                    [ptr](const AllocationInfo& info) {
                        return info.ptr == ptr && info.active;
                    });
                
                if (alloc_it != allocations_.end()) {
                    category = alloc_it->category;
                    allocation_time = alloc_it->timestamp;
                }
            }
            
            f64 age = allocation_time > 0.0 ? current_time - allocation_time : 0.0;
            
            blocks.push_back(BlockInfo{
                ptr,
                block_idx,
                chunk_idx,
                is_allocated,
                category,
                allocation_time,
                age
            });
        }
    }
    
    return blocks;
}

PoolAllocator::FreeListInfo PoolAllocator::get_free_list_info() const {
    FreeListInfo info;
    info.total_free = total_free_blocks_;
    info.max_contiguous_free = 0;
    info.fragmentation_score = 0.0;
    info.free_chunks.resize(chunks_.size(), 0);
    
    // Walk the free list to gather detailed information
    FreeBlock* current = free_head_;
    usize free_count = 0;
    
    while (current && free_count < total_free_blocks_) {
        info.free_blocks.push_back(current);
        
        // Find which chunk this block belongs to
        for (usize chunk_idx = 0; chunk_idx < chunks_.size(); ++chunk_idx) {
            if (chunks_[chunk_idx].contains(current)) {
                info.free_chunks[chunk_idx]++;
                break;
            }
        }
        
        current = current->next;
        free_count++;
    }
    
    // Calculate fragmentation score based on free block distribution
    if (!chunks_.empty()) {
        f64 ideal_distribution = static_cast<f64>(total_free_blocks_) / chunks_.size();
        f64 deviation_sum = 0.0;
        
        for (usize count : info.free_chunks) {
            f64 deviation = std::abs(static_cast<f64>(count) - ideal_distribution);
            deviation_sum += deviation * deviation;
        }
        
        info.fragmentation_score = std::sqrt(deviation_sum / chunks_.size()) / ideal_distribution;
    }
    
    return info;
}

std::vector<PoolAllocator::ChunkInfo> PoolAllocator::get_chunk_info() const {
    std::vector<ChunkInfo> chunk_infos;
    f64 current_time = get_current_time();
    
    for (const auto& chunk : chunks_) {
        ChunkInfo info;
        info.base_address = chunk.memory;
        info.block_count = chunk.block_count;
        info.allocated_blocks = chunk.blocks_allocated;
        info.utilization = chunk.utilization();
        info.creation_time = chunk.creation_time;
        info.age = current_time - chunk.creation_time;
        info.can_be_freed = chunk.is_empty() && chunks_.size() > 1; // Keep at least one chunk
        
        chunk_infos.push_back(info);
    }
    
    return chunk_infos;
}

// ===========================================
// Allocation History and Debugging
// ===========================================

std::vector<AllocationInfo> PoolAllocator::get_active_allocations() const {
    std::vector<AllocationInfo> active_allocs;
    
    if (!enable_tracking_) return active_allocs;
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    std::copy_if(allocations_.begin(), allocations_.end(),
                std::back_inserter(active_allocs),
                [](const AllocationInfo& info) { return info.active; });
    
    return active_allocs;
}

std::vector<AllocationInfo> PoolAllocator::get_all_allocations() const {
    if (!enable_tracking_) return {};
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    return allocations_;
}

const AllocationInfo* PoolAllocator::find_allocation_info(const void* ptr) const {
    if (!enable_tracking_ || !ptr) return nullptr;
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    auto it = std::find_if(allocations_.begin(), allocations_.end(),
        [ptr](const AllocationInfo& info) {
            return info.ptr == ptr && info.active;
        });
    
    return it != allocations_.end() ? &(*it) : nullptr;
}

bool PoolAllocator::validate_integrity() const {
    try {
        // Check basic pool state
        if (chunks_.empty()) {
            LOG_ERROR("Pool '" + name_ + "' integrity check failed: no chunks");
            return false;
        }
        
        // Validate each chunk
        usize total_blocks = 0;
        usize total_allocated = 0;
        
        for (const auto& chunk : chunks_) {
            // Check chunk memory
            if (!chunk.memory) {
                LOG_ERROR("Pool '" + name_ + "' integrity check failed: chunk has null memory");
                return false;
            }
            
            // Check chunk parameters
            if (chunk.block_size != block_size_) {
                LOG_ERROR("Pool '" + name_ + "' integrity check failed: chunk block size mismatch");
                return false;
            }
            
            if (chunk.blocks_allocated > chunk.block_count) {
                LOG_ERROR("Pool '" + name_ + "' integrity check failed: chunk over-allocated");
                return false;
            }
            
            total_blocks += chunk.block_count;
            total_allocated += chunk.blocks_allocated;
        }
        
        // Check free list integrity
        usize free_list_count = 0;
        FreeBlock* current = free_head_;
        std::unordered_set<void*> visited;
        
        while (current && free_list_count < total_blocks) {
            // Check for cycles
            if (visited.count(current) > 0) {
                LOG_ERROR("Pool '" + name_ + "' integrity check failed: cycle detected in free list");
                return false;
            }
            visited.insert(current);
            
            // Check that free block belongs to a chunk
            bool found_chunk = false;
            for (const auto& chunk : chunks_) {
                if (chunk.contains(current)) {
                    found_chunk = true;
                    break;
                }
            }
            
            if (!found_chunk) {
                LOG_ERROR("Pool '" + name_ + "' integrity check failed: free block not in any chunk");
                return false;
            }
            
            current = current->next;
            free_list_count++;
        }
        
        // Check free list count matches expected
        if (free_list_count != total_free_blocks_) {
            LOG_ERROR("Pool '" + name_ + "' integrity check failed: free list count mismatch");
            return false;
        }
        
        // Check allocation count consistency
        if (total_allocated != stats_.total_allocated) {
            LOG_ERROR("Pool '" + name_ + "' integrity check failed: allocated count mismatch");
            return false;
        }
        
        // Check total capacity
        if (total_blocks != stats_.total_capacity) {
            LOG_ERROR("Pool '" + name_ + "' integrity check failed: capacity mismatch");
            return false;
        }
        
        LOG_DEBUG("Pool '" + name_ + "' integrity check passed");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Pool '" + name_ + "' integrity check failed with exception: " + std::string(e.what()));
        return false;
    }
}

std::string PoolAllocator::generate_diagnostic_report() const {
    std::stringstream report;
    f64 current_time = get_current_time();
    
    // Header
    report << "=== Pool Allocator Diagnostic Report ===\n";
    report << "Pool Name: " << name_ << "\n";
    report << "Block Size: " << block_size_ << " bytes\n";
    report << "Alignment: " << alignment_ << " bytes\n";
    report << "Report Time: " << std::fixed << std::setprecision(3) << current_time << "\n\n";
    
    // Configuration
    report << "--- Configuration ---\n";
    report << "Initial Capacity: " << initial_capacity_ << " blocks\n";
    report << "Max Chunks: " << (max_chunks_ == 0 ? "Unlimited" : std::to_string(max_chunks_)) << "\n";
    report << "Allow Expansion: " << (allow_expansion_ ? "Yes" : "No") << "\n";
    report << "Tracking Enabled: " << (enable_tracking_ ? "Yes" : "No") << "\n";
    report << "Thread Safety: " << (enable_thread_safety_ ? "Yes" : "No") << "\n";
    report << "Debug Fill: " << (enable_debug_fill_ ? "Yes" : "No") << "\n\n";
    
    // Current State
    report << "--- Current State ---\n";
    report << "Total Chunks: " << chunks_.size() << "\n";
    report << "Total Capacity: " << stats_.total_capacity << " blocks\n";
    report << "Allocated Blocks: " << stats_.total_allocated << "\n";
    report << "Free Blocks: " << total_free_blocks_ << "\n";
    report << "Utilization: " << std::fixed << std::setprecision(2) << (utilization_ratio() * 100.0) << "%\n\n";
    
    // Memory Usage
    report << "--- Memory Usage ---\n";
    report << "Total Memory: " << (stats_.total_memory_used / 1024.0) << " KB\n";
    report << "Overhead: " << (stats_.overhead_bytes / 1024.0) << " KB (" 
           << std::fixed << std::setprecision(1) << (stats_.overhead_ratio() * 100.0) << "%)\n";
    report << "Wasted Bytes: " << stats_.wasted_bytes << " (" 
           << std::fixed << std::setprecision(1) << (stats_.internal_fragmentation * 100.0) << "%)\n\n";
    
    // Performance Metrics
    report << "--- Performance Metrics ---\n";
    report << "Total Allocations: " << stats_.total_allocations << "\n";
    report << "Total Deallocations: " << stats_.total_deallocations << "\n";
    report << "Peak Allocated: " << stats_.peak_allocated << " blocks\n";
    report << "Chunk Expansions: " << stats_.chunk_expansions << "\n";
    report << "Average Alloc Time: " << std::fixed << std::setprecision(1) << stats_.average_alloc_time << " ns\n";
    report << "Average Dealloc Time: " << std::fixed << std::setprecision(1) << stats_.average_dealloc_time << " ns\n";
    report << "Cache Misses (Est.): " << stats_.cache_misses_estimated << "\n\n";
    
    // Fragmentation Analysis
    report << "--- Fragmentation Analysis ---\n";
    report << "External Fragmentation: " << std::fixed << std::setprecision(1) << (stats_.external_fragmentation * 100.0) << "%\n";
    report << "Internal Fragmentation: " << std::fixed << std::setprecision(1) << (stats_.internal_fragmentation * 100.0) << "%\n";
    report << "Average Chunk Usage: " << std::fixed << std::setprecision(1) << (stats_.average_chunk_usage * 100.0) << "%\n";
    report << "Free List Length: " << stats_.free_list_length << "\n";
    report << "Max Free List Length: " << stats_.max_free_list_length << "\n\n";
    
    // Chunk Details
    report << "--- Chunk Details ---\n";
    for (usize i = 0; i < chunks_.size(); ++i) {
        const auto& chunk = chunks_[i];
        report << "Chunk " << i << ": " << chunk.blocks_allocated << "/" << chunk.block_count 
               << " blocks (" << std::fixed << std::setprecision(1) << (chunk.utilization() * 100.0) 
               << "%) - Age: " << std::fixed << std::setprecision(1) << (current_time - chunk.creation_time) 
               << "s\n";
    }
    
    // Integrity Check
    report << "\n--- Integrity Check ---\n";
    bool integrity_ok = const_cast<PoolAllocator*>(this)->validate_integrity();
    report << "Status: " << (integrity_ok ? "PASSED" : "FAILED") << "\n";
    
    return report.str();
}

// ===========================================
// Private Implementation Methods
// ===========================================

void PoolAllocator::initialize_pool() {
    if (initial_capacity_ == 0) {
        LOG_WARN("Pool '" + name_ + "' initialized with zero capacity");
        return;
    }
    
    // Create the initial chunk
    expand_pool(initial_capacity_);
    
    LOG_DEBUG("Pool '" + name_ + "' initialized with " + std::to_string(initial_capacity_) + " blocks");
}

void PoolAllocator::cleanup_pool() {
    // Chunks will be automatically destroyed by their destructors
    chunks_.clear();
    free_head_ = nullptr;
    total_free_blocks_ = 0;
    
    if (enable_tracking_) {
        std::lock_guard<std::mutex> lock(tracking_mutex_);
        allocated_blocks_.clear();
        allocations_.clear();
    }
}

PoolChunk* PoolAllocator::find_chunk_for_ptr(const void* ptr) const noexcept {
    for (const auto& chunk : chunks_) {
        if (chunk.contains(ptr)) {
            return const_cast<PoolChunk*>(&chunk);
        }
    }
    return nullptr;
}

void PoolAllocator::rebuild_free_list() {
    // This is an expensive operation that rebuilds the entire free list
    // It's used during shrinking operations or integrity recovery
    
    free_head_ = nullptr;
    total_free_blocks_ = 0;
    
    for (auto& chunk : chunks_) {
        // Reinitialize each chunk's free list
        chunk.initialize_free_list();
        
        if (chunk.free_head) {
            // Count actual free blocks in this chunk
            FreeBlock* current = chunk.free_head;
            usize chunk_free_count = 0;
            FreeBlock* tail = nullptr;
            
            while (current && chunk_free_count < chunk.block_count) {
                // Skip if this block is allocated (checking our allocation tracker)
                if (enable_tracking_ && allocated_blocks_.count(current) > 0) {
                    // This block is actually allocated, skip it
                    if (tail) {
                        tail->next = current->next;
                    } else {
                        chunk.free_head = current->next;
                    }
                } else {
                    tail = current;
                    chunk_free_count++;
                }
                current = current->next;
            }
            
            // Connect chunk's free list to global free list
            if (tail && chunk_free_count > 0) {
                tail->next = free_head_;
                free_head_ = chunk.free_head;
                total_free_blocks_ += chunk_free_count;
            }
        }
    }
}

void PoolAllocator::record_allocation(void* ptr, const char* category, 
                                    const char* file, int line, const char* function) {
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    
    allocations_.push_back(AllocationInfo{
        ptr,
        block_size_,        // All blocks are the same size
        alignment_,
        category,
        get_current_time(),
        true,
        file,
        line,
        function
    });
    
    allocated_blocks_.insert(ptr);
}

void PoolAllocator::record_deallocation(void* ptr) {
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    
    // Mark allocation as inactive
    auto it = std::find_if(allocations_.begin(), allocations_.end(),
        [ptr](AllocationInfo& info) {
            return info.ptr == ptr && info.active;
        });
    
    if (it != allocations_.end()) {
        it->active = false;
    }
    
    allocated_blocks_.erase(ptr);
}

void PoolAllocator::fill_memory(void* ptr, usize size, byte pattern) {
    if (ptr && size > 0) {
        std::memset(ptr, pattern, size);
    }
}

void PoolAllocator::update_fragmentation_stats() {
    // Calculate fragmentation metrics based on chunk utilization patterns
    if (chunks_.empty()) return;
    
    // External fragmentation: measure distribution of free blocks
    f64 utilization_variance = 0.0;
    f64 mean_utilization = 0.0;
    
    for (const auto& chunk : chunks_) {
        mean_utilization += chunk.utilization();
    }
    mean_utilization /= chunks_.size();
    
    for (const auto& chunk : chunks_) {
        f64 deviation = chunk.utilization() - mean_utilization;
        utilization_variance += deviation * deviation;
    }
    utilization_variance /= chunks_.size();
    
    // Higher variance indicates more fragmentation
    stats_.external_fragmentation = std::sqrt(utilization_variance);
    
    // Internal fragmentation: for fixed-size pools, this is minimal
    // We consider wasted bytes from alignment and chunk overhead
    if (stats_.total_memory_used > 0) {
        stats_.internal_fragmentation = static_cast<f64>(stats_.wasted_bytes) / stats_.total_memory_used;
    }
}

void PoolAllocator::update_performance_stats() {
    // Calculate allocation frequency based on recent activity
    f64 current_time = get_current_time();
    static f64 last_update_time = current_time;
    
    f64 time_delta = current_time - last_update_time;
    if (time_delta > 1.0) { // Update every second
        stats_.allocation_frequency = allocation_counter_.exchange(0) / time_delta;
        last_update_time = current_time;
    }
}

f64 PoolAllocator::get_current_time() const noexcept {
    return core::get_time_seconds();
}

void PoolAllocator::estimate_cache_behavior(void* ptr) {
    // Simple cache miss estimation based on allocation patterns
    // This is educational and provides rough estimates
    
    static void* last_allocated_ptr = nullptr;
    static f64 last_allocation_time = 0.0;
    
    f64 current_time = get_current_time();
    
    if (last_allocated_ptr) {
        // Estimate cache miss if allocation is far from previous or too much time has passed
        uintptr_t current_addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t last_addr = reinterpret_cast<uintptr_t>(last_allocated_ptr);
        uintptr_t distance = current_addr > last_addr ? current_addr - last_addr : last_addr - current_addr;
        
        constexpr uintptr_t CACHE_LINE_SIZE = 64;
        constexpr f64 CACHE_TIMEOUT = 0.001; // 1ms timeout for cache invalidation
        
        if (distance > CACHE_LINE_SIZE || (current_time - last_allocation_time) > CACHE_TIMEOUT) {
            stats_.cache_misses_estimated++;
        }
    }
    
    last_allocated_ptr = ptr;
    last_allocation_time = current_time;
}

void PoolAllocator::poison_block(void* ptr, bool is_allocation) {
    if (!ptr) return;
    
    byte pattern = is_allocation ? debug_alloc_pattern_ : debug_free_pattern_;
    fill_memory(ptr, block_size_, pattern);
}

bool PoolAllocator::check_poison_pattern(void* ptr, bool expect_alloc_pattern) const {
    if (!ptr) return false;
    
    byte expected_pattern = expect_alloc_pattern ? debug_alloc_pattern_ : debug_free_pattern_;
    const byte* byte_ptr = static_cast<const byte*>(ptr);
    
    // Check first few bytes for the pattern
    for (usize i = 0; i < std::min(block_size_, usize(16)); ++i) {
        if (byte_ptr[i] != expected_pattern) {
            return false;
        }
    }
    
    return true;
}

// ===========================================
// Pool Registry Implementation
// ===========================================

namespace pool_registry {
    
    void register_pool(PoolAllocator* pool) {
        if (!pool) return;
        
        std::lock_guard<std::mutex> lock(g_pool_registry_mutex);
        auto it = std::find(g_registered_pools.begin(), g_registered_pools.end(), pool);
        if (it == g_registered_pools.end()) {
            g_registered_pools.push_back(pool);
        }
    }
    
    void unregister_pool(PoolAllocator* pool) {
        if (!pool) return;
        
        std::lock_guard<std::mutex> lock(g_pool_registry_mutex);
        auto it = std::find(g_registered_pools.begin(), g_registered_pools.end(), pool);
        if (it != g_registered_pools.end()) {
            g_registered_pools.erase(it);
        }
    }
    
    std::vector<PoolAllocator*> get_all_pools() {
        std::lock_guard<std::mutex> lock(g_pool_registry_mutex);
        return g_registered_pools;
    }
    
    PoolStats get_combined_stats() {
        PoolStats combined;
        combined.reset();
        
        std::lock_guard<std::mutex> lock(g_pool_registry_mutex);
        for (const auto* pool : g_registered_pools) {
            if (!pool) continue;
            
            const auto& stats = pool->stats();
            combined.total_capacity += stats.total_capacity;
            combined.total_allocated += stats.total_allocated;
            combined.peak_allocated += stats.peak_allocated;
            combined.total_allocations += stats.total_allocations;
            combined.total_deallocations += stats.total_deallocations;
            combined.total_memory_used += stats.total_memory_used;
            combined.wasted_bytes += stats.wasted_bytes;
            combined.overhead_bytes += stats.overhead_bytes;
            combined.free_list_length += stats.free_list_length;
            combined.chunk_count += stats.chunk_count;
            combined.total_alloc_time += stats.total_alloc_time;
            combined.total_dealloc_time += stats.total_dealloc_time;
            combined.cache_misses_estimated += stats.cache_misses_estimated;
            combined.chunk_expansions += stats.chunk_expansions;
        }
        
        // Calculate averages and ratios
        if (!g_registered_pools.empty()) {
            combined.average_chunk_usage /= g_registered_pools.size();
            
            if (combined.total_allocations > 0) {
                combined.average_alloc_time = combined.total_alloc_time / combined.total_allocations;
            }
            
            if (combined.total_deallocations > 0) {
                combined.average_dealloc_time = combined.total_dealloc_time / combined.total_deallocations;
            }
            
            if (combined.total_capacity > 0) {
                combined.external_fragmentation = static_cast<f64>(combined.free_list_length) / combined.total_capacity;
            }
            
            if (combined.total_memory_used > 0) {
                combined.internal_fragmentation = static_cast<f64>(combined.wasted_bytes) / combined.total_memory_used;
            }
        }
        
        return combined;
    }
    
    std::vector<PoolAllocator*> get_pools_by_type(usize type_hash) {
        std::vector<PoolAllocator*> filtered_pools;
        
        std::lock_guard<std::mutex> lock(g_pool_registry_mutex);
        for (auto* pool : g_registered_pools) {
            // Note: type_hash comparison would need to be implemented if type tracking is needed
            // For now, this is a placeholder for future enhancement
            filtered_pools.push_back(pool);
        }
        
        return filtered_pools;
    }
    
    std::string generate_system_report() {
        std::stringstream report;
        
        std::lock_guard<std::mutex> lock(g_pool_registry_mutex);
        
        report << "=== Pool Allocator System Report ===\n";
        report << "Total Pools: " << g_registered_pools.size() << "\n";
        report << "Report Time: " << std::fixed << std::setprecision(3) << core::get_time_seconds() << "\n\n";
        
        PoolStats combined = get_combined_stats();
        
        report << "--- System-Wide Statistics ---\n";
        report << "Total Capacity: " << combined.total_capacity << " blocks\n";
        report << "Total Allocated: " << combined.total_allocated << " blocks\n";
        report << "Peak Allocated: " << combined.peak_allocated << " blocks\n";
        report << "Total Memory: " << (combined.total_memory_used / 1024.0 / 1024.0) << " MB\n";
        report << "Overhead: " << (combined.overhead_bytes / 1024.0) << " KB\n";
        report << "System Efficiency: " << std::fixed << std::setprecision(1) 
               << ((combined.total_capacity > 0 ? static_cast<f64>(combined.total_allocated) / combined.total_capacity : 0.0) * 100.0) << "%\n\n";
        
        report << "--- Individual Pool Details ---\n";
        for (usize i = 0; i < g_registered_pools.size(); ++i) {
            const auto* pool = g_registered_pools[i];
            if (!pool) continue;
            
            report << "Pool " << i << " (" << pool->name() << "):\n";
            report << "  Block Size: " << pool->block_size() << " bytes\n";
            report << "  Capacity: " << pool->total_capacity() << " blocks\n";
            report << "  Allocated: " << pool->allocated_count() << " blocks\n";
            report << "  Utilization: " << std::fixed << std::setprecision(1) << (pool->utilization_ratio() * 100.0) << "%\n";
            report << "  Chunks: " << pool->chunk_count() << "\n\n";
        }
        
        return report.str();
    }
    
} // namespace pool_registry

} // namespace ecscope::memory