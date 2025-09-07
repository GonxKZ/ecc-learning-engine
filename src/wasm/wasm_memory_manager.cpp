// =============================================================================
// ECScope WebAssembly Memory Manager Implementation
// =============================================================================

#include "wasm_memory_manager.hpp"
#include <emscripten.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace ecscope::wasm {

// =============================================================================
// MEMORY POOL IMPLEMENTATION
// =============================================================================

void* WasmMemoryManager::allocateFromPool(std::vector<MemoryBlock>& pool, size_t size, size_t alignment) {
    // Try to find a free block
    for (auto& block : pool) {
        if (!block.in_use && block.size >= size) {
            // Check alignment
            uintptr_t ptr = reinterpret_cast<uintptr_t>(block.ptr);
            uintptr_t aligned_ptr = (ptr + alignment - 1) & ~(alignment - 1);
            
            if (aligned_ptr + size <= ptr + block.size) {
                block.in_use = true;
                return reinterpret_cast<void*>(aligned_ptr);
            }
        }
    }
    
    // No suitable block found, allocate new one
    void* new_ptr = std::aligned_alloc(alignment, size);
    if (new_ptr) {
        pool.emplace_back(new_ptr, size);
        return new_ptr;
    }
    
    return nullptr;
}

void WasmMemoryManager::deallocateToPool(std::vector<MemoryBlock>& pool, void* ptr, size_t size) {
    // Find the block and mark it as free
    for (auto& block : pool) {
        if (block.ptr == ptr && block.size == size) {
            block.in_use = false;
            return;
        }
    }
    
    // If not found in pool, it might be a direct allocation
    std::free(ptr);
}

size_t WasmMemoryManager::getActiveBlockCount() const {
    size_t count = 0;
    
    auto count_active = [&count](const std::vector<MemoryBlock>& pool) {
        for (const auto& block : pool) {
            if (block.in_use) {
                count++;
            }
        }
    };
    
    count_active(small_block_pool_);
    count_active(medium_block_pool_);
    count_active(large_block_pool_);
    
    return count;
}

// =============================================================================
// WEB-SPECIFIC MEMORY UTILITIES
// =============================================================================

void WasmMemoryManager::compactPools() {
    auto compact_pool = [](std::vector<MemoryBlock>& pool) {
        // Remove unused blocks and defragment
        pool.erase(std::remove_if(pool.begin(), pool.end(), 
            [](const MemoryBlock& block) { 
                if (!block.in_use) {
                    std::free(block.ptr);
                    return true;
                }
                return false;
            }), pool.end());
    };
    
    compact_pool(small_block_pool_);
    compact_pool(medium_block_pool_);
    compact_pool(large_block_pool_);
}

void WasmMemoryManager::reportDetailedStats() const {
    EM_ASM({
        const stats = {
            totalAllocated: $0,
            peakUsage: $1,
            allocationCount: $2,
            activeBlocks: $3,
            pools: {
                small: {
                    capacity: $4,
                    used: $5
                },
                medium: {
                    capacity: $6,
                    used: $7
                },
                large: {
                    capacity: $8,
                    used: $9
                }
            }
        };
        
        console.log('Detailed ECScope Memory Statistics:');
        console.table(stats);
        
        if (window.ECScope && window.ECScope.onDetailedMemoryReport) {
            window.ECScope.onDetailedMemoryReport(stats);
        }
    }, 
    total_allocated_, peak_usage_, allocation_count_, getActiveBlockCount(),
    small_block_pool_.capacity(), getPoolUsage(small_block_pool_),
    medium_block_pool_.capacity(), getPoolUsage(medium_block_pool_),
    large_block_pool_.capacity(), getPoolUsage(large_block_pool_));
}

size_t WasmMemoryManager::getPoolUsage(const std::vector<MemoryBlock>& pool) const {
    size_t used = 0;
    for (const auto& block : pool) {
        if (block.in_use) {
            used++;
        }
    }
    return used;
}

// =============================================================================
// WEBASSEMBLY SPECIFIC OPTIMIZATIONS
// =============================================================================

void WasmMemoryManager::optimizeForWeb() {
    // Compact pools to reduce memory fragmentation
    compactPools();
    
    // Report current state to JavaScript for debugging
    EM_ASM({
        const heapSize = HEAPU8.length;
        const usedHeap = $0;
        const efficiency = usedHeap / heapSize;
        
        console.log('WebAssembly Memory Efficiency:', {
            heapSize: heapSize,
            usedHeap: usedHeap,
            efficiency: (efficiency * 100).toFixed(2) + '%',
            fragmentationLevel: $1
        });
        
        if (window.ECScope && window.ECScope.onMemoryOptimization) {
            window.ECScope.onMemoryOptimization({
                heapSize: heapSize,
                usedHeap: usedHeap,
                efficiency: efficiency
            });
        }
    }, total_allocated_, calculateFragmentation());
}

double WasmMemoryManager::calculateFragmentation() const {
    // Simple fragmentation calculation based on unused blocks
    size_t total_blocks = small_block_pool_.size() + medium_block_pool_.size() + large_block_pool_.size();
    size_t active_blocks = getActiveBlockCount();
    
    if (total_blocks == 0) return 0.0;
    
    return 1.0 - (static_cast<double>(active_blocks) / total_blocks);
}

void WasmMemoryManager::enableMemoryProfiling(bool enable) {
    config_.enable_tracking = enable;
    
    if (enable) {
        EM_ASM({
            console.log('ECScope memory profiling enabled');
            window.ECScope = window.ECScope || {};
            window.ECScope.memoryProfiling = true;
        });
    } else {
        EM_ASM({
            console.log('ECScope memory profiling disabled');
            if (window.ECScope) {
                window.ECScope.memoryProfiling = false;
            }
        });
    }
}

}  // namespace ecscope::wasm