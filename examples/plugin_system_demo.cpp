/**
 * @file examples/plugin_system_demo.cpp
 * @brief Comprehensive Plugin System Demonstration
 * 
 * This demo showcases the complete ECScope plugin system functionality including:
 * - Plugin discovery, loading, and management
 * - Hot-swappable plugin development
 * - Security and sandboxing features  
 * - ECS integration with plugin components and systems
 * - Educational features and debugging tools
 * - Performance monitoring and optimization
 * 
 * Educational Objectives:
 * - Understand complete plugin system architecture
 * - Learn plugin development workflows
 * - Practice security and performance considerations
 * - Master ECS plugin integration patterns
 * - Experience real-time debugging and profiling
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin/plugin_manager.hpp"
#include "plugin/ecs_plugin_integration.hpp"
#include "plugin/plugin_testing.hpp"
#include "plugin/plugin_sdk.hpp"
#include "ecs/registry.hpp"
#include "core/log.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace ecscope;
using namespace ecscope::plugin;

/**
 * @brief Educational Plugin System Demo Application
 */
class PluginSystemDemo {
private:
    // Core systems
    std::unique_ptr<ecs::Registry> ecs_registry_;
    std::unique_ptr<PluginManager> plugin_manager_;
    std::unique_ptr<ECSPluginIntegrationManager> integration_manager_;
    std::unique_ptr<testing::PluginTestRunner> test_runner_;
    std::unique_ptr<sdk::PluginSDK> plugin_sdk_;
    
    // Demo configuration
    bool enable_hot_reload_{true};
    bool enable_educational_mode_{true};
    bool enable_security_validation_{true};
    bool enable_performance_monitoring_{true};
    
    // Demo state
    bool is_running_{false};
    std::atomic<u32> demo_frame_count_{0};
    std::chrono::high_resolution_clock::time_point start_time_;

public:
    /**
     * @brief Initialize the plugin system demo
     */
    bool initialize() {
        LOG_INFO("=== ECScope Plugin System Demo Initialization ===");
        
        start_time_ = std::chrono::high_resolution_clock::now();
        
        // Initialize ECS Registry
        LOG_INFO("Initializing ECS Registry...");
        ecs_registry_ = std::make_unique<ecs::Registry>(
            ecs::AllocatorConfig::create_educational_focused(), "Demo_Registry");
        
        if (!ecs_registry_) {
            LOG_ERROR("Failed to initialize ECS Registry");
            return false;
        }
        
        // Initialize Plugin Manager
        LOG_INFO("Initializing Plugin Manager...");
        auto plugin_config = PluginManagerConfig::create_educational();
        plugin_config.enable_hot_reload = enable_hot_reload_;
        plugin_config.enable_security_validation = enable_security_validation_;
        plugin_config.enable_performance_profiling = enable_performance_monitoring_;
        
        plugin_manager_ = std::make_unique<PluginManager>(plugin_config);
        if (!plugin_manager_->initialize()) {
            LOG_ERROR("Failed to initialize Plugin Manager");
            return false;
        }
        
        // Initialize ECS Plugin Integration
        LOG_INFO("Initializing ECS Plugin Integration...");
        auto integration_config = create_educational_integration_config();
        integration_manager_ = std::make_unique<ECSPluginIntegrationManager>(
            *ecs_registry_, *plugin_manager_, integration_config);
        
        if (!integration_manager_->initialize()) {
            LOG_ERROR("Failed to initialize ECS Plugin Integration");
            return false;
        }
        
        // Initialize Test Runner
        LOG_INFO("Initializing Plugin Test Runner...");
        auto test_config = testing::create_educational_test_config();
        test_runner_ = std::make_unique<testing::PluginTestRunner>(
            *plugin_manager_, *integration_manager_, test_config);
        
        // Initialize Plugin SDK
        LOG_INFO("Initializing Plugin SDK...");
        plugin_sdk_ = std::make_unique<sdk::PluginSDK>("./plugin_sdk");
        
        LOG_INFO("Plugin System Demo initialized successfully!");
        return true;
    }
    
    /**
     * @brief Run the educational plugin system demonstration
     */
    void run_demo() {
        LOG_INFO("=== Starting Plugin System Educational Demo ===");
        is_running_ = true;
        
        // Phase 1: Plugin Discovery and Loading
        demonstrate_plugin_discovery();
        
        // Phase 2: Plugin Development with SDK
        demonstrate_plugin_development();
        
        // Phase 3: Hot Reload Capabilities
        if (enable_hot_reload_) {
            demonstrate_hot_reload();
        }
        
        // Phase 4: Security and Sandboxing
        if (enable_security_validation_) {
            demonstrate_security_features();
        }
        
        // Phase 5: ECS Integration
        demonstrate_ecs_integration();
        
        // Phase 6: Performance Monitoring
        if (enable_performance_monitoring_) {
            demonstrate_performance_monitoring();
        }
        
        // Phase 7: Testing Framework
        demonstrate_testing_framework();
        
        // Phase 8: Educational Features
        demonstrate_educational_features();
        
        // Main demo loop
        run_main_demo_loop();
        
        LOG_INFO("Plugin System Demo completed successfully!");
    }
    
    /**
     * @brief Clean shutdown of demo
     */
    void shutdown() {
        LOG_INFO("=== Plugin System Demo Shutdown ===");
        is_running_ = false;
        
        if (test_runner_) {
            test_runner_.reset();
        }
        
        if (integration_manager_) {
            integration_manager_->shutdown();
            integration_manager_.reset();
        }
        
        if (plugin_manager_) {
            plugin_manager_->shutdown();
            plugin_manager_.reset();
        }
        
        if (ecs_registry_) {
            ecs_registry_->clear();
            ecs_registry_.reset();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64>(end_time - start_time_).count();
        
        LOG_INFO("Demo ran for {:.2f} seconds with {} frames", duration, demo_frame_count_.load());
        LOG_INFO("Plugin System Demo shutdown complete");
    }

private:
    /**
     * @brief Phase 1: Demonstrate plugin discovery and loading
     */
    void demonstrate_plugin_discovery() {
        LOG_INFO("\n=== PHASE 1: Plugin Discovery and Loading ===");
        
        // Discover available plugins
        LOG_INFO("Discovering plugins in configured directories...");
        auto discovered_plugins = plugin_manager_->discover_plugins();
        
        LOG_INFO("Found {} plugins:", discovered_plugins.size());
        for (const auto& plugin_info : discovered_plugins) {
            if (plugin_info.is_valid) {
                LOG_INFO("  - {} v{} by {} ({})", 
                        plugin_info.metadata.display_name,
                        plugin_info.metadata.version.to_string(),
                        plugin_info.metadata.author,
                        plugin_info.metadata.description);
            } else {
                LOG_WARN("  - Invalid plugin: {} ({})", 
                        plugin_info.file_path, plugin_info.error_message);
            }
        }
        
        // Load educational plugins
        LOG_INFO("\nLoading educational plugins...");
        auto load_results = plugin_manager_->load_plugins_by_category(PluginCategory::Educational);
        
        u32 loaded_count = 0;
        for (const auto& result : load_results) {
            if (result.success) {
                loaded_count++;
                LOG_INFO("  ✓ Loaded: {}", result.metadata.display_name);
            } else {
                LOG_ERROR("  ✗ Failed to load: {} ({})", 
                         result.metadata.display_name, result.error_message);
            }
        }
        
        LOG_INFO("Successfully loaded {}/{} plugins", loaded_count, load_results.size());
        
        // Display loaded plugin information
        display_loaded_plugin_info();
    }
    
    /**
     * @brief Phase 2: Demonstrate plugin development with SDK
     */
    void demonstrate_plugin_development() {
        LOG_INFO("\n=== PHASE 2: Plugin Development with SDK ===");
        
        // Create a sample plugin project using the SDK
        LOG_INFO("Creating sample plugin project using SDK...");
        
        sdk::TemplateConfig template_config;
        template_config.type = sdk::PluginTemplateType::Educational;
        template_config.plugin_name = "DemoEducationalPlugin";
        template_config.display_name = "Demo Educational Plugin";
        template_config.description = "A sample educational plugin created during demo";
        template_config.author = "Demo User";
        template_config.category = PluginCategory::Educational;
        template_config.is_educational = true;
        template_config.difficulty_level = "beginner";
        template_config.learning_objectives = {
            "Learn plugin creation process",
            "Understand plugin architecture",
            "Practice SDK usage"
        };
        template_config.output_directory = "./demo_plugins";
        
        if (plugin_sdk_->create_plugin_project(template_config)) {
            LOG_INFO("  ✓ Sample plugin project created successfully");
            
            // Generate documentation
            auto documentation = plugin_sdk_->generate_documentation();
            LOG_INFO("  ✓ Generated plugin documentation ({} characters)", documentation.length());
            
            // Analyze code quality
            auto quality_metrics = plugin_sdk_->analyze_quality();
            LOG_INFO("  ✓ Code quality analysis completed (Score: {}/100)", quality_metrics.overall_score);
            
        } else {
            LOG_WARN("  ✗ Failed to create sample plugin project");
        }
        
        // Show available SDK tutorials
        auto tutorials = plugin_sdk_->get_available_tutorials();
        LOG_INFO("Available SDK tutorials: {}", tutorials.size());
        for (const auto& tutorial : tutorials) {
            LOG_INFO("  - {}", tutorial);
        }
    }
    
    /**
     * @brief Phase 3: Demonstrate hot reload capabilities
     */
    void demonstrate_hot_reload() {
        LOG_INFO("\n=== PHASE 3: Hot Reload Capabilities ===");
        
        LOG_INFO("Setting up hot reload monitoring...");
        
        // Enable hot reload for all loaded plugins
        auto loaded_plugins = plugin_manager_->get_loaded_plugin_names();
        for (const auto& plugin_name : loaded_plugins) {
            if (plugin_manager_->enable_hot_reload(plugin_name)) {
                LOG_INFO("  ✓ Hot reload enabled for: {}", plugin_name);
            }
        }
        
        // Check for file changes (simulated)
        LOG_INFO("Checking for plugin file changes...");
        auto changed_plugins = plugin_manager_->check_for_plugin_changes();
        
        if (!changed_plugins.empty()) {
            LOG_INFO("Detected changes in {} plugins:", changed_plugins.size());
            for (const auto& plugin_name : changed_plugins) {
                LOG_INFO("  - {}", plugin_name);
                
                // Demonstrate hot reload process
                LOG_INFO("    Performing hot reload...");
                if (plugin_manager_->hot_reload_plugin(plugin_name)) {
                    LOG_INFO("    ✓ Hot reload successful");
                } else {
                    LOG_ERROR("    ✗ Hot reload failed");
                }
            }
        } else {
            LOG_INFO("No plugin changes detected (this is normal for demo)");
        }
        
        // Show hot reload statistics
        auto hot_reload_stats = plugin_manager_->get_statistics();
        LOG_INFO("Hot reload statistics:");
        LOG_INFO("  - Total reloads performed: {}", hot_reload_stats.hot_reloads_performed);
        LOG_INFO("  - Average reload time: {:.2f}ms", hot_reload_stats.average_hot_reload_time_ms);
    }
    
    /**
     * @brief Phase 4: Demonstrate security and sandboxing
     */
    void demonstrate_security_features() {
        LOG_INFO("\n=== PHASE 4: Security and Sandboxing ===");
        
        LOG_INFO("Validating plugin security...");
        
        auto loaded_plugins = plugin_manager_->get_loaded_plugin_names();
        for (const auto& plugin_name : loaded_plugins) {
            // Get security context
            auto security_context = plugin_manager_->get_plugin_security_context(plugin_name);
            if (security_context) {
                LOG_INFO("Security context for '{}':", plugin_name);
                LOG_INFO("  - Memory limit: {} MB", security_context->memory_limit / (1024 * 1024));
                LOG_INFO("  - Thread limit: {}", security_context->thread_limit);
                LOG_INFO("  - Execution timeout: {}ms", security_context->execution_timeout.count());
                LOG_INFO("  - Memory protection: {}", security_context->enable_memory_protection ? "enabled" : "disabled");
            }
            
            // Validate plugin signature
            if (plugin_manager_->validate_plugin_signature(plugin_name)) {
                LOG_INFO("  ✓ Plugin signature valid: {}", plugin_name);
            } else {
                LOG_WARN("  ⚠ Plugin signature validation failed: {}", plugin_name);
            }
        }
        
        // Display quarantined plugins
        auto quarantined = plugin_manager_->get_quarantined_plugins();
        if (!quarantined.empty()) {
            LOG_WARN("Quarantined plugins ({}):", quarantined.size());
            for (const auto& plugin_name : quarantined) {
                LOG_WARN("  - {}", plugin_name);
            }
        } else {
            LOG_INFO("No plugins are currently quarantined");
        }
        
        // Show security statistics
        auto stats = plugin_manager_->get_statistics();
        LOG_INFO("Security statistics:");
        LOG_INFO("  - Security violations: {}", stats.security_violations_detected);
        LOG_INFO("  - Quarantined plugins: {}", stats.plugins_quarantined_for_security);
    }
    
    /**
     * @brief Phase 5: Demonstrate ECS integration
     */
    void demonstrate_ecs_integration() {
        LOG_INFO("\n=== PHASE 5: ECS Integration ===");
        
        LOG_INFO("Demonstrating ECS-Plugin integration...");
        
        // Show registered plugin components
        auto& component_bridge = integration_manager_->get_component_bridge();
        auto component_stats = component_bridge.get_component_usage_stats();
        
        LOG_INFO("Plugin components registered with ECS:");
        for (const auto& [component_name, stats] : component_stats) {
            LOG_INFO("  - {} (Plugin: {})", component_name, stats.providing_plugin);
            LOG_INFO("    Total instances: {}", stats.total_instances);
            LOG_INFO("    Memory usage: {} KB", stats.memory_usage / 1024);
        }
        
        // Show registered plugin systems
        auto& system_bridge = integration_manager_->get_system_bridge();
        auto system_performance = system_bridge.get_system_performance();
        
        LOG_INFO("Plugin systems registered with ECS:");
        for (const auto& [system_name, metrics] : system_performance) {
            LOG_INFO("  - {} (Plugin: {})", system_name, metrics.plugin_name);
            LOG_INFO("    Average execution time: {:.2f}ms", metrics.average_execution_time_ms);
            LOG_INFO("    Performance score: {}/100", static_cast<u32>(metrics.performance_score));
        }
        
        // Create some entities with plugin components for demonstration
        demonstrate_entity_creation_with_plugins();
        
        // Show integration statistics
        auto integration_stats = integration_manager_->get_integration_stats();
        LOG_INFO("Integration statistics:");
        LOG_INFO("  - Plugin components: {}", integration_stats.total_plugin_components);
        LOG_INFO("  - Plugin systems: {}", integration_stats.total_plugin_systems);
        LOG_INFO("  - Events bridged: {}", integration_stats.total_events_bridged);
        LOG_INFO("  - Integration efficiency: {:.1f}%", integration_stats.integration_efficiency_score * 100.0f);
    }
    
    /**
     * @brief Phase 6: Demonstrate performance monitoring
     */
    void demonstrate_performance_monitoring() {
        LOG_INFO("\n=== PHASE 6: Performance Monitoring ===");
        
        LOG_INFO("Analyzing plugin performance...");
        
        // Get overall plugin statistics
        auto plugin_stats = plugin_manager_->get_statistics();
        LOG_INFO("Plugin Manager Statistics:");
        LOG_INFO("  - Plugins loaded: {}", plugin_stats.plugins_loaded);
        LOG_INFO("  - Total load time: {:.2f}ms", plugin_stats.total_load_time_ms);
        LOG_INFO("  - Average load time: {:.2f}ms", plugin_stats.average_load_time_ms);
        LOG_INFO("  - Total memory usage: {} MB", plugin_stats.total_plugin_memory_usage / (1024 * 1024));
        
        // Get individual plugin performance
        auto all_plugin_stats = plugin_manager_->get_all_plugin_stats();
        LOG_INFO("\nIndividual Plugin Performance:");
        for (const auto& [plugin_name, stats] : all_plugin_stats) {
            LOG_INFO("  {}:", plugin_name);
            LOG_INFO("    - CPU time: {:.2f}ms", stats.total_cpu_time_ms);
            LOG_INFO("    - Memory usage: {} KB", stats.current_memory_usage / 1024);
            LOG_INFO("    - Function calls: {}", stats.total_function_calls);
            LOG_INFO("    - Performance score: {}/100", stats.performance_score);
        }
        
        // Generate comprehensive performance report
        auto performance_report = plugin_manager_->generate_performance_report();
        LOG_INFO("\nPerformance report generated ({} characters)", performance_report.length());
    }
    
    /**
     * @brief Phase 7: Demonstrate testing framework
     */
    void demonstrate_testing_framework() {
        LOG_INFO("\n=== PHASE 7: Plugin Testing Framework ===");
        
        LOG_INFO("Running plugin validation tests...");
        
        // Run tests for all loaded plugins
        test_runner_->run_all_plugin_tests();
        
        // Show test results
        auto loaded_plugins = plugin_manager_->get_loaded_plugin_names();
        for (const auto& plugin_name : loaded_plugins) {
            auto test_results = test_runner_->get_plugin_test_results(plugin_name);
            
            if (!test_results.empty()) {
                u32 passed = 0, failed = 0;
                for (const auto& result : test_results) {
                    if (result.passed) passed++;
                    else failed++;
                }
                
                LOG_INFO("Test results for '{}': {} passed, {} failed", plugin_name, passed, failed);
                
                // Show any failed tests
                for (const auto& result : test_results) {
                    if (!result.passed) {
                        LOG_WARN("  ✗ {} failed: {}", result.test_name, result.error_message);
                    }
                }
            }
        }
        
        // Generate comprehensive test report
        auto test_report = test_runner_->generate_comprehensive_report();
        LOG_INFO("Comprehensive test report generated ({} characters)", test_report.length());
    }
    
    /**
     * @brief Phase 8: Demonstrate educational features
     */
    void demonstrate_educational_features() {
        LOG_INFO("\n=== PHASE 8: Educational Features ===");
        
        LOG_INFO("Showcasing educational plugin features...");
        
        // Show learning resources for loaded plugins
        auto loaded_plugins = plugin_manager_->get_loaded_plugin_names();
        for (const auto& plugin_name : loaded_plugins) {
            auto plugin = plugin_manager_->get_plugin(plugin_name);
            if (plugin && plugin->get_plugin_instance()) {
                auto resources = plugin->get_plugin_instance()->get_learning_resources();
                if (!resources.empty()) {
                    LOG_INFO("Learning resources for '{}':", plugin_name);
                    for (const auto& resource : resources) {
                        LOG_INFO("  - {}", resource);
                    }
                }
                
                // Show functionality explanation
                auto explanation = plugin->get_plugin_instance()->explain_functionality();
                if (!explanation.empty()) {
                    LOG_INFO("Functionality explanation for '{}':", plugin_name);
                    LOG_INFO("{}", explanation);
                }
            }
        }
        
        // Show integration tutorials
        auto integration_tutorials = integration_manager_->get_integration_tutorials();
        LOG_INFO("Available integration tutorials:");
        for (const auto& tutorial : integration_tutorials) {
            LOG_INFO("  - {}", tutorial);
        }
        
        // Show best practices
        auto best_practices = integration_manager_->get_best_practices();
        LOG_INFO("Plugin development best practices:");
        for (const auto& [category, practice] : best_practices) {
            LOG_INFO("  {}: {}", category, practice);
        }
    }
    
    /**
     * @brief Main demo loop with continuous monitoring
     */
    void run_main_demo_loop() {
        LOG_INFO("\n=== Main Demo Loop (Running for 10 seconds) ===");
        LOG_INFO("Monitoring plugin system in real-time...");
        
        auto loop_start = std::chrono::high_resolution_clock::now();
        auto last_stats_time = loop_start;
        const f64 target_delta_time = 1.0 / 60.0; // 60 FPS
        
        while (is_running_) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            // Update systems
            integration_manager_->update(target_delta_time);
            plugin_manager_->update_plugins(target_delta_time);
            
            demo_frame_count_++;
            
            // Show periodic statistics
            auto current_time = std::chrono::high_resolution_clock::now();
            auto stats_elapsed = std::chrono::duration<f64>(current_time - last_stats_time).count();
            
            if (stats_elapsed >= 2.0) { // Every 2 seconds
                show_realtime_statistics();
                last_stats_time = current_time;
            }
            
            // Check if demo time is up (10 seconds)
            auto total_elapsed = std::chrono::duration<f64>(current_time - loop_start).count();
            if (total_elapsed >= 10.0) {
                is_running_ = false;
            }
            
            // Frame rate limiting
            auto frame_end = std::chrono::high_resolution_clock::now();
            auto frame_time = std::chrono::duration<f64>(frame_end - frame_start).count();
            auto sleep_time = target_delta_time - frame_time;
            
            if (sleep_time > 0) {
                std::this_thread::sleep_for(std::chrono::duration<f64>(sleep_time));
            }
        }
        
        LOG_INFO("Demo loop completed after {} frames", demo_frame_count_.load());
    }
    
    /**
     * @brief Display information about loaded plugins
     */
    void display_loaded_plugin_info() {
        LOG_INFO("\nLoaded Plugin Details:");
        
        auto loaded_plugins = plugin_manager_->get_loaded_plugin_names();
        for (const auto& plugin_name : loaded_plugins) {
            auto metadata = plugin_manager_->get_plugin_metadata(plugin_name);
            if (metadata) {
                LOG_INFO("Plugin: {} v{}", metadata->display_name, metadata->version.to_string());
                LOG_INFO("  Category: {}", plugin_category_to_string(metadata->category));
                LOG_INFO("  Priority: {}", static_cast<u32>(metadata->priority));
                LOG_INFO("  Educational: {}", metadata->is_educational ? "Yes" : "No");
                if (metadata->is_educational) {
                    LOG_INFO("  Difficulty: {}", metadata->difficulty_level);
                    LOG_INFO("  Purpose: {}", metadata->educational_purpose);
                }
                LOG_INFO("  Memory limit: {} MB", metadata->max_memory_usage / (1024 * 1024));
                LOG_INFO(""); // Empty line for readability
            }
        }
    }
    
    /**
     * @brief Demonstrate creating entities with plugin components
     */
    void demonstrate_entity_creation_with_plugins() {
        LOG_INFO("Creating demonstration entities with plugin components...");
        
        // This would create entities using components from loaded plugins
        // For the demo, we'll simulate this process
        
        try {
            // Example: Create entities with health and experience components (if available)
            for (u32 i = 0; i < 5; ++i) {
                auto entity = ecs_registry_->create_entity();
                LOG_DEBUG("Created demo entity: {}", entity);
            }
            
            LOG_INFO("  ✓ Created 5 demonstration entities");
            
        } catch (const std::exception& e) {
            LOG_WARN("  ⚠ Entity creation demonstration limited: {}", e.what());
        }
    }
    
    /**
     * @brief Show real-time statistics during demo loop
     */
    void show_realtime_statistics() {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<f64>(current_time - start_time_).count();
        auto fps = demo_frame_count_ / elapsed;
        
        LOG_INFO("Real-time Stats | Frame: {} | Time: {:.1f}s | FPS: {:.1f}", 
                demo_frame_count_.load(), elapsed, fps);
        
        // Show memory usage
        auto total_memory = plugin_manager_->get_total_plugin_memory_usage();
        LOG_INFO("  Memory: {} MB | Active Plugins: {}", 
                total_memory / (1024 * 1024), 
                plugin_manager_->get_loaded_plugin_names().size());
    }
};

/**
 * @brief Main entry point for plugin system demo
 */
int main(int argc, char* argv[]) {
    LOG_INFO("ECScope Plugin System Educational Demo");
    LOG_INFO("=====================================");
    
    try {
        // Create and initialize demo
        PluginSystemDemo demo;
        
        if (!demo.initialize()) {
            LOG_ERROR("Failed to initialize plugin system demo");
            return 1;
        }
        
        // Run the comprehensive demonstration
        demo.run_demo();
        
        // Clean shutdown
        demo.shutdown();
        
        LOG_INFO("Demo completed successfully!");
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Demo failed with exception: {}", e.what());
        return 1;
    } catch (...) {
        LOG_ERROR("Demo failed with unknown exception");
        return 1;
    }
}