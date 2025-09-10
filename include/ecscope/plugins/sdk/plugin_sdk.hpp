#pragma once

#include "../plugin_interface.hpp"
#include "../plugin_context.hpp"
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <vector>

/**
 * @brief ECScope Plugin SDK
 * 
 * This header provides a simplified interface for plugin development,
 * abstracting away the complexities of the full plugin system while
 * providing all necessary functionality for most plugin use cases.
 */

namespace ecscope::plugins::sdk {

    /**
     * @brief Base class for simple plugins
     * 
     * Provides default implementations and helper methods to make
     * plugin development easier.
     */
    class PluginBase : public IPlugin {
    public:
        explicit PluginBase(const std::string& name, const PluginVersion& version);
        virtual ~PluginBase() = default;
        
        // IPlugin interface implementation
        bool initialize(PluginContext* context) override;
        void shutdown() override;
        void update(double delta_time) override {}
        void pause() override;
        void resume() override;
        
        const PluginMetadata& get_metadata() const override { return metadata_; }
        PluginState get_state() const override { return state_; }
        PluginPriority get_priority() const override { return priority_; }
        
        void on_event(const std::string& event_name, const std::map<std::string, std::string>& params) override;
        std::string handle_message(const std::string& message, const std::map<std::string, std::string>& params) override;
        
        void configure(const std::map<std::string, std::string>& config) override;
        std::map<std::string, std::string> get_configuration() const override;
        
    protected:
        // Helper methods for plugin developers
        void log_debug(const std::string& message);
        void log_info(const std::string& message);
        void log_warning(const std::string& message);
        void log_error(const std::string& message);
        
        // Configuration helpers
        void set_config(const std::string& key, const std::string& value);
        std::string get_config(const std::string& key, const std::string& default_value = "") const;
        bool has_config(const std::string& key) const;
        
        // Event system helpers
        void subscribe_to_event(const std::string& event_name, std::function<void(const std::map<std::string, std::string>&)> callback);
        void emit_event(const std::string& event_name, const std::map<std::string, std::string>& data = {});
        
        // Message system helpers
        bool send_message(const std::string& recipient, const std::string& message, const std::map<std::string, std::string>& params = {});
        void set_message_handler(const std::string& message_type, std::function<std::string(const std::map<std::string, std::string>&)> handler);
        
        // Resource helpers
        template<typename T>
        void store_resource(const std::string& key, T&& resource) {
            if (context_) {
                context_->store_resource(key, std::forward<T>(resource));
            }
        }
        
        template<typename T>
        T* get_resource(const std::string& key) {
            return context_ ? context_->get_resource<T>(key) : nullptr;
        }
        
        // Engine system access
        ecs::Registry* get_ecs_registry() { return context_ ? context_->get_ecs_registry() : nullptr; }
        ecs::World* get_ecs_world() { return context_ ? context_->get_ecs_world() : nullptr; }
        rendering::Renderer* get_renderer() { return context_ ? context_->get_renderer() : nullptr; }
        assets::AssetManager* get_asset_manager() { return context_ ? context_->get_asset_manager() : nullptr; }
        
        // Directory helpers
        std::string get_plugin_directory() const { return context_ ? context_->get_plugin_directory() : ""; }
        std::string get_data_directory() const { return context_ ? context_->get_plugin_data_directory() : ""; }
        std::string get_config_directory() const { return context_ ? context_->get_plugin_config_directory() : ""; }
        
        // Permission helpers
        bool has_permission(Permission perm) const { return context_ ? context_->has_permission(perm) : false; }
        bool request_permission(Permission perm, const std::string& reason = "") { 
            return context_ ? context_->request_permission(perm, reason) : false; 
        }
        
        // Metadata setup helpers
        void set_display_name(const std::string& name) { metadata_.display_name = name; }
        void set_description(const std::string& desc) { metadata_.description = desc; }
        void set_author(const std::string& author) { metadata_.author = author; }
        void set_website(const std::string& website) { metadata_.website = website; }
        void set_license(const std::string& license) { metadata_.license = license; }
        void add_tag(const std::string& tag) { metadata_.tags.push_back(tag); }
        void set_priority(PluginPriority prio) { priority_ = prio; }
        void add_dependency(const std::string& name, const PluginVersion& min_version = {}, const PluginVersion& max_version = {999, 999, 999}) {
            metadata_.dependencies.emplace_back(name, min_version, max_version);
        }
        
        // Virtual hooks for plugin developers to override
        virtual bool on_initialize() { return true; }
        virtual void on_shutdown() {}
        virtual void on_pause() {}
        virtual void on_resume() {}
        virtual void on_configure(const std::map<std::string, std::string>& config) {}
        
    private:
        PluginMetadata metadata_;
        PluginPriority priority_{PluginPriority::Normal};
        std::map<std::string, std::function<std::string(const std::map<std::string, std::string>&)>> message_handlers_;
        std::map<std::string, std::string> config_;
    };
    
    /**
     * @brief Plugin builder for easy plugin creation
     */
    class PluginBuilder {
    public:
        PluginBuilder(const std::string& name, const PluginVersion& version);
        
        // Metadata setup
        PluginBuilder& display_name(const std::string& name);
        PluginBuilder& description(const std::string& desc);
        PluginBuilder& author(const std::string& author);
        PluginBuilder& website(const std::string& website);
        PluginBuilder& license(const std::string& license);
        PluginBuilder& tag(const std::string& tag);
        PluginBuilder& priority(PluginPriority prio);
        PluginBuilder& dependency(const std::string& name, const PluginVersion& min_version = {}, const PluginVersion& max_version = {999, 999, 999});
        
        // Resource requirements
        PluginBuilder& memory_limit(uint64_t bytes);
        PluginBuilder& cpu_limit(uint32_t ms);
        PluginBuilder& sandbox_required(bool required = true);
        PluginBuilder& permission(Permission perm);
        
        // Lifecycle callbacks
        PluginBuilder& on_initialize(std::function<bool()> callback);
        PluginBuilder& on_shutdown(std::function<void()> callback);
        PluginBuilder& on_update(std::function<void(double)> callback);
        PluginBuilder& on_pause(std::function<void()> callback);
        PluginBuilder& on_resume(std::function<void()> callback);
        
        // Event handling
        PluginBuilder& on_event(const std::string& event_name, std::function<void(const std::map<std::string, std::string>&)> callback);
        
        // Message handling
        PluginBuilder& on_message(const std::string& message_type, std::function<std::string(const std::map<std::string, std::string>&)> callback);
        
        // Configuration
        PluginBuilder& on_configure(std::function<void(const std::map<std::string, std::string>&)> callback);
        
        // Build the plugin
        std::unique_ptr<IPlugin> build();
        
    private:
        class BuilderPlugin;
        std::unique_ptr<BuilderPlugin> plugin_;
    };
    
    /**
     * @brief Utility functions for plugin development
     */
    namespace utils {
        
        // File I/O helpers
        std::string read_text_file(const std::string& path);
        bool write_text_file(const std::string& path, const std::string& content);
        std::vector<uint8_t> read_binary_file(const std::string& path);
        bool write_binary_file(const std::string& path, const std::vector<uint8_t>& data);
        bool file_exists(const std::string& path);
        bool create_directory(const std::string& path);
        
        // Configuration file helpers
        std::map<std::string, std::string> load_config_file(const std::string& path);
        bool save_config_file(const std::string& path, const std::map<std::string, std::string>& config);
        
        // JSON helpers (simple key-value pairs)
        std::map<std::string, std::string> parse_json_simple(const std::string& json);
        std::string serialize_json_simple(const std::map<std::string, std::string>& data);
        
        // String utilities
        std::vector<std::string> split_string(const std::string& str, char delimiter);
        std::string trim_string(const std::string& str);
        std::string to_lower(const std::string& str);
        std::string to_upper(const std::string& str);
        bool starts_with(const std::string& str, const std::string& prefix);
        bool ends_with(const std::string& str, const std::string& suffix);
        
        // Time utilities
        uint64_t get_current_timestamp_ms();
        std::string format_timestamp(uint64_t timestamp_ms);
        
        // Plugin manifest creation
        bool create_plugin_manifest(const std::string& path, const PluginMetadata& metadata);
        bool validate_plugin_manifest(const std::string& path, PluginMetadata& metadata);
        
        // Debug helpers
        void debug_print_plugin_info(const PluginMetadata& metadata);
        std::string get_plugin_status_string(PluginState state);
        std::string get_permission_string(Permission perm);
        
    } // namespace utils
    
    /**
     * @brief Plugin testing framework
     */
    namespace testing {
        
        /**
         * @brief Mock plugin context for testing
         */
        class MockPluginContext : public PluginContext {
        public:
            MockPluginContext(const std::string& plugin_name);
            
            // Mock system access (return test doubles)
            ecs::Registry* get_ecs_registry() override;
            ecs::World* get_ecs_world() override;
            rendering::Renderer* get_renderer() override;
            assets::AssetManager* get_asset_manager() override;
            
            // Test utilities
            void set_permission(Permission perm, bool granted) { permissions_[perm] = granted; }
            void set_config_value(const std::string& key, const std::string& value) { test_config_[key] = value; }
            std::vector<std::string> get_log_messages() const { return log_messages_; }
            void clear_logs() { log_messages_.clear(); }
            
        private:
            std::unordered_map<Permission, bool> permissions_;
            std::map<std::string, std::string> test_config_;
            mutable std::vector<std::string> log_messages_;
        };
        
        /**
         * @brief Plugin test fixture
         */
        class PluginTestFixture {
        public:
            PluginTestFixture(std::unique_ptr<IPlugin> plugin);
            ~PluginTestFixture();
            
            // Test lifecycle
            bool initialize_plugin();
            void shutdown_plugin();
            void update_plugin(double delta_time = 0.016);
            
            // Test utilities
            MockPluginContext* get_context() { return context_.get(); }
            IPlugin* get_plugin() { return plugin_.get(); }
            
            // Assertions
            void assert_plugin_initialized();
            void assert_plugin_state(PluginState expected_state);
            void assert_log_contains(const std::string& message);
            void assert_event_emitted(const std::string& event_name);
            void assert_message_sent(const std::string& recipient);
            
        private:
            std::unique_ptr<IPlugin> plugin_;
            std::unique_ptr<MockPluginContext> context_;
            std::vector<std::string> emitted_events_;
            std::vector<std::pair<std::string, std::string>> sent_messages_;
        };
        
        // Test macros
        #define PLUGIN_TEST(test_name) void test_name()
        #define ASSERT_TRUE(condition) if (!(condition)) throw std::runtime_error("Assertion failed: " #condition)
        #define ASSERT_FALSE(condition) if ((condition)) throw std::runtime_error("Assertion failed: " #condition)
        #define ASSERT_EQ(expected, actual) if ((expected) != (actual)) throw std::runtime_error("Assertion failed: expected != actual")
        
    } // namespace testing
    
} // namespace ecscope::plugins::sdk

// Convenience macros for plugin creation

/**
 * @brief Simple plugin declaration macro
 */
#define DECLARE_SIMPLE_PLUGIN(PluginClass, plugin_name, major, minor, patch) \
    class PluginClass : public ecscope::plugins::sdk::PluginBase { \
    public: \
        PluginClass() : PluginBase(plugin_name, {major, minor, patch}) {} \
        static const ecscope::plugins::PluginMetadata& get_static_metadata() { \
            static ecscope::plugins::PluginMetadata metadata; \
            if (metadata.name.empty()) { \
                metadata.name = plugin_name; \
                metadata.version = {major, minor, patch}; \
                metadata.display_name = plugin_name; \
            } \
            return metadata; \
        } \
    protected: \
        bool on_initialize() override; \
        void on_shutdown() override; \
    }; \
    DECLARE_PLUGIN(PluginClass, plugin_name, #major "." #minor "." #patch) \
    DECLARE_PLUGIN_API_VERSION()

/**
 * @brief Plugin with update loop declaration macro
 */
#define DECLARE_UPDATE_PLUGIN(PluginClass, plugin_name, major, minor, patch) \
    class PluginClass : public ecscope::plugins::sdk::PluginBase { \
    public: \
        PluginClass() : PluginBase(plugin_name, {major, minor, patch}) {} \
        static const ecscope::plugins::PluginMetadata& get_static_metadata() { \
            static ecscope::plugins::PluginMetadata metadata; \
            if (metadata.name.empty()) { \
                metadata.name = plugin_name; \
                metadata.version = {major, minor, patch}; \
                metadata.display_name = plugin_name; \
            } \
            return metadata; \
        } \
        void update(double delta_time) override; \
    protected: \
        bool on_initialize() override; \
        void on_shutdown() override; \
    }; \
    DECLARE_PLUGIN(PluginClass, plugin_name, #major "." #minor "." #patch) \
    DECLARE_PLUGIN_API_VERSION()