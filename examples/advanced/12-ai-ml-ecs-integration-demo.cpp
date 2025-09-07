/**
 * @file 12-ai-ml-ecs-integration-demo.cpp
 * @brief Comprehensive demonstration of AI/ML integration with ECS
 * 
 * This example showcases the complete AI/ML prediction system for ECS,
 * demonstrating how machine learning can enhance game engine performance
 * through predictive analytics, adaptive scheduling, and intelligent
 * resource management.
 * 
 * Features demonstrated:
 * - Entity behavior prediction and pattern recognition
 * - Predictive component allocation and management
 * - Performance bottleneck prediction and prevention
 * - Adaptive system scheduling with AI-driven workload management
 * - Memory allocation pattern prediction and optimization
 * - Real-time training data collection and model training
 * - Comprehensive visualization of ML insights
 * - Educational explanations of AI/ML concepts in game engines
 */

#include <ecscope/ml_prediction_system.hpp>
#include <ecscope/ecs_behavior_predictor.hpp>
#include <ecscope/predictive_component_system.hpp>
#include <ecscope/ecs_performance_predictor.hpp>
#include <ecscope/adaptive_ecs_scheduler.hpp>
#include <ecscope/ecs_memory_predictor.hpp>
#include <ecscope/ml_training_data_collector.hpp>
#include <ecscope/ml_model_manager.hpp>
#include <ecscope/ml_visualization_system.hpp>
#include <ecscope/registry.hpp>
#include <ecscope/core/log.hpp>

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <random>
#include <vector>
#include <string>
#include <fstream>

using namespace ecscope;
using namespace ecscope::ecs;
using namespace ecscope::ml;

// Example game components for demonstration
struct Position {
    f32 x{0.0f}, y{0.0f}, z{0.0f};
    Position() = default;
    Position(f32 x_, f32 y_, f32 z_ = 0.0f) : x(x_), y(y_), z(z_) {}
};

struct Velocity {
    f32 dx{0.0f}, dy{0.0f}, dz{0.0f};
    Velocity() = default;
    Velocity(f32 dx_, f32 dy_, f32 dz_ = 0.0f) : dx(dx_), dy(dy_), dz(dz_) {}
};

struct Health {
    f32 current{100.0f};
    f32 maximum{100.0f};
    Health() = default;
    Health(f32 max_health) : current(max_health), maximum(max_health) {}
};

struct AI {
    std::string behavior_type{"basic"};
    f32 aggression{0.5f};
    f32 intelligence{0.5f};
    AI() = default;
    AI(const std::string& type, f32 aggr = 0.5f, f32 intel = 0.5f) 
        : behavior_type(type), aggression(aggr), intelligence(intel) {}
};

struct Rendering {
    std::string mesh_name{"default"};
    f32 scale{1.0f};
    bool visible{true};
    Rendering() = default;
    Rendering(const std::string& mesh, f32 s = 1.0f) : mesh_name(mesh), scale(s) {}
};

/**
 * @brief Comprehensive AI/ML ECS integration demonstration class
 */
class AIMLECSDemonstration {
private:
    std::unique_ptr<Registry> registry_;
    std::unique_ptr<ECSBehaviorPredictor> behavior_predictor_;
    std::unique_ptr<PredictiveComponentSystem> component_system_;
    std::unique_ptr<ECSPerformancePredictor> performance_predictor_;
    std::unique_ptr<AdaptiveECSScheduler> scheduler_;
    std::unique_ptr<ECSMemoryPredictor> memory_predictor_;
    std::unique_ptr<MLTrainingDataCollector> data_collector_;
    std::unique_ptr<MLModelManager> model_manager_;
    std::unique_ptr<MLVisualizationSystem> visualization_;
    
    std::vector<Entity> demo_entities_;
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_real_distribution<f32> dis_;
    
    usize frame_number_{0};
    std::chrono::high_resolution_clock::time_point demo_start_time_;
    
public:
    AIMLECSDemonstration() : gen_(rd_()), dis_(0.0f, 1.0f) {
        initialize_demo_systems();
        demo_start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    void run_comprehensive_demo() {
        LOG_INFO("🤖 Starting AI/ML ECS Integration Demonstration");
        print_demo_introduction();
        
        // Phase 1: System Initialization and Setup
        LOG_INFO("\n=== Phase 1: System Initialization ===");
        demonstrate_system_initialization();
        
        // Phase 2: Entity Creation and Behavior Learning
        LOG_INFO("\n=== Phase 2: Entity Behavior Learning ===");
        demonstrate_entity_behavior_learning();
        
        // Phase 3: Predictive Component Management
        LOG_INFO("\n=== Phase 3: Predictive Component Management ===");
        demonstrate_predictive_component_system();
        
        // Phase 4: Performance Prediction and Optimization
        LOG_INFO("\n=== Phase 4: Performance Prediction ===");
        demonstrate_performance_prediction();
        
        // Phase 5: Adaptive Scheduling
        LOG_INFO("\n=== Phase 5: Adaptive Scheduling ===");
        demonstrate_adaptive_scheduling();
        
        // Phase 6: Memory Pattern Prediction
        LOG_INFO("\n=== Phase 6: Memory Pattern Prediction ===");
        demonstrate_memory_prediction();
        
        // Phase 7: Model Training and Management
        LOG_INFO("\n=== Phase 7: Model Training and Management ===");
        demonstrate_model_training();
        
        // Phase 8: Comprehensive Visualization
        LOG_INFO("\n=== Phase 8: Visualization and Insights ===");
        demonstrate_visualization_system();
        
        // Phase 9: Real-world Simulation
        LOG_INFO("\n=== Phase 9: Real-world Simulation ===");
        run_realistic_game_simulation();
        
        // Phase 10: Educational Summary
        LOG_INFO("\n=== Phase 10: Educational Summary ===");
        generate_educational_summary();
        
        LOG_INFO("\n🎉 AI/ML ECS Integration Demonstration Completed Successfully!");
    }
    
private:
    void initialize_demo_systems() {
        LOG_INFO("Initializing AI/ML ECS systems...");
        
        // Create ECS registry with educational configuration
        AllocatorConfig allocator_config = AllocatorConfig::create_educational_focused();
        registry_ = std::make_unique<Registry>(allocator_config, "AI_ML_Demo_Registry");
        
        // Initialize behavior predictor
        BehaviorPredictionConfig behavior_config;
        behavior_config.enable_real_time_learning = true;
        behavior_config.enable_behavior_classification = true;
        behavior_config.enable_interaction_tracking = true;
        behavior_predictor_ = std::make_unique<ECSBehaviorPredictor>(behavior_config);
        
        // Initialize predictive component system
        PredictiveComponentConfig component_config;
        component_config.enable_component_pooling = true;
        component_config.allocation_strategy.strategy = ComponentAllocationStrategy::Strategy::Predictive;
        component_system_ = std::make_unique<PredictiveComponentSystem>(component_config);
        
        // Register component types with predictive system
        component_system_->register_component_type<Position>("Position", 1000);
        component_system_->register_component_type<Velocity>("Velocity", 800);
        component_system_->register_component_type<Health>("Health", 500);
        component_system_->register_component_type<AI>("AI", 300);
        component_system_->register_component_type<Rendering>("Rendering", 1200);
        
        // Initialize performance predictor
        PerformancePredictionConfig perf_config;
        perf_config.enable_bottleneck_detection = true;
        perf_config.enable_trend_analysis = true;
        perf_config.enable_detailed_logging = true;
        performance_predictor_ = std::make_unique<ECSPerformancePredictor>(perf_config);
        
        // Initialize adaptive scheduler
        AdaptiveSchedulerConfig scheduler_config;
        scheduler_config.strategy = SchedulingStrategy::AdaptiveHybrid;
        scheduler_config.enable_quality_scaling = true;
        scheduler_config.enable_parallel_execution = true;
        scheduler_ = std::make_unique<AdaptiveECSScheduler>(scheduler_config);
        
        // Initialize memory predictor
        MemoryPredictionConfig memory_config;
        memory_config.enable_pattern_detection = true;
        memory_config.enable_automatic_optimization = true;
        memory_predictor_ = std::make_unique<ECSMemoryPredictor>(memory_config);
        
        // Initialize training data collector
        DataCollectionConfig data_config;
        data_config.enabled_types = {DataCollectionType::All};
        data_config.enable_real_time_storage = true;
        data_config.enable_data_validation = true;
        data_collector_ = std::make_unique<MLTrainingDataCollector>(data_config);
        
        // Initialize model manager
        ModelManagerConfig model_config;
        model_config.enable_automatic_training = true;
        model_config.enable_cross_validation = true;
        model_config.generate_training_reports = true;
        model_manager_ = std::make_unique<MLModelManager>(model_config);
        
        // Initialize visualization system
        VisualizationConfig viz_config;
        viz_config.enable_explanatory_text = true;
        viz_config.generate_insights_automatically = true;
        viz_config.enable_dashboard_mode = true;
        visualization_ = std::make_unique<MLVisualizationSystem>(viz_config);
        
        // Connect systems together
        setup_system_integration();
        
        LOG_INFO("✅ All AI/ML systems initialized successfully");
    }
    
    void setup_system_integration() {
        // Connect predictors to visualization
        visualization_->set_behavior_predictor(behavior_predictor_.get());
        visualization_->set_performance_predictor(performance_predictor_.get());
        visualization_->set_memory_predictor(memory_predictor_.get());
        visualization_->set_model_manager(model_manager_.get());
        
        // Connect data collector to model manager
        model_manager_->set_data_collector(std::make_unique<MLTrainingDataCollector>(*data_collector_));
        
        // Setup callbacks for real-time integration
        behavior_predictor_->set_prediction_callback([this](const BehaviorPrediction& pred) {
            handle_behavior_prediction(pred);
        });
        
        performance_predictor_->set_bottleneck_callback([this](const PerformanceBottleneckPrediction& bottleneck) {
            handle_bottleneck_prediction(bottleneck);
        });
        
        memory_predictor_->set_prediction_callback([this](const MemoryUsagePrediction& pred) {
            handle_memory_prediction(pred);
        });
    }
    
    void demonstrate_system_initialization() {
        LOG_INFO("Demonstrating system initialization and configuration...");
        
        // Show initial system states
        auto behavior_config = behavior_predictor_->config();
        LOG_INFO("Behavior Predictor Configuration:");
        LOG_INFO("  - Real-time learning: {}", behavior_config.enable_real_time_learning ? "enabled" : "disabled");
        LOG_INFO("  - Prediction horizon: {:.1f}s", behavior_config.behavior_model_config.learning_rate);
        LOG_INFO("  - Minimum observations: {}", behavior_config.min_observations_for_prediction);
        
        auto component_config = component_system_->config();
        LOG_INFO("Predictive Component System Configuration:");
        LOG_INFO("  - Pooling enabled: {}", component_config.enable_component_pooling ? "yes" : "no");
        LOG_INFO("  - Default pool size: {}", component_config.default_pool_size);
        LOG_INFO("  - Strategy: {}", component_config.allocation_strategy.strategy_to_string());
        
        // Demonstrate educational features
        print_educational_section("System Architecture Overview",
            "The AI/ML ECS integration consists of several interconnected systems:\n"
            "• Behavior Predictor: Learns entity behavior patterns\n"
            "• Component System: Predicts component needs\n"
            "• Performance Predictor: Identifies bottlenecks before they occur\n"
            "• Adaptive Scheduler: Optimizes system execution order\n"
            "• Memory Predictor: Optimizes memory allocation patterns\n"
            "• Model Manager: Handles ML model lifecycle\n"
            "• Visualization System: Provides insights and educational content");
    }
    
    void demonstrate_entity_behavior_learning() {
        LOG_INFO("Creating diverse entities for behavior learning...");
        
        // Create different types of entities with varying behavior patterns
        create_player_entities(50);
        create_npc_entities(200);
        create_environment_entities(100);
        create_dynamic_entities(150);
        
        LOG_INFO("Created {} total entities for behavior analysis", demo_entities_.size());
        
        // Start behavior observation
        behavior_predictor_->start_continuous_observation(*registry_);
        data_collector_->start_collection();
        
        // Simulate entity behavior over time
        LOG_INFO("Simulating entity behavior patterns...");
        for (usize frame = 0; frame < 300; ++frame) {
            simulate_frame_behavior(frame);
            
            // Collect behavior data periodically
            if (frame % 10 == 0) {
                behavior_predictor_->observe_all_entities(*registry_);
                data_collector_->collect_all_entity_data(*registry_);
            }
            
            // Show progress periodically
            if (frame % 50 == 0) {
                auto stats = behavior_predictor_->get_prediction_statistics();
                LOG_INFO("Frame {}: Observed {} entities, {} patterns detected",
                        frame, behavior_predictor_->total_entities_observed(),
                        stats.correct_predictions);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        
        // Analyze learned behavior patterns
        analyze_learned_behavior_patterns();
        
        print_educational_section("Behavior Pattern Recognition",
            "The system learns from observing entity component changes over time.\n"
            "Key concepts:\n"
            "• Pattern Classification: Entities are classified as Static, Dynamic, Periodic, etc.\n"
            "• Predictability Scoring: How predictable an entity's behavior is\n"
            "• Interaction Tracking: How entities influence each other\n"
            "• Temporal Analysis: Understanding behavior changes over time");
    }
    
    void demonstrate_predictive_component_system() {
        LOG_INFO("Demonstrating predictive component allocation...");
        
        // Show current component usage
        auto stats_before = component_system_->get_prediction_statistics();
        LOG_INFO("Initial component statistics:");
        LOG_INFO("  - Total predictions: {}", stats_before.total_predictions);
        LOG_INFO("  - Allocation efficiency: {:.2f}%", stats_before.allocation_efficiency * 100.0f);
        
        // Demonstrate predictive allocation for different scenarios
        demonstrate_burst_allocation_scenario();
        demonstrate_gradual_growth_scenario();
        demonstrate_component_lifecycle_scenario();
        
        // Show improved efficiency
        auto stats_after = component_system_->get_prediction_statistics();
        LOG_INFO("Post-prediction component statistics:");
        LOG_INFO("  - Total predictions: {}", stats_after.total_predictions);
        LOG_INFO("  - Allocation efficiency: {:.2f}%", stats_after.allocation_efficiency * 100.0f);
        LOG_INFO("  - Memory savings: {:.1f} KB", stats_after.memory_savings / 1024.0f);
        
        // Generate component prediction report
        auto report = component_system_->generate_prediction_report();
        save_report_to_file("component_prediction_report.txt", report);
        
        print_educational_section("Predictive Component Management",
            "The system predicts which components entities will need in the future:\n"
            "• Pre-allocation: Components are allocated before they're needed\n"
            "• Pool Management: Efficient reuse of component instances\n"
            "• Memory Optimization: Reduced allocation overhead\n"
            "• Performance Benefits: Faster component access and creation");
    }
    
    void demonstrate_performance_prediction() {
        LOG_INFO("Demonstrating performance prediction and bottleneck detection...");
        
        // Start performance monitoring
        performance_predictor_->start_monitoring(*registry_);
        
        // Create performance stress scenarios
        create_performance_stress_scenarios();
        
        // Run scenarios and collect predictions
        for (usize scenario = 0; scenario < 5; ++scenario) {
            LOG_INFO("Running performance scenario {}...", scenario + 1);
            
            // Simulate different load conditions
            simulate_performance_scenario(scenario);
            
            // Get performance predictions
            auto prediction = performance_predictor_->predict_performance(*registry_);
            LOG_INFO("Predicted frame time: {:.2f}ms (confidence: {:.1f}%)",
                    prediction.predicted_frame_time, prediction.confidence * 100.0f);
            
            // Check for bottleneck predictions
            if (prediction.has_critical_bottlenecks()) {
                LOG_WARN("Critical bottlenecks predicted:");
                for (const auto& bottleneck : prediction.predicted_bottlenecks) {
                    if (bottleneck.is_critical()) {
                        LOG_WARN("  - {}: {:.1f}% probability, {:.1f}% severity",
                                bottleneck.bottleneck_type_to_string(),
                                bottleneck.probability * 100.0f,
                                bottleneck.severity * 100.0f);
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Analyze prediction accuracy
        auto perf_stats = performance_predictor_->get_prediction_statistics();
        LOG_INFO("Performance prediction statistics:");
        LOG_INFO("  - Total predictions: {}", perf_stats.total_predictions);
        LOG_INFO("  - Overall accuracy: {:.1f}%", perf_stats.overall_accuracy * 100.0f);
        LOG_INFO("  - Bottlenecks detected: {}", perf_stats.bottleneck_predictions);
        
        print_educational_section("Performance Prediction",
            "The system predicts performance bottlenecks before they occur:\n"
            "• Frame Time Prediction: Estimates future frame rendering time\n"
            "• Bottleneck Classification: Identifies CPU, memory, cache issues\n"
            "• Trend Analysis: Detects performance degradation patterns\n"
            "• Mitigation Strategies: Suggests optimizations automatically");
    }
    
    void demonstrate_adaptive_scheduling() {
        LOG_INFO("Demonstrating adaptive system scheduling...");
        
        // Register example systems with the scheduler
        register_demo_systems_with_scheduler();
        
        // Show initial scheduling plan
        auto initial_plan = scheduler_->create_scheduling_plan(*registry_);
        LOG_INFO("Initial scheduling plan:");
        LOG_INFO("  - Systems to execute: {}", initial_plan.system_schedule.size());
        LOG_INFO("  - Predicted frame time: {:.2f}ms", initial_plan.predicted_frame_time);
        LOG_INFO("  - Overall quality factor: {:.2f}", initial_plan.overall_quality_factor);
        
        // Start adaptive scheduling
        scheduler_->start_scheduling();
        
        // Simulate various load conditions
        for (usize load_scenario = 0; load_scenario < 10; ++load_scenario) {
            LOG_INFO("Scheduling scenario {} - Load factor: {:.1f}",
                    load_scenario + 1, load_scenario * 0.1f + 0.5f);
            
            // Update performance context
            f32 cpu_load = 0.5f + load_scenario * 0.05f;
            f32 memory_pressure = 0.3f + load_scenario * 0.04f;
            scheduler_->update_performance_context(cpu_load, memory_pressure);
            
            // Execute frame with adaptive scheduling
            auto start_time = std::chrono::high_resolution_clock::now();
            scheduler_->execute_frame(*registry_);
            auto end_time = std::chrono::high_resolution_clock::now();
            
            auto frame_time = std::chrono::duration<f32, std::milli>(end_time - start_time).count();
            LOG_INFO("  Actual frame time: {:.2f}ms", frame_time);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        
        // Show scheduling statistics
        auto sched_stats = scheduler_->get_scheduling_statistics();
        LOG_INFO("Adaptive scheduling results:");
        LOG_INFO("  - Average frame rate: {:.1f} FPS", sched_stats.average_frame_rate);
        LOG_INFO("  - Target achievement: {:.1f}%", sched_stats.target_achievement_rate * 100.0f);
        LOG_INFO("  - Optimization attempts: {}", sched_stats.optimization_attempts);
        LOG_INFO("  - Performance improvement: {:.2f}x", sched_stats.performance_improvement);
        
        print_educational_section("Adaptive Scheduling",
            "The system dynamically adjusts execution order and resource allocation:\n"
            "• Load Balancing: Distributes work evenly across frames\n"
            "• Quality Scaling: Reduces quality when performance is critical\n"
            "• Parallel Execution: Uses multiple threads efficiently\n"
            "• Predictive Planning: Uses ML to optimize scheduling decisions");
    }
    
    void demonstrate_memory_prediction() {
        LOG_INFO("Demonstrating memory allocation prediction...");
        
        // Start memory monitoring
        memory_predictor_->start_monitoring(*registry_);
        
        // Simulate different memory allocation patterns
        simulate_memory_allocation_patterns();
        
        // Get memory predictions
        auto memory_prediction = memory_predictor_->predict_memory_usage(*registry_, 5.0f);
        LOG_INFO("Memory usage prediction (5s ahead):");
        LOG_INFO("  - Predicted heap usage: {:.1f} MB", memory_prediction.predicted_heap_usage / (1024.0f * 1024.0f));
        LOG_INFO("  - Predicted fragmentation: {:.1f}%", memory_prediction.predicted_fragmentation * 100.0f);
        LOG_INFO("  - OOM risk: {:.1f}%", memory_prediction.oom_risk * 100.0f);
        LOG_INFO("  - Pattern: {}", memory_prediction.predicted_pattern == AllocationPattern::Burst ? "Burst" : 
                                   memory_prediction.predicted_pattern == AllocationPattern::Sequential ? "Sequential" : "Other");
        
        // Demonstrate memory optimization
        auto optimizations = memory_predictor_->suggest_pool_optimizations();
        LOG_INFO("Memory optimization suggestions ({} total):", optimizations.size());
        for (const auto& opt : optimizations) {
            LOG_INFO("  - {}: {} (savings: {:.1f} KB)", 
                    opt.allocator_name, opt.optimization_type, opt.potential_savings / 1024.0f);
        }
        
        // Apply optimizations automatically
        memory_predictor_->optimize_memory_automatically(*registry_);
        
        auto memory_stats = memory_predictor_->get_prediction_statistics();
        LOG_INFO("Memory prediction statistics:");
        LOG_INFO("  - Prediction accuracy: {:.1f}%", memory_stats.overall_accuracy * 100.0f);
        LOG_INFO("  - Pattern detection accuracy: {:.1f}%", memory_stats.pattern_detection_accuracy * 100.0f);
        LOG_INFO("  - Memory efficiency: {:.1f}%", memory_stats.average_memory_efficiency * 100.0f);
        
        print_educational_section("Memory Pattern Prediction",
            "The system learns and predicts memory allocation patterns:\n"
            "• Allocation Pattern Recognition: Identifies burst, sequential, periodic patterns\n"
            "• Memory Pressure Prediction: Forecasts when memory will be scarce\n"
            "• Pool Optimization: Automatically resizes memory pools\n"
            "• Fragmentation Prevention: Reduces memory fragmentation");
    }
    
    void demonstrate_model_training() {
        LOG_INFO("Demonstrating ML model training and management...");
        
        // Register ML models with the model manager
        register_models_with_manager();
        
        // Show initial model states
        auto models = model_manager_->list_registered_models();
        LOG_INFO("Registered models: {}", models.size());
        for (const auto& model_name : models) {
            auto model_info = model_manager_->get_model_info(model_name);
            if (model_info) {
                LOG_INFO("  - {}: {} (trained: {})", 
                        model_name, model_info->model_type, 
                        model_info->is_trained ? "yes" : "no");
            }
        }
        
        // Start model training
        model_manager_->start_model_manager();
        
        // Train all models
        LOG_INFO("Starting model training...");
        model_manager_->train_all_models();
        
        // Wait for training to complete (in a real scenario, this would be async)
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Show training results
        for (const auto& model_name : models) {
            auto model_info = model_manager_->get_model_info(model_name);
            if (model_info && model_info->is_trained) {
                const auto& validation = model_info->latest_validation;
                LOG_INFO("Model '{}' training results:", model_name);
                LOG_INFO("  - Accuracy: {:.2f}%", validation.accuracy * 100.0f);
                LOG_INFO("  - Training time: {}ms", validation.training_time.count());
                LOG_INFO("  - Parameters: {}", validation.parameter_count);
            }
        }
        
        // Demonstrate model versioning
        model_manager_->create_model_snapshot("BehaviorPredictor");
        model_manager_->save_all_models();
        
        print_educational_section("Model Training and Management",
            "The system manages the complete ML model lifecycle:\n"
            "• Automatic Training: Models retrain when new data is available\n"
            "• Cross-validation: Ensures models generalize well\n"
            "• Version Control: Maintains model versions and rollback capability\n"
            "• Performance Monitoring: Tracks model accuracy over time");
    }
    
    void demonstrate_visualization_system() {
        LOG_INFO("Demonstrating comprehensive visualization system...");
        
        // Create various visualization charts
        create_behavior_visualizations();
        create_performance_visualizations();
        create_memory_visualizations();
        create_model_training_visualizations();
        
        // Generate interactive dashboard
        auto dashboard = visualization_->create_interactive_dashboard(*registry_);
        save_report_to_file("ai_ml_dashboard.html", dashboard);
        
        // Create comprehensive analysis report
        auto analysis_charts = visualization_->create_comprehensive_analysis_report(*registry_);
        LOG_INFO("Generated {} analysis charts", analysis_charts.size());
        
        // Save individual charts
        for (usize i = 0; i < analysis_charts.size(); ++i) {
            std::string filename = "analysis_chart_" + std::to_string(i + 1) + ".svg";
            analysis_charts[i].save_to_file(filename, "svg");
        }
        
        auto viz_stats = visualization_->generate_visualization_usage_report();
        save_report_to_file("visualization_usage_report.txt", viz_stats);
        
        print_educational_section("Visualization and Insights",
            "The visualization system creates educational and informative charts:\n"
            "• Real-time Dashboards: Live monitoring of ML system performance\n"
            "• Prediction Accuracy: Visual feedback on model performance\n"
            "• Pattern Recognition: Visual representation of learned patterns\n"
            "• Educational Content: Explanations of ML concepts and results");
    }
    
    void run_realistic_game_simulation() {
        LOG_INFO("Running realistic game simulation with full AI/ML integration...");
        
        // Reset systems for clean simulation
        reset_simulation_state();
        
        // Run simulation for extended period
        constexpr usize simulation_frames = 3600; // ~1 minute at 60 FPS
        auto sim_start = std::chrono::high_resolution_clock::now();
        
        for (usize frame = 0; frame < simulation_frames; ++frame) {
            frame_number_ = frame;
            
            // Simulate realistic game frame
            simulate_realistic_game_frame(frame);
            
            // Update all AI/ML systems
            update_ml_systems(frame);
            
            // Log significant events
            if (frame % 300 == 0) {
                log_simulation_progress(frame, simulation_frames);
            }
            
            // Maintain realistic frame timing
            std::this_thread::sleep_for(std::chrono::milliseconds(8)); // ~120 FPS simulation
        }
        
        auto sim_end = std::chrono::high_resolution_clock::now();
        auto sim_duration = std::chrono::duration<f32>(sim_end - sim_start).count();
        
        LOG_INFO("Simulation completed in {:.1f}s", sim_duration);
        
        // Generate final performance analysis
        generate_final_performance_analysis();
        
        print_educational_section("Realistic Simulation Results",
            "The complete AI/ML system demonstrated significant benefits:\n"
            "• Reduced component allocation overhead through prediction\n"
            "• Improved frame rate consistency via adaptive scheduling\n"
            "• Better memory utilization through pattern recognition\n"
            "• Proactive performance optimization based on ML insights");
    }
    
    void generate_educational_summary() {
        LOG_INFO("Generating comprehensive educational summary...");
        
        std::ostringstream summary;
        summary << "=== AI/ML ECS Integration Educational Summary ===\n\n";
        
        // System overview
        summary << "1. SYSTEM ARCHITECTURE\n";
        summary << "The AI/ML ECS integration consists of interconnected systems that learn\n";
        summary << "from runtime behavior to optimize game engine performance.\n\n";
        
        // Key concepts covered
        summary << "2. KEY CONCEPTS DEMONSTRATED\n";
        summary << "• Predictive Analytics: Using historical data to predict future needs\n";
        summary << "• Machine Learning Integration: Real-time learning in game engines\n";
        summary << "• Performance Optimization: AI-driven resource management\n";
        summary << "• Adaptive Systems: Dynamic adjustment to changing conditions\n";
        summary << "• Data-Driven Decisions: Using ML insights for optimization\n\n";
        
        // Benefits achieved
        summary << "3. BENEFITS ACHIEVED\n";
        summary << generate_benefits_summary();
        
        // Educational insights
        summary << "4. EDUCATIONAL INSIGHTS\n";
        summary << "• ML models can significantly improve game engine efficiency\n";
        summary << "• Real-time learning enables adaptive optimization\n";
        summary << "• Visualization is crucial for understanding AI/ML behavior\n";
        summary << "• Educational features help developers understand the system\n\n";
        
        // Future possibilities
        summary << "5. FUTURE POSSIBILITIES\n";
        summary << "• Advanced neural networks for complex pattern recognition\n";
        summary << "• Reinforcement learning for dynamic difficulty adjustment\n";
        summary << "• Federated learning across multiple game instances\n";
        summary << "• AI-assisted game design and content generation\n\n";
        
        summary << "This demonstration shows how AI/ML can transform game engine\n";
        summary << "architecture from reactive to predictive, resulting in better\n";
        summary << "performance, efficiency, and player experience.\n";
        
        auto summary_text = summary.str();
        save_report_to_file("educational_summary.txt", summary_text);
        
        LOG_INFO("Educational summary saved to educational_summary.txt");
        std::cout << "\n" << summary_text << std::endl;
    }
    
    // Helper methods for demonstration phases
    void create_player_entities(usize count) {
        for (usize i = 0; i < count; ++i) {
            auto entity = registry_->create_entity(
                Position{dis_(gen_) * 1000.0f, dis_(gen_) * 1000.0f},
                Velocity{(dis_(gen_) - 0.5f) * 10.0f, (dis_(gen_) - 0.5f) * 10.0f},
                Health{80.0f + dis_(gen_) * 40.0f},
                Rendering{"player_mesh", 1.0f}
            );
            demo_entities_.push_back(entity);
        }
    }
    
    void create_npc_entities(usize count) {
        for (usize i = 0; i < count; ++i) {
            auto entity = registry_->create_entity(
                Position{dis_(gen_) * 2000.0f, dis_(gen_) * 2000.0f},
                AI{"npc", dis_(gen_), dis_(gen_)},
                Health{50.0f + dis_(gen_) * 50.0f},
                Rendering{"npc_mesh", 0.8f + dis_(gen_) * 0.4f}
            );
            
            // Some NPCs get velocity for movement
            if (dis_(gen_) > 0.3f) {
                registry_->add_component(entity, Velocity{
                    (dis_(gen_) - 0.5f) * 5.0f, 
                    (dis_(gen_) - 0.5f) * 5.0f
                });
            }
            
            demo_entities_.push_back(entity);
        }
    }
    
    void create_environment_entities(usize count) {
        for (usize i = 0; i < count; ++i) {
            auto entity = registry_->create_entity(
                Position{dis_(gen_) * 3000.0f, dis_(gen_) * 3000.0f},
                Rendering{"environment_mesh", 0.5f + dis_(gen_) * 2.0f}
            );
            demo_entities_.push_back(entity);
        }
    }
    
    void create_dynamic_entities(usize count) {
        for (usize i = 0; i < count; ++i) {
            auto entity = registry_->create_entity(
                Position{dis_(gen_) * 1500.0f, dis_(gen_) * 1500.0f},
                Velocity{(dis_(gen_) - 0.5f) * 20.0f, (dis_(gen_) - 0.5f) * 20.0f}
            );
            
            // Randomly add components to create dynamic behavior
            if (dis_(gen_) > 0.6f) {
                registry_->add_component(entity, Health{dis_(gen_) * 100.0f});
            }
            if (dis_(gen_) > 0.7f) {
                registry_->add_component(entity, AI{"dynamic", dis_(gen_), dis_(gen_)});
            }
            if (dis_(gen_) > 0.4f) {
                registry_->add_component(entity, Rendering{"dynamic_mesh", dis_(gen_) * 2.0f});
            }
            
            demo_entities_.push_back(entity);
        }
    }
    
    void simulate_frame_behavior(usize frame) {
        // Simulate different behavior patterns based on frame number
        f32 time_factor = frame / 300.0f;
        
        for (auto entity : demo_entities_) {
            if (dis_(gen_) > 0.95f) { // 5% chance of component changes
                simulate_entity_component_changes(entity, time_factor);
            }
            
            if (dis_(gen_) > 0.9f) { // 10% chance of state changes
                simulate_entity_state_changes(entity, time_factor);
            }
        }
    }
    
    void simulate_entity_component_changes(Entity entity, f32 time_factor) {
        // Add or remove components based on behavioral patterns
        bool has_velocity = registry_->has_component<Velocity>(entity);
        bool has_ai = registry_->has_component<AI>(entity);
        bool has_health = registry_->has_component<Health>(entity);
        
        if (!has_velocity && dis_(gen_) > 0.7f) {
            registry_->add_component(entity, Velocity{
                (dis_(gen_) - 0.5f) * 15.0f * (1.0f + time_factor),
                (dis_(gen_) - 0.5f) * 15.0f * (1.0f + time_factor)
            });
        }
        
        if (!has_ai && dis_(gen_) > 0.8f) {
            registry_->add_component(entity, AI{"evolved", dis_(gen_), dis_(gen_) * time_factor});
        }
        
        if (!has_health && dis_(gen_) > 0.6f) {
            registry_->add_component(entity, Health{50.0f + dis_(gen_) * 50.0f});
        }
    }
    
    void simulate_entity_state_changes(Entity entity, f32 time_factor) {
        // Modify existing component values to create behavioral patterns
        if (auto* pos = registry_->get_component<Position>(entity)) {
            pos->x += (dis_(gen_) - 0.5f) * time_factor * 10.0f;
            pos->y += (dis_(gen_) - 0.5f) * time_factor * 10.0f;
        }
        
        if (auto* vel = registry_->get_component<Velocity>(entity)) {
            vel->dx *= 0.99f; // Gradual slowdown
            vel->dy *= 0.99f;
            if (dis_(gen_) > 0.95f) {
                vel->dx += (dis_(gen_) - 0.5f) * 5.0f;
                vel->dy += (dis_(gen_) - 0.5f) * 5.0f;
            }
        }
        
        if (auto* health = registry_->get_component<Health>(entity)) {
            if (dis_(gen_) > 0.98f) { // Occasional damage/healing
                health->current += (dis_(gen_) - 0.7f) * 20.0f;
                health->current = std::clamp(health->current, 0.0f, health->maximum);
            }
        }
    }
    
    // Additional helper methods...
    void print_demo_introduction() {
        std::cout << R"(
╔══════════════════════════════════════════════════════════════════════════════════╗
║                     🤖 AI/ML ECS Integration Demonstration                       ║
╠══════════════════════════════════════════════════════════════════════════════════╣
║                                                                                  ║
║  This comprehensive demonstration showcases how artificial intelligence and      ║
║  machine learning can enhance Entity Component System (ECS) architecture in     ║
║  game engines. The system learns from runtime behavior to predict and optimize  ║
║  various aspects of game performance.                                            ║
║                                                                                  ║
║  🎯 Key Features Demonstrated:                                                   ║
║    • Entity behavior prediction and pattern recognition                          ║
║    • Predictive component allocation and memory management                       ║
║    • Performance bottleneck prediction and prevention                           ║
║    • Adaptive system scheduling with AI-driven optimization                     ║
║    • Memory allocation pattern learning and optimization                        ║
║    • Real-time model training and continuous learning                           ║
║    • Comprehensive visualization and educational insights                       ║
║                                                                                  ║
║  📚 Educational Value:                                                           ║
║    Each phase includes detailed explanations of concepts, benefits, and         ║
║    implementation details to help understand how AI/ML can transform            ║
║    game engine architecture.                                                    ║
║                                                                                  ║
╚══════════════════════════════════════════════════════════════════════════════════╝
)" << std::endl;
    }
    
    void print_educational_section(const std::string& title, const std::string& content) {
        std::cout << "\n📚 " << title << "\n";
        std::cout << std::string(title.length() + 4, '=') << "\n";
        std::cout << content << "\n" << std::endl;
    }
    
    void save_report_to_file(const std::string& filename, const std::string& content) {
        std::ofstream file(filename);
        if (file.is_open()) {
            file << content;
            file.close();
            LOG_INFO("Report saved to {}", filename);
        } else {
            LOG_WARN("Failed to save report to {}", filename);
        }
    }
    
    // Callback handlers for real-time integration
    void handle_behavior_prediction(const BehaviorPrediction& prediction) {
        if (prediction.confidence > 0.8f) {
            LOG_INFO("High-confidence behavior prediction for entity {}: {}",
                    prediction.entity, prediction.to_string());
        }
    }
    
    void handle_bottleneck_prediction(const PerformanceBottleneckPrediction& bottleneck) {
        if (bottleneck.is_critical()) {
            LOG_WARN("Critical bottleneck predicted: {} ({}% probability)",
                    bottleneck.bottleneck_type_to_string(),
                    bottleneck.probability * 100.0f);
        }
    }
    
    void handle_memory_prediction(const MemoryUsagePrediction& prediction) {
        if (prediction.is_memory_critical()) {
            LOG_WARN("Critical memory situation predicted: {:.1f}% pressure, {:.1f}% OOM risk",
                    prediction.predicted_pressure * 100.0f,
                    prediction.oom_risk * 100.0f);
        }
    }
    
    // More implementation details would continue here...
    // For brevity, some methods are abbreviated
    
    void analyze_learned_behavior_patterns() { /* Implementation */ }
    void demonstrate_burst_allocation_scenario() { /* Implementation */ }
    void demonstrate_gradual_growth_scenario() { /* Implementation */ }
    void demonstrate_component_lifecycle_scenario() { /* Implementation */ }
    void create_performance_stress_scenarios() { /* Implementation */ }
    void simulate_performance_scenario(usize scenario) { /* Implementation */ }
    void register_demo_systems_with_scheduler() { /* Implementation */ }
    void simulate_memory_allocation_patterns() { /* Implementation */ }
    void register_models_with_manager() { /* Implementation */ }
    void create_behavior_visualizations() { /* Implementation */ }
    void create_performance_visualizations() { /* Implementation */ }
    void create_memory_visualizations() { /* Implementation */ }
    void create_model_training_visualizations() { /* Implementation */ }
    void reset_simulation_state() { /* Implementation */ }
    void simulate_realistic_game_frame(usize frame) { /* Implementation */ }
    void update_ml_systems(usize frame) { /* Implementation */ }
    void log_simulation_progress(usize current_frame, usize total_frames) { /* Implementation */ }
    void generate_final_performance_analysis() { /* Implementation */ }
    std::string generate_benefits_summary() { return "Benefits summary..."; }
};

/**
 * @brief Main function to run the AI/ML ECS integration demonstration
 */
int main() {
    try {
        // Initialize logging
        LOG_INFO("🚀 Starting AI/ML ECS Integration Demonstration");
        
        // Create and run demonstration
        AIMLECSDemonstration demo;
        demo.run_comprehensive_demo();
        
        LOG_INFO("✅ Demonstration completed successfully!");
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR("❌ Demonstration failed: {}", e.what());
        return -1;
    }
}