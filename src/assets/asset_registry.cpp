// Asset Registry Implementation
#include "ecscope/assets/asset_registry.hpp"
#include <filesystem>

namespace ecscope::assets {

AssetRegistry::AssetRegistry() = default;
AssetRegistry::~AssetRegistry() = default;

bool AssetRegistry::initialize(const std::string& root_path) {
    root_path_ = root_path;
    if (!std::filesystem::exists(root_path_)) {
        std::filesystem::create_directories(root_path_);
    }
    return true;
}

void AssetRegistry::shutdown() {
    assets_.clear();
    type_handlers_.clear();
}

AssetHandle AssetRegistry::register_asset(const std::string& path, AssetType type) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    AssetHandle handle = next_handle_++;
    
    AssetInfo info;
    info.handle = handle;
    info.path = path;
    info.type = type;
    info.state = AssetState::Unloaded;
    info.ref_count = 0;
    
    // Calculate full path
    std::filesystem::path full_path = root_path_ / path;
    info.full_path = full_path.string();
    
    // Check if file exists
    if (std::filesystem::exists(full_path)) {
        info.file_size = std::filesystem::file_size(full_path);
        info.last_modified = std::filesystem::last_write_time(full_path);
    }
    
    assets_[handle] = info;
    path_to_handle_[path] = handle;
    
    return handle;
}

void AssetRegistry::unregister_asset(AssetHandle handle) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = assets_.find(handle);
    if (it != assets_.end()) {
        path_to_handle_.erase(it->second.path);
        assets_.erase(it);
    }
}

AssetHandle AssetRegistry::get_handle(const std::string& path) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = path_to_handle_.find(path);
    return (it != path_to_handle_.end()) ? it->second : INVALID_ASSET_HANDLE;
}

const AssetInfo* AssetRegistry::get_asset_info(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = assets_.find(handle);
    return (it != assets_.end()) ? &it->second : nullptr;
}

bool AssetRegistry::is_valid(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    return assets_.find(handle) != assets_.end();
}

void AssetRegistry::set_asset_state(AssetHandle handle, AssetState state) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = assets_.find(handle);
    if (it != assets_.end()) {
        it->second.state = state;
    }
}

AssetState AssetRegistry::get_asset_state(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = assets_.find(handle);
    return (it != assets_.end()) ? it->second.state : AssetState::Invalid;
}

void AssetRegistry::increment_ref_count(AssetHandle handle) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = assets_.find(handle);
    if (it != assets_.end()) {
        it->second.ref_count++;
    }
}

void AssetRegistry::decrement_ref_count(AssetHandle handle) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = assets_.find(handle);
    if (it != assets_.end() && it->second.ref_count > 0) {
        it->second.ref_count--;
    }
}

uint32_t AssetRegistry::get_ref_count(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = assets_.find(handle);
    return (it != assets_.end()) ? it->second.ref_count : 0;
}

void AssetRegistry::register_type_handler(AssetType type, std::unique_ptr<AssetTypeHandler> handler) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    type_handlers_[type] = std::move(handler);
}

AssetTypeHandler* AssetRegistry::get_type_handler(AssetType type) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = type_handlers_.find(type);
    return (it != type_handlers_.end()) ? it->second.get() : nullptr;
}

std::vector<AssetHandle> AssetRegistry::get_assets_by_type(AssetType type) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::vector<AssetHandle> result;
    for (const auto& [handle, info] : assets_) {
        if (info.type == type) {
            result.push_back(handle);
        }
    }
    return result;
}

std::vector<AssetHandle> AssetRegistry::get_assets_by_state(AssetState state) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::vector<AssetHandle> result;
    for (const auto& [handle, info] : assets_) {
        if (info.state == state) {
            result.push_back(handle);
        }
    }
    return result;
}

void AssetRegistry::scan_directory(const std::string& directory, bool recursive) {
    std::filesystem::path dir_path = root_path_ / directory;
    
    if (!std::filesystem::exists(dir_path)) {
        return;
    }
    
    auto scan_impl = [this](const std::filesystem::path& path, bool recurse) -> void {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                // Determine asset type based on extension
                std::string extension = entry.path().extension().string();
                AssetType type = determine_asset_type(extension);
                
                if (type != AssetType::Unknown) {
                    // Get relative path from root
                    std::string relative_path = std::filesystem::relative(entry.path(), root_path_).string();
                    register_asset(relative_path, type);
                }
            } else if (entry.is_directory() && recurse) {
                scan_impl(entry.path(), recurse);
            }
        }
    };
    
    scan_impl(dir_path, recursive);
}

AssetType AssetRegistry::determine_asset_type(const std::string& extension) {
    static std::unordered_map<std::string, AssetType> extension_map = {
        {".png", AssetType::Texture},
        {".jpg", AssetType::Texture},
        {".jpeg", AssetType::Texture},
        {".tga", AssetType::Texture},
        {".bmp", AssetType::Texture},
        {".wav", AssetType::Audio},
        {".mp3", AssetType::Audio},
        {".ogg", AssetType::Audio},
        {".obj", AssetType::Mesh},
        {".fbx", AssetType::Mesh},
        {".gltf", AssetType::Mesh},
        {".glsl", AssetType::Shader},
        {".vert", AssetType::Shader},
        {".frag", AssetType::Shader},
        {".json", AssetType::Data},
        {".xml", AssetType::Data},
        {".yaml", AssetType::Data},
        {".yml", AssetType::Data}
    };
    
    auto it = extension_map.find(extension);
    return (it != extension_map.end()) ? it->second : AssetType::Unknown;
}

size_t AssetRegistry::get_total_assets() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    return assets_.size();
}

size_t AssetRegistry::get_loaded_assets() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    size_t count = 0;
    for (const auto& [handle, info] : assets_) {
        if (info.state == AssetState::Loaded) {
            count++;
        }
    }
    return count;
}

size_t AssetRegistry::get_memory_usage() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    size_t total = 0;
    for (const auto& [handle, info] : assets_) {
        if (info.state == AssetState::Loaded) {
            total += info.file_size;
        }
    }
    return total;
}

} // namespace ecscope::assets