#pragma once

#include "../asset.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <future>

namespace ecscope::assets::processors {

    // Processing result
    struct ProcessingResult {
        bool success = false;
        std::string error_message;
        std::vector<uint8_t> processed_data;
        AssetMetadata output_metadata;
        std::unordered_map<std::string, std::string> processing_info;
        std::chrono::milliseconds processing_time{0};
        
        ProcessingResult() = default;
        ProcessingResult(bool success) : success(success) {}
        ProcessingResult(const std::string& error) : success(false), error_message(error) {}
    };

    // Processing options
    struct ProcessingOptions {
        QualityLevel quality = QualityLevel::MEDIUM;
        bool enable_compression = true;
        bool generate_mipmaps = true;
        bool optimize_for_size = false;
        bool optimize_for_speed = true;
        std::unordered_map<std::string, std::string> custom_options;
        
        // Quality-specific options
        struct TextureOptions {
            int compression_quality = 85;
            bool use_bc_compression = true;
            int max_resolution = 2048;
            bool generate_normal_maps = false;
        } texture_options;
        
        struct MeshOptions {
            bool optimize_vertices = true;
            bool generate_normals = true;
            bool generate_tangents = true;
            float normal_smoothing_angle = 45.0f;
            int target_triangle_count = -1; // -1 = no limit
        } mesh_options;
        
        struct AudioOptions {
            int sample_rate = 44100;
            int bit_depth = 16;
            bool convert_to_mono = false;
            float compression_quality = 0.7f;
        } audio_options;
    };

    // Asset processor interface
    class AssetProcessor {
    public:
        virtual ~AssetProcessor() = default;
        
        // Processor information
        virtual AssetType get_supported_type() const = 0;
        virtual std::vector<std::string> get_supported_extensions() const = 0;
        virtual std::string get_processor_name() const = 0;
        virtual std::string get_processor_version() const = 0;
        
        // Processing capabilities
        virtual bool can_process(const std::string& file_path, const AssetMetadata& metadata) const = 0;
        virtual bool supports_quality_level(QualityLevel quality) const = 0;
        virtual bool supports_streaming() const { return false; }
        
        // Processing operations
        virtual ProcessingResult process(const std::vector<uint8_t>& input_data,
                                       const AssetMetadata& input_metadata,
                                       const ProcessingOptions& options) = 0;
        
        virtual std::future<ProcessingResult> process_async(const std::vector<uint8_t>& input_data,
                                                           const AssetMetadata& input_metadata,
                                                           const ProcessingOptions& options) = 0;
        
        // Validation
        virtual bool validate_input(const std::vector<uint8_t>& input_data,
                                  const AssetMetadata& metadata) const = 0;
        virtual bool validate_output(const ProcessingResult& result) const = 0;
        
        // Metadata extraction
        virtual AssetMetadata extract_metadata(const std::vector<uint8_t>& data,
                                             const std::string& file_path) const = 0;
        
        // Processing estimation
        virtual std::chrono::milliseconds estimate_processing_time(size_t input_size,
                                                                  const ProcessingOptions& options) const = 0;
        virtual size_t estimate_output_size(size_t input_size,
                                           const ProcessingOptions& options) const = 0;
        
        // Configuration
        virtual void configure(const std::unordered_map<std::string, std::string>& config) {}
        virtual std::unordered_map<std::string, std::string> get_configuration() const { return {}; }
    };

    // Base processor implementation
    class BaseAssetProcessor : public AssetProcessor {
    public:
        explicit BaseAssetProcessor(AssetType type, const std::string& name, const std::string& version);
        
        // AssetProcessor implementation
        AssetType get_supported_type() const override { return m_supported_type; }
        std::string get_processor_name() const override { return m_processor_name; }
        std::string get_processor_version() const override { return m_processor_version; }
        
        std::future<ProcessingResult> process_async(const std::vector<uint8_t>& input_data,
                                                   const AssetMetadata& input_metadata,
                                                   const ProcessingOptions& options) override;
        
        bool supports_quality_level(QualityLevel quality) const override;
        bool validate_output(const ProcessingResult& result) const override;
        
        std::chrono::milliseconds estimate_processing_time(size_t input_size,
                                                          const ProcessingOptions& options) const override;
        
        void configure(const std::unordered_map<std::string, std::string>& config) override;
        std::unordered_map<std::string, std::string> get_configuration() const override;
        
    protected:
        AssetType m_supported_type;
        std::string m_processor_name;
        std::string m_processor_version;
        std::unordered_map<std::string, std::string> m_configuration;
        
        // Helper methods
        ProcessingResult create_error_result(const std::string& error) const;
        ProcessingResult create_success_result(std::vector<uint8_t> data, 
                                              const AssetMetadata& metadata) const;
        
        // Quality level utilities
        int get_compression_quality_for_level(QualityLevel quality) const;
        int get_max_resolution_for_level(QualityLevel quality) const;
        float get_quality_multiplier(QualityLevel quality) const;
    };

    // Processing pipeline
    class ProcessingPipeline {
    public:
        ProcessingPipeline();
        ~ProcessingPipeline();
        
        // Processor registration
        void register_processor(std::unique_ptr<AssetProcessor> processor);
        void unregister_processor(AssetType type);
        bool has_processor(AssetType type) const;
        AssetProcessor* get_processor(AssetType type) const;
        
        // File extension mapping
        void register_extension_mapping(const std::string& extension, AssetType type);
        AssetType detect_asset_type(const std::string& file_path) const;
        std::vector<std::string> get_supported_extensions() const;
        
        // Processing operations
        ProcessingResult process_asset(const std::string& file_path,
                                     const std::vector<uint8_t>& input_data,
                                     const ProcessingOptions& options = ProcessingOptions{});
        
        ProcessingResult process_asset(AssetType type,
                                     const std::vector<uint8_t>& input_data,
                                     const AssetMetadata& metadata,
                                     const ProcessingOptions& options = ProcessingOptions{});
        
        std::future<ProcessingResult> process_asset_async(const std::string& file_path,
                                                         const std::vector<uint8_t>& input_data,
                                                         const ProcessingOptions& options = ProcessingOptions{});
        
        // Batch processing
        std::vector<ProcessingResult> process_assets_batch(
            const std::vector<std::pair<std::string, std::vector<uint8_t>>>& assets,
            const ProcessingOptions& options = ProcessingOptions{});
        
        std::future<std::vector<ProcessingResult>> process_assets_batch_async(
            const std::vector<std::pair<std::string, std::vector<uint8_t>>>& assets,
            const ProcessingOptions& options = ProcessingOptions{});
        
        // Validation
        bool validate_asset(const std::string& file_path, const std::vector<uint8_t>& data) const;
        std::vector<std::string> get_validation_errors(const std::string& file_path,
                                                       const std::vector<uint8_t>& data) const;
        
        // Metadata extraction
        AssetMetadata extract_metadata(const std::string& file_path,
                                     const std::vector<uint8_t>& data) const;
        
        // Processing estimation
        std::chrono::milliseconds estimate_processing_time(const std::string& file_path,
                                                          size_t input_size,
                                                          const ProcessingOptions& options) const;
        
        // Configuration
        void set_thread_count(size_t count);
        size_t get_thread_count() const;
        
        void set_cache_enabled(bool enabled) { m_cache_enabled = enabled; }
        bool is_cache_enabled() const { return m_cache_enabled; }
        
        // Statistics
        struct ProcessingStatistics {
            std::atomic<uint64_t> total_processed{0};
            std::atomic<uint64_t> successful_processed{0};
            std::atomic<uint64_t> failed_processed{0};
            std::atomic<uint64_t> bytes_processed{0};
            std::atomic<uint64_t> total_processing_time_ms{0};
            
            void reset() {
                total_processed = 0;
                successful_processed = 0;
                failed_processed = 0;
                bytes_processed = 0;
                total_processing_time_ms = 0;
            }
            
            double get_success_rate() const {
                auto total = total_processed.load();
                return total > 0 ? static_cast<double>(successful_processed.load()) / total : 0.0;
            }
            
            double get_average_processing_time_ms() const {
                auto total = total_processed.load();
                return total > 0 ? static_cast<double>(total_processing_time_ms.load()) / total : 0.0;
            }
        };
        
        const ProcessingStatistics& get_statistics() const { return m_statistics; }
        void reset_statistics() { m_statistics.reset(); }
        
        // Debugging
        void dump_processor_info() const;
        std::vector<std::string> get_processor_names() const;
        
    private:
        std::unordered_map<AssetType, std::unique_ptr<AssetProcessor>> m_processors;
        std::unordered_map<std::string, AssetType> m_extension_mapping;
        
        // Configuration
        size_t m_thread_count;
        bool m_cache_enabled;
        
        // Processing cache
        struct CacheKey {
            std::string file_path;
            size_t data_hash;
            ProcessingOptions options;
            
            bool operator==(const CacheKey& other) const;
        };
        
        struct CacheKeyHash {
            size_t operator()(const CacheKey& key) const;
        };
        
        std::unordered_map<CacheKey, ProcessingResult, CacheKeyHash> m_processing_cache;
        
        // Statistics
        mutable ProcessingStatistics m_statistics;
        
        // Thread safety
        mutable std::shared_mutex m_processors_mutex;
        mutable std::mutex m_cache_mutex;
        
        // Internal methods
        ProcessingResult process_internal(AssetProcessor* processor,
                                        const std::vector<uint8_t>& input_data,
                                        const AssetMetadata& metadata,
                                        const ProcessingOptions& options);
        
        void update_statistics(const ProcessingResult& result, 
                              std::chrono::milliseconds processing_time);
        
        size_t calculate_data_hash(const std::vector<uint8_t>& data) const;
        std::string extract_file_extension(const std::string& file_path) const;
    };

    // Global processing pipeline
    ProcessingPipeline& get_processing_pipeline();
    void set_processing_pipeline(std::unique_ptr<ProcessingPipeline> pipeline);

    // Processor factory
    template<typename T>
    std::unique_ptr<AssetProcessor> create_processor() {
        static_assert(std::is_base_of_v<AssetProcessor, T>, "T must derive from AssetProcessor");
        return std::make_unique<T>();
    }

    // Processing utilities
    namespace utils {
        std::string get_file_extension(const std::string& file_path);
        std::string get_file_name(const std::string& file_path);
        std::string get_file_directory(const std::string& file_path);
        
        bool is_power_of_two(uint32_t value);
        uint32_t next_power_of_two(uint32_t value);
        uint32_t previous_power_of_two(uint32_t value);
        
        std::vector<uint8_t> read_file(const std::string& file_path);
        bool write_file(const std::string& file_path, const std::vector<uint8_t>& data);
        
        size_t calculate_hash(const std::vector<uint8_t>& data);
        std::string calculate_md5(const std::vector<uint8_t>& data);
        std::string calculate_sha256(const std::vector<uint8_t>& data);
    }

} // namespace ecscope::assets::processors