#include "ecscope/gui/scripting_ui.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <iomanip>
#include <cstring>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

namespace ecscope {
namespace gui {

// SyntaxHighlighter Implementation
void SyntaxHighlighter::set_language(ScriptLanguage language) {
    current_language_ = language;
    initialize_patterns();
}

std::vector<SyntaxToken> SyntaxHighlighter::tokenize(const std::string& text) {
    std::vector<SyntaxToken> tokens;
    if (text.empty()) return tokens;
    
    // Mock tokenization - in a real implementation, this would use proper parsing
    // For demonstration, we'll highlight common programming constructs
    
    size_t pos = 0;
    std::string current_token;
    
    // Simple tokenization example
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        
        if (std::isalnum(c) || c == '_') {
            current_token += c;
        } else {
            if (!current_token.empty()) {
                SyntaxToken token;
                token.start = i - current_token.length();
                token.length = current_token.length();
                token.text = current_token;
                
                // Determine token type based on content
                if (current_token == "function" || current_token == "if" || 
                    current_token == "else" || current_token == "for" || 
                    current_token == "while" || current_token == "return" ||
                    current_token == "local" || current_token == "end" ||
                    current_token == "def" || current_token == "class" ||
                    current_token == "import" || current_token == "from") {
                    token.type = SyntaxHighlightType::Keyword;
                } else if (std::isdigit(current_token[0])) {
                    token.type = SyntaxHighlightType::Number;
                } else {
                    token.type = SyntaxHighlightType::Variable;
                }
                
                tokens.push_back(token);
                current_token.clear();
            }
            
            // Handle operators and brackets
            if (c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '!' ||
                c == '<' || c == '>' || c == '&' || c == '|') {
                SyntaxToken token;
                token.start = i;
                token.length = 1;
                token.text = std::string(1, c);
                token.type = SyntaxHighlightType::Operator;
                tokens.push_back(token);
            } else if (c == '(' || c == ')' || c == '[' || c == ']' || 
                      c == '{' || c == '}') {
                SyntaxToken token;
                token.start = i;
                token.length = 1;
                token.text = std::string(1, c);
                token.type = SyntaxHighlightType::Bracket;
                tokens.push_back(token);
            } else if (c == '"' || c == '\'') {
                // String literal (simplified)
                size_t end = text.find(c, i + 1);
                if (end != std::string::npos) {
                    SyntaxToken token;
                    token.start = i;
                    token.length = end - i + 1;
                    token.text = text.substr(i, token.length);
                    token.type = SyntaxHighlightType::String;
                    tokens.push_back(token);
                    i = end;
                }
            }
        }
    }
    
    return tokens;
}

ImU32 SyntaxHighlighter::get_token_color(SyntaxHighlightType type) const {
    auto it = color_scheme_.find(type);
    if (it != color_scheme_.end()) {
        return it->second;
    }
    
    // Default colors
    switch (type) {
        case SyntaxHighlightType::Keyword:  return IM_COL32(86, 156, 214, 255);   // Blue
        case SyntaxHighlightType::String:   return IM_COL32(214, 157, 133, 255);  // Orange
        case SyntaxHighlightType::Number:   return IM_COL32(181, 206, 168, 255);  // Green
        case SyntaxHighlightType::Comment:  return IM_COL32(106, 153, 85, 255);   // Dark Green
        case SyntaxHighlightType::Function: return IM_COL32(220, 220, 170, 255);  // Yellow
        case SyntaxHighlightType::Variable: return IM_COL32(156, 220, 254, 255);  // Light Blue
        case SyntaxHighlightType::Operator: return IM_COL32(212, 212, 212, 255);  // Light Gray
        case SyntaxHighlightType::Bracket:  return IM_COL32(255, 255, 0, 255);    // Yellow
        case SyntaxHighlightType::Error:    return IM_COL32(255, 0, 0, 255);      // Red
        default:                           return IM_COL32_WHITE;
    }
}

void SyntaxHighlighter::initialize_patterns() {
    keyword_patterns_.clear();
    string_patterns_.clear();
    number_patterns_.clear();
    comment_patterns_.clear();
    function_patterns_.clear();
    operator_patterns_.clear();
    
    // Language-specific patterns would be initialized here
    // This is a simplified mock implementation
}

void SyntaxHighlighter::add_custom_keyword(const std::string& keyword) {
    // In a real implementation, this would add the keyword to patterns
}

void SyntaxHighlighter::add_custom_function(const std::string& function) {
    // In a real implementation, this would add the function to patterns
}

void SyntaxHighlighter::set_color_scheme(const std::unordered_map<SyntaxHighlightType, ImU32>& colors) {
    color_scheme_ = colors;
}

// CodeEditor Implementation
CodeEditor::CodeEditor() 
    : read_only_(false), show_line_numbers_(true), show_whitespace_(false),
      word_wrap_(false), auto_indent_(true), has_selection_(false),
      max_undo_levels_(100), line_height_(0), char_advance_(0) {
    highlighter_ = std::make_unique<SyntaxHighlighter>();
    highlighter_->set_language(ScriptLanguage::Lua);
}

void CodeEditor::render(const std::string& window_id) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::BeginChild(window_id.c_str(), ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    // Calculate text dimensions
    line_height_ = ImGui::GetTextLineHeight();
    char_advance_ = ImGui::CalcTextSize("M").x;
    
    render_line_numbers();
    ImGui::SameLine();
    render_breakpoint_margin();
    ImGui::SameLine();
    render_text_editor();
    
    handle_keyboard_input();
    handle_mouse_input();
    
    ImGui::EndChild();
#endif
}

void CodeEditor::set_text(const std::string& text) {
    if (!read_only_) {
        // Save current state for undo
        if (undo_stack_.size() >= max_undo_levels_) {
            undo_stack_.erase(undo_stack_.begin());
        }
        undo_stack_.push_back(content_);
        redo_stack_.clear();
        
        content_ = text;
        update_syntax_highlighting();
    }
}

std::string CodeEditor::get_text() const {
    return content_;
}

void CodeEditor::set_language(ScriptLanguage language) {
    highlighter_->set_language(language);
    update_syntax_highlighting();
}

void CodeEditor::render_text_editor() {
#ifdef ECSCOPE_HAS_IMGUI
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    
    // Draw background
    draw_list->AddRectFilled(canvas_pos, 
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(30, 30, 30, 255));
    
    // Split content into lines
    std::vector<std::string> lines;
    std::istringstream iss(content_);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    if (lines.empty()) {
        lines.push_back("");
    }
    
    // Render each line with syntax highlighting
    float y_offset = canvas_pos.y;
    for (size_t line_idx = 0; line_idx < lines.size(); ++line_idx) {
        const std::string& line_text = lines[line_idx];
        
        // Highlight current line
        if (line_idx == static_cast<size_t>(get_current_line())) {
            draw_list->AddRectFilled(
                ImVec2(canvas_pos.x, y_offset),
                ImVec2(canvas_pos.x + canvas_size.x, y_offset + line_height_),
                IM_COL32(40, 40, 40, 255));
        }
        
        // Check for error markers
        bool has_error = std::find(error_lines_.begin(), error_lines_.end(), 
                                 static_cast<int>(line_idx + 1)) != error_lines_.end();
        if (has_error) {
            draw_list->AddRectFilled(
                ImVec2(canvas_pos.x, y_offset),
                ImVec2(canvas_pos.x + canvas_size.x, y_offset + line_height_),
                IM_COL32(80, 20, 20, 128));
        }
        
        // Render text with syntax highlighting
        auto tokens = highlighter_->tokenize(line_text);
        float x_offset = canvas_pos.x + 10;
        
        for (const auto& token : tokens) {
            ImU32 color = highlighter_->get_token_color(token.type);
            draw_list->AddText(ImVec2(x_offset, y_offset), color, token.text.c_str());
            x_offset += ImGui::CalcTextSize(token.text.c_str()).x;
        }
        
        y_offset += line_height_;
    }
    
    // Render cursor
    float cursor_x = canvas_pos.x + 10 + cursor_position_.x * char_advance_;
    float cursor_y = canvas_pos.y + cursor_position_.y * line_height_;
    draw_list->AddLine(ImVec2(cursor_x, cursor_y), 
                      ImVec2(cursor_x, cursor_y + line_height_), 
                      IM_COL32_WHITE, 2.0f);
    
    ImGui::InvisibleButton("editor", canvas_size);
#endif
}

void CodeEditor::render_line_numbers() {
#ifdef ECSCOPE_HAS_IMGUI
    if (!show_line_numbers_) return;
    
    ImGui::BeginChild("line_numbers", ImVec2(50, 0), false);
    
    std::vector<std::string> lines;
    std::istringstream iss(content_);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    if (lines.empty()) lines.push_back("");
    
    for (size_t i = 0; i < lines.size(); ++i) {
        bool is_current = (i == static_cast<size_t>(get_current_line()));
        if (is_current) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        }
        
        ImGui::Text("%3d", static_cast<int>(i + 1));
        ImGui::PopStyleColor();
    }
    
    ImGui::EndChild();
#endif
}

void CodeEditor::render_breakpoint_margin() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::BeginChild("breakpoints", ImVec2(20, 0), false);
    
    std::vector<std::string> lines;
    std::istringstream iss(content_);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    if (lines.empty()) lines.push_back("");
    
    for (size_t i = 0; i < lines.size(); ++i) {
        int line_num = static_cast<int>(i + 1);
        bool has_breakpoint = std::find_if(breakpoints_.begin(), breakpoints_.end(),
            [line_num](const ScriptBreakpoint& bp) { return bp.line_number == line_num; }) != breakpoints_.end();
        
        if (has_breakpoint) {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(150, 50, 50, 255));
            if (ImGui::SmallButton("●")) {
                remove_breakpoint(line_num);
            }
            ImGui::PopStyleColor();
        } else {
            if (ImGui::SmallButton(" ")) {
                add_breakpoint(line_num);
            }
        }
    }
    
    ImGui::EndChild();
#endif
}

void CodeEditor::handle_keyboard_input() {
    // Keyboard input handling would be implemented here
    // This is a simplified mock implementation
}

void CodeEditor::handle_mouse_input() {
    // Mouse input handling would be implemented here
    // This is a simplified mock implementation
}

void CodeEditor::update_syntax_highlighting() {
    // Update syntax highlighting cache
    // This would be implemented for performance optimization
}

// Mock implementations for other methods
void CodeEditor::insert_text_at_cursor(const std::string& text) {
    if (!read_only_) {
        // Simple insertion at current position
        content_.insert(static_cast<size_t>(cursor_position_.x), text);
    }
}

std::string CodeEditor::get_selected_text() const {
    if (!has_selection_) return "";
    // Return selected text based on selection bounds
    return content_.substr(0, 10); // Mock implementation
}

void CodeEditor::goto_line(int line) {
    cursor_position_.y = static_cast<float>(line - 1);
    cursor_position_.x = 0;
}

void CodeEditor::add_breakpoint(int line) {
    ScriptBreakpoint bp;
    bp.line_number = line;
    bp.enabled = true;
    bp.conditional = false;
    bp.hit_count = 0;
    breakpoints_.push_back(bp);
}

void CodeEditor::remove_breakpoint(int line) {
    breakpoints_.erase(
        std::remove_if(breakpoints_.begin(), breakpoints_.end(),
            [line](const ScriptBreakpoint& bp) { return bp.line_number == line; }),
        breakpoints_.end());
}

void CodeEditor::toggle_breakpoint(int line) {
    auto it = std::find_if(breakpoints_.begin(), breakpoints_.end(),
        [line](const ScriptBreakpoint& bp) { return bp.line_number == line; });
    
    if (it != breakpoints_.end()) {
        remove_breakpoint(line);
    } else {
        add_breakpoint(line);
    }
}

std::vector<int> CodeEditor::get_breakpoints() const {
    std::vector<int> lines;
    for (const auto& bp : breakpoints_) {
        lines.push_back(bp.line_number);
    }
    return lines;
}

int CodeEditor::get_current_line() const {
    return static_cast<int>(cursor_position_.y) + 1;
}

int CodeEditor::get_cursor_column() const {
    return static_cast<int>(cursor_position_.x) + 1;
}

void CodeEditor::set_error_markers(const std::vector<int>& error_lines) {
    error_lines_ = error_lines;
}

void CodeEditor::clear_error_markers() {
    error_lines_.clear();
}

// Other CodeEditor methods with mock implementations
void CodeEditor::replace_selection(const std::string& text) {}
void CodeEditor::set_read_only(bool read_only) { read_only_ = read_only; }
void CodeEditor::set_show_line_numbers(bool show) { show_line_numbers_ = show; }
void CodeEditor::set_show_whitespace(bool show) { show_whitespace_ = show; }
void CodeEditor::set_word_wrap(bool wrap) { word_wrap_ = wrap; }
void CodeEditor::set_auto_indent(bool auto_indent) { auto_indent_ = auto_indent; }
void CodeEditor::find_text(const std::string& text, bool case_sensitive) {}
void CodeEditor::replace_text(const std::string& find, const std::string& replace) {}
void CodeEditor::set_cursor_position(int line, int column) {
    cursor_position_.x = static_cast<float>(column - 1);
    cursor_position_.y = static_cast<float>(line - 1);
}
void CodeEditor::undo() {}
void CodeEditor::redo() {}
bool CodeEditor::can_undo() const { return !undo_stack_.empty(); }
bool CodeEditor::can_redo() const { return !redo_stack_.empty(); }

// MockScriptInterpreter Implementation
bool MockScriptInterpreter::initialize(ScriptLanguage language) {
    language_ = language;
    state_ = ScriptExecutionState::Idle;
    return true;
}

void MockScriptInterpreter::shutdown() {
    state_ = ScriptExecutionState::Idle;
    global_variables_.clear();
}

ScriptExecutionResult MockScriptInterpreter::execute_script(const std::string& script) {
    ScriptExecutionResult result;
    execution_start_ = std::chrono::steady_clock::now();
    
    // Mock execution
    state_ = ScriptExecutionState::Running;
    
    // Simulate processing time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Generate mock output based on script content
    if (script.find("print") != std::string::npos) {
        result.output = "Mock output: Hello from script!\n";
    } else if (script.find("error") != std::string::npos) {
        result.state = ScriptExecutionState::Error;
        result.error_message = "Mock error: Simulated script error";
        result.error_line = 1;
    } else {
        result.output = "Script executed successfully\n";
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(end_time - execution_start_).count();
    result.memory_used_bytes = 1024; // Mock memory usage
    
    if (result.state != ScriptExecutionState::Error) {
        result.state = ScriptExecutionState::Completed;
    }
    
    state_ = result.state;
    return result;
}

ScriptExecutionResult MockScriptInterpreter::execute_file(const std::string& file_path) {
    // Try to read the file and execute its contents
    std::ifstream file(file_path);
    if (!file.is_open()) {
        ScriptExecutionResult result;
        result.state = ScriptExecutionState::Error;
        result.error_message = "Failed to open file: " + file_path;
        return result;
    }
    
    std::string script_content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    file.close();
    
    return execute_script(script_content);
}

bool MockScriptInterpreter::stop_execution() {
    state_ = ScriptExecutionState::Stopped;
    return true;
}

bool MockScriptInterpreter::pause_execution() {
    if (state_ == ScriptExecutionState::Running) {
        state_ = ScriptExecutionState::Paused;
        return true;
    }
    return false;
}

bool MockScriptInterpreter::resume_execution() {
    if (state_ == ScriptExecutionState::Paused) {
        state_ = ScriptExecutionState::Running;
        return true;
    }
    return false;
}

void MockScriptInterpreter::set_global_variable(const std::string& name, const std::string& value) {
    global_variables_[name] = value;
}

std::string MockScriptInterpreter::get_global_variable(const std::string& name) {
    auto it = global_variables_.find(name);
    return (it != global_variables_.end()) ? it->second : "";
}

std::vector<ScriptVariable> MockScriptInterpreter::get_all_variables() {
    std::vector<ScriptVariable> variables;
    
    for (const auto& pair : global_variables_) {
        ScriptVariable var;
        var.name = pair.first;
        var.value = pair.second;
        var.type = "string"; // Mock type
        var.is_local = false;
        var.is_watchable = true;
        variables.push_back(var);
    }
    
    return variables;
}

std::vector<ScriptCallFrame> MockScriptInterpreter::get_call_stack() {
    std::vector<ScriptCallFrame> stack;
    
    // Mock call stack
    ScriptCallFrame frame1;
    frame1.function_name = "main";
    frame1.source_file = "script.lua";
    frame1.line_number = 15;
    stack.push_back(frame1);
    
    ScriptCallFrame frame2;
    frame2.function_name = "process_data";
    frame2.source_file = "script.lua";
    frame2.line_number = 8;
    stack.push_back(frame2);
    
    return stack;
}

bool MockScriptInterpreter::set_breakpoint(const std::string& file, int line) {
    breakpoints_.push_back({file, line});
    return true;
}

bool MockScriptInterpreter::remove_breakpoint(const std::string& file, int line) {
    breakpoints_.erase(
        std::remove_if(breakpoints_.begin(), breakpoints_.end(),
            [&file, line](const std::pair<std::string, int>& bp) {
                return bp.first == file && bp.second == line;
            }),
        breakpoints_.end());
    return true;
}

void MockScriptInterpreter::register_function(const std::string& name, 
    std::function<std::string(const std::vector<std::string>&)> callback) {
    registered_functions_[name] = callback;
}

void MockScriptInterpreter::register_object(const std::string& name, void* object_ptr) {
    // Mock object registration
}

// ScriptConsole Implementation
ScriptConsole::ScriptConsole() 
    : history_index_(-1), max_history_size_(100), auto_scroll_(true), show_timestamps_(false) {
}

void ScriptConsole::render() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::BeginChild("ConsoleOutput", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    for (const auto& entry : entries_) {
        if (show_timestamps_) {
            auto time_t = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()); // Mock timestamp
            std::ostringstream oss;
            oss << std::put_time(std::localtime(&time_t), "[%H:%M:%S] ");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", oss.str().c_str());
            ImGui::SameLine();
        }
        
        ImVec4 color = ImGui::ColorConvertU32ToFloat4(entry.color);
        ImGui::TextColored(color, "%s", entry.text.c_str());
    }
    
    if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    
    render_input_line();
#endif
}

void ScriptConsole::render_input_line() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Separator();
    
    bool reclaim_focus = false;
    ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue | 
                                     ImGuiInputTextFlags_CallbackHistory;
    
    // Create buffer for ImGui input (imgui_stdlib.h not available)
    static char input_buffer[256] = "";
    if (current_command_.size() < sizeof(input_buffer) - 1) {
        strcpy(input_buffer, current_command_.c_str());
    }
    
    if (ImGui::InputText("##console_input", input_buffer, sizeof(input_buffer), input_flags)) {
        current_command_ = input_buffer;
        execute_command(current_command_);
        current_command_.clear();
        input_buffer[0] = '\0';
        reclaim_focus = true;
    }
    
    // Update current_command_ if user is typing
    if (strlen(input_buffer) != current_command_.size() || strcmp(input_buffer, current_command_.c_str()) != 0) {
        current_command_ = input_buffer;
    }
    
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus) {
        ImGui::SetKeyboardFocusHere(-1);
    }
#endif
}

void ScriptConsole::execute_command(const std::string& command) {
    if (command.empty()) return;
    
    // Add to history
    if (command_history_.empty() || command_history_.back() != command) {
        command_history_.push_back(command);
        if (command_history_.size() > max_history_size_) {
            command_history_.erase(command_history_.begin());
        }
    }
    history_index_ = -1;
    
    // Display command
    add_command("> " + command);
    
    // Execute with interpreter
    if (interpreter_) {
        auto result = interpreter_->execute_script(command);
        if (!result.output.empty()) {
            add_result(result.output, false);
        }
        if (result.state == ScriptExecutionState::Error && !result.error_message.empty()) {
            add_result("Error: " + result.error_message, true);
        }
    } else {
        add_result("No interpreter available", true);
    }
}

void ScriptConsole::clear() {
    entries_.clear();
}

void ScriptConsole::add_message(const std::string& message, ImU32 color) {
    ConsoleEntry entry;
    entry.text = message;
    entry.color = color;
    entry.timestamp = std::chrono::steady_clock::now();
    entries_.push_back(entry);
}

void ScriptConsole::add_command(const std::string& command) {
    add_message(command, IM_COL32(255, 255, 255, 255));
}

void ScriptConsole::add_result(const std::string& result, bool is_error) {
    ImU32 color = is_error ? IM_COL32(255, 100, 100, 255) : IM_COL32(150, 255, 150, 255);
    add_message(result, color);
}

void ScriptConsole::set_interpreter(std::shared_ptr<ScriptInterpreter> interpreter) {
    interpreter_ = interpreter;
}

void ScriptConsole::set_command_history_size(size_t max_size) {
    max_history_size_ = max_size;
}

// ScriptingUI Implementation
ScriptingUI::ScriptingUI() 
    : current_language_(ScriptLanguage::Lua), execution_state_(ScriptExecutionState::Idle),
      show_window_(true), show_file_explorer_(true), show_console_(true), 
      show_debugger_(false), show_project_manager_(false), show_templates_(false),
      show_api_reference_(false), editor_width_(600), output_height_(200), 
      sidebar_width_(250), execution_thread_running_(false) {
    
    editor_ = std::make_unique<CodeEditor>();
    interpreter_ = std::make_shared<MockScriptInterpreter>();
    debugger_ = std::make_unique<ScriptDebugger>();
    console_ = std::make_unique<ScriptConsole>();
    project_manager_ = std::make_unique<ScriptProjectManager>();
    
    console_->set_interpreter(interpreter_);
    debugger_->set_interpreter(interpreter_);
}

ScriptingUI::~ScriptingUI() {
    shutdown();
}

bool ScriptingUI::initialize() {
    interpreter_->initialize(current_language_);
    debugger_->initialize();
    console_->add_message("Scripting environment initialized", IM_COL32(100, 255, 100, 255));
    return true;
}

void ScriptingUI::render() {
#ifdef ECSCOPE_HAS_IMGUI
    if (!show_window_) return;
    
    ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("ECScope Scripting Environment", &show_window_)) {
        render_menu_bar();
        render_toolbar();
        
        // Main layout with splitters
        ImGui::BeginChild("MainContent", ImVec2(0, -output_height_ - 5), false);
        
        // Left panel (file explorer, project manager, etc.)
        if (show_file_explorer_ || show_project_manager_ || show_templates_) {
            ImGui::BeginChild("SidePanel", ImVec2(sidebar_width_, 0), true);
            render_side_panels();
            ImGui::EndChild();
            
            ImGui::SameLine();
            
            // Splitter
            ImGui::InvisibleButton("VSplitter", ImVec2(8, 0));
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
            if (ImGui::IsItemActive()) {
                sidebar_width_ += ImGui::GetIO().MouseDelta.x;
                if (sidebar_width_ < 100) sidebar_width_ = 100;
                if (sidebar_width_ > 400) sidebar_width_ = 400;
            }
            ImGui::SameLine();
        }
        
        // Main editor area
        ImGui::BeginChild("EditorArea", ImVec2(0, 0), true);
        render_main_editor();
        ImGui::EndChild();
        
        ImGui::EndChild();
        
        // Bottom panel (console, output, etc.)
        if (show_console_) {
            ImGui::InvisibleButton("HSplitter", ImVec2(0, 5));
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }
            if (ImGui::IsItemActive()) {
                output_height_ -= ImGui::GetIO().MouseDelta.y;
                if (output_height_ < 100) output_height_ = 100;
                if (output_height_ > 400) output_height_ = 400;
            }
            
            render_output_panel();
        }
    }
    ImGui::End();
    
    // Render debugger window separately
    if (show_debugger_ && debugger_) {
        debugger_->render();
    }
#endif
}

void ScriptingUI::render_menu_bar() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New File", "Ctrl+N")) {
                create_new_file(current_language_);
            }
            if (ImGui::MenuItem("Open File", "Ctrl+O")) {
                open_file_dialog();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                save_current_file();
            }
            if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
                save_file_dialog();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, editor_->can_undo())) {
                editor_->undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, editor_->can_redo())) {
                editor_->redo();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Run")) {
            if (ImGui::MenuItem("Execute Script", "F5")) {
                execute_current_script();
            }
            if (ImGui::MenuItem("Execute Selection", "F9")) {
                execute_selection();
            }
            if (ImGui::MenuItem("Stop Execution", "Shift+F5")) {
                stop_execution();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Debug")) {
            if (ImGui::MenuItem("Toggle Debugger", "F12")) {
                toggle_debugger();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("File Explorer", nullptr, &show_file_explorer_);
            ImGui::MenuItem("Console", nullptr, &show_console_);
            ImGui::MenuItem("Project Manager", nullptr, &show_project_manager_);
            ImGui::MenuItem("Templates", nullptr, &show_templates_);
            ImGui::MenuItem("API Reference", nullptr, &show_api_reference_);
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
#endif
}

void ScriptingUI::render_toolbar() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Button("New")) {
        create_new_file(current_language_);
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Open")) {
        open_file_dialog();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Save")) {
        save_current_file();
    }
    ImGui::SameLine();
    
    ImGui::Separator();
    ImGui::SameLine();
    
    if (ImGui::Button("Run")) {
        execute_current_script();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Stop")) {
        stop_execution();
    }
    ImGui::SameLine();
    
    ImGui::Separator();
    ImGui::SameLine();
    
    // Language selection
    const char* language_names[] = { "Lua", "Python", "JavaScript", "C#", "Custom" };
    int current_lang = static_cast<int>(current_language_);
    if (ImGui::Combo("Language", &current_lang, language_names, 5)) {
        set_language(static_cast<ScriptLanguage>(current_lang));
    }
#endif
}

void ScriptingUI::render_main_editor() {
#ifdef ECSCOPE_HAS_IMGUI
    if (editor_) {
        editor_->render("MainEditor");
    }
#endif
}

void ScriptingUI::render_output_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::BeginChild("OutputPanel", ImVec2(0, output_height_), true);
    
    if (ImGui::BeginTabBar("OutputTabs")) {
        if (ImGui::BeginTabItem("Console")) {
            console_->render();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Output")) {
            ImGui::TextWrapped("%s", last_output_.c_str());
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Errors")) {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", last_error_.c_str());
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::EndChild();
#endif
}

void ScriptingUI::render_side_panels() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginTabBar("SidePanelTabs")) {
        if (show_file_explorer_ && ImGui::BeginTabItem("Explorer")) {
            render_file_explorer();
            ImGui::EndTabItem();
        }
        
        if (show_project_manager_ && ImGui::BeginTabItem("Projects")) {
            project_manager_->render();
            ImGui::EndTabItem();
        }
        
        if (show_templates_ && ImGui::BeginTabItem("Templates")) {
            render_script_templates();
            ImGui::EndTabItem();
        }
        
        if (show_api_reference_ && ImGui::BeginTabItem("API")) {
            render_api_reference();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
#endif
}

void ScriptingUI::render_file_explorer() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Recent Files:");
    ImGui::Separator();
    
    for (const auto& file : recent_files_) {
        if (ImGui::Selectable(std::filesystem::path(file).filename().string().c_str())) {
            open_script_file(file);
        }
    }
#endif
}

void ScriptingUI::render_script_templates() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Script Templates:");
    ImGui::Separator();
    
    static std::vector<std::pair<std::string, std::string>> templates = {
        {"Basic Lua Script", "-- Basic Lua template\nprint(\"Hello, World!\")\n"},
        {"Python Script", "# Basic Python template\nprint(\"Hello, World!\")\n"},
        {"JavaScript", "// Basic JavaScript template\nconsole.log(\"Hello, World!\");\n"}
    };
    
    for (const auto& [name, content] : templates) {
        if (ImGui::Selectable(name.c_str())) {
            editor_->set_text(content);
        }
    }
#endif
}

void ScriptingUI::render_api_reference() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("ECScope Script API:");
    ImGui::Separator();
    
    ImGui::Text("Entity Functions:");
    ImGui::BulletText("create_entity() -> EntityID");
    ImGui::BulletText("destroy_entity(id)");
    ImGui::BulletText("has_component(id, type)");
    
    ImGui::Text("\nComponent Functions:");
    ImGui::BulletText("add_component(id, type, data)");
    ImGui::BulletText("get_component(id, type)");
    ImGui::BulletText("remove_component(id, type)");
    
    ImGui::Text("\nSystem Functions:");
    ImGui::BulletText("get_entities_with(components...)");
    ImGui::BulletText("for_each_entity(callback)");
#endif
}

void ScriptingUI::update(float delta_time) {
    if (debugger_) {
        debugger_->update();
    }
}

void ScriptingUI::shutdown() {
    if (execution_thread_running_ && execution_thread_.joinable()) {
        execution_thread_running_ = false;
        execution_thread_.join();
    }
    
    if (interpreter_) {
        interpreter_->shutdown();
    }
}

void ScriptingUI::execute_current_script() {
    if (!editor_) return;
    
    std::string script = editor_->get_text();
    if (script.empty()) {
        console_->add_result("No script to execute", true);
        return;
    }
    
    console_->add_command("Executing script...");
    
    auto result = interpreter_->execute_script(script);
    last_output_ = result.output;
    last_error_ = result.error_message;
    
    if (result.state == ScriptExecutionState::Error) {
        console_->add_result(result.error_message, true);
        if (result.error_line > 0) {
            editor_->set_error_markers({result.error_line});
        }
    } else {
        console_->add_result(result.output, false);
        editor_->clear_error_markers();
    }
}

// Mock implementations for remaining methods
void ScriptingUI::open_script_file(const std::filesystem::path& file_path) {
    current_file_path_ = file_path;
    // Add to recent files
    auto path_str = file_path.string();
    recent_files_.erase(std::remove(recent_files_.begin(), recent_files_.end(), path_str), recent_files_.end());
    recent_files_.insert(recent_files_.begin(), path_str);
    if (recent_files_.size() > 10) recent_files_.resize(10);
}

void ScriptingUI::save_current_file() {}
void ScriptingUI::save_file_as(const std::filesystem::path& file_path) {}
void ScriptingUI::close_current_file() {}
void ScriptingUI::execute_selection() {}
void ScriptingUI::stop_execution() { interpreter_->stop_execution(); }
void ScriptingUI::toggle_debugger() { show_debugger_ = !show_debugger_; }
void ScriptingUI::set_language(ScriptLanguage language) { 
    current_language_ = language; 
    editor_->set_language(language);
}
void ScriptingUI::create_new_file(ScriptLanguage language) {}
void ScriptingUI::open_file_dialog() {}
void ScriptingUI::save_file_dialog() {}
void ScriptingUI::update_syntax_highlighting() {}
void ScriptingUI::update_auto_completion() {}
void ScriptingUI::update_error_checking() {}

void ScriptingUI::register_engine_function(const std::string& name, std::function<std::string(const std::vector<std::string>&)> callback) {
    interpreter_->register_function(name, callback);
}

void ScriptingUI::register_engine_object(const std::string& name, void* object_ptr) {
    interpreter_->register_object(name, object_ptr);
}

// Mock implementations for other classes
void ScriptDebugger::initialize() {}
void ScriptDebugger::render() {}
void ScriptDebugger::update() {}
void ScriptDebugger::set_interpreter(std::shared_ptr<ScriptInterpreter> interpreter) { interpreter_ = interpreter; }
void ScriptDebugger::set_current_script_file(const std::string& file_path) { current_script_file_ = file_path; }
void ScriptDebugger::step_over() {}
void ScriptDebugger::step_into() {}
void ScriptDebugger::step_out() {}
void ScriptDebugger::continue_execution() {}
void ScriptDebugger::stop_debugging() {}
void ScriptDebugger::add_watch_expression(const std::string& expression) {}
void ScriptDebugger::remove_watch_expression(const std::string& expression) {}
void ScriptDebugger::render_call_stack_panel() {}
void ScriptDebugger::render_variables_panel() {}
void ScriptDebugger::render_watch_panel() {}
void ScriptDebugger::render_breakpoints_panel() {}
void ScriptDebugger::update_watch_values() {}
void ScriptDebugger::update_call_stack() {}

void ScriptProjectManager::initialize() {}
void ScriptProjectManager::render() {}
void ScriptProjectManager::update() {}
void ScriptProjectManager::create_new_project(const std::string& name, const std::filesystem::path& location, ScriptLanguage language) {}
bool ScriptProjectManager::open_project(const std::filesystem::path& project_file) { return false; }
void ScriptProjectManager::save_current_project() {}
void ScriptProjectManager::close_current_project() {}
void ScriptProjectManager::add_script_file(const std::filesystem::path& file_path) {}
void ScriptProjectManager::remove_script_file(const std::filesystem::path& file_path) {}
void ScriptProjectManager::rename_script_file(const std::filesystem::path& old_path, const std::filesystem::path& new_path) {}
std::vector<ScriptProject> ScriptProjectManager::get_recent_projects() const { return {}; }
ScriptProject* ScriptProjectManager::get_current_project() { return nullptr; }
std::vector<std::filesystem::path> ScriptProjectManager::get_project_files() const { return {}; }
void ScriptProjectManager::render_project_tree() {}
void ScriptProjectManager::render_file_browser() {}
void ScriptProjectManager::render_project_settings() {}
void ScriptProjectManager::scan_project_directory() {}
void ScriptProjectManager::save_project_file() {}
void ScriptProjectManager::load_project_file(const std::filesystem::path& project_file) {}
bool ScriptProjectManager::is_script_file(const std::filesystem::path& file_path) const { return false; }
std::string ScriptProjectManager::get_file_extension_for_language(ScriptLanguage language) const { return ".lua"; }

// ScriptingSystem Implementation
void ScriptingSystem::initialize() {}
void ScriptingSystem::shutdown() {}
void ScriptingSystem::update(float delta_time) {}
void ScriptingSystem::register_scripting_ui(ScriptingUI* ui) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.push_back(ui);
}
void ScriptingSystem::unregister_scripting_ui(ScriptingUI* ui) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.erase(std::remove(registered_uis_.begin(), registered_uis_.end(), ui), registered_uis_.end());
}
void ScriptingSystem::register_global_function(const std::string& name, std::function<std::string(const std::vector<std::string>&)> callback) {
    global_functions_[name] = callback;
}
void ScriptingSystem::register_global_object(const std::string& name, void* object_ptr) {
    global_objects_[name] = object_ptr;
}
void ScriptingSystem::execute_script(const std::string& script, ScriptLanguage language) {}
void ScriptingSystem::execute_script_file(const std::filesystem::path& file_path) {}
void ScriptingSystem::set_script_directory(const std::filesystem::path& directory) { script_directory_ = directory; }
std::filesystem::path ScriptingSystem::get_script_directory() const { return script_directory_; }

}} // namespace ecscope::gui