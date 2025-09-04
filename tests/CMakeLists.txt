# =============================================================================
# ECScope Comprehensive Test Suite
# =============================================================================

if(NOT ECSCOPE_BUILD_TESTS)
    return()
endif()

# Configure testing framework
ecscope_enable_coverage()
ecscope_configure_test_sanitizers()
ecscope_configure_performance_tests()
ecscope_configure_ci_testing()

# =============================================================================
# UNIT TESTS
# =============================================================================

# Core system unit tests
ecscope_add_test(
    NAME core_tests
    UNIT
    SOURCES
        core/test_types.cpp
        core/test_log.cpp
        core/test_time.cpp
        core/test_id.cpp
        core/test_memory_tracking.cpp
    DEPENDENCIES ECScope::ecscope
    LABELS "core" "fast"
    TIMEOUT 30
)

# ECS system unit tests
ecscope_add_test(
    NAME ecs_tests
    UNIT
    SOURCES
        ecs/test_entity.cpp
        ecs/test_component.cpp
        ecs/test_registry.cpp
        ecs/test_query.cpp
        ecs/test_systems.cpp
        ecs/test_archetype.cpp
    DEPENDENCIES ECScope::ecscope
    LABELS "ecs" "core"
    TIMEOUT 60
)

# Memory system unit tests
ecscope_add_test(
    NAME memory_tests
    UNIT
    SOURCES
        memory/test_arena.cpp
        memory/test_pool.cpp
        memory/test_allocators.cpp
        memory/test_tracking.cpp
        memory/test_numa.cpp
        memory/test_cache_aware.cpp
    DEPENDENCIES ECScope::ecscope
    LABELS "memory" "performance"
    TIMEOUT 90
)

# Physics system unit tests
ecscope_add_test(
    NAME physics_tests
    UNIT
    SOURCES
        physics/test_math.cpp
        physics/test_simd_math.cpp
        physics/test_collision.cpp
        physics/test_world3d.cpp
        physics/test_integration.cpp
    DEPENDENCIES ECScope::ecscope
    LABELS "physics" "math"
    TIMEOUT 60
)

# Job system unit tests (if enabled)
if(ECSCOPE_ENABLE_JOB_SYSTEM)
    ecscope_add_test(
        NAME job_system_tests
        UNIT
        SOURCES
            job_system/test_work_stealing.cpp
            job_system/test_scheduler.cpp
            job_system/test_ecs_integration.cpp
            job_system/test_profiler.cpp
        DEPENDENCIES ECScope::ecscope Threads::Threads
        LABELS "jobs" "threading"
        TIMEOUT 120
    )
endif()

# Graphics system unit tests (if enabled)
if(ECSCOPE_HAS_GRAPHICS)
    ecscope_add_test(
        NAME graphics_tests
        UNIT
        SOURCES
            renderer/test_window.cpp
            renderer/test_renderer_2d.cpp
            renderer/test_batch_renderer.cpp
            renderer/test_resources.cpp
        DEPENDENCIES ECScope::ecscope
        LABELS "graphics" "rendering"
        TIMEOUT 90
    )
endif()

# Scripting system unit tests (if enabled)
if(ECSCOPE_HAS_SCRIPTING)
    ecscope_add_test(
        NAME scripting_tests
        UNIT
        SOURCES
            scripting/test_python_integration.cpp
            scripting/test_lua_integration.cpp
            scripting/test_hot_reload.cpp
        DEPENDENCIES ECScope::ecscope
        LABELS "scripting" "integration"
        TIMEOUT 90
    )
endif()

# Platform and hardware detection tests (if enabled)
if(ECSCOPE_ENABLE_HARDWARE_DETECTION)
    ecscope_add_test(
        NAME platform_tests
        UNIT
        SOURCES
            platform/test_hardware_detection.cpp
            platform/test_optimization_engine.cpp
            platform/test_system_integration.cpp
        DEPENDENCIES ECScope::ecscope
        LABELS "platform" "hardware"
        TIMEOUT 60
    )
endif()

# =============================================================================
# INTEGRATION TESTS
# =============================================================================

# ECS + Memory integration tests
ecscope_add_test(
    NAME ecs_memory_integration
    INTEGRATION
    SOURCES
        integration/test_ecs_memory_integration.cpp
        integration/test_large_scenes.cpp
        integration/test_memory_patterns.cpp
    DEPENDENCIES ECScope::ecscope
    LABELS "integration" "memory" "ecs"
    TIMEOUT 180
)

# Physics + ECS integration tests
ecscope_add_test(
    NAME physics_ecs_integration
    INTEGRATION
    SOURCES
        integration/test_physics_ecs_integration.cpp
        integration/test_physics_components.cpp
        integration/test_collision_detection.cpp
    DEPENDENCIES ECScope::ecscope
    LABELS "integration" "physics" "ecs"
    TIMEOUT 120
)

# Job System + ECS integration tests (if enabled)
if(ECSCOPE_ENABLE_JOB_SYSTEM)
    ecscope_add_test(
        NAME job_ecs_integration
        INTEGRATION
        SOURCES
            integration/test_parallel_ecs.cpp
            integration/test_job_scheduling.cpp
            integration/test_load_balancing.cpp
        DEPENDENCIES ECScope::ecscope Threads::Threads
        LABELS "integration" "jobs" "ecs"
        TIMEOUT 180
    )
endif()

# Full system integration tests
ecscope_add_test(
    NAME full_system_integration
    INTEGRATION
    SOURCES
        integration/test_full_system.cpp
        integration/test_educational_scenarios.cpp
        integration/test_complex_workflows.cpp
    DEPENDENCIES ECScope::ecscope
    LABELS "integration" "system" "educational"
    TIMEOUT 300
)

# =============================================================================
# PERFORMANCE TESTS
# =============================================================================

# Memory performance benchmarks
if(ECSCOPE_BUILD_BENCHMARKS)
    ecscope_add_benchmark(
        NAME memory_performance
        SOURCES
            benchmarks/bench_memory_allocators.cpp
            benchmarks/bench_cache_performance.cpp
            benchmarks/bench_numa_performance.cpp
    )
    
    # ECS performance benchmarks
    ecscope_add_benchmark(
        NAME ecs_performance
        SOURCES
            benchmarks/bench_ecs_creation.cpp
            benchmarks/bench_ecs_queries.cpp
            benchmarks/bench_ecs_systems.cpp
    )
    
    # Physics performance benchmarks
    ecscope_add_benchmark(
        NAME physics_performance
        SOURCES
            benchmarks/bench_physics_math.cpp
            benchmarks/bench_collision_detection.cpp
            benchmarks/bench_simd_operations.cpp
    )
    
    # Job system performance benchmarks (if enabled)
    if(ECSCOPE_ENABLE_JOB_SYSTEM)
        ecscope_add_benchmark(
            NAME job_system_performance
            SOURCES
                benchmarks/bench_work_stealing.cpp
                benchmarks/bench_parallel_ecs.cpp
                benchmarks/bench_scheduling_overhead.cpp
        )
    endif()
    
    # Graphics performance benchmarks (if enabled)
    if(ECSCOPE_HAS_GRAPHICS)
        ecscope_add_benchmark(
            NAME graphics_performance
            SOURCES
                benchmarks/bench_rendering_2d.cpp
                benchmarks/bench_batch_rendering.cpp
                benchmarks/bench_gpu_utilization.cpp
        )
    endif()
endif()

# =============================================================================
# STRESS TESTS
# =============================================================================

# Memory stress tests
ecscope_add_test(
    NAME memory_stress_tests
    PERFORMANCE
    SOURCES
        stress/test_memory_stress.cpp
        stress/test_fragmentation.cpp
        stress/test_allocation_patterns.cpp
    DEPENDENCIES ECScope::ecscope
    LABELS "stress" "memory"
    TIMEOUT 600  # 10 minutes for stress tests
)

# ECS stress tests
ecscope_add_test(
    NAME ecs_stress_tests
    PERFORMANCE
    SOURCES
        stress/test_ecs_stress.cpp
        stress/test_massive_entities.cpp
        stress/test_query_performance.cpp
    DEPENDENCIES ECScope::ecscope
    LABELS "stress" "ecs"
    TIMEOUT 600
)

# Threading stress tests (if job system enabled)
if(ECSCOPE_ENABLE_JOB_SYSTEM)
    ecscope_add_test(
        NAME threading_stress_tests
        PERFORMANCE
        SOURCES
            stress/test_threading_stress.cpp
            stress/test_contention.cpp
            stress/test_scalability.cpp
        DEPENDENCIES ECScope::ecscope Threads::Threads
        LABELS "stress" "threading"
        TIMEOUT 900  # 15 minutes for threading stress tests
    )
endif()

# =============================================================================
# TEST DATA AND RESOURCES
# =============================================================================

# Add test resources if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_data")
    # Copy test data for all test targets
    set(test_targets 
        core_tests ecs_tests memory_tests physics_tests
        ecs_memory_integration physics_ecs_integration full_system_integration
        memory_stress_tests ecs_stress_tests
    )
    
    if(ECSCOPE_ENABLE_JOB_SYSTEM)
        list(APPEND test_targets job_system_tests job_ecs_integration threading_stress_tests)
    endif()
    
    if(ECSCOPE_HAS_GRAPHICS)
        list(APPEND test_targets graphics_tests)
    endif()
    
    if(ECSCOPE_HAS_SCRIPTING)
        list(APPEND test_targets scripting_tests)
    endif()
    
    if(ECSCOPE_ENABLE_HARDWARE_DETECTION)
        list(APPEND test_targets platform_tests)
    endif()
    
    foreach(target ${test_targets})
        ecscope_add_test_resources(${target})
    endforeach()
endif()

# =============================================================================
# TEST SUMMARY
# =============================================================================

message(STATUS "")
message(STATUS "ECScope Test Configuration Summary:")
message(STATUS "  Unit Tests: ✓ Enabled")
message(STATUS "  Integration Tests: ✓ Enabled")
message(STATUS "  Performance Tests: ${ECSCOPE_BUILD_BENCHMARKS}")
message(STATUS "  Coverage Reporting: ${ECSCOPE_ENABLE_COVERAGE}")
message(STATUS "  Sanitizers: ${ECSCOPE_ENABLE_SANITIZERS}")
message(STATUS "  CI Integration: ✓ Enabled")
message(STATUS "")
message(STATUS "Test Execution Commands:")
message(STATUS "  All tests: ctest")
message(STATUS "  Unit tests only: ctest -L unit")
message(STATUS "  Integration tests: ctest -L integration")
message(STATUS "  Performance tests: ctest -L performance")
message(STATUS "  CI test suite: make ci_tests")
if(ECSCOPE_BUILD_BENCHMARKS)
    message(STATUS "  Benchmarks: make run_benchmarks")
endif()
if(ECSCOPE_ENABLE_COVERAGE)
    message(STATUS "  Coverage report: make coverage")
endif()
message(STATUS "")