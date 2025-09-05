#pragma once

/**
 * @file physics_education_tools.hpp  
 * @brief Educational Visualization and Interactive Tuning Tools for ECScope Physics
 * 
 * This header provides comprehensive educational tools for physics simulation,
 * including algorithm visualization, interactive parameter tuning, step-by-step
 * analysis, and real-time performance monitoring. Designed to teach physics
 * concepts while maintaining high performance.
 * 
 * Key Features:
 * - Real-time visualization of forces, velocities, accelerations
 * - Interactive parameter adjustment with immediate feedback
 * - Step-by-step algorithm breakdown and analysis
 * - Performance profiling with educational insights  
 * - Comparative analysis between different algorithms
 * - Material property visualization and stress analysis
 * - Fluid flow visualization with field overlays
 * - Educational overlays explaining physics concepts
 * 
 * Educational Goals:
 * - Make abstract physics concepts visible and understandable
 * - Allow experimentation with parameters to see effects
 * - Demonstrate the relationship between math and simulation
 * - Show performance implications of different approaches
 * - Provide guided learning experiences
 * 
 * Performance Considerations:
 * - Minimal impact on simulation performance
 * - Efficient data collection and visualization
 * - Optional features that can be disabled for production
 * - GPU-accelerated visualization when available
 * 
 * @author ECScope Educational Physics Engine
 * @date 2024
 */

#include "physics.hpp"
#include "soft_body_physics.hpp"
#include "fluid_simulation.hpp"
#include "advanced_materials.hpp"
#include "../ecs/component.hpp"
#include "../debug_renderer.hpp"
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <variant>

namespace ecscope::physics::education {

// Import necessary types
using namespace ecscope::physics::math;
using namespace ecscope::ecs;

//=============================================================================
// Visualization Data Structures
//=============================================================================

/**
 * @brief Color utilities for physics visualization
 */
namespace colors {
    struct Color {
        u8 r, g, b, a;
        
        constexpr Color(u8 red, u8 green, u8 blue, u8 alpha = 255) noexcept
            : r(red), g(green), b(blue), a(alpha) {}
        
        static constexpr Color white() { return {255, 255, 255}; }
        static constexpr Color black() { return {0, 0, 0}; }
        static constexpr Color red() { return {255, 0, 0}; }
        static constexpr Color green() { return {0, 255, 0}; }
        static constexpr Color blue() { return {0, 0, 255}; }
        static constexpr Color yellow() { return {255, 255, 0}; }
        static constexpr Color cyan() { return {0, 255, 255}; }
        static constexpr Color magenta() { return {255, 0, 255}; }
        static constexpr Color orange() { return {255, 165, 0}; }
        static constexpr Color purple() { return {128, 0, 128}; }
    };
    
    /** @brief Generate color from scalar value using heat map */
    Color scalar_to_heatmap(f32 value, f32 min_val, f32 max_val) noexcept {
        f32 normalized = std::clamp((value - min_val) / (max_val - min_val), 0.0f, 1.0f);
        
        if (normalized < 0.25f) {
            // Blue to cyan
            f32 t = normalized * 4.0f;
            return {0, static_cast<u8>(255 * t), 255};
        } else if (normalized < 0.5f) {
            // Cyan to green
            f32 t = (normalized - 0.25f) * 4.0f;
            return {0, 255, static_cast<u8>(255 * (1.0f - t))};
        } else if (normalized < 0.75f) {
            // Green to yellow
            f32 t = (normalized - 0.5f) * 4.0f;
            return {static_cast<u8>(255 * t), 255, 0};
        } else {
            // Yellow to red
            f32 t = (normalized - 0.75f) * 4.0f;
            return {255, static_cast<u8>(255 * (1.0f - t)), 0};
        }
    }
    
    /** @brief Generate color from velocity magnitude */
    Color velocity_to_color(f32 speed, f32 max_speed) noexcept {
        return scalar_to_heatmap(speed, 0.0f, max_speed);
    }
    
    /** @brief Generate color from pressure value */
    Color pressure_to_color(f32 pressure, f32 max_pressure) noexcept {
        if (pressure >= 0.0f) {
            return scalar_to_heatmap(pressure, 0.0f, max_pressure);
        } else {
            // Negative pressure in blue tones
            f32 normalized = std::clamp(-pressure / max_pressure, 0.0f, 1.0f);
            return {0, 0, static_cast<u8>(128 + 127 * normalized)};
        }
    }
}

/**
 * @brief Vector field visualization data
 * 
 * Represents a 2D vector field for visualizing forces, velocities, etc.
 */
struct VectorField {
    /** @brief Grid dimensions */
    u32 width, height;
    
    /** @brief Grid spacing in world units */
    f32 grid_spacing;
    
    /** @brief Origin of the grid */
    Vec2 origin;
    
    /** @brief Vector data at each grid point */
    std::vector<Vec2> vectors;
    
    /** @brief Magnitude data for coloring */
    std::vector<f32> magnitudes;
    
    /** @brief Field type for labeling */
    std::string field_name;
    
    VectorField(u32 w, u32 h, f32 spacing, Vec2 orig, std::string_view name)
        : width(w), height(h), grid_spacing(spacing), origin(orig), field_name(name) {
        vectors.resize(width * height);
        magnitudes.resize(width * height);
    }
    
    /** @brief Set vector at grid position */
    void set_vector(u32 x, u32 y, const Vec2& vector) {
        if (x < width && y < height) {
            usize index = y * width + x;
            vectors[index] = vector;
            magnitudes[index] = vector.length();
        }
    }
    
    /** @brief Get world position for grid coordinates */
    Vec2 grid_to_world(u32 x, u32 y) const noexcept {
        return origin + Vec2(x * grid_spacing, y * grid_spacing);
    }
    
    /** @brief Clear all vectors */
    void clear() {
        std::fill(vectors.begin(), vectors.end(), Vec2::zero());
        std::fill(magnitudes.begin(), magnitudes.end(), 0.0f);
    }
    
    /** @brief Get maximum magnitude for normalization */
    f32 get_max_magnitude() const noexcept {
        return *std::max_element(magnitudes.begin(), magnitudes.end());
    }
};

/**
 * @brief Scalar field visualization data
 * 
 * Represents a 2D scalar field for visualizing pressure, temperature, density, etc.
 */
struct ScalarField {
    /** @brief Grid dimensions */
    u32 width, height;
    
    /** @brief Grid spacing in world units */
    f32 grid_spacing;
    
    /** @brief Origin of the grid */
    Vec2 origin;
    
    /** @brief Scalar values at each grid point */
    std::vector<f32> values;
    
    /** @brief Field type for labeling */
    std::string field_name;
    
    /** @brief Units for display */
    std::string units;
    
    ScalarField(u32 w, u32 h, f32 spacing, Vec2 orig, std::string_view name, std::string_view unit)
        : width(w), height(h), grid_spacing(spacing), origin(orig), field_name(name), units(unit) {
        values.resize(width * height);
    }
    
    /** @brief Set value at grid position */
    void set_value(u32 x, u32 y, f32 value) {
        if (x < width && y < height) {
            values[y * width + x] = value;
        }
    }
    
    /** @brief Get world position for grid coordinates */
    Vec2 grid_to_world(u32 x, u32 y) const noexcept {
        return origin + Vec2(x * grid_spacing, y * grid_spacing);
    }
    
    /** @brief Clear all values */
    void clear() {
        std::fill(values.begin(), values.end(), 0.0f);
    }
    
    /** @brief Get value range for normalization */
    std::pair<f32, f32> get_value_range() const noexcept {
        auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
        return {*min_it, *max_it};
    }
};

/**
 * @brief Particle trail for motion visualization
 */
struct ParticleTrail {
    /** @brief Maximum trail length */
    static constexpr u32 MAX_TRAIL_LENGTH = 100;
    
    /** @brief Trail positions */
    std::array<Vec2, MAX_TRAIL_LENGTH> positions;
    
    /** @brief Trail timestamps */
    std::array<f32, MAX_TRAIL_LENGTH> timestamps;
    
    /** @brief Current trail length */
    u32 trail_length{0};
    
    /** @brief Trail start index (circular buffer) */
    u32 start_index{0};
    
    /** @brief Trail color */
    colors::Color color{colors::Color::white()};
    
    /** @brief Add new position to trail */
    void add_position(const Vec2& pos, f32 time) noexcept {
        u32 next_index = (start_index + trail_length) % MAX_TRAIL_LENGTH;
        
        positions[next_index] = pos;
        timestamps[next_index] = time;
        
        if (trail_length < MAX_TRAIL_LENGTH) {
            trail_length++;
        } else {
            start_index = (start_index + 1) % MAX_TRAIL_LENGTH;
        }
    }
    
    /** @brief Get trail position by age (0 = newest, trail_length-1 = oldest) */
    Vec2 get_position(u32 age) const noexcept {
        if (age >= trail_length) return Vec2::zero();
        
        u32 index = (start_index + trail_length - 1 - age) % MAX_TRAIL_LENGTH;
        return positions[index];
    }
    
    /** @brief Clear trail */
    void clear() noexcept {
        trail_length = 0;
        start_index = 0;
    }
};

//=============================================================================
// Interactive Parameter System
//=============================================================================

/**
 * @brief Base class for interactive parameters
 */
class InteractiveParameter {
public:
    enum class Type {
        Float, Int, Bool, Vector2, Color, Enum
    };
    
protected:
    std::string name_;
    std::string description_;
    Type type_;
    bool is_modified_{false};
    
public:
    InteractiveParameter(std::string name, std::string desc, Type t)
        : name_(std::move(name)), description_(std::move(desc)), type_(t) {}
    
    virtual ~InteractiveParameter() = default;
    
    const std::string& get_name() const noexcept { return name_; }
    const std::string& get_description() const noexcept { return description_; }
    Type get_type() const noexcept { return type_; }
    bool is_modified() const noexcept { return is_modified_; }
    void clear_modified() noexcept { is_modified_ = false; }
    
    virtual std::string get_value_string() const = 0;
    virtual void reset_to_default() = 0;
};

/**
 * @brief Float parameter with range constraints
 */
class FloatParameter : public InteractiveParameter {
private:
    f32* value_ptr_;
    f32 default_value_;
    f32 min_value_, max_value_;
    f32 step_;
    
public:
    FloatParameter(std::string name, std::string desc, f32* value,
                   f32 def_val, f32 min_val, f32 max_val, f32 step = 0.1f)
        : InteractiveParameter(std::move(name), std::move(desc), Type::Float),
          value_ptr_(value), default_value_(def_val), 
          min_value_(min_val), max_value_(max_val), step_(step) {}
    
    f32 get_value() const noexcept { return *value_ptr_; }
    f32 get_min() const noexcept { return min_value_; }
    f32 get_max() const noexcept { return max_value_; }
    f32 get_step() const noexcept { return step_; }
    
    void set_value(f32 new_value) noexcept {
        f32 clamped = std::clamp(new_value, min_value_, max_value_);
        if (std::abs(*value_ptr_ - clamped) > 1e-6f) {
            *value_ptr_ = clamped;
            is_modified_ = true;
        }
    }
    
    std::string get_value_string() const override {
        return std::to_string(*value_ptr_);
    }
    
    void reset_to_default() override {
        set_value(default_value_);
    }
};

/**
 * @brief Boolean parameter
 */
class BoolParameter : public InteractiveParameter {
private:
    bool* value_ptr_;
    bool default_value_;
    
public:
    BoolParameter(std::string name, std::string desc, bool* value, bool def_val)
        : InteractiveParameter(std::move(name), std::move(desc), Type::Bool),
          value_ptr_(value), default_value_(def_val) {}
    
    bool get_value() const noexcept { return *value_ptr_; }
    
    void set_value(bool new_value) noexcept {
        if (*value_ptr_ != new_value) {
            *value_ptr_ = new_value;
            is_modified_ = true;
        }
    }
    
    void toggle() noexcept {
        set_value(!*value_ptr_);
    }
    
    std::string get_value_string() const override {
        return *value_ptr_ ? "true" : "false";
    }
    
    void reset_to_default() override {
        set_value(default_value_);
    }
};

/**
 * @brief Vector2 parameter
 */
class Vector2Parameter : public InteractiveParameter {
private:
    Vec2* value_ptr_;
    Vec2 default_value_;
    Vec2 min_value_, max_value_;
    
public:
    Vector2Parameter(std::string name, std::string desc, Vec2* value,
                     Vec2 def_val, Vec2 min_val, Vec2 max_val)
        : InteractiveParameter(std::move(name), std::move(desc), Type::Vector2),
          value_ptr_(value), default_value_(def_val), 
          min_value_(min_val), max_value_(max_val) {}
    
    const Vec2& get_value() const noexcept { return *value_ptr_; }
    const Vec2& get_min() const noexcept { return min_value_; }
    const Vec2& get_max() const noexcept { return max_value_; }
    
    void set_value(const Vec2& new_value) noexcept {
        Vec2 clamped = {
            std::clamp(new_value.x, min_value_.x, max_value_.x),
            std::clamp(new_value.y, min_value_.y, max_value_.y)
        };
        if ((*value_ptr_ - clamped).length_squared() > 1e-6f) {
            *value_ptr_ = clamped;
            is_modified_ = true;
        }
    }
    
    std::string get_value_string() const override {
        return "(" + std::to_string(value_ptr_->x) + ", " + std::to_string(value_ptr_->y) + ")";
    }
    
    void reset_to_default() override {
        set_value(default_value_);
    }
};

/**
 * @brief Parameter group for organized display
 */
class ParameterGroup {
private:
    std::string name_;
    std::vector<std::unique_ptr<InteractiveParameter>> parameters_;
    bool is_expanded_{true};
    
public:
    explicit ParameterGroup(std::string name) : name_(std::move(name)) {}
    
    const std::string& get_name() const noexcept { return name_; }
    bool is_expanded() const noexcept { return is_expanded_; }
    void set_expanded(bool expanded) noexcept { is_expanded_ = expanded; }
    
    template<typename T, typename... Args>
    T* add_parameter(Args&&... args) {
        auto param = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = param.get();
        parameters_.push_back(std::move(param));
        return ptr;
    }
    
    const std::vector<std::unique_ptr<InteractiveParameter>>& get_parameters() const noexcept {
        return parameters_;
    }
    
    /** @brief Reset all parameters to default values */
    void reset_all_to_default() {
        for (auto& param : parameters_) {
            param->reset_to_default();
        }
    }
    
    /** @brief Check if any parameters were modified */
    bool has_modifications() const noexcept {
        return std::any_of(parameters_.begin(), parameters_.end(),
                          [](const auto& param) { return param->is_modified(); });
    }
    
    /** @brief Clear all modification flags */
    void clear_all_modifications() {
        for (auto& param : parameters_) {
            param->clear_modified();
        }
    }
};

//=============================================================================
// Physics Algorithm Visualization
//=============================================================================

/**
 * @brief Step-by-step algorithm analyzer
 * 
 * Breaks down physics algorithms into discrete steps for educational analysis.
 */
class AlgorithmStepper {
public:
    /** @brief Algorithm step information */
    struct Step {
        std::string name;
        std::string description;
        std::function<void()> execute;
        std::function<void()> visualize;
        f64 execution_time{0.0};
        bool is_completed{false};
        
        Step(std::string n, std::string desc, 
             std::function<void()> exec, std::function<void()> vis = nullptr)
            : name(std::move(n)), description(std::move(desc)), 
              execute(std::move(exec)), visualize(std::move(vis)) {}
    };
    
private:
    std::vector<Step> steps_;
    usize current_step_{0};
    bool auto_step_{false};
    f32 auto_step_delay_{1.0f}; // seconds
    f32 step_timer_{0.0f};
    
public:
    /** @brief Add a step to the algorithm */
    void add_step(Step step) {
        steps_.push_back(std::move(step));
    }
    
    /** @brief Execute current step */
    void execute_current_step() {
        if (current_step_ < steps_.size()) {
            auto& step = steps_[current_step_];
            
            auto start_time = std::chrono::high_resolution_clock::now();
            step.execute();
            auto end_time = std::chrono::high_resolution_clock::now();
            
            step.execution_time = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
            step.is_completed = true;
            
            if (step.visualize) {
                step.visualize();
            }
        }
    }
    
    /** @brief Advance to next step */
    void next_step() {
        if (current_step_ < steps_.size()) {
            execute_current_step();
            current_step_++;
        }
    }
    
    /** @brief Go back to previous step */
    void previous_step() {
        if (current_step_ > 0) {
            current_step_--;
            steps_[current_step_].is_completed = false;
        }
    }
    
    /** @brief Reset to beginning */
    void reset() {
        current_step_ = 0;
        step_timer_ = 0.0f;
        for (auto& step : steps_) {
            step.is_completed = false;
            step.execution_time = 0.0;
        }
    }
    
    /** @brief Execute all remaining steps */
    void execute_all() {
        while (current_step_ < steps_.size()) {
            next_step();
        }
    }
    
    /** @brief Update for automatic stepping */
    void update(f32 delta_time) {
        if (auto_step_ && current_step_ < steps_.size()) {
            step_timer_ += delta_time;
            if (step_timer_ >= auto_step_delay_) {
                next_step();
                step_timer_ = 0.0f;
            }
        }
    }
    
    /** @brief Get current step information */
    const Step* get_current_step() const noexcept {
        return (current_step_ < steps_.size()) ? &steps_[current_step_] : nullptr;
    }
    
    /** @brief Get all steps */
    const std::vector<Step>& get_all_steps() const noexcept {
        return steps_;
    }
    
    /** @brief Get current step index */
    usize get_current_step_index() const noexcept { return current_step_; }
    
    /** @brief Get total number of steps */
    usize get_step_count() const noexcept { return steps_.size(); }
    
    /** @brief Check if algorithm is complete */
    bool is_complete() const noexcept { return current_step_ >= steps_.size(); }
    
    /** @brief Enable/disable automatic stepping */
    void set_auto_step(bool enabled, f32 delay = 1.0f) noexcept {
        auto_step_ = enabled;
        auto_step_delay_ = delay;
        step_timer_ = 0.0f;
    }
    
    /** @brief Get total execution time for completed steps */
    f64 get_total_execution_time() const noexcept {
        f64 total = 0.0;
        for (const auto& step : steps_) {
            if (step.is_completed) {
                total += step.execution_time;
            }
        }
        return total;
    }
};

//=============================================================================
// Performance Analysis Tools
//=============================================================================

/**
 * @brief Performance profiler for educational analysis
 */
class EducationalProfiler {
public:
    /** @brief Profile sample */
    struct ProfileSample {
        std::string name;
        f64 start_time;
        f64 duration;
        u32 call_count;
        
        ProfileSample(std::string n) : name(std::move(n)), start_time(0.0), duration(0.0), call_count(0) {}
    };
    
    /** @brief Performance statistics */
    struct PerformanceStats {
        f64 total_time{0.0};
        f64 average_time{0.0};
        f64 min_time{1e6};
        f64 max_time{0.0};
        u32 sample_count{0};
        f32 fps_equivalent{0.0f};
        std::string bottleneck_name;
    };
    
private:
    std::unordered_map<std::string, ProfileSample> samples_;
    std::unordered_map<std::string, PerformanceStats> stats_;
    std::vector<std::pair<std::string, f64>> frame_samples_;
    bool profiling_enabled_{true};
    
public:
    /** @brief Start timing a section */
    void begin_sample(const std::string& name) {
        if (!profiling_enabled_) return;
        
        auto& sample = samples_[name];
        sample.name = name;
        sample.start_time = get_current_time();
        sample.call_count++;
    }
    
    /** @brief End timing a section */
    void end_sample(const std::string& name) {
        if (!profiling_enabled_) return;
        
        f64 end_time = get_current_time();
        auto it = samples_.find(name);
        if (it != samples_.end()) {
            f64 duration = end_time - it->second.start_time;
            it->second.duration = duration;
            
            // Update statistics
            auto& stats = stats_[name];
            stats.total_time += duration;
            stats.sample_count++;
            stats.average_time = stats.total_time / stats.sample_count;
            stats.min_time = std::min(stats.min_time, duration);
            stats.max_time = std::max(stats.max_time, duration);
            
            frame_samples_.push_back({name, duration});
        }
    }
    
    /** @brief RAII-style profiler guard */
    class ScopedProfiler {
        EducationalProfiler* profiler_;
        std::string name_;
    public:
        ScopedProfiler(EducationalProfiler* prof, std::string name) 
            : profiler_(prof), name_(std::move(name)) {
            profiler_->begin_sample(name_);
        }
        ~ScopedProfiler() {
            profiler_->end_sample(name_);
        }
    };
    
    #define PROFILE_SCOPE(profiler, name) \
        EducationalProfiler::ScopedProfiler prof_guard(profiler, name)
    
    /** @brief Get performance statistics for a section */
    const PerformanceStats* get_stats(const std::string& name) const {
        auto it = stats_.find(name);
        return (it != stats_.end()) ? &it->second : nullptr;
    }
    
    /** @brief Get all performance statistics */
    const std::unordered_map<std::string, PerformanceStats>& get_all_stats() const noexcept {
        return stats_;
    }
    
    /** @brief Clear all statistics */
    void clear() {
        samples_.clear();
        stats_.clear();
        frame_samples_.clear();
    }
    
    /** @brief End frame and calculate frame statistics */
    void end_frame() {
        if (!profiling_enabled_) return;
        
        // Find bottleneck for this frame
        if (!frame_samples_.empty()) {
            auto max_sample = std::max_element(frame_samples_.begin(), frame_samples_.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            
            for (auto& [name, stat] : stats_) {
                if (name == max_sample->first) {
                    stat.bottleneck_name = name;
                    break;
                }
            }
        }
        
        // Calculate FPS equivalent for each section
        for (auto& [name, stat] : stats_) {
            if (stat.average_time > 0.0) {
                stat.fps_equivalent = static_cast<f32>(1000.0 / stat.average_time);
            }
        }
        
        frame_samples_.clear();
    }
    
    /** @brief Generate performance report */
    std::string generate_report() const {
        std::ostringstream oss;
        oss << "=== Educational Performance Report ===\n";
        oss << std::fixed << std::setprecision(3);
        
        std::vector<std::pair<std::string, PerformanceStats>> sorted_stats;
        for (const auto& [name, stats] : stats_) {
            sorted_stats.push_back({name, stats});
        }
        
        // Sort by average time (descending)
        std::sort(sorted_stats.begin(), sorted_stats.end(),
                 [](const auto& a, const auto& b) { return a.second.average_time > b.second.average_time; });
        
        for (const auto& [name, stats] : sorted_stats) {
            oss << name << ":\n";
            oss << "  Average: " << stats.average_time << " ms\n";
            oss << "  Min/Max: " << stats.min_time << " / " << stats.max_time << " ms\n";
            oss << "  FPS Impact: " << stats.fps_equivalent << " FPS\n";
            oss << "  Samples: " << stats.sample_count << "\n\n";
        }
        
        return oss.str();
    }
    
    /** @brief Enable/disable profiling */
    void set_enabled(bool enabled) noexcept {
        profiling_enabled_ = enabled;
    }
    
private:
    /** @brief Get current high-resolution time in milliseconds */
    f64 get_current_time() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration<f64, std::milli>(duration).count();
    }
};

//=============================================================================
// Educational Visualization Manager
//=============================================================================

/**
 * @brief Main manager for educational physics visualization
 */
class PhysicsEducationManager {
private:
    // Visualization data
    std::vector<VectorField> vector_fields_;
    std::vector<ScalarField> scalar_fields_;
    std::unordered_map<u32, ParticleTrail> particle_trails_;
    
    // Parameter management
    std::vector<std::unique_ptr<ParameterGroup>> parameter_groups_;
    
    // Algorithm analysis
    std::unique_ptr<AlgorithmStepper> current_algorithm_;
    
    // Performance analysis
    std::unique_ptr<EducationalProfiler> profiler_;
    
    // Visualization settings
    struct {
        bool show_forces{true};
        bool show_velocities{true};
        bool show_accelerations{false};
        bool show_pressure{false};
        bool show_density{false};
        bool show_temperature{false};
        bool show_particle_trails{true};
        bool show_grid{false};
        bool show_stress_visualization{false};
        f32 vector_scale{1.0f};
        f32 trail_length{2.0f}; // seconds
        u32 field_resolution{32};
        f32 visualization_alpha{0.7f};
    } viz_settings_;
    
    // Educational overlays
    std::vector<std::string> educational_texts_;
    bool show_educational_overlays_{true};
    
public:
    PhysicsEducationManager() {
        profiler_ = std::make_unique<EducationalProfiler>();
        initialize_default_parameters();
    }
    
    //-------------------------------------------------------------------------
    // Vector Field Visualization
    //-------------------------------------------------------------------------
    
    /** @brief Add or update vector field */
    void set_vector_field(std::string_view name, u32 width, u32 height, 
                         f32 spacing, Vec2 origin) {
        // Remove existing field with same name
        vector_fields_.erase(
            std::remove_if(vector_fields_.begin(), vector_fields_.end(),
                          [name](const VectorField& field) { return field.field_name == name; }),
            vector_fields_.end());
        
        // Add new field
        vector_fields_.emplace_back(width, height, spacing, origin, std::string(name));
    }
    
    /** @brief Update vector field data from physics simulation */
    void update_force_field(const std::vector<Vec2>& positions, const std::vector<Vec2>& forces) {
        auto* field = get_vector_field("Forces");
        if (!field) return;
        
        field->clear();
        
        // Sample forces at grid points
        for (u32 y = 0; y < field->height; ++y) {
            for (u32 x = 0; x < field->width; ++x) {
                Vec2 grid_pos = field->grid_to_world(x, y);
                Vec2 interpolated_force = interpolate_force_at_position(grid_pos, positions, forces);
                field->set_vector(x, y, interpolated_force);
            }
        }
    }
    
    /** @brief Update velocity field */
    void update_velocity_field(const std::vector<Vec2>& positions, const std::vector<Vec2>& velocities) {
        auto* field = get_vector_field("Velocities");
        if (!field) return;
        
        field->clear();
        
        for (u32 y = 0; y < field->height; ++y) {
            for (u32 x = 0; x < field->width; ++x) {
                Vec2 grid_pos = field->grid_to_world(x, y);
                Vec2 interpolated_velocity = interpolate_velocity_at_position(grid_pos, positions, velocities);
                field->set_vector(x, y, interpolated_velocity);
            }
        }
    }
    
    //-------------------------------------------------------------------------
    // Scalar Field Visualization
    //-------------------------------------------------------------------------
    
    /** @brief Add or update scalar field */
    void set_scalar_field(std::string_view name, u32 width, u32 height,
                         f32 spacing, Vec2 origin, std::string_view units) {
        scalar_fields_.erase(
            std::remove_if(scalar_fields_.begin(), scalar_fields_.end(),
                          [name](const ScalarField& field) { return field.field_name == name; }),
            scalar_fields_.end());
        
        scalar_fields_.emplace_back(width, height, spacing, origin, std::string(name), std::string(units));
    }
    
    /** @brief Update pressure field for fluid visualization */
    void update_pressure_field(const fluid::FluidParticle* particles, usize particle_count, f32 kernel_radius) {
        auto* field = get_scalar_field("Pressure");
        if (!field) return;
        
        field->clear();
        
        // Calculate pressure at each grid point using SPH interpolation
        for (u32 y = 0; y < field->height; ++y) {
            for (u32 x = 0; x < field->width; ++x) {
                Vec2 grid_pos = field->grid_to_world(x, y);
                f32 pressure = interpolate_pressure_sph(grid_pos, particles, particle_count, kernel_radius);
                field->set_value(x, y, pressure);
            }
        }
    }
    
    //-------------------------------------------------------------------------
    // Particle Trail Management
    //-------------------------------------------------------------------------
    
    /** @brief Start tracking particle trail */
    void start_particle_trail(u32 particle_id, colors::Color color = colors::Color::white()) {
        particle_trails_[particle_id] = ParticleTrail{};
        particle_trails_[particle_id].color = color;
    }
    
    /** @brief Update particle trail */
    void update_particle_trail(u32 particle_id, const Vec2& position, f32 current_time) {
        auto it = particle_trails_.find(particle_id);
        if (it != particle_trails_.end()) {
            it->second.add_position(position, current_time);
        }
    }
    
    /** @brief Stop tracking particle trail */
    void stop_particle_trail(u32 particle_id) {
        particle_trails_.erase(particle_id);
    }
    
    //-------------------------------------------------------------------------
    // Parameter Management
    //-------------------------------------------------------------------------
    
    /** @brief Create parameter group */
    ParameterGroup* create_parameter_group(std::string name) {
        auto group = std::make_unique<ParameterGroup>(std::move(name));
        ParameterGroup* ptr = group.get();
        parameter_groups_.push_back(std::move(group));
        return ptr;
    }
    
    /** @brief Get parameter group by name */
    ParameterGroup* get_parameter_group(const std::string& name) {
        auto it = std::find_if(parameter_groups_.begin(), parameter_groups_.end(),
                              [&name](const auto& group) { return group->get_name() == name; });
        return (it != parameter_groups_.end()) ? it->get() : nullptr;
    }
    
    //-------------------------------------------------------------------------
    // Algorithm Stepping
    //-------------------------------------------------------------------------
    
    /** @brief Set current algorithm for stepping */
    void set_algorithm_stepper(std::unique_ptr<AlgorithmStepper> stepper) {
        current_algorithm_ = std::move(stepper);
    }
    
    /** @brief Get current algorithm stepper */
    AlgorithmStepper* get_algorithm_stepper() { return current_algorithm_.get(); }
    
    //-------------------------------------------------------------------------
    // Educational Overlays
    //-------------------------------------------------------------------------
    
    /** @brief Add educational text overlay */
    void add_educational_text(std::string text) {
        educational_texts_.push_back(std::move(text));
    }
    
    /** @brief Clear educational overlays */
    void clear_educational_texts() {
        educational_texts_.clear();
    }
    
    /** @brief Get educational texts */
    const std::vector<std::string>& get_educational_texts() const noexcept {
        return educational_texts_;
    }
    
    //-------------------------------------------------------------------------
    // Performance Analysis
    //-------------------------------------------------------------------------
    
    /** @brief Get profiler for performance analysis */
    EducationalProfiler* get_profiler() { return profiler_.get(); }
    
    //-------------------------------------------------------------------------
    // Update and Rendering
    //-------------------------------------------------------------------------
    
    /** @brief Update visualization data */
    void update(f32 delta_time) {
        // Update algorithm stepper
        if (current_algorithm_) {
            current_algorithm_->update(delta_time);
        }
        
        // Clean up old trail data
        cleanup_old_trails(delta_time);
        
        // End profiler frame
        profiler_->end_frame();
    }
    
    /** @brief Render educational visualization */
    void render(debug::PhysicsDebugRenderer* debug_renderer) {
        if (!debug_renderer) return;
        
        // Render vector fields
        if (viz_settings_.show_forces || viz_settings_.show_velocities) {
            render_vector_fields(debug_renderer);
        }
        
        // Render scalar fields
        if (viz_settings_.show_pressure || viz_settings_.show_density || viz_settings_.show_temperature) {
            render_scalar_fields(debug_renderer);
        }
        
        // Render particle trails
        if (viz_settings_.show_particle_trails) {
            render_particle_trails(debug_renderer);
        }
        
        // Render educational overlays
        if (show_educational_overlays_) {
            render_educational_overlays(debug_renderer);
        }
    }
    
    //-------------------------------------------------------------------------
    // Settings Access
    //-------------------------------------------------------------------------
    
    /** @brief Get visualization settings */
    auto& get_visualization_settings() { return viz_settings_; }
    
    /** @brief Enable/disable educational overlays */
    void set_show_educational_overlays(bool show) noexcept {
        show_educational_overlays_ = show;
    }

private:
    /** @brief Initialize default parameter groups */
    void initialize_default_parameters();
    
    /** @brief Get vector field by name */
    VectorField* get_vector_field(const std::string& name);
    
    /** @brief Get scalar field by name */
    ScalarField* get_scalar_field(const std::string& name);
    
    /** @brief Interpolate force at position */
    Vec2 interpolate_force_at_position(const Vec2& pos, const std::vector<Vec2>& positions, 
                                      const std::vector<Vec2>& forces);
    
    /** @brief Interpolate velocity at position */
    Vec2 interpolate_velocity_at_position(const Vec2& pos, const std::vector<Vec2>& positions,
                                         const std::vector<Vec2>& velocities);
    
    /** @brief Interpolate pressure using SPH */
    f32 interpolate_pressure_sph(const Vec2& pos, const fluid::FluidParticle* particles,
                                usize count, f32 kernel_radius);
    
    /** @brief Cleanup old particle trails */
    void cleanup_old_trails(f32 delta_time);
    
    /** @brief Render vector fields */
    void render_vector_fields(debug::PhysicsDebugRenderer* renderer);
    
    /** @brief Render scalar fields */
    void render_scalar_fields(debug::PhysicsDebugRenderer* renderer);
    
    /** @brief Render particle trails */
    void render_particle_trails(debug::PhysicsDebugRenderer* renderer);
    
    /** @brief Render educational text overlays */
    void render_educational_overlays(debug::PhysicsDebugRenderer* renderer);
};

} // namespace ecscope::physics::education