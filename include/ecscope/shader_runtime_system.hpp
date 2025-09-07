#pragma once

/**
 * @file shader_runtime_system.hpp
 * @brief Advanced Shader Runtime with Hot-Reload and Caching for ECScope
 * 
 * This system provides comprehensive shader runtime capabilities including:
 * - Real-time hot-reloading of shaders during development
 * - Intelligent binary caching for fast startup times
 * - Automatic shader variant generation and management
 * - Performance monitoring and profiling
 * - Memory-efficient shader resource management
 * - Cross-platform shader compatibility
 * - Educational debugging and visualization tools
 * 
 * @author ECScope Educational ECS Framework - Advanced Shader System
 * @date 2024
 */

#include "advanced_shader_compiler.hpp"
#include "visual_shader_editor.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <queue>

namespace ecscope::renderer::shader_runtime {

//=============================================================================
// Shader Resource Management
//=============================================================================

enum class ShaderState : u8 {
    Unloaded = 0,       ///< Shader not loaded
    Loading,            ///< Currently loading
    Compiling,          ///< Currently compiling
    Ready,              ///< Ready for use
    Error,              ///< Compilation error
    Reloading,          ///< Currently reloading
    Deprecated          ///< Marked for removal
};

struct ShaderMetadata {
    std::string name;
    std::string file_path;
    std::string description;
    std::string author;
    std::string version{"1.0"};
    std::vector<std::string> tags;
    
    // Compilation info
    shader_compiler::CompilationTarget target{shader_compiler::CompilationTarget::OpenGL_4_5};
    shader_compiler::OptimizationLevel optimization{shader_compiler::OptimizationLevel::Development};
    std::vector<std::string> defines;
    std::vector<std::string> include_paths;
    
    // Runtime info
    u64 file_timestamp{0};
    u64 last_compile_time{0};
    usize memory_usage{0};
    f32 avg_compile_time{0.0f};
    u32 usage_count{0};
    
    // Educational metadata
    std::string difficulty_level{"Beginner"}; // "Beginner", "Intermediate", "Advanced"
    std::string learning_objective;
    std::vector<std::string> prerequisites;
    bool is_educational{false};
};

//=============================================================================
// Shader Variant System
//=============================================================================

struct ShaderVariant {
    std::string name;
    std::vector<std::string> defines;
    std::unordered_map<std::string, std::string> specializations;
    shader_compiler::CompilationResult compilation_result;
    bool is_compiled{false};
    u32 usage_frequency{0};
    
    // Performance tracking
    f32 compilation_time{0.0f};
    f32 last_gpu_time{0.0f};
    f32 avg_gpu_time{0.0f};
    u32 draw_call_count{0};
    
    std::string get_cache_key() const {
        std::string key = name;
        for (const auto& define : defines) {
            key += "_" + define;
        }
        for (const auto& [spec_name, spec_value] : specializations) {
            key += "_" + spec_name + "=" + spec_value;
        }
        return key;
    }
};

//=============================================================================
// Hot-Reload System
//=============================================================================

class ShaderFileWatcher {
public:
    using ChangeCallback = std::function<void(const std::string& file_path, bool is_dependency)>;
    
    explicit ShaderFileWatcher(ChangeCallback callback);
    ~ShaderFileWatcher();
    
    void add_file(const std::string& file_path);
    void remove_file(const std::string& file_path);
    void add_directory(const std::string& dir_path, bool recursive = true);
    void remove_directory(const std::string& dir_path);
    
    void set_poll_interval(std::chrono::milliseconds interval) { poll_interval_ = interval; }
    void enable_watching(bool enabled) { watching_enabled_ = enabled; }
    bool is_watching() const { return watching_enabled_; }
    
    // Get list of watched files
    std::vector<std::string> get_watched_files() const;
    std::vector<std::string> get_watched_directories() const;
    
    // Statistics
    u32 get_change_count() const { return change_count_; }
    void reset_change_count() { change_count_ = 0; }
    
private:
    struct FileInfo {
        std::filesystem::file_time_type last_write_time;
        usize file_size;
        bool is_directory;
    };
    
    ChangeCallback callback_;
    std::unordered_map<std::string, FileInfo> watched_files_;
    std::vector<std::string> watched_directories_;
    
    std::thread watcher_thread_;
    std::atomic<bool> should_stop_{false};
    std::atomic<bool> watching_enabled_{true};
    std::chrono::milliseconds poll_interval_{500};
    std::atomic<u32> change_count_{0};
    
    mutable std::mutex files_mutex_;
    
    void watch_loop();
    void check_file_changes();
    void scan_directory(const std::string& dir_path, bool recursive);
    bool has_shader_extension(const std::string& file_path) const;
};

//=============================================================================
// Shader Cache System
//=============================================================================

class ShaderBinaryCache {
public:
    struct CacheEntry {
        std::string cache_key;
        std::vector<u8> binary_data;
        shader_compiler::CompilationResult::ReflectionData reflection;
        u64 creation_time;
        u64 last_access_time;
        u32 access_count;
        usize binary_size;
        std::string source_hash;
        
        bool is_expired(u64 max_age_seconds) const {
            auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            return (now - creation_time) > static_cast<i64>(max_age_seconds);
        }
    };
    
    explicit ShaderBinaryCache(const std::string& cache_directory);
    ~ShaderBinaryCache();
    
    // Cache operations
    bool store_shader(const std::string& cache_key, const std::vector<u8>& binary_data,
                     const shader_compiler::CompilationResult::ReflectionData& reflection,
                     const std::string& source_hash);
    
    std::optional<CacheEntry> load_shader(const std::string& cache_key);
    bool has_shader(const std::string& cache_key) const;
    void remove_shader(const std::string& cache_key);
    
    // Cache management
    void clear_cache();
    void cleanup_expired_entries(u64 max_age_seconds = 7 * 24 * 3600); // 7 days default
    void compact_cache();
    
    // Statistics and information
    struct CacheStatistics {
        usize total_entries{0};
        usize total_size_bytes{0};
        u32 cache_hits{0};
        u32 cache_misses{0};
        f32 hit_ratio{0.0f};
        u64 oldest_entry_age{0};
        u64 newest_entry_age{0};
        std::string cache_directory;
    };
    
    CacheStatistics get_statistics() const;
    void reset_statistics();
    
    // Configuration
    void set_max_cache_size(usize max_size_bytes) { max_cache_size_ = max_size_bytes; }
    void set_max_entries(u32 max_entries) { max_entries_ = max_entries; }
    void enable_compression(bool enabled) { compression_enabled_ = enabled; }
    
private:
    std::string cache_directory_;
    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, CacheEntry> memory_cache_;
    
    // Configuration
    usize max_cache_size_{100 * 1024 * 1024}; // 100MB default
    u32 max_entries_{1000};
    bool compression_enabled_{true};
    
    // Statistics
    mutable CacheStatistics stats_;
    
    std::string get_cache_file_path(const std::string& cache_key) const;
    bool save_entry_to_disk(const CacheEntry& entry) const;
    std::optional<CacheEntry> load_entry_from_disk(const std::string& cache_key) const;
    void update_access_time(const std::string& cache_key) const;
    void enforce_cache_limits();
    
    std::vector<u8> compress_data(const std::vector<u8>& data) const;
    std::vector<u8> decompress_data(const std::vector<u8>& compressed_data) const;
    std::string calculate_hash(const std::vector<u8>& data) const;
};

//=============================================================================
// Advanced Shader Runtime Manager
//=============================================================================

class ShaderRuntimeManager {
public:
    struct RuntimeConfig {
        // Hot-reload settings
        bool enable_hot_reload{true};
        std::chrono::milliseconds hot_reload_check_interval{500};
        bool auto_recompile_on_change{true};
        bool reload_dependencies{true};
        
        // Caching settings
        bool enable_binary_cache{true};
        std::string cache_directory{"shader_cache"};
        usize max_cache_size{100 * 1024 * 1024}; // 100MB
        u32 max_cache_entries{1000};
        bool cache_compression{true};
        
        // Performance settings
        u32 max_concurrent_compilations{4};
        bool enable_background_compilation{true};
        bool precompile_variants{false};
        bool enable_shader_profiling{false};
        
        // Development settings
        bool enable_shader_debugging{false};
        bool log_compilation_times{true};
        bool collect_usage_statistics{true};
        bool educational_mode{false};
        
        // Memory management
        u32 shader_lru_cache_size{50};
        bool unload_unused_shaders{false};
        std::chrono::seconds unused_shader_timeout{300}; // 5 minutes
    };
    
    explicit ShaderRuntimeManager(shader_compiler::AdvancedShaderCompiler* compiler,
                                 const RuntimeConfig& config = RuntimeConfig{});
    ~ShaderRuntimeManager();
    
    // Shader loading and management
    using ShaderHandle = u32;
    static constexpr ShaderHandle INVALID_SHADER_HANDLE = 0;
    
    ShaderHandle load_shader(const std::string& file_path, const std::string& name = "");
    ShaderHandle create_shader(const std::string& source, resources::ShaderStage stage,
                              const std::string& name, const ShaderMetadata& metadata = {});
    
    bool reload_shader(ShaderHandle handle);
    void unload_shader(ShaderHandle handle);
    void unload_all_shaders();
    
    // Shader access
    bool is_shader_ready(ShaderHandle handle) const;
    ShaderState get_shader_state(ShaderHandle handle) const;
    const ShaderMetadata* get_shader_metadata(ShaderHandle handle) const;
    
    // Variant management
    ShaderHandle create_shader_variant(ShaderHandle base_shader, const std::string& variant_name,
                                      const std::vector<std::string>& defines,
                                      const std::unordered_map<std::string, std::string>& specializations = {});
    
    std::vector<ShaderHandle> get_shader_variants(ShaderHandle base_shader) const;
    ShaderHandle find_best_variant(ShaderHandle base_shader, 
                                  const std::vector<std::string>& required_features) const;
    
    // Compilation management
    void compile_shader_async(ShaderHandle handle,
                             std::function<void(ShaderHandle, const shader_compiler::CompilationResult&)> callback = nullptr);
    
    void precompile_all_shaders();
    void precompile_common_variants();
    
    // Hot-reload management
    void enable_hot_reload(bool enabled);
    void force_reload_all();
    void add_shader_dependency(ShaderHandle shader, const std::string& dependency_path);
    
    // Performance monitoring
    struct ShaderPerformanceData {
        f32 last_compile_time{0.0f};
        f32 avg_compile_time{0.0f};
        f32 last_gpu_time{0.0f};
        f32 avg_gpu_time{0.0f};
        u32 draw_call_count{0};
        u32 usage_count{0};
        usize memory_usage{0};
        
        f32 performance_score{100.0f}; // 0-100, higher is better
        std::vector<std::string> performance_warnings;
    };
    
    ShaderPerformanceData get_shader_performance(ShaderHandle handle) const;
    void record_gpu_time(ShaderHandle handle, f32 gpu_time_ms);
    void record_draw_call(ShaderHandle handle);
    
    // Runtime statistics
    struct RuntimeStatistics {
        u32 total_shaders{0};
        u32 loaded_shaders{0};
        u32 compiled_shaders{0};
        u32 error_shaders{0};
        u32 variants_created{0};
        
        u32 hot_reloads_performed{0};
        u32 cache_hits{0};
        u32 cache_misses{0};
        f32 cache_hit_ratio{0.0f};
        
        f32 total_compile_time{0.0f};
        f32 avg_compile_time{0.0f};
        usize total_memory_usage{0};
        
        u32 background_compilations{0};
        u32 failed_compilations{0};
        
        std::chrono::steady_clock::time_point start_time;
        f32 uptime_seconds{0.0f};
    };
    
    RuntimeStatistics get_runtime_statistics() const;
    void reset_statistics();
    
    // Educational and debugging features
    struct ShaderDebugInfo {
        std::string original_source;
        std::string preprocessed_source;
        std::string compiled_assembly;
        std::vector<shader_compiler::CompilationDiagnostic> diagnostics;
        shader_compiler::CompilationResult::ReflectionData reflection;
        std::vector<std::string> optimization_suggestions;
        std::string performance_analysis;
    };
    
    std::optional<ShaderDebugInfo> get_shader_debug_info(ShaderHandle handle) const;
    std::string generate_shader_report(ShaderHandle handle) const;
    std::vector<std::string> get_shader_optimization_hints(ShaderHandle handle) const;
    
    // Shader library management
    void register_shader_library(const std::string& library_path);
    std::vector<std::string> get_available_shader_templates() const;
    ShaderHandle create_from_template(const std::string& template_name, const std::string& name);
    
    // System integration
    void update(); // Call once per frame
    void handle_context_lost();
    void handle_context_restored();
    
    // Configuration management
    void set_config(const RuntimeConfig& config);
    const RuntimeConfig& get_config() const { return config_; }
    
    // Visual shader editor integration
    void register_visual_editor(visual_editor::VisualShaderEditor* editor) {
        visual_editor_ = editor;
    }
    
    ShaderHandle create_from_visual_graph(const visual_editor::VisualShaderGraph& graph,
                                         const std::string& name);
    
private:
    //=============================================================================
    // Internal Data Structures
    //=============================================================================
    
    struct ShaderEntry {
        ShaderHandle handle;
        std::string name;
        std::string file_path;
        ShaderMetadata metadata;
        ShaderState state{ShaderState::Unloaded};
        
        // Source and compilation
        std::string source_code;
        shader_compiler::CompilationResult compilation_result;
        std::vector<ShaderVariant> variants;
        
        // Dependencies and hot-reload
        std::vector<std::string> dependencies;
        u64 last_file_check_time{0};
        bool needs_recompilation{false};
        
        // Performance tracking
        ShaderPerformanceData performance;
        std::chrono::steady_clock::time_point last_use_time;
        
        // Usage tracking
        u32 reference_count{0};
        bool is_system_shader{false};
        bool is_template{false};
        
        ShaderEntry() = default;
        ShaderEntry(ShaderHandle h, std::string n) : handle(h), name(std::move(n)) {}
    };
    
    //=============================================================================
    // Core Systems
    //=============================================================================
    
    shader_compiler::AdvancedShaderCompiler* compiler_;
    std::unique_ptr<ShaderFileWatcher> file_watcher_;
    std::unique_ptr<ShaderBinaryCache> binary_cache_;
    visual_editor::VisualShaderEditor* visual_editor_{nullptr};
    
    //=============================================================================
    // Configuration and State
    //=============================================================================
    
    RuntimeConfig config_;
    std::unordered_map<ShaderHandle, std::unique_ptr<ShaderEntry>> shaders_;
    std::unordered_map<std::string, ShaderHandle> name_to_handle_;
    std::unordered_map<std::string, ShaderHandle> path_to_handle_;
    
    ShaderHandle next_handle_{1};
    mutable std::mutex shaders_mutex_;
    
    //=============================================================================
    // Background Processing
    //=============================================================================
    
    struct CompilationTask {
        ShaderHandle handle;
        std::string source;
        resources::ShaderStage stage;
        std::function<void(ShaderHandle, const shader_compiler::CompilationResult&)> callback;
        std::chrono::steady_clock::time_point submit_time;
    };
    
    std::queue<CompilationTask> compilation_queue_;
    std::vector<std::thread> compilation_threads_;
    std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    std::atomic<bool> shutdown_requested_{false};
    
    //=============================================================================
    // Statistics and Monitoring
    //=============================================================================
    
    mutable RuntimeStatistics stats_;
    std::chrono::steady_clock::time_point system_start_time_;
    mutable std::mutex stats_mutex_;
    
    //=============================================================================
    // Internal Methods
    //=============================================================================
    
    // Shader management
    ShaderHandle generate_handle() { return next_handle_++; }
    ShaderEntry* get_shader_entry(ShaderHandle handle);
    const ShaderEntry* get_shader_entry(ShaderHandle handle) const;
    
    // File operations
    std::string load_file_content(const std::string& file_path) const;
    std::vector<std::string> resolve_shader_dependencies(const std::string& source) const;
    bool is_file_newer(const std::string& file_path, u64 reference_time) const;
    
    // Hot-reload callbacks
    void on_file_changed(const std::string& file_path, bool is_dependency);
    void check_shader_dependencies();
    
    // Compilation management
    void compilation_worker_thread();
    void submit_compilation_task(const CompilationTask& task);
    void process_compilation_result(ShaderHandle handle, const shader_compiler::CompilationResult& result);
    
    // Cache management
    std::string generate_cache_key(const ShaderEntry& entry, const ShaderVariant* variant = nullptr) const;
    bool try_load_from_cache(ShaderEntry& entry, ShaderVariant* variant = nullptr);
    void save_to_cache(const ShaderEntry& entry, const ShaderVariant* variant = nullptr);
    
    // Performance tracking
    void update_shader_performance(ShaderEntry& entry, const shader_compiler::CompilationResult& result);
    void update_usage_statistics(ShaderHandle handle);
    
    // Memory management
    void cleanup_unused_shaders();
    void enforce_memory_limits();
    
    // Educational features
    void generate_learning_materials(const ShaderEntry& entry);
    std::vector<std::string> analyze_shader_complexity(const ShaderEntry& entry) const;
    
    // System maintenance
    void update_statistics();
    void perform_housekeeping();
};

//=============================================================================
// Shader Resource Pool for Efficient Management
//=============================================================================

template<typename ResourceType>
class ShaderResourcePool {
public:
    using ResourceHandle = u32;
    static constexpr ResourceHandle INVALID_HANDLE = 0;
    
    explicit ShaderResourcePool(usize initial_capacity = 100)
        : next_handle_(1), resources_(initial_capacity) {
        
        // Initialize free list
        for (usize i = 0; i < initial_capacity; ++i) {
            free_indices_.push(i);
        }
    }
    
    ResourceHandle acquire(ResourceType&& resource) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (free_indices_.empty()) {
            // Expand pool
            usize old_size = resources_.size();
            usize new_size = old_size * 2;
            resources_.resize(new_size);
            
            for (usize i = old_size; i < new_size; ++i) {
                free_indices_.push(i);
            }
        }
        
        usize index = free_indices_.front();
        free_indices_.pop();
        
        ResourceHandle handle = next_handle_++;
        resources_[index] = {handle, std::move(resource), true};
        handle_to_index_[handle] = index;
        
        return handle;
    }
    
    void release(ResourceHandle handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = handle_to_index_.find(handle);
        if (it != handle_to_index_.end()) {
            usize index = it->second;
            resources_[index].is_active = false;
            free_indices_.push(index);
            handle_to_index_.erase(it);
        }
    }
    
    ResourceType* get(ResourceHandle handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = handle_to_index_.find(handle);
        if (it != handle_to_index_.end()) {
            usize index = it->second;
            if (resources_[index].is_active && resources_[index].handle == handle) {
                return &resources_[index].resource;
            }
        }
        return nullptr;
    }
    
    const ResourceType* get(ResourceHandle handle) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = handle_to_index_.find(handle);
        if (it != handle_to_index_.end()) {
            usize index = it->second;
            if (resources_[index].is_active && resources_[index].handle == handle) {
                return &resources_[index].resource;
            }
        }
        return nullptr;
    }
    
    usize get_active_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return handle_to_index_.size();
    }
    
    usize get_capacity() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return resources_.size();
    }
    
private:
    struct PoolEntry {
        ResourceHandle handle;
        ResourceType resource;
        bool is_active;
        
        PoolEntry() : handle(INVALID_HANDLE), is_active(false) {}
        PoolEntry(ResourceHandle h, ResourceType&& r, bool active)
            : handle(h), resource(std::move(r)), is_active(active) {}
    };
    
    mutable std::mutex mutex_;
    ResourceHandle next_handle_;
    std::vector<PoolEntry> resources_;
    std::queue<usize> free_indices_;
    std::unordered_map<ResourceHandle, usize> handle_to_index_;
};

//=============================================================================
// Utility Functions and Helpers
//=============================================================================

namespace utils {
    // Shader file utilities
    bool is_shader_file(const std::string& file_path);
    std::string get_shader_extension(resources::ShaderStage stage);
    resources::ShaderStage detect_shader_stage_from_path(const std::string& file_path);
    
    // Performance utilities
    std::string format_shader_performance(const ShaderRuntimeManager::ShaderPerformanceData& data);
    f32 calculate_shader_complexity_score(const std::string& source);
    std::vector<std::string> suggest_shader_optimizations(const std::string& source);
    
    // Educational utilities
    std::string generate_shader_explanation(const std::string& source, resources::ShaderStage stage);
    std::vector<std::string> extract_learning_concepts(const std::string& source);
    std::string get_difficulty_assessment(const std::string& source);
    
    // System utilities
    std::string format_memory_size(usize bytes);
    std::string format_duration(std::chrono::milliseconds duration);
    bool ensure_directory_exists(const std::string& path);
    
    // Configuration helpers
    ShaderRuntimeManager::RuntimeConfig create_development_config();
    ShaderRuntimeManager::RuntimeConfig create_production_config();
    ShaderRuntimeManager::RuntimeConfig create_educational_config();
}

} // namespace ecscope::renderer::shader_runtime