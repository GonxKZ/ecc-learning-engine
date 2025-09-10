#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>

namespace ecscope::plugins {

    // Forward declarations
    class PluginContext;
    class PluginRegistry;
    
    /**
     * @brief Plugin version structure for semantic versioning
     */
    struct PluginVersion {
        uint32_t major{0};
        uint32_t minor{0};
        uint32_t patch{0};
        std::string pre_release;
        
        PluginVersion() = default;
        PluginVersion(uint32_t maj, uint32_t min, uint32_t pat, const std::string& pre = "")
            : major(maj), minor(min), patch(pat), pre_release(pre) {}
        
        bool is_compatible(const PluginVersion& other) const noexcept;
        std::string to_string() const;
        
        bool operator==(const PluginVersion& other) const noexcept;
        bool operator<(const PluginVersion& other) const noexcept;
        bool operator>(const PluginVersion& other) const noexcept;
    };
    
    /**
     * @brief Plugin dependency information
     */
    struct PluginDependency {
        std::string name;
        PluginVersion min_version;
        PluginVersion max_version;
        bool optional{false};
        
        PluginDependency(const std::string& plugin_name, 
                        const PluginVersion& min_ver = {}, 
                        const PluginVersion& max_ver = {999, 999, 999},
                        bool is_optional = false)
            : name(plugin_name), min_version(min_ver), max_version(max_ver), optional(is_optional) {}
    };
    
    /**
     * @brief Plugin metadata containing all plugin information
     */
    struct PluginMetadata {
        std::string name;
        std::string display_name;
        std::string description;
        std::string author;
        std::string website;
        PluginVersion version;
        PluginVersion engine_version_min;
        PluginVersion engine_version_max;
        std::vector<PluginDependency> dependencies;
        std::vector<std::string> tags;
        std::string license;
        std::string manifest_path;
        bool sandbox_required{true};
        uint64_t memory_limit{1024 * 1024 * 100}; // 100MB default
        uint32_t cpu_time_limit{1000}; // 1000ms default
        std::vector<std::string> required_permissions;
        
        bool is_valid() const noexcept;
    };
    
    /**
     * @brief Plugin lifecycle states
     */
    enum class PluginState {
        Unloaded,
        Loading,
        Loaded,
        Initializing,
        Active,
        Paused,
        Shutting_Down,
        Error,
        Unloading
    };
    
    /**
     * @brief Plugin priority for loading order
     */
    enum class PluginPriority {
        Critical = 0,     // Core engine plugins
        High = 100,       // Important system plugins
        Normal = 500,     // Regular plugins
        Low = 1000        // Optional/cosmetic plugins
    };
    
    /**
     * @brief Base interface for all plugins
     * 
     * This interface defines the contract that all plugins must implement.
     * It provides lifecycle management, metadata access, and context interaction.
     */
    class IPlugin {
    public:
        virtual ~IPlugin() = default;
        
        // Lifecycle management
        virtual bool initialize(PluginContext* context) = 0;
        virtual void shutdown() = 0;
        virtual void update(double delta_time) {}
        virtual void pause() {}
        virtual void resume() {}
        
        // Metadata access
        virtual const PluginMetadata& get_metadata() const = 0;
        virtual PluginState get_state() const = 0;
        virtual PluginPriority get_priority() const { return PluginPriority::Normal; }
        
        // Event handling
        virtual void on_event(const std::string& event_name, const std::map<std::string, std::string>& params) {}
        
        // Plugin communication
        virtual std::string handle_message(const std::string& message, const std::map<std::string, std::string>& params) { return ""; }
        
        // Resource management
        virtual void on_resource_loaded(const std::string& resource_id) {}
        virtual void on_resource_unloaded(const std::string& resource_id) {}
        
        // Configuration
        virtual void configure(const std::map<std::string, std::string>& config) {}
        virtual std::map<std::string, std::string> get_configuration() const { return {}; }
        
    protected:
        PluginContext* context_{nullptr};
        PluginState state_{PluginState::Unloaded};
        PluginMetadata metadata_;
    };
    
    /**
     * @brief Plugin factory function signature
     */
    using PluginFactoryFunc = std::function<std::unique_ptr<IPlugin>()>;
    
    /**
     * @brief Plugin cleanup function signature
     */
    using PluginCleanupFunc = std::function<void()>;
    
    /**
     * @brief Plugin export structure for dynamic loading
     */
    struct PluginExport {
        const char* name;
        const char* version;
        PluginFactoryFunc factory;
        PluginCleanupFunc cleanup;
        const PluginMetadata* metadata;
    };
    
} // namespace ecscope::plugins

// Plugin export macros for dynamic loading
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))

#define DECLARE_PLUGIN(PluginClass, plugin_name, plugin_version) \
    PLUGIN_EXPORT ecscope::plugins::PluginExport* get_plugin_export() { \
        static auto factory = []() -> std::unique_ptr<ecscope::plugins::IPlugin> { \
            return std::make_unique<PluginClass>(); \
        }; \
        static auto cleanup = []() { /* Plugin-specific cleanup */ }; \
        static ecscope::plugins::PluginExport export_info = { \
            plugin_name, \
            plugin_version, \
            factory, \
            cleanup, \
            &PluginClass::get_static_metadata() \
        }; \
        return &export_info; \
    }

#define PLUGIN_API_VERSION 1

#define DECLARE_PLUGIN_API_VERSION() \
    PLUGIN_EXPORT int get_plugin_api_version() { \
        return PLUGIN_API_VERSION; \
    }