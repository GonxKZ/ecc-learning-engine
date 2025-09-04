#include "script_engine.hpp"
#include "core/log.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <shared_mutex>

namespace ecscope::scripting {

// FileWatchState implementation
FileWatchState::FileWatchState(const std::string& path) 
    : filepath(path) {
    try {
        if (std::filesystem::exists(path)) {
            last_modified = std::filesystem::last_write_time(path);
            
            // Calculate content hash for more accurate change detection
            std::ifstream file(path, std::ios::binary);
            if (file) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                content_hash = std::hash<std::string>{}(content);
                is_valid = true;
            }
        } else {
            is_valid = false;
        }
    } catch (const std::exception& e) {
        LOG_WARN("Failed to initialize file watch state for {}: {}", path, e.what());
        is_valid = false;
    }
}

bool FileWatchState::has_changed() const {
    if (!is_valid || !std::filesystem::exists(filepath)) {
        return false;
    }
    
    try {
        auto current_modified = std::filesystem::last_write_time(filepath);
        if (current_modified != last_modified) {
            return true;
        }
        
        // Double-check with content hash to avoid false positives from file system quirks
        std::ifstream file(filepath, std::ios::binary);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            usize current_hash = std::hash<std::string>{}(content);
            return current_hash != content_hash;
        }
    } catch (const std::exception& e) {
        LOG_WARN("Error checking file changes for {}: {}", filepath, e.what());
    }
    
    return false;
}

void FileWatchState::update() {
    if (!std::filesystem::exists(filepath)) {
        is_valid = false;
        return;
    }
    
    try {
        last_modified = std::filesystem::last_write_time(filepath);
        
        std::ifstream file(filepath, std::ios::binary);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            content_hash = std::hash<std::string>{}(content);
            is_valid = true;
        }
    } catch (const std::exception& e) {
        LOG_WARN("Error updating file watch state for {}: {}", filepath, e.what());
        is_valid = false;
    }
}

// Static member definitions
thread_local std::optional<ScriptEngine::PerformanceMeasurement> ScriptEngine::current_measurement_;

// ScriptEngine implementation
ScriptEngine::ScriptEngine(const std::string& engine_name)
    : engine_name_(engine_name) {
    LOG_INFO("Created {} script engine", engine_name_);
}

ScriptEngine::~ScriptEngine() {
    stop_hot_reload_thread();
    LOG_INFO("Destroyed {} script engine", engine_name_);
}

void ScriptEngine::enable_hot_reload(const HotReloadConfig& config) {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    
    if (hot_reload_enabled_) {
        LOG_WARN("Hot-reload already enabled for {} engine", engine_name_);
        return;
    }
    
    hot_reload_config_ = config;
    hot_reload_enabled_ = true;
    hot_reload_shutdown_ = false;
    
    // Initialize file watching for existing scripts
    for (auto& [name, context] : script_contexts_) {
        if (!context->filepath.empty() && std::filesystem::exists(context->filepath)) {
            context->file_state = FileWatchState(context->filepath);
        }
    }
    
    start_hot_reload_thread();
    
    LOG_INFO("Hot-reload enabled for {} engine (watch dir: {}, poll interval: {}ms)", 
             engine_name_, config.watch_directory, config.poll_interval_ms);
}

void ScriptEngine::disable_hot_reload() {
    if (!hot_reload_enabled_) {
        return;
    }
    
    hot_reload_enabled_ = false;
    stop_hot_reload_thread();
    
    LOG_INFO("Hot-reload disabled for {} engine", engine_name_);
}

void ScriptEngine::check_for_changes() {
    if (!hot_reload_enabled_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    
    for (auto& [name, context] : script_contexts_) {
        if (!context->filepath.empty() && context->file_state.has_changed()) {
            LOG_INFO("Detected changes in script: {} ({})", name, context->filepath);
            context->requires_reload = true;
        }
    }
}

ScriptMetrics ScriptEngine::get_metrics(const std::string& script_name) const {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    
    auto it = script_contexts_.find(script_name);
    if (it != script_contexts_.end()) {
        return it->second->metrics;
    }
    
    return ScriptMetrics{script_name, engine_name_};
}

std::vector<ScriptMetrics> ScriptEngine::get_all_metrics() const {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    
    std::vector<ScriptMetrics> all_metrics;
    all_metrics.reserve(script_contexts_.size());
    
    for (const auto& [name, context] : script_contexts_) {
        all_metrics.push_back(context->metrics);
    }
    
    return all_metrics;
}

void ScriptEngine::reset_metrics(const std::string& script_name) {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    
    auto it = script_contexts_.find(script_name);
    if (it != script_contexts_.end()) {
        it->second->reset_metrics();
        LOG_DEBUG("Reset metrics for script: {}", script_name);
    }
}

void ScriptEngine::reset_all_metrics() {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    
    for (auto& [name, context] : script_contexts_) {
        context->reset_metrics();
    }
    
    LOG_INFO("Reset all metrics for {} engine", engine_name_);
}

void ScriptEngine::benchmark_against_native(const std::string& script_name,
                                          const std::string& operation_name,
                                          std::function<void()> native_implementation,
                                          usize iterations) {
    LOG_INFO("Benchmarking {} script '{}' against native C++ implementation", 
             engine_name_, script_name);
    
    // Benchmark native implementation
    auto start = std::chrono::high_resolution_clock::now();
    for (usize i = 0; i < iterations; ++i) {
        native_implementation();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    f64 native_time = std::chrono::duration<f64, std::milli>(end - start).count() / iterations;
    
    // Store native timing for comparison
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    auto it = script_contexts_.find(script_name);
    if (it != script_contexts_.end()) {
        it->second->metrics.native_equivalent_time_ms = native_time;
        
        // Update performance ratio if we have script execution data
        if (it->second->metrics.average_execution_time_ms > 0.0) {
            it->second->metrics.performance_ratio = 
                it->second->metrics.average_execution_time_ms / native_time;
            it->second->metrics.overhead_percentage = 
                (it->second->metrics.performance_ratio - 1.0) * 100.0;
        }
        
        LOG_INFO("Native {} execution: {:.3f}ms avg, Script overhead: {:.1f}%",
                operation_name, native_time, it->second->metrics.overhead_percentage);
    }
}

std::vector<std::string> ScriptEngine::get_loaded_scripts() const {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    
    std::vector<std::string> script_names;
    script_names.reserve(script_contexts_.size());
    
    for (const auto& [name, context] : script_contexts_) {
        script_names.push_back(name);
    }
    
    return script_names;
}

bool ScriptEngine::has_script(const std::string& name) const {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    return script_contexts_.find(name) != script_contexts_.end();
}

ScriptResult<void> ScriptEngine::unload_script(const std::string& name) {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    
    auto it = script_contexts_.find(name);
    if (it == script_contexts_.end()) {
        ScriptError error{
            ScriptError::Type::InvalidArgument,
            "Script not found: " + name,
            name,
            0, 0,
            "",
            "Check if the script was loaded with the correct name"
        };
        return ScriptResult<void>::error_result(error);
    }
    
    script_contexts_.erase(it);
    LOG_INFO("Unloaded script: {}", name);
    
    return ScriptResult<void>::success_result();
}

void ScriptEngine::unload_all_scripts() {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    
    usize count = script_contexts_.size();
    script_contexts_.clear();
    
    LOG_INFO("Unloaded all {} scripts from {} engine", count, engine_name_);
}

std::string ScriptEngine::generate_performance_report() const {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    
    std::ostringstream oss;
    oss << "=== " << engine_name_ << " Script Engine Performance Report ===\n";
    oss << "Total Scripts Loaded: " << script_contexts_.size() << "\n\n";
    
    for (const auto& [name, context] : script_contexts_) {
        const auto& metrics = context->metrics;
        
        oss << "Script: " << name << "\n";
        oss << "  Language: " << metrics.script_language << "\n";
        oss << "  Executions: " << metrics.execution_count << "\n";
        oss << "  Average Execution Time: " << metrics.average_execution_time_ms << "ms\n";
        oss << "  Compilation Time: " << metrics.compilation_time_ms << "ms\n";
        oss << "  Memory Usage: " << (metrics.memory_usage_bytes / 1024) << " KB\n";
        oss << "  Peak Memory: " << (metrics.peak_memory_usage_bytes / 1024) << " KB\n";
        
        if (metrics.native_equivalent_time_ms > 0.0) {
            oss << "  Performance vs Native: " << metrics.performance_ratio << "x ";
            if (metrics.performance_ratio > 1.0) {
                oss << "(" << metrics.overhead_percentage << "% slower)";
            } else {
                oss << "(" << (100.0 - metrics.performance_ratio * 100.0) << "% faster)";
            }
            oss << "\n";
        }
        
        oss << "  Cache Hit Ratio: " << (metrics.cache_hit_ratio() * 100.0) << "%\n";
        oss << "  Allocations: " << metrics.allocations_performed << "\n";
        oss << "\n";
    }
    
    return oss.str();
}

// Protected utility methods
ScriptContext* ScriptEngine::get_script_context(const std::string& name) {
    auto it = script_contexts_.find(name);
    return it != script_contexts_.end() ? it->second.get() : nullptr;
}

const ScriptContext* ScriptEngine::get_script_context(const std::string& name) const {
    auto it = script_contexts_.find(name);
    return it != script_contexts_.end() ? it->second.get() : nullptr;
}

ScriptContext* ScriptEngine::create_script_context(const std::string& name, const std::string& language) {
    auto context = std::make_unique<ScriptContext>(name, language);
    context->metrics.script_name = name;
    context->metrics.script_language = language;
    
    ScriptContext* context_ptr = context.get();
    script_contexts_[name] = std::move(context);
    
    return context_ptr;
}

void ScriptEngine::remove_script_context(const std::string& name) {
    script_contexts_.erase(name);
}

void ScriptEngine::start_performance_measurement(const std::string& script_name, const std::string& operation) {
    current_measurement_ = PerformanceMeasurement{
        script_name,
        operation,
        std::chrono::high_resolution_clock::now()
    };
}

void ScriptEngine::end_performance_measurement(const std::string& script_name, const std::string& operation) {
    if (!current_measurement_.has_value() || 
        current_measurement_->script_name != script_name ||
        current_measurement_->operation_name != operation) {
        return;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 duration_ms = std::chrono::duration<f64, std::milli>(
        end_time - current_measurement_->start_time).count();
    
    // Update script metrics
    {
        std::lock_guard<std::mutex> lock(contexts_mutex_);
        auto it = script_contexts_.find(script_name);
        if (it != script_contexts_.end()) {
            if (operation == "execution") {
                it->second->metrics.update_execution(duration_ms);
            } else if (operation == "compilation") {
                it->second->metrics.compilation_time_ms = duration_ms;
            }
        }
    }
    
    // Store timing data for analysis
    {
        std::lock_guard<std::mutex> lock(performance_mutex_);
        operation_timings_[script_name + "::" + operation].push_back(duration_ms);
    }
    
    current_measurement_.reset();
}

void ScriptEngine::start_hot_reload_thread() {
    if (hot_reload_thread_) {
        return; // Already started
    }
    
    hot_reload_thread_ = std::make_unique<std::thread>(&ScriptEngine::hot_reload_worker, this);
    LOG_DEBUG("Started hot-reload thread for {} engine", engine_name_);
}

void ScriptEngine::stop_hot_reload_thread() {
    if (!hot_reload_thread_) {
        return;
    }
    
    hot_reload_shutdown_ = true;
    
    if (hot_reload_thread_->joinable()) {
        hot_reload_thread_->join();
    }
    
    hot_reload_thread_.reset();
    LOG_DEBUG("Stopped hot-reload thread for {} engine", engine_name_);
}

void ScriptEngine::hot_reload_worker() {
    LOG_DEBUG("Hot-reload worker started for {} engine", engine_name_);
    
    while (!hot_reload_shutdown_.load()) {
        try {
            check_for_changes();
            
            // Process scripts that require reloading
            std::vector<std::string> scripts_to_reload;
            {
                std::lock_guard<std::mutex> lock(contexts_mutex_);
                for (auto& [name, context] : script_contexts_) {
                    if (context->requires_reload.exchange(false)) {
                        scripts_to_reload.push_back(name);
                    }
                }
            }
            
            // Perform reloads outside of lock
            for (const auto& script_name : scripts_to_reload) {
                auto start_time = std::chrono::high_resolution_clock::now();
                
                auto result = reload_script(script_name);
                
                auto end_time = std::chrono::high_resolution_clock::now();
                f64 reload_time = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
                
                if (result) {
                    LOG_INFO("Hot-reloaded script '{}' in {:.2f}ms", script_name, reload_time);
                    
                    if (reload_time > hot_reload_config_.max_reload_time_budget_ms) {
                        LOG_WARN("Hot-reload of '{}' exceeded time budget ({:.2f}ms > {:.2f}ms)",
                                script_name, reload_time, hot_reload_config_.max_reload_time_budget_ms);
                    }
                } else {
                    LOG_ERROR("Failed to hot-reload script '{}': {}", 
                             script_name, 
                             result.error ? result.error->to_string() : "Unknown error");
                }
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Error in hot-reload worker: {}", e.what());
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(hot_reload_config_.poll_interval_ms));
    }
    
    LOG_DEBUG("Hot-reload worker stopped for {} engine", engine_name_);
}

// ScriptRegistry implementation
ScriptRegistry& ScriptRegistry::instance() {
    static ScriptRegistry registry;
    return registry;
}

void ScriptRegistry::register_engine(std::unique_ptr<ScriptEngine> engine) {
    if (!engine) {
        LOG_ERROR("Attempted to register null script engine");
        return;
    }
    
    std::unique_lock<std::shared_mutex> lock(engines_mutex_);
    
    std::string engine_name = engine->get_engine_name();
    
    if (engines_.find(engine_name) != engines_.end()) {
        LOG_WARN("Script engine '{}' already registered, replacing", engine_name);
    }
    
    engines_[engine_name] = std::move(engine);
    LOG_INFO("Registered script engine: {}", engine_name);
}

ScriptEngine* ScriptRegistry::get_engine(const std::string& engine_name) {
    std::shared_lock<std::shared_mutex> lock(engines_mutex_);
    
    auto it = engines_.find(engine_name);
    return it != engines_.end() ? it->second.get() : nullptr;
}

const ScriptEngine* ScriptRegistry::get_engine(const std::string& engine_name) const {
    std::shared_lock<std::shared_mutex> lock(engines_mutex_);
    
    auto it = engines_.find(engine_name);
    return it != engines_.end() ? it->second.get() : nullptr;
}

ScriptResult<void> ScriptRegistry::load_script_auto(const std::string& filepath) {
    std::string language = detect_script_language(filepath);
    
    if (language.empty()) {
        ScriptError error{
            ScriptError::Type::InvalidArgument,
            "Cannot detect script language from file extension: " + filepath,
            filepath,
            0, 0,
            "",
            "Supported extensions: .lua, .py"
        };
        return ScriptResult<void>::error_result(error);
    }
    
    ScriptEngine* engine = get_engine(language);
    if (!engine) {
        ScriptError error{
            ScriptError::Type::CompilationError,
            "No engine registered for language: " + language,
            filepath,
            0, 0,
            "",
            "Make sure the appropriate script engine is registered"
        };
        return ScriptResult<void>::error_result(error);
    }
    
    std::string script_name = std::filesystem::path(filepath).stem().string();
    return engine->load_script_file(script_name, filepath);
}

std::vector<std::string> ScriptRegistry::get_all_engines() const {
    std::shared_lock<std::shared_mutex> lock(engines_mutex_);
    
    std::vector<std::string> engine_names;
    engine_names.reserve(engines_.size());
    
    for (const auto& [name, engine] : engines_) {
        engine_names.push_back(name);
    }
    
    return engine_names;
}

std::vector<std::string> ScriptRegistry::get_all_scripts() const {
    std::shared_lock<std::shared_mutex> lock(engines_mutex_);
    
    std::vector<std::string> all_scripts;
    
    for (const auto& [name, engine] : engines_) {
        auto scripts = engine->get_loaded_scripts();
        for (const auto& script : scripts) {
            all_scripts.push_back(name + "::" + script);
        }
    }
    
    return all_scripts;
}

std::string ScriptRegistry::generate_comparative_report() const {
    std::shared_lock<std::shared_mutex> lock(engines_mutex_);
    
    std::ostringstream oss;
    oss << "=== Script Registry Comparative Performance Report ===\n";
    oss << "Registered Engines: " << engines_.size() << "\n\n";
    
    for (const auto& [name, engine] : engines_) {
        oss << engine->generate_performance_report() << "\n";
    }
    
    return oss.str();
}

void ScriptRegistry::benchmark_all_engines(const std::string& operation_name,
                                         std::function<void()> native_implementation,
                                         usize iterations) {
    std::shared_lock<std::shared_mutex> lock(engines_mutex_);
    
    LOG_INFO("Benchmarking all script engines for operation: {}", operation_name);
    
    for (const auto& [name, engine] : engines_) {
        auto scripts = engine->get_loaded_scripts();
        for (const auto& script : scripts) {
            engine->benchmark_against_native(script, operation_name, native_implementation, iterations);
        }
    }
}

std::string ScriptRegistry::explain_engine_differences() const {
    return R"(
Script Engine Characteristics:

Lua Engine:
- Strengths: Lightweight, fast, easy integration, excellent for game logic
- Weaknesses: Limited standard library, single-threaded execution
- Best for: Configuration, game scripts, embedded logic
- Performance: Generally fastest scripting language
- Memory: Minimal footprint, efficient garbage collection

Python Engine:
- Strengths: Rich standard library, extensive ecosystem, excellent for AI/ML
- Weaknesses: Slower execution, GIL limitations for threading
- Best for: Data processing, AI/ML algorithms, complex business logic
- Performance: Slower than Lua but more feature-rich
- Memory: Higher memory usage due to rich object model

General Guidelines:
- Use Lua for performance-critical scripts and simple logic
- Use Python for complex algorithms and data manipulation
- Consider JIT compilation for performance-sensitive applications
- Monitor memory usage and execution times for optimization
)";
}

std::vector<std::string> ScriptRegistry::recommend_engine_for_usecase(const std::string& usecase) const {
    std::vector<std::string> recommendations;
    
    std::string usecase_lower = usecase;
    std::transform(usecase_lower.begin(), usecase_lower.end(), usecase_lower.begin(), ::tolower);
    
    if (usecase_lower.find("game") != std::string::npos ||
        usecase_lower.find("performance") != std::string::npos ||
        usecase_lower.find("embedded") != std::string::npos) {
        recommendations.push_back("Lua - Lightweight and fast for game logic and embedded scripts");
    }
    
    if (usecase_lower.find("ai") != std::string::npos ||
        usecase_lower.find("ml") != std::string::npos ||
        usecase_lower.find("data") != std::string::npos ||
        usecase_lower.find("complex") != std::string::npos) {
        recommendations.push_back("Python - Rich ecosystem for AI/ML and complex data processing");
    }
    
    if (usecase_lower.find("config") != std::string::npos ||
        usecase_lower.find("settings") != std::string::npos) {
        recommendations.push_back("Lua - Simple and efficient for configuration scripts");
    }
    
    if (recommendations.empty()) {
        recommendations.push_back("Lua - General-purpose recommendation for balanced performance and ease of use");
        recommendations.push_back("Python - Alternative for feature-rich scripting needs");
    }
    
    return recommendations;
}

void ScriptRegistry::shutdown_all() {
    std::unique_lock<std::shared_mutex> lock(engines_mutex_);
    
    LOG_INFO("Shutting down all script engines");
    
    for (auto& [name, engine] : engines_) {
        if (engine->is_initialized()) {
            engine->shutdown();
        }
    }
    
    engines_.clear();
}

std::string ScriptRegistry::detect_script_language(const std::string& filepath) const {
    std::filesystem::path path(filepath);
    std::string extension = path.extension().string();
    
    if (extension == ".lua") {
        return "Lua";
    } else if (extension == ".py") {
        return "Python";
    }
    
    return "";
}

} // namespace ecscope::scripting