/**
 * @file performance_laboratory.cpp
 * @brief Interactive Performance Laboratory Demo - ECScope Memory Behavior Laboratory
 * 
 * This demo showcases ECScope's comprehensive performance laboratory, demonstrating
 * the "laboratorio de memoria en movimiento" (memory lab in motion) concept through
 * interactive experiments, real-time visualizations, and educational insights.
 * 
 * Demo Features:
 * - Interactive memory access pattern experiments
 * - Real-time allocator performance comparisons
 * - ECS archetype migration visualization
 * - Educational performance insights and recommendations
 * - Live performance monitoring and analysis
 * 
 * Educational Goals:
 * - Demonstrate impact of memory layout decisions on performance
 * - Show real-world cache behavior and optimization techniques
 * - Provide hands-on experience with different allocation strategies
 * - Illustrate ECS-specific performance characteristics
 * - Generate actionable optimization recommendations
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "performance/performance_lab.hpp"
#include "performance/memory_experiments.hpp"
#include "performance/allocation_benchmarks.hpp"
#include "memory/memory_tracker.hpp"
#include "memory/arena.hpp"
#include "memory/pool.hpp"
#include "ecs/registry.hpp"
#include "ecs/components/transform.hpp"
#include "physics/physics_world.hpp"
#include "renderer/renderer_2d.hpp"
#include "ui/overlay.hpp"
#include "ui/panels/panel_memory.hpp"
#include "core/log.hpp"
#include "core/time.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <memory>
#include <string>
#include <vector>

using namespace ecscope;
using namespace ecscope::performance;
using namespace std::chrono_literals;

/**
 * @brief Interactive console interface for the performance laboratory
 */
class PerformanceLabConsole {
private:
    std::unique_ptr<PerformanceLab> lab_;
    std::shared_ptr<ecs::Registry> ecs_registry_;
    std::shared_ptr<physics::PhysicsWorld> physics_world_;
    std::shared_ptr<renderer::Renderer2D> renderer_;
    bool running_;
    
    // Demo configurations
    struct DemoConfig {
        usize entity_count = 10000;
        usize iterations = 1000;
        f64 duration_seconds = 10.0;
        bool enable_visualization = true;
        bool capture_detailed_metrics = true;
    } demo_config_;
    
public:
    PerformanceLabConsole() : running_(true) {
        // Initialize core systems
        ecs_registry_ = std::make_shared<ecs::Registry>();
        physics_world_ = std::make_shared<physics::PhysicsWorld>();
        renderer_ = std::make_shared<renderer::Renderer2D>();
        
        // Initialize performance laboratory
        lab_ = PerformanceLabFactory::create_educational_lab();
        lab_->set_ecs_registry(ecs_registry_);
        lab_->set_physics_world(physics_world_);
        lab_->set_renderer(renderer_);
        
        if (!lab_->initialize()) {
            LOG_ERROR("Failed to initialize performance laboratory");
            return;
        }
        
        LOG_INFO("Performance Laboratory initialized successfully");
    }
    
    ~PerformanceLabConsole() {
        if (lab_) {
            lab_->shutdown();
        }
    }
    
    void run() {
        print_welcome_message();
        
        while (running_) {
            print_main_menu();
            int choice = get_user_choice();
            handle_menu_choice(choice);
        }
        
        print_farewell_message();
    }

private:
    void print_welcome_message() {
        std::cout << "\n";
        std::cout << "═══════════════════════════════════════════════════════════════════════\n";
        std::cout << "               ECScope Performance Laboratory Demo                      \n";
        std::cout << "                \"Laboratorio de Memoria en Movimiento\"                \n";
        std::cout << "═══════════════════════════════════════════════════════════════════════\n";
        std::cout << "\n";
        std::cout << "Welcome to ECScope's comprehensive memory behavior laboratory!\n";
        std::cout << "This interactive demo demonstrates real-world performance characteristics\n";
        std::cout << "of different memory allocation strategies and ECS design patterns.\n";
        std::cout << "\n";
        std::cout << "Educational Features:\n";
        std::cout << "• Memory Access Pattern Analysis (SoA vs AoS)\n";
        std::cout << "• Allocation Strategy Benchmarks (Arena, Pool, PMR)\n";
        std::cout << "• ECS Archetype Migration Performance\n";
        std::cout << "• Cache Behavior Visualization\n";
        std::cout << "• Real-time Performance Monitoring\n";
        std::cout << "• Optimization Recommendations\n";
        std::cout << "\n";
        std::cout << "Press Enter to continue...";
        std::cin.get();
    }
    
    void print_main_menu() {
        std::cout << "\n";
        std::cout << "┌─────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│                     Performance Laboratory Menu                     │\n";
        std::cout << "├─────────────────────────────────────────────────────────────────────┤\n";
        std::cout << "│  1. Memory Access Pattern Experiments                              │\n";
        std::cout << "│  2. Allocation Strategy Benchmarks                                 │\n";
        std::cout << "│  3. ECS Performance Analysis                                        │\n";
        std::cout << "│  4. Comprehensive Performance Suite                                │\n";
        std::cout << "│  5. Real-time Performance Monitoring                               │\n";
        std::cout << "│  6. Educational Insights & Recommendations                         │\n";
        std::cout << "│  7. Configuration & Settings                                       │\n";
        std::cout << "│  8. Export Results & Reports                                       │\n";
        std::cout << "│  9. Quick Performance Health Check                                 │\n";
        std::cout << "│  0. Exit                                                           │\n";
        std::cout << "└─────────────────────────────────────────────────────────────────────┘\n";
        std::cout << "Choose an option: ";
    }
    
    int get_user_choice() {
        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            return -1;
        }
        std::cin.ignore(); // Consume newline
        return choice;
    }
    
    void handle_menu_choice(int choice) {
        switch (choice) {
            case 1: run_memory_experiments(); break;
            case 2: run_allocation_benchmarks(); break;
            case 3: run_ecs_analysis(); break;
            case 4: run_comprehensive_suite(); break;
            case 5: run_real_time_monitoring(); break;
            case 6: show_insights_and_recommendations(); break;
            case 7: configure_settings(); break;
            case 8: export_results(); break;
            case 9: run_quick_health_check(); break;
            case 0: running_ = false; break;
            default: 
                std::cout << "Invalid choice. Please try again.\n";
                std::this_thread::sleep_for(1s);
                break;
        }
    }
    
    void run_memory_experiments() {
        std::cout << "\n";
        std::cout << "┌─────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│                    Memory Access Pattern Experiments                │\n";
        std::cout << "└─────────────────────────────────────────────────────────────────────┘\n";
        
        auto& memory_experiments = lab_->get_memory_experiments();
        
        // 1. SoA vs AoS Comparison
        std::cout << "\n1. Running Structure of Arrays vs Array of Structures comparison...\n";
        auto soa_aos_result = memory_experiments.run_soa_vs_aos_comparison();
        print_memory_experiment_result("SoA vs AoS", soa_aos_result);
        
        // 2. Cache Behavior Analysis
        std::cout << "\n2. Analyzing cache behavior with different data layouts...\n";
        auto cache_result = memory_experiments.run_cache_behavior_analysis();
        print_memory_experiment_result("Cache Behavior", cache_result);
        
        // 3. Archetype Migration Analysis
        std::cout << "\n3. Measuring ECS archetype migration performance...\n";
        auto migration_result = memory_experiments.run_archetype_migration_analysis();
        print_memory_experiment_result("Archetype Migration", migration_result);
        
        // 4. Memory Bandwidth Analysis
        std::cout << "\n4. Testing memory bandwidth utilization...\n";
        auto bandwidth_result = memory_experiments.run_memory_bandwidth_analysis();
        print_memory_experiment_result("Memory Bandwidth", bandwidth_result);
        
        // Generate insights
        std::cout << "\n" << std::string(70, '═') << "\n";
        std::cout << "EXPERIMENT INSIGHTS:\n";
        std::cout << std::string(70, '═') << "\n";
        
        auto recommendations = memory_experiments.get_memory_optimization_recommendations();
        for (const auto& rec : recommendations) {
            print_recommendation(rec);
        }
        
        wait_for_user_input();
    }
    
    void run_allocation_benchmarks() {
        std::cout << "\n";
        std::cout << "┌─────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│                   Allocation Strategy Benchmarks                   │\n";
        std::cout << "└─────────────────────────────────────────────────────────────────────┘\n";
        
        auto& allocation_benchmarks = lab_->get_allocation_benchmarks();
        
        // Configuration for benchmarks
        AllocationBenchmarkConfig config;
        config.total_allocations = demo_config_.iterations;
        config.duration_seconds = demo_config_.duration_seconds;
        config.measure_fragmentation = true;
        config.measure_cache_performance = true;
        
        std::cout << "\nRunning allocation strategy comparison...\n";
        std::cout << "Iterations: " << config.total_allocations << "\n";
        std::cout << "Duration: " << config.duration_seconds << " seconds\n";
        std::cout << "This may take a moment...\n\n";
        
        // 1. Arena Allocator
        std::cout << "1. Benchmarking Arena Allocator...\n";
        auto arena_result = allocation_benchmarks.run_arena_analysis(config);
        print_allocation_result("Arena", arena_result);
        
        // 2. Pool Allocator
        std::cout << "\n2. Benchmarking Pool Allocator...\n";
        config.pool_block_size = 64; // Typical component size
        auto pool_result = allocation_benchmarks.run_pool_analysis(config);
        print_allocation_result("Pool", pool_result);
        
        // 3. PMR Allocator
        std::cout << "\n3. Benchmarking PMR Allocator...\n";
        auto pmr_result = allocation_benchmarks.run_pmr_analysis(config);
        print_allocation_result("PMR", pmr_result);
        
        // 4. Standard Allocator (baseline)
        std::cout << "\n4. Benchmarking Standard Allocator (baseline)...\n";
        auto standard_result = allocation_benchmarks.run_standard_analysis(config);
        print_allocation_result("Standard", standard_result);
        
        // Comprehensive comparison
        std::cout << "\n" << std::string(70, '═') << "\n";
        std::cout << "ALLOCATION STRATEGY COMPARISON:\n";
        std::cout << std::string(70, '═') << "\n";
        
        print_allocation_comparison({arena_result, pool_result, pmr_result, standard_result});
        
        wait_for_user_input();
    }
    
    void run_ecs_analysis() {
        std::cout << "\n";
        std::cout << "┌─────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│                      ECS Performance Analysis                       │\n";
        std::cout << "└─────────────────────────────────────────────────────────────────────┘\n";
        
        // Set up ECS test scenario
        setup_ecs_test_scenario();
        
        // Run ECS-specific performance tests
        auto& ecs_profiler = lab_->get_ecs_profiler();
        
        std::cout << "\nAnalyzing ECS performance characteristics...\n";
        std::cout << "Entities: " << demo_config_.entity_count << "\n";
        std::cout << "Test duration: " << demo_config_.duration_seconds << " seconds\n\n";
        
        // Start monitoring
        lab_->start_monitoring();
        
        // Simulate ECS workload
        simulate_ecs_workload();
        
        // Stop monitoring and get results
        lab_->stop_monitoring();
        
        auto performance_history = lab_->get_performance_history();
        print_ecs_performance_analysis(performance_history);
        
        wait_for_user_input();
    }
    
    void run_comprehensive_suite() {
        std::cout << "\n";
        std::cout << "┌─────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│                  Comprehensive Performance Suite                   │\n";
        std::cout << "└─────────────────────────────────────────────────────────────────────┘\n";
        
        std::cout << "\nRunning comprehensive performance analysis...\n";
        std::cout << "This will execute all experiments and provide a complete analysis.\n";
        std::cout << "Estimated time: 2-5 minutes depending on system performance.\n";
        std::cout << "\nContinue? (y/N): ";
        
        std::string response;
        std::getline(std::cin, response);
        if (response != "y" && response != "Y") {
            return;
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Run all experiments
        std::vector<std::string> experiments = lab_->get_available_experiments();
        std::cout << "\nFound " << experiments.size() << " available experiments\n";
        
        ExperimentConfig config = PerformanceLabFactory::create_educational_config();
        auto results = lab_->run_experiment_suite(experiments, config);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        std::cout << "\nComprehensive suite completed in " << duration.count() << " seconds\n";
        std::cout << "Generated " << results.size() << " performance reports\n";
        
        // Print summary
        print_comprehensive_summary(results);
        
        wait_for_user_input();
    }
    
    void run_real_time_monitoring() {
        std::cout << "\n";
        std::cout << "┌─────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│                   Real-time Performance Monitoring                 │\n";
        std::cout << "└─────────────────────────────────────────────────────────────────────┘\n";
        
        std::cout << "\nStarting real-time performance monitoring...\n";
        std::cout << "Press 'q' and Enter to stop monitoring\n\n";
        
        lab_->start_monitoring();
        
        // Monitoring loop
        auto start_time = std::chrono::high_resolution_clock::now();
        std::string input;
        
        while (input != "q") {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);
            
            // Get current snapshot
            auto snapshot = lab_->get_current_snapshot();
            
            // Clear screen (simple approach)
            std::cout << "\033[2J\033[H";
            
            // Print real-time data
            std::cout << "Real-time Performance Monitor (Elapsed: " << elapsed.count() << "s)\n";
            std::cout << std::string(60, '=') << "\n";
            print_performance_snapshot(snapshot);
            
            std::cout << "\nPress 'q' and Enter to stop: ";
            
            // Non-blocking input check
            std::this_thread::sleep_for(1s);
            if (std::cin.rdbuf()->in_avail()) {
                std::getline(std::cin, input);
            }
        }
        
        lab_->stop_monitoring();
        std::cout << "\nMonitoring stopped.\n";
        
        wait_for_user_input();
    }
    
    void show_insights_and_recommendations() {
        std::cout << "\n";
        std::cout << "┌─────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│                Educational Insights & Recommendations               │\n";
        std::cout << "└─────────────────────────────────────────────────────────────────────┘\n";
        
        // Get current recommendations
        auto recommendations = lab_->get_current_recommendations();
        auto insights = lab_->get_current_insights();
        
        if (recommendations.empty() && insights.empty()) {
            std::cout << "\nNo performance data available yet.\n";
            std::cout << "Run some experiments first to generate insights and recommendations.\n";
        } else {
            // Print insights
            if (!insights.empty()) {
                std::cout << "\nCURRENT PERFORMANCE INSIGHTS:\n";
                std::cout << std::string(40, '-') << "\n";
                for (const auto& insight : insights) {
                    std::cout << "• " << insight << "\n";
                }
            }
            
            // Print recommendations
            if (!recommendations.empty()) {
                std::cout << "\nOPTIMIZATION RECOMMENDATIONS:\n";
                std::cout << std::string(40, '-') << "\n";
                for (const auto& rec : recommendations) {
                    print_recommendation(rec);
                }
            }
            
            // Educational explanations
            std::cout << "\nEDUCATIONAL EXPLANATIONS:\n";
            std::cout << std::string(40, '-') << "\n";
            auto explanations = lab_->get_available_explanations();
            for (const auto& topic : explanations) {
                std::cout << "\n" << topic << ":\n";
                std::cout << lab_->get_explanation(topic) << "\n";
            }
        }
        
        wait_for_user_input();
    }
    
    void configure_settings() {
        std::cout << "\n";
        std::cout << "┌─────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│                    Configuration & Settings                         │\n";
        std::cout << "└─────────────────────────────────────────────────────────────────────┘\n";
        
        std::cout << "\nCurrent Configuration:\n";
        std::cout << "Entity Count: " << demo_config_.entity_count << "\n";
        std::cout << "Iterations: " << demo_config_.iterations << "\n";
        std::cout << "Duration: " << demo_config_.duration_seconds << " seconds\n";
        std::cout << "Visualization: " << (demo_config_.enable_visualization ? "Enabled" : "Disabled") << "\n";
        std::cout << "Detailed Metrics: " << (demo_config_.capture_detailed_metrics ? "Enabled" : "Disabled") << "\n";
        
        std::cout << "\nModify settings:\n";
        std::cout << "1. Change entity count\n";
        std::cout << "2. Change iteration count\n";
        std::cout << "3. Change test duration\n";
        std::cout << "4. Toggle visualization\n";
        std::cout << "5. Toggle detailed metrics\n";
        std::cout << "0. Back to main menu\n";
        
        int choice = get_user_choice();
        
        switch (choice) {
            case 1: {
                std::cout << "Enter new entity count: ";
                std::cin >> demo_config_.entity_count;
                std::cin.ignore();
                break;
            }
            case 2: {
                std::cout << "Enter new iteration count: ";
                std::cin >> demo_config_.iterations;
                std::cin.ignore();
                break;
            }
            case 3: {
                std::cout << "Enter new duration (seconds): ";
                std::cin >> demo_config_.duration_seconds;
                std::cin.ignore();
                break;
            }
            case 4:
                demo_config_.enable_visualization = !demo_config_.enable_visualization;
                std::cout << "Visualization " << (demo_config_.enable_visualization ? "enabled" : "disabled") << "\n";
                break;
            case 5:
                demo_config_.capture_detailed_metrics = !demo_config_.capture_detailed_metrics;
                std::cout << "Detailed metrics " << (demo_config_.capture_detailed_metrics ? "enabled" : "disabled") << "\n";
                break;
        }
    }
    
    void export_results() {
        std::cout << "\n";
        std::cout << "┌─────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│                       Export Results & Reports                     │\n";
        std::cout << "└─────────────────────────────────────────────────────────────────────┘\n";
        
        auto timestamp = std::time(nullptr);
        auto local_time = *std::localtime(&timestamp);
        
        std::ostringstream oss;
        oss << std::put_time(&local_time, "%Y%m%d_%H%M%S");
        std::string time_str = oss.str();
        
        std::cout << "\nExporting performance data...\n";
        
        try {
            // Export JSON results
            std::string json_filename = "performance_results_" + time_str + ".json";
            lab_->export_results_to_json(json_filename);
            std::cout << "✓ Results exported to: " << json_filename << "\n";
            
            // Export performance report
            std::string report_filename = "performance_report_" + time_str + ".txt";
            lab_->export_performance_report(report_filename);
            std::cout << "✓ Performance report exported to: " << report_filename << "\n";
            
            // Export recommendations
            std::string rec_filename = "recommendations_" + time_str + ".txt";
            lab_->export_recommendations_report(rec_filename);
            std::cout << "✓ Recommendations exported to: " << rec_filename << "\n";
            
            std::cout << "\nExport completed successfully!\n";
        } catch (const std::exception& e) {
            std::cout << "Export failed: " << e.what() << "\n";
        }
        
        wait_for_user_input();
    }
    
    void run_quick_health_check() {
        std::cout << "\n";
        std::cout << "┌─────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│                    Quick Performance Health Check                   │\n";
        std::cout << "└─────────────────────────────────────────────────────────────────────┘\n";
        
        std::cout << "\nRunning quick performance health check...\n";
        
        // System integration validation
        bool integration_ok = lab_->validate_system_integration();
        std::cout << "System Integration: " << (integration_ok ? "✓ OK" : "✗ ISSUES") << "\n";
        
        // Memory efficiency estimate
        f64 memory_efficiency = lab_->estimate_memory_efficiency();
        std::cout << "Memory Efficiency: " << std::fixed << std::setprecision(1) 
                  << (memory_efficiency * 100.0) << "% ";
        print_health_status(memory_efficiency);
        
        // ECS performance estimate
        f64 ecs_performance = lab_->estimate_ecs_performance();
        std::cout << "ECS Performance: " << std::fixed << std::setprecision(1) 
                  << (ecs_performance * 100.0) << "% ";
        print_health_status(ecs_performance);
        
        // Overall health score
        f64 health_score = lab_->estimate_overall_health_score();
        std::cout << "Overall Health: " << std::fixed << std::setprecision(1) 
                  << (health_score * 100.0) << "% ";
        print_health_status(health_score);
        
        std::cout << "\n";
        if (health_score < 0.7) {
            std::cout << "⚠️  Performance issues detected. Consider running comprehensive analysis.\n";
        } else if (health_score < 0.9) {
            std::cout << "ℹ️  Performance is acceptable but could be improved.\n";
        } else {
            std::cout << "✅ Excellent performance characteristics!\n";
        }
        
        wait_for_user_input();
    }
    
    // Helper functions
    void print_memory_experiment_result(const std::string& name, const auto& result) {
        std::cout << "\n" << name << " Results:\n";
        std::cout << std::string(30, '-') << "\n";
        std::cout << "Total Time: " << std::fixed << std::setprecision(2) << result.total_time_ms << " ms\n";
        std::cout << "Time per Element: " << std::fixed << std::setprecision(2) << result.time_per_element_ns << " ns\n";
        std::cout << "Memory Bandwidth: " << std::fixed << std::setprecision(2) << result.memory_bandwidth_gbps << " GB/s\n";
        std::cout << "Cache Efficiency: " << std::fixed << std::setprecision(1) << (result.cache_efficiency * 100.0) << "%\n";
        
        if (!result.key_observations.empty()) {
            std::cout << "Key Observations:\n";
            for (const auto& obs : result.key_observations) {
                std::cout << "  • " << obs << "\n";
            }
        }
    }
    
    void print_allocation_result(const std::string& name, const auto& result) {
        std::cout << name << " Allocator Results:\n";
        std::cout << std::string(25, '-') << "\n";
        std::cout << "Allocation Rate: " << std::fixed << std::setprecision(0) << result.allocations_per_second << " allocs/sec\n";
        std::cout << "Average Time: " << std::fixed << std::setprecision(2) << result.average_allocation_time_ns << " ns\n";
        std::cout << "Memory Efficiency: " << std::fixed << std::setprecision(1) << (result.memory_efficiency * 100.0) << "%\n";
        std::cout << "Fragmentation: " << std::fixed << std::setprecision(1) << (result.fragmentation_ratio * 100.0) << "%\n";
    }
    
    void print_allocation_comparison(const std::vector<auto>& results) {
        std::cout << std::setw(12) << "Allocator" 
                  << std::setw(15) << "Rate (K/sec)" 
                  << std::setw(15) << "Avg Time (ns)" 
                  << std::setw(15) << "Efficiency %" 
                  << std::setw(15) << "Fragmentation %" << "\n";
        std::cout << std::string(72, '-') << "\n";
        
        for (const auto& result : results) {
            std::cout << std::setw(12) << result.allocator_name
                      << std::setw(15) << std::fixed << std::setprecision(1) << (result.allocations_per_second / 1000.0)
                      << std::setw(15) << std::fixed << std::setprecision(1) << result.average_allocation_time_ns
                      << std::setw(15) << std::fixed << std::setprecision(1) << (result.memory_efficiency * 100.0)
                      << std::setw(15) << std::fixed << std::setprecision(1) << (result.fragmentation_ratio * 100.0) << "\n";
        }
    }
    
    void print_recommendation(const PerformanceRecommendation& rec) {
        std::cout << "\n📋 " << rec.title << "\n";
        std::cout << "Priority: " << priority_to_string(rec.priority) << "\n";
        std::cout << "Category: " << category_to_string(rec.category) << "\n";
        std::cout << "Estimated Improvement: " << std::fixed << std::setprecision(1) 
                  << rec.estimated_improvement << "%\n";
        std::cout << "Description: " << rec.description << "\n";
        
        if (!rec.implementation_steps.empty()) {
            std::cout << "Implementation Steps:\n";
            for (const auto& step : rec.implementation_steps) {
                std::cout << "  • " << step << "\n";
            }
        }
    }
    
    void print_performance_snapshot(const SystemPerformanceSnapshot& snapshot) {
        std::cout << "CPU Usage: " << std::fixed << std::setprecision(1) << snapshot.cpu_usage_percent << "%\n";
        std::cout << "Memory Usage: " << format_bytes(snapshot.memory_usage_bytes) << "\n";
        std::cout << "Frame Time: " << std::fixed << std::setprecision(2) << snapshot.frame_time_ms << " ms\n";
        std::cout << "FPS: " << std::fixed << std::setprecision(1) << snapshot.fps << "\n";
        std::cout << "Active Entities: " << snapshot.entity_count << "\n";
        std::cout << "Archetypes: " << snapshot.archetype_count << "\n";
        std::cout << "ECS Update Time: " << std::fixed << std::setprecision(2) << snapshot.ecs_update_time_ms << " ms\n";
    }
    
    void print_health_status(f64 score) {
        if (score >= 0.9) std::cout << "✅ Excellent\n";
        else if (score >= 0.7) std::cout << "✓ Good\n";
        else if (score >= 0.5) std::cout << "⚠️ Fair\n";
        else std::cout << "❌ Poor\n";
    }
    
    std::string priority_to_string(PerformanceRecommendation::Priority p) {
        switch (p) {
            case PerformanceRecommendation::Priority::Critical: return "🔴 Critical";
            case PerformanceRecommendation::Priority::High: return "🟠 High";
            case PerformanceRecommendation::Priority::Medium: return "🟡 Medium";
            case PerformanceRecommendation::Priority::Low: return "🟢 Low";
            default: return "Unknown";
        }
    }
    
    std::string category_to_string(PerformanceRecommendation::Category c) {
        switch (c) {
            case PerformanceRecommendation::Category::Memory: return "Memory";
            case PerformanceRecommendation::Category::ECS: return "ECS";
            case PerformanceRecommendation::Category::Physics: return "Physics";
            case PerformanceRecommendation::Category::Rendering: return "Rendering";
            case PerformanceRecommendation::Category::Integration: return "Integration";
            case PerformanceRecommendation::Category::Algorithm: return "Algorithm";
            default: return "Unknown";
        }
    }
    
    std::string format_bytes(usize bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unit_idx = 0;
        f64 size = static_cast<f64>(bytes);
        
        while (size >= 1024.0 && unit_idx < 3) {
            size /= 1024.0;
            unit_idx++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << size << " " << units[unit_idx];
        return oss.str();
    }
    
    void setup_ecs_test_scenario() {
        // Create test entities with various component combinations
        for (usize i = 0; i < demo_config_.entity_count; ++i) {
            auto entity = ecs_registry_->create();
            
            // Add transform component to all entities
            ecs_registry_->add_component<ecs::Transform>(entity, {
                static_cast<f32>(i % 1000), 
                static_cast<f32>(i / 1000), 
                0.0f
            });
            
            // Add additional components based on patterns
            if (i % 3 == 0) {
                // Add velocity component
                // ecs_registry_->add_component<VelocityComponent>(entity, {1.0f, 1.0f, 0.0f});
            }
            
            if (i % 5 == 0) {
                // Add physics component
                // ecs_registry_->add_component<PhysicsComponent>(entity, {1.0f, false});
            }
        }
        
        std::cout << "Created " << demo_config_.entity_count << " entities for ECS testing\n";
    }
    
    void simulate_ecs_workload() {
        auto start_time = std::chrono::high_resolution_clock::now();
        auto end_time = start_time + std::chrono::seconds(static_cast<int>(demo_config_.duration_seconds));
        
        while (std::chrono::high_resolution_clock::now() < end_time) {
            // Simulate frame processing
            ecs_registry_->update(0.016f); // 60 FPS target
            
            // Simulate some archetype migrations
            if (rand() % 100 < 5) { // 5% chance per frame
                auto entities = ecs_registry_->view<ecs::Transform>().entities();
                if (!entities.empty()) {
                    auto entity = entities[rand() % entities.size()];
                    // Randomly add/remove components to trigger migrations
                    // This would trigger archetype changes in a real scenario
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
    }
    
    void print_ecs_performance_analysis(const std::vector<SystemPerformanceSnapshot>& history) {
        if (history.empty()) {
            std::cout << "No performance data collected\n";
            return;
        }
        
        // Calculate averages
        f64 avg_frame_time = 0.0;
        f64 avg_ecs_time = 0.0;
        u32 total_migrations = 0;
        
        for (const auto& snapshot : history) {
            avg_frame_time += snapshot.frame_time_ms;
            avg_ecs_time += snapshot.ecs_update_time_ms;
            total_migrations += snapshot.component_migrations;
        }
        
        avg_frame_time /= history.size();
        avg_ecs_time /= history.size();
        
        std::cout << "\nECS Performance Analysis:\n";
        std::cout << std::string(30, '=') << "\n";
        std::cout << "Average Frame Time: " << std::fixed << std::setprecision(2) << avg_frame_time << " ms\n";
        std::cout << "Average ECS Update Time: " << std::fixed << std::setprecision(2) << avg_ecs_time << " ms\n";
        std::cout << "ECS Overhead: " << std::fixed << std::setprecision(1) 
                  << ((avg_ecs_time / avg_frame_time) * 100.0) << "%\n";
        std::cout << "Total Component Migrations: " << total_migrations << "\n";
        std::cout << "Migration Rate: " << std::fixed << std::setprecision(2) 
                  << (static_cast<f64>(total_migrations) / demo_config_.duration_seconds) << " migrations/sec\n";
    }
    
    void print_comprehensive_summary(const std::vector<BenchmarkResult>& results) {
        std::cout << "\n" << std::string(70, '═') << "\n";
        std::cout << "COMPREHENSIVE PERFORMANCE SUMMARY\n";
        std::cout << std::string(70, '═') << "\n";
        
        f64 total_efficiency = 0.0;
        usize valid_results = 0;
        
        for (const auto& result : results) {
            if (result.is_valid) {
                std::cout << "\n📊 " << result.name << "\n";
                std::cout << "   Efficiency: " << std::fixed << std::setprecision(1) 
                          << (result.efficiency_score * 100.0) << "%\n";
                std::cout << "   Throughput: " << std::fixed << std::setprecision(1) 
                          << result.throughput << " ops/sec\n";
                std::cout << "   Memory Usage: " << format_bytes(result.memory_usage_bytes) << "\n";
                
                if (!result.insights.empty()) {
                    std::cout << "   Key Insight: " << result.insights[0] << "\n";
                }
                
                total_efficiency += result.efficiency_score;
                valid_results++;
            }
        }
        
        if (valid_results > 0) {
            f64 overall_efficiency = total_efficiency / valid_results;
            std::cout << "\n" << std::string(40, '-') << "\n";
            std::cout << "Overall System Efficiency: " << std::fixed << std::setprecision(1) 
                      << (overall_efficiency * 100.0) << "%\n";
            
            if (overall_efficiency >= 0.9) {
                std::cout << "✅ Excellent performance across all systems!\n";
            } else if (overall_efficiency >= 0.7) {
                std::cout << "✓ Good performance with room for optimization\n";
            } else {
                std::cout << "⚠️ Performance optimization recommended\n";
            }
        }
    }
    
    void print_farewell_message() {
        std::cout << "\n";
        std::cout << "═══════════════════════════════════════════════════════════════════════\n";
        std::cout << "Thank you for exploring ECScope's Performance Laboratory!\n";
        std::cout << "\n";
        std::cout << "Key Takeaways:\n";
        std::cout << "• Memory layout decisions have measurable performance impact\n";
        std::cout << "• Different allocators excel in different scenarios\n";
        std::cout << "• Cache-friendly data structures significantly improve performance\n";
        std::cout << "• ECS archetype design affects memory access patterns\n";
        std::cout << "• Real-time monitoring helps identify performance bottlenecks\n";
        std::cout << "\n";
        std::cout << "Continue exploring and optimizing your memory-conscious applications!\n";
        std::cout << "═══════════════════════════════════════════════════════════════════════\n";
    }
    
    void wait_for_user_input() {
        std::cout << "\nPress Enter to continue...";
        std::cin.get();
    }
};

int main() {
    try {
        // Initialize logging
        LOG_INFO("Starting ECScope Performance Laboratory Demo");
        
        // Initialize memory tracking
        memory::TrackerConfig tracker_config;
        tracker_config.enable_tracking = true;
        tracker_config.enable_call_stacks = false; // Disable for demo performance
        tracker_config.enable_access_tracking = true;
        tracker_config.enable_heat_mapping = true;
        memory::MemoryTracker::initialize(tracker_config);
        
        // Run the interactive console
        PerformanceLabConsole console;
        console.run();
        
        // Cleanup
        memory::MemoryTracker::shutdown();
        
        LOG_INFO("Performance Laboratory Demo completed successfully");
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        LOG_ERROR("Performance Laboratory Demo failed: " + std::string(e.what()));
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        LOG_ERROR("Performance Laboratory Demo failed with unknown error");
        return 1;
    }
}