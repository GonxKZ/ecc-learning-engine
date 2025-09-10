#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <any>
#include <chrono>
#include <optional>
#include <future>
#include <atomic>
#include <mutex>
#include <shared_mutex>

#include "../core/types.hpp"
#include "../core/result.hpp"

namespace ecscope::scripting {

// Forward declarations
class ScriptEngine;
class ScriptContext;
class ScriptDebugger;
class ScriptProfiler;

/**
 * @brief Script execution metrics for performance analysis
 */
struct ScriptMetrics {
    std::chrono::nanoseconds compilation_time{0};
    std::chrono::nanoseconds execution_time{0};
    std::chrono::nanoseconds load_time{0};
    usize memory_usage_bytes{0};
    usize memory_peak_bytes{0};
    u32 function_calls{0};
    u32 garbage_collections{0};
    f64 cpu_time_percent{0.0};
    
    void reset() {
        compilation_time = std::chrono::nanoseconds{0};
        execution_time = std::chrono::nanoseconds{0};
        load_time = std::chrono::nanoseconds{0};
        memory_usage_bytes = 0;
        memory_peak_bytes = 0;
        function_calls = 0;
        garbage_collections = 0;
        cpu_time_percent = 0.0;
    }
};

/**
 * @brief Script error information with detailed context
 */
struct ScriptError {
    std::string script_name;
    std::string message;
    std::string stack_trace;
    u32 line_number{0};
    u32 column_number{0};
    std::string error_type;
    std::chrono::system_clock::time_point timestamp;
    
    ScriptError() = default;
    
    ScriptError(std::string name, std::string msg, 
                std::string trace = "", u32 line = 0, u32 col = 0,
                std::string type = "RuntimeError")
        : script_name(std::move(name))
        , message(std::move(msg))
        , stack_trace(std::move(trace))
        , line_number(line)
        , column_number(col)
        , error_type(std::move(type))
        , timestamp(std::chrono::system_clock::now()) {}
    
    std::string format_error() const {
        std::string formatted = "[" + error_type + "] " + script_name;
        if (line_number > 0) {
            formatted += ":" + std::to_string(line_number);
            if (column_number > 0) {
                formatted += ":" + std::to_string(column_number);
            }
        }
        formatted += " - " + message;
        if (!stack_trace.empty()) {
            formatted += "\nStack trace:\n" + stack_trace;
        }
        return formatted;
    }
};

/**
 * @brief Script execution result with comprehensive error handling
 */
template<typename T>
class ScriptResult {
public:
    static ScriptResult success_result(T&& value, const ScriptMetrics& metrics = {}) {
        ScriptResult result;
        result.success_ = true;
        result.value_ = std::forward<T>(value);
        result.metrics_ = metrics;
        return result;
    }
    
    static ScriptResult error_result(const ScriptError& error) {
        ScriptResult result;
        result.success_ = false;
        result.error_ = error;
        return result;
    }
    
    bool is_success() const { return success_; }
    bool is_error() const { return !success_; }
    
    const T& value() const { 
        if (!success_) {
            throw std::runtime_error("Attempted to access value of failed script result: " + error_.message);
        }
        return value_; 
    }
    
    T& value() { 
        if (!success_) {
            throw std::runtime_error("Attempted to access value of failed script result: " + error_.message);
        }
        return value_; 
    }
    
    const ScriptError& error() const { return error_; }
    const ScriptMetrics& metrics() const { return metrics_; }
    
    // Monadic operations
    template<typename F>
    auto map(F&& func) -> ScriptResult<decltype(func(value_))> {
        if (success_) {
            return ScriptResult<decltype(func(value_))>::success_result(
                func(value_), metrics_);
        } else {
            return ScriptResult<decltype(func(value_))>::error_result(error_);
        }
    }
    
    template<typename F>
    auto flat_map(F&& func) -> decltype(func(value_)) {
        if (success_) {
            return func(value_);
        } else {
            return decltype(func(value_))::error_result(error_);
        }
    }

private:
    bool success_{false};
    T value_{};
    ScriptError error_;
    ScriptMetrics metrics_;
};

// Specialization for void
template<>
class ScriptResult<void> {
public:
    static ScriptResult success_result(const ScriptMetrics& metrics = {}) {
        ScriptResult result;
        result.success_ = true;
        result.metrics_ = metrics;
        return result;
    }
    
    static ScriptResult error_result(const ScriptError& error) {
        ScriptResult result;
        result.success_ = false;
        result.error_ = error;
        return result;
    }
    
    bool is_success() const { return success_; }
    bool is_error() const { return !success_; }
    
    const ScriptError& error() const { return error_; }
    const ScriptMetrics& metrics() const { return metrics_; }

private:
    bool success_{false};
    ScriptError error_;
    ScriptMetrics metrics_;
};

/**
 * @brief Script execution context with state management
 */
class ScriptContext {
public:
    ScriptContext(std::string name, std::string language)
        : script_name(std::move(name))
        , script_language(std::move(language))
        , creation_time(std::chrono::steady_clock::now()) {}
    
    virtual ~ScriptContext() = default;
    
    // Non-copyable, movable
    ScriptContext(const ScriptContext&) = delete;
    ScriptContext& operator=(const ScriptContext&) = delete;
    ScriptContext(ScriptContext&&) = default;
    ScriptContext& operator=(ScriptContext&&) = default;
    
    // Context information
    const std::string script_name;
    const std::string script_language;
    const std::chrono::steady_clock::time_point creation_time;
    
    // Runtime state
    bool is_loaded{false};
    bool is_compiled{false};
    bool is_running{false};
    std::atomic<bool> should_stop{false};
    
    // Source code management
    std::string source_code;
    std::string compiled_bytecode;
    std::string source_file_path;
    std::chrono::file_time_t source_last_modified{};
    
    // Performance metrics
    ScriptMetrics metrics;
    mutable std::shared_mutex metrics_mutex;
    
    // Debugging support
    bool debug_enabled{false};
    std::vector<u32> breakpoints;
    std::unordered_map<std::string, std::any> debug_variables;
    
    // Hot-reload support
    bool hot_reload_enabled{true};
    std::function<void()> reload_callback;
    
    // Get thread-safe metrics copy
    ScriptMetrics get_metrics() const {
        std::shared_lock lock(metrics_mutex);
        return metrics;
    }
    
    // Update metrics in thread-safe way
    void update_metrics(const std::function<void(ScriptMetrics&)>& updater) {
        std::unique_lock lock(metrics_mutex);
        updater(metrics);
    }
};

/**
 * @brief Script language capabilities and metadata
 */
struct ScriptLanguageInfo {
    std::string name;
    std::string version;
    std::string description;
    std::vector<std::string> file_extensions;
    bool supports_compilation{false};
    bool supports_hot_reload{true};
    bool supports_debugging{true};
    bool supports_profiling{true};
    bool supports_coroutines{false};
    bool is_thread_safe{false};
    std::unordered_map<std::string, std::string> features;
};

/**
 * @brief Abstract base class for script engines
 * 
 * Provides a unified interface for different scripting languages
 * with comprehensive development and debugging features
 */
class ScriptEngine {
public:
    virtual ~ScriptEngine() = default;
    
    // Engine lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool is_initialized() const = 0;
    virtual ScriptLanguageInfo get_language_info() const = 0;
    
    // Script management
    virtual ScriptResult<void> load_script(const std::string& name, const std::string& source) = 0;
    virtual ScriptResult<void> load_script_file(const std::string& name, const std::string& filepath) = 0;
    virtual ScriptResult<void> unload_script(const std::string& name);
    virtual ScriptResult<void> reload_script(const std::string& name) = 0;
    virtual bool has_script(const std::string& name) const;
    virtual std::vector<std::string> get_loaded_scripts() const;
    
    // Script compilation (optional for interpreted languages)
    virtual ScriptResult<void> compile_script(const std::string& name) = 0;
    virtual ScriptResult<void> compile_all_scripts();
    
    // Script execution
    virtual ScriptResult<void> execute_script(const std::string& name) = 0;
    virtual ScriptResult<void> execute_string(const std::string& code, const std::string& context_name = "inline") = 0;
    
    // Function calls with type safety
    template<typename ReturnType = void, typename... Args>
    ScriptResult<ReturnType> call_function(const std::string& script_name,
                                          const std::string& function_name,
                                          Args&&... args);
    
    // Async execution support
    std::future<ScriptResult<void>> execute_script_async(const std::string& name);
    
    template<typename ReturnType = void, typename... Args>
    std::future<ScriptResult<ReturnType>> call_function_async(const std::string& script_name,
                                                             const std::string& function_name,
                                                             Args&&... args);
    
    // Variable management
    virtual ScriptResult<void> set_global_variable(const std::string& script_name, 
                                                   const std::string& var_name,
                                                   const std::any& value) = 0;
    virtual ScriptResult<std::any> get_global_variable(const std::string& script_name,
                                                      const std::string& var_name) = 0;
    
    // Memory management
    virtual usize get_memory_usage(const std::string& script_name) const = 0;
    virtual usize get_total_memory_usage() const;
    virtual void collect_garbage() = 0;
    virtual void set_memory_limit(const std::string& script_name, usize limit_bytes) = 0;
    
    // Performance monitoring
    virtual ScriptMetrics get_script_metrics(const std::string& script_name) const;
    virtual void reset_metrics(const std::string& script_name);
    virtual void enable_profiling(const std::string& script_name, bool enable);
    
    // Hot-reload support
    virtual void enable_hot_reload(const std::string& script_name, bool enable);
    virtual void check_for_file_changes();
    virtual void set_hot_reload_callback(const std::string& script_name, 
                                        std::function<void()> callback);
    
    // Debugging support
    virtual void enable_debugging(const std::string& script_name, bool enable);
    virtual void set_breakpoint(const std::string& script_name, u32 line);
    virtual void remove_breakpoint(const std::string& script_name, u32 line);
    virtual void step_over(const std::string& script_name);
    virtual void step_into(const std::string& script_name);
    virtual void continue_execution(const std::string& script_name);
    virtual std::unordered_map<std::string, std::any> get_local_variables(const std::string& script_name);
    
    // Error handling
    virtual void set_error_handler(std::function<void(const ScriptError&)> handler);
    virtual std::vector<ScriptError> get_recent_errors(const std::string& script_name) const;
    virtual void clear_errors(const std::string& script_name);
    
    // Security and sandboxing
    virtual void enable_sandboxing(const std::string& script_name, bool enable);
    virtual void set_execution_timeout(const std::string& script_name, 
                                      std::chrono::milliseconds timeout);
    virtual void set_allowed_modules(const std::string& script_name,
                                    const std::vector<std::string>& modules);
    
    // Engine information and diagnostics
    virtual std::string get_version_info() const = 0;
    virtual std::string explain_performance_characteristics() const = 0;
    virtual std::vector<std::string> get_optimization_suggestions(const std::string& script_name) const = 0;
    virtual void print_engine_diagnostics() const;
    
protected:
    // Context management
    std::unordered_map<std::string, std::unique_ptr<ScriptContext>> script_contexts_;
    mutable std::shared_mutex contexts_mutex_;
    
    // Performance tracking
    void start_performance_measurement(const std::string& script_name, const std::string& operation);
    void end_performance_measurement(const std::string& script_name, const std::string& operation);
    
    // Error management
    std::function<void(const ScriptError&)> error_handler_;
    std::unordered_map<std::string, std::vector<ScriptError>> script_errors_;
    mutable std::mutex errors_mutex_;
    
    // Internal function call implementation
    virtual ScriptResult<void> call_function_impl_void(const std::string& script_name,
                                                       const std::string& function_name,
                                                       const std::vector<std::any>& args) = 0;
    
    template<typename ReturnType>
    virtual ScriptResult<ReturnType> call_function_impl_typed(const std::string& script_name,
                                                             const std::string& function_name,
                                                             const std::vector<std::any>& args) = 0;
    
    // Utility functions
    ScriptContext* get_context(const std::string& name);
    const ScriptContext* get_context(const std::string& name) const;
    void add_error(const std::string& script_name, const ScriptError& error);
    
private:
    // Performance measurement state
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> measurement_start_times_;
    std::mutex measurement_mutex_;
    
    // Hot-reload monitoring
    std::atomic<bool> hot_reload_monitoring_enabled_{true};
    std::thread hot_reload_monitor_thread_;
    void hot_reload_monitor_loop();
};

/**
 * @brief Script engine factory for creating language-specific engines
 */
class ScriptEngineFactory {
public:
    using EngineCreator = std::function<std::unique_ptr<ScriptEngine>()>;
    
    static ScriptEngineFactory& instance() {
        static ScriptEngineFactory factory;
        return factory;
    }
    
    void register_engine(const std::string& language, EngineCreator creator);
    std::unique_ptr<ScriptEngine> create_engine(const std::string& language);
    std::vector<std::string> get_supported_languages() const;
    ScriptLanguageInfo get_language_info(const std::string& language);
    
private:
    std::unordered_map<std::string, EngineCreator> creators_;
    std::unordered_map<std::string, ScriptLanguageInfo> language_infos_;
    std::mutex factory_mutex_;
};

// Template implementations
template<typename ReturnType, typename... Args>
ScriptResult<ReturnType> ScriptEngine::call_function(const std::string& script_name,
                                                     const std::string& function_name,
                                                     Args&&... args) {
    std::vector<std::any> arg_vector;
    arg_vector.reserve(sizeof...(args));
    (arg_vector.emplace_back(std::forward<Args>(args)), ...);
    
    if constexpr (std::is_void_v<ReturnType>) {
        return call_function_impl_void(script_name, function_name, arg_vector);
    } else {
        return call_function_impl_typed<ReturnType>(script_name, function_name, arg_vector);
    }
}

} // namespace ecscope::scripting