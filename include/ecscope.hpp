/**
 * @file ecscope.hpp
 * @brief Main ECScope header - includes all core functionality
 * 
 * ECScope - Educational ECS Engine with Memory Observatory
 * A comprehensive Entity Component System implementation designed for learning.
 */

#pragma once

// Core system headers
#include "ecscope/log.hpp"
#include "ecscope/time.hpp"
#include "ecscope/id.hpp"

// ECS core headers
#include "ecscope/component.hpp"
#include "ecscope/entity.hpp" 
#include "ecscope/registry.hpp"
#include "ecscope/system.hpp"
#include "ecscope/query.hpp"
#include "ecscope/archetype.hpp"

// Memory system headers
#include "ecscope/pool_allocator.hpp"
#include "ecscope/mem_tracker.hpp"

// Physics system headers (if enabled)
#ifdef ECSCOPE_HAS_3D_PHYSICS
#include "ecscope/world3d.hpp"
#include "ecscope/simd_math.hpp"
#endif

// Job system headers (if enabled)
#ifdef ECSCOPE_HAS_JOB_SYSTEM
#include "ecscope/work_stealing_job_system.hpp"
#include "ecscope/ecs_job_integration.hpp"
#include "ecscope/ecs_parallel_scheduler.hpp"
#include "ecscope/job_profiler.hpp"
#endif

// Graphics headers (if enabled)
#ifdef ECSCOPE_HAS_GRAPHICS
#include "ecscope/window.hpp"
#include "ecscope/renderer_2d.hpp"
#include "ecscope/batch_renderer.hpp"
#endif

// UI headers (if available)
#if defined(ECSCOPE_HAS_GRAPHICS) && defined(ECSCOPE_HAS_IMGUI)
#include "ecscope/overlay.hpp"
#include "ecscope/panel_ecs_inspector.hpp"
#include "ecscope/panel_memory.hpp"
#include "ecscope/panel_stats.hpp"
#include "ecscope/panel_performance_lab.hpp"
#endif

// Educational and performance analysis tools
#ifdef ECSCOPE_EDUCATIONAL_MODE
#include "ecscope/performance_lab.hpp"
#include "ecscope/memory_experiments.hpp"
#include "ecscope/allocation_benchmarks.hpp"
#endif

// Hardware detection and optimization (if enabled)
#ifdef ECSCOPE_HAS_HARDWARE_DETECTION
#include "ecscope/hardware_detection.hpp"
#include "ecscope/optimization_engine.hpp"
#include "ecscope/system_integration.hpp"
#include "ecscope/performance_benchmark.hpp"
#endif

/**
 * @namespace ecscope
 * @brief Main ECScope namespace containing all engine functionality
 */
namespace ecscope {
    
    /**
     * @brief Initialize ECScope engine
     * @return true if initialization successful
     */
    inline bool initialize() {
        // Initialize core systems
        log::initialize();
        
        #ifdef ECSCOPE_ENABLE_INSTRUMENTATION
        mem_tracker::initialize();
        #endif
        
        #ifdef ECSCOPE_HAS_JOB_SYSTEM
        // Initialize job system with hardware-detected thread count
        // This will be handled by the job system implementation
        #endif
        
        return true;
    }
    
    /**
     * @brief Shutdown ECScope engine
     */
    inline void shutdown() {
        #ifdef ECSCOPE_ENABLE_INSTRUMENTATION
        mem_tracker::shutdown();
        #endif
        
        log::shutdown();
    }
    
} // namespace ecscope