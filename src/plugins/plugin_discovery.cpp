#include "ecscope/plugins/plugin_loader.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <sstream>

// Simple JSON parsing for plugin manifests
#include <map>
#include <string>

namespace ecscope::plugins {

    PluginDiscovery::PluginDiscovery(PluginLoader* loader) : loader_(loader) {
        if (!loader_) {
            throw std::invalid_argument("PluginLoader cannot be null");
        }
    }
    
    std::vector<PluginDiscovery::PluginCandidate> PluginDiscovery::discover_plugins(const std::string& directory) {
        std::vector<PluginCandidate> candidates;
        
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
            return candidates;
        }
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file() && is_plugin_file(entry.path().string())) {
                    PluginCandidate candidate = analyze_plugin_file(entry.path().string());
                    if (candidate.valid || !filter_ || filter_(candidate)) {
                        candidates.push_back(std::move(candidate));
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error discovering plugins in " << directory << ": " << e.what() << std::endl;
        }
        
        // Also look for manifest files
        std::vector<std::string> manifest_files = find_manifest_files(directory);
        for (const std::string& manifest_path : manifest_files) {
            PluginCandidate candidate;
            candidate.path = manifest_path;
            candidate.name = std::filesystem::path(manifest_path).stem().string();
            
            if (load_manifest(manifest_path, candidate.metadata)) {
                candidate.valid = true;
                if (!filter_ || filter_(candidate)) {
                    candidates.push_back(std::move(candidate));
                }
            } else {
                candidate.valid = false;
                candidate.error_message = "Failed to parse manifest file";
                if (!filter_ || filter_(candidate)) {
                    candidates.push_back(std::move(candidate));
                }
            }
        }
        
        return candidates;
    }
    
    std::vector<PluginDiscovery::PluginCandidate> PluginDiscovery::discover_plugins_recursive(const std::string& directory) {
        std::vector<PluginCandidate> candidates;
        
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
            return candidates;
        }
        
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    if (is_plugin_file(entry.path().string())) {
                        PluginCandidate candidate = analyze_plugin_file(entry.path().string());
                        if (candidate.valid || !filter_ || filter_(candidate)) {
                            candidates.push_back(std::move(candidate));
                        }
                    } else if (entry.path().filename() == get_default_manifest_name()) {
                        PluginCandidate candidate;
                        candidate.path = entry.path().string();
                        candidate.name = entry.path().parent_path().filename().string();
                        
                        if (load_manifest(entry.path().string(), candidate.metadata)) {
                            candidate.valid = true;
                            if (!filter_ || filter_(candidate)) {
                                candidates.push_back(std::move(candidate));
                            }
                        } else {
                            candidate.valid = false;
                            candidate.error_message = "Failed to parse manifest file";
                            if (!filter_ || filter_(candidate)) {
                                candidates.push_back(std::move(candidate));
                            }
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error discovering plugins recursively in " << directory << ": " << e.what() << std::endl;
        }
        
        return candidates;
    }
    
    void PluginDiscovery::scan_for_plugins() {
        available_plugins_.clear();
        
        for (const std::string& directory : plugin_directories_) {
            std::vector<PluginCandidate> candidates = discover_plugins_recursive(directory);
            available_plugins_.insert(available_plugins_.end(), 
                                    std::make_move_iterator(candidates.begin()),
                                    std::make_move_iterator(candidates.end()));
        }
        
        // Remove duplicates based on plugin name
        std::sort(available_plugins_.begin(), available_plugins_.end(),
                 [](const PluginCandidate& a, const PluginCandidate& b) {
                     return a.name < b.name;
                 });
        
        available_plugins_.erase(
            std::unique(available_plugins_.begin(), available_plugins_.end(),
                       [](const PluginCandidate& a, const PluginCandidate& b) {
                           return a.name == b.name;
                       }),
            available_plugins_.end()
        );
    }
    
    void PluginDiscovery::add_plugin_directory(const std::string& directory) {
        if (std::find(plugin_directories_.begin(), plugin_directories_.end(), directory) == plugin_directories_.end()) {
            plugin_directories_.push_back(directory);
        }
    }
    
    void PluginDiscovery::remove_plugin_directory(const std::string& directory) {
        plugin_directories_.erase(
            std::remove(plugin_directories_.begin(), plugin_directories_.end(), directory),
            plugin_directories_.end()
        );
    }
    
    std::vector<std::string> PluginDiscovery::get_plugin_directories() const {
        return plugin_directories_;
    }
    
    std::vector<PluginDiscovery::PluginCandidate> PluginDiscovery::get_available_plugins() const {
        if (filter_) {
            std::vector<PluginCandidate> filtered;
            std::copy_if(available_plugins_.begin(), available_plugins_.end(),
                        std::back_inserter(filtered), filter_);
            return filtered;
        }
        return available_plugins_;
    }
    
    PluginDiscovery::PluginCandidate PluginDiscovery::find_plugin(const std::string& name) const {
        for (const PluginCandidate& candidate : available_plugins_) {
            if (candidate.name == name) {
                return candidate;
            }
        }
        
        PluginCandidate empty;
        empty.valid = false;
        empty.error_message = "Plugin not found: " + name;
        return empty;
    }
    
    std::vector<PluginDiscovery::PluginCandidate> PluginDiscovery::find_plugins_by_tag(const std::string& tag) const {
        std::vector<PluginCandidate> matching;
        
        for (const PluginCandidate& candidate : available_plugins_) {
            if (candidate.valid) {
                const auto& tags = candidate.metadata.tags;
                if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
                    matching.push_back(candidate);
                }
            }
        }
        
        return matching;
    }
    
    bool PluginDiscovery::load_manifest(const std::string& manifest_path, PluginMetadata& metadata) {
        if (!file_exists(manifest_path)) {
            return false;
        }
        
        std::string content = read_file(manifest_path);
        if (content.empty()) {
            return false;
        }
        
        return parse_json_manifest(content, metadata);
    }
    
    bool PluginDiscovery::save_manifest(const std::string& manifest_path, const PluginMetadata& metadata) {
        std::string content = serialize_json_manifest(metadata);
        return write_file(manifest_path, content);
    }
    
    void PluginDiscovery::set_filter(std::function<bool(const PluginCandidate&)> filter) {
        filter_ = std::move(filter);
    }
    
    void PluginDiscovery::clear_filter() {
        filter_ = nullptr;
    }
    
    void PluginDiscovery::sort_by_priority() {
        std::sort(available_plugins_.begin(), available_plugins_.end(),
                 [](const PluginCandidate& a, const PluginCandidate& b) {
                     // Sort by tags containing "critical", "high", etc.
                     auto has_tag = [](const std::vector<std::string>& tags, const std::string& tag) {
                         return std::find(tags.begin(), tags.end(), tag) != tags.end();
                     };
                     
                     int priority_a = 1000, priority_b = 1000;
                     if (has_tag(a.metadata.tags, "critical")) priority_a = 0;
                     else if (has_tag(a.metadata.tags, "high")) priority_a = 100;
                     else if (has_tag(a.metadata.tags, "normal")) priority_a = 500;
                     else if (has_tag(a.metadata.tags, "low")) priority_a = 1000;
                     
                     if (has_tag(b.metadata.tags, "critical")) priority_b = 0;
                     else if (has_tag(b.metadata.tags, "high")) priority_b = 100;
                     else if (has_tag(b.metadata.tags, "normal")) priority_b = 500;
                     else if (has_tag(b.metadata.tags, "low")) priority_b = 1000;
                     
                     return priority_a < priority_b;
                 });
    }
    
    void PluginDiscovery::sort_by_name() {
        std::sort(available_plugins_.begin(), available_plugins_.end(),
                 [](const PluginCandidate& a, const PluginCandidate& b) {
                     return a.name < b.name;
                 });
    }
    
    void PluginDiscovery::sort_by_version() {
        std::sort(available_plugins_.begin(), available_plugins_.end(),
                 [](const PluginCandidate& a, const PluginCandidate& b) {
                     return a.metadata.version > b.metadata.version; // Newest first
                 });
    }
    
    // Private helper methods
    
    bool PluginDiscovery::is_plugin_file(const std::string& path) {
        std::filesystem::path file_path(path);
        std::string extension = file_path.extension().string();
        
        #ifdef _WIN32
            return extension == ".dll";
        #elif defined(__APPLE__)
            return extension == ".dylib";
        #else
            return extension == ".so";
        #endif
    }
    
    PluginDiscovery::PluginCandidate PluginDiscovery::analyze_plugin_file(const std::string& path) {
        PluginCandidate candidate;
        candidate.path = path;
        candidate.name = std::filesystem::path(path).stem().string();
        
        // Try to load the plugin to get its metadata
        LoadInfo load_info = loader_->load_library(path);
        if (load_info.is_success()) {
            candidate.metadata = load_info.metadata;
            candidate.valid = true;
            
            // Unload the plugin after getting metadata
            loader_->unload_library(path);
        } else {
            candidate.valid = false;
            candidate.error_message = load_info.error_message;
        }
        
        // Look for accompanying manifest file
        std::string manifest_path = std::filesystem::path(path).parent_path() / get_default_manifest_name();
        if (file_exists(manifest_path)) {
            PluginMetadata manifest_metadata;
            if (load_manifest(manifest_path, manifest_metadata)) {
                // Merge manifest data with library metadata
                if (candidate.metadata.description.empty() && !manifest_metadata.description.empty()) {
                    candidate.metadata.description = manifest_metadata.description;
                }
                if (candidate.metadata.author.empty() && !manifest_metadata.author.empty()) {
                    candidate.metadata.author = manifest_metadata.author;
                }
                if (candidate.metadata.website.empty() && !manifest_metadata.website.empty()) {
                    candidate.metadata.website = manifest_metadata.website;
                }
                // Merge tags
                for (const std::string& tag : manifest_metadata.tags) {
                    if (std::find(candidate.metadata.tags.begin(), candidate.metadata.tags.end(), tag) == candidate.metadata.tags.end()) {
                        candidate.metadata.tags.push_back(tag);
                    }
                }
            }
        }
        
        return candidate;
    }
    
    std::vector<std::string> PluginDiscovery::find_manifest_files(const std::string& directory) {
        std::vector<std::string> manifests;
        std::string manifest_name = get_default_manifest_name();
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().filename() == manifest_name) {
                    manifests.push_back(entry.path().string());
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            // Ignore errors
        }
        
        return manifests;
    }
    
    bool PluginDiscovery::parse_json_manifest(const std::string& content, PluginMetadata& metadata) {
        // Simple JSON parser for plugin manifests
        // This is a basic implementation - in production, use a proper JSON library
        
        try {
            // Remove whitespace and parse basic JSON structure
            std::string trimmed = content;
            trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), [](char c) {
                return c == '\n' || c == '\r' || c == '\t';
            }), trimmed.end());
            
            if (trimmed.front() != '{' || trimmed.back() != '}') {
                return false;
            }
            
            // Extract key-value pairs
            std::map<std::string, std::string> values;
            size_t pos = 1; // Skip opening brace
            
            while (pos < trimmed.length() - 1) {
                // Find key
                size_t key_start = trimmed.find('"', pos);
                if (key_start == std::string::npos) break;
                key_start++;
                
                size_t key_end = trimmed.find('"', key_start);
                if (key_end == std::string::npos) break;
                
                std::string key = trimmed.substr(key_start, key_end - key_start);
                
                // Find colon
                size_t colon_pos = trimmed.find(':', key_end);
                if (colon_pos == std::string::npos) break;
                
                // Find value
                size_t value_start = colon_pos + 1;
                while (value_start < trimmed.length() && (trimmed[value_start] == ' ' || trimmed[value_start] == '\t')) {
                    value_start++;
                }
                
                std::string value;
                if (trimmed[value_start] == '"') {
                    // String value
                    value_start++;
                    size_t value_end = trimmed.find('"', value_start);
                    if (value_end == std::string::npos) break;
                    value = trimmed.substr(value_start, value_end - value_start);
                    pos = value_end + 1;
                } else {
                    // Number or boolean value
                    size_t value_end = trimmed.find_first_of(",}", value_start);
                    if (value_end == std::string::npos) break;
                    value = trimmed.substr(value_start, value_end - value_start);
                    pos = value_end;
                }
                
                values[key] = value;
                
                // Skip comma
                if (pos < trimmed.length() && trimmed[pos] == ',') {
                    pos++;
                }
            }
            
            // Fill metadata from parsed values
            if (values.count("name")) metadata.name = values["name"];
            if (values.count("display_name")) metadata.display_name = values["display_name"];
            if (values.count("description")) metadata.description = values["description"];
            if (values.count("author")) metadata.author = values["author"];
            if (values.count("website")) metadata.website = values["website"];
            if (values.count("license")) metadata.license = values["license"];
            
            // Parse version
            if (values.count("version")) {
                std::string version_str = values["version"];
                std::istringstream iss(version_str);
                std::string part;
                if (std::getline(iss, part, '.')) metadata.version.major = std::stoul(part);
                if (std::getline(iss, part, '.')) metadata.version.minor = std::stoul(part);
                if (std::getline(iss, part, '-')) metadata.version.patch = std::stoul(part);
                if (std::getline(iss, part)) metadata.version.pre_release = part;
            }
            
            // Parse memory limit
            if (values.count("memory_limit")) {
                metadata.memory_limit = std::stoull(values["memory_limit"]);
            }
            
            // Parse CPU time limit
            if (values.count("cpu_time_limit")) {
                metadata.cpu_time_limit = std::stoul(values["cpu_time_limit"]);
            }
            
            // Parse sandbox requirement
            if (values.count("sandbox_required")) {
                metadata.sandbox_required = (values["sandbox_required"] == "true");
            }
            
            return true;
            
        } catch (const std::exception&) {
            return false;
        }
    }
    
    std::string PluginDiscovery::serialize_json_manifest(const PluginMetadata& metadata) {
        std::ostringstream oss;
        oss << "{\n";
        oss << "  \"name\": \"" << metadata.name << "\",\n";
        oss << "  \"display_name\": \"" << metadata.display_name << "\",\n";
        oss << "  \"description\": \"" << metadata.description << "\",\n";
        oss << "  \"author\": \"" << metadata.author << "\",\n";
        oss << "  \"website\": \"" << metadata.website << "\",\n";
        oss << "  \"version\": \"" << metadata.version.to_string() << "\",\n";
        oss << "  \"license\": \"" << metadata.license << "\",\n";
        oss << "  \"memory_limit\": " << metadata.memory_limit << ",\n";
        oss << "  \"cpu_time_limit\": " << metadata.cpu_time_limit << ",\n";
        oss << "  \"sandbox_required\": " << (metadata.sandbox_required ? "true" : "false") << "\n";
        oss << "}";
        return oss.str();
    }
    
    std::vector<std::string> PluginDiscovery::list_files(const std::string& directory, const std::string& extension) {
        std::vector<std::string> files;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string file_path = entry.path().string();
                    if (extension.empty() || file_path.ends_with(extension)) {
                        files.push_back(file_path);
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            // Ignore errors
        }
        
        return files;
    }
    
    bool PluginDiscovery::is_directory(const std::string& path) {
        return std::filesystem::is_directory(path);
    }
    
    bool PluginDiscovery::file_exists(const std::string& path) {
        return std::filesystem::exists(path);
    }
    
    std::string PluginDiscovery::read_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "";
        }
        
        std::ostringstream oss;
        oss << file.rdbuf();
        return oss.str();
    }
    
    bool PluginDiscovery::write_file(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        file << content;
        return file.good();
    }

} // namespace ecscope::plugins