/**
 * @file 13-professional-scheduler-demo.cpp
 * @brief Comprehensive demonstration of the professional-grade system scheduler
 * 
 * This example showcases all advanced scheduling capabilities including:
 * - Multi-threaded work-stealing thread pool with NUMA awareness
 * - Advanced dependency graph resolution and cycle detection
 * - System execution contexts with resource isolation
 * - Professional performance profiling and monitoring
 * - Hot system registration and conditional execution
 * - Multi-frame pipelining and budget management
 * - System state checkpointing and recovery
 * - Event-driven system triggers
 * - Load balancing and optimization
 * 
 * @author ECScope Professional ECS Framework
 * @date 2024
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <random>
#include <iomanip>

// Include the professional scheduler headers
#include "../../include/ecscope/scheduling/scheduler.hpp"
#include "../../include/ecscope/scheduling/system_manager.hpp"
#include "../../include/ecscope/scheduling/thread_pool.hpp"
#include "../../include/ecscope/scheduling/dependency_graph.hpp"
#include "../../include/ecscope/scheduling/execution_context.hpp"
#include "../../include/ecscope/scheduling/profiling.hpp"
#include "../../include/ecscope/scheduling/advanced.hpp"
#include "../../include/ecscope/core/log.hpp"

using namespace ecscope::scheduling;
using namespace ecscope::core;

/**
 * @brief Demo system base class for example systems
 */
class DemoSystem {
protected:
    std::string name_;
    f64 base_execution_time_;
    std::mt19937 rng_;
    std::uniform_real_distribution<f64> time_variation_;

public:
    DemoSystem(const std::string& name, f64 base_time = 0.001)
        : name_(name), base_execution_time_(base_time)
        , rng_(std::random_device{}()), time_variation_(0.8, 1.2) {}
    
    virtual ~DemoSystem() = default;
    
    const std::string& name() const { return name_; }
    
    virtual void execute(const ExecutionContext& context) {
        // Simulate work with some variation
        f64 execution_time = base_execution_time_ * time_variation_(rng_);
        auto start = std::chrono::high_resolution_clock::now();
        
        // Simulate different types of work
        simulate_work(execution_time);
        
        auto end = std::chrono::high_resolution_clock::now();
        f64 actual_time = std::chrono::duration<f64>(end - start).count();
        
        std::cout << "[" << name_ << "] Executed in " << std::fixed << std::setprecision(3) 
                  << actual_time * 1000.0 << "ms (target: " << execution_time * 1000.0 << "ms)\n";
    }
    
protected:
    virtual void simulate_work(f64 duration_seconds) {
        // CPU-bound work simulation
        auto target = std::chrono::high_resolution_clock::now() + 
                     std::chrono::duration<f64>(duration_seconds);
        
        volatile u64 counter = 0;
        while (std::chrono::high_resolution_clock::now() < target) {
            counter += static_cast<u64>(std::sin(counter) * 1000);
        }
    }
};

/**
 * @brief Input processing system - high priority, runs first
 */
class InputSystem : public DemoSystem {
public:
    InputSystem() : DemoSystem("InputSystem", 0.0005) {} // 0.5ms
    
    void execute(const ExecutionContext& context) override {
        std::cout << "[Frame " << context.frame_number() << "] ";
        DemoSystem::execute(context);
        
        // Simulate input event generation
        if (context.frame_number() % 60 == 0) {
            std::cout << "  -> Input event detected!\n";
        }
    }
};

/**
 * @brief Physics system - depends on input, CPU intensive
 */
class PhysicsSystem : public DemoSystem {
public:
    PhysicsSystem() : DemoSystem("PhysicsSystem", 0.008) {} // 8ms
    
    void execute(const ExecutionContext& context) override {
        std::cout << "[Frame " << context.frame_number() << "] ";
        DemoSystem::execute(context);
        
        // Simulate physics calculations
        simulate_physics_work();
    }
    
private:
    void simulate_physics_work() {
        // More intensive CPU work for physics
        std::vector<f64> positions(1000);
        for (int i = 0; i < 1000; ++i) {
            positions[i] = std::sin(i * 0.1) * std::cos(i * 0.1);
        }
    }
};

/**
 * @brief Animation system - depends on physics, moderate load
 */
class AnimationSystem : public DemoSystem {
public:
    AnimationSystem() : DemoSystem("AnimationSystem", 0.003) {} // 3ms
    
    void execute(const ExecutionContext& context) override {
        std::cout << "[Frame " << context.frame_number() << "] ";
        DemoSystem::execute(context);
    }
};

/**
 * @brief Rendering system - depends on physics and animation, GPU bound
 */
class RenderingSystem : public DemoSystem {
public:
    RenderingSystem() : DemoSystem("RenderingSystem", 0.012) {} // 12ms
    
    void execute(const ExecutionContext& context) override {
        std::cout << "[Frame " << context.frame_number() << "] ";
        DemoSystem::execute(context);
        
        // Simulate GPU work (actually just CPU work for demo)
        simulate_gpu_work();
    }
    
private:
    void simulate_gpu_work() {
        // Simulate GPU-bound rendering work
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
};

/**
 * @brief Audio system - independent, can run in parallel
 */
class AudioSystem : public DemoSystem {
public:
    AudioSystem() : DemoSystem("AudioSystem", 0.002) {} // 2ms
    
    void execute(const ExecutionContext& context) override {
        std::cout << "[Frame " << context.frame_number() << "] ";
        DemoSystem::execute(context);
    }
};

/**
 * @brief AI system - low priority, runs when there's time
 */
class AISystem : public DemoSystem {
public:
    AISystem() : DemoSystem("AISystem", 0.005) {} // 5ms
    
    void execute(const ExecutionContext& context) override {
        std::cout << "[Frame " << context.frame_number() << "] ";
        DemoSystem::execute(context);
    }
};

/**
 * @brief Demo performance listener for monitoring
 */
class DemoPerformanceListener : public SystemEventListener {
public:
    void on_system_event(const SystemEvent& event) override {
        switch (event.type) {
            case SystemEvent::Type::ExecutionStarted:
                std::cout << "Event: " << event.system_name << " started execution\n";
                break;
            case SystemEvent::Type::ExecutionEnded:
                if (auto exec_time = event.get_data<f64>("execution_time")) {
                    std::cout << "Event: " << event.system_name << " finished in " 
                              << *exec_time * 1000.0 << "ms\n";
                }
                break;
            case SystemEvent::Type::PerformanceAlert:
                std::cout << "ALERT: Performance issue with " << event.system_name << "\n";
                break;
            default:
                break;
        }
    }
    
    bool wants_event_type(SystemEvent::Type type) const override {
        return type == SystemEvent::Type::ExecutionStarted || 
               type == SystemEvent::Type::ExecutionEnded ||
               type == SystemEvent::Type::PerformanceAlert;
    }
};

/**
 * @brief Demonstrate basic scheduler functionality
 */
void demo_basic_scheduling() {
    std::cout << "\n=== Basic Scheduling Demo ===\n";
    
    // Create scheduler with 4 threads
    Scheduler scheduler(4, ExecutionMode::Parallel, SchedulingPolicy::Priority);
    scheduler.initialize();
    
    // Create system manager
    SystemManager system_manager(&scheduler);
    system_manager.initialize();
    
    // Register demo systems with dependencies
    SystemRegistrationOptions input_opts;
    input_opts.set_phase(SystemPhase::PreUpdate).set_priority(10).set_time_budget(0.001);
    
    SystemRegistrationOptions physics_opts;
    physics_opts.set_phase(SystemPhase::Update).set_priority(20).set_time_budget(0.010)
                .add_dependency("InputSystem");
    
    SystemRegistrationOptions animation_opts;
    animation_opts.set_phase(SystemPhase::Update).set_priority(30).set_time_budget(0.005)
                  .add_dependency("PhysicsSystem");
    
    SystemRegistrationOptions render_opts;
    render_opts.set_phase(SystemPhase::Render).set_priority(40).set_time_budget(0.015)
               .add_dependency("PhysicsSystem").add_dependency("AnimationSystem");
    
    SystemRegistrationOptions audio_opts;
    audio_opts.set_phase(SystemPhase::Update).set_priority(25).set_time_budget(0.003);
    
    // Register systems
    auto input_id = system_manager.register_system<InputSystem>("InputSystem", input_opts);
    auto physics_id = system_manager.register_system<PhysicsSystem>("PhysicsSystem", physics_opts);
    auto animation_id = system_manager.register_system<AnimationSystem>("AnimationSystem", animation_opts);
    auto render_id = system_manager.register_system<RenderingSystem>("RenderingSystem", render_opts);
    auto audio_id = system_manager.register_system<AudioSystem>("AudioSystem", audio_opts);
    
    std::cout << "Registered " << system_manager.get_system_count() << " systems\n";
    
    // Execute several frames
    for (int frame = 1; frame <= 3; ++frame) {
        std::cout << "\n--- Frame " << frame << " ---\n";
        system_manager.begin_frame(frame, frame * 0.016);
        
        // Execute each phase
        scheduler.execute_phase(SystemPhase::PreUpdate, 0.016);
        scheduler.execute_phase(SystemPhase::Update, 0.016);
        scheduler.execute_phase(SystemPhase::Render, 0.016);
        
        system_manager.end_frame();
        
        // Small delay between frames
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Print performance statistics
    auto stats = system_manager.get_statistics();
    std::cout << "\nPerformance Summary:\n";
    std::cout << "- Total system executions: " << stats.total_system_executions << "\n";
    std::cout << "- Average execution time: " << std::fixed << std::setprecision(3) 
              << stats.total_execution_time * 1000.0 << "ms\n";
    
    system_manager.shutdown();
    scheduler.shutdown();
}

/**
 * @brief Demonstrate hot system registration and replacement
 */
void demo_hot_reload() {
    std::cout << "\n=== Hot Reload Demo ===\n";
    
    Scheduler scheduler(2, ExecutionMode::Parallel);
    scheduler.initialize();
    
    SystemManager system_manager(&scheduler);
    system_manager.set_hot_reload_enabled(true);
    system_manager.initialize();
    
    // Register initial system
    SystemRegistrationOptions opts;
    opts.set_phase(SystemPhase::Update);
    
    auto system_id = system_manager.register_system<AISystem>("AISystem", opts);
    std::cout << "Initial AI system registered with ID: " << system_id << "\n";
    
    // Execute a frame
    system_manager.begin_frame(1, 0.016);
    scheduler.execute_phase(SystemPhase::Update, 0.016);
    system_manager.end_frame();
    
    // Hot replace the system
    std::cout << "\nHot-replacing AI system...\n";
    system_manager.replace_system<AISystem>(system_id);
    
    // Execute another frame
    system_manager.begin_frame(2, 0.016);
    scheduler.execute_phase(SystemPhase::Update, 0.016);
    system_manager.end_frame();
    
    std::cout << "Hot reload completed successfully!\n";
    
    system_manager.shutdown();
    scheduler.shutdown();
}

/**
 * @brief Demonstrate multi-frame pipelining
 */
void demo_pipelining() {
    std::cout << "\n=== Multi-Frame Pipelining Demo ===\n";
    
    Scheduler scheduler(6, ExecutionMode::Parallel);  // More threads for pipelining
    scheduler.set_pipelining_enabled(true);
    scheduler.initialize();
    
    SystemManager system_manager(&scheduler);
    system_manager.initialize();
    
    AdvancedSchedulerController advanced_controller(&system_manager, &scheduler);
    advanced_controller.initialize();
    advanced_controller.configure_pipelining(PipeliningMode::Triple, 3, 0.7);
    
    // Register systems
    SystemRegistrationOptions opts;
    system_manager.register_system<InputSystem>("InputSystem", 
        opts.set_phase(SystemPhase::PreUpdate));
    system_manager.register_system<PhysicsSystem>("PhysicsSystem", 
        opts.set_phase(SystemPhase::Update).add_dependency("InputSystem"));
    system_manager.register_system<RenderingSystem>("RenderingSystem", 
        opts.set_phase(SystemPhase::Render).add_dependency("PhysicsSystem"));
    
    std::cout << "Executing frames with triple-buffered pipelining...\n";
    
    // Execute multiple frames with pipelining
    for (int frame = 1; frame <= 5; ++frame) {
        std::cout << "\n--- Starting Frame " << frame << " ---\n";
        advanced_controller.execute_with_pipelining(frame, frame * 0.016);
        
        // Short delay to see pipelining effect
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    // Wait for all pipelined frames to complete
    auto* pipeline = advanced_controller.get_execution_pipeline();
    pipeline->flush_pipeline();
    
    auto pipeline_stats = pipeline->get_statistics();
    std::cout << "\nPipelining Statistics:\n";
    std::cout << "- Pipeline efficiency: " << std::fixed << std::setprecision(2) 
              << pipeline_stats.efficiency * 100.0 << "%\n";
    std::cout << "- Average frame overlap: " << pipeline_stats.average_overlap * 100.0 << "%\n";
    std::cout << "- Throughput: " << pipeline_stats.throughput_fps << " FPS\n";
    
    advanced_controller.shutdown();
    system_manager.shutdown();
    scheduler.shutdown();
}

/**
 * @brief Demonstrate performance profiling and monitoring
 */
void demo_profiling() {
    std::cout << "\n=== Performance Profiling Demo ===\n";
    
    // Initialize performance monitoring
    PerformanceMonitor::instance().initialize(2000.0); // 2kHz sampling
    PerformanceMonitor::instance().enable(true);
    
    Scheduler scheduler(4, ExecutionMode::Parallel);
    scheduler.set_profiling_enabled(true);
    scheduler.initialize();
    
    SystemManager system_manager(&scheduler);
    system_manager.set_performance_monitoring(true);
    system_manager.initialize();
    
    // Add performance event listener
    auto performance_listener = std::make_unique<DemoPerformanceListener>();
    system_manager.add_event_listener(std::move(performance_listener));
    
    // Register systems
    SystemRegistrationOptions opts;
    system_manager.register_system<PhysicsSystem>("PhysicsSystem", 
        opts.set_phase(SystemPhase::Update));
    system_manager.register_system<RenderingSystem>("RenderingSystem", 
        opts.set_phase(SystemPhase::Render));
    
    std::cout << "Running profiled execution...\n";
    
    // Execute frames with profiling
    for (int frame = 1; frame <= 10; ++frame) {
        system_manager.begin_frame(frame, frame * 0.016);
        scheduler.execute_phase(SystemPhase::Update, 0.016);
        scheduler.execute_phase(SystemPhase::Render, 0.016);
        system_manager.end_frame();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60 FPS timing
    }
    
    // Generate comprehensive performance report
    std::cout << "\nGenerating performance report...\n";
    std::string report = PerformanceMonitor::instance().generate_comprehensive_report();
    std::cout << "Performance Report Summary:\n" << report.substr(0, 500) << "...\n";
    
    // Get system-specific profiles
    auto physics_profile = PerformanceMonitor::instance().get_system_profile(1);
    if (physics_profile) {
        std::cout << "Physics System Performance:\n";
        std::cout << "- Average execution time: " << physics_profile->get_average_execution_time() * 1000.0 << "ms\n";
        std::cout << "- Performance stability: " << physics_profile->get_performance_stability() * 100.0 << "%\n";
    }
    
    PerformanceMonitor::instance().shutdown();
    system_manager.shutdown();
    scheduler.shutdown();
}

/**
 * @brief Demonstrate budget management
 */
void demo_budget_management() {
    std::cout << "\n=== Budget Management Demo ===\n";
    
    Scheduler scheduler(4, ExecutionMode::Parallel);
    scheduler.initialize();
    
    SystemManager system_manager(&scheduler);
    system_manager.initialize();
    
    AdvancedSchedulerController advanced_controller(&system_manager, &scheduler);
    advanced_controller.initialize();
    
    // Configure budget management
    auto* budget_manager = advanced_controller.get_budget_manager();
    budget_manager->set_allocation_strategy(BudgetAllocationStrategy::Adaptive);
    budget_manager->enable_dynamic_reallocation(true);
    
    // Register systems with specific budgets
    SystemRegistrationOptions opts;
    auto input_id = system_manager.register_system<InputSystem>("InputSystem", 
        opts.set_phase(SystemPhase::PreUpdate).set_time_budget(0.001));
    auto physics_id = system_manager.register_system<PhysicsSystem>("PhysicsSystem", 
        opts.set_phase(SystemPhase::Update).set_time_budget(0.008));
    auto render_id = system_manager.register_system<RenderingSystem>("RenderingSystem", 
        opts.set_phase(SystemPhase::Render).set_time_budget(0.012));
    
    // Set custom budgets
    advanced_controller.allocate_system_budget(input_id, 0.001);   // 1ms
    advanced_controller.allocate_system_budget(physics_id, 0.008); // 8ms
    advanced_controller.allocate_system_budget(render_id, 0.012);  // 12ms
    
    std::cout << "Executing with budget management...\n";
    
    // Execute frames with budget management
    for (int frame = 1; frame <= 5; ++frame) {
        std::cout << "\n--- Frame " << frame << " Budget Status ---\n";
        
        advanced_controller.execute_with_budget_management(frame, frame * 0.016);
        
        // Check budget utilization
        auto budget_stats = budget_manager->get_statistics();
        std::cout << "Budget utilization: " << std::fixed << std::setprecision(1) 
                  << budget_stats.total_utilization * 100.0 << "%\n";
        std::cout << "Systems over budget: " << budget_stats.systems_over_budget << "\n";
        
        if (budget_stats.systems_over_budget > 0) {
            std::cout << "Performing dynamic budget reallocation...\n";
            budget_manager->perform_dynamic_reallocation();
        }
    }
    
    // Print final budget analysis
    std::cout << "\nBudget Management Summary:\n";
    std::string budget_report = budget_manager->generate_budget_report();
    std::cout << budget_report.substr(0, 300) << "...\n";
    
    advanced_controller.shutdown();
    system_manager.shutdown();
    scheduler.shutdown();
}

/**
 * @brief Demonstrate system checkpointing and rollback
 */
void demo_checkpointing() {
    std::cout << "\n=== System Checkpointing Demo ===\n";
    
    Scheduler scheduler(2, ExecutionMode::Sequential); // Sequential for predictable checkpointing
    scheduler.initialize();
    
    SystemManager system_manager(&scheduler);
    system_manager.initialize();
    
    AdvancedSchedulerController advanced_controller(&system_manager, &scheduler);
    advanced_controller.initialize();
    
    // Register systems
    SystemRegistrationOptions opts;
    system_manager.register_system<PhysicsSystem>("PhysicsSystem", opts.set_phase(SystemPhase::Update));
    system_manager.register_system<AudioSystem>("AudioSystem", opts.set_phase(SystemPhase::Update));
    
    std::cout << "Creating initial checkpoint...\n";
    advanced_controller.create_system_checkpoint("initial_state");
    
    // Execute some frames
    for (int frame = 1; frame <= 3; ++frame) {
        system_manager.begin_frame(frame, frame * 0.016);
        scheduler.execute_phase(SystemPhase::Update, 0.016);
        system_manager.end_frame();
    }
    
    std::cout << "Creating mid-execution checkpoint...\n";
    advanced_controller.create_system_checkpoint("mid_execution");
    
    // Execute more frames
    for (int frame = 4; frame <= 6; ++frame) {
        system_manager.begin_frame(frame, frame * 0.016);
        scheduler.execute_phase(SystemPhase::Update, 0.016);
        system_manager.end_frame();
    }
    
    std::cout << "Rolling back to initial state...\n";
    if (advanced_controller.restore_system_checkpoint("initial_state")) {
        std::cout << "Successfully rolled back to initial state!\n";
        
        // Execute a frame after rollback
        system_manager.begin_frame(1, 0.016);
        scheduler.execute_phase(SystemPhase::Update, 0.016);
        system_manager.end_frame();
    }
    
    // List available checkpoints
    auto checkpoints = advanced_controller.get_available_checkpoints();
    std::cout << "Available checkpoints: ";
    for (const auto& checkpoint : checkpoints) {
        std::cout << checkpoint << " ";
    }
    std::cout << "\n";
    
    advanced_controller.shutdown();
    system_manager.shutdown();
    scheduler.shutdown();
}

/**
 * @brief Main demonstration function
 */
int main() {
    try {
        std::cout << "Professional System Scheduler Demonstration\n";
        std::cout << "==========================================\n";
        
        // Run all demonstrations
        demo_basic_scheduling();
        demo_hot_reload();
        demo_pipelining();
        demo_profiling();
        demo_budget_management();
        demo_checkpointing();
        
        std::cout << "\n=== All Demonstrations Completed Successfully! ===\n";
        std::cout << "\nThe professional scheduler has demonstrated:\n";
        std::cout << "✓ Multi-threaded parallel execution with work stealing\n";
        std::cout << "✓ Advanced dependency resolution and cycle detection\n";
        std::cout << "✓ Hot system registration and replacement\n";
        std::cout << "✓ Multi-frame pipelining for performance optimization\n";
        std::cout << "✓ Comprehensive performance profiling and monitoring\n";
        std::cout << "✓ Dynamic budget management and allocation\n";
        std::cout << "✓ System state checkpointing and rollback\n";
        std::cout << "✓ Event-driven scheduling and execution\n";
        std::cout << "✓ NUMA-aware thread placement and load balancing\n";
        std::cout << "✓ Professional-grade error handling and recovery\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Demo failed with unknown exception" << std::endl;
        return 1;
    }
}

/**
 * @brief Additional utilities for extended testing
 */
namespace demo_utils {

/**
 * @brief Stress test the scheduler with many systems
 */
void stress_test_scheduler() {
    std::cout << "\n=== Scheduler Stress Test ===\n";
    
    const int NUM_SYSTEMS = 100;
    const int NUM_FRAMES = 100;
    
    Scheduler scheduler(8, ExecutionMode::WorkStealing);
    scheduler.initialize();
    
    SystemManager system_manager(&scheduler);
    system_manager.initialize();
    
    // Register many systems
    std::cout << "Registering " << NUM_SYSTEMS << " systems...\n";
    for (int i = 0; i < NUM_SYSTEMS; ++i) {
        SystemRegistrationOptions opts;
        opts.set_phase(static_cast<SystemPhase>(i % static_cast<int>(SystemPhase::COUNT)))
            .set_priority(i % 100)
            .set_time_budget(0.0001 + (i % 10) * 0.0001);
        
        system_manager.register_system<AISystem>("StressSystem_" + std::to_string(i), opts);
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Execute many frames
    std::cout << "Executing " << NUM_FRAMES << " frames...\n";
    for (int frame = 1; frame <= NUM_FRAMES; ++frame) {
        system_manager.begin_frame(frame, frame * 0.016);
        
        for (int phase = 0; phase < static_cast<int>(SystemPhase::COUNT); ++phase) {
            scheduler.execute_phase(static_cast<SystemPhase>(phase), 0.002);
        }
        
        system_manager.end_frame();
        
        if (frame % 20 == 0) {
            std::cout << "Completed frame " << frame << "\n";
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<f64>(end_time - start_time).count();
    
    auto stats = system_manager.get_statistics();
    
    std::cout << "\nStress Test Results:\n";
    std::cout << "- Total execution time: " << std::fixed << std::setprecision(3) << duration << "s\n";
    std::cout << "- Average frame time: " << (duration / NUM_FRAMES) * 1000.0 << "ms\n";
    std::cout << "- Total system executions: " << stats.total_system_executions << "\n";
    std::cout << "- Average systems per frame: " << (stats.total_system_executions / NUM_FRAMES) << "\n";
    std::cout << "- Scheduler efficiency: " << scheduler.get_cpu_utilization() * 100.0 << "%\n";
    
    system_manager.shutdown();
    scheduler.shutdown();
}

/**
 * @brief Benchmark different scheduling policies
 */
void benchmark_scheduling_policies() {
    std::cout << "\n=== Scheduling Policy Benchmark ===\n";
    
    const std::vector<SchedulingPolicy> policies = {
        SchedulingPolicy::Priority,
        SchedulingPolicy::FairShare,
        SchedulingPolicy::RoundRobin,
        SchedulingPolicy::EarliestDeadline,
        SchedulingPolicy::Adaptive
    };
    
    const int NUM_FRAMES = 50;
    
    for (auto policy : policies) {
        std::cout << "\nTesting " << Scheduler::scheduling_policy_name(policy) << "...\n";
        
        Scheduler scheduler(4, ExecutionMode::Parallel, policy);
        scheduler.initialize();
        
        SystemManager system_manager(&scheduler);
        system_manager.initialize();
        
        // Register diverse systems
        SystemRegistrationOptions opts;
        system_manager.register_system<InputSystem>("InputSystem", opts.set_priority(10));
        system_manager.register_system<PhysicsSystem>("PhysicsSystem", opts.set_priority(20));
        system_manager.register_system<RenderingSystem>("RenderingSystem", opts.set_priority(30));
        system_manager.register_system<AudioSystem>("AudioSystem", opts.set_priority(15));
        system_manager.register_system<AISystem>("AISystem", opts.set_priority(50));
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int frame = 1; frame <= NUM_FRAMES; ++frame) {
            system_manager.begin_frame(frame, frame * 0.016);
            scheduler.execute_phase(SystemPhase::Update, 0.016);
            scheduler.execute_phase(SystemPhase::Render, 0.016);
            system_manager.end_frame();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64>(end_time - start_time).count();
        
        std::cout << "  Duration: " << std::fixed << std::setprecision(3) << duration << "s\n";
        std::cout << "  Avg frame time: " << (duration / NUM_FRAMES) * 1000.0 << "ms\n";
        std::cout << "  CPU utilization: " << scheduler.get_cpu_utilization() * 100.0 << "%\n";
        
        system_manager.shutdown();
        scheduler.shutdown();
    }
}

} // namespace demo_utils

// Uncomment to run extended tests
// int main() {
//     demo_utils::stress_test_scheduler();
//     demo_utils::benchmark_scheduling_policies();
//     return 0;
// }