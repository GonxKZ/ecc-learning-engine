#include "../../include/ecscope/scripting/script_engine.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace ecscope::scripting {

// ScriptEngine implementation
ScriptResult<void> ScriptEngine::unload_script(const std::string& name) {
    std::unique_lock lock(contexts_mutex_);
    
    auto it = script_contexts_.find(name);
    if (it == script_contexts_.end()) {
        ScriptError error(name, "Script not loaded", "", 0, 0, "NotFound");
        return ScriptResult<void>::error_result(error);
    }
    
    // Stop the script if it's running
    auto* context = it->second.get();
    context->should_stop = true;
    
    // Clear any errors associated with this script
    {
        std::lock_guard error_lock(errors_mutex_);
        script_errors_.erase(name);
    }
    
    // Remove the context
    script_contexts_.erase(it);
    
    return ScriptResult<void>::success_result();
}

bool ScriptEngine::has_script(const std::string& name) const {
    std::shared_lock lock(contexts_mutex_);
    return script_contexts_.find(name) != script_contexts_.end();
}

std::vector<std::string> ScriptEngine::get_loaded_scripts() const {
    std::shared_lock lock(contexts_mutex_);
    std::vector<std::string> scripts;
    scripts.reserve(script_contexts_.size());
    
    for (const auto& [name, context] : script_contexts_) {
        scripts.push_back(name);
    }
    
    return scripts;
}

ScriptResult<void> ScriptEngine::compile_all_scripts() {
    std::shared_lock lock(contexts_mutex_);
    
    for (const auto& [name, context] : script_contexts_) {
        if (!context->is_compiled) {
            auto result = compile_script(name);
            if (result.is_error()) {
                return result;
            }
        }
    }
    
    return ScriptResult<void>::success_result();
}

std::future<ScriptResult<void>> ScriptEngine::execute_script_async(const std::string& name) {
    return std::async(std::launch::async, [this, name]() {
        return execute_script(name);
    });
}

usize ScriptEngine::get_total_memory_usage() const {
    std::shared_lock lock(contexts_mutex_);
    usize total = 0;
    
    for (const auto& [name, context] : script_contexts_) {
        total += get_memory_usage(name);
    }
    
    return total;
}

ScriptMetrics ScriptEngine::get_script_metrics(const std::string& script_name) const {
    auto* context = get_context(script_name);
    if (!context) {
        return ScriptMetrics{};
    }
    
    return context->get_metrics();
}

void ScriptEngine::reset_metrics(const std::string& script_name) {
    auto* context = get_context(script_name);
    if (context) {
        context->update_metrics([](ScriptMetrics& metrics) {
            metrics.reset();
        });
    }
}

void ScriptEngine::enable_profiling(const std::string& script_name, bool enable) {
    auto* context = get_context(script_name);
    if (context) {
        // Implementation would depend on the specific engine
        // This is a base implementation
    }
}

void ScriptEngine::enable_hot_reload(const std::string& script_name, bool enable) {
    auto* context = get_context(script_name);
    if (context) {
        context->hot_reload_enabled = enable;
    }
}

void ScriptEngine::check_for_file_changes() {
    std::shared_lock lock(contexts_mutex_);
    
    for (const auto& [name, context] : script_contexts_) {
        if (!context->hot_reload_enabled || context->source_file_path.empty()) {
            continue;
        }
        
        try {
            auto last_write = std::filesystem::last_write_time(context->source_file_path);
            if (last_write > context->source_last_modified) {
                context->source_last_modified = last_write;
                
                // Trigger reload callback if set
                if (context->reload_callback) {
                    context->reload_callback();
                }
                
                // Reload the script
                reload_script(name);
            }
        } catch (const std::filesystem::filesystem_error&) {
            // File might not exist anymore or be inaccessible
            // Handle gracefully
        }
    }
}

void ScriptEngine::set_hot_reload_callback(const std::string& script_name, 
                                          std::function<void()> callback) {
    auto* context = get_context(script_name);
    if (context) {
        context->reload_callback = std::move(callback);
    }
}

void ScriptEngine::enable_debugging(const std::string& script_name, bool enable) {
    auto* context = get_context(script_name);
    if (context) {
        context->debug_enabled = enable;
    }
}

void ScriptEngine::set_breakpoint(const std::string& script_name, u32 line) {
    auto* context = get_context(script_name);
    if (context) {
        context->breakpoints.push_back(line);
    }
}

void ScriptEngine::remove_breakpoint(const std::string& script_name, u32 line) {
    auto* context = get_context(script_name);
    if (context) {
        auto& breakpoints = context->breakpoints;
        breakpoints.erase(
            std::remove(breakpoints.begin(), breakpoints.end(), line),
            breakpoints.end()
        );
    }
}

void ScriptEngine::step_over(const std::string& script_name) {
    // Base implementation - would be overridden by specific engines
}

void ScriptEngine::step_into(const std::string& script_name) {
    // Base implementation - would be overridden by specific engines
}

void ScriptEngine::continue_execution(const std::string& script_name) {
    // Base implementation - would be overridden by specific engines
}

std::unordered_map<std::string, std::any> ScriptEngine::get_local_variables(const std::string& script_name) {
    // Base implementation - would be overridden by specific engines
    return {};
}

void ScriptEngine::set_error_handler(std::function<void(const ScriptError&)> handler) {
    error_handler_ = std::move(handler);
}

std::vector<ScriptError> ScriptEngine::get_recent_errors(const std::string& script_name) const {
    std::lock_guard lock(errors_mutex_);
    
    auto it = script_errors_.find(script_name);
    if (it != script_errors_.end()) {
        return it->second;
    }
    
    return {};
}

void ScriptEngine::clear_errors(const std::string& script_name) {
    std::lock_guard lock(errors_mutex_);
    script_errors_.erase(script_name);
}

void ScriptEngine::enable_sandboxing(const std::string& script_name, bool enable) {
    // Base implementation - would be overridden by specific engines
}

void ScriptEngine::set_execution_timeout(const std::string& script_name, 
                                        std::chrono::milliseconds timeout) {
    // Base implementation - would be overridden by specific engines
}

void ScriptEngine::set_allowed_modules(const std::string& script_name,
                                      const std::vector<std::string>& modules) {
    // Base implementation - would be overridden by specific engines
}

void ScriptEngine::print_engine_diagnostics() const {
    auto language_info = get_language_info();
    
    std::ostringstream oss;
    oss << "=== Script Engine Diagnostics ===\n";
    oss << "Engine: " << language_info.name << " " << language_info.version << "\n";
    oss << "Description: " << language_info.description << "\n";
    oss << "Initialized: " << (is_initialized() ? "Yes" : "No") << "\n";
    oss << "Loaded Scripts: " << get_loaded_scripts().size() << "\n";
    oss << "Total Memory Usage: " << get_total_memory_usage() << " bytes\n";
    
    oss << "\nCapabilities:\n";
    oss << "  Compilation: " << (language_info.supports_compilation ? "Yes" : "No") << "\n";
    oss << "  Hot Reload: " << (language_info.supports_hot_reload ? "Yes" : "No") << "\n";
    oss << "  Debugging: " << (language_info.supports_debugging ? "Yes" : "No") << "\n";
    oss << "  Profiling: " << (language_info.supports_profiling ? "Yes" : "No") << "\n";
    oss << "  Coroutines: " << (language_info.supports_coroutines ? "Yes" : "No") << "\n";
    oss << "  Thread Safety: " << (language_info.is_thread_safe ? "Yes" : "No") << "\n";
    
    if (!language_info.features.empty()) {
        oss << "\nFeatures:\n";
        for (const auto& [feature, description] : language_info.features) {
            oss << "  " << feature << ": " << description << "\n";
        }
    }
    
    std::printf("%s", oss.str().c_str());
}

ScriptContext* ScriptEngine::get_context(const std::string& name) {
    std::shared_lock lock(contexts_mutex_);
    auto it = script_contexts_.find(name);
    return it != script_contexts_.end() ? it->second.get() : nullptr;
}

const ScriptContext* ScriptEngine::get_context(const std::string& name) const {
    std::shared_lock lock(contexts_mutex_);
    auto it = script_contexts_.find(name);
    return it != script_contexts_.end() ? it->second.get() : nullptr;
}

void ScriptEngine::add_error(const std::string& script_name, const ScriptError& error) {
    std::lock_guard lock(errors_mutex_);
    
    auto& errors = script_errors_[script_name];
    errors.push_back(error);
    
    // Keep only the last 100 errors per script
    if (errors.size() > 100) {
        errors.erase(errors.begin());
    }
    
    // Call error handler if set
    if (error_handler_) {
        error_handler_(error);
    }
}

void ScriptEngine::start_performance_measurement(const std::string& script_name, const std::string& operation) {
    std::lock_guard lock(measurement_mutex_);
    std::string key = script_name + "::" + operation;
    measurement_start_times_[key] = std::chrono::steady_clock::now();
}

void ScriptEngine::end_performance_measurement(const std::string& script_name, const std::string& operation) {
    auto end_time = std::chrono::steady_clock::now();
    
    std::lock_guard lock(measurement_mutex_);
    std::string key = script_name + "::" + operation;
    
    auto it = measurement_start_times_.find(key);
    if (it != measurement_start_times_.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - it->second);
        
        // Update script metrics
        auto* context = get_context(script_name);
        if (context) {
            context->update_metrics([&](ScriptMetrics& metrics) {
                if (operation == "execution") {
                    metrics.execution_time += duration;
                } else if (operation == "compilation") {
                    metrics.compilation_time += duration;
                } else if (operation == "load") {
                    metrics.load_time += duration;
                } else if (operation.find("function_call") != std::string::npos) {
                    metrics.function_calls++;
                }
            });
        }
        
        measurement_start_times_.erase(it);
    }
}

void ScriptEngine::hot_reload_monitor_loop() {
    while (hot_reload_monitoring_enabled_.load()) {
        check_for_file_changes();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// ScriptEngineFactory implementation
void ScriptEngineFactory::register_engine(const std::string& language, EngineCreator creator) {
    std::lock_guard lock(factory_mutex_);
    creators_[language] = std::move(creator);
}

std::unique_ptr<ScriptEngine> ScriptEngineFactory::create_engine(const std::string& language) {
    std::lock_guard lock(factory_mutex_);
    
    auto it = creators_.find(language);
    if (it != creators_.end()) {
        return it->second();
    }
    
    return nullptr;
}

std::vector<std::string> ScriptEngineFactory::get_supported_languages() const {
    std::lock_guard lock(factory_mutex_);
    std::vector<std::string> languages;
    languages.reserve(creators_.size());
    
    for (const auto& [language, creator] : creators_) {
        languages.push_back(language);
    }
    
    return languages;
}

ScriptLanguageInfo ScriptEngineFactory::get_language_info(const std::string& language) {
    std::lock_guard lock(factory_mutex_);
    
    auto it = language_infos_.find(language);
    if (it != language_infos_.end()) {
        return it->second;
    }
    
    // If no cached info, create an engine to get the info
    auto engine = create_engine(language);
    if (engine && engine->initialize()) {
        auto info = engine->get_language_info();
        language_infos_[language] = info;
        engine->shutdown();
        return info;
    }
    
    return ScriptLanguageInfo{};
}

} // namespace ecscope::scripting