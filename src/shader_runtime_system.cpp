#include "../include/ecscope/shader_runtime_system.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <regex>
#include <filesystem>
#include <functional>

// For binary cache compression
#ifdef ECSCOPE_USE_ZLIB
#include <zlib.h>
#endif

namespace ecscope::renderer::shader_runtime {

//=============================================================================
// Shader File Watcher Implementation
//=============================================================================

ShaderFileWatcher::ShaderFileWatcher(ChangeCallback callback)
    : callback_(std::move(callback)), watcher_thread_(&ShaderFileWatcher::watch_loop, this) {
}

ShaderFileWatcher::~ShaderFileWatcher() {
    should_stop_ = true;
    if (watcher_thread_.joinable()) {
        watcher_thread_.join();
    }
}

void ShaderFileWatcher::add_file(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(files_mutex_);
    
    if (std::filesystem::exists(file_path)) {
        FileInfo info;
        info.last_write_time = std::filesystem::last_write_time(file_path);
        info.file_size = std::filesystem::file_size(file_path);
        info.is_directory = false;
        
        watched_files_[file_path] = info;
    }
}

void ShaderFileWatcher::remove_file(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(files_mutex_);
    watched_files_.erase(file_path);
}

void ShaderFileWatcher::add_directory(const std::string& dir_path, bool recursive) {
    std::lock_guard<std::mutex> lock(files_mutex_);
    
    if (std::filesystem::exists(dir_path) && std::filesystem::is_directory(dir_path)) {
        watched_directories_.push_back(dir_path);
        scan_directory(dir_path, recursive);
    }
}

void ShaderFileWatcher::remove_directory(const std::string& dir_path) {
    std::lock_guard<std::mutex> lock(files_mutex_);
    
    auto it = std::find(watched_directories_.begin(), watched_directories_.end(), dir_path);
    if (it != watched_directories_.end()) {
        watched_directories_.erase(it);
    }
    
    // Remove all files from this directory
    auto file_it = watched_files_.begin();
    while (file_it != watched_files_.end()) {
        if (file_it->first.find(dir_path) == 0) {
            file_it = watched_files_.erase(file_it);
        } else {
            ++file_it;
        }
    }
}

std::vector<std::string> ShaderFileWatcher::get_watched_files() const {
    std::lock_guard<std::mutex> lock(files_mutex_);
    
    std::vector<std::string> files;
    files.reserve(watched_files_.size());
    for (const auto& [path, info] : watched_files_) {
        files.push_back(path);
    }
    return files;
}

std::vector<std::string> ShaderFileWatcher::get_watched_directories() const {
    std::lock_guard<std::mutex> lock(files_mutex_);
    return watched_directories_;
}

void ShaderFileWatcher::watch_loop() {
    while (!should_stop_) {
        if (watching_enabled_) {
            check_file_changes();
        }
        
        std::this_thread::sleep_for(poll_interval_);
    }
}

void ShaderFileWatcher::check_file_changes() {
    std::lock_guard<std::mutex> lock(files_mutex_);
    
    for (auto& [file_path, info] : watched_files_) {
        try {
            if (std::filesystem::exists(file_path)) {
                auto current_time = std::filesystem::last_write_time(file_path);
                auto current_size = std::filesystem::file_size(file_path);
                
                if (current_time != info.last_write_time || current_size != info.file_size) {
                    info.last_write_time = current_time;
                    info.file_size = current_size;
                    
                    ++change_count_;
                    
                    // Call callback without holding the lock
                    files_mutex_.unlock();
                    callback_(file_path, false);
                    files_mutex_.lock();
                }
            } else {
                // File was deleted
                ++change_count_;
                files_mutex_.unlock();
                callback_(file_path, false);
                files_mutex_.lock();
            }
        } catch (const std::filesystem::filesystem_error&) {
            // Ignore filesystem errors (file might be temporarily inaccessible)
        }
    }
    
    // Check directories for new files
    for (const std::string& dir_path : watched_directories_) {
        try {
            if (std::filesystem::exists(dir_path)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
                    if (entry.is_regular_file() && has_shader_extension(entry.path().string())) {
                        std::string file_path = entry.path().string();
                        
                        if (watched_files_.find(file_path) == watched_files_.end()) {
                            // New file detected
                            FileInfo info;
                            info.last_write_time = entry.last_write_time();
                            info.file_size = entry.file_size();
                            info.is_directory = false;
                            
                            watched_files_[file_path] = info;
                            
                            ++change_count_;
                            files_mutex_.unlock();
                            callback_(file_path, false);
                            files_mutex_.lock();
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            // Ignore filesystem errors
        }
    }
}

void ShaderFileWatcher::scan_directory(const std::string& dir_path, bool recursive) {
    try {
        auto iterator = recursive ? 
            std::filesystem::recursive_directory_iterator(dir_path) :
            std::filesystem::directory_iterator(dir_path);
        
        for (const auto& entry : iterator) {
            if (entry.is_regular_file() && has_shader_extension(entry.path().string())) {
                FileInfo info;
                info.last_write_time = entry.last_write_time();
                info.file_size = entry.file_size();
                info.is_directory = false;
                
                watched_files_[entry.path().string()] = info;
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // Ignore filesystem errors
    }
}

bool ShaderFileWatcher::has_shader_extension(const std::string& file_path) const {
    std::filesystem::path path(file_path);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".geom" ||
           ext == ".comp" || ext == ".hlsl" || ext == ".fx" || ext == ".shader" ||
           ext == ".vs" || ext == ".ps" || ext == ".gs" || ext == ".cs";
}

//=============================================================================
// Shader Binary Cache Implementation
//=============================================================================

ShaderBinaryCache::ShaderBinaryCache(const std::string& cache_directory)
    : cache_directory_(cache_directory) {
    
    // Create cache directory if it doesn't exist
    std::filesystem::create_directories(cache_directory_);
    
    // Load existing cache entries from disk
    try {
        for (const auto& entry : std::filesystem::directory_iterator(cache_directory_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".cache") {
                std::string cache_key = entry.path().stem().string();
                auto cache_entry = load_entry_from_disk(cache_key);
                if (cache_entry) {
                    memory_cache_[cache_key] = std::move(*cache_entry);
                }
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // Ignore errors when loading cache
    }
    
    stats_.cache_directory = cache_directory_;
    stats_.total_entries = memory_cache_.size();
}

ShaderBinaryCache::~ShaderBinaryCache() {
    // Save all memory cache entries to disk
    for (const auto& [key, entry] : memory_cache_) {
        save_entry_to_disk(entry);
    }
}

bool ShaderBinaryCache::store_shader(const std::string& cache_key, const std::vector<u8>& binary_data,
                                    const shader_compiler::CompilationResult::ReflectionData& reflection,
                                    const std::string& source_hash) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    CacheEntry entry;
    entry.cache_key = cache_key;
    entry.binary_data = binary_data;
    entry.reflection = reflection;
    entry.source_hash = source_hash;
    entry.binary_size = binary_data.size();
    
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.creation_time = now;
    entry.last_access_time = now;
    entry.access_count = 1;
    
    // Compress binary data if enabled
    if (compression_enabled_) {
        entry.binary_data = compress_data(binary_data);
    }
    
    memory_cache_[cache_key] = entry;
    
    // Save to disk
    bool disk_save_success = save_entry_to_disk(entry);
    
    // Update statistics
    stats_.total_entries = memory_cache_.size();
    stats_.total_size_bytes += entry.binary_size;
    
    enforce_cache_limits();
    
    return disk_save_success;
}

std::optional<ShaderBinaryCache::CacheEntry> ShaderBinaryCache::load_shader(const std::string& cache_key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = memory_cache_.find(cache_key);
    if (it != memory_cache_.end()) {
        update_access_time(cache_key);
        stats_.cache_hits++;
        
        CacheEntry entry = it->second;
        
        // Decompress if needed
        if (compression_enabled_ && entry.binary_data.size() < entry.binary_size) {
            entry.binary_data = decompress_data(entry.binary_data);
        }
        
        return entry;
    }
    
    // Try loading from disk
    auto disk_entry = load_entry_from_disk(cache_key);
    if (disk_entry) {
        memory_cache_[cache_key] = *disk_entry;
        update_access_time(cache_key);
        stats_.cache_hits++;
        return *disk_entry;
    }
    
    stats_.cache_misses++;
    return std::nullopt;
}

bool ShaderBinaryCache::has_shader(const std::string& cache_key) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (memory_cache_.find(cache_key) != memory_cache_.end()) {
        return true;
    }
    
    // Check disk
    return std::filesystem::exists(get_cache_file_path(cache_key));
}

void ShaderBinaryCache::remove_shader(const std::string& cache_key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = memory_cache_.find(cache_key);
    if (it != memory_cache_.end()) {
        stats_.total_size_bytes -= it->second.binary_size;
        memory_cache_.erase(it);
    }
    
    // Remove from disk
    std::filesystem::remove(get_cache_file_path(cache_key));
    
    stats_.total_entries = memory_cache_.size();
}

void ShaderBinaryCache::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    memory_cache_.clear();
    
    // Remove all cache files from disk
    try {
        for (const auto& entry : std::filesystem::directory_iterator(cache_directory_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".cache") {
                std::filesystem::remove(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // Ignore errors
    }
    
    stats_.total_entries = 0;
    stats_.total_size_bytes = 0;
}

void ShaderBinaryCache::cleanup_expired_entries(u64 max_age_seconds) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = memory_cache_.begin();
    while (it != memory_cache_.end()) {
        if (it->second.is_expired(max_age_seconds)) {
            stats_.total_size_bytes -= it->second.binary_size;
            
            // Remove from disk
            std::filesystem::remove(get_cache_file_path(it->first));
            
            it = memory_cache_.erase(it);
        } else {
            ++it;
        }
    }
    
    stats_.total_entries = memory_cache_.size();
}

ShaderBinaryCache::CacheStatistics ShaderBinaryCache::get_statistics() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto stats = stats_;
    
    if (stats.cache_hits + stats.cache_misses > 0) {
        stats.hit_ratio = static_cast<f32>(stats.cache_hits) / 
                         (stats.cache_hits + stats.cache_misses);
    }
    
    // Calculate age statistics
    if (!memory_cache_.empty()) {
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        u64 oldest_age = 0, newest_age = UINT64_MAX;
        for (const auto& [key, entry] : memory_cache_) {
            u64 age = now - entry.creation_time;
            oldest_age = std::max(oldest_age, age);
            newest_age = std::min(newest_age, age);
        }
        
        stats.oldest_entry_age = oldest_age;
        stats.newest_entry_age = newest_age;
    }
    
    return stats;
}

void ShaderBinaryCache::reset_statistics() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    stats_.cache_hits = 0;
    stats_.cache_misses = 0;
}

std::string ShaderBinaryCache::get_cache_file_path(const std::string& cache_key) const {
    return cache_directory_ + "/" + cache_key + ".cache";
}

bool ShaderBinaryCache::save_entry_to_disk(const CacheEntry& entry) const {
    try {
        std::ofstream file(get_cache_file_path(entry.cache_key), std::ios::binary);
        if (!file.is_open()) return false;
        
        // Write header
        file.write(reinterpret_cast<const char*>(&entry.creation_time), sizeof(entry.creation_time));
        file.write(reinterpret_cast<const char*>(&entry.last_access_time), sizeof(entry.last_access_time));
        file.write(reinterpret_cast<const char*>(&entry.access_count), sizeof(entry.access_count));
        file.write(reinterpret_cast<const char*>(&entry.binary_size), sizeof(entry.binary_size));
        
        // Write source hash
        u32 hash_size = static_cast<u32>(entry.source_hash.size());
        file.write(reinterpret_cast<const char*>(&hash_size), sizeof(hash_size));
        file.write(entry.source_hash.data(), hash_size);
        
        // Write binary data
        u32 data_size = static_cast<u32>(entry.binary_data.size());
        file.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
        file.write(reinterpret_cast<const char*>(entry.binary_data.data()), data_size);
        
        // Write reflection data (simplified)
        // In a full implementation, you'd serialize the reflection data properly
        
        file.close();
        return true;
        
    } catch (const std::exception&) {
        return false;
    }
}

std::optional<ShaderBinaryCache::CacheEntry> ShaderBinaryCache::load_entry_from_disk(const std::string& cache_key) const {
    try {
        std::ifstream file(get_cache_file_path(cache_key), std::ios::binary);
        if (!file.is_open()) return std::nullopt;
        
        CacheEntry entry;
        entry.cache_key = cache_key;
        
        // Read header
        file.read(reinterpret_cast<char*>(&entry.creation_time), sizeof(entry.creation_time));
        file.read(reinterpret_cast<char*>(&entry.last_access_time), sizeof(entry.last_access_time));
        file.read(reinterpret_cast<char*>(&entry.access_count), sizeof(entry.access_count));
        file.read(reinterpret_cast<char*>(&entry.binary_size), sizeof(entry.binary_size));
        
        // Read source hash
        u32 hash_size;
        file.read(reinterpret_cast<char*>(&hash_size), sizeof(hash_size));
        entry.source_hash.resize(hash_size);
        file.read(entry.source_hash.data(), hash_size);
        
        // Read binary data
        u32 data_size;
        file.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));
        entry.binary_data.resize(data_size);
        file.read(reinterpret_cast<char*>(entry.binary_data.data()), data_size);
        
        // Read reflection data (simplified)
        
        file.close();
        return entry;
        
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

void ShaderBinaryCache::update_access_time(const std::string& cache_key) const {
    auto it = memory_cache_.find(cache_key);
    if (it != memory_cache_.end()) {
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        const_cast<CacheEntry&>(it->second).last_access_time = now;
        const_cast<CacheEntry&>(it->second).access_count++;
    }
}

void ShaderBinaryCache::enforce_cache_limits() {
    // Remove least recently used entries if over limits
    if (memory_cache_.size() > max_entries_ || stats_.total_size_bytes > max_cache_size_) {
        // Create sorted list by access time
        std::vector<std::pair<std::string, u64>> entries_by_access;
        for (const auto& [key, entry] : memory_cache_) {
            entries_by_access.emplace_back(key, entry.last_access_time);
        }
        
        std::sort(entries_by_access.begin(), entries_by_access.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });
        
        // Remove oldest entries
        usize entries_to_remove = 0;
        if (memory_cache_.size() > max_entries_) {
            entries_to_remove = memory_cache_.size() - max_entries_;
        }
        
        // Also remove entries to free up space
        usize current_size = stats_.total_size_bytes;
        for (const auto& [key, access_time] : entries_by_access) {
            if (entries_to_remove == 0 && current_size <= max_cache_size_) break;
            
            auto it = memory_cache_.find(key);
            if (it != memory_cache_.end()) {
                current_size -= it->second.binary_size;
                std::filesystem::remove(get_cache_file_path(key));
                memory_cache_.erase(it);
                
                if (entries_to_remove > 0) entries_to_remove--;
            }
        }
        
        stats_.total_entries = memory_cache_.size();
        stats_.total_size_bytes = current_size;
    }
}

std::vector<u8> ShaderBinaryCache::compress_data(const std::vector<u8>& data) const {
    #ifdef ECSCOPE_USE_ZLIB
    // Use zlib compression
    uLongf compressed_size = compressBound(data.size());
    std::vector<u8> compressed_data(compressed_size);
    
    int result = compress(compressed_data.data(), &compressed_size,
                         data.data(), data.size());
    
    if (result == Z_OK) {
        compressed_data.resize(compressed_size);
        return compressed_data;
    }
    #endif
    
    // Return original data if compression fails or is not available
    return data;
}

std::vector<u8> ShaderBinaryCache::decompress_data(const std::vector<u8>& compressed_data) const {
    #ifdef ECSCOPE_USE_ZLIB
    // Use zlib decompression - this is simplified
    // In practice, you'd need to store the original size
    uLongf decompressed_size = compressed_data.size() * 4; // Estimate
    std::vector<u8> decompressed_data(decompressed_size);
    
    int result = uncompress(decompressed_data.data(), &decompressed_size,
                           compressed_data.data(), compressed_data.size());
    
    if (result == Z_OK) {
        decompressed_data.resize(decompressed_size);
        return decompressed_data;
    }
    #endif
    
    return compressed_data;
}

std::string ShaderBinaryCache::calculate_hash(const std::vector<u8>& data) const {
    // Simple hash calculation - in practice you'd use SHA256 or similar
    std::hash<std::string> hasher;
    std::string data_str(data.begin(), data.end());
    return std::to_string(hasher(data_str));
}

//=============================================================================
// Shader Runtime Manager Implementation
//=============================================================================

ShaderRuntimeManager::ShaderRuntimeManager(shader_compiler::AdvancedShaderCompiler* compiler,
                                          const RuntimeConfig& config)
    : compiler_(compiler), config_(config), system_start_time_(std::chrono::steady_clock::now()) {
    
    // Initialize binary cache if enabled
    if (config_.enable_binary_cache) {
        binary_cache_ = std::make_unique<ShaderBinaryCache>(config_.cache_directory);
        binary_cache_->set_max_cache_size(config_.max_cache_size);
        binary_cache_->set_max_entries(config_.max_cache_entries);
        binary_cache_->enable_compression(config_.cache_compression);
    }
    
    // Initialize file watcher if hot-reload is enabled
    if (config_.enable_hot_reload) {
        file_watcher_ = std::make_unique<ShaderFileWatcher>(
            [this](const std::string& path, bool is_dep) { on_file_changed(path, is_dep); }
        );
        file_watcher_->set_poll_interval(config_.hot_reload_check_interval);
    }
    
    // Start background compilation threads
    if (config_.enable_background_compilation) {
        for (u32 i = 0; i < config_.max_concurrent_compilations; ++i) {
            compilation_threads_.emplace_back(&ShaderRuntimeManager::compilation_worker_thread, this);
        }
    }
}

ShaderRuntimeManager::~ShaderRuntimeManager() {
    // Stop background threads
    shutdown_requested_ = true;
    queue_condition_.notify_all();
    
    for (auto& thread : compilation_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

ShaderRuntimeManager::ShaderHandle ShaderRuntimeManager::load_shader(const std::string& file_path, 
                                                                     const std::string& name) {
    std::string shader_name = name.empty() ? std::filesystem::path(file_path).stem().string() : name;
    
    // Check if already loaded
    {
        std::lock_guard<std::mutex> lock(shaders_mutex_);
        auto it = path_to_handle_.find(file_path);
        if (it != path_to_handle_.end()) {
            return it->second;
        }
    }
    
    // Load file content
    std::string source = load_file_content(file_path);
    if (source.empty()) {
        return INVALID_SHADER_HANDLE;
    }
    
    // Detect shader stage from file extension
    auto stage = utils::detect_shader_stage_from_path(file_path);
    
    // Create shader metadata
    ShaderMetadata metadata;
    metadata.name = shader_name;
    metadata.file_path = file_path;
    metadata.file_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::filesystem::last_write_time(file_path).time_since_epoch()).count();
    
    // Create shader entry
    ShaderHandle handle = create_shader(source, stage, shader_name, metadata);
    
    if (handle != INVALID_SHADER_HANDLE) {
        std::lock_guard<std::mutex> lock(shaders_mutex_);
        path_to_handle_[file_path] = handle;
        
        // Add to file watcher
        if (file_watcher_) {
            file_watcher_->add_file(file_path);
        }
        
        // Resolve and watch dependencies
        if (config_.enable_hot_reload) {
            auto* entry = get_shader_entry(handle);
            if (entry) {
                entry->dependencies = resolve_shader_dependencies(source);
                for (const auto& dep_path : entry->dependencies) {
                    add_shader_dependency(handle, dep_path);
                }
            }
        }
    }
    
    return handle;
}

ShaderRuntimeManager::ShaderHandle ShaderRuntimeManager::create_shader(const std::string& source,
                                                                       resources::ShaderStage stage,
                                                                       const std::string& name,
                                                                       const ShaderMetadata& metadata) {
    ShaderHandle handle = generate_handle();
    
    auto entry = std::make_unique<ShaderEntry>(handle, name);
    entry->source_code = source;
    entry->metadata = metadata;
    entry->state = ShaderState::Loading;
    
    // Try to load from cache first
    bool loaded_from_cache = false;
    if (config_.enable_binary_cache && binary_cache_) {
        loaded_from_cache = try_load_from_cache(*entry);
    }
    
    if (!loaded_from_cache) {
        // Submit for compilation
        if (config_.enable_background_compilation) {
            CompilationTask task;
            task.handle = handle;
            task.source = source;
            task.stage = stage;
            task.submit_time = std::chrono::steady_clock::now();
            submit_compilation_task(task);
        } else {
            // Compile synchronously
            auto result = compiler_->compile_shader(source, stage, "main", metadata.file_path);
            process_compilation_result(handle, result);
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(shaders_mutex_);
        shaders_[handle] = std::move(entry);
        name_to_handle_[name] = handle;
    }
    
    return handle;
}

bool ShaderRuntimeManager::reload_shader(ShaderHandle handle) {
    auto* entry = get_shader_entry(handle);
    if (!entry) return false;
    
    entry->state = ShaderState::Reloading;
    
    // Reload from file if available
    if (!entry->metadata.file_path.empty()) {
        std::string new_source = load_file_content(entry->metadata.file_path);
        if (!new_source.empty()) {
            entry->source_code = new_source;
            entry->metadata.file_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                std::filesystem::last_write_time(entry->metadata.file_path).time_since_epoch()).count();
        }
    }
    
    // Recompile
    auto stage = utils::detect_shader_stage_from_path(entry->metadata.file_path);
    
    CompilationTask task;
    task.handle = handle;
    task.source = entry->source_code;
    task.stage = stage;
    task.submit_time = std::chrono::steady_clock::now();
    task.callback = [this](ShaderHandle h, const shader_compiler::CompilationResult& result) {
        auto* entry = get_shader_entry(h);
        if (entry) {
            ++stats_.hot_reloads_performed;
            
            // Update dependencies
            if (config_.reload_dependencies) {
                entry->dependencies = resolve_shader_dependencies(entry->source_code);
            }
        }
    };
    
    submit_compilation_task(task);
    return true;
}

void ShaderRuntimeManager::unload_shader(ShaderHandle handle) {
    std::lock_guard<std::mutex> lock(shaders_mutex_);
    
    auto it = shaders_.find(handle);
    if (it != shaders_.end()) {
        const auto& entry = it->second;
        
        // Remove from lookup tables
        name_to_handle_.erase(entry->name);
        if (!entry->metadata.file_path.empty()) {
            path_to_handle_.erase(entry->metadata.file_path);
            
            // Remove from file watcher
            if (file_watcher_) {
                file_watcher_->remove_file(entry->metadata.file_path);
            }
        }
        
        shaders_.erase(it);
    }
}

bool ShaderRuntimeManager::is_shader_ready(ShaderHandle handle) const {
    const auto* entry = get_shader_entry(handle);
    return entry && entry->state == ShaderState::Ready;
}

ShaderState ShaderRuntimeManager::get_shader_state(ShaderHandle handle) const {
    const auto* entry = get_shader_entry(handle);
    return entry ? entry->state : ShaderState::Unloaded;
}

const ShaderMetadata* ShaderRuntimeManager::get_shader_metadata(ShaderHandle handle) const {
    const auto* entry = get_shader_entry(handle);
    return entry ? &entry->metadata : nullptr;
}

void ShaderRuntimeManager::update() {
    // Update statistics
    update_statistics();
    
    // Perform housekeeping tasks
    perform_housekeeping();
    
    // Clean up unused shaders if enabled
    if (config_.unload_unused_shaders) {
        cleanup_unused_shaders();
    }
}

void ShaderRuntimeManager::on_file_changed(const std::string& file_path, bool is_dependency) {
    if (!config_.auto_recompile_on_change) return;
    
    std::lock_guard<std::mutex> lock(shaders_mutex_);
    
    // Find shaders that use this file
    std::vector<ShaderHandle> shaders_to_reload;
    
    for (const auto& [handle, entry] : shaders_) {
        if (entry->metadata.file_path == file_path) {
            shaders_to_reload.push_back(handle);
        } else if (is_dependency) {
            // Check if this is a dependency of the shader
            auto it = std::find(entry->dependencies.begin(), entry->dependencies.end(), file_path);
            if (it != entry->dependencies.end()) {
                shaders_to_reload.push_back(handle);
            }
        }
    }
    
    // Reload affected shaders
    shaders_mutex_.unlock();
    for (ShaderHandle handle : shaders_to_reload) {
        reload_shader(handle);
    }
    shaders_mutex_.lock();
}

void ShaderRuntimeManager::compilation_worker_thread() {
    while (!shutdown_requested_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_condition_.wait(lock, [this] { return !compilation_queue_.empty() || shutdown_requested_; });
        
        if (shutdown_requested_) break;
        
        CompilationTask task = compilation_queue_.front();
        compilation_queue_.pop();
        lock.unlock();
        
        // Update state
        auto* entry = get_shader_entry(task.handle);
        if (entry) {
            entry->state = ShaderState::Compiling;
        }
        
        // Compile shader
        auto stage = utils::detect_shader_stage_from_path(entry ? entry->metadata.file_path : "");
        auto result = compiler_->compile_shader(task.source, stage, "main", 
                                               entry ? entry->metadata.file_path : "");
        
        // Process result
        process_compilation_result(task.handle, result);
        
        // Call callback if provided
        if (task.callback) {
            task.callback(task.handle, result);
        }
        
        ++stats_.background_compilations;
    }
}

void ShaderRuntimeManager::submit_compilation_task(const CompilationTask& task) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    compilation_queue_.push(task);
    queue_condition_.notify_one();
}

void ShaderRuntimeManager::process_compilation_result(ShaderHandle handle, 
                                                     const shader_compiler::CompilationResult& result) {
    auto* entry = get_shader_entry(handle);
    if (!entry) return;
    
    entry->compilation_result = result;
    
    if (result.success) {
        entry->state = ShaderState::Ready;
        update_shader_performance(*entry, result);
        
        // Save to cache
        if (config_.enable_binary_cache && binary_cache_) {
            save_to_cache(*entry);
        }
        
        ++stats_.compiled_shaders;
    } else {
        entry->state = ShaderState::Error;
        ++stats_.failed_compilations;
    }
    
    ++stats_.total_shaders;
}

std::string ShaderRuntimeManager::load_file_content(const std::string& file_path) const {
    std::ifstream file(file_path);
    if (!file.is_open()) return "";
    
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

std::vector<std::string> ShaderRuntimeManager::resolve_shader_dependencies(const std::string& source) const {
    std::vector<std::string> dependencies;
    
    // Simple regex to find #include directives
    std::regex include_regex(R"(#include\s*[<"](.*)[">])");
    std::sregex_iterator iter(source.begin(), source.end(), include_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::smatch match = *iter;
        dependencies.push_back(match[1].str());
    }
    
    return dependencies;
}

ShaderRuntimeManager::ShaderEntry* ShaderRuntimeManager::get_shader_entry(ShaderHandle handle) {
    auto it = shaders_.find(handle);
    return (it != shaders_.end()) ? it->second.get() : nullptr;
}

const ShaderRuntimeManager::ShaderEntry* ShaderRuntimeManager::get_shader_entry(ShaderHandle handle) const {
    auto it = shaders_.find(handle);
    return (it != shaders_.end()) ? it->second.get() : nullptr;
}

bool ShaderRuntimeManager::try_load_from_cache(ShaderEntry& entry, ShaderVariant* variant) {
    if (!binary_cache_) return false;
    
    std::string cache_key = generate_cache_key(entry, variant);
    auto cached_shader = binary_cache_->load_shader(cache_key);
    
    if (cached_shader) {
        entry.compilation_result.success = true;
        entry.compilation_result.bytecode = cached_shader->binary_data;
        entry.compilation_result.reflection = cached_shader->reflection;
        entry.compilation_result.loaded_from_cache = true;
        entry.state = ShaderState::Ready;
        
        ++stats_.cache_hits;
        return true;
    }
    
    ++stats_.cache_misses;
    return false;
}

void ShaderRuntimeManager::save_to_cache(const ShaderEntry& entry, const ShaderVariant* variant) {
    if (!binary_cache_) return;
    
    std::string cache_key = generate_cache_key(entry, variant);
    std::string source_hash = calculate_hash(entry.source_code);
    
    binary_cache_->store_shader(cache_key, entry.compilation_result.bytecode,
                               entry.compilation_result.reflection, source_hash);
}

std::string ShaderRuntimeManager::generate_cache_key(const ShaderEntry& entry, const ShaderVariant* variant) const {
    std::string key = entry.name;
    
    if (variant) {
        key += "_variant_" + variant->name;
        for (const auto& define : variant->defines) {
            key += "_" + define;
        }
    }
    
    // Add config-specific information
    key += "_target_" + std::to_string(static_cast<int>(entry.metadata.target));
    key += "_opt_" + std::to_string(static_cast<int>(entry.metadata.optimization));
    
    return key;
}

std::string ShaderRuntimeManager::calculate_hash(const std::string& source) const {
    std::hash<std::string> hasher;
    return std::to_string(hasher(source));
}

void ShaderRuntimeManager::update_shader_performance(ShaderEntry& entry, 
                                                    const shader_compiler::CompilationResult& result) {
    entry.performance.last_compile_time = result.performance.compilation_time;
    
    // Update average
    if (entry.performance.avg_compile_time == 0.0f) {
        entry.performance.avg_compile_time = result.performance.compilation_time;
    } else {
        entry.performance.avg_compile_time = 
            (entry.performance.avg_compile_time * 0.9f) + (result.performance.compilation_time * 0.1f);
    }
    
    entry.performance.memory_usage = result.bytecode.size();
    
    // Calculate performance score
    f32 compile_score = std::max(0.0f, 100.0f - (entry.performance.avg_compile_time / 10.0f));
    f32 size_score = std::max(0.0f, 100.0f - (entry.performance.memory_usage / 10240.0f));
    entry.performance.performance_score = (compile_score + size_score) / 2.0f;
}

void ShaderRuntimeManager::update_statistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - system_start_time_);
    stats_.uptime_seconds = static_cast<f32>(elapsed.count());
    
    // Count shaders by state
    stats_.total_shaders = shaders_.size();
    stats_.loaded_shaders = 0;
    stats_.compiled_shaders = 0;
    stats_.error_shaders = 0;
    
    usize total_memory = 0;
    for (const auto& [handle, entry] : shaders_) {
        switch (entry->state) {
            case ShaderState::Ready: ++stats_.compiled_shaders; break;
            case ShaderState::Error: ++stats_.error_shaders; break;
            default: break;
        }
        
        total_memory += entry->performance.memory_usage;
    }
    
    stats_.total_memory_usage = total_memory;
    
    // Update cache statistics
    if (binary_cache_) {
        auto cache_stats = binary_cache_->get_statistics();
        stats_.cache_hits = cache_stats.cache_hits;
        stats_.cache_misses = cache_stats.cache_misses;
        stats_.cache_hit_ratio = cache_stats.hit_ratio;
    }
}

void ShaderRuntimeManager::perform_housekeeping() {
    // Clean up expired cache entries
    if (binary_cache_) {
        binary_cache_->cleanup_expired_entries();
    }
    
    // Update shader usage statistics
    auto now = std::chrono::steady_clock::now();
    for (auto& [handle, entry] : shaders_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - entry->last_use_time);
        if (elapsed.count() > config_.unused_shader_timeout.count()) {
            // Mark as potentially unloadable
        }
    }
}

void ShaderRuntimeManager::cleanup_unused_shaders() {
    auto now = std::chrono::steady_clock::now();
    std::vector<ShaderHandle> to_unload;
    
    for (const auto& [handle, entry] : shaders_) {
        if (entry->reference_count == 0 && !entry->is_system_shader) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - entry->last_use_time);
            if (elapsed > config_.unused_shader_timeout) {
                to_unload.push_back(handle);
            }
        }
    }
    
    for (ShaderHandle handle : to_unload) {
        unload_shader(handle);
    }
}

//=============================================================================
// Utility Functions Implementation
//=============================================================================

namespace utils {
    bool is_shader_file(const std::string& file_path) {
        std::filesystem::path path(file_path);
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        return ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".geom" ||
               ext == ".comp" || ext == ".hlsl" || ext == ".fx" || ext == ".shader";
    }
    
    std::string get_shader_extension(resources::ShaderStage stage) {
        switch (stage) {
            case resources::ShaderStage::Vertex: return ".vert";
            case resources::ShaderStage::Fragment: return ".frag";
            case resources::ShaderStage::Geometry: return ".geom";
            case resources::ShaderStage::Compute: return ".comp";
            case resources::ShaderStage::TessControl: return ".tesc";
            case resources::ShaderStage::TessEvaluation: return ".tese";
            default: return ".glsl";
        }
    }
    
    resources::ShaderStage detect_shader_stage_from_path(const std::string& file_path) {
        std::filesystem::path path(file_path);
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".vert" || ext == ".vs") return resources::ShaderStage::Vertex;
        if (ext == ".frag" || ext == ".ps") return resources::ShaderStage::Fragment;
        if (ext == ".geom" || ext == ".gs") return resources::ShaderStage::Geometry;
        if (ext == ".comp" || ext == ".cs") return resources::ShaderStage::Compute;
        if (ext == ".tesc") return resources::ShaderStage::TessControl;
        if (ext == ".tese") return resources::ShaderStage::TessEvaluation;
        
        return resources::ShaderStage::Fragment; // Default
    }
    
    std::string format_memory_size(usize bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unit = 0;
        f32 size = static_cast<f32>(bytes);
        
        while (size >= 1024.0f && unit < 3) {
            size /= 1024.0f;
            unit++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
        return oss.str();
    }
    
    ShaderRuntimeManager::RuntimeConfig create_development_config() {
        ShaderRuntimeManager::RuntimeConfig config;
        config.enable_hot_reload = true;
        config.enable_shader_debugging = true;
        config.log_compilation_times = true;
        config.educational_mode = true;
        config.enable_background_compilation = true;
        return config;
    }
    
    ShaderRuntimeManager::RuntimeConfig create_production_config() {
        ShaderRuntimeManager::RuntimeConfig config;
        config.enable_hot_reload = false;
        config.enable_shader_debugging = false;
        config.log_compilation_times = false;
        config.educational_mode = false;
        config.precompile_variants = true;
        config.unload_unused_shaders = true;
        return config;
    }
}

} // namespace ecscope::renderer::shader_runtime