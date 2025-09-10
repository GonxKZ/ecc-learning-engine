#include "ecscope/assets/asset_types.hpp"
#include <random>
#include <functional>
#include <sstream>
#include <iomanip>

namespace ecscope::assets {

    // Asset type to string conversion
    const char* asset_type_to_string(AssetType type) {
        switch (type) {
            case AssetType::TEXTURE: return "TEXTURE";
            case AssetType::MESH: return "MESH";
            case AssetType::MATERIAL: return "MATERIAL";
            case AssetType::SHADER: return "SHADER";
            case AssetType::AUDIO: return "AUDIO";
            case AssetType::ANIMATION: return "ANIMATION";
            case AssetType::FONT: return "FONT";
            case AssetType::SCENE: return "SCENE";
            case AssetType::SCRIPT: return "SCRIPT";
            case AssetType::CONFIG: return "CONFIG";
            case AssetType::BINARY: return "BINARY";
            case AssetType::UNKNOWN:
            default: return "UNKNOWN";
        }
    }

    // String to asset type conversion
    AssetType string_to_asset_type(const std::string& str) {
        if (str == "TEXTURE") return AssetType::TEXTURE;
        if (str == "MESH") return AssetType::MESH;
        if (str == "MATERIAL") return AssetType::MATERIAL;
        if (str == "SHADER") return AssetType::SHADER;
        if (str == "AUDIO") return AssetType::AUDIO;
        if (str == "ANIMATION") return AssetType::ANIMATION;
        if (str == "FONT") return AssetType::FONT;
        if (str == "SCENE") return AssetType::SCENE;
        if (str == "SCRIPT") return AssetType::SCRIPT;
        if (str == "CONFIG") return AssetType::CONFIG;
        if (str == "BINARY") return AssetType::BINARY;
        return AssetType::UNKNOWN;
    }

    // Asset state to string conversion
    const char* asset_state_to_string(AssetState state) {
        switch (state) {
            case AssetState::UNLOADED: return "UNLOADED";
            case AssetState::QUEUED: return "QUEUED";
            case AssetState::LOADING: return "LOADING";
            case AssetState::LOADED: return "LOADED";
            case AssetState::ERROR: return "ERROR";
            case AssetState::STALE: return "STALE";
            case AssetState::STREAMING: return "STREAMING";
            default: return "UNKNOWN";
        }
    }

    // Asset ID generation
    AssetID generate_asset_id() {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<AssetID> dis(1, UINT64_MAX);
        
        AssetID id;
        do {
            id = dis(gen);
        } while (id == INVALID_ASSET_ID);
        
        return id;
    }

    // Path to asset ID conversion (deterministic hash)
    AssetID path_to_asset_id(const std::string& path) {
        if (path.empty()) {
            return INVALID_ASSET_ID;
        }
        
        // Use FNV-1a hash for deterministic ID generation
        constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ull;
        constexpr uint64_t FNV_PRIME = 1099511628211ull;
        
        uint64_t hash = FNV_OFFSET_BASIS;
        for (char c : path) {
            hash ^= static_cast<uint64_t>(c);
            hash *= FNV_PRIME;
        }
        
        // Ensure we don't generate INVALID_ASSET_ID
        if (hash == INVALID_ASSET_ID) {
            hash = 1;
        }
        
        return hash;
    }

} // namespace ecscope::assets