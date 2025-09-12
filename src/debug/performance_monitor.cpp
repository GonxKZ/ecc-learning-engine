// Performance Monitor Implementation
#include "ecscope/debug/performance_monitor.hpp"
#include <algorithm>
#include <numeric>
#include <chrono>

namespace ecscope::debug {

PerformanceMonitor::PerformanceMonitor() {
    start_time_ = std::chrono::high_resolution_clock::now();
}

PerformanceMonitor::~PerformanceMonitor() = default;

void PerformanceMonitor::start_frame() {
    frame_start_time_ = std::chrono::high_resolution_clock::now();
    current_frame_metrics_ = FrameMetrics{};
    active_timers_.clear();
}

void PerformanceMonitor::end_frame() {
    auto frame_end_time = std::chrono::high_resolution_clock::now();
    auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_end_time - frame_start_time_);
    
    current_frame_metrics_.total_frame_time = frame_duration.count() / 1000.0; // Convert to milliseconds
    current_frame_metrics_.fps = (current_frame_metrics_.total_frame_time > 0) ? 1000.0 / current_frame_metrics_.total_frame_time : 0.0;
    
    // Store frame metrics
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        frame_history_.push_back(current_frame_metrics_);
        
        // Maintain history size
        if (frame_history_.size() > max_frame_history_) {
            frame_history_.erase(frame_history_.begin());
        }
        
        total_frames_++;
    }
    
    // Update running averages
    update_averages();
}

void PerformanceMonitor::begin_timer(const std::string& name) {
    auto& timer_data = active_timers_[name];
    timer_data.start_time = std::chrono::high_resolution_clock::now();
    timer_data.is_active = true;
}

void PerformanceMonitor::end_timer(const std::string& name) {
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto it = active_timers_.find(name);
    if (it != active_timers_.end() && it->second.is_active) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - it->second.start_time);
        double duration_ms = duration.count() / 1000.0;
        
        // Record the timing
        record_timing(name, duration_ms);
        
        it->second.is_active = false;
        it->second.total_time += duration_ms;
    }
}

void PerformanceMonitor::record_timing(const std::string& name, double duration_ms) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto& stats = timing_stats_[name];
    stats.total_time += duration_ms;
    stats.call_count++;
    stats.min_time = (stats.call_count == 1) ? duration_ms : std::min(stats.min_time, duration_ms);
    stats.max_time = std::max(stats.max_time, duration_ms);
    
    // Add to current frame metrics
    current_frame_metrics_.system_times[name] += duration_ms;
}

void PerformanceMonitor::increment_counter(const std::string& name, uint64_t value) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    counters_[name] += value;
    current_frame_metrics_.counters[name] += value;
}

void PerformanceMonitor::set_gauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    gauges_[name] = value;
    current_frame_metrics_.gauges[name] = value;
}

void PerformanceMonitor::record_memory_usage(const std::string& category, size_t bytes) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    memory_usage_[category] = bytes;
    current_frame_metrics_.memory_usage[category] = bytes;
}

PerformanceStats PerformanceMonitor::get_performance_stats() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    PerformanceStats stats;
    
    if (!frame_history_.empty()) {
        // Calculate frame time statistics
        std::vector<double> frame_times;
        double total_frame_time = 0.0;
        double total_fps = 0.0;
        
        for (const auto& frame : frame_history_) {
            frame_times.push_back(frame.total_frame_time);
            total_frame_time += frame.total_frame_time;
            total_fps += frame.fps;
        }
        
        stats.avg_frame_time = total_frame_time / frame_history_.size();
        stats.avg_fps = total_fps / frame_history_.size();
        
        // Min/Max frame times
        auto minmax = std::minmax_element(frame_times.begin(), frame_times.end());
        stats.min_frame_time = *minmax.first;
        stats.max_frame_time = *minmax.second;
        
        // Calculate percentiles
        std::sort(frame_times.begin(), frame_times.end());
        size_t p95_index = static_cast<size_t>(frame_times.size() * 0.95);
        size_t p99_index = static_cast<size_t>(frame_times.size() * 0.99);
        
        stats.p95_frame_time = frame_times[std::min(p95_index, frame_times.size() - 1)];
        stats.p99_frame_time = frame_times[std::min(p99_index, frame_times.size() - 1)];
        
        // Frame drops (frames taking longer than 16.67ms for 60fps)
        stats.frame_drops = std::count_if(frame_times.begin(), frame_times.end(), 
                                        [](double t) { return t > 16.67; });
    }
    
    stats.total_frames = total_frames_;
    
    // Runtime
    auto current_time = std::chrono::high_resolution_clock::now();
    auto runtime = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time_);
    stats.total_runtime_seconds = runtime.count();
    
    // System timings
    stats.system_timings = timing_stats_;
    
    // Counters and gauges
    stats.counters = counters_;
    stats.gauges = gauges_;
    
    // Memory usage
    stats.memory_usage = memory_usage_;
    
    return stats;
}

const FrameMetrics& PerformanceMonitor::get_current_frame_metrics() const {
    return current_frame_metrics_;
}

std::vector<FrameMetrics> PerformanceMonitor::get_frame_history(size_t count) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (count == 0 || count >= frame_history_.size()) {
        return frame_history_;
    }
    
    // Return last 'count' frames
    return std::vector<FrameMetrics>(frame_history_.end() - count, frame_history_.end());
}

void PerformanceMonitor::reset_stats() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    frame_history_.clear();
    timing_stats_.clear();
    counters_.clear();
    gauges_.clear();
    memory_usage_.clear();
    total_frames_ = 0;
    start_time_ = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::set_max_frame_history(size_t max_frames) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    max_frame_history_ = max_frames;
    
    // Trim current history if needed
    if (frame_history_.size() > max_frame_history_) {
        frame_history_.erase(frame_history_.begin(), frame_history_.end() - max_frame_history_);
    }
}

void PerformanceMonitor::update_averages() {
    if (frame_history_.size() < 2) return;
    
    const size_t window_size = std::min(static_cast<size_t>(60), frame_history_.size()); // 1 second at 60fps
    auto recent_frames = std::vector<FrameMetrics>(frame_history_.end() - window_size, frame_history_.end());
    
    // Update moving averages for important metrics
    double total_frame_time = 0.0;
    for (const auto& frame : recent_frames) {
        total_frame_time += frame.total_frame_time;
    }
    
    moving_avg_frame_time_ = total_frame_time / window_size;
    moving_avg_fps_ = (moving_avg_frame_time_ > 0) ? 1000.0 / moving_avg_frame_time_ : 0.0;
}

// Scoped Timer Implementation
ScopedTimer::ScopedTimer(PerformanceMonitor& monitor, const std::string& name) 
    : monitor_(monitor), name_(name) {
    monitor_.begin_timer(name_);
}

ScopedTimer::~ScopedTimer() {
    monitor_.end_timer(name_);
}

// Performance Profiler Implementation
PerformanceProfiler::PerformanceProfiler() = default;
PerformanceProfiler::~PerformanceProfiler() = default;

void PerformanceProfiler::start_profiling(const std::string& session_name) {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    current_session_ = session_name;
    is_profiling_ = true;
    session_start_time_ = std::chrono::high_resolution_clock::now();
    
    // Clear previous data
    profile_data_.clear();
}

void PerformanceProfiler::stop_profiling() {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    is_profiling_ = false;
}

void PerformanceProfiler::record_event(const ProfileEvent& event) {
    if (!is_profiling_) return;
    
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    profile_data_.push_back(event);
}

std::vector<ProfileEvent> PerformanceProfiler::get_profile_data() const {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    return profile_data_;
}

void PerformanceProfiler::save_profile(const std::string& filename) const {
    // In a real implementation, this would save to a file format like JSON or Chrome trace format
    // For now, just a placeholder
}

bool PerformanceProfiler::is_profiling() const {
    return is_profiling_;
}

const std::string& PerformanceProfiler::get_current_session() const {
    return current_session_;
}

} // namespace ecscope::debug