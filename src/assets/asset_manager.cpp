// Asset Manager Implementation
#include "ecscope/assets/asset_manager.hpp"
#include <filesystem>
#include <fstream>

namespace ecscope::assets {

AssetManager::AssetManager() = default;
AssetManager::~AssetManager() = default;

bool AssetManager::initialize(const std::string& assets_directory) {
    assets_directory_ = assets_directory;
    if (!std::filesystem::exists(assets_directory_)) {
        std::filesystem::create_directories(assets_directory_);
    }
    return true;
}

void AssetManager::shutdown() {
    assets_.clear();
}

std::shared_ptr<Asset> AssetManager::load_asset(const std::string& path, AssetType type) {
    auto it = assets_.find(path);
    if (it != assets_.end()) {
        return it->second;
    }
    
    auto asset = std::make_shared<Asset>();
    asset->path = path;
    asset->type = type;
    asset->state = AssetState::Loading;
    
    // Simulate loading
    std::filesystem::path full_path = assets_directory_ / path;
    if (std::filesystem::exists(full_path)) {
        asset->state = AssetState::Loaded;
    } else {
        asset->state = AssetState::Failed;
    }
    
    assets_[path] = asset;
    return asset;
}

void AssetManager::unload_asset(const std::string& path) {
    assets_.erase(path);
}

std::shared_ptr<Asset> AssetManager::get_asset(const std::string& path) {
    auto it = assets_.find(path);
    return (it != assets_.end()) ? it->second : nullptr;
}

bool AssetManager::is_loaded(const std::string& path) const {
    auto it = assets_.find(path);
    return it != assets_.end() && it->second->state == AssetState::Loaded;
}

void AssetManager::update() {
    // Process async loading tasks
}

} // namespace ecscope::assets