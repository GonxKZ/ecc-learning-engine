#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>
#include <thread>
#include <queue>
#include <filesystem>
#include <regex>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

namespace ecscope {
namespace gui {

enum class ScriptLanguage {
    Lua,
    Python,
    JavaScript,
    C_Sharp,
    Custom
};

enum class ScriptExecutionState {
    Idle,
    Running,
    Paused,
    Completed,
    Error,
    Stopped
};

enum class SyntaxHighlightType {
    None,
    Keyword,
    String,
    Number,
    Comment,
    Function,
    Variable,
    Operator,
    Bracket,
    Error
};

struct SyntaxToken {
    size_t start;
    size_t length;
    SyntaxHighlightType type;
    std::string text;
};

struct ScriptBreakpoint {
    int line_number;
    bool enabled;
    bool conditional;
    std::string condition;
    int hit_count;
    std::string log_message;
};

struct ScriptVariable {
    std::string name;
    std::string type;
    std::string value;
    bool is_local;
    bool is_watchable;
    std::vector<ScriptVariable> children;
};

struct ScriptCallFrame {
    std::string function_name;
    std::string source_file;
    int line_number;
    std::unordered_map<std::string, ScriptVariable> local_variables;
};

struct ScriptExecutionResult {
    ScriptExecutionState state;
    std::string output;
    std::string error_message;
    int error_line;
    double execution_time_ms;
    size_t memory_used_bytes;
};

struct ScriptProject {
    std::string project_id;
    std::string name;
    std::string description;
    std::filesystem::path root_directory;
    std::vector<std::filesystem::path> script_files;
    std::string main_script;
    ScriptLanguage language;
    std::unordered_map<std::string, std::string> project_settings;
    std::chrono::system_clock::time_point last_modified;
};

class SyntaxHighlighter {
public:
    SyntaxHighlighter() = default;
    ~SyntaxHighlighter() = default;

    void set_language(ScriptLanguage language);
    std::vector<SyntaxToken> tokenize(const std::string& text);
    ImU32 get_token_color(SyntaxHighlightType type) const;
    
    void add_custom_keyword(const std::string& keyword);
    void add_custom_function(const std::string& function);
    void set_color_scheme(const std::unordered_map<SyntaxHighlightType, ImU32>& colors);

private:
    ScriptLanguage current_language_ = ScriptLanguage::Lua;
    std::vector<std::regex> keyword_patterns_;
    std::vector<std::regex> string_patterns_;
    std::vector<std::regex> number_patterns_;
    std::vector<std::regex> comment_patterns_;
    std::vector<std::regex> function_patterns_;
    std::vector<std::regex> operator_patterns_;
    
    std::unordered_map<SyntaxHighlightType, ImU32> color_scheme_;
    
    void initialize_patterns();
    void add_pattern(std::vector<std::regex>& patterns, const std::string& pattern);
};

class CodeEditor {
public:
    CodeEditor();
    ~CodeEditor() = default;

    void render(const std::string& window_id);
    void set_text(const std::string& text);
    std::string get_text() const;
    void set_language(ScriptLanguage language);
    
    void insert_text_at_cursor(const std::string& text);
    void replace_selection(const std::string& text);
    std::string get_selected_text() const;
    
    void set_read_only(bool read_only);
    void set_show_line_numbers(bool show);
    void set_show_whitespace(bool show);
    void set_word_wrap(bool wrap);
    void set_auto_indent(bool auto_indent);
    
    void goto_line(int line);
    void find_text(const std::string& text, bool case_sensitive = false);
    void replace_text(const std::string& find, const std::string& replace);
    
    void add_breakpoint(int line);
    void remove_breakpoint(int line);
    void toggle_breakpoint(int line);
    std::vector<int> get_breakpoints() const;
    
    void set_error_markers(const std::vector<int>& error_lines);
    void clear_error_markers();
    
    int get_current_line() const;
    int get_cursor_column() const;
    void set_cursor_position(int line, int column);
    
    void undo();
    void redo();
    bool can_undo() const;
    bool can_redo() const;

private:
    std::string content_;
    std::unique_ptr<SyntaxHighlighter> highlighter_;
    
    bool read_only_;
    bool show_line_numbers_;
    bool show_whitespace_;
    bool word_wrap_;
    bool auto_indent_;
    
    ImVec2 cursor_position_;
    ImVec2 selection_start_;
    ImVec2 selection_end_;
    bool has_selection_;
    
    std::vector<ScriptBreakpoint> breakpoints_;
    std::vector<int> error_lines_;
    
    std::vector<std::string> undo_stack_;
    std::vector<std::string> redo_stack_;
    size_t max_undo_levels_;
    
    float line_height_;
    float char_advance_;
    
    void render_text_editor();
    void render_line_numbers();
    void render_breakpoint_margin();
    void handle_keyboard_input();
    void handle_mouse_input();
    void update_syntax_highlighting();
};

class ScriptInterpreter {
public:
    ScriptInterpreter() = default;
    virtual ~ScriptInterpreter() = default;

    virtual bool initialize(ScriptLanguage language) = 0;
    virtual void shutdown() = 0;
    
    virtual ScriptExecutionResult execute_script(const std::string& script) = 0;
    virtual ScriptExecutionResult execute_file(const std::string& file_path) = 0;
    virtual bool stop_execution() = 0;
    virtual bool pause_execution() = 0;
    virtual bool resume_execution() = 0;
    
    virtual void set_global_variable(const std::string& name, const std::string& value) = 0;
    virtual std::string get_global_variable(const std::string& name) = 0;
    virtual std::vector<ScriptVariable> get_all_variables() = 0;
    
    virtual std::vector<ScriptCallFrame> get_call_stack() = 0;
    virtual bool set_breakpoint(const std::string& file, int line) = 0;
    virtual bool remove_breakpoint(const std::string& file, int line) = 0;
    
    virtual void register_function(const std::string& name, std::function<std::string(const std::vector<std::string>&)> callback) = 0;
    virtual void register_object(const std::string& name, void* object_ptr) = 0;

protected:
    ScriptLanguage language_ = ScriptLanguage::Lua;
    ScriptExecutionState state_ = ScriptExecutionState::Idle;
    std::string output_buffer_;
    std::string error_buffer_;
};

class MockScriptInterpreter : public ScriptInterpreter {
public:
    MockScriptInterpreter() = default;
    ~MockScriptInterpreter() override = default;

    bool initialize(ScriptLanguage language) override;
    void shutdown() override;
    
    ScriptExecutionResult execute_script(const std::string& script) override;
    ScriptExecutionResult execute_file(const std::string& file_path) override;
    bool stop_execution() override;
    bool pause_execution() override;
    bool resume_execution() override;
    
    void set_global_variable(const std::string& name, const std::string& value) override;
    std::string get_global_variable(const std::string& name) override;
    std::vector<ScriptVariable> get_all_variables() override;
    
    std::vector<ScriptCallFrame> get_call_stack() override;
    bool set_breakpoint(const std::string& file, int line) override;
    bool remove_breakpoint(const std::string& file, int line) override;
    
    void register_function(const std::string& name, std::function<std::string(const std::vector<std::string>&)> callback) override;
    void register_object(const std::string& name, void* object_ptr) override;

private:
    std::unordered_map<std::string, std::string> global_variables_;
    std::vector<std::pair<std::string, int>> breakpoints_;
    std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> registered_functions_;
    std::chrono::steady_clock::time_point execution_start_;
};

class ScriptDebugger {
public:
    ScriptDebugger() = default;
    ~ScriptDebugger() = default;

    void initialize();
    void render();
    void update();
    
    void set_interpreter(std::shared_ptr<ScriptInterpreter> interpreter);
    void set_current_script_file(const std::string& file_path);
    
    void step_over();
    void step_into();
    void step_out();
    void continue_execution();
    void stop_debugging();
    
    void add_watch_expression(const std::string& expression);
    void remove_watch_expression(const std::string& expression);
    
    bool is_debugging() const { return is_debugging_; }

private:
    std::shared_ptr<ScriptInterpreter> interpreter_;
    std::string current_script_file_;
    bool is_debugging_;
    
    std::vector<std::string> watch_expressions_;
    std::vector<ScriptVariable> watch_values_;
    std::vector<ScriptCallFrame> call_stack_;
    
    void render_call_stack_panel();
    void render_variables_panel();
    void render_watch_panel();
    void render_breakpoints_panel();
    
    void update_watch_values();
    void update_call_stack();
};

class ScriptConsole {
public:
    ScriptConsole();
    ~ScriptConsole() = default;

    void render();
    void clear();
    void add_message(const std::string& message, ImU32 color = IM_COL32_WHITE);
    void add_command(const std::string& command);
    void add_result(const std::string& result, bool is_error = false);
    
    void set_interpreter(std::shared_ptr<ScriptInterpreter> interpreter);
    void set_command_history_size(size_t max_size);

private:
    struct ConsoleEntry {
        std::string text;
        ImU32 color;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::vector<ConsoleEntry> entries_;
    std::vector<std::string> command_history_;
    std::string current_command_;
    int history_index_;
    size_t max_history_size_;
    bool auto_scroll_;
    bool show_timestamps_;
    
    std::shared_ptr<ScriptInterpreter> interpreter_;
    
    void execute_command(const std::string& command);
    void render_input_line();
    void handle_command_history();
};

class ScriptProjectManager {
public:
    ScriptProjectManager() = default;
    ~ScriptProjectManager() = default;

    void initialize();
    void render();
    void update();
    
    void create_new_project(const std::string& name, const std::filesystem::path& location, ScriptLanguage language);
    bool open_project(const std::filesystem::path& project_file);
    void save_current_project();
    void close_current_project();
    
    void add_script_file(const std::filesystem::path& file_path);
    void remove_script_file(const std::filesystem::path& file_path);
    void rename_script_file(const std::filesystem::path& old_path, const std::filesystem::path& new_path);
    
    std::vector<ScriptProject> get_recent_projects() const;
    ScriptProject* get_current_project();
    std::vector<std::filesystem::path> get_project_files() const;

private:
    std::unique_ptr<ScriptProject> current_project_;
    std::vector<ScriptProject> recent_projects_;
    std::string selected_file_;
    
    void render_project_tree();
    void render_file_browser();
    void render_project_settings();
    
    void scan_project_directory();
    void save_project_file();
    void load_project_file(const std::filesystem::path& project_file);
    
    bool is_script_file(const std::filesystem::path& file_path) const;
    std::string get_file_extension_for_language(ScriptLanguage language) const;
};

class ScriptingUI {
public:
    ScriptingUI();
    ~ScriptingUI();

    bool initialize();
    void render();
    void update(float delta_time);
    void shutdown();
    
    void open_script_file(const std::filesystem::path& file_path);
    void save_current_file();
    void save_file_as(const std::filesystem::path& file_path);
    void close_current_file();
    
    void execute_current_script();
    void execute_selection();
    void stop_execution();
    
    void toggle_debugger();
    void set_language(ScriptLanguage language);
    
    void register_engine_function(const std::string& name, std::function<std::string(const std::vector<std::string>&)> callback);
    void register_engine_object(const std::string& name, void* object_ptr);
    
    bool is_window_open() const { return show_window_; }
    void set_window_open(bool open) { show_window_ = open; }
    
    void set_script_execution_callback(std::function<void(const std::string&, const std::string&)> callback);
    void set_file_changed_callback(std::function<void(const std::string&)> callback);

private:
    void render_menu_bar();
    void render_toolbar();
    void render_main_editor();
    void render_output_panel();
    void render_side_panels();
    
    void render_file_explorer();
    void render_script_templates();
    void render_api_reference();
    void render_execution_controls();
    
    void create_new_file(ScriptLanguage language);
    void open_file_dialog();
    void save_file_dialog();
    
    void update_syntax_highlighting();
    void update_auto_completion();
    void update_error_checking();
    
    std::filesystem::path current_file_path_;
    std::unique_ptr<CodeEditor> editor_;
    std::shared_ptr<ScriptInterpreter> interpreter_;
    std::unique_ptr<ScriptDebugger> debugger_;
    std::unique_ptr<ScriptConsole> console_;
    std::unique_ptr<ScriptProjectManager> project_manager_;
    
    ScriptLanguage current_language_;
    ScriptExecutionState execution_state_;
    std::string last_output_;
    std::string last_error_;
    
    std::function<void(const std::string&, const std::string&)> script_execution_callback_;
    std::function<void(const std::string&)> file_changed_callback_;
    
    bool show_window_;
    bool show_file_explorer_;
    bool show_console_;
    bool show_debugger_;
    bool show_project_manager_;
    bool show_templates_;
    bool show_api_reference_;
    
    float editor_width_;
    float output_height_;
    float sidebar_width_;
    
    std::vector<std::string> script_templates_;
    std::vector<std::string> recent_files_;
    
    std::mutex execution_mutex_;
    std::thread execution_thread_;
    bool execution_thread_running_;
};

class ScriptingSystem {
public:
    static ScriptingSystem& instance() {
        static ScriptingSystem instance;
        return instance;
    }

    void initialize();
    void shutdown();
    void update(float delta_time);

    void register_scripting_ui(ScriptingUI* ui);
    void unregister_scripting_ui(ScriptingUI* ui);

    void register_global_function(const std::string& name, std::function<std::string(const std::vector<std::string>&)> callback);
    void register_global_object(const std::string& name, void* object_ptr);
    
    void execute_script(const std::string& script, ScriptLanguage language = ScriptLanguage::Lua);
    void execute_script_file(const std::filesystem::path& file_path);
    
    void set_script_directory(const std::filesystem::path& directory);
    std::filesystem::path get_script_directory() const;

private:
    ScriptingSystem() = default;
    ~ScriptingSystem() = default;

    std::vector<ScriptingUI*> registered_uis_;
    std::unordered_map<ScriptLanguage, std::shared_ptr<ScriptInterpreter>> interpreters_;
    std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> global_functions_;
    std::unordered_map<std::string, void*> global_objects_;
    
    std::filesystem::path script_directory_;
    std::mutex ui_mutex_;
    std::mutex interpreters_mutex_;
};

// Convenience macros for script integration
#define ECSCOPE_REGISTER_SCRIPT_FUNCTION(name, func) \
    ScriptingSystem::instance().register_global_function(name, func)

#define ECSCOPE_REGISTER_SCRIPT_OBJECT(name, obj) \
    ScriptingSystem::instance().register_global_object(name, obj)

#define ECSCOPE_EXECUTE_SCRIPT(script) \
    ScriptingSystem::instance().execute_script(script)

}} // namespace ecscope::gui