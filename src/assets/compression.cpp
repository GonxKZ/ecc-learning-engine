#include "ecscope/assets/asset_loader.hpp"
#include <cstring>
#include <algorithm>

// For a real implementation, you would include:
// #include <lz4.h>
// #include <zstd.h>
// For now, we'll provide stub implementations

namespace ecscope::assets::compression {

    // LZ4 compression (stub implementation)
    std::vector<uint8_t> compress_lz4(const std::vector<uint8_t>& data) {
        if (data.empty()) {
            return {};
        }

        // In a real implementation, this would use LZ4_compress_default
        // For now, we'll just add a simple header and copy the data
        std::vector<uint8_t> compressed;
        compressed.reserve(data.size() + 16);
        
        // Magic bytes for LZ4
        compressed.push_back(0x04);
        compressed.push_back(0x22);
        compressed.push_back(0x4D);
        compressed.push_back(0x18);
        
        // Original size (little endian)
        uint32_t original_size = static_cast<uint32_t>(data.size());
        compressed.push_back(original_size & 0xFF);
        compressed.push_back((original_size >> 8) & 0xFF);
        compressed.push_back((original_size >> 16) & 0xFF);
        compressed.push_back((original_size >> 24) & 0xFF);
        
        // Compressed size placeholder (for real implementation)
        uint32_t compressed_size = static_cast<uint32_t>(data.size());
        compressed.push_back(compressed_size & 0xFF);
        compressed.push_back((compressed_size >> 8) & 0xFF);
        compressed.push_back((compressed_size >> 16) & 0xFF);
        compressed.push_back((compressed_size >> 24) & 0xFF);
        
        // Reserved bytes
        compressed.push_back(0x00);
        compressed.push_back(0x00);
        compressed.push_back(0x00);
        compressed.push_back(0x00);
        
        // Copy original data (in real implementation, this would be compressed)
        compressed.insert(compressed.end(), data.begin(), data.end());
        
        return compressed;
    }

    std::vector<uint8_t> decompress_lz4(const std::vector<uint8_t>& compressed_data, size_t uncompressed_size) {
        if (compressed_data.size() < 16) {
            return {};
        }
        
        // Verify magic bytes
        if (compressed_data[0] != 0x04 || compressed_data[1] != 0x22 ||
            compressed_data[2] != 0x4D || compressed_data[3] != 0x18) {
            return {};
        }
        
        // Read original size
        uint32_t original_size = static_cast<uint32_t>(compressed_data[4]) |
                                (static_cast<uint32_t>(compressed_data[5]) << 8) |
                                (static_cast<uint32_t>(compressed_data[6]) << 16) |
                                (static_cast<uint32_t>(compressed_data[7]) << 24);
        
        if (uncompressed_size != 0 && original_size != uncompressed_size) {
            return {};
        }
        
        // In real implementation, would use LZ4_decompress_safe
        // For now, just copy the data after header
        std::vector<uint8_t> decompressed;
        decompressed.reserve(original_size);
        
        auto start_it = compressed_data.begin() + 16;
        decompressed.insert(decompressed.end(), start_it, compressed_data.end());
        
        if (decompressed.size() != original_size) {
            decompressed.resize(original_size);
        }
        
        return decompressed;
    }

    // Zstandard compression (stub implementation)
    std::vector<uint8_t> compress_zstd(const std::vector<uint8_t>& data, int level) {
        if (data.empty()) {
            return {};
        }

        // In a real implementation, this would use ZSTD_compress
        std::vector<uint8_t> compressed;
        compressed.reserve(data.size() + 16);
        
        // Magic bytes for Zstandard
        compressed.push_back(0x28);
        compressed.push_back(0xB5);
        compressed.push_back(0x2F);
        compressed.push_back(0xFD);
        
        // Frame header (simplified)
        compressed.push_back(0x20); // Frame_Header_Descriptor
        compressed.push_back(0x00); // Frame_Content_Size_flag = 0
        compressed.push_back(0x00);
        compressed.push_back(0x00);
        
        // Original size (8 bytes, little endian)
        uint64_t original_size = data.size();
        for (int i = 0; i < 8; ++i) {
            compressed.push_back((original_size >> (i * 8)) & 0xFF);
        }
        
        // Copy original data (in real implementation, this would be compressed)
        compressed.insert(compressed.end(), data.begin(), data.end());
        
        return compressed;
    }

    std::vector<uint8_t> decompress_zstd(const std::vector<uint8_t>& compressed_data) {
        if (compressed_data.size() < 16) {
            return {};
        }
        
        // Verify magic bytes
        if (compressed_data[0] != 0x28 || compressed_data[1] != 0xB5 ||
            compressed_data[2] != 0x2F || compressed_data[3] != 0xFD) {
            return {};
        }
        
        // Read original size (skip frame header for simplicity)
        uint64_t original_size = 0;
        for (int i = 0; i < 8; ++i) {
            original_size |= static_cast<uint64_t>(compressed_data[8 + i]) << (i * 8);
        }
        
        // In real implementation, would use ZSTD_decompress
        std::vector<uint8_t> decompressed;
        decompressed.reserve(original_size);
        
        auto start_it = compressed_data.begin() + 16;
        decompressed.insert(decompressed.end(), start_it, compressed_data.end());
        
        if (decompressed.size() != original_size) {
            decompressed.resize(original_size);
        }
        
        return decompressed;
    }

    // Compression detection
    CompressionType detect_compression(const std::vector<uint8_t>& data) {
        if (data.size() < 4) {
            return CompressionType::NONE;
        }
        
        // Check LZ4 magic
        if (data[0] == 0x04 && data[1] == 0x22 && data[2] == 0x4D && data[3] == 0x18) {
            return CompressionType::LZ4;
        }
        
        // Check Zstandard magic
        if (data[0] == 0x28 && data[1] == 0xB5 && data[2] == 0x2F && data[3] == 0xFD) {
            return CompressionType::ZSTD;
        }
        
        return CompressionType::NONE;
    }

    std::vector<uint8_t> compress(const std::vector<uint8_t>& data, CompressionType type, int level) {
        switch (type) {
            case CompressionType::LZ4:
                return compress_lz4(data);
            case CompressionType::ZSTD:
                return compress_zstd(data, level);
            case CompressionType::NONE:
            default:
                return data;
        }
    }

    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data, 
                                   CompressionType type, size_t uncompressed_size) {
        switch (type) {
            case CompressionType::LZ4:
                return decompress_lz4(compressed_data, uncompressed_size);
            case CompressionType::ZSTD:
                return decompress_zstd(compressed_data);
            case CompressionType::NONE:
            default:
                return compressed_data;
        }
    }

    // Auto-detect and decompress
    std::vector<uint8_t> auto_decompress(const std::vector<uint8_t>& data) {
        CompressionType type = detect_compression(data);
        if (type == CompressionType::NONE) {
            return data;
        }
        return decompress(data, type, 0);
    }

    // Compression ratio calculation
    float calculate_compression_ratio(const std::vector<uint8_t>& original,
                                     const std::vector<uint8_t>& compressed) {
        if (original.empty()) {
            return 1.0f;
        }
        return static_cast<float>(compressed.size()) / static_cast<float>(original.size());
    }

    // Best compression selection
    CompressionType select_best_compression(const std::vector<uint8_t>& data,
                                           float* best_ratio) {
        if (data.empty()) {
            if (best_ratio) *best_ratio = 1.0f;
            return CompressionType::NONE;
        }

        std::vector<uint8_t> lz4_compressed = compress_lz4(data);
        std::vector<uint8_t> zstd_compressed = compress_zstd(data, 3);

        float lz4_ratio = calculate_compression_ratio(data, lz4_compressed);
        float zstd_ratio = calculate_compression_ratio(data, zstd_compressed);

        if (lz4_ratio < zstd_ratio) {
            if (best_ratio) *best_ratio = lz4_ratio;
            return CompressionType::LZ4;
        } else {
            if (best_ratio) *best_ratio = zstd_ratio;
            return CompressionType::ZSTD;
        }
    }

} // namespace ecscope::assets::compression