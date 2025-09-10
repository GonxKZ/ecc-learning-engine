/**
 * @file hello_world_plugin.cpp
 * @brief Simple "Hello World" plugin demonstrating basic plugin functionality
 * 
 * This plugin demonstrates:
 * - Basic plugin structure and lifecycle
 * - Configuration handling
 * - Event system usage
 * - Message handling
 * - Logging functionality
 */

#include <ecscope/plugins/sdk/plugin_sdk.hpp>
#include <iostream>
#include <string>
#include <map>

/**
 * @brief Hello World Plugin Class
 * 
 * A simple plugin that demonstrates basic functionality including
 * initialization, configuration, event handling, and messaging.
 */
class HelloWorldPlugin : public ecscope::plugins::sdk::PluginBase {
public:
    HelloWorldPlugin() : PluginBase("hello_world", {1, 0, 0}) {
        // Set plugin metadata
        set_display_name("Hello World Plugin");
        set_description("A simple plugin that demonstrates basic functionality");
        set_author("ECScope Team");
        set_website("https://github.com/ecscope/hello-world-plugin");
        set_license("MIT");
        
        // Add tags for categorization
        add_tag("example");
        add_tag("tutorial");
        add_tag("basic");
        
        // Set plugin priority (normal priority)
        set_priority(ecscope::plugins::PluginPriority::Normal);
    }
    
    // Static metadata accessor required for plugin registration
    static const ecscope::plugins::PluginMetadata& get_static_metadata() {
        static ecscope::plugins::PluginMetadata metadata;
        if (metadata.name.empty()) {
            metadata.name = "hello_world";
            metadata.display_name = "Hello World Plugin";
            metadata.description = "A simple plugin that demonstrates basic functionality";
            metadata.author = "ECScope Team";
            metadata.version = {1, 0, 0};
            metadata.license = "MIT";
            metadata.sandbox_required = true;
            metadata.memory_limit = 1024 * 1024 * 10; // 10MB
            metadata.cpu_time_limit = 50; // 50ms
            metadata.tags = {"example", "tutorial", "basic"};
            metadata.required_permissions = {"PluginCommunication"};
        }
        return metadata;
    }
    
protected:
    bool on_initialize() override {
        log_info("Hello World Plugin is initializing!");
        
        // Request necessary permissions
        if (!request_permission(ecscope::plugins::Permission::PluginCommunication, 
                              "For demonstrating inter-plugin communication")) {
            log_error("Failed to get communication permission");
            return false;
        }
        
        // Subscribe to some events
        subscribe_to_event("engine.start", [this](const std::map<std::string, std::string>& params) {
            log_info("Engine started! Hello from the plugin!");
        });
        
        subscribe_to_event("engine.stop", [this](const std::map<std::string, std::string>& params) {
            log_info("Engine stopping. Goodbye from the plugin!");
        });
        
        subscribe_to_event("player.spawn", [this](const std::map<std::string, std::string>& params) {
            auto player_name = params.find("name");
            if (player_name != params.end()) {
                log_info("Player spawned: " + player_name->second);
                
                // Emit a welcome event
                emit_event("plugin.player_welcome", {
                    {"plugin", "hello_world"},
                    {"message", "Welcome to the game, " + player_name->second + "!"}
                });
            }
        });
        
        // Set up message handlers
        set_message_handler("ping", [this](const std::map<std::string, std::string>& params) -> std::string {
            log_info("Received ping message");
            return "pong from Hello World Plugin!";
        });
        
        set_message_handler("greet", [this](const std::map<std::string, std::string>& params) -> std::string {
            auto name_it = params.find("name");
            std::string name = (name_it != params.end()) ? name_it->second : "Unknown";
            
            std::string greeting = "Hello, " + name + "! Greetings from the Hello World Plugin.";
            log_info("Generated greeting: " + greeting);
            return greeting;
        });
        
        set_message_handler("status", [this](const std::map<std::string, std::string>& params) -> std::string {
            return "Hello World Plugin is running and ready!";
        });
        
        // Load configuration
        std::string greeting_prefix = get_config("greeting_prefix", "Hello");
        std::string greeting_suffix = get_config("greeting_suffix", "!");
        log_info("Using greeting format: '" + greeting_prefix + " [name] " + greeting_suffix + "'");
        
        // Store some resources for demonstration
        store_resource("initialized_time", std::chrono::system_clock::now());
        store_resource("greeting_count", 0);
        
        log_info("Hello World Plugin initialized successfully!");
        return true;
    }
    
    void on_shutdown() override {
        log_info("Hello World Plugin is shutting down!");
        
        // Show some statistics
        auto greeting_count = get_resource<int>("greeting_count");
        if (greeting_count) {
            log_info("Total greetings sent: " + std::to_string(*greeting_count));
        }
        
        auto init_time = get_resource<std::chrono::system_clock::time_point>("initialized_time");
        if (init_time) {
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - *init_time);
            log_info("Plugin was running for " + std::to_string(duration.count()) + " seconds");
        }
        
        log_info("Goodbye from Hello World Plugin!");
    }
    
    void update(double delta_time) override {
        // This plugin doesn't need continuous updates, but we could add logic here
        // For example, periodic messages or status updates
        
        static double accumulated_time = 0.0;
        accumulated_time += delta_time;
        
        // Send a periodic status message every 30 seconds
        if (accumulated_time >= 30.0) {
            emit_event("plugin.heartbeat", {
                {"plugin", "hello_world"},
                {"uptime", std::to_string(accumulated_time)}
            });
            accumulated_time = 0.0;
        }
    }
    
    void on_pause() override {
        log_info("Hello World Plugin paused");
    }
    
    void on_resume() override {
        log_info("Hello World Plugin resumed");
    }
    
    void on_configure(const std::map<std::string, std::string>& config) override {
        log_info("Hello World Plugin configuration updated");
        
        for (const auto& [key, value] : config) {
            log_debug("Config update: " + key + " = " + value);
            set_config(key, value);
        }
        
        // React to specific configuration changes
        if (config.find("greeting_prefix") != config.end() || 
            config.find("greeting_suffix") != config.end()) {
            
            std::string prefix = get_config("greeting_prefix", "Hello");
            std::string suffix = get_config("greeting_suffix", "!");
            log_info("Greeting format updated: '" + prefix + " [name] " + suffix + "'");
        }
    }

private:
    void increment_greeting_count() {
        auto count_ptr = get_resource<int>("greeting_count");
        if (count_ptr) {
            (*count_ptr)++;
        } else {
            store_resource("greeting_count", 1);
        }
    }
};

// Plugin export declaration - this makes the plugin loadable by the engine
DECLARE_PLUGIN(HelloWorldPlugin, "hello_world", "1.0.0")
DECLARE_PLUGIN_API_VERSION()

/**
 * Example usage and testing:
 * 
 * 1. Load the plugin:
 *    auto registry = std::make_unique<PluginRegistry>();
 *    registry->load_plugin("hello_world_plugin.so");
 * 
 * 2. Send messages:
 *    registry->send_message("engine", "hello_world", "ping", {});
 *    registry->send_message("engine", "hello_world", "greet", {{"name", "Alice"}});
 * 
 * 3. Configure the plugin:
 *    auto plugin = registry->get_plugin("hello_world");
 *    plugin->configure({
 *        {"greeting_prefix", "Hi there"},
 *        {"greeting_suffix", "! Have a great day!"}
 *    });
 * 
 * 4. Emit events:
 *    registry->emit_event("player.spawn", {{"name", "Bob"}});
 */