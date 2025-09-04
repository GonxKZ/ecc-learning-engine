#pragma once

#include "script_engine.hpp"
#include "core/types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>

namespace ecscope::scripting {

/**
 * @brief Script execution context for debugging
 */
struct DebugContext {
    std::string script_name;
    std::string function_name;
    usize line_number{0};
    std::string source_line;
    std::unordered_map<std::string, std::any> local_variables;
    std::unordered_map<std::string, std::any> global_variables;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    DebugContext(const std::string& script, const std::string& function, usize line = 0)
        : script_name(script), function_name(function), line_number(line)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
};

/**
 * @brief Breakpoint information
 */
struct Breakpoint {
    enum class Type {
        Line,           // Break on specific line
        Function,       // Break on function entry
        Variable,       // Break on variable change
        Performance,    // Break on performance threshold
        Exception       // Break on script error
    };
    
    Type type;
    std::string script_name;
    std::string target;     // Line number, function name, variable name, etc.
    bool enabled{true};
    bool one_shot{false};   // Remove after first hit
    usize hit_count{0};
    
    // Conditions
    std::string condition_expression;  // Optional condition to evaluate
    usize skip_count{0};               // Skip N hits before breaking
    
    // Performance breakpoint data
    f64 performance_threshold{0.0};    // Time threshold in ms
    
    Breakpoint(Type bp_type, const std::string& script, const std::string& bp_target)
        : type(bp_type), script_name(script), target(bp_target) {}
};

/**
 * @brief Script profiling data
 */
struct ProfileData {
    struct FunctionProfile {
        std::string name;
        usize call_count{0};
        f64 total_time_ms{0.0};
        f64 min_time_ms{std::numeric_limits<f64>::max()};
        f64 max_time_ms{0.0};
        f64 avg_time_ms{0.0};
        
        // Call stack information
        std::vector<std::string> callers;
        std::unordered_map<std::string, usize> caller_counts;
        
        void add_call(f64 execution_time, const std::string& caller = "") {
            call_count++;
            total_time_ms += execution_time;
            min_time_ms = std::min(min_time_ms, execution_time);
            max_time_ms = std::max(max_time_ms, execution_time);
            avg_time_ms = total_time_ms / call_count;
            
            if (!caller.empty()) {
                caller_counts[caller]++;
            }
        }
        
        void reset() {
            call_count = 0;
            total_time_ms = 0.0;
            min_time_ms = std::numeric_limits<f64>::max();
            max_time_ms = 0.0;
            avg_time_ms = 0.0;
            caller_counts.clear();
        }
    };
    
    std::string script_name;
    std::unordered_map<std::string, FunctionProfile> functions;
    
    // Global script metrics
    f64 total_execution_time_ms{0.0};
    usize total_function_calls{0};
    usize memory_allocations{0};
    usize peak_memory_usage{0};
    
    // Performance hotspots
    std::vector<std::string> get_top_functions(usize count = 10) const;
    std::string generate_profile_report() const;
    void reset();
};

/**
 * @brief Script call stack frame
 */
struct CallStackFrame {
    std::string script_name;
    std::string function_name;
    usize line_number{0};
    std::string source_line;
    std::unordered_map<std::string, std::any> local_vars;
    std::chrono::high_resolution_clock::time_point entry_time;
    
    CallStackFrame(const std::string& script, const std::string& function, usize line = 0)
        : script_name(script), function_name(function), line_number(line)
        , entry_time(std::chrono::high_resolution_clock::now()) {}
        
    f64 get_execution_time_ms() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<f64, std::milli>(now - entry_time).count();
    }
};

/**
 * @brief Debug event types for callbacks
 */
enum class DebugEvent {
    BreakpointHit,
    StepInto,
    StepOver,
    StepOut,
    ScriptError,
    PerformanceWarning,
    FunctionEntry,
    FunctionExit,
    VariableChanged
};

/**
 * @brief Debug event callback function
 */
using DebugEventCallback = std::function<void(DebugEvent, const DebugContext&)>;

/**
 * @brief Comprehensive script debugger and profiler
 */
class ScriptDebugger {
public:
    ScriptDebugger();
    ~ScriptDebugger();
    
    // Debugger control
    void attach_to_engine(ScriptEngine* engine);
    void detach_from_engine(ScriptEngine* engine);
    
    bool start_debugging_session(const std::string& session_name);
    void end_debugging_session();
    bool is_debugging_active() const { return debugging_active_; }
    
    // Breakpoint management
    void set_line_breakpoint(const std::string& script_name, usize line_number);
    void set_function_breakpoint(const std::string& script_name, const std::string& function_name);
    void set_variable_breakpoint(const std::string& script_name, const std::string& variable_name);
    void set_performance_breakpoint(const std::string& script_name, f64 threshold_ms);
    void set_exception_breakpoint(const std::string& script_name);
    
    void remove_breakpoint(const std::string& script_name, const std::string& target);
    void clear_all_breakpoints();
    void enable_breakpoint(const std::string& script_name, const std::string& target);
    void disable_breakpoint(const std::string& script_name, const std::string& target);
    
    std::vector<Breakpoint> get_breakpoints() const;
    
    // Execution control
    void continue_execution();
    void step_into();
    void step_over();
    void step_out();
    void pause_execution();
    void stop_execution();
    
    // Call stack inspection
    std::vector<CallStackFrame> get_call_stack() const;
    void set_current_frame(usize frame_index);
    
    // Variable inspection
    std::unordered_map<std::string, std::any> get_local_variables() const;
    std::unordered_map<std::string, std::any> get_global_variables() const;
    std::any get_variable_value(const std::string& variable_name) const;
    void set_variable_value(const std::string& variable_name, const std::any& value);
    
    // Script evaluation during debugging
    std::any evaluate_expression(const std::string& expression);
    
    // Profiling
    void start_profiling(const std::string& script_name);
    void stop_profiling(const std::string& script_name);
    void reset_profiling_data(const std::string& script_name);
    
    const ProfileData* get_profile_data(const std::string& script_name) const;
    std::string generate_profiling_report(const std::string& script_name) const;
    std::string generate_comparative_profiling_report() const;
    
    // Performance analysis
    void analyze_performance_bottlenecks(const std::string& script_name);
    std::vector<std::string> suggest_performance_improvements(const std::string& script_name) const;
    void detect_memory_leaks(const std::string& script_name);
    
    // Event callbacks
    void set_debug_event_callback(DebugEventCallback callback);
    void remove_debug_event_callback();
    
    // Educational features
    void explain_debugging_concepts() const;
    void demonstrate_debugging_workflow() const;
    void create_debugging_tutorial() const;
    
    // Advanced features
    void save_debugging_session(const std::string& filename) const;
    void load_debugging_session(const std::string& filename);
    void export_profiling_data(const std::string& filename, const std::string& format = "json") const;
    
    // Integration with IDE/external tools
    void start_debug_server(u16 port = 9999);
    void stop_debug_server();
    bool is_debug_server_running() const { return debug_server_running_; }

private:
    // Core state
    std::atomic<bool> debugging_active_{false};
    std::atomic<bool> execution_paused_{false};
    std::string current_session_name_;
    
    // Attached engines
    std::unordered_set<ScriptEngine*> attached_engines_;
    mutable std::shared_mutex engines_mutex_;
    
    // Breakpoints
    std::vector<Breakpoint> breakpoints_;
    mutable std::mutex breakpoints_mutex_;
    
    // Call stack
    std::vector<CallStackFrame> call_stack_;
    usize current_frame_index_{0};
    mutable std::mutex call_stack_mutex_;
    
    // Profiling data
    std::unordered_map<std::string, ProfileData> profile_data_;
    std::unordered_set<std::string> profiled_scripts_;
    mutable std::mutex profiling_mutex_;
    
    // Event handling
    DebugEventCallback event_callback_;
    mutable std::mutex callback_mutex_;
    
    // Debug server
    std::atomic<bool> debug_server_running_{false};
    std::unique_ptr<std::thread> debug_server_thread_;
    u16 debug_server_port_{9999};
    
    // Internal methods
    void notify_debug_event(DebugEvent event, const DebugContext& context);
    bool check_breakpoint_conditions(const Breakpoint& breakpoint, const DebugContext& context);
    void update_call_stack(const std::string& script_name, const std::string& function_name, usize line_number);
    void pop_call_stack();
    
    // Profiling helpers
    void record_function_entry(const std::string& script_name, const std::string& function_name);
    void record_function_exit(const std::string& script_name, const std::string& function_name, f64 execution_time);
    void update_memory_statistics(const std::string& script_name, usize memory_usage);
    
    // Performance analysis helpers
    std::vector<std::string> identify_slow_functions(const ProfileData& data, f64 threshold_ms = 10.0) const;
    std::vector<std::string> identify_frequent_functions(const ProfileData& data, usize call_threshold = 1000) const;
    f64 calculate_script_efficiency(const ProfileData& data) const;
    
    // Debug server implementation
    void run_debug_server();
    void handle_debug_client(int client_socket);
    void send_debug_response(int client_socket, const std::string& response);
    
    // Serialization helpers
    std::string serialize_debug_context(const DebugContext& context) const;
    std::string serialize_call_stack() const;
    std::string serialize_variables(const std::unordered_map<std::string, std::any>& variables) const;
};

/**
 * @brief Visual script profiler for educational purposes
 */
class VisualProfiler {
public:
    explicit VisualProfiler(ScriptDebugger* debugger);
    
    // Visualization methods
    void generate_flame_graph(const std::string& script_name, const std::string& output_file);
    void generate_call_graph(const std::string& script_name, const std::string& output_file);
    void generate_timeline_chart(const std::string& script_name, const std::string& output_file);
    void generate_memory_usage_chart(const std::string& script_name, const std::string& output_file);
    
    // Interactive visualization (if UI is available)
    void show_live_profiler_window(const std::string& script_name);
    void show_performance_dashboard();
    
    // Educational visualizations
    void create_performance_comparison_chart(const std::vector<std::string>& script_names);
    void visualize_optimization_impact(const std::string& script_name, const std::string& before_file, const std::string& after_file);
    
private:
    ScriptDebugger* debugger_;
    
    // Visualization helpers
    std::string generate_svg_flame_graph(const ProfileData& data) const;
    std::string generate_dot_call_graph(const ProfileData& data) const;
    std::string generate_timeline_data(const ProfileData& data) const;
    
    // Color generation for visualization
    std::string get_function_color(const std::string& function_name) const;
    std::string get_performance_color(f64 execution_time_ms) const;
};

/**
 * @brief Memory profiler for script engines
 */
class MemoryProfiler {
public:
    struct MemorySnapshot {
        std::string script_name;
        usize total_allocated{0};
        usize peak_allocated{0};
        usize current_allocated{0};
        usize allocation_count{0};
        usize deallocation_count{0};
        std::chrono::high_resolution_clock::time_point timestamp;
        
        // Allocation patterns
        std::unordered_map<usize, usize> allocation_sizes;  // size -> count
        std::vector<std::pair<usize, usize>> large_allocations; // size, count
        
        MemorySnapshot(const std::string& script) 
            : script_name(script), timestamp(std::chrono::high_resolution_clock::now()) {}
    };
    
    explicit MemoryProfiler(ScriptDebugger* debugger);
    
    // Memory monitoring
    void start_memory_monitoring(const std::string& script_name);
    void stop_memory_monitoring(const std::string& script_name);
    
    MemorySnapshot take_memory_snapshot(const std::string& script_name);
    std::vector<MemorySnapshot> get_memory_history(const std::string& script_name) const;
    
    // Leak detection
    void detect_memory_leaks(const std::string& script_name);
    std::vector<std::string> identify_leak_sources(const std::string& script_name) const;
    
    // Memory analysis
    void analyze_allocation_patterns(const std::string& script_name);
    std::string generate_memory_report(const std::string& script_name) const;
    void suggest_memory_optimizations(const std::string& script_name) const;

private:
    ScriptDebugger* debugger_;
    std::unordered_map<std::string, std::vector<MemorySnapshot>> memory_history_;
    std::unordered_set<std::string> monitored_scripts_;
    mutable std::mutex memory_mutex_;
    
    // Internal monitoring
    void record_allocation(const std::string& script_name, usize size);
    void record_deallocation(const std::string& script_name, usize size);
    
    // Analysis helpers
    f64 calculate_memory_efficiency(const std::vector<MemorySnapshot>& history) const;
    std::vector<usize> identify_common_allocation_sizes(const std::vector<MemorySnapshot>& history) const;
    bool detect_memory_growth_trend(const std::vector<MemorySnapshot>& history) const;
};

/**
 * @brief Global debugging interface for easy access
 */
class GlobalDebugInterface {
public:
    static GlobalDebugInterface& instance();
    
    // Quick debugging commands
    void debug_script(const std::string& script_name);
    void profile_script(const std::string& script_name);
    void break_on_line(const std::string& script_name, usize line);
    void break_on_function(const std::string& script_name, const std::string& function);
    
    // Quick profiling
    void start_profiling_all();
    void stop_profiling_all();
    void print_performance_summary();
    
    // Educational shortcuts
    void explain_current_performance();
    void suggest_optimizations();
    void compare_script_performance(const std::string& script1, const std::string& script2);
    
    ScriptDebugger& get_debugger() { return *debugger_; }
    VisualProfiler& get_visual_profiler() { return *visual_profiler_; }
    MemoryProfiler& get_memory_profiler() { return *memory_profiler_; }

private:
    std::unique_ptr<ScriptDebugger> debugger_;
    std::unique_ptr<VisualProfiler> visual_profiler_;
    std::unique_ptr<MemoryProfiler> memory_profiler_;
    
    GlobalDebugInterface();
    ~GlobalDebugInterface() = default;
};

// Convenience macros for debugging
#define DEBUG_SCRIPT(script_name) \
    ecscope::scripting::GlobalDebugInterface::instance().debug_script(script_name)

#define PROFILE_SCRIPT(script_name) \
    ecscope::scripting::GlobalDebugInterface::instance().profile_script(script_name)

#define BREAK_ON_LINE(script_name, line) \
    ecscope::scripting::GlobalDebugInterface::instance().break_on_line(script_name, line)

#define BREAK_ON_FUNCTION(script_name, function) \
    ecscope::scripting::GlobalDebugInterface::instance().break_on_function(script_name, function)

} // namespace ecscope::scripting