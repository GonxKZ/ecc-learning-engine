#pragma once

#include "audio_types.hpp"
#include <memory>
#include <vector>
#include <array>
#include <complex>
#include <functional>

namespace ecscope::audio {

// Base audio effect interface
class AudioEffect {
public:
    virtual ~AudioEffect() = default;
    
    // Effect control
    virtual void initialize(uint32_t sample_rate, uint32_t buffer_size) = 0;
    virtual void process(AudioBuffer& buffer) = 0;
    virtual void process_stereo(StereoBuffer& buffer) = 0;
    virtual void reset() = 0;
    
    // Parameter management
    virtual void set_parameter(const std::string& name, float value) = 0;
    virtual float get_parameter(const std::string& name) const = 0;
    virtual std::vector<std::string> get_parameter_names() const = 0;
    
    // Effect information
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual bool is_enabled() const = 0;
    virtual void set_enabled(bool enabled) = 0;
    
    // Latency and timing
    virtual uint32_t get_latency_samples() const = 0;
    virtual bool requires_stereo() const = 0;
};

// Effect parameter with range and description
struct EffectParameter {
    std::string name;
    std::string description;
    float min_value;
    float max_value;
    float default_value;
    std::string unit;
    bool is_logarithmic = false;
};

// Base effect class with common functionality
class BaseEffect : public AudioEffect {
public:
    BaseEffect(const std::string& name, const std::string& description);
    ~BaseEffect() override = default;
    
    void initialize(uint32_t sample_rate, uint32_t buffer_size) override;
    void reset() override;
    
    void set_parameter(const std::string& name, float value) override;
    float get_parameter(const std::string& name) const override;
    std::vector<std::string> get_parameter_names() const override;
    
    std::string get_name() const override;
    std::string get_description() const override;
    bool is_enabled() const override;
    void set_enabled(bool enabled) override;
    
    uint32_t get_latency_samples() const override;
    bool requires_stereo() const override;

protected:
    // Parameter management for derived classes
    void add_parameter(const EffectParameter& param);
    void set_parameter_value(const std::string& name, float value);
    float get_parameter_value(const std::string& name) const;
    
    // Utility functions
    void apply_wet_dry_mix(AudioBuffer& buffer, const AudioBuffer& dry_buffer, float mix);
    void apply_gain(AudioBuffer& buffer, float gain);
    
    uint32_t m_sample_rate = 44100;
    uint32_t m_buffer_size = 1024;
    
private:
    std::string m_name;
    std::string m_description;
    bool m_enabled = true;
    uint32_t m_latency_samples = 0;
    bool m_requires_stereo = false;
    
    std::vector<EffectParameter> m_parameters;
    std::unordered_map<std::string, float> m_parameter_values;
    mutable std::mutex m_parameter_mutex;
};

// Reverb effect for environmental audio
class ReverbEffect : public BaseEffect {
public:
    ReverbEffect();
    ~ReverbEffect() override;
    
    void initialize(uint32_t sample_rate, uint32_t buffer_size) override;
    void process(AudioBuffer& buffer) override;
    void process_stereo(StereoBuffer& buffer) override;
    void reset() override;
    
    // Reverb-specific methods
    void set_room_size(float size);
    void set_damping(float damping);
    void set_wet_level(float wet);
    void set_dry_level(float dry);
    void set_width(float width);
    void set_freeze_mode(bool freeze);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Convolution reverb for realistic acoustic simulation
class ConvolutionReverbEffect : public BaseEffect {
public:
    ConvolutionReverbEffect();
    ~ConvolutionReverbEffect() override;
    
    void initialize(uint32_t sample_rate, uint32_t buffer_size) override;
    void process(AudioBuffer& buffer) override;
    void process_stereo(StereoBuffer& buffer) override;
    void reset() override;
    
    // Impulse response management
    bool load_impulse_response(const std::string& filepath);
    bool load_impulse_response(const AudioBuffer& left_ir, const AudioBuffer& right_ir);
    void set_mix_level(float mix);
    void set_gain(float gain);
    
    // Advanced convolution settings
    void set_convolution_method(ConvolutionMethod method);
    void enable_early_reflections(bool enable);
    void set_predelay(float delay_ms);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Dynamic range compressor
class CompressorEffect : public BaseEffect {
public:
    CompressorEffect();
    ~CompressorEffect() override;
    
    void initialize(uint32_t sample_rate, uint32_t buffer_size) override;
    void process(AudioBuffer& buffer) override;
    void process_stereo(StereoBuffer& buffer) override;
    void reset() override;
    
    // Compressor parameters
    void set_threshold(float threshold_db);
    void set_ratio(float ratio);
    void set_attack_time(float attack_ms);
    void set_release_time(float release_ms);
    void set_knee_width(float knee_db);
    void set_makeup_gain(float gain_db);
    void enable_auto_makeup(bool enable);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Parametric equalizer
class EqualizerEffect : public BaseEffect {
public:
    struct EQBand {
        enum Type {
            LOW_PASS,
            HIGH_PASS,
            LOW_SHELF,
            HIGH_SHELF,
            PEAKING,
            NOTCH,
            ALL_PASS
        };
        
        Type type = PEAKING;
        float frequency = 1000.0f;
        float gain_db = 0.0f;
        float q_factor = 1.0f;
        bool enabled = true;
    };
    
    EqualizerEffect(size_t num_bands = 10);
    ~EqualizerEffect() override;
    
    void initialize(uint32_t sample_rate, uint32_t buffer_size) override;
    void process(AudioBuffer& buffer) override;
    void process_stereo(StereoBuffer& buffer) override;
    void reset() override;
    
    // EQ band management
    void set_band(size_t index, const EQBand& band);
    EQBand get_band(size_t index) const;
    size_t get_band_count() const;
    void enable_band(size_t index, bool enabled);
    
    // Preset management
    void load_preset(const std::string& name);
    void save_preset(const std::string& name);
    std::vector<std::string> get_available_presets() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio delay/echo effect
class DelayEffect : public BaseEffect {
public:
    DelayEffect();
    ~DelayEffect() override;
    
    void initialize(uint32_t sample_rate, uint32_t buffer_size) override;
    void process(AudioBuffer& buffer) override;
    void process_stereo(StereoBuffer& buffer) override;
    void reset() override;
    
    // Delay parameters
    void set_delay_time(float delay_ms);
    void set_feedback(float feedback);
    void set_wet_level(float wet);
    void set_dry_level(float dry);
    void enable_ping_pong(bool enable);  // For stereo delays
    void set_modulation(float rate_hz, float depth);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Chorus effect
class ChorusEffect : public BaseEffect {
public:
    ChorusEffect();
    ~ChorusEffect() override;
    
    void initialize(uint32_t sample_rate, uint32_t buffer_size) override;
    void process(AudioBuffer& buffer) override;
    void process_stereo(StereoBuffer& buffer) override;
    void reset() override;
    
    // Chorus parameters
    void set_rate(float rate_hz);
    void set_depth(float depth);
    void set_feedback(float feedback);
    void set_mix(float mix);
    void set_voices(int num_voices);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Distortion effect
class DistortionEffect : public BaseEffect {
public:
    enum DistortionType {
        SOFT_CLIPPING,
        HARD_CLIPPING,
        OVERDRIVE,
        FUZZ,
        BITCRUSHER,
        WAVESHAPER
    };
    
    DistortionEffect();
    ~DistortionEffect() override;
    
    void initialize(uint32_t sample_rate, uint32_t buffer_size) override;
    void process(AudioBuffer& buffer) override;
    void process_stereo(StereoBuffer& buffer) override;
    void reset() override;
    
    // Distortion parameters
    void set_drive(float drive);
    void set_type(DistortionType type);
    void set_tone(float tone);
    void set_output_level(float level);
    void set_bit_depth(int bits);  // For bitcrusher
    void set_sample_rate_reduction(float factor);  // For bitcrusher

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Multi-tap delay for complex echo patterns
class MultiTapDelayEffect : public BaseEffect {
public:
    struct DelayTap {
        float delay_ms = 100.0f;
        float gain = 0.5f;
        float pan = 0.0f;  // -1.0 to 1.0
        bool enabled = true;
    };
    
    MultiTapDelayEffect(size_t max_taps = 8);
    ~MultiTapDelayEffect() override;
    
    void initialize(uint32_t sample_rate, uint32_t buffer_size) override;
    void process(AudioBuffer& buffer) override;
    void process_stereo(StereoBuffer& buffer) override;
    void reset() override;
    
    // Tap management
    void set_tap(size_t index, const DelayTap& tap);
    DelayTap get_tap(size_t index) const;
    size_t get_tap_count() const;
    void enable_tap(size_t index, bool enabled);
    void clear_all_taps();
    
    // Global parameters
    void set_feedback(float feedback);
    void set_mix_level(float mix);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio effects chain
class EffectsChain {
public:
    EffectsChain();
    ~EffectsChain();
    
    // Chain management
    void add_effect(std::unique_ptr<AudioEffect> effect);
    void insert_effect(size_t index, std::unique_ptr<AudioEffect> effect);
    void remove_effect(size_t index);
    void clear_effects();
    void move_effect(size_t from_index, size_t to_index);
    
    // Effect access
    AudioEffect* get_effect(size_t index);
    const AudioEffect* get_effect(size_t index) const;
    size_t get_effect_count() const;
    
    // Processing
    void initialize(uint32_t sample_rate, uint32_t buffer_size);
    void process(AudioBuffer& buffer);
    void process_stereo(StereoBuffer& buffer);
    void reset();
    
    // Chain control
    void set_enabled(bool enabled);
    bool is_enabled() const;
    void bypass_effect(size_t index, bool bypass);
    
    // Latency compensation
    uint32_t get_total_latency() const;
    void enable_latency_compensation(bool enable);
    
    // Preset management
    void save_chain_preset(const std::string& name);
    void load_chain_preset(const std::string& name);
    std::vector<std::string> get_chain_presets() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Environmental audio processor
class EnvironmentalProcessor {
public:
    EnvironmentalProcessor();
    ~EnvironmentalProcessor();
    
    // Initialization
    void initialize(uint32_t sample_rate, uint32_t buffer_size);
    void shutdown();
    
    // Environmental settings
    void set_environment_settings(const EnvironmentalAudio& settings);
    EnvironmentalAudio get_environment_settings() const;
    
    // Processing
    void process_environmental_audio(AudioBuffer& buffer, 
                                   const Vector3f& source_position,
                                   const Vector3f& listener_position);
    
    // Room simulation
    void set_room_parameters(float size, float damping, float diffusion);
    void set_material_properties(const std::vector<EnvironmentalAudio::MaterialProperties>& materials);
    void enable_early_reflections(bool enable);
    void set_air_absorption(bool enable);
    
    // Real-time adjustment
    void update_listener_position(const Vector3f& position);
    void update_environment_in_realtime(bool enable);
    
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Occlusion processor for realistic sound blocking
class OcclusionProcessor {
public:
    OcclusionProcessor();
    ~OcclusionProcessor();
    
    // Initialization
    void initialize(uint32_t sample_rate, uint32_t buffer_size);
    void set_geometry(const std::vector<Vector3f>& geometry);
    
    // Occlusion calculation
    float calculate_occlusion_factor(const Vector3f& source, const Vector3f& listener) const;
    float calculate_obstruction_factor(const Vector3f& source, const Vector3f& listener) const;
    
    // Audio processing
    void apply_occlusion(AudioBuffer& buffer, float occlusion_factor);
    void apply_obstruction(AudioBuffer& buffer, float obstruction_factor);
    
    // Material properties
    void set_material_absorption(float absorption);
    void set_material_transmission(float transmission);
    
    // Performance settings
    void set_ray_tracing_quality(int quality);
    void enable_approximation_mode(bool enable);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Effect factory for creating effects by name
class AudioEffectFactory {
public:
    // Built-in effects
    static std::unique_ptr<AudioEffect> create_reverb();
    static std::unique_ptr<AudioEffect> create_convolution_reverb();
    static std::unique_ptr<AudioEffect> create_compressor();
    static std::unique_ptr<AudioEffect> create_equalizer(size_t num_bands = 10);
    static std::unique_ptr<AudioEffect> create_delay();
    static std::unique_ptr<AudioEffect> create_chorus();
    static std::unique_ptr<AudioEffect> create_distortion();
    static std::unique_ptr<AudioEffect> create_multi_tap_delay(size_t max_taps = 8);
    
    // Factory registration
    using EffectCreator = std::function<std::unique_ptr<AudioEffect>()>;
    static void register_effect(const std::string& name, EffectCreator creator);
    static std::unique_ptr<AudioEffect> create_effect(const std::string& name);
    static std::vector<std::string> get_available_effects();
    
    // Preset management
    static void save_effect_preset(const AudioEffect& effect, const std::string& preset_name);
    static void load_effect_preset(AudioEffect& effect, const std::string& preset_name);
    static std::vector<std::string> get_effect_presets(const std::string& effect_name);
};

} // namespace ecscope::audio