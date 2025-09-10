#pragma once

#include "asset_types.hpp"
#include <memory>
#include <mutex>
#include <atomic>

namespace ecscope::assets {

    // Base asset class
    class Asset {
    public:
        Asset(AssetID id, AssetType type, const std::string& path);
        virtual ~Asset() = default;

        // Core asset interface
        AssetID get_id() const { return m_id; }
        AssetType get_type() const { return m_type; }
        const std::string& get_path() const { return m_path; }
        AssetState get_state() const { return m_state.load(); }
        size_t get_size() const { return m_size_bytes; }
        AssetVersion get_version() const { return m_version; }

        // State management
        void set_state(AssetState state) { m_state.store(state); }
        bool is_loaded() const { return m_state.load() == AssetState::LOADED; }
        bool is_loading() const { return m_state.load() == AssetState::LOADING; }
        bool has_error() const { return m_state.load() == AssetState::ERROR; }

        // Reference counting
        void add_reference() { ++m_ref_count; }
        void remove_reference() { --m_ref_count; }
        uint32_t get_reference_count() const { return m_ref_count.load(); }

        // Asset data access
        virtual void* get_data() = 0;
        virtual const void* get_data() const = 0;
        virtual size_t get_data_size() const = 0;

        // Asset operations
        virtual bool load(const std::vector<uint8_t>& data) = 0;
        virtual bool reload() = 0;
        virtual void unload() = 0;
        virtual std::shared_ptr<Asset> clone() const = 0;

        // Serialization
        virtual bool serialize(std::vector<uint8_t>& data) const = 0;
        virtual bool deserialize(const std::vector<uint8_t>& data) = 0;

        // Memory management
        virtual size_t get_memory_usage() const = 0;
        virtual void compress() {}
        virtual void decompress() {}

        // Hot reload support
        void set_last_modified(std::chrono::system_clock::time_point time) { m_last_modified = time; }
        std::chrono::system_clock::time_point get_last_modified() const { return m_last_modified; }

        // Metadata
        void set_metadata(const AssetMetadata& metadata) { m_metadata = metadata; }
        const AssetMetadata& get_metadata() const { return m_metadata; }

    protected:
        AssetID m_id;
        AssetType m_type;
        std::string m_path;
        std::atomic<AssetState> m_state;
        std::atomic<uint32_t> m_ref_count;
        AssetVersion m_version;
        size_t m_size_bytes;
        std::chrono::system_clock::time_point m_last_modified;
        AssetMetadata m_metadata;
        mutable std::mutex m_mutex;

        void set_size(size_t size) { m_size_bytes = size; }
        void increment_version() { ++m_version; }
    };

    // Asset handle for safe asset access
    class AssetHandle {
    public:
        AssetHandle() = default;
        explicit AssetHandle(std::shared_ptr<Asset> asset);
        AssetHandle(const AssetHandle& other);
        AssetHandle(AssetHandle&& other) noexcept;
        ~AssetHandle();

        AssetHandle& operator=(const AssetHandle& other);
        AssetHandle& operator=(AssetHandle&& other) noexcept;

        // Asset access
        Asset* get() const { return m_asset.get(); }
        Asset* operator->() const { return m_asset.get(); }
        Asset& operator*() const { return *m_asset; }

        // Validity checks
        bool is_valid() const { return m_asset != nullptr; }
        bool is_loaded() const { return m_asset && m_asset->is_loaded(); }
        explicit operator bool() const { return is_valid(); }

        // Asset info
        AssetID get_id() const { return m_asset ? m_asset->get_id() : INVALID_ASSET_ID; }
        AssetType get_type() const { return m_asset ? m_asset->get_type() : AssetType::UNKNOWN; }
        AssetState get_state() const { return m_asset ? m_asset->get_state() : AssetState::UNLOADED; }

        // Reset handle
        void reset();

        // Comparison
        bool operator==(const AssetHandle& other) const;
        bool operator!=(const AssetHandle& other) const { return !(*this == other); }

    private:
        std::shared_ptr<Asset> m_asset;
    };

    // Typed asset handle template
    template<typename T>
    class TypedAssetHandle {
        static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset");

    public:
        TypedAssetHandle() = default;
        explicit TypedAssetHandle(AssetHandle handle) : m_handle(std::move(handle)) {}
        explicit TypedAssetHandle(std::shared_ptr<T> asset) : m_handle(asset) {}

        // Typed access
        T* get() const { 
            return m_handle.is_valid() ? static_cast<T*>(m_handle.get()) : nullptr;
        }
        T* operator->() const { return get(); }
        T& operator*() const { return *get(); }

        // Base handle access
        const AssetHandle& get_handle() const { return m_handle; }
        AssetHandle& get_handle() { return m_handle; }

        // Validity and state
        bool is_valid() const { return m_handle.is_valid(); }
        bool is_loaded() const { return m_handle.is_loaded(); }
        explicit operator bool() const { return is_valid(); }

        // Asset info
        AssetID get_id() const { return m_handle.get_id(); }
        AssetType get_type() const { return m_handle.get_type(); }
        AssetState get_state() const { return m_handle.get_state(); }

        // Reset
        void reset() { m_handle.reset(); }

        // Comparison
        bool operator==(const TypedAssetHandle& other) const {
            return m_handle == other.m_handle;
        }
        bool operator!=(const TypedAssetHandle& other) const {
            return !(*this == other);
        }

    private:
        AssetHandle m_handle;
    };

    // Asset factory interface
    class AssetFactory {
    public:
        virtual ~AssetFactory() = default;
        virtual AssetType get_asset_type() const = 0;
        virtual std::shared_ptr<Asset> create_asset(AssetID id, const std::string& path) = 0;
        virtual bool can_load(const std::string& extension) const = 0;
        virtual std::vector<std::string> get_supported_extensions() const = 0;
    };

    // Asset factory template
    template<typename T>
    class TypedAssetFactory : public AssetFactory {
        static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset");

    public:
        std::shared_ptr<Asset> create_asset(AssetID id, const std::string& path) override {
            return std::make_shared<T>(id, get_asset_type(), path);
        }
    };

} // namespace ecscope::assets