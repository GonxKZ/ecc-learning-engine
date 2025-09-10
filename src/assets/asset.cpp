#include "ecscope/assets/asset.hpp"
#include <algorithm>

namespace ecscope::assets {

    // Asset implementation
    Asset::Asset(AssetID id, AssetType type, const std::string& path)
        : m_id(id)
        , m_type(type)
        , m_path(path)
        , m_state(AssetState::UNLOADED)
        , m_ref_count(0)
        , m_version(1)
        , m_size_bytes(0)
        , m_last_modified(std::chrono::system_clock::now())
    {
        m_metadata.id = id;
        m_metadata.type = type;
        m_metadata.path = path;
        m_metadata.version = m_version;
        m_metadata.last_modified = m_last_modified;
    }

    // AssetHandle implementation
    AssetHandle::AssetHandle(std::shared_ptr<Asset> asset) : m_asset(std::move(asset)) {
        if (m_asset) {
            m_asset->add_reference();
        }
    }

    AssetHandle::AssetHandle(const AssetHandle& other) : m_asset(other.m_asset) {
        if (m_asset) {
            m_asset->add_reference();
        }
    }

    AssetHandle::AssetHandle(AssetHandle&& other) noexcept : m_asset(std::move(other.m_asset)) {
        other.m_asset = nullptr;
    }

    AssetHandle::~AssetHandle() {
        if (m_asset) {
            m_asset->remove_reference();
        }
    }

    AssetHandle& AssetHandle::operator=(const AssetHandle& other) {
        if (this != &other) {
            if (m_asset) {
                m_asset->remove_reference();
            }
            
            m_asset = other.m_asset;
            
            if (m_asset) {
                m_asset->add_reference();
            }
        }
        return *this;
    }

    AssetHandle& AssetHandle::operator=(AssetHandle&& other) noexcept {
        if (this != &other) {
            if (m_asset) {
                m_asset->remove_reference();
            }
            
            m_asset = std::move(other.m_asset);
            other.m_asset = nullptr;
        }
        return *this;
    }

    void AssetHandle::reset() {
        if (m_asset) {
            m_asset->remove_reference();
            m_asset = nullptr;
        }
    }

    bool AssetHandle::operator==(const AssetHandle& other) const {
        return m_asset == other.m_asset;
    }

} // namespace ecscope::assets