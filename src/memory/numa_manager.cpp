#include "numa_manager.hpp"
#include "core/log.hpp"

namespace ecscope::memory::numa {

// Global NUMA manager instance (simplified implementation)
NumaManager& get_global_numa_manager() {
    static NumaManager instance;
    return instance;
}

} // namespace ecscope::memory::numa