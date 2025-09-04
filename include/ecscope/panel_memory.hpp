#pragma once

#include "../overlay.hpp"
#include "core/types.hpp"
#include <vector>
#include <string>
#include <array>

namespace ecscope::ui {

class MemoryObserverPanel : public Panel {
private:
    // Memory tracking data
    struct MemorySnapshot {
        f64 timestamp;
        usize total_allocated;
        usize total_freed;
        usize current_usage;
        usize peak_usage;
        u32 allocation_count;
        u32 free_count;
    };
    
    // Circular buffer for history
    static constexpr usize HISTORY_SIZE = 300; // 5 minutes at 60fps
    std::array<MemorySnapshot, HISTORY_SIZE> memory_history_;
    usize history_head_{0};
    usize history_count_{0};
    
    // Display settings
    f32 update_frequency_{10.0f}; // Hz
    f64 last_update_time_{0.0};
    bool auto_scale_{true};
    f32 manual_scale_max_{100.0f}; // MB
    
    // View options
    bool show_current_stats_{true};
    bool show_allocation_graph_{true};
    bool show_allocator_breakdown_{true};
    bool show_memory_map_{false};
    
    // Analysis data
    f64 average_allocation_rate_{0.0}; // bytes/sec
    f64 peak_allocation_rate_{0.0};
    usize largest_allocation_{0};
    usize fragmentation_score_{0};
    
    // Color scheme for different memory types
    struct MemoryColors {
        float ecs_memory[4] = {0.2f, 0.8f, 0.2f, 1.0f};      // Green
        float system_memory[4] = {0.8f, 0.2f, 0.2f, 1.0f};   // Red  
        float cache_memory[4] = {0.2f, 0.2f, 0.8f, 1.0f};    // Blue
        float temp_memory[4] = {0.8f, 0.8f, 0.2f, 1.0f};     // Yellow
    } colors_;
    
public:
    MemoryObserverPanel();
    
    void render() override;
    void update(f64 delta_time) override;
    
    // Configuration
    void set_update_frequency(f32 hz) { update_frequency_ = std::clamp(hz, 1.0f, 60.0f); }
    f32 update_frequency() const { return update_frequency_; }
    
    void set_auto_scale(bool auto_scale) { auto_scale_ = auto_scale; }
    bool auto_scale() const { return auto_scale_; }
    
    // Memory tracking interface
    void record_allocation(usize size, const char* category = nullptr);
    void record_deallocation(usize size, const char* category = nullptr);
    
    // Statistics
    usize current_memory_usage() const;
    usize peak_memory_usage() const;
    f64 allocation_rate() const { return average_allocation_rate_; }
    
private:
    void update_memory_snapshot();
    void render_current_stats();
    void render_allocation_graph();
    void render_allocator_breakdown();
    void render_memory_map();
    void render_controls();
    
    void analyze_allocation_patterns();
    MemorySnapshot get_current_snapshot() const;
    
    // Graph utilities
    void render_memory_graph(const char* label, const float* data, usize count, 
                            float scale_min, float scale_max, const float* color);
    void render_pie_chart(const char* label, const std::vector<std::pair<std::string, usize>>& data);
    
    // Utility functions
    std::string format_memory_size(usize bytes) const;
    std::string format_rate(f64 bytes_per_sec) const;
    float* get_history_buffer(usize& out_count) const;
};

// Memory tracking utilities (can be used globally)
namespace memory_tracker {
    void initialize();
    void shutdown();
    
    // Hook into allocations (for debugging)
    void track_allocation(void* ptr, usize size, const char* category = nullptr);
    void track_deallocation(void* ptr, usize size, const char* category = nullptr);
    
    // Get current stats
    usize get_current_usage();
    usize get_peak_usage();
    u32 get_allocation_count();
    u32 get_free_count();
    
    // Memory debugging
    std::vector<std::pair<std::string, usize>> get_category_breakdown();
    bool detect_leaks();
    void dump_allocations(const std::string& filename);
}

} // namespace ecscope::ui