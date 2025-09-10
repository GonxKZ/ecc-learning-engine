#pragma once

#include "script_engine.hpp"
#include "lua_engine.hpp"
#include "python_engine.hpp"
#include "../ecs/registry.hpp"

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <filesystem>

namespace ecscope::scripting {

/**
 * @brief Cross-language function call support
 */
struct CrossLanguageCall {
    std::string source_script;
    std::string source_language;
    std::string target_script;
    std::string target_language;
    std::string function_name;
    std::vector<std::any> arguments;
    std::any return_value;
    bool is_async{false};
    std::chrono::nanoseconds call_duration{0};
};

/**
 * @brief Script dependency tracking and resolution
 */
struct ScriptDependency {
    std::string script_name;
    std::string dependency_name;
    std::string dependency_language;
    std::string dependency_path;
    bool is_optional{false};
    bool is_circular{false};
    std::chrono::file_time_t last_check_time{};
};

/**
 * @brief Script project configuration
 */
struct ScriptProject {
    std::string name;
    std::string root_directory;
    std::vector<std::string> script_directories;
    std::vector<std::string> asset_directories;
    std::unordered_map<std::string, std::string> environment_variables;
    std::vector<ScriptDependency> dependencies;
    std::unordered_map<std::string, std::any> project_settings;
    std::string main_script;
    std::string main_language;
};

/**
 * @brief Advanced script hot-reload system with state preservation
 */
class ScriptHotReloader {
public:
    ScriptHotReloader();
    ~ScriptHotReloader();
    
    // Hot-reload configuration
    void enable_hot_reload(bool enable);
    void set_watch_directories(const std::vector<std::string>& directories);
    void set_reload_callback(std::function<void(const std::string&)> callback);
    
    // File watching
    void start_watching();
    void stop_watching();
    bool is_watching() const { return watching_; }
    
    // State preservation
    void enable_state_preservation(bool enable);
    void register_state_serializer(const std::string& script_name, 
                                  std::function<std::string()> serializer);
    void register_state_deserializer(const std::string& script_name,
                                    std::function<void(const std::string&)> deserializer);
    
    // Manual reload triggers
    void force_reload(const std::string& script_name);
    void reload_all_scripts();
    
    // Reload statistics
    usize get_reload_count(const std::string& script_name) const;
    std::chrono::milliseconds get_last_reload_time(const std::string& script_name) const;
    std::vector<std::string> get_recently_reloaded_scripts(std::chrono::seconds window) const;

private:
    std::atomic<bool> watching_{false};
    std::atomic<bool> state_preservation_enabled_{true};
    std::thread file_watcher_thread_;
    
    std::vector<std::string> watch_directories_;
    std::unordered_map<std::string, std::filesystem::file_time_type> file_timestamps_;
    
    std::function<void(const std::string&)> reload_callback_;
    std::unordered_map<std::string, std::function<std::string()>> state_serializers_;
    std::unordered_map<std::string, std::function<void(const std::string&)>> state_deserializers_;
    
    // Reload statistics
    std::unordered_map<std::string, usize> reload_counts_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_reload_times_;
    
    void file_watcher_loop();
    void handle_file_changed(const std::string& filepath);
    std::string extract_script_name_from_path(const std::string& filepath);
};

/**
 * @brief Comprehensive script debugging and profiling manager
 */
class ScriptDebugManager {
public:
    ScriptDebugManager();
    ~ScriptDebugManager() = default;
    
    // Global debugging control
    void enable_global_debugging(bool enable);
    void set_debug_output_file(const std::string& filepath);
    
    // Breakpoint management across all scripts
    void set_global_breakpoint(const std::string& script_name, u32 line);
    void remove_global_breakpoint(const std::string& script_name, u32 line);
    void clear_all_breakpoints();
    
    // Execution control
    void pause_all_scripts();
    void resume_all_scripts();
    void step_all_scripts();
    
    // Cross-language debugging
    void enable_cross_language_debugging(bool enable);
    void trace_cross_language_calls(bool enable);
    
    // Performance profiling
    void start_global_profiling();
    void stop_global_profiling();
    std::string generate_comprehensive_profile_report();
    
    // Memory analysis
    std::unordered_map<std::string, usize> get_memory_usage_by_script();
    std::string generate_memory_analysis_report();
    
    // Error aggregation
    std::vector<ScriptError> get_all_recent_errors(std::chrono::minutes window = std::chrono::minutes(5));
    void set_global_error_handler(std::function<void(const ScriptError&)> handler);

private:
    bool global_debugging_enabled_{false};
    bool cross_language_debugging_enabled_{false};
    bool call_tracing_enabled_{false};
    bool global_profiling_enabled_{false};
    
    std::string debug_output_file_;
    std::function<void(const ScriptError&)> global_error_handler_;
    
    std::unordered_map<std::string, std::vector<u32>> global_breakpoints_;
    std::vector<ScriptError> aggregated_errors_;
    std::mutex error_mutex_;
    
    std::chrono::steady_clock::time_point profiling_start_time_;
    std::unordered_map<std::string, std::chrono::nanoseconds> script_execution_times_;
};

/**
 * @brief Advanced REPL with multi-language support
 */
class MultiLanguageREPL {
public:
    MultiLanguageREPL();
    ~MultiLanguageREPL() = default;
    
    // REPL lifecycle
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    // Language switching
    void set_current_language(const std::string& language);
    std::string get_current_language() const { return current_language_; }
    std::vector<std::string> get_supported_languages() const;
    
    // Command execution with language detection
    std::string execute_command(const std::string& command);
    std::string execute_command_in_language(const std::string& command, const std::string& language);
    
    // Cross-language execution
    std::string call_function_from_repl(const std::string& script_name, 
                                       const std::string& function_name,
                                       const std::string& language,
                                       const std::vector<std::string>& args);
    
    // Enhanced features
    void enable_syntax_highlighting(bool enable);
    void enable_auto_completion(bool enable);
    void enable_command_history(bool enable);
    
    // Magic commands
    void register_magic_command(const std::string& command, 
                               std::function<std::string(const std::string&)> handler);
    std::vector<std::string> get_available_magic_commands() const;
    
    // Session management
    void save_session(const std::string& filepath);
    void load_session(const std::string& filepath);
    void clear_session();

private:
    std::atomic<bool> running_{false};
    std::string current_language_{"lua"};
    std::thread repl_thread_;
    
    bool syntax_highlighting_enabled_{true};
    bool auto_completion_enabled_{true};
    bool command_history_enabled_{true};
    
    std::vector<std::string> session_history_;
    std::unordered_map<std::string, std::function<std::string(const std::string&)>> magic_commands_;
    
    void repl_loop();
    void setup_builtin_magic_commands();
    std::string detect_language(const std::string& command);
    std::string format_output(const std::string& result, const std::string& language);
    
    // Built-in magic commands
    std::string magic_help(const std::string& args);
    std::string magic_switch_language(const std::string& args);
    std::string magic_list_scripts(const std::string& args);
    std::string magic_reload_script(const std::string& args);
    std::string magic_profile_script(const std::string& args);
};

/**
 * @brief Comprehensive script manager with multi-language support
 * 
 * This is the main entry point for the scripting system, providing:
 * - Multi-language script execution and management
 * - Cross-language function calls and data sharing
 * - Advanced hot-reloading with state preservation
 * - Comprehensive debugging and profiling
 * - Interactive multi-language REPL
 * - Project-based script organization
 * - Performance monitoring and optimization
 * - Educational examples and documentation
 */
class ScriptManager {
public:
    ScriptManager();
    ~ScriptManager();
    
    // Manager lifecycle
    bool initialize();
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // Engine registration and management
    void register_engine(const std::string& language, std::unique_ptr<ScriptEngine> engine);
    ScriptEngine* get_engine(const std::string& language);
    std::vector<std::string> get_supported_languages() const;
    ScriptLanguageInfo get_language_info(const std::string& language) const;
    
    // Project management
    void create_project(const ScriptProject& project_config);
    void load_project(const std::string& project_file);
    void save_project(const std::string& project_file) const;
    void set_current_project(const std::string& project_name);
    ScriptProject* get_current_project();
    
    // Script management across languages
    ScriptResult<void> load_script(const std::string& name, const std::string& source, const std::string& language);
    ScriptResult<void> load_script_file(const std::string& filepath);
    ScriptResult<void> unload_script(const std::string& name);
    ScriptResult<void> reload_script(const std::string& name);
    
    // Execution with automatic language detection
    ScriptResult<void> execute_script(const std::string& name);
    ScriptResult<void> execute_script_in_language(const std::string& name, const std::string& language);
    
    // Cross-language function calls
    template<typename ReturnType = void, typename... Args>
    ScriptResult<ReturnType> call_function(const std::string& script_name,
                                          const std::string& function_name,
                                          Args&&... args);
    
    ScriptResult<std::any> call_cross_language_function(const std::string& source_script,
                                                        const std::string& target_script,
                                                        const std::string& function_name,
                                                        const std::vector<std::any>& args);
    
    // Variable sharing between languages
    ScriptResult<void> share_variable(const std::string& source_script,
                                     const std::string& target_script,
                                     const std::string& variable_name);
    
    ScriptResult<void> set_global_shared_variable(const std::string& name, const std::any& value);
    ScriptResult<std::any> get_global_shared_variable(const std::string& name);
    
    // Engine system integration
    void bind_ecs_registry(ecs::Registry* registry);
    void bind_physics_world(physics::World* world);
    void bind_renderer(rendering::Renderer* renderer);
    void bind_audio_system(audio::AudioSystem* audio);
    void bind_all_engine_systems(ecs::Registry* ecs, physics::World* physics, 
                                rendering::Renderer* renderer, audio::AudioSystem* audio);
    
    // Hot-reload management
    ScriptHotReloader* get_hot_reloader() { return &hot_reloader_; }
    void enable_hot_reload(bool enable);
    void watch_directory(const std::string& directory);
    
    // Debugging and profiling
    ScriptDebugManager* get_debug_manager() { return &debug_manager_; }
    void enable_global_debugging(bool enable);
    void start_global_profiling();
    void stop_global_profiling();
    std::string generate_comprehensive_report();
    
    // Interactive REPL
    MultiLanguageREPL* get_repl() { return &multi_repl_; }
    void start_repl();
    void stop_repl();
    
    // Memory management
    usize get_total_memory_usage() const;
    std::unordered_map<std::string, usize> get_memory_usage_by_language() const;
    void collect_all_garbage();
    void set_global_memory_limit(usize limit_bytes);
    
    // Performance monitoring
    ScriptMetrics get_aggregated_metrics() const;
    std::unordered_map<std::string, ScriptMetrics> get_metrics_by_script() const;
    std::unordered_map<std::string, ScriptMetrics> get_metrics_by_language() const;
    
    // Educational features
    void create_comprehensive_tutorial();
    void create_cross_language_examples();
    void generate_best_practices_guide();
    std::string explain_scripting_system_architecture() const;
    
    // Plugin system integration
    void register_script_plugin(const std::string& name, std::function<void(ScriptEngine*)> plugin_setup);
    void apply_plugin_to_engine(const std::string& plugin_name, const std::string& language);
    void apply_plugin_to_all_engines(const std::string& plugin_name);
    
    // Advanced features
    void enable_distributed_scripting(bool enable);
    void enable_script_sandboxing(bool enable);
    void set_execution_timeout(std::chrono::milliseconds timeout);
    
private:
    bool initialized_{false};
    
    // Engine management
    std::unordered_map<std::string, std::unique_ptr<ScriptEngine>> engines_;
    
    // Project management
    std::unordered_map<std::string, ScriptProject> projects_;
    std::string current_project_;
    
    // Script registry with language information
    struct ManagedScript {
        std::string name;
        std::string language;
        std::string filepath;
        std::chrono::file_time_t last_modified;
        bool auto_reload_enabled{true};
    };
    std::unordered_map<std::string, ManagedScript> managed_scripts_;
    
    // Cross-language data sharing
    std::unordered_map<std::string, std::any> global_shared_variables_;
    std::mutex shared_variables_mutex_;
    
    // System components
    ScriptHotReloader hot_reloader_;
    ScriptDebugManager debug_manager_;
    MultiLanguageREPL multi_repl_;
    
    // Plugin system
    std::unordered_map<std::string, std::function<void(ScriptEngine*)>> script_plugins_;
    
    // Performance tracking
    mutable std::shared_mutex metrics_mutex_;
    std::unordered_map<std::string, std::vector<CrossLanguageCall>> cross_language_call_history_;
    
    // Internal utility functions
    std::string detect_script_language(const std::string& filepath);
    std::string get_file_extension(const std::string& filepath);
    void setup_default_engines();
    void apply_project_configuration(const ScriptProject& project);
    
    // Cross-language call implementation
    ScriptResult<std::any> execute_cross_language_call(const CrossLanguageCall& call);
    std::any convert_value_between_languages(const std::any& value, 
                                           const std::string& from_language,
                                           const std::string& to_language);
    
    // Educational content generation
    void generate_getting_started_guide();
    void generate_language_comparison_guide();
    void generate_performance_optimization_guide();
    void generate_debugging_guide();
    
    friend class ScriptHotReloader;
    friend class ScriptDebugManager;
    friend class MultiLanguageREPL;
};

// Template implementations
template<typename ReturnType, typename... Args>
ScriptResult<ReturnType> ScriptManager::call_function(const std::string& script_name,
                                                      const std::string& function_name,
                                                      Args&&... args) {
    auto it = managed_scripts_.find(script_name);
    if (it == managed_scripts_.end()) {
        ScriptError error(script_name, "Script not found in manager", "", 0, 0, "NotFound");
        return ScriptResult<ReturnType>::error_result(error);
    }
    
    auto* engine = get_engine(it->second.language);
    if (!engine) {
        ScriptError error(script_name, "Engine not available for language: " + it->second.language, "", 0, 0, "EngineNotAvailable");
        return ScriptResult<ReturnType>::error_result(error);
    }
    
    return engine->call_function<ReturnType>(script_name, function_name, std::forward<Args>(args)...);
}

// Global script manager instance
ScriptManager& get_script_manager();

} // namespace ecscope::scripting