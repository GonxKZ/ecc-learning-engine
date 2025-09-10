#pragma once

#include "asset.hpp"
#include <memory>
#include <vector>
#include <string>
#include <future>
#include <fstream>

namespace ecscope::assets {

    // File loading interface
    class FileLoader {
    public:
        virtual ~FileLoader() = default;
        
        // Synchronous loading
        virtual std::vector<uint8_t> load_file(const std::string& path) = 0;
        virtual bool save_file(const std::string& path, const std::vector<uint8_t>& data) = 0;
        
        // Asynchronous loading
        virtual std::future<std::vector<uint8_t>> load_file_async(const std::string& path) = 0;
        
        // File information
        virtual bool file_exists(const std::string& path) const = 0;
        virtual size_t get_file_size(const std::string& path) const = 0;
        virtual std::chrono::system_clock::time_point get_file_modified_time(const std::string& path) const = 0;
        
        // Streaming support
        virtual bool supports_streaming() const { return false; }
        virtual std::unique_ptr<std::istream> open_stream(const std::string& path) { return nullptr; }
    };

    // Standard file system loader
    class FileSystemLoader : public FileLoader {
    public:
        explicit FileSystemLoader(const std::string& root_path = "");
        
        // FileLoader implementation
        std::vector<uint8_t> load_file(const std::string& path) override;
        bool save_file(const std::string& path, const std::vector<uint8_t>& data) override;
        std::future<std::vector<uint8_t>> load_file_async(const std::string& path) override;
        bool file_exists(const std::string& path) const override;
        size_t get_file_size(const std::string& path) const override;
        std::chrono::system_clock::time_point get_file_modified_time(const std::string& path) const override;
        bool supports_streaming() const override { return true; }
        std::unique_ptr<std::istream> open_stream(const std::string& path) override;
        
    private:
        std::string m_root_path;
        std::string resolve_path(const std::string& path) const;
    };

    // Memory-mapped file loader
    class MemoryMappedLoader : public FileLoader {
    public:
        explicit MemoryMappedLoader(const std::string& root_path = "");
        ~MemoryMappedLoader();
        
        // FileLoader implementation
        std::vector<uint8_t> load_file(const std::string& path) override;
        bool save_file(const std::string& path, const std::vector<uint8_t>& data) override;
        std::future<std::vector<uint8_t>> load_file_async(const std::string& path) override;
        bool file_exists(const std::string& path) const override;
        size_t get_file_size(const std::string& path) const override;
        std::chrono::system_clock::time_point get_file_modified_time(const std::string& path) const override;
        
        // Memory mapping specific
        const uint8_t* map_file(const std::string& path, size_t& size);
        void unmap_file(const std::string& path);
        
    private:
        std::string m_root_path;
        struct MappedFile;
        std::unordered_map<std::string, std::unique_ptr<MappedFile>> m_mapped_files;
        std::mutex m_mapped_files_mutex;
        
        std::string resolve_path(const std::string& path) const;
    };

    // Network loader for remote assets
    class NetworkLoader : public FileLoader {
    public:
        explicit NetworkLoader(const std::string& base_url = "");
        
        // FileLoader implementation
        std::vector<uint8_t> load_file(const std::string& path) override;
        bool save_file(const std::string& path, const std::vector<uint8_t>& data) override;
        std::future<std::vector<uint8_t>> load_file_async(const std::string& path) override;
        bool file_exists(const std::string& path) const override;
        size_t get_file_size(const std::string& path) const override;
        std::chrono::system_clock::time_point get_file_modified_time(const std::string& path) const override;
        
        // Network specific
        void set_base_url(const std::string& url) { m_base_url = url; }
        void set_timeout(int timeout_ms) { m_timeout_ms = timeout_ms; }
        
    private:
        std::string m_base_url;
        int m_timeout_ms = 30000; // 30 seconds default
        
        std::string resolve_url(const std::string& path) const;
    };

    // Asset loader - high-level asset loading interface
    class AssetLoader {
    public:
        AssetLoader();
        ~AssetLoader();
        
        // Loader management
        void add_loader(std::unique_ptr<FileLoader> loader, int priority = 0);
        void remove_loader(FileLoader* loader);
        void clear_loaders();
        
        // Asset loading
        std::vector<uint8_t> load_raw_data(const std::string& path);
        std::future<std::vector<uint8_t>> load_raw_data_async(const std::string& path);
        
        bool load_asset_data(Asset* asset);
        std::future<bool> load_asset_data_async(Asset* asset);
        
        // File operations
        bool file_exists(const std::string& path) const;
        size_t get_file_size(const std::string& path) const;
        std::chrono::system_clock::time_point get_file_modified_time(const std::string& path) const;
        
        // Streaming support
        std::unique_ptr<std::istream> open_stream(const std::string& path);
        
        // Configuration
        void set_root_path(const std::string& path);
        const std::string& get_root_path() const { return m_root_path; }
        
        // Statistics
        size_t get_load_count() const { return m_load_count; }
        size_t get_bytes_loaded() const { return m_bytes_loaded; }
        void reset_statistics() { m_load_count = 0; m_bytes_loaded = 0; }
        
    private:
        struct LoaderEntry {
            std::unique_ptr<FileLoader> loader;
            int priority;
            
            bool operator<(const LoaderEntry& other) const {
                return priority < other.priority; // Higher priority first
            }
        };
        
        std::vector<LoaderEntry> m_loaders;
        std::string m_root_path;
        mutable std::shared_mutex m_loaders_mutex;
        
        // Statistics
        std::atomic<size_t> m_load_count{0};
        std::atomic<size_t> m_bytes_loaded{0};
        
        FileLoader* find_loader_for_path(const std::string& path) const;
    };

    // Compression utilities
    namespace compression {
        // LZ4 compression
        std::vector<uint8_t> compress_lz4(const std::vector<uint8_t>& data);
        std::vector<uint8_t> decompress_lz4(const std::vector<uint8_t>& compressed_data, size_t uncompressed_size);
        
        // Zstandard compression
        std::vector<uint8_t> compress_zstd(const std::vector<uint8_t>& data, int level = 3);
        std::vector<uint8_t> decompress_zstd(const std::vector<uint8_t>& compressed_data);
        
        // Compression detection
        enum class CompressionType {
            NONE = 0,
            LZ4,
            ZSTD
        };
        
        CompressionType detect_compression(const std::vector<uint8_t>& data);
        std::vector<uint8_t> compress(const std::vector<uint8_t>& data, CompressionType type, int level = 3);
        std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data, CompressionType type, size_t uncompressed_size = 0);
    }

    // Streaming reader for large assets
    class StreamingReader {
    public:
        StreamingReader(std::unique_ptr<std::istream> stream, size_t buffer_size = 64 * 1024);
        ~StreamingReader();
        
        // Read operations
        size_t read(uint8_t* buffer, size_t size);
        size_t read_chunk(std::vector<uint8_t>& buffer);
        bool read_all(std::vector<uint8_t>& buffer);
        
        // Stream state
        bool is_eof() const;
        bool has_error() const;
        size_t get_position() const { return m_position; }
        size_t get_total_size() const { return m_total_size; }
        float get_progress() const;
        
        // Seeking (if supported)
        bool seek(size_t position);
        bool can_seek() const;
        
    private:
        std::unique_ptr<std::istream> m_stream;
        size_t m_buffer_size;
        size_t m_position;
        size_t m_total_size;
        std::vector<uint8_t> m_internal_buffer;
    };

} // namespace ecscope::assets