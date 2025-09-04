/**
 * @file job_system_demo.cpp
 * @brief Comprehensive Work-Stealing Job System Demonstration
 * 
 * This demonstration showcases the advanced work-stealing job system
 * with educational examples, performance benchmarks, and real-world
 * usage patterns in game engine context.
 * 
 * Educational Features Demonstrated:
 * - Work-stealing queue mechanics and visualization
 * - Parallel ECS system execution with dependency analysis
 * - Performance comparison between sequential and parallel execution
 * - Real-time profiling and educational insights
 * - Integration with physics, rendering, and memory systems
 * 
 * Performance Examples:
 * - Parallel physics simulation with automatic load balancing
 * - Batch rendering with parallel command generation
 * - SIMD-optimized parallel algorithms
 * - NUMA-aware memory allocation and thread scheduling
 * 
 * @author ECScope Educational ECS Framework - Job System Demo
 * @date 2025
 */

#include "job_system/work_stealing_job_system.hpp"
#include "job_system/ecs_parallel_scheduler.hpp"
#include "job_system/ecs_job_integration.hpp"
#include "job_system/job_profiler.hpp"
#include "ecs/registry.hpp"
#include "ecs/system.hpp"
#include "physics/physics_system.hpp"
#include "physics/simd_math.hpp"
#include "renderer/batch_renderer.hpp"
#include "core/log.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>

using namespace ecscope;
using namespace ecscope::job_system;

//=============================================================================
// Demo Configuration and Utilities
//=============================================================================

struct DemoConfig {
    u32 entity_count = 10000;
    u32 worker_thread_count = 0; // 0 = auto-detect
    bool enable_profiling = true;
    bool enable_visualization = true;
    bool enable_educational_output = true;
    bool run_performance_benchmarks = true;
    f64 demo_duration_seconds = 30.0;
    
    static DemoConfig create_quick_demo() {
        DemoConfig config;
        config.entity_count = 1000;
        config.demo_duration_seconds = 5.0;
        config.run_performance_benchmarks = false;
        return config;
    }
    
    static DemoConfig create_comprehensive_demo() {
        DemoConfig config;
        config.entity_count = 50000;
        config.demo_duration_seconds = 60.0;
        config.run_performance_benchmarks = true;
        return config;
    }
    
    static DemoConfig create_performance_benchmark() {
        DemoConfig config;
        config.entity_count = 100000;
        config.demo_duration_seconds = 120.0;
        config.enable_visualization = false;
        config.enable_educational_output = false;
        config.run_performance_benchmarks = true;
        return config;
    }
};

//=============================================================================
// Demo ECS Components
//=============================================================================

struct DemoVelocity {
    physics::Vec2 velocity{0.0f, 0.0f};
    f32 damping = 0.99f;
};

struct DemoParticle {
    f32 mass = 1.0f;
    f32 radius = 1.0f;
    physics::Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    bool is_active = true;
};

struct DemoPhysics {
    physics::Vec2 force{0.0f, 0.0f};
    physics::Vec2 acceleration{0.0f, 0.0f};
    f32 inverse_mass = 1.0f;
};

struct DemoRenderData {
    u32 material_id = 0;
    f32 depth = 0.0f;
    bool is_visible = true;
};

//=============================================================================
// Demo ECS Systems
//=============================================================================

/**
 * @brief Demo physics system showing parallel computation patterns
 */
class DemoPhysicsSystem : public JobEnabledSystem {
public:
    explicit DemoPhysicsSystem(JobSystem* job_system, ECSParallelScheduler* scheduler)
        : JobEnabledSystem("DemoPhysicsSystem", job_system, scheduler,
                          ecs::SystemPhase::Update, ecs::SystemExecutionType::Parallel) {
        
        // Configure component access patterns for dependency analysis
        if (scheduler) {
            scheduler->configure_system_component_access<DemoVelocity>(
                name(), ComponentAccessType::ReadWrite, "Velocity updates");
            scheduler->configure_system_component_access<DemoPhysics>(
                name(), ComponentAccessType::ReadWrite, "Physics state updates");
            scheduler->configure_system_component_access<ecs::components::Transform>(
                name(), ComponentAccessType::ReadWrite, "Position integration");
        }
    }
    
    void update(const ecs::SystemContext& context) override {
        const f64 dt = context.delta_time();
        ecs::Registry& registry = context.registry();
        
        // Phase 1: Parallel force calculation and integration
        auto force_job = submit_dependent_job("ForceCalculation", [&, dt]() {
            calculate_forces_parallel(registry, dt);
        }, {}, JobPriority::High);
        
        // Phase 2: Parallel velocity integration (depends on forces)
        auto velocity_job = submit_dependent_job("VelocityIntegration", [&, dt]() {
            integrate_velocities_parallel(registry, dt);
        }, {force_job}, JobPriority::High);
        
        // Phase 3: Parallel position integration (depends on velocities)
        auto position_job = submit_dependent_job("PositionIntegration", [&, dt]() {
            integrate_positions_parallel(registry, dt);
        }, {velocity_job}, JobPriority::High);
        
        // Wait for all physics jobs to complete
        job_system_->wait_for_job(position_job);
    }
    
private:
    void calculate_forces_parallel(ecs::Registry& registry, f64 dt) {
        // Demonstrate SIMD-optimized parallel force calculations
        parallel_for_entities<DemoPhysics, DemoParticle>(registry,
            [dt](ecs::Entity entity, DemoPhysics& physics, const DemoParticle& particle) {
                // Simple gravity and damping forces
                physics.force = physics::Vec2(0.0f, -9.81f * particle.mass);
                physics.acceleration = physics.force * physics.inverse_mass;
            }, 1000);
    }
    
    void integrate_velocities_parallel(ecs::Registry& registry, f64 dt) {
        parallel_for_entities<DemoVelocity, DemoPhysics>(registry,
            [dt](ecs::Entity entity, DemoVelocity& velocity, const DemoPhysics& physics) {
                // Euler integration
                velocity.velocity += physics.acceleration * static_cast<f32>(dt);
                velocity.velocity *= velocity.damping;
            }, 1000);
    }
    
    void integrate_positions_parallel(ecs::Registry& registry, f64 dt) {
        parallel_for_entities<ecs::components::Transform, DemoVelocity>(registry,
            [dt](ecs::Entity entity, ecs::components::Transform& transform, const DemoVelocity& velocity) {
                // Update position from velocity
                transform.position.x += velocity.velocity.x * static_cast<f32>(dt);
                transform.position.y += velocity.velocity.y * static_cast<f32>(dt);
            }, 1000);
    }
};

/**
 * @brief Demo rendering system showing parallel command generation
 */
class DemoRenderingSystem : public JobEnabledSystem {
private:
    mutable std::vector<std::string> render_commands_;
    mutable std::mutex commands_mutex_;
    
public:
    explicit DemoRenderingSystem(JobSystem* job_system, ECSParallelScheduler* scheduler)
        : JobEnabledSystem("DemoRenderingSystem", job_system, scheduler,
                          ecs::SystemPhase::Render, ecs::SystemExecutionType::Sequential) {
        
        if (scheduler) {
            scheduler->configure_system_component_access<DemoRenderData>(
                name(), ComponentAccessType::Read, "Render data access");
            scheduler->configure_system_component_access<ecs::components::Transform>(
                name(), ComponentAccessType::Read, "Transform for rendering");
            scheduler->configure_system_component_access<DemoParticle>(
                name(), ComponentAccessType::Read, "Particle visual properties");
        }
    }
    
    void update(const ecs::SystemContext& context) override {
        ecs::Registry& registry = context.registry();
        
        // Clear previous frame commands
        {
            std::lock_guard<std::mutex> lock(commands_mutex_);
            render_commands_.clear();
        }
        
        // Phase 1: Parallel frustum culling
        auto culling_job = submit_dependent_job("FrustumCulling", [&]() {
            perform_frustum_culling_parallel(registry);
        }, {}, JobPriority::High);
        
        // Phase 2: Parallel render command generation
        auto command_job = submit_dependent_job("CommandGeneration", [&]() {
            generate_render_commands_parallel(registry);
        }, {culling_job}, JobPriority::Normal);
        
        // Phase 3: Sequential command submission (GPU operations)
        job_system_->wait_for_job(command_job);
        submit_render_commands_sequential();
    }
    
    const std::vector<std::string>& get_render_commands() const {
        std::lock_guard<std::mutex> lock(commands_mutex_);
        return render_commands_;
    }
    
private:
    void perform_frustum_culling_parallel(ecs::Registry& registry) {
        // Simulate frustum culling with simple bounds checking
        parallel_for_entities<DemoRenderData, ecs::components::Transform>(registry,
            [](ecs::Entity entity, DemoRenderData& render_data, const ecs::components::Transform& transform) {
                // Simple visibility test based on position bounds
                const f32 view_bounds = 100.0f;
                bool in_bounds = (std::abs(transform.position.x) < view_bounds && 
                                std::abs(transform.position.y) < view_bounds);
                render_data.is_visible = in_bounds;
            }, 500);
    }
    
    void generate_render_commands_parallel(ecs::Registry& registry) {
        // Generate render commands in parallel, but collect them thread-safely
        auto entities = registry.get_entities_with<DemoRenderData, ecs::components::Transform, DemoParticle>();
        
        // Filter visible entities first
        std::vector<ecs::Entity> visible_entities;
        for (auto entity : entities) {
            auto* render_data = registry.get_component<DemoRenderData>(entity);
            if (render_data && render_data->is_visible) {
                visible_entities.push_back(entity);
            }
        }
        
        // Generate commands in parallel
        job_system_->parallel_for_each(visible_entities, [&](ecs::Entity entity) {
            auto* render_data = registry.get_component<DemoRenderData>(entity);
            auto* transform = registry.get_component<ecs::components::Transform>(entity);
            auto* particle = registry.get_component<DemoParticle>(entity);
            
            if (render_data && transform && particle) {
                std::string command = create_render_command(entity, *render_data, *transform, *particle);
                
                std::lock_guard<std::mutex> lock(commands_mutex_);
                render_commands_.push_back(command);
            }
        }, 200);
    }
    
    void submit_render_commands_sequential() {
        std::lock_guard<std::mutex> lock(commands_mutex_);
        
        // Sort commands by depth for proper rendering order
        std::sort(render_commands_.begin(), render_commands_.end());
        
        // Simulate GPU submission (in real implementation, this would submit to graphics API)
        LOG_DEBUG("Submitted {} render commands", render_commands_.size());
    }
    
    std::string create_render_command(ecs::Entity entity, const DemoRenderData& render_data,
                                    const ecs::components::Transform& transform,
                                    const DemoParticle& particle) {
        // Create a simple render command string (in real implementation, this would be a GPU command)
        std::ostringstream cmd;
        cmd << "DRAW_PARTICLE entity=" << entity.id() 
            << " pos=(" << transform.position.x << "," << transform.position.y << ")"
            << " depth=" << render_data.depth
            << " material=" << render_data.material_id;
        return cmd.str();
    }
};

//=============================================================================
// Performance Benchmark Suite
//=============================================================================

class JobSystemBenchmarkSuite {
private:
    std::unique_ptr<JobSystem> job_system_;
    std::unique_ptr<JobProfiler> profiler_;
    std::unique_ptr<PerformanceComparator> comparator_;
    
public:
    explicit JobSystemBenchmarkSuite(const JobSystem::Config& config = JobSystem::Config::create_performance_optimized())
        : job_system_(std::make_unique<JobSystem>(config)) {
        
        job_system_->initialize();
        
        JobProfiler::Config profiler_config = JobProfiler::Config::create_comprehensive();
        profiler_ = std::make_unique<JobProfiler>(profiler_config);
        
        comparator_ = std::make_unique<PerformanceComparator>(job_system_.get());
    }
    
    void run_comprehensive_benchmarks() {
        LOG_INFO("=== Job System Performance Benchmark Suite ===");
        
        profiler_->start_profiling();
        
        // Benchmark 1: Parallel For Performance
        benchmark_parallel_for();
        
        // Benchmark 2: Work Stealing Effectiveness
        benchmark_work_stealing();
        
        // Benchmark 3: Job Dependency Graph Performance
        benchmark_dependency_graph();
        
        // Benchmark 4: SIMD Integration Performance
        benchmark_simd_integration();
        
        // Benchmark 5: ECS System Parallelization
        benchmark_ecs_parallelization();
        
        profiler_->stop_profiling();
        
        // Generate comprehensive report
        generate_benchmark_report();
    }
    
private:
    void benchmark_parallel_for() {
        LOG_INFO("Running Parallel For benchmarks...");
        
        const std::vector<usize> work_sizes = {1000, 10000, 100000, 1000000};
        const std::vector<usize> grain_sizes = {100, 1000, 10000};
        
        for (usize work_size : work_sizes) {
            for (usize grain_size : grain_sizes) {
                std::ostringstream test_name_oss;
                test_name_oss << "ParallelFor_" << work_size << "_" << grain_size;
                std::string test_name = test_name_oss.str();
                
                // Sequential version
                auto sequential_func = [work_size]() {
                    std::vector<f64> data(work_size);
                    for (usize i = 0; i < work_size; ++i) {
                        data[i] = std::sin(static_cast<f64>(i)) * std::cos(static_cast<f64>(i));
                    }
                };
                
                // Parallel version
                auto parallel_func = [&, work_size, grain_size]() {
                    std::vector<f64> data(work_size);
                    job_system_->parallel_for(0, work_size, [&](usize i) {
                        data[i] = std::sin(static_cast<f64>(i)) * std::cos(static_cast<f64>(i));
                    }, grain_size);
                };
                
                comparator_->benchmark_workload(test_name, sequential_func, parallel_func, 5);
            }
        }
    }
    
    void benchmark_work_stealing() {
        LOG_INFO("Running Work Stealing benchmarks...");
        
        // Create uneven workload to test work stealing effectiveness
        const usize total_jobs = 10000;
        const usize heavy_job_count = 100;
        
        std::string test_name = "WorkStealing_UnevenLoad";
        
        // Submit jobs with varying computational load
        auto parallel_func = [&]() {
            std::vector<JobID> jobs;
            jobs.reserve(total_jobs);
            
            // Submit light jobs
            for (usize i = 0; i < total_jobs - heavy_job_count; ++i) {
                std::ostringstream light_job_name_oss;
                light_job_name_oss << "LightJob_" << i;
                auto job_id = job_system_->submit_job(
                    light_job_name_oss.str(),
                    []() {
                        // Light computational work
                        volatile f64 result = 0.0;
                        for (int j = 0; j < 1000; ++j) {
                            result += std::sin(j);
                        }
                    }
                );
                jobs.push_back(job_id);
            }
            
            // Submit heavy jobs
            for (usize i = 0; i < heavy_job_count; ++i) {
                std::ostringstream heavy_job_name_oss;
                heavy_job_name_oss << "HeavyJob_" << i;
                auto job_id = job_system_->submit_job(
                    heavy_job_name_oss.str(),
                    []() {
                        // Heavy computational work
                        volatile f64 result = 0.0;
                        for (int j = 0; j < 100000; ++j) {
                            result += std::sin(j) * std::cos(j) * std::tan(j);
                        }
                    }
                );
                jobs.push_back(job_id);
            }
            
            // Wait for all jobs to complete
            job_system_->wait_for_batch(jobs);
        };
        
        // Time the parallel execution
        auto start_time = std::chrono::high_resolution_clock::now();
        parallel_func();
        auto end_time = std::chrono::high_resolution_clock::now();
        
        f64 execution_time = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        LOG_INFO("Work stealing benchmark completed in {:.2f}ms", execution_time);
    }
    
    void benchmark_dependency_graph() {
        LOG_INFO("Running Dependency Graph benchmarks...");
        
        // Create a complex dependency graph and measure execution time
        const usize job_count = 1000;
        const usize dependency_chains = 10;
        
        std::vector<JobID> all_jobs;
        all_jobs.reserve(job_count);
        
        // Create dependency chains
        for (usize chain = 0; chain < dependency_chains; ++chain) {
            std::vector<JobID> chain_jobs;
            
            for (usize i = 0; i < job_count / dependency_chains; ++i) {
                std::ostringstream chain_job_name_oss;
                chain_job_name_oss << "ChainJob_" << chain << "_" << i;
                std::string job_name = chain_job_name_oss.str();
                
                auto job_func = []() {
                    // Simulate work
                    volatile f64 result = 0.0;
                    for (int j = 0; j < 10000; ++j) {
                        result += std::sin(j);
                    }
                };
                
                JobID job_id;
                if (i == 0) {
                    // First job in chain has no dependencies
                    job_id = job_system_->submit_job(job_name, job_func);
                } else {
                    // Each job depends on the previous one in the chain
                    job_id = job_system_->submit_job_with_dependencies(
                        job_name, job_func, {chain_jobs.back()});
                }
                
                chain_jobs.push_back(job_id);
                all_jobs.push_back(job_id);
            }
        }
        
        // Measure execution time
        auto start_time = std::chrono::high_resolution_clock::now();
        job_system_->wait_for_batch(all_jobs);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        f64 execution_time = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        LOG_INFO("Dependency graph benchmark completed in {:.2f}ms with {} jobs", 
                execution_time, all_jobs.size());
    }
    
    void benchmark_simd_integration() {
        LOG_INFO("Running SIMD Integration benchmarks...");
        
        const usize vector_count = 100000;
        
        // Create test data
        std::vector<physics::Vec2> vectors_a(vector_count);
        std::vector<physics::Vec2> vectors_b(vector_count);
        std::vector<physics::Vec2> results(vector_count);
        
        // Initialize with random data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dist(-100.0f, 100.0f);
        
        for (usize i = 0; i < vector_count; ++i) {
            vectors_a[i] = physics::Vec2(dist(gen), dist(gen));
            vectors_b[i] = physics::Vec2(dist(gen), dist(gen));
        }
        
        // Sequential SIMD operations
        auto sequential_func = [&]() {
            physics::simd::batch_ops::add_vec2_arrays(
                vectors_a.data(), vectors_b.data(), results.data(), vector_count);
        };
        
        // Parallel SIMD operations
        auto parallel_func = [&]() {
            const usize chunk_size = vector_count / job_system_->worker_count();
            
            std::vector<JobID> jobs;
            for (u32 i = 0; i < job_system_->worker_count(); ++i) {
                usize start_idx = i * chunk_size;
                usize end_idx = (i == job_system_->worker_count() - 1) ? 
                               vector_count : (i + 1) * chunk_size;
                
                std::ostringstream simd_job_name_oss;
                simd_job_name_oss << "SIMDJob_" << i;
                auto job_id = job_system_->submit_job(
                    simd_job_name_oss.str(),
                    [&, start_idx, end_idx]() {
                        physics::simd::batch_ops::add_vec2_arrays(
                            vectors_a.data() + start_idx, 
                            vectors_b.data() + start_idx, 
                            results.data() + start_idx, 
                            end_idx - start_idx);
                    }
                );
                jobs.push_back(job_id);
            }
            
            job_system_->wait_for_batch(jobs);
        };
        
        comparator_->benchmark_workload("SIMD_Integration", sequential_func, parallel_func, 10);
    }
    
    void benchmark_ecs_parallelization() {
        LOG_INFO("Running ECS Parallelization benchmarks...");
        
        // Create a test ECS registry with many entities
        const usize entity_count = 50000;
        
        ecs::Registry registry;
        std::vector<ecs::Entity> entities;
        entities.reserve(entity_count);
        
        // Create entities with components
        for (usize i = 0; i < entity_count; ++i) {
            auto entity = registry.create_entity(
                ecs::components::Transform{physics::Vec3(0.0f, 0.0f, 0.0f)},
                DemoVelocity{physics::Vec2(1.0f, 1.0f)},
                DemoParticle{1.0f, 1.0f}
            );
            entities.push_back(entity);
        }
        
        // Sequential ECS update
        auto sequential_func = [&]() {
            registry.for_each<ecs::components::Transform, DemoVelocity>(
                [](ecs::Entity entity, ecs::components::Transform& transform, DemoVelocity& velocity) {
                    transform.position.x += velocity.velocity.x * 0.016f;
                    transform.position.y += velocity.velocity.y * 0.016f;
                    velocity.velocity *= 0.99f;
                });
        };
        
        // Parallel ECS update using job system
        auto parallel_func = [&]() {
            auto entities_with_components = registry.get_entities_with<ecs::components::Transform, DemoVelocity>();
            
            job_system_->parallel_for_each(entities_with_components, [&](ecs::Entity entity) {
                auto* transform = registry.get_component<ecs::components::Transform>(entity);
                auto* velocity = registry.get_component<DemoVelocity>(entity);
                
                if (transform && velocity) {
                    transform->position.x += velocity->velocity.x * 0.016f;
                    transform->position.y += velocity->velocity.y * 0.016f;
                    velocity->velocity *= 0.99f;
                }
            }, 1000);
        };
        
        comparator_->benchmark_workload("ECS_Parallelization", sequential_func, parallel_func, 10);
    }
    
    void generate_benchmark_report() {
        LOG_INFO("=== Benchmark Results ===");
        
        // Generate profiler report
        auto profiler_report = profiler_->generate_report();
        LOG_INFO("Total jobs executed: {}", profiler_report.total_jobs_executed);
        LOG_INFO("Average execution time: {:.2f}ms", profiler_report.average_execution_time_ms);
        LOG_INFO("Work stealing success rate: {:.1f}%", profiler_report.steal_success_rate * 100.0);
        LOG_INFO("Overall thread utilization: {:.1f}%", profiler_report.overall_utilization * 100.0);
        
        // Generate comparison report
        auto comparison_report = comparator_->generate_comparison_report();
        LOG_INFO("Average speedup: {:.2f}x", comparison_report.average_speedup);
        LOG_INFO("Best speedup: {:.2f}x", comparison_report.best_speedup);
        LOG_INFO("Average efficiency: {:.1f}%", comparison_report.average_efficiency);
        
        comparator_->print_comparison_table();
        
        // Export detailed data
        profiler_->export_timeline_data("job_system_timeline.csv");
        profiler_->export_performance_frames("job_system_performance.csv");
        comparator_->export_comparison_data("job_system_benchmarks.csv");
        
        LOG_INFO("Benchmark data exported to CSV files");
    }
};

//=============================================================================
// Interactive Demo Runner
//=============================================================================

class InteractiveDemoRunner {
private:
    DemoConfig config_;
    std::unique_ptr<ecs::Registry> registry_;
    std::unique_ptr<ecs::SystemManager> system_manager_;
    std::unique_ptr<ECSJobSystemIntegrator> integrator_;
    std::unique_ptr<JobProfiler> profiler_;
    std::unique_ptr<EducationalVisualizer> visualizer_;
    
    std::vector<ecs::Entity> demo_entities_;
    
public:
    explicit InteractiveDemoRunner(const DemoConfig& config = DemoConfig{})
        : config_(config) {}
    
    bool initialize() {
        LOG_INFO("Initializing Interactive Job System Demo...");
        LOG_INFO("Configuration:");
        LOG_INFO("  Entity Count: {}", config_.entity_count);
        LOG_INFO("  Worker Threads: {}", config_.worker_thread_count == 0 ? 
                "Auto-detect" : std::to_string(config_.worker_thread_count));
        LOG_INFO("  Profiling: {}", config_.enable_profiling ? "Enabled" : "Disabled");
        LOG_INFO("  Educational Output: {}", config_.enable_educational_output ? "Enabled" : "Disabled");
        
        // Create ECS registry and system manager
        registry_ = std::make_unique<ecs::Registry>();
        system_manager_ = std::make_unique<ecs::SystemManager>(registry_.get());
        
        // Create job system integrator
        integrator_ = std::make_unique<ECSJobSystemIntegrator>(system_manager_.get());
        
        if (config_.enable_educational_output) {
            integrator_->configure_for_education();
        } else {
            integrator_->configure_for_performance();
        }
        
        if (!integrator_->initialize()) {
            LOG_ERROR("Failed to initialize Job System Integrator");
            return false;
        }
        
        // Create profiler if enabled
        if (config_.enable_profiling) {
            JobProfiler::Config profiler_config = config_.enable_educational_output ?
                JobProfiler::Config::create_comprehensive() :
                JobProfiler::Config::create_lightweight();
                
            profiler_ = std::make_unique<JobProfiler>(profiler_config);
            
            if (config_.enable_visualization) {
                EducationalVisualizer::Config viz_config;
                visualizer_ = std::make_unique<EducationalVisualizer>(profiler_.get(), viz_config);
            }
        }
        
        // Create demo entities
        create_demo_entities();
        
        // Add demo systems
        add_demo_systems();
        
        // Initialize all systems
        system_manager_->initialize_all_systems();
        
        LOG_INFO("Interactive Demo initialized successfully");
        return true;
    }
    
    void run_demo() {
        if (config_.enable_educational_output) {
            print_educational_introduction();
        }
        
        if (profiler_) {
            profiler_->start_profiling();
        }
        
        if (visualizer_) {
            visualizer_->start_visualization();
        }
        
        // Run demo loop
        const f64 target_framerate = 60.0;
        const f64 frame_time = 1.0 / target_framerate;
        const u64 total_frames = static_cast<u64>(config_.demo_duration_seconds * target_framerate);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto last_frame_time = start_time;
        
        LOG_INFO("Starting demo execution for {:.1f} seconds ({} frames)...", 
                config_.demo_duration_seconds, total_frames);
        
        for (u64 frame = 0; frame < total_frames; ++frame) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            if (profiler_) {
                profiler_->start_frame();
            }
            
            // Execute frame
            system_manager_->execute_frame(frame_time);
            
            if (profiler_) {
                profiler_->end_frame();
            }
            
            // Update visualization
            if (visualizer_ && frame % 10 == 0) { // Update every 10 frames
                visualizer_->update_display();
            }
            
            // Progress reporting
            if (frame % (total_frames / 10) == 0) {
                f64 progress = static_cast<f64>(frame) / total_frames * 100.0;
                LOG_INFO("Demo progress: {:.1f}%", progress);
            }
            
            // Frame rate limiting
            auto frame_end = std::chrono::high_resolution_clock::now();
            auto frame_duration = std::chrono::duration<f64>(frame_end - frame_start).count();
            
            if (frame_duration < frame_time) {
                std::this_thread::sleep_for(
                    std::chrono::duration<f64>(frame_time - frame_duration));
            }
            
            last_frame_time = frame_start;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 total_time = std::chrono::duration<f64>(end_time - start_time).count();
        
        LOG_INFO("Demo completed in {:.2f} seconds", total_time);
        
        if (visualizer_) {
            visualizer_->stop_visualization();
        }
        
        if (profiler_) {
            profiler_->stop_profiling();
        }
        
        // Generate final reports
        generate_final_reports();
    }
    
    void shutdown() {
        if (system_manager_) {
            system_manager_->shutdown_all_systems();
        }
        
        if (integrator_) {
            integrator_->shutdown();
        }
    }
    
private:
    void create_demo_entities() {
        LOG_INFO("Creating {} demo entities...", config_.entity_count);
        
        demo_entities_.reserve(config_.entity_count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_dist(-50.0f, 50.0f);
        std::uniform_real_distribution<f32> vel_dist(-10.0f, 10.0f);
        std::uniform_real_distribution<f32> mass_dist(0.5f, 2.0f);
        std::uniform_int_distribution<u32> material_dist(0, 7);
        
        for (usize i = 0; i < config_.entity_count; ++i) {
            auto entity = registry_->create_entity(
                ecs::components::Transform{
                    physics::Vec3(pos_dist(gen), pos_dist(gen), 0.0f)
                },
                DemoVelocity{
                    physics::Vec2(vel_dist(gen), vel_dist(gen)),
                    0.99f
                },
                DemoParticle{
                    mass_dist(gen),
                    1.0f,
                    physics::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
                    true
                },
                DemoPhysics{
                    physics::Vec2(0.0f, 0.0f),
                    physics::Vec2(0.0f, 0.0f),
                    1.0f / mass_dist(gen)
                },
                DemoRenderData{
                    material_dist(gen),
                    pos_dist(gen) * 0.01f,
                    true
                }
            );
            
            demo_entities_.push_back(entity);
        }
        
        LOG_INFO("Created {} entities", demo_entities_.size());
    }
    
    void add_demo_systems() {
        LOG_INFO("Adding demo systems...");
        
        // Add parallel physics system
        auto* physics_system = system_manager_->add_system<DemoPhysicsSystem>(
            integrator_->job_system(), integrator_->parallel_scheduler());
        physics_system->set_min_entities_for_parallel(100);
        
        // Add parallel rendering system
        auto* rendering_system = system_manager_->add_system<DemoRenderingSystem>(
            integrator_->job_system(), integrator_->parallel_scheduler());
        rendering_system->set_min_entities_for_parallel(200);
        
        LOG_INFO("Added {} systems", system_manager_->system_count());
    }
    
    void print_educational_introduction() {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                    ECScope Work-Stealing Job System Demo                     ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║                                                                              ║\n";
        std::cout << "║  This demonstration showcases advanced parallel processing techniques        ║\n";
        std::cout << "║  in game engine development using a work-stealing job system.               ║\n";
        std::cout << "║                                                                              ║\n";
        std::cout << "║  Key Concepts Demonstrated:                                                  ║\n";
        std::cout << "║  • Work-stealing queues for automatic load balancing                        ║\n";
        std::cout << "║  • Parallel ECS system execution with dependency analysis                   ║\n";
        std::cout << "║  • Cache-friendly memory access patterns                                     ║\n";
        std::cout << "║  • SIMD optimization integration                                             ║\n";
        std::cout << "║  • Real-time performance monitoring and profiling                           ║\n";
        std::cout << "║                                                                              ║\n";
        std::cout << "║  Watch the console output for real-time performance metrics and             ║\n";
        std::cout << "║  educational insights about parallel programming!                           ║\n";
        std::cout << "║                                                                              ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        
        if (visualizer_) {
            visualizer_->print_parallelization_tutorial();
        }
    }
    
    void generate_final_reports() {
        LOG_INFO("=== Final Performance Report ===");
        
        // Job system performance
        if (integrator_) {
            integrator_->print_performance_report();
        }
        
        // Profiler insights
        if (profiler_) {
            auto insights = profiler_->generate_educational_insights();
            profiler_->print_educational_summary();
            
            LOG_INFO("Performance Grade: {}", insights.performance_grade);
            LOG_INFO("Key Takeaways:");
            for (const auto& takeaway : insights.key_takeaways) {
                LOG_INFO("  • {}", takeaway);
            }
        }
        
        // System manager statistics
        LOG_INFO("ECS System Statistics:");
        LOG_INFO("  Total Systems: {}", system_manager_->system_count());
        LOG_INFO("  Total Execution Time: {:.2f}ms", system_manager_->get_total_system_time() * 1000.0);
        LOG_INFO("  Frame Budget Utilization: {:.1f}%", system_manager_->get_frame_budget_utilization() * 100.0);
        
        auto slow_systems = system_manager_->get_slowest_systems(3);
        if (!slow_systems.empty()) {
            LOG_INFO("Slowest Systems:");
            for (const auto& system_name : slow_systems) {
                LOG_INFO("  • {}", system_name);
            }
        }
        
        // Educational insights
        if (config_.enable_educational_output && integrator_) {
            integrator_->demonstrate_parallel_benefits();
        }
        
        LOG_INFO("Demo completed successfully!");
    }
};

//=============================================================================
// Main Demo Application
//=============================================================================

int main(int argc, char* argv[]) {
    // Initialize logging
    LOG_INFO("ECScope Work-Stealing Job System Demo");
    LOG_INFO("Hardware Concurrency: {} threads", std::thread::hardware_concurrency());
    
    try {
        DemoConfig config;
        
        // Parse command line arguments
        if (argc > 1) {
            std::string demo_type(argv[1]);
            if (demo_type == "quick") {
                config = DemoConfig::create_quick_demo();
                LOG_INFO("Running quick demo");
            } else if (demo_type == "comprehensive") {
                config = DemoConfig::create_comprehensive_demo();
                LOG_INFO("Running comprehensive demo");
            } else if (demo_type == "benchmark") {
                config = DemoConfig::create_performance_benchmark();
                LOG_INFO("Running performance benchmark");
            } else {
                LOG_WARN("Unknown demo type '{}', using default", demo_type);
            }
        }
        
        if (config.run_performance_benchmarks) {
            // Run benchmark suite
            LOG_INFO("=== Running Performance Benchmark Suite ===");
            JobSystemBenchmarkSuite benchmark_suite;
            benchmark_suite.run_comprehensive_benchmarks();
        }
        
        // Run interactive demo
        LOG_INFO("=== Running Interactive Demo ===");
        InteractiveDemoRunner demo(config);
        
        if (!demo.initialize()) {
            LOG_ERROR("Failed to initialize demo");
            return 1;
        }
        
        demo.run_demo();
        demo.shutdown();
        
        LOG_INFO("Demo completed successfully!");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Demo failed with exception: {}", e.what());
        return 1;
    } catch (...) {
        LOG_ERROR("Demo failed with unknown exception");
        return 1;
    }
    
    return 0;
}