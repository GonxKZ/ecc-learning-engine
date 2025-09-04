#pragma once

#include "types.hpp"
#include <chrono>
#include <ratio>
#include <thread>

namespace ecscope::core {

// High-resolution time utilities for profiling and frame timing
class Time {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;
    
    // Time units
    using Nanoseconds = std::chrono::nanoseconds;
    using Microseconds = std::chrono::microseconds;
    using Milliseconds = std::chrono::milliseconds;
    using Seconds = std::chrono::seconds;
    
    // Current time
    static TimePoint now() noexcept {
        return Clock::now();
    }
    
    // Convert duration to different units
    template<typename Rep, typename Period>
    static f64 to_seconds(const std::chrono::duration<Rep, Period>& duration) noexcept {
        return std::chrono::duration<f64>(duration).count();
    }
    
    template<typename Rep, typename Period>
    static f64 to_milliseconds(const std::chrono::duration<Rep, Period>& duration) noexcept {
        return std::chrono::duration<f64, std::milli>(duration).count();
    }
    
    template<typename Rep, typename Period>
    static f64 to_microseconds(const std::chrono::duration<Rep, Period>& duration) noexcept {
        return std::chrono::duration<f64, std::micro>(duration).count();
    }
    
    template<typename Rep, typename Period>
    static u64 to_nanoseconds(const std::chrono::duration<Rep, Period>& duration) noexcept {
        return std::chrono::duration_cast<Nanoseconds>(duration).count();
    }
    
    // Helper to create durations
    static constexpr Duration from_seconds(f64 seconds) noexcept {
        return std::chrono::duration_cast<Duration>(std::chrono::duration<f64>(seconds));
    }
    
    static constexpr Duration from_milliseconds(f64 milliseconds) noexcept {
        return std::chrono::duration_cast<Duration>(std::chrono::duration<f64, std::milli>(milliseconds));
    }
    
    static constexpr Duration from_microseconds(f64 microseconds) noexcept {
        return std::chrono::duration_cast<Duration>(std::chrono::duration<f64, std::micro>(microseconds));
    }
};

// Delta time manager for frame timing
class DeltaTime {
private:
    Time::Duration delta_{Time::from_seconds(1.0 / 60.0)}; // Default to 60 FPS
    Time::TimePoint last_update_{Time::now()};
    f64 smooth_delta_{1.0 / 60.0};
    f64 smoothing_factor_{0.1}; // Lower = more smoothing
    
public:
    // Update delta time (call once per frame)
    void update() noexcept {
        auto current = Time::now();
        delta_ = current - last_update_;
        last_update_ = current;
        
        // Smooth delta to reduce jitter
        f64 raw_delta = Time::to_seconds(delta_);
        smooth_delta_ = smooth_delta_ * (1.0 - smoothing_factor_) + raw_delta * smoothing_factor_;
    }
    
    // Raw delta time (actual time since last frame)
    Time::Duration raw_delta() const noexcept { return delta_; }
    f64 raw_delta_seconds() const noexcept { return Time::to_seconds(delta_); }
    f64 raw_delta_milliseconds() const noexcept { return Time::to_milliseconds(delta_); }
    
    // Smoothed delta time (reduces jitter)
    f64 delta_seconds() const noexcept { return smooth_delta_; }
    f64 delta_milliseconds() const noexcept { return smooth_delta_ * 1000.0; }
    
    // Frame rate calculations
    f64 fps() const noexcept { 
        return smooth_delta_ > 0.0 ? 1.0 / smooth_delta_ : 0.0; 
    }
    
    f64 raw_fps() const noexcept {
        f64 raw = Time::to_seconds(delta_);
        return raw > 0.0 ? 1.0 / raw : 0.0;
    }
    
    // Configuration
    void set_smoothing_factor(f64 factor) noexcept {
        smoothing_factor_ = std::clamp(factor, 0.0, 1.0);
    }
    
    f64 smoothing_factor() const noexcept { return smoothing_factor_; }
};

// High-precision timer for profiling
class Timer {
private:
    Time::TimePoint start_time_;
    
public:
    Timer() noexcept : start_time_(Time::now()) {}
    
    // Start/restart timer
    void start() noexcept {
        start_time_ = Time::now();
    }
    
    void restart() noexcept {
        start();
    }
    
    // Get elapsed time since start
    Time::Duration elapsed() const noexcept {
        return Time::now() - start_time_;
    }
    
    f64 elapsed_seconds() const noexcept {
        return Time::to_seconds(elapsed());
    }
    
    f64 elapsed_milliseconds() const noexcept {
        return Time::to_milliseconds(elapsed());
    }
    
    f64 elapsed_microseconds() const noexcept {
        return Time::to_microseconds(elapsed());
    }
    
    u64 elapsed_nanoseconds() const noexcept {
        return Time::to_nanoseconds(elapsed());
    }
};

// RAII scope timer for automatic profiling
class ScopeTimer {
private:
    Timer timer_;
    f64* result_;
    
public:
    // Store result in milliseconds
    explicit ScopeTimer(f64* result_ms) noexcept : result_(result_ms) {}
    
    ~ScopeTimer() noexcept {
        if (result_) {
            *result_ = timer_.elapsed_milliseconds();
        }
    }
    
    // Non-copyable, movable
    ScopeTimer(const ScopeTimer&) = delete;
    ScopeTimer& operator=(const ScopeTimer&) = delete;
    
    ScopeTimer(ScopeTimer&& other) noexcept 
        : timer_(std::move(other.timer_)), result_(other.result_) {
        other.result_ = nullptr;
    }
    
    ScopeTimer& operator=(ScopeTimer&& other) noexcept {
        if (this != &other) {
            timer_ = std::move(other.timer_);
            result_ = other.result_;
            other.result_ = nullptr;
        }
        return *this;
    }
    
    // Get current elapsed time without stopping the timer
    f64 peek_milliseconds() const noexcept {
        return timer_.elapsed_milliseconds();
    }
};

// Frame rate limiter
class FrameLimiter {
private:
    Time::Duration target_frame_time_;
    Time::TimePoint last_frame_;
    
public:
    explicit FrameLimiter(f64 target_fps = 60.0) noexcept 
        : target_frame_time_(Time::from_seconds(1.0 / target_fps)),
          last_frame_(Time::now()) {}
    
    // Wait until it's time for the next frame
    void limit() noexcept {
        auto current = Time::now();
        auto elapsed = current - last_frame_;
        
        if (elapsed < target_frame_time_) {
            auto sleep_time = target_frame_time_ - elapsed;
            std::this_thread::sleep_for(sleep_time);
        }
        
        last_frame_ = Time::now();
    }
    
    // Set target frame rate
    void set_target_fps(f64 fps) noexcept {
        target_frame_time_ = Time::from_seconds(1.0 / fps);
    }
    
    f64 target_fps() const noexcept {
        return 1.0 / Time::to_seconds(target_frame_time_);
    }
    
    // Check if we should render a new frame
    bool should_update() const noexcept {
        auto elapsed = Time::now() - last_frame_;
        return elapsed >= target_frame_time_;
    }
};

// Global delta time instance
DeltaTime& delta_time() noexcept;

// Convenience macros for timing
#define TIME_SCOPE(var) ScopeTimer _timer(&var)
#define TIME_SCOPE_MS(var) f64 var = 0.0; ScopeTimer _timer(&var)

} // namespace ecscope::core