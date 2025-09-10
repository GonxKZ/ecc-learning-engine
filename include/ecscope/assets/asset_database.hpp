#pragma once

#include "asset_types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <future>

namespace ecscope::assets {

    // Database query interface
    struct AssetQuery {
        // Filter criteria
        std::optional<AssetType> type;
        std::optional<AssetState> state;
        std::optional<QualityLevel> quality;
        std::vector<std::string> tags;
        std::string name_pattern;
        std::string path_pattern;
        
        // Size filters
        std::optional<size_t> min_size;
        std::optional<size_t> max_size;
        
        // Date filters
        std::optional<std::chrono::system_clock::time_point> created_after;
        std::optional<std::chrono::system_clock::time_point> created_before;
        std::optional<std::chrono::system_clock::time_point> modified_after;
        std::optional<std::chrono::system_clock::time_point> modified_before;
        
        // Dependency filters
        bool has_dependencies = false;
        std::vector<AssetID> depends_on;
        std::vector<AssetID> depended_by;
        
        // Sorting
        enum class SortBy {
            ID, NAME, SIZE, TYPE, CREATED, MODIFIED, ACCESS_COUNT
        };
        SortBy sort_by = SortBy::ID;
        bool ascending = true;
        
        // Pagination
        size_t offset = 0;
        size_t limit = 100;
    };

    // Database record for assets
    struct AssetRecord {
        AssetID id;
        std::string path;
        std::string name;
        AssetType type;
        AssetState state;
        QualityLevel quality;
        AssetVersion version;
        size_t size_bytes;
        std::chrono::system_clock::time_point created_time;
        std::chrono::system_clock::time_point modified_time;
        std::chrono::system_clock::time_point last_accessed;
        uint32_t access_count;
        LoadFlags flags;
        
        // Extended metadata
        std::vector<std::string> tags;
        std::unordered_map<std::string, std::string> custom_properties;
        std::vector<AssetID> dependencies;
        std::vector<AssetID> dependents;
        
        // File information
        std::string file_hash;
        std::string mime_type;
        bool is_compressed;
        size_t compressed_size;
        
        // Usage statistics
        std::chrono::milliseconds total_load_time{0};
        uint32_t load_count = 0;
        uint32_t error_count = 0;
        
        AssetRecord() : id(INVALID_ASSET_ID), type(AssetType::UNKNOWN), 
                       state(AssetState::UNLOADED), quality(QualityLevel::MEDIUM),
                       version(0), size_bytes(0), access_count(0), 
                       flags(LoadFlags::NONE), is_compressed(false), 
                       compressed_size(0) {}
    };

    // Asset database interface
    class AssetDatabase {
    public:
        virtual ~AssetDatabase() = default;
        
        // Connection management
        virtual bool connect(const std::string& connection_string) = 0;
        virtual void disconnect() = 0;
        virtual bool is_connected() const = 0;
        
        // Schema management
        virtual bool create_schema() = 0;
        virtual bool update_schema(int target_version) = 0;
        virtual int get_schema_version() const = 0;
        
        // Asset operations
        virtual bool insert_asset(const AssetRecord& record) = 0;
        virtual bool update_asset(const AssetRecord& record) = 0;
        virtual bool delete_asset(AssetID id) = 0;
        virtual bool asset_exists(AssetID id) const = 0;
        
        // Asset retrieval
        virtual std::optional<AssetRecord> get_asset(AssetID id) const = 0;
        virtual std::optional<AssetRecord> get_asset_by_path(const std::string& path) const = 0;
        virtual std::vector<AssetRecord> query_assets(const AssetQuery& query) const = 0;
        virtual size_t count_assets(const AssetQuery& query) const = 0;
        
        // Batch operations
        virtual bool insert_assets_batch(const std::vector<AssetRecord>& records) = 0;
        virtual bool update_assets_batch(const std::vector<AssetRecord>& records) = 0;
        virtual bool delete_assets_batch(const std::vector<AssetID>& ids) = 0;
        
        // Dependency management
        virtual bool add_dependency(AssetID asset, AssetID dependency) = 0;
        virtual bool remove_dependency(AssetID asset, AssetID dependency) = 0;
        virtual std::vector<AssetID> get_dependencies(AssetID asset) const = 0;
        virtual std::vector<AssetID> get_dependents(AssetID asset) const = 0;
        
        // Tag management
        virtual bool add_tag(AssetID asset, const std::string& tag) = 0;
        virtual bool remove_tag(AssetID asset, const std::string& tag) = 0;
        virtual std::vector<std::string> get_tags(AssetID asset) const = 0;
        virtual std::vector<AssetID> find_assets_by_tag(const std::string& tag) const = 0;
        
        // Statistics and analytics
        virtual size_t get_total_asset_count() const = 0;
        virtual size_t get_total_size_bytes() const = 0;
        virtual std::unordered_map<AssetType, size_t> get_asset_count_by_type() const = 0;
        virtual std::unordered_map<AssetState, size_t> get_asset_count_by_state() const = 0;
        
        // Maintenance
        virtual bool optimize_database() = 0;
        virtual bool vacuum_database() = 0;
        virtual bool backup_database(const std::string& backup_path) = 0;
        virtual bool restore_database(const std::string& backup_path) = 0;
        
        // Async operations
        virtual std::future<std::vector<AssetRecord>> query_assets_async(const AssetQuery& query) const = 0;
        virtual std::future<bool> insert_asset_async(const AssetRecord& record) = 0;
        virtual std::future<bool> update_asset_async(const AssetRecord& record) = 0;
    };

    // SQLite database implementation
    class SQLiteAssetDatabase : public AssetDatabase {
    public:
        explicit SQLiteAssetDatabase();
        ~SQLiteAssetDatabase();
        
        // AssetDatabase implementation
        bool connect(const std::string& connection_string) override;
        void disconnect() override;
        bool is_connected() const override;
        
        bool create_schema() override;
        bool update_schema(int target_version) override;
        int get_schema_version() const override;
        
        bool insert_asset(const AssetRecord& record) override;
        bool update_asset(const AssetRecord& record) override;
        bool delete_asset(AssetID id) override;
        bool asset_exists(AssetID id) const override;
        
        std::optional<AssetRecord> get_asset(AssetID id) const override;
        std::optional<AssetRecord> get_asset_by_path(const std::string& path) const override;
        std::vector<AssetRecord> query_assets(const AssetQuery& query) const override;
        size_t count_assets(const AssetQuery& query) const override;
        
        bool insert_assets_batch(const std::vector<AssetRecord>& records) override;
        bool update_assets_batch(const std::vector<AssetRecord>& records) override;
        bool delete_assets_batch(const std::vector<AssetID>& ids) override;
        
        bool add_dependency(AssetID asset, AssetID dependency) override;
        bool remove_dependency(AssetID asset, AssetID dependency) override;
        std::vector<AssetID> get_dependencies(AssetID asset) const override;
        std::vector<AssetID> get_dependents(AssetID asset) const override;
        
        bool add_tag(AssetID asset, const std::string& tag) override;
        bool remove_tag(AssetID asset, const std::string& tag) override;
        std::vector<std::string> get_tags(AssetID asset) const override;
        std::vector<AssetID> find_assets_by_tag(const std::string& tag) const override;
        
        size_t get_total_asset_count() const override;
        size_t get_total_size_bytes() const override;
        std::unordered_map<AssetType, size_t> get_asset_count_by_type() const override;
        std::unordered_map<AssetState, size_t> get_asset_count_by_state() const override;
        
        bool optimize_database() override;
        bool vacuum_database() override;
        bool backup_database(const std::string& backup_path) override;
        bool restore_database(const std::string& backup_path) override;
        
        std::future<std::vector<AssetRecord>> query_assets_async(const AssetQuery& query) const override;
        std::future<bool> insert_asset_async(const AssetRecord& record) override;
        std::future<bool> update_asset_async(const AssetRecord& record) override;
        
        // SQLite-specific methods
        void set_wal_mode(bool enable);
        void set_cache_size(size_t size_kb);
        void set_synchronous_mode(int mode); // 0=OFF, 1=NORMAL, 2=FULL
        
        // Transaction support
        bool begin_transaction();
        bool commit_transaction();
        bool rollback_transaction();
        
    private:
        struct SQLiteImpl;
        std::unique_ptr<SQLiteImpl> m_impl;
        
        bool execute_sql(const std::string& sql) const;
        std::string build_query_sql(const AssetQuery& query) const;
        AssetRecord record_from_row(void* stmt) const;
        void bind_record_parameters(void* stmt, const AssetRecord& record) const;
    };

    // In-memory database for testing and caching
    class MemoryAssetDatabase : public AssetDatabase {
    public:
        MemoryAssetDatabase();
        ~MemoryAssetDatabase();
        
        // AssetDatabase implementation
        bool connect(const std::string& connection_string) override;
        void disconnect() override;
        bool is_connected() const override;
        
        bool create_schema() override;
        bool update_schema(int target_version) override;
        int get_schema_version() const override;
        
        bool insert_asset(const AssetRecord& record) override;
        bool update_asset(const AssetRecord& record) override;
        bool delete_asset(AssetID id) override;
        bool asset_exists(AssetID id) const override;
        
        std::optional<AssetRecord> get_asset(AssetID id) const override;
        std::optional<AssetRecord> get_asset_by_path(const std::string& path) const override;
        std::vector<AssetRecord> query_assets(const AssetQuery& query) const override;
        size_t count_assets(const AssetQuery& query) const override;
        
        bool insert_assets_batch(const std::vector<AssetRecord>& records) override;
        bool update_assets_batch(const std::vector<AssetRecord>& records) override;
        bool delete_assets_batch(const std::vector<AssetID>& ids) override;
        
        bool add_dependency(AssetID asset, AssetID dependency) override;
        bool remove_dependency(AssetID asset, AssetID dependency) override;
        std::vector<AssetID> get_dependencies(AssetID asset) const override;
        std::vector<AssetID> get_dependents(AssetID asset) const override;
        
        bool add_tag(AssetID asset, const std::string& tag) override;
        bool remove_tag(AssetID asset, const std::string& tag) override;
        std::vector<std::string> get_tags(AssetID asset) const override;
        std::vector<AssetID> find_assets_by_tag(const std::string& tag) const override;
        
        size_t get_total_asset_count() const override;
        size_t get_total_size_bytes() const override;
        std::unordered_map<AssetType, size_t> get_asset_count_by_type() const override;
        std::unordered_map<AssetState, size_t> get_asset_count_by_state() const override;
        
        bool optimize_database() override;
        bool vacuum_database() override;
        bool backup_database(const std::string& backup_path) override;
        bool restore_database(const std::string& backup_path) override;
        
        std::future<std::vector<AssetRecord>> query_assets_async(const AssetQuery& query) const override;
        std::future<bool> insert_asset_async(const AssetRecord& record) override;
        std::future<bool> update_asset_async(const AssetRecord& record) override;
        
        // Memory-specific methods
        void clear();
        void dump_to_json(const std::string& file_path) const;
        bool load_from_json(const std::string& file_path);
        
    private:
        std::unordered_map<AssetID, AssetRecord> m_assets;
        std::unordered_map<std::string, AssetID> m_path_to_id;
        std::unordered_map<AssetID, std::vector<AssetID>> m_dependencies;
        std::unordered_map<AssetID, std::vector<AssetID>> m_dependents;
        std::unordered_map<AssetID, std::vector<std::string>> m_asset_tags;
        std::unordered_map<std::string, std::vector<AssetID>> m_tag_to_assets;
        
        mutable std::shared_mutex m_mutex;
        bool m_connected;
        int m_schema_version;
        
        bool matches_query(const AssetRecord& record, const AssetQuery& query) const;
        std::vector<AssetRecord> sort_results(std::vector<AssetRecord> results, const AssetQuery& query) const;
    };

    // Database factory
    std::unique_ptr<AssetDatabase> create_sqlite_database(const std::string& db_path);
    std::unique_ptr<AssetDatabase> create_memory_database();

    // Database utilities
    namespace db_utils {
        std::string asset_type_to_string(AssetType type);
        AssetType string_to_asset_type(const std::string& str);
        
        std::string asset_state_to_string(AssetState state);
        AssetState string_to_asset_state(const std::string& str);
        
        std::string quality_level_to_string(QualityLevel level);
        QualityLevel string_to_quality_level(const std::string& str);
        
        std::string calculate_file_hash(const std::string& file_path);
        std::string calculate_data_hash(const std::vector<uint8_t>& data);
        
        std::string get_mime_type(const std::string& file_path);
        
        // Query builder helpers
        class QueryBuilder {
        public:
            QueryBuilder& filter_by_type(AssetType type);
            QueryBuilder& filter_by_state(AssetState state);
            QueryBuilder& filter_by_tag(const std::string& tag);
            QueryBuilder& filter_by_path_pattern(const std::string& pattern);
            QueryBuilder& filter_by_size_range(size_t min_size, size_t max_size);
            QueryBuilder& filter_by_date_range(std::chrono::system_clock::time_point start,
                                             std::chrono::system_clock::time_point end);
            QueryBuilder& sort_by(AssetQuery::SortBy sort_by, bool ascending = true);
            QueryBuilder& limit(size_t count, size_t offset = 0);
            
            AssetQuery build() const { return m_query; }
            
        private:
            AssetQuery m_query;
        };
        
        QueryBuilder create_query();
    }

} // namespace ecscope::assets