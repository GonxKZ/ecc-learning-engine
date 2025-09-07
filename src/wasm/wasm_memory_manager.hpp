// =============================================================================
// ECScope WebAssembly Memory Manager Header
// =============================================================================

#pragma once

#include "wasm_core.hpp"
#include <vector>
#include <memory>

namespace ecscope::wasm {

// Additional methods for the WasmMemoryManager class
class WasmMemoryManager {
    // ... (existing members from wasm_core.hpp)
    
public:
    // Additional web-specific methods
    void compactPools();
    void reportDetailedStats() const;
    void optimizeForWeb();
    void enableMemoryProfiling(bool enable);
    
    double calculateFragmentation() const;
    size_t getPoolUsage(const std::vector<MemoryBlock>& pool) const;
    
    // Memory pressure handling
    void handleMemoryPressure();
    bool isMemoryPressureHigh() const;
};

}  // namespace ecscope::wasm