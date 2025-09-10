#pragma once

#include "network_types.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

namespace ecscope::networking {

/**
 * @brief Compression Algorithm Types
 */
enum class CompressionAlgorithm : uint8_t {
    NONE = 0,
    LZ4 = 1,
    ZSTD = 2,
    CUSTOM = 255
};

/**
 * @brief Compression Level
 */
enum class CompressionLevel : uint8_t {
    FASTEST = 1,
    FAST = 3,
    BALANCED = 6,
    GOOD = 9,
    BEST = 12
};

/**
 * @brief Compression Statistics
 */
struct CompressionStats {
    uint64_t bytes_compressed = 0;
    uint64_t bytes_decompressed = 0;
    uint64_t original_size = 0;
    uint64_t compressed_size = 0;
    uint64_t compression_operations = 0;
    uint64_t decompression_operations = 0;
    double average_compression_ratio = 1.0;
    double average_compression_time_us = 0.0;
    double average_decompression_time_us = 0.0;
    
    void update_compression_ratio() {
        if (original_size > 0) {
            average_compression_ratio = static_cast<double>(compressed_size) / 
                                      static_cast<double>(original_size);
        }
    }
    
    double get_compression_percentage() const {
        return (1.0 - average_compression_ratio) * 100.0;
    }
};

/**
 * @brief Base Compressor Interface
 */
class Compressor {
public:
    virtual ~Compressor() = default;
    
    // Compression operations
    virtual std::vector<uint8_t> compress(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> compress(const void* data, size_t size) = 0;
    virtual bool compress(const void* input, size_t input_size, 
                         void* output, size_t& output_size) = 0;
    
    // Decompression operations
    virtual std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data) = 0;
    virtual std::vector<uint8_t> decompress(const void* compressed_data, size_t compressed_size) = 0;
    virtual bool decompress(const void* compressed_input, size_t compressed_size,
                           void* output, size_t& output_size) = 0;
    
    // Size estimation
    virtual size_t get_max_compressed_size(size_t input_size) const = 0;
    virtual size_t get_decompressed_size(const void* compressed_data, size_t compressed_size) const = 0;
    
    // Configuration
    virtual void set_compression_level(CompressionLevel level) = 0;
    virtual CompressionLevel get_compression_level() const = 0;
    
    // Statistics
    virtual CompressionStats get_statistics() const = 0;
    virtual void reset_statistics() = 0;
    
    // Algorithm info
    virtual CompressionAlgorithm get_algorithm() const = 0;
    virtual std::string get_algorithm_name() const = 0;
};

/**
 * @brief No-op Compressor (pass-through)
 */
class NullCompressor : public Compressor {
public:
    NullCompressor() = default;
    ~NullCompressor() override = default;
    
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> compress(const void* data, size_t size) override;
    bool compress(const void* input, size_t input_size, 
                 void* output, size_t& output_size) override;
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data) override;
    std::vector<uint8_t> decompress(const void* compressed_data, size_t compressed_size) override;
    bool decompress(const void* compressed_input, size_t compressed_size,
                   void* output, size_t& output_size) override;
    
    size_t get_max_compressed_size(size_t input_size) const override;
    size_t get_decompressed_size(const void* compressed_data, size_t compressed_size) const override;
    
    void set_compression_level(CompressionLevel level) override {}
    CompressionLevel get_compression_level() const override { return CompressionLevel::FASTEST; }
    
    CompressionStats get_statistics() const override;
    void reset_statistics() override;
    
    CompressionAlgorithm get_algorithm() const override { return CompressionAlgorithm::NONE; }
    std::string get_algorithm_name() const override { return "None"; }

private:
    mutable CompressionStats statistics_;
    mutable std::mutex statistics_mutex_;
};

/**
 * @brief LZ4 Compressor Implementation
 */
class LZ4Compressor : public Compressor {
public:
    explicit LZ4Compressor(CompressionLevel level = CompressionLevel::FAST);
    ~LZ4Compressor() override;
    
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> compress(const void* data, size_t size) override;
    bool compress(const void* input, size_t input_size, 
                 void* output, size_t& output_size) override;
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data) override;
    std::vector<uint8_t> decompress(const void* compressed_data, size_t compressed_size) override;
    bool decompress(const void* compressed_input, size_t compressed_size,
                   void* output, size_t& output_size) override;
    
    size_t get_max_compressed_size(size_t input_size) const override;
    size_t get_decompressed_size(const void* compressed_data, size_t compressed_size) const override;
    
    void set_compression_level(CompressionLevel level) override;
    CompressionLevel get_compression_level() const override;
    
    CompressionStats get_statistics() const override;
    void reset_statistics() override;
    
    CompressionAlgorithm get_algorithm() const override { return CompressionAlgorithm::LZ4; }
    std::string get_algorithm_name() const override { return "LZ4"; }

private:
    CompressionLevel compression_level_;
    mutable CompressionStats statistics_;
    mutable std::mutex statistics_mutex_;
    
    void update_compression_stats(size_t original_size, size_t compressed_size, 
                                double time_microseconds) const;
    void update_decompression_stats(size_t compressed_size, size_t decompressed_size,
                                  double time_microseconds) const;
};

/**
 * @brief Zstandard Compressor Implementation
 */
class ZstdCompressor : public Compressor {
public:
    explicit ZstdCompressor(CompressionLevel level = CompressionLevel::BALANCED);
    ~ZstdCompressor() override;
    
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> compress(const void* data, size_t size) override;
    bool compress(const void* input, size_t input_size, 
                 void* output, size_t& output_size) override;
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data) override;
    std::vector<uint8_t> decompress(const void* compressed_data, size_t compressed_size) override;
    bool decompress(const void* compressed_input, size_t compressed_size,
                   void* output, size_t& output_size) override;
    
    size_t get_max_compressed_size(size_t input_size) const override;
    size_t get_decompressed_size(const void* compressed_data, size_t compressed_size) const override;
    
    void set_compression_level(CompressionLevel level) override;
    CompressionLevel get_compression_level() const override;
    
    CompressionStats get_statistics() const override;
    void reset_statistics() override;
    
    CompressionAlgorithm get_algorithm() const override { return CompressionAlgorithm::ZSTD; }
    std::string get_algorithm_name() const override { return "Zstandard"; }

private:
    struct ZstdContext;
    std::unique_ptr<ZstdContext> context_;
    CompressionLevel compression_level_;
    mutable CompressionStats statistics_;
    mutable std::mutex statistics_mutex_;
    
    void initialize_context();
    void cleanup_context();
    void update_compression_stats(size_t original_size, size_t compressed_size, 
                                double time_microseconds) const;
    void update_decompression_stats(size_t compressed_size, size_t decompressed_size,
                                  double time_microseconds) const;
};

/**
 * @brief Custom Compressor Interface
 * 
 * Allows users to implement their own compression algorithms.
 */
class CustomCompressor : public Compressor {
public:
    using CompressFunction = std::function<std::vector<uint8_t>(const void*, size_t)>;
    using DecompressFunction = std::function<std::vector<uint8_t>(const void*, size_t)>;
    using MaxSizeFunction = std::function<size_t(size_t)>;
    using DecompressedSizeFunction = std::function<size_t(const void*, size_t)>;
    
    CustomCompressor(const std::string& name,
                    CompressFunction compress_func,
                    DecompressFunction decompress_func,
                    MaxSizeFunction max_size_func,
                    DecompressedSizeFunction decompressed_size_func);
    ~CustomCompressor() override = default;
    
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> compress(const void* data, size_t size) override;
    bool compress(const void* input, size_t input_size, 
                 void* output, size_t& output_size) override;
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data) override;
    std::vector<uint8_t> decompress(const void* compressed_data, size_t compressed_size) override;
    bool decompress(const void* compressed_input, size_t compressed_size,
                   void* output, size_t& output_size) override;
    
    size_t get_max_compressed_size(size_t input_size) const override;
    size_t get_decompressed_size(const void* compressed_data, size_t compressed_size) const override;
    
    void set_compression_level(CompressionLevel level) override;
    CompressionLevel get_compression_level() const override;
    
    CompressionStats get_statistics() const override;
    void reset_statistics() override;
    
    CompressionAlgorithm get_algorithm() const override { return CompressionAlgorithm::CUSTOM; }
    std::string get_algorithm_name() const override { return algorithm_name_; }

private:
    std::string algorithm_name_;
    CompressFunction compress_function_;
    DecompressFunction decompress_function_;
    MaxSizeFunction max_size_function_;
    DecompressedSizeFunction decompressed_size_function_;
    CompressionLevel compression_level_;
    mutable CompressionStats statistics_;
    mutable std::mutex statistics_mutex_;
};

/**
 * @brief Compression Factory
 */
class CompressionFactory {
public:
    static std::unique_ptr<Compressor> create_compressor(CompressionAlgorithm algorithm,
                                                        CompressionLevel level = CompressionLevel::BALANCED);
    
    static std::unique_ptr<Compressor> create_null_compressor();
    static std::unique_ptr<Compressor> create_lz4_compressor(CompressionLevel level = CompressionLevel::FAST);
    static std::unique_ptr<Compressor> create_zstd_compressor(CompressionLevel level = CompressionLevel::BALANCED);
    
    // Registration for custom compressors
    using CompressorCreator = std::function<std::unique_ptr<Compressor>(CompressionLevel)>;
    static void register_compressor(CompressionAlgorithm algorithm, CompressorCreator creator);
    static void unregister_compressor(CompressionAlgorithm algorithm);
    
    // Query available algorithms
    static std::vector<CompressionAlgorithm> get_available_algorithms();
    static bool is_algorithm_available(CompressionAlgorithm algorithm);
    static std::string get_algorithm_name(CompressionAlgorithm algorithm);

private:
    static std::unordered_map<CompressionAlgorithm, CompressorCreator>& get_compressor_registry();
    static std::mutex& get_registry_mutex();
};

/**
 * @brief Adaptive Compression Manager
 * 
 * Automatically selects the best compression algorithm and level based on
 * data characteristics and network conditions.
 */
class AdaptiveCompressionManager {
public:
    struct AdaptiveConfig {
        bool enable_adaptive_algorithm = true;
        bool enable_adaptive_level = true;
        size_t min_data_size_for_compression = 64; // Don't compress small data
        double compression_ratio_threshold = 0.9;  // Don't use if compression ratio > this
        size_t analysis_window_size = 100;         // Number of samples for analysis
        std::chrono::milliseconds analysis_interval{1000}; // How often to analyze and adapt
    };
    
    explicit AdaptiveCompressionManager(const AdaptiveConfig& config = {});
    ~AdaptiveCompressionManager() = default;
    
    // Compression operations
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    std::vector<uint8_t> compress(const void* data, size_t size);
    bool compress(const void* input, size_t input_size, void* output, size_t& output_size);
    
    // Decompression operations
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data);
    std::vector<uint8_t> decompress(const void* compressed_data, size_t compressed_size);
    bool decompress(const void* compressed_input, size_t compressed_size,
                   void* output, size_t& output_size);
    
    // Configuration
    void set_config(const AdaptiveConfig& config) { config_ = config; }
    const AdaptiveConfig& get_config() const { return config_; }
    
    // Force specific algorithm/level (overrides adaptive behavior)
    void force_algorithm(CompressionAlgorithm algorithm);
    void force_level(CompressionLevel level);
    void clear_forced_settings();
    
    // Current settings
    CompressionAlgorithm get_current_algorithm() const { return current_algorithm_; }
    CompressionLevel get_current_level() const { return current_level_; }
    
    // Statistics and analysis
    CompressionStats get_statistics() const;
    void reset_statistics();
    
    // Performance analysis results
    struct AlgorithmPerformance {
        CompressionAlgorithm algorithm;
        double average_compression_ratio;
        double average_compression_time_us;
        double average_decompression_time_us;
        size_t sample_count;
        double performance_score; // Combined metric
    };
    
    std::vector<AlgorithmPerformance> get_algorithm_performance() const;

private:
    struct CompressionSample {
        CompressionAlgorithm algorithm;
        CompressionLevel level;
        size_t original_size;
        size_t compressed_size;
        double compression_time_us;
        double decompression_time_us;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    AdaptiveConfig config_;
    
    // Current compressor
    std::unique_ptr<Compressor> current_compressor_;
    CompressionAlgorithm current_algorithm_;
    CompressionLevel current_level_;
    
    // Forced settings
    std::optional<CompressionAlgorithm> forced_algorithm_;
    std::optional<CompressionLevel> forced_level_;
    
    // Performance tracking
    std::vector<CompressionSample> performance_samples_;
    std::chrono::steady_clock::time_point last_analysis_time_;
    mutable std::mutex samples_mutex_;
    
    // Analysis and adaptation
    void analyze_performance();
    void adapt_algorithm();
    void adapt_level();
    double calculate_performance_score(const AlgorithmPerformance& perf) const;
    void record_compression_sample(CompressionAlgorithm algorithm, CompressionLevel level,
                                 size_t original_size, size_t compressed_size,
                                 double compression_time_us, double decompression_time_us);
    
    // Compressor management
    void update_compressor_if_needed();
    std::unique_ptr<Compressor> create_compressor(CompressionAlgorithm algorithm, 
                                                 CompressionLevel level);
};

/**
 * @brief Network Compression Utilities
 */
namespace compression_utils {
    
    // Data analysis for compression decision
    struct DataCharacteristics {
        double entropy = 0.0;           // Shannon entropy
        double repetition_rate = 0.0;   // Percentage of repeated patterns
        size_t unique_bytes = 0;        // Number of unique byte values
        bool is_text = false;           // Appears to be text data
        bool is_binary = false;         // Appears to be binary data
        bool is_already_compressed = false; // Appears to be already compressed
    };
    
    DataCharacteristics analyze_data(const void* data, size_t size);
    bool should_compress(const DataCharacteristics& characteristics, size_t data_size);
    CompressionAlgorithm recommend_algorithm(const DataCharacteristics& characteristics);
    CompressionLevel recommend_level(const DataCharacteristics& characteristics);
    
    // Compression testing
    struct CompressionBenchmark {
        CompressionAlgorithm algorithm;
        CompressionLevel level;
        size_t original_size;
        size_t compressed_size;
        double compression_ratio;
        double compression_time_us;
        double decompression_time_us;
        double compression_throughput_mbps;
        double decompression_throughput_mbps;
    };
    
    std::vector<CompressionBenchmark> benchmark_compression(const void* data, size_t size,
                                                           const std::vector<CompressionAlgorithm>& algorithms = {},
                                                           const std::vector<CompressionLevel>& levels = {});
    
    CompressionBenchmark find_best_compression(const std::vector<CompressionBenchmark>& benchmarks,
                                              double compression_weight = 0.7,
                                              double speed_weight = 0.3);
}

} // namespace ecscope::networking