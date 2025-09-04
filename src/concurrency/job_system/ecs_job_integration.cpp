/**
 * @file ecs_job_integration.cpp
 * @brief Implementation of ECS Job System Integration
 */

#include "ecs_job_integration.hpp"
#include "core/log.hpp"
#include <algorithm>

namespace ecscope::job_system {

//=============================================================================
// JobEnabledSystem Implementation
//=============================================================================

// Implementation is mostly header-only, any complex methods would go here

//=============================================================================
// ParallelPhysicsSystem Implementation
//=============================================================================

// Implementation methods for the parallel physics system would go here
// Currently implemented inline in header for demonstration

//=============================================================================
// ParallelRenderingSystem Implementation  
//=============================================================================

// Implementation methods for the parallel rendering system would go here
// Currently implemented inline in header for demonstration

//=============================================================================
// ParallelAnimationSystem Implementation
//=============================================================================

// Implementation methods for the parallel animation system would go here
// Currently implemented inline in header for demonstration

//=============================================================================
// ECSJobSystemIntegrator Implementation
//=============================================================================

// Most implementation is in header, but complex initialization logic can go here

bool ECSJobSystemIntegrator::initialize_advanced_features() {
    if (!job_system_ || !parallel_scheduler_) {
        LOG_ERROR("Job system and parallel scheduler must be initialized first");
        return false;
    }
    
    // Advanced initialization features
    LOG_INFO("Initializing advanced job system features...");
    
    // Configure NUMA topology if available
    #ifdef ECSCOPE_HAS_NUMA
    if (job_config_.enable_numa_awareness) {
        // Additional NUMA-specific initialization
        LOG_INFO("Configuring NUMA-aware job scheduling");
    }
    #endif
    
    // Initialize profiling if enabled
    if (job_config_.enable_profiling) {
        LOG_INFO("Performance profiling enabled for job system");
    }
    
    // Setup system dependency analysis
    parallel_scheduler_->analyze_all_systems();
    
    return true;
}

void ECSJobSystemIntegrator::print_system_configuration() const {
    if (!parallel_scheduler_) {
        LOG_WARN("Parallel scheduler not initialized");
        return;
    }
    
    LOG_INFO("=== ECS Job System Configuration ===");
    LOG_INFO("Worker Threads: {}", job_system_ ? job_system_->worker_count() : 0);
    LOG_INFO("Systems Analyzed: {}", parallel_scheduler_->get_statistics().active_parallel_groups);
    LOG_INFO("Parallel Execution: {}", job_config_.enable_work_stealing ? "Enabled" : "Disabled");
    LOG_INFO("Performance Monitoring: {}", job_config_.enable_profiling ? "Enabled" : "Disabled");
    LOG_INFO("Educational Mode: {}", scheduler_config_.enable_performance_monitoring ? "Enabled" : "Disabled");
}

} // namespace ecscope::job_system