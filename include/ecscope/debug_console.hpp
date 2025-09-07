#pragma once

/**
 * @file debug_console.hpp
 * @brief Comprehensive Debug Console for ECScope Advanced Profiling
 * 
 * This comprehensive debug console provides:
 * - Interactive command system with auto-completion
 * - Script execution and parameter modification
 * - Live profiling control and analysis
 * - Memory inspection and debugging commands
 * - GPU profiling controls
 * - Performance analysis commands
 * - System state inspection
 * - Educational help system
 * - Command history and scripting
 * 
 * The console supports both text-based commands and integration with
 * the visual debugging interface for a complete debugging experience.
 * 
 * @author ECScope Development Team
 * @date 2024
 */

#include "advanced_profiler.hpp"
#include "types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <deque>
#include <sstream>
#include <regex>
#include <chrono>
#include <fstream>
#include <variant>
#include <optional>

namespace ecscope::profiling {

//=============================================================================
// Command System Infrastructure
//=============================================================================

// Command argument types
using CommandArg = std::variant<std::string, i64, f64, bool>;

// Command result
struct CommandResult {
    enum class Status {
        SUCCESS,
        ERROR,
        INVALID_SYNTAX,
        INVALID_ARGUMENTS,
        COMMAND_NOT_FOUND,
        INSUFFICIENT_PERMISSIONS
    } status;
    
    std::string message;
    std::vector<std::string> output_lines;
    bool should_clear_console = false;
    
    CommandResult(Status s = Status::SUCCESS, const std::string& msg = "") 
        : status(s), message(msg) {}
    
    void add_line(const std::string& line) { output_lines.push_back(line); }
    
    bool is_success() const { return status == Status::SUCCESS; }
    bool is_error() const { return status != Status::SUCCESS; }
    
    std::string get_status_string() const {
        switch (status) {
            case Status::SUCCESS: return "Success";
            case Status::ERROR: return "Error";
            case Status::INVALID_SYNTAX: return "Invalid Syntax";
            case Status::INVALID_ARGUMENTS: return "Invalid Arguments";
            case Status::COMMAND_NOT_FOUND: return "Command Not Found";
            case Status::INSUFFICIENT_PERMISSIONS: return "Insufficient Permissions";
        }
        return "Unknown";
    }
};

// Command parameter definition
struct CommandParameter {
    std::string name;
    std::string type;
    bool required;
    std::string description;
    CommandArg default_value;
    std::vector<std::string> allowed_values; // For enum-like parameters
    
    CommandParameter(const std::string& n, const std::string& t, bool req = true, 
                    const std::string& desc = "", const CommandArg& def_val = std::string{})
        : name(n), type(t), required(req), description(desc), default_value(def_val) {}
};

// Command definition
struct CommandDefinition {
    std::string name;
    std::string category;
    std::string short_description;
    std::string long_description;
    std::vector<CommandParameter> parameters;
    std::vector<std::string> aliases;
    std::vector<std::string> examples;
    bool requires_profiler_active = true;
    u32 permission_level = 0; // 0 = public, higher = more restricted
    
    CommandDefinition(const std::string& n, const std::string& cat, const std::string& short_desc)
        : name(n), category(cat), short_description(short_desc) {}
    
    CommandDefinition& add_parameter(const CommandParameter& param) {
        parameters.push_back(param);
        return *this;
    }
    
    CommandDefinition& add_alias(const std::string& alias) {
        aliases.push_back(alias);
        return *this;
    }
    
    CommandDefinition& add_example(const std::string& example) {
        examples.push_back(example);
        return *this;
    }
    
    CommandDefinition& set_description(const std::string& desc) {
        long_description = desc;
        return *this;
    }
};

// Command execution context
struct CommandContext {
    AdvancedProfiler* profiler;
    std::unordered_map<std::string, CommandArg> variables;
    std::string current_directory;
    u32 permission_level;
    bool verbose_mode;
    
    CommandContext(AdvancedProfiler* prof) 
        : profiler(prof), current_directory("/"), permission_level(0), verbose_mode(false) {}
    
    template<typename T>
    void set_variable(const std::string& name, const T& value) {
        variables[name] = CommandArg(value);
    }
    
    template<typename T>
    std::optional<T> get_variable(const std::string& name) const {
        auto it = variables.find(name);
        if (it != variables.end()) {
            if (std::holds_alternative<T>(it->second)) {
                return std::get<T>(it->second);
            }
        }
        return std::nullopt;
    }
};

// Command handler function type
using CommandHandler = std::function<CommandResult(const std::vector<CommandArg>&, CommandContext&)>;

// Auto-completion provider
using AutoCompleteProvider = std::function<std::vector<std::string>(const std::string& partial_command, const CommandContext&)>;

//=============================================================================
// Console History and State
//=============================================================================

// Console entry (input or output)
struct ConsoleEntry {
    enum class Type {
        COMMAND_INPUT,
        COMMAND_OUTPUT,
        SYSTEM_MESSAGE,
        ERROR_MESSAGE,
        WARNING_MESSAGE,
        INFO_MESSAGE
    } type;
    
    std::string content;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    ConsoleEntry(Type t, const std::string& content) 
        : type(t), content(content), timestamp(std::chrono::high_resolution_clock::now()) {}
    
    std::string get_formatted_time() const {
        auto time_t = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now() + 
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                timestamp - std::chrono::high_resolution_clock::now()));
        return std::to_string(time_t);
    }
};

// Command history management
class CommandHistory {
private:
    std::deque<std::string> commands_;
    usize max_history_size_;
    usize current_index_;
    
public:
    CommandHistory(usize max_size = 1000) 
        : max_history_size_(max_size), current_index_(0) {}
    
    void add_command(const std::string& command) {
        if (!command.empty() && (commands_.empty() || commands_.back() != command)) {
            commands_.push_back(command);
            if (commands_.size() > max_history_size_) {
                commands_.pop_front();
            }
        }
        current_index_ = commands_.size();
    }
    
    std::optional<std::string> get_previous() {
        if (!commands_.empty() && current_index_ > 0) {
            --current_index_;
            return commands_[current_index_];
        }
        return std::nullopt;
    }
    
    std::optional<std::string> get_next() {
        if (!commands_.empty() && current_index_ < commands_.size() - 1) {
            ++current_index_;
            return commands_[current_index_];
        } else if (current_index_ == commands_.size() - 1) {
            current_index_ = commands_.size();
            return std::string{}; // Return empty string to clear input
        }
        return std::nullopt;
    }
    
    void reset_position() { current_index_ = commands_.size(); }
    
    const std::deque<std::string>& get_all_commands() const { return commands_; }
    
    void save_to_file(const std::string& filename) const {
        std::ofstream file(filename);
        for (const auto& command : commands_) {
            file << command << "\n";
        }
    }
    
    void load_from_file(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                commands_.push_back(line);
            }
        }
        if (commands_.size() > max_history_size_) {
            commands_.erase(commands_.begin(), 
                          commands_.begin() + (commands_.size() - max_history_size_));
        }
        current_index_ = commands_.size();
    }
};

//=============================================================================
// Script System
//=============================================================================

// Script execution engine
class ScriptEngine {
private:
    struct ScriptVariable {
        std::string name;
        CommandArg value;
        std::string type;
        std::string description;
    };
    
    std::unordered_map<std::string, ScriptVariable> variables_;
    std::vector<std::string> script_paths_;
    
public:
    // Execute script from file
    CommandResult execute_script(const std::string& filename, CommandContext& context);
    
    // Execute script from string
    CommandResult execute_script_string(const std::string& script, CommandContext& context);
    
    // Variable management
    void set_variable(const std::string& name, const CommandArg& value, const std::string& type = "auto");
    std::optional<CommandArg> get_variable(const std::string& name) const;
    void clear_variables();
    std::vector<std::string> list_variables() const;
    
    // Script path management
    void add_script_path(const std::string& path) { script_paths_.push_back(path); }
    std::vector<std::string> get_script_paths() const { return script_paths_; }
    
    // Built-in script functions
    CommandResult evaluate_expression(const std::string& expression, CommandContext& context);
    CommandResult execute_conditional(const std::string& condition, 
                                    const std::string& true_command,
                                    const std::string& false_command,
                                    CommandContext& context);
    CommandResult execute_loop(const std::string& loop_spec, 
                             const std::string& command,
                             CommandContext& context);
    
private:
    std::vector<std::string> tokenize_script(const std::string& script);
    CommandResult process_script_line(const std::string& line, CommandContext& context);
    std::string expand_variables(const std::string& text);
    bool evaluate_condition(const std::string& condition, const CommandContext& context);
};

//=============================================================================
// Main Debug Console Class
//=============================================================================

class DebugConsole {
private:
    // Core components
    AdvancedProfiler* profiler_;
    std::unique_ptr<ScriptEngine> script_engine_;
    std::unique_ptr<CommandHistory> command_history_;
    CommandContext context_;
    
    // Command system
    std::unordered_map<std::string, CommandDefinition> command_definitions_;
    std::unordered_map<std::string, CommandHandler> command_handlers_;
    std::unordered_map<std::string, std::string> command_aliases_;
    std::vector<AutoCompleteProvider> auto_complete_providers_;
    
    // Console state
    std::deque<ConsoleEntry> console_entries_;
    usize max_console_entries_;
    std::string current_input_;
    usize cursor_position_;
    bool enabled_;
    bool visible_;
    
    // Input handling
    std::string command_prompt_;
    bool input_active_;
    std::vector<std::string> auto_complete_suggestions_;
    usize current_suggestion_;
    
    // Display settings
    u32 max_display_lines_;
    bool show_timestamps_;
    bool auto_scroll_;
    f32 console_height_;
    
    // Filtering and search
    std::string filter_text_;
    ConsoleEntry::Type filter_type_;
    bool case_sensitive_filter_;
    
public:
    DebugConsole(AdvancedProfiler* profiler);
    ~DebugConsole();
    
    void initialize();
    void shutdown();
    void update(f32 delta_time);
    void render();
    
    // Core interface
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }
    
    void set_visible(bool visible) { visible_ = visible; }
    bool is_visible() const { return visible_; }
    
    void toggle_visibility() { visible_ = !visible_; }
    
    // Command execution
    CommandResult execute_command(const std::string& command_line);
    void execute_command_async(const std::string& command_line);
    
    // Output methods
    void print(const std::string& message, ConsoleEntry::Type type = ConsoleEntry::Type::SYSTEM_MESSAGE);
    void print_info(const std::string& message) { print("[INFO] " + message, ConsoleEntry::Type::INFO_MESSAGE); }
    void print_warning(const std::string& message) { print("[WARNING] " + message, ConsoleEntry::Type::WARNING_MESSAGE); }
    void print_error(const std::string& message) { print("[ERROR] " + message, ConsoleEntry::Type::ERROR_MESSAGE); }
    
    // Input handling
    void handle_key_press(int key, bool ctrl, bool shift, bool alt);
    void handle_text_input(const std::string& text);
    void handle_mouse_click(f32 x, f32 y);
    
    // Command registration
    void register_command(const CommandDefinition& definition, const CommandHandler& handler);
    void unregister_command(const std::string& command_name);
    void register_alias(const std::string& alias, const std::string& command);
    
    // Auto-completion
    void add_auto_complete_provider(const AutoCompleteProvider& provider);
    std::vector<std::string> get_auto_complete_suggestions(const std::string& partial_command);
    
    // History management
    void save_history(const std::string& filename) const;
    void load_history(const std::string& filename);
    void clear_history();
    
    // Console management
    void clear_console();
    void save_console_log(const std::string& filename) const;
    void set_filter(const std::string& filter, ConsoleEntry::Type type = ConsoleEntry::Type::SYSTEM_MESSAGE);
    void clear_filter();
    
    // Configuration
    void set_max_entries(usize max_entries) { max_console_entries_ = max_entries; }
    void set_prompt(const std::string& prompt) { command_prompt_ = prompt; }
    void set_show_timestamps(bool show) { show_timestamps_ = show; }
    void set_auto_scroll(bool auto_scroll) { auto_scroll_ = auto_scroll; }
    
    // Script execution
    CommandResult execute_script(const std::string& filename) {
        return script_engine_->execute_script(filename, context_);
    }
    
    CommandResult execute_script_string(const std::string& script) {
        return script_engine_->execute_script_string(script, context_);
    }
    
private:
    void initialize_built_in_commands();
    void setup_profiling_commands();
    void setup_memory_commands();
    void setup_gpu_commands();
    void setup_system_commands();
    void setup_utility_commands();
    void setup_educational_commands();
    
    // Command parsing
    std::vector<CommandArg> parse_command_arguments(const std::string& args_string, 
                                                   const CommandDefinition& definition);
    std::string resolve_alias(const std::string& command_name);
    bool validate_arguments(const std::vector<CommandArg>& args, const CommandDefinition& definition);
    
    // Input processing
    void process_input();
    void update_auto_complete();
    void move_cursor(int delta);
    void delete_character(bool forward = true);
    void insert_text(const std::string& text);
    
    // Display helpers
    std::vector<ConsoleEntry> get_filtered_entries() const;
    std::string format_entry(const ConsoleEntry& entry) const;
    void scroll_to_bottom();
    
    // Built-in command handlers
    CommandResult cmd_help(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_clear(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_exit(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_echo(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_set(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_get(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_list_commands(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_history(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_script(const std::vector<CommandArg>& args, CommandContext& context);
    
    // Profiling command handlers
    CommandResult cmd_profile_start(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_profile_stop(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_profile_reset(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_profile_report(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_list_systems(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_system_info(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_profile_config(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_export_data(const std::vector<CommandArg>& args, CommandContext& context);
    
    // Memory command handlers
    CommandResult cmd_memory_info(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_memory_leaks(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_memory_fragmentation(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_memory_allocations(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_memory_pools(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_memory_track(const std::vector<CommandArg>& args, CommandContext& context);
    
    // GPU command handlers
    CommandResult cmd_gpu_info(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_gpu_metrics(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_gpu_shaders(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_gpu_resources(const std::vector<CommandArg>& args, CommandContext& context);
    
    // Analysis command handlers
    CommandResult cmd_analyze_performance(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_detect_anomalies(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_trend_analysis(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_recommendations(const std::vector<CommandArg>& args, CommandContext& context);
    
    // Utility command handlers
    CommandResult cmd_save_report(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_load_config(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_save_config(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_benchmark(const std::vector<CommandArg>& args, CommandContext& context);
    CommandResult cmd_simulate_load(const std::vector<CommandArg>& args, CommandContext& context);
    
    // Auto-complete providers
    std::vector<std::string> autocomplete_commands(const std::string& partial, const CommandContext& context);
    std::vector<std::string> autocomplete_system_names(const std::string& partial, const CommandContext& context);
    std::vector<std::string> autocomplete_file_paths(const std::string& partial, const CommandContext& context);
    std::vector<std::string> autocomplete_variables(const std::string& partial, const CommandContext& context);
    
    // Helper utilities
    std::string format_bytes(usize bytes) const;
    std::string format_time(std::chrono::high_resolution_clock::duration duration) const;
    std::string format_percentage(f32 percentage) const;
    std::string format_number(f64 number) const;
    Color get_type_color(ConsoleEntry::Type type) const;
    
    template<typename T>
    std::string to_string_with_precision(T value, int precision = 2) {
        std::ostringstream out;
        out.precision(precision);
        out << std::fixed << value;
        return out.str();
    }
};

//=============================================================================
// Educational Help System
//=============================================================================

class EducationalHelpSystem {
private:
    struct HelpTopic {
        std::string name;
        std::string category;
        std::string description;
        std::vector<std::string> content;
        std::vector<std::string> examples;
        std::vector<std::string> related_commands;
        std::vector<std::string> see_also;
    };
    
    std::unordered_map<std::string, HelpTopic> help_topics_;
    DebugConsole* console_;
    
public:
    EducationalHelpSystem(DebugConsole* console) : console_(console) {
        initialize_help_system();
    }
    
    void show_help(const std::string& topic = "");
    void show_tutorial(const std::string& tutorial_name);
    void list_topics(const std::string& category = "");
    void search_help(const std::string& query);
    
private:
    void initialize_help_system();
    void create_profiling_help();
    void create_memory_help();
    void create_gpu_help();
    void create_analysis_help();
    void create_scripting_help();
    void create_troubleshooting_help();
};

} // namespace ecscope::profiling