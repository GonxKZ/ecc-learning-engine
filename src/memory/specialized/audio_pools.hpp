#pragma once

/**
 * @file memory/specialized/audio_pools.hpp
 * @brief Specialized Audio Memory Pools for Real-Time Audio Processing
 * 
 * This header implements specialized memory pools optimized for audio processing
 * workloads with real-time constraints. It provides educational insights into
 * audio memory management patterns and integration with existing infrastructure.
 * 
 * Key Features:
 * - Lock-free audio buffer pools for real-time threads
 * - Specialized pools for different audio formats (PCM, compressed, etc.)
 * - Ring buffer management for streaming audio data
 * - SIMD-aligned memory layouts for audio processing
 * - Real-time safe memory allocation patterns
 * - Audio format-aware memory optimization
 * - Cross-platform audio memory management
 * - Educational audio processing insights
 * 
 * Educational Value:
 * - Demonstrates real-time audio memory constraints and solutions
 * - Shows lock-free programming techniques for audio threads
 * - Illustrates audio buffer management and streaming patterns
 * - Provides performance analysis for different audio formats
 * - Educational examples of SIMD optimization in audio processing
 * 
 * @author ECScope Educational ECS Framework - Audio Memory Management
 * @date 2025
 */

#include "memory/pools/hierarchical_pools.hpp"
#include "memory/allocators/lockfree_allocators.hpp"
#include "memory/memory_tracker.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <array>
#include <span>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace ecscope::memory::specialized::audio {

//=============================================================================
// Audio Format and Property Definitions
//=============================================================================

/**
 * @brief Audio sample formats supported by the pool system
 */
enum class AudioFormat : u8 {
    Unknown     = 0,
    PCM_8       = 1,  // 8-bit PCM
    PCM_16      = 2,  // 16-bit PCM (CD quality)
    PCM_24      = 3,  // 24-bit PCM (studio quality)
    PCM_32      = 4,  // 32-bit PCM
    Float32     = 5,  // 32-bit float
    Float64     = 6,  // 64-bit float (double precision)
    Compressed  = 7,  // Compressed audio (MP3, OGG, etc.)
    COUNT       = 8
};

/**
 * @brief Audio buffer usage patterns for optimization
 */
enum class AudioBufferUsage : u8 {
    Playback          = 0,  // Audio playback buffers
    Recording         = 1,  // Audio recording buffers
    Processing        = 2,  // Audio processing intermediate buffers
    Mixing            = 3,  // Audio mixing buffers
    Effects           = 4,  // Audio effects processing buffers
    Streaming         = 5,  // Streaming audio buffers
    Synthesis         = 6,  // Audio synthesis buffers
    Analysis          = 7,  // Audio analysis buffers (FFT, etc.)
    COUNT             = 8
};

/**
 * @brief Real-time audio constraints and properties
 */
struct AudioRealtimeProperties {
    f32 sample_rate;           // Sample rate in Hz (e.g., 44100, 48000)
    u32 channels;              // Number of audio channels
    u32 buffer_size_samples;   // Buffer size in samples
    u32 buffer_size_bytes;     // Buffer size in bytes
    f64 buffer_duration_ms;    // Buffer duration in milliseconds
    
    bool is_realtime_thread;   // Running on real-time thread
    bool requires_lock_free;   // Must use lock-free allocation
    bool requires_simd_align;  // Requires SIMD alignment
    u32 simd_alignment;        // Required SIMD alignment (16, 32, 64 bytes)
    
    f32 max_latency_ms;        // Maximum acceptable latency
    f32 target_latency_ms;     // Target latency for optimal performance
    
    AudioRealtimeProperties() noexcept {
        sample_rate = 48000.0f;
        channels = 2;
        buffer_size_samples = 512;
        buffer_size_bytes = buffer_size_samples * channels * sizeof(f32);
        buffer_duration_ms = (buffer_size_samples / sample_rate) * 1000.0;
        is_realtime_thread = false;
        requires_lock_free = false;
        requires_simd_align = true;
        simd_alignment = 32; // AVX alignment
        max_latency_ms = 20.0f;
        target_latency_ms = 10.0f;
    }
    
    void update_derived_properties() {
        buffer_size_bytes = buffer_size_samples * channels * get_format_size(AudioFormat::Float32);
        buffer_duration_ms = (static_cast<f64>(buffer_size_samples) / sample_rate) * 1000.0;
    }
    
private:
    static usize get_format_size(AudioFormat format) {
        switch (format) {
            case AudioFormat::PCM_8: return 1;
            case AudioFormat::PCM_16: return 2;
            case AudioFormat::PCM_24: return 3;
            case AudioFormat::PCM_32: return 4;
            case AudioFormat::Float32: return 4;
            case AudioFormat::Float64: return 8;
            default: return 4;
        }
    }
};

/**
 * @brief Audio buffer pool properties combining format and real-time requirements
 */
struct AudioPoolProperties {
    AudioFormat format;
    AudioBufferUsage usage;
    AudioRealtimeProperties realtime_props;
    
    // Pool-specific properties
    usize initial_buffer_count;     // Initial number of buffers in pool
    usize max_buffer_count;         // Maximum buffers in pool
    usize buffer_growth_factor;     // How many buffers to add when expanding
    f64 buffer_lifetime_estimate;   // Expected buffer lifetime in seconds
    
    // Performance properties
    bool enable_prefetch;           // Enable CPU cache prefetching
    bool enable_temporal_locality;  // Optimize for temporal locality
    bool enable_zero_copy;          // Enable zero-copy optimizations
    
    AudioPoolProperties(AudioFormat fmt = AudioFormat::Float32, 
                       AudioBufferUsage usage_type = AudioBufferUsage::Playback) 
        : format(fmt), usage(usage_type) {
        
        initial_buffer_count = 16;
        max_buffer_count = 256;
        buffer_growth_factor = 8;
        buffer_lifetime_estimate = 0.1; // 100ms typical
        enable_prefetch = true;
        enable_temporal_locality = true;
        enable_zero_copy = false;
        
        // Adjust properties based on usage
        adjust_for_usage_pattern();
    }
    
private:
    void adjust_for_usage_pattern() {
        switch (usage) {
            case AudioBufferUsage::Playback:
                realtime_props.requires_lock_free = true;
                realtime_props.target_latency_ms = 10.0f;
                buffer_lifetime_estimate = 0.05; // 50ms for playback
                break;
                
            case AudioBufferUsage::Recording:
                realtime_props.requires_lock_free = true;
                realtime_props.target_latency_ms = 5.0f;
                buffer_lifetime_estimate = 0.02; // 20ms for recording
                break;
                
            case AudioBufferUsage::Processing:
                realtime_props.requires_simd_align = true;
                enable_prefetch = true;
                buffer_lifetime_estimate = 0.001; // 1ms for processing
                break;
                
            case AudioBufferUsage::Mixing:
                realtime_props.requires_simd_align = true;
                realtime_props.simd_alignment = 64; // AVX-512
                buffer_lifetime_estimate = 0.02; // 20ms for mixing
                break;
                
            case AudioBufferUsage::Effects:
                realtime_props.requires_simd_align = true;
                enable_temporal_locality = true;
                buffer_lifetime_estimate = 0.1; // 100ms for effects
                break;
                
            case AudioBufferUsage::Streaming:
                max_buffer_count = 64; // Limit for streaming
                buffer_lifetime_estimate = 1.0; // 1 second for streaming
                enable_zero_copy = true;
                break;
                
            case AudioBufferUsage::Synthesis:
                realtime_props.requires_lock_free = true;
                realtime_props.requires_simd_align = true;
                buffer_lifetime_estimate = 0.02; // 20ms for synthesis
                break;
                
            case AudioBufferUsage::Analysis:
                realtime_props.requires_simd_align = true;
                realtime_props.simd_alignment = 64; // FFT optimization
                buffer_lifetime_estimate = 0.5; // 500ms for analysis
                break;
        }
    }
};

//=============================================================================
// Lock-Free Audio Buffer Queue
//=============================================================================

/**
 * @brief Lock-free queue for audio buffers in real-time contexts
 */
template<typename T, usize Capacity = 256>
class AudioBufferQueue {
private:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    
    struct Buffer {
        alignas(64) T* data;                    // Buffer data pointer
        usize size_bytes;                       // Buffer size in bytes
        usize size_samples;                     // Buffer size in samples
        u32 channels;                           // Number of channels
        f32 sample_rate;                        // Sample rate
        std::atomic<u32> ref_count{0};         // Reference count for safety
        f64 timestamp;                          // Buffer timestamp
        u32 sequence_id;                        // Sequence ID for ordering
        
        Buffer() noexcept : data(nullptr), size_bytes(0), size_samples(0), 
                           channels(0), sample_rate(0.0f), timestamp(0.0), sequence_id(0) {}
    };
    
    alignas(core::CACHE_LINE_SIZE) std::atomic<usize> head_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<usize> tail_{0};
    alignas(core::CACHE_LINE_SIZE) std::array<Buffer, Capacity> buffers_;
    
    static constexpr usize MASK = Capacity - 1;
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> enqueue_count_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> dequeue_count_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> full_queue_count_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> empty_queue_count_{0};
    
public:
    AudioBufferQueue() = default;
    
    /**
     * @brief Enqueue audio buffer (producer side - lock-free)
     */
    bool enqueue(T* buffer_data, usize size_bytes, usize size_samples, 
                 u32 channels, f32 sample_rate, u32 sequence_id = 0) noexcept {
        
        const usize current_tail = tail_.load(std::memory_order_relaxed);
        const usize next_tail = (current_tail + 1) & MASK;
        
        // Check if queue is full
        if (next_tail == head_.load(std::memory_order_acquire)) {
            full_queue_count_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        
        // Fill buffer entry
        Buffer& entry = buffers_[current_tail];
        entry.data = buffer_data;
        entry.size_bytes = size_bytes;
        entry.size_samples = size_samples;
        entry.channels = channels;
        entry.sample_rate = sample_rate;
        entry.sequence_id = sequence_id;
        entry.timestamp = get_current_time();
        entry.ref_count.store(1, std::memory_order_relaxed);
        
        // Commit the entry
        tail_.store(next_tail, std::memory_order_release);
        enqueue_count_.fetch_add(1, std::memory_order_relaxed);
        
        return true;
    }
    
    /**
     * @brief Dequeue audio buffer (consumer side - lock-free)
     */
    std::optional<Buffer> dequeue() noexcept {
        const usize current_head = head_.load(std::memory_order_relaxed);
        
        // Check if queue is empty
        if (current_head == tail_.load(std::memory_order_acquire)) {
            empty_queue_count_.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }
        
        // Get buffer entry
        Buffer entry = buffers_[current_head];
        
        // Advance head
        head_.store((current_head + 1) & MASK, std::memory_order_release);
        dequeue_count_.fetch_add(1, std::memory_order_relaxed);
        
        return entry;
    }
    
    /**
     * @brief Check if queue is empty (approximate)
     */
    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Check if queue is full (approximate)
     */
    bool full() const noexcept {
        const usize current_tail = tail_.load(std::memory_order_acquire);
        const usize next_tail = (current_tail + 1) & MASK;
        return next_tail == head_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get current queue size (approximate)
     */
    usize size() const noexcept {
        const usize current_head = head_.load(std::memory_order_acquire);
        const usize current_tail = tail_.load(std::memory_order_acquire);
        return (current_tail - current_head) & MASK;
    }
    
    /**
     * @brief Get queue statistics
     */
    struct QueueStatistics {
        u64 enqueue_count;
        u64 dequeue_count;
        u64 full_queue_count;
        u64 empty_queue_count;
        usize current_size;
        f64 utilization_ratio;
        f64 throughput_buffers_per_second;
    };
    
    QueueStatistics get_statistics() const noexcept {
        QueueStatistics stats{};
        
        stats.enqueue_count = enqueue_count_.load(std::memory_order_relaxed);
        stats.dequeue_count = dequeue_count_.load(std::memory_order_relaxed);
        stats.full_queue_count = full_queue_count_.load(std::memory_order_relaxed);
        stats.empty_queue_count = empty_queue_count_.load(std::memory_order_relaxed);
        stats.current_size = size();
        stats.utilization_ratio = static_cast<f64>(stats.current_size) / Capacity;
        
        // Calculate throughput (simplified)
        stats.throughput_buffers_per_second = static_cast<f64>(stats.dequeue_count);
        
        return stats;
    }
    
private:
    f64 get_current_time() const noexcept {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// SIMD-Optimized Audio Buffer Pool
//=============================================================================

/**
 * @brief Pool allocator optimized for SIMD audio processing
 */
template<typename SampleType = f32>
class SIMDAudioBufferPool {
private:
    struct AudioBuffer {
        alignas(64) SampleType* data;          // SIMD-aligned audio data
        usize size_samples;                     // Size in samples
        usize size_bytes;                       // Size in bytes
        u32 channels;                           // Channel count
        f32 sample_rate;                        // Sample rate
        AudioFormat format;                     // Audio format
        
        // Memory management
        void* raw_allocation;                   // Original allocation pointer
        usize raw_size;                         // Original allocation size
        bool is_available;                      // Available for allocation
        
        // Performance tracking
        f64 allocation_time;                    // When allocated
        f64 last_access_time;                   // Last access time
        u32 access_count;                       // Access counter
        
        AudioBuffer() noexcept : data(nullptr), size_samples(0), size_bytes(0),
                               channels(0), sample_rate(0.0f), format(AudioFormat::Unknown),
                               raw_allocation(nullptr), raw_size(0), is_available(true),
                               allocation_time(0.0), last_access_time(0.0), access_count(0) {}
    };
    
    // Pool management
    std::vector<std::unique_ptr<AudioBuffer>> buffer_pool_;
    mutable std::mutex pool_mutex_;  // Simple mutex for non-realtime operations
    
    // Lock-free free list for real-time allocation
    AudioBufferQueue<AudioBuffer*, 1024> free_buffers_;
    AudioBufferQueue<AudioBuffer*, 1024> used_buffers_;
    
    // Pool properties
    AudioPoolProperties properties_;
    usize buffer_count_;
    usize total_memory_allocated_;
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> allocations_count_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> deallocations_count_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> realtime_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> failed_allocations_{0};
    
    // SIMD optimization support
    bool cpu_supports_sse_;
    bool cpu_supports_avx_;
    bool cpu_supports_avx512_;
    
public:
    explicit SIMDAudioBufferPool(const AudioPoolProperties& props) 
        : properties_(props), buffer_count_(0), total_memory_allocated_(0) {
        
        detect_simd_capabilities();
        initialize_buffer_pool();
        
        LOG_INFO("Initialized SIMD audio buffer pool: format={}, usage={}, buffers={}", 
                 static_cast<u32>(props.format), static_cast<u32>(props.usage), 
                 props.initial_buffer_count);
    }
    
    ~SIMDAudioBufferPool() {
        cleanup_buffer_pool();
        
        LOG_DEBUG("SIMD audio buffer pool destroyed: total_allocations={}, peak_buffers={}", 
                 allocations_count_.load(), buffer_count_);
    }
    
    /**
     * @brief Allocate audio buffer with SIMD alignment
     */
    SampleType* allocate_buffer(usize size_samples, u32 channels, f32 sample_rate) {
        if (size_samples == 0 || channels == 0) return nullptr;
        
        allocations_count_.fetch_add(1, std::memory_order_relaxed);
        
        // Try lock-free allocation first for real-time threads
        if (properties_.realtime_props.requires_lock_free) {
            realtime_allocations_.fetch_add(1, std::memory_order_relaxed);
            return allocate_lockfree(size_samples, channels, sample_rate);
        }
        
        // Fall back to mutex-protected allocation
        return allocate_with_mutex(size_samples, channels, sample_rate);
    }
    
    /**
     * @brief Deallocate audio buffer
     */
    void deallocate_buffer(SampleType* buffer) {
        if (!buffer) return;
        
        deallocations_count_.fetch_add(1, std::memory_order_relaxed);
        
        // Find buffer in our pool
        AudioBuffer* audio_buf = find_buffer_by_data(buffer);
        if (!audio_buf) {
            LOG_WARNING("Attempted to deallocate unknown audio buffer");
            return;
        }
        
        audio_buf->is_available = true;
        audio_buf->last_access_time = get_current_time();
        
        // Return to free list for lock-free reuse
        if (properties_.realtime_props.requires_lock_free) {
            bool success = free_buffers_.enqueue(audio_buf, audio_buf->size_bytes, 
                                               audio_buf->size_samples, audio_buf->channels,
                                               audio_buf->sample_rate);
            if (!success) {
                LOG_WARNING("Failed to return audio buffer to lock-free pool");
            }
        }
    }
    
    /**
     * @brief Zero-copy buffer acquisition for streaming
     */
    SampleType* acquire_zero_copy_buffer(usize size_samples, u32 channels, f32 sample_rate) {
        if (!properties_.enable_zero_copy) {
            return allocate_buffer(size_samples, channels, sample_rate);
        }
        
        // Implementation would provide direct access to pre-allocated streaming buffers
        return allocate_buffer(size_samples, channels, sample_rate); // Simplified
    }
    
    /**
     * @brief Prefetch buffer data for CPU cache optimization
     */
    void prefetch_buffer(SampleType* buffer, usize size_samples) const {
        if (!properties_.enable_prefetch || !buffer) return;
        
        usize size_bytes = size_samples * sizeof(SampleType);
        const char* data = reinterpret_cast<const char*>(buffer);
        
        // Prefetch in cache line chunks
        for (usize offset = 0; offset < size_bytes; offset += core::CACHE_LINE_SIZE) {
            __builtin_prefetch(data + offset, 0, 3); // Read prefetch, high locality
        }
    }
    
    /**
     * @brief Fill buffer with silence (optimized for SIMD)
     */
    void fill_silence(SampleType* buffer, usize size_samples) const {
        if (!buffer || size_samples == 0) return;
        
        // Use SIMD-optimized memset for larger buffers
        if (size_samples * sizeof(SampleType) >= 1024 && cpu_supports_avx_) {
            simd_fill_silence_avx(buffer, size_samples);
        } else if (size_samples * sizeof(SampleType) >= 256 && cpu_supports_sse_) {
            simd_fill_silence_sse(buffer, size_samples);
        } else {
            std::fill_n(buffer, size_samples, static_cast<SampleType>(0));
        }
    }
    
    /**
     * @brief Get comprehensive pool statistics
     */
    struct PoolStatistics {
        // Pool metrics
        usize total_buffers;
        usize available_buffers;
        usize used_buffers;
        usize total_memory_bytes;
        f64 utilization_ratio;
        
        // Performance metrics
        u64 total_allocations;
        u64 total_deallocations;
        u64 realtime_allocations;
        u64 failed_allocations;
        f64 allocation_success_rate;
        
        // SIMD capabilities
        bool sse_supported;
        bool avx_supported;
        bool avx512_supported;
        usize optimal_alignment;
        
        // Queue statistics
        typename AudioBufferQueue<AudioBuffer*>::QueueStatistics free_queue_stats;
        typename AudioBufferQueue<AudioBuffer*>::QueueStatistics used_queue_stats;
        
        // Performance analysis
        f64 average_buffer_lifetime;
        f64 cache_efficiency_estimate;
        std::vector<std::pair<usize, u32>> size_distribution; // size -> count
    };
    
    PoolStatistics get_statistics() const {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        PoolStatistics stats{};
        
        // Basic pool metrics
        stats.total_buffers = buffer_pool_.size();
        stats.total_memory_bytes = total_memory_allocated_;
        
        usize available_count = 0;
        f64 total_lifetime = 0.0;
        std::unordered_map<usize, u32> size_counts;
        
        for (const auto& buf : buffer_pool_) {
            if (buf->is_available) available_count++;
            
            if (buf->allocation_time > 0.0) {
                f64 lifetime = get_current_time() - buf->allocation_time;
                total_lifetime += lifetime;
            }
            
            size_counts[buf->size_samples]++;
        }
        
        stats.available_buffers = available_count;
        stats.used_buffers = stats.total_buffers - available_count;
        
        if (stats.total_buffers > 0) {
            stats.utilization_ratio = static_cast<f64>(stats.used_buffers) / stats.total_buffers;
            stats.average_buffer_lifetime = total_lifetime / stats.total_buffers;
        }
        
        // Performance metrics
        stats.total_allocations = allocations_count_.load(std::memory_order_relaxed);
        stats.total_deallocations = deallocations_count_.load(std::memory_order_relaxed);
        stats.realtime_allocations = realtime_allocations_.load(std::memory_order_relaxed);
        stats.failed_allocations = failed_allocations_.load(std::memory_order_relaxed);
        
        u64 total_attempts = stats.total_allocations + stats.failed_allocations;
        if (total_attempts > 0) {
            stats.allocation_success_rate = static_cast<f64>(stats.total_allocations) / total_attempts;
        }
        
        // SIMD capabilities
        stats.sse_supported = cpu_supports_sse_;
        stats.avx_supported = cpu_supports_avx_;
        stats.avx512_supported = cpu_supports_avx512_;
        stats.optimal_alignment = properties_.realtime_props.simd_alignment;
        
        // Queue statistics
        stats.free_queue_stats = free_buffers_.get_statistics();
        stats.used_queue_stats = used_buffers_.get_statistics();
        
        // Size distribution
        for (const auto& [size, count] : size_counts) {
            stats.size_distribution.emplace_back(size, count);
        }
        
        // Cache efficiency estimate (simplified heuristic)
        stats.cache_efficiency_estimate = std::min(1.0, stats.utilization_ratio * 1.2);
        
        return stats;
    }
    
    const AudioPoolProperties& get_properties() const { return properties_; }
    
    /**
     * @brief Educational performance analysis
     */
    struct PerformanceAnalysis {
        std::string performance_summary;
        std::vector<std::string> optimization_suggestions;
        std::vector<std::string> educational_insights;
        f64 realtime_safety_score;
        f64 simd_utilization_score;
    };
    
    PerformanceAnalysis analyze_performance() const {
        auto stats = get_statistics();
        PerformanceAnalysis analysis{};
        
        // Performance summary
        if (stats.allocation_success_rate > 0.95) {
            analysis.performance_summary = "Excellent - Low allocation failures";
        } else if (stats.allocation_success_rate > 0.90) {
            analysis.performance_summary = "Good - Some allocation pressure";
        } else {
            analysis.performance_summary = "Poor - High allocation failure rate";
        }
        
        // Real-time safety score
        f64 rt_ratio = static_cast<f64>(stats.realtime_allocations) / stats.total_allocations;
        analysis.realtime_safety_score = rt_ratio * stats.allocation_success_rate;
        
        // SIMD utilization score
        analysis.simd_utilization_score = 0.5; // Simplified
        if (stats.avx512_supported && properties_.realtime_props.simd_alignment >= 64) {
            analysis.simd_utilization_score = 1.0;
        } else if (stats.avx_supported && properties_.realtime_props.simd_alignment >= 32) {
            analysis.simd_utilization_score = 0.8;
        } else if (stats.sse_supported && properties_.realtime_props.simd_alignment >= 16) {
            analysis.simd_utilization_score = 0.6;
        }
        
        // Optimization suggestions
        if (stats.utilization_ratio < 0.3) {
            analysis.optimization_suggestions.push_back("Consider reducing initial pool size");
        }
        if (stats.failed_allocations > stats.total_allocations * 0.05) {
            analysis.optimization_suggestions.push_back("Increase pool size to reduce allocation failures");
        }
        if (!properties_.realtime_props.requires_lock_free && rt_ratio > 0.8) {
            analysis.optimization_suggestions.push_back("Enable lock-free mode for better real-time performance");
        }
        
        // Educational insights
        analysis.educational_insights.push_back("Audio buffers require careful memory alignment for SIMD optimization");
        analysis.educational_insights.push_back("Real-time audio threads must avoid blocking memory allocations");
        analysis.educational_insights.push_back("Buffer pooling reduces garbage collection pressure in audio applications");
        
        return analysis;
    }
    
private:
    void detect_simd_capabilities() {
        // Simplified SIMD detection - real implementation would use CPUID
        cpu_supports_sse_ = true;    // Assume SSE is always available
        cpu_supports_avx_ = true;    // Simplified assumption
        cpu_supports_avx512_ = false; // Conservative assumption
        
        LOG_DEBUG("SIMD capabilities: SSE={}, AVX={}, AVX512={}", 
                 cpu_supports_sse_, cpu_supports_avx_, cpu_supports_avx512_);
    }
    
    void initialize_buffer_pool() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        buffer_pool_.reserve(properties_.initial_buffer_count);
        
        for (usize i = 0; i < properties_.initial_buffer_count; ++i) {
            create_new_buffer();
        }
        
        LOG_DEBUG("Initialized {} audio buffers, total memory: {}KB", 
                 buffer_pool_.size(), total_memory_allocated_ / 1024);
    }
    
    void create_new_buffer() {
        auto buffer = std::make_unique<AudioBuffer>();
        
        // Calculate buffer size based on properties
        usize samples_per_buffer = properties_.realtime_props.buffer_size_samples;
        u32 channels = properties_.realtime_props.channels;
        usize sample_size = get_sample_size(properties_.format);
        usize buffer_size_bytes = samples_per_buffer * channels * sample_size;
        
        // Add alignment padding
        usize alignment = properties_.realtime_props.simd_alignment;
        usize aligned_size = buffer_size_bytes + alignment - 1;
        
        // Allocate raw memory
        buffer->raw_allocation = std::aligned_alloc(alignment, aligned_size);
        if (!buffer->raw_allocation) {
            LOG_ERROR("Failed to allocate aligned audio buffer");
            return;
        }
        
        buffer->raw_size = aligned_size;
        buffer->data = static_cast<SampleType*>(buffer->raw_allocation);
        buffer->size_samples = samples_per_buffer * channels;
        buffer->size_bytes = buffer_size_bytes;
        buffer->channels = channels;
        buffer->sample_rate = properties_.realtime_props.sample_rate;
        buffer->format = properties_.format;
        buffer->is_available = true;
        buffer->allocation_time = get_current_time();
        
        // Initialize with silence
        fill_silence(buffer->data, buffer->size_samples);
        
        // Add to free queue for lock-free access
        free_buffers_.enqueue(buffer.get(), buffer->size_bytes, buffer->size_samples,
                            buffer->channels, buffer->sample_rate);
        
        total_memory_allocated_ += aligned_size;
        buffer_pool_.push_back(std::move(buffer));
        buffer_count_++;
    }
    
    void cleanup_buffer_pool() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        for (auto& buffer : buffer_pool_) {
            if (buffer->raw_allocation) {
                std::free(buffer->raw_allocation);
            }
        }
        
        buffer_pool_.clear();
        total_memory_allocated_ = 0;
        buffer_count_ = 0;
    }
    
    SampleType* allocate_lockfree(usize size_samples, u32 channels, f32 sample_rate) {
        auto buffer_opt = free_buffers_.dequeue();
        if (!buffer_opt.has_value()) {
            failed_allocations_.fetch_add(1, std::memory_order_relaxed);
            return nullptr;
        }
        
        AudioBuffer* buffer = buffer_opt->data;
        buffer->is_available = false;
        buffer->last_access_time = get_current_time();
        buffer->access_count++;
        
        return buffer->data;
    }
    
    SampleType* allocate_with_mutex(usize size_samples, u32 channels, f32 sample_rate) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        // Find available buffer
        for (auto& buffer : buffer_pool_) {
            if (buffer->is_available && buffer->size_samples >= size_samples * channels) {
                buffer->is_available = false;
                buffer->last_access_time = get_current_time();
                buffer->access_count++;
                return buffer->data;
            }
        }
        
        // No available buffer, try to create new one
        if (buffer_count_ < properties_.max_buffer_count) {
            create_new_buffer();
            
            // Try allocation again
            for (auto& buffer : buffer_pool_) {
                if (buffer->is_available && buffer->size_samples >= size_samples * channels) {
                    buffer->is_available = false;
                    buffer->last_access_time = get_current_time();
                    buffer->access_count++;
                    return buffer->data;
                }
            }
        }
        
        failed_allocations_.fetch_add(1, std::memory_order_relaxed);
        return nullptr;
    }
    
    AudioBuffer* find_buffer_by_data(SampleType* data) const {
        for (const auto& buffer : buffer_pool_) {
            if (buffer->data == data) {
                return buffer.get();
            }
        }
        return nullptr;
    }
    
    void simd_fill_silence_sse(SampleType* buffer, usize size_samples) const {
        // SSE implementation for filling silence - simplified
        std::fill_n(buffer, size_samples, static_cast<SampleType>(0));
    }
    
    void simd_fill_silence_avx(SampleType* buffer, usize size_samples) const {
        // AVX implementation for filling silence - simplified
        std::fill_n(buffer, size_samples, static_cast<SampleType>(0));
    }
    
    usize get_sample_size(AudioFormat format) const {
        switch (format) {
            case AudioFormat::PCM_8: return 1;
            case AudioFormat::PCM_16: return 2;
            case AudioFormat::PCM_24: return 3;
            case AudioFormat::PCM_32: return 4;
            case AudioFormat::Float32: return 4;
            case AudioFormat::Float64: return 8;
            default: return 4;
        }
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Audio Pool Manager
//=============================================================================

/**
 * @brief Manager for multiple audio pools with different characteristics
 */
class AudioPoolManager {
private:
    using AudioPool = SIMDAudioBufferPool<f32>;
    
    struct PoolEntry {
        std::unique_ptr<AudioPool> pool;
        AudioPoolProperties properties;
        f64 creation_time;
        std::atomic<u64> allocation_requests{0};
        std::atomic<f64> average_utilization{0.0};
    };
    
    // Pool storage indexed by [format][usage]
    std::array<std::array<std::unique_ptr<PoolEntry>, 
                         static_cast<usize>(AudioBufferUsage::COUNT)>,
               static_cast<usize>(AudioFormat::COUNT)> pools_;
    mutable std::shared_mutex pools_mutex_;
    
    // Global statistics
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<usize> total_memory_managed_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u32> active_pool_count_{0};
    
    // Memory tracking integration
    memory::MemoryTracker* memory_tracker_;
    
public:
    explicit AudioPoolManager(memory::MemoryTracker* tracker = nullptr) 
        : memory_tracker_(tracker) {
        
        LOG_INFO("Initialized audio pool manager for {} formats and {} usage types",
                 static_cast<usize>(AudioFormat::COUNT), static_cast<usize>(AudioBufferUsage::COUNT));
    }
    
    /**
     * @brief Allocate audio buffer with specific format and usage
     */
    f32* allocate_audio_buffer(AudioFormat format, AudioBufferUsage usage,
                              usize size_samples, u32 channels, f32 sample_rate) {
        
        auto& pool = get_or_create_pool(format, usage);
        if (!pool) {
            LOG_ERROR("Failed to get audio pool for format={}, usage={}", 
                     static_cast<u32>(format), static_cast<u32>(usage));
            return nullptr;
        }
        
        pool->allocation_requests.fetch_add(1, std::memory_order_relaxed);
        total_allocations_.fetch_add(1, std::memory_order_relaxed);
        
        f32* buffer = pool->pool->allocate_buffer(size_samples, channels, sample_rate);
        
        if (buffer && memory_tracker_) {
            usize size_bytes = size_samples * channels * sizeof(f32);
            memory_tracker_->track_allocation(
                buffer, size_bytes, size_bytes, 32, // Assume 32-byte alignment
                memory::AllocationCategory::Audio_Buffers,
                memory::AllocatorType::Custom,
                "AudioPool",
                static_cast<u32>(format) * 100 + static_cast<u32>(usage)
            );
        }
        
        return buffer;
    }
    
    /**
     * @brief Deallocate audio buffer
     */
    void deallocate_audio_buffer(f32* buffer) {
        if (!buffer) return;
        
        // Find which pool owns this buffer
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        
        for (auto& format_pools : pools_) {
            for (auto& pool_entry : format_pools) {
                if (pool_entry && pool_entry->pool) {
                    // Simple ownership check - in real implementation would be more sophisticated
                    pool_entry->pool->deallocate_buffer(buffer);
                    
                    if (memory_tracker_) {
                        memory_tracker_->track_deallocation(
                            buffer, memory::AllocatorType::Custom,
                            "AudioPool", 0
                        );
                    }
                    
                    return;
                }
            }
        }
        
        LOG_WARNING("Attempted to deallocate unknown audio buffer");
    }
    
    /**
     * @brief Get comprehensive audio pool statistics
     */
    struct AudioManagerStatistics {
        struct FormatUsageStats {
            AudioFormat format;
            AudioBufferUsage usage;
            typename AudioPool::PoolStatistics pool_stats;
            u64 allocation_requests;
            f64 average_utilization;
            bool is_active;
        };
        
        std::vector<FormatUsageStats> pool_statistics;
        u64 total_allocations;
        usize total_memory_managed;
        u32 active_pool_count;
        f64 overall_efficiency_score;
        
        // Educational insights
        AudioFormat most_used_format;
        AudioBufferUsage most_used_usage;
        std::string performance_summary;
    };
    
    AudioManagerStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        
        AudioManagerStatistics stats{};
        stats.total_allocations = total_allocations_.load();
        stats.total_memory_managed = total_memory_managed_.load();
        stats.active_pool_count = active_pool_count_.load();
        
        u64 max_requests = 0;
        AudioFormat most_used_fmt = AudioFormat::Float32;
        AudioBufferUsage most_used_usage_type = AudioBufferUsage::Playback;
        f64 total_efficiency = 0.0;
        u32 efficiency_count = 0;
        
        for (usize fmt = 0; fmt < static_cast<usize>(AudioFormat::COUNT); ++fmt) {
            for (usize usage = 0; usage < static_cast<usize>(AudioBufferUsage::COUNT); ++usage) {
                const auto& pool_entry = pools_[fmt][usage];
                if (!pool_entry) continue;
                
                AudioManagerStatistics::FormatUsageStats pool_stat{};
                pool_stat.format = static_cast<AudioFormat>(fmt);
                pool_stat.usage = static_cast<AudioBufferUsage>(usage);
                pool_stat.pool_stats = pool_entry->pool->get_statistics();
                pool_stat.allocation_requests = pool_entry->allocation_requests.load();
                pool_stat.average_utilization = pool_entry->average_utilization.load();
                pool_stat.is_active = pool_stat.allocation_requests > 0;
                
                if (pool_stat.allocation_requests > max_requests) {
                    max_requests = pool_stat.allocation_requests;
                    most_used_fmt = pool_stat.format;
                    most_used_usage_type = pool_stat.usage;
                }
                
                if (pool_stat.is_active) {
                    total_efficiency += pool_stat.pool_stats.allocation_success_rate;
                    efficiency_count++;
                }
                
                stats.pool_statistics.push_back(pool_stat);
            }
        }
        
        stats.most_used_format = most_used_fmt;
        stats.most_used_usage = most_used_usage_type;
        
        if (efficiency_count > 0) {
            stats.overall_efficiency_score = total_efficiency / efficiency_count;
        }
        
        // Performance summary
        if (stats.overall_efficiency_score > 0.95) {
            stats.performance_summary = "Excellent audio memory performance";
        } else if (stats.overall_efficiency_score > 0.85) {
            stats.performance_summary = "Good audio memory performance";
        } else {
            stats.performance_summary = "Audio memory performance needs optimization";
        }
        
        return stats;
    }
    
private:
    PoolEntry& get_or_create_pool(AudioFormat format, AudioBufferUsage usage) {
        usize fmt_idx = static_cast<usize>(format);
        usize usage_idx = static_cast<usize>(usage);
        
        if (fmt_idx >= static_cast<usize>(AudioFormat::COUNT) ||
            usage_idx >= static_cast<usize>(AudioBufferUsage::COUNT)) {
            fmt_idx = static_cast<usize>(AudioFormat::Float32);
            usage_idx = static_cast<usize>(AudioBufferUsage::Playback);
        }
        
        std::unique_lock<std::shared_mutex> lock(pools_mutex_);
        
        auto& pool_entry = pools_[fmt_idx][usage_idx];
        if (!pool_entry) {
            // Create new pool
            AudioPoolProperties props(format, usage);
            
            pool_entry = std::make_unique<PoolEntry>();
            pool_entry->properties = props;
            pool_entry->pool = std::make_unique<AudioPool>(props);
            pool_entry->creation_time = get_current_time();
            
            active_pool_count_.fetch_add(1, std::memory_order_relaxed);
            
            LOG_DEBUG("Created new audio pool: format={}, usage={}", 
                     static_cast<u32>(format), static_cast<u32>(usage));
        }
        
        return *pool_entry;
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Global Audio Pool Manager Instance
//=============================================================================

AudioPoolManager& get_global_audio_pool_manager();

} // namespace ecscope::memory::specialized::audio