#pragma once

#include "core/types.hpp"
#include "core/log.hpp"
#include "memory/arena.hpp"
#include "memory/mem_tracker.hpp"
#include "performance/timer.hpp"
#include <string>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace ecscope::scripting {

/**
 * @brief Script execution performance metrics
 */
struct ScriptMetrics {
    std::string script_name;
    std::string script_language;
    f64 compilation_time_ms{0.0};
    f64 execution_time_ms{0.0};
    f64 average_execution_time_ms{0.0};
    usize execution_count{0};
    usize memory_usage_bytes{0};
    usize peak_memory_usage_bytes{0};
    
    // Performance comparison with native C++
    f64 native_equivalent_time_ms{0.0};  // Time for equivalent C++ operation
    f64 performance_ratio{0.0};          // script_time / native_time (higher = slower)
    f64 overhead_percentage{0.0};        // (performance_ratio - 1.0) * 100
    
    // Cache and allocation metrics
    usize cache_hits{0};
    usize cache_misses{0};
    usize allocations_performed{0};
    usize deallocations_performed{0};
    
    void update_execution(f64 execution_time) {
        execution_time_ms = execution_time;
        average_execution_time_ms = (average_execution_time_ms * execution_count + execution_time) / (execution_count + 1);
        ++execution_count;
        
        // Update performance ratio if native time is available
        if (native_equivalent_time_ms > 0.0) {
            performance_ratio = execution_time_ms / native_equivalent_time_ms;
            overhead_percentage = (performance_ratio - 1.0) * 100.0;
        }
    }
    
    f64 cache_hit_ratio() const {
        usize total = cache_hits + cache_misses;
        return total > 0 ? static_cast<f64>(cache_hits) / total : 0.0;
    }
    
    void reset() {
        compilation_time_ms = 0.0;
        execution_time_ms = 0.0;
        average_execution_time_ms = 0.0;
        execution_count = 0;
        memory_usage_bytes = 0;
        peak_memory_usage_bytes = 0;
        cache_hits = 0;
        cache_misses = 0;
        allocations_performed = 0;
        deallocations_performed = 0;
    }
};

/**
 * @brief Script error information with educational context
 */
struct ScriptError {
    enum class Type {
        SyntaxError,      // Script syntax errors
        RuntimeError,     // Runtime execution errors
        CompilationError, // Compilation/loading errors
        BindingError,     // Binding-related errors
        MemoryError,      // Memory allocation/management errors
        TypeMismatch,     // Type conversion errors
        InvalidArgument   // Invalid function arguments
    };
    
    Type type;
    std::string message;
    std::string script_name;
    usize line_number{0};
    usize column_number{0};
    std::string context_code;        // Code around the error
    std::string educational_hint;    // Educational explanation
    
    std::string to_string() const {
        std::ostringstream oss;
        oss << "[" << type_to_string(type) << "] ";
        if (!script_name.empty()) {
            oss << script_name;
            if (line_number > 0) {
                oss << ":" << line_number;
                if (column_number > 0) {
                    oss << ":" << column_number;
                }
            }
            oss << " - ";
        }
        oss << message;
        if (!educational_hint.empty()) {
            oss << "\nHint: " << educational_hint;
        }
        return oss.str();
    }
    
private:
    const char* type_to_string(Type t) const {
        switch (t) {
            case Type::SyntaxError: return "Syntax Error";
            case Type::RuntimeError: return "Runtime Error";
            case Type::CompilationError: return "Compilation Error";
            case Type::BindingError: return "Binding Error";
            case Type::MemoryError: return "Memory Error";
            case Type::TypeMismatch: return "Type Mismatch";
            case Type::InvalidArgument: return "Invalid Argument";
            default: return "Unknown Error";
        }
    }
};

/**
 * @brief Script execution result with comprehensive information
 */
template<typename T = void>
struct ScriptResult {
    bool success{false};
    std::optional<T> result;
    std::optional<ScriptError> error;
    ScriptMetrics metrics;
    
    explicit operator bool() const { return success; }
    
    const T& value() const {
        if (!success || !result.has_value()) {
            throw std::runtime_error("Attempting to access result of failed script execution");
        }
        return result.value();
    }
    
    T& value() {
        if (!success || !result.has_value()) {
            throw std::runtime_error("Attempting to access result of failed script execution");
        }
        return result.value();
    }
    
    static ScriptResult success_result(T&& value, const ScriptMetrics& metrics = {}) {
        ScriptResult result;
        result.success = true;
        result.result = std::forward<T>(value);
        result.metrics = metrics;
        return result;
    }
    
    static ScriptResult error_result(const ScriptError& err, const ScriptMetrics& metrics = {}) {
        ScriptResult result;
        result.success = false;
        result.error = err;
        result.metrics = metrics;
        return result;
    }
};

// Specialization for void return type
template<>
struct ScriptResult<void> {
    bool success{false};
    std::optional<ScriptError> error;
    ScriptMetrics metrics;
    
    explicit operator bool() const { return success; }
    
    static ScriptResult success_result(const ScriptMetrics& metrics = {}) {
        ScriptResult result;
        result.success = true;
        result.metrics = metrics;
        return result;
    }
    
    static ScriptResult error_result(const ScriptError& err, const ScriptMetrics& metrics = {}) {
        ScriptResult result;
        result.success = false;
        result.error = err;
        result.metrics = metrics;
        return result;
    }
};

/**
 * @brief Hot-reload configuration and state
 */
struct HotReloadConfig {
    bool enable_hot_reload{true};
    std::string watch_directory{"scripts/"};
    std::vector<std::string> watch_extensions{".lua", ".py"};
    u32 poll_interval_ms{100};           // File watching poll interval
    bool zero_downtime_reload{true};     // Atomic script replacement
    bool backup_on_reload{true};         // Keep backup of working scripts
    bool validate_before_reload{true};   // Compile/validate before replacing
    
    // Performance monitoring during reload
    bool profile_reload_performance{true};
    f64 max_reload_time_budget_ms{50.0}; // Maximum time budget for reload
};

/**
 * @brief File watching state for hot-reload
 */
struct FileWatchState {
    std::string filepath;
    std::chrono::file_time_t last_modified;
    usize content_hash{0};
    bool is_valid{true};
    
    FileWatchState() = default;
    FileWatchState(const std::string& path);
    
    bool has_changed() const;
    void update();
};

/**
 * @brief Script execution context with educational tracking
 */
struct ScriptContext {
    std::string name;
    std::string language;
    std::string source_code;
    std::string filepath;
    
    // Memory management
    std::unique_ptr<memory::ArenaAllocator> script_arena;
    usize memory_limit_bytes{64 * 1024 * 1024}; // 64MB default limit
    
    // Execution state
    bool is_compiled{false};
    bool is_loaded{false};
    void* engine_specific_state{nullptr}; // Lua state, Python module, etc.
    
    // Hot-reload state
    FileWatchState file_state;
    std::atomic<bool> requires_reload{false};
    
    // Performance tracking
    ScriptMetrics metrics;
    std::chrono::high_resolution_clock::time_point last_executed;
    
    ScriptContext(const std::string& script_name, const std::string& script_lang)
        : name(script_name), language(script_lang) {
        script_arena = std::make_unique<memory::ArenaAllocator>(memory_limit_bytes);
        last_executed = std::chrono::high_resolution_clock::now();
    }
    
    ~ScriptContext() {
        if (script_arena) {
            script_arena->reset();
        }
    }
    
    // Non-copyable, movable
    ScriptContext(const ScriptContext&) = delete;
    ScriptContext& operator=(const ScriptContext&) = delete;
    ScriptContext(ScriptContext&&) = default;
    ScriptContext& operator=(ScriptContext&&) = default;
    
    void reset_metrics() {
        metrics.reset();
    }
    
    void update_memory_usage() {
        if (script_arena) {
            metrics.memory_usage_bytes = script_arena->used_size();
            metrics.peak_memory_usage_bytes = std::max(
                metrics.peak_memory_usage_bytes, 
                metrics.memory_usage_bytes
            );
        }
    }
};

/**
 * @brief Base class for script engine implementations
 * 
 * This abstract base class defines the interface for all script engines (Lua, Python, etc.)
 * and provides common functionality for performance monitoring, memory management,
 * and educational insights.
 */
class ScriptEngine {
public:
    explicit ScriptEngine(const std::string& engine_name);
    virtual ~ScriptEngine();
    
    // Non-copyable, movable
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;
    ScriptEngine(ScriptEngine&&) = default;
    ScriptEngine& operator=(ScriptEngine&&) = default;
    
    // Core scripting interface
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool is_initialized() const = 0;
    
    // Script loading and compilation
    virtual ScriptResult<void> load_script(const std::string& name, const std::string& source) = 0;
    virtual ScriptResult<void> load_script_file(const std::string& name, const std::string& filepath) = 0;
    virtual ScriptResult<void> compile_script(const std::string& name) = 0;
    virtual ScriptResult<void> reload_script(const std::string& name) = 0;
    
    // Script execution
    virtual ScriptResult<void> execute_script(const std::string& name) = 0;
    
    // Function calls with type safety
    template<typename ReturnType, typename... Args>
    ScriptResult<ReturnType> call_function(const std::string& script_name, 
                                          const std::string& function_name,
                                          Args&&... args) {
        return call_function_impl<ReturnType>(script_name, function_name, 
                                            std::forward<Args>(args)...);
    }
    
    // Memory management
    virtual usize get_memory_usage(const std::string& script_name) const = 0;
    virtual void collect_garbage() = 0;
    virtual void set_memory_limit(const std::string& script_name, usize limit_bytes) = 0;
    
    // Hot-reload functionality
    virtual void enable_hot_reload(const HotReloadConfig& config);
    virtual void disable_hot_reload();
    virtual void check_for_changes();
    virtual bool is_hot_reload_enabled() const { return hot_reload_enabled_; }
    
    // Performance monitoring
    virtual ScriptMetrics get_metrics(const std::string& script_name) const;
    virtual std::vector<ScriptMetrics> get_all_metrics() const;
    virtual void reset_metrics(const std::string& script_name);
    virtual void reset_all_metrics();
    
    // Educational performance comparison
    virtual void benchmark_against_native(const std::string& script_name,
                                         const std::string& operation_name,
                                         std::function<void()> native_implementation,
                                         usize iterations = 1000);
    
    // Script management
    virtual std::vector<std::string> get_loaded_scripts() const;
    virtual bool has_script(const std::string& name) const;
    virtual ScriptResult<void> unload_script(const std::string& name);
    virtual void unload_all_scripts();
    
    // Error handling and debugging
    virtual std::optional<ScriptError> get_last_error() const { return last_error_; }
    virtual void clear_errors() { last_error_.reset(); }
    
    // Engine information
    virtual const std::string& get_engine_name() const { return engine_name_; }
    virtual std::string get_version_info() const = 0;
    virtual std::string generate_performance_report() const;
    
    // Educational insights
    virtual std::string explain_performance_characteristics() const = 0;
    virtual std::vector<std::string> get_optimization_suggestions(const std::string& script_name) const = 0;

protected:
    // Internal implementation for specific engines
    virtual ScriptResult<void> call_function_impl_void(const std::string& script_name,
                                                      const std::string& function_name,
                                                      const std::vector<std::any>& args) = 0;
    
    template<typename ReturnType>
    ScriptResult<ReturnType> call_function_impl(const std::string& script_name,
                                               const std::string& function_name,
                                               auto&&... args) {
        std::vector<std::any> arg_vector;
        (arg_vector.push_back(std::any(args)), ...);
        
        if constexpr (std::is_void_v<ReturnType>) {
            return call_function_impl_void(script_name, function_name, arg_vector);
        } else {
            return call_function_impl_typed<ReturnType>(script_name, function_name, arg_vector);
        }
    }
    
    template<typename ReturnType>
    virtual ScriptResult<ReturnType> call_function_impl_typed(const std::string& script_name,
                                                             const std::string& function_name,
                                                             const std::vector<std::any>& args) = 0;
    
    // Utility methods for implementations
    ScriptContext* get_script_context(const std::string& name);
    const ScriptContext* get_script_context(const std::string& name) const;
    ScriptContext* create_script_context(const std::string& name, const std::string& language);
    void remove_script_context(const std::string& name);
    
    // Error reporting utilities
    void set_error(const ScriptError& error) { last_error_ = error; }
    void clear_error() { last_error_.reset(); }
    
    // Performance tracking utilities
    void start_performance_measurement(const std::string& script_name, const std::string& operation);
    void end_performance_measurement(const std::string& script_name, const std::string& operation);
    
    // Hot-reload utilities
    void start_hot_reload_thread();
    void stop_hot_reload_thread();
    void hot_reload_worker();
    
private:
    std::string engine_name_;
    std::unordered_map<std::string, std::unique_ptr<ScriptContext>> script_contexts_;
    std::optional<ScriptError> last_error_;
    
    // Hot-reload state
    bool hot_reload_enabled_{false};
    HotReloadConfig hot_reload_config_;
    std::unique_ptr<std::thread> hot_reload_thread_;
    std::atomic<bool> hot_reload_shutdown_{false};
    std::mutex contexts_mutex_;
    
    // Performance measurement state
    struct PerformanceMeasurement {
        std::string script_name;
        std::string operation_name;
        std::chrono::high_resolution_clock::time_point start_time;
    };
    thread_local static std::optional<PerformanceMeasurement> current_measurement_;
    
    mutable std::mutex performance_mutex_;
    std::unordered_map<std::string, std::vector<f64>> operation_timings_;
};

/**
 * @brief Script registry for managing multiple script engines
 */
class ScriptRegistry {
public:
    static ScriptRegistry& instance();
    
    // Engine management
    void register_engine(std::unique_ptr<ScriptEngine> engine);
    ScriptEngine* get_engine(const std::string& engine_name);
    const ScriptEngine* get_engine(const std::string& engine_name) const;
    
    // Script operations across all engines
    ScriptResult<void> load_script_auto(const std::string& filepath);
    std::vector<std::string> get_all_engines() const;
    std::vector<std::string> get_all_scripts() const;
    
    // Performance analysis across engines
    std::string generate_comparative_report() const;
    void benchmark_all_engines(const std::string& operation_name,
                              std::function<void()> native_implementation,
                              usize iterations = 1000);
    
    // Educational insights
    std::string explain_engine_differences() const;
    std::vector<std::string> recommend_engine_for_usecase(const std::string& usecase) const;
    
    void shutdown_all();
    
private:
    ScriptRegistry() = default;
    ~ScriptRegistry() { shutdown_all(); }
    
    std::unordered_map<std::string, std::unique_ptr<ScriptEngine>> engines_;
    mutable std::shared_mutex engines_mutex_;
    
    std::string detect_script_language(const std::string& filepath) const;
};

} // namespace ecscope::scripting