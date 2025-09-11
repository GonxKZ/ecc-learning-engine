#pragma once

#include "../core/ai_types.hpp"
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

namespace ecscope::ai {

/**
 * @brief Noise Generation System - Procedural Content Generation
 * 
 * Provides multiple noise algorithms for procedural generation including
 * Perlin noise, Simplex noise, Worley noise, and various fractal combinations.
 * Optimized for real-time generation in game environments.
 */

/**
 * @brief Base Noise Generator Interface
 */
class NoiseBase {
public:
    virtual ~NoiseBase() = default;
    
    virtual f32 sample_1d(f32 x) = 0;
    virtual f32 sample_2d(f32 x, f32 y) = 0;
    virtual f32 sample_3d(f32 x, f32 y, f32 z) = 0;
    
    virtual void set_seed(u64 seed) = 0;
    virtual void set_frequency(f32 frequency) = 0;
    virtual void set_amplitude(f32 amplitude) = 0;
    
    // Fractal properties
    virtual void set_octaves(u32 octaves) = 0;
    virtual void set_persistence(f32 persistence) = 0;
    virtual void set_lacunarity(f32 lacunarity) = 0;
    
    virtual NoiseType get_type() const = 0;
};

/**
 * @brief Perlin Noise Implementation
 */
class PerlinNoise : public NoiseBase {
public:
    explicit PerlinNoise(u64 seed = 0) 
        : seed_(seed), frequency_(1.0f), amplitude_(1.0f), 
          octaves_(1), persistence_(0.5f), lacunarity_(2.0f) {
        initialize_permutation();
    }
    
    f32 sample_1d(f32 x) override {
        f32 value = 0.0f;
        f32 current_amplitude = amplitude_;
        f32 current_frequency = frequency_;
        
        for (u32 i = 0; i < octaves_; ++i) {
            value += perlin_1d(x * current_frequency) * current_amplitude;
            current_amplitude *= persistence_;
            current_frequency *= lacunarity_;
        }
        
        return value;
    }
    
    f32 sample_2d(f32 x, f32 y) override {
        f32 value = 0.0f;
        f32 current_amplitude = amplitude_;
        f32 current_frequency = frequency_;
        
        for (u32 i = 0; i < octaves_; ++i) {
            value += perlin_2d(x * current_frequency, y * current_frequency) * current_amplitude;
            current_amplitude *= persistence_;
            current_frequency *= lacunarity_;
        }
        
        return value;
    }
    
    f32 sample_3d(f32 x, f32 y, f32 z) override {
        f32 value = 0.0f;
        f32 current_amplitude = amplitude_;
        f32 current_frequency = frequency_;
        
        for (u32 i = 0; i < octaves_; ++i) {
            value += perlin_3d(x * current_frequency, y * current_frequency, z * current_frequency) * current_amplitude;
            current_amplitude *= persistence_;
            current_frequency *= lacunarity_;
        }
        
        return value;
    }
    
    void set_seed(u64 seed) override {
        seed_ = seed;
        initialize_permutation();
    }
    
    void set_frequency(f32 frequency) override { frequency_ = frequency; }
    void set_amplitude(f32 amplitude) override { amplitude_ = amplitude; }
    void set_octaves(u32 octaves) override { octaves_ = std::min(octaves, MAX_OCTAVES); }
    void set_persistence(f32 persistence) override { persistence_ = persistence; }
    void set_lacunarity(f32 lacunarity) override { lacunarity_ = lacunarity; }
    
    NoiseType get_type() const override { return NoiseType::PERLIN; }
    
private:
    static constexpr u32 PERMUTATION_SIZE = 512;
    static constexpr u32 MAX_OCTAVES = 8;
    
    u64 seed_;
    f32 frequency_;
    f32 amplitude_;
    u32 octaves_;
    f32 persistence_;
    f32 lacunarity_;
    
    std::array<i32, PERMUTATION_SIZE> permutation_;
    
    void initialize_permutation() {
        std::iota(permutation_.begin(), permutation_.begin() + 256, 0);
        
        std::mt19937_64 rng(seed_);
        std::shuffle(permutation_.begin(), permutation_.begin() + 256, rng);
        
        // Duplicate for overflow handling
        for (u32 i = 0; i < 256; ++i) {
            permutation_[256 + i] = permutation_[i];
        }
    }
    
    static f32 fade(f32 t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }
    
    static f32 lerp(f32 t, f32 a, f32 b) {
        return a + t * (b - a);
    }
    
    static f32 grad_1d(i32 hash, f32 x) {
        return (hash & 1) ? x : -x;
    }
    
    static f32 grad_2d(i32 hash, f32 x, f32 y) {
        switch (hash & 3) {
            case 0: return x + y;
            case 1: return -x + y;
            case 2: return x - y;
            case 3: return -x - y;
        }
        return 0.0f;
    }
    
    static f32 grad_3d(i32 hash, f32 x, f32 y, f32 z) {
        switch (hash & 15) {
            case 0: return x + y;     case 1: return -x + y;
            case 2: return x - y;     case 3: return -x - y;
            case 4: return x + z;     case 5: return -x + z;
            case 6: return x - z;     case 7: return -x - z;
            case 8: return y + z;     case 9: return -y + z;
            case 10: return y - z;    case 11: return -y - z;
            case 12: return y + x;    case 13: return -y + z;
            case 14: return y - x;    case 15: return -y - z;
        }
        return 0.0f;
    }
    
    f32 perlin_1d(f32 x) {
        i32 X = static_cast<i32>(std::floor(x)) & 255;
        x -= std::floor(x);
        
        f32 u = fade(x);
        
        return lerp(u, grad_1d(permutation_[X], x),
                      grad_1d(permutation_[X + 1], x - 1.0f));
    }
    
    f32 perlin_2d(f32 x, f32 y) {
        i32 X = static_cast<i32>(std::floor(x)) & 255;
        i32 Y = static_cast<i32>(std::floor(y)) & 255;
        
        x -= std::floor(x);
        y -= std::floor(y);
        
        f32 u = fade(x);
        f32 v = fade(y);
        
        i32 A = permutation_[X] + Y;
        i32 B = permutation_[X + 1] + Y;
        
        return lerp(v, lerp(u, grad_2d(permutation_[A], x, y),
                              grad_2d(permutation_[B], x - 1.0f, y)),
                      lerp(u, grad_2d(permutation_[A + 1], x, y - 1.0f),
                              grad_2d(permutation_[B + 1], x - 1.0f, y - 1.0f)));
    }
    
    f32 perlin_3d(f32 x, f32 y, f32 z) {
        i32 X = static_cast<i32>(std::floor(x)) & 255;
        i32 Y = static_cast<i32>(std::floor(y)) & 255;
        i32 Z = static_cast<i32>(std::floor(z)) & 255;
        
        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);
        
        f32 u = fade(x);
        f32 v = fade(y);
        f32 w = fade(z);
        
        i32 A = permutation_[X] + Y;
        i32 AA = permutation_[A] + Z;
        i32 AB = permutation_[A + 1] + Z;
        i32 B = permutation_[X + 1] + Y;
        i32 BA = permutation_[B] + Z;
        i32 BB = permutation_[B + 1] + Z;
        
        return lerp(w, lerp(v, lerp(u, grad_3d(permutation_[AA], x, y, z),
                                      grad_3d(permutation_[BA], x - 1.0f, y, z)),
                              lerp(u, grad_3d(permutation_[AB], x, y - 1.0f, z),
                                      grad_3d(permutation_[BB], x - 1.0f, y - 1.0f, z))),
                      lerp(v, lerp(u, grad_3d(permutation_[AA + 1], x, y, z - 1.0f),
                                      grad_3d(permutation_[BA + 1], x - 1.0f, y, z - 1.0f)),
                              lerp(u, grad_3d(permutation_[AB + 1], x, y - 1.0f, z - 1.0f),
                                      grad_3d(permutation_[BB + 1], x - 1.0f, y - 1.0f, z - 1.0f))));
    }
};

/**
 * @brief Simplex Noise Implementation (Ken Perlin's improved algorithm)
 */
class SimplexNoise : public NoiseBase {
public:
    explicit SimplexNoise(u64 seed = 0)
        : seed_(seed), frequency_(1.0f), amplitude_(1.0f),
          octaves_(1), persistence_(0.5f), lacunarity_(2.0f) {
        initialize_permutation();
    }
    
    f32 sample_1d(f32 x) override {
        f32 value = 0.0f;
        f32 current_amplitude = amplitude_;
        f32 current_frequency = frequency_;
        
        for (u32 i = 0; i < octaves_; ++i) {
            value += simplex_2d(x * current_frequency, 0.0f) * current_amplitude;
            current_amplitude *= persistence_;
            current_frequency *= lacunarity_;
        }
        
        return value;
    }
    
    f32 sample_2d(f32 x, f32 y) override {
        f32 value = 0.0f;
        f32 current_amplitude = amplitude_;
        f32 current_frequency = frequency_;
        
        for (u32 i = 0; i < octaves_; ++i) {
            value += simplex_2d(x * current_frequency, y * current_frequency) * current_amplitude;
            current_amplitude *= persistence_;
            current_frequency *= lacunarity_;
        }
        
        return value;
    }
    
    f32 sample_3d(f32 x, f32 y, f32 z) override {
        f32 value = 0.0f;
        f32 current_amplitude = amplitude_;
        f32 current_frequency = frequency_;
        
        for (u32 i = 0; i < octaves_; ++i) {
            value += simplex_3d(x * current_frequency, y * current_frequency, z * current_frequency) * current_amplitude;
            current_amplitude *= persistence_;
            current_frequency *= lacunarity_;
        }
        
        return value;
    }
    
    void set_seed(u64 seed) override {
        seed_ = seed;
        initialize_permutation();
    }
    
    void set_frequency(f32 frequency) override { frequency_ = frequency; }
    void set_amplitude(f32 amplitude) override { amplitude_ = amplitude; }
    void set_octaves(u32 octaves) override { octaves_ = std::min(octaves, 8u); }
    void set_persistence(f32 persistence) override { persistence_ = persistence; }
    void set_lacunarity(f32 lacunarity) override { lacunarity_ = lacunarity; }
    
    NoiseType get_type() const override { return NoiseType::SIMPLEX; }
    
private:
    u64 seed_;
    f32 frequency_;
    f32 amplitude_;
    u32 octaves_;
    f32 persistence_;
    f32 lacunarity_;
    
    std::array<i32, 512> perm_;
    std::array<i32, 512> perm_mod12_;
    
    void initialize_permutation() {
        std::array<i32, 256> p;
        std::iota(p.begin(), p.end(), 0);
        
        std::mt19937_64 rng(seed_);
        std::shuffle(p.begin(), p.end(), rng);
        
        for (i32 i = 0; i < 512; ++i) {
            perm_[i] = p[i & 255];
            perm_mod12_[i] = perm_[i] % 12;
        }
    }
    
    static constexpr std::array<std::array<i32, 3>, 12> grad3 = {{
        {{1,1,0}}, {{-1,1,0}}, {{1,-1,0}}, {{-1,-1,0}},
        {{1,0,1}}, {{-1,0,1}}, {{1,0,-1}}, {{-1,0,-1}},
        {{0,1,1}}, {{0,-1,1}}, {{0,1,-1}}, {{0,-1,-1}}
    }};
    
    static f32 dot(const std::array<i32, 3>& g, f32 x, f32 y) {
        return g[0] * x + g[1] * y;
    }
    
    static f32 dot(const std::array<i32, 3>& g, f32 x, f32 y, f32 z) {
        return g[0] * x + g[1] * y + g[2] * z;
    }
    
    f32 simplex_2d(f32 xin, f32 yin) {
        static constexpr f32 F2 = 0.5f * (std::sqrt(3.0f) - 1.0f);
        static constexpr f32 G2 = (3.0f - std::sqrt(3.0f)) / 6.0f;
        
        f32 n0, n1, n2;
        
        f32 s = (xin + yin) * F2;
        i32 i = static_cast<i32>(std::floor(xin + s));
        i32 j = static_cast<i32>(std::floor(yin + s));
        
        f32 t = (i + j) * G2;
        f32 X0 = i - t;
        f32 Y0 = j - t;
        f32 x0 = xin - X0;
        f32 y0 = yin - Y0;
        
        i32 i1, j1;
        if (x0 > y0) { i1 = 1; j1 = 0; }
        else { i1 = 0; j1 = 1; }
        
        f32 x1 = x0 - i1 + G2;
        f32 y1 = y0 - j1 + G2;
        f32 x2 = x0 - 1.0f + 2.0f * G2;
        f32 y2 = y0 - 1.0f + 2.0f * G2;
        
        i32 ii = i & 255;
        i32 jj = j & 255;
        i32 gi0 = perm_mod12_[ii + perm_[jj]];
        i32 gi1 = perm_mod12_[ii + i1 + perm_[jj + j1]];
        i32 gi2 = perm_mod12_[ii + 1 + perm_[jj + 1]];
        
        f32 t0 = 0.5f - x0 * x0 - y0 * y0;
        if (t0 < 0) n0 = 0.0f;
        else {
            t0 *= t0;
            n0 = t0 * t0 * dot(grad3[gi0], x0, y0);
        }
        
        f32 t1 = 0.5f - x1 * x1 - y1 * y1;
        if (t1 < 0) n1 = 0.0f;
        else {
            t1 *= t1;
            n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
        }
        
        f32 t2 = 0.5f - x2 * x2 - y2 * y2;
        if (t2 < 0) n2 = 0.0f;
        else {
            t2 *= t2;
            n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
        }
        
        return 70.0f * (n0 + n1 + n2);
    }
    
    f32 simplex_3d(f32 xin, f32 yin, f32 zin) {
        static constexpr f32 F3 = 1.0f / 3.0f;
        static constexpr f32 G3 = 1.0f / 6.0f;
        
        f32 n0, n1, n2, n3;
        
        f32 s = (xin + yin + zin) * F3;
        i32 i = static_cast<i32>(std::floor(xin + s));
        i32 j = static_cast<i32>(std::floor(yin + s));
        i32 k = static_cast<i32>(std::floor(zin + s));
        
        f32 t = (i + j + k) * G3;
        f32 X0 = i - t;
        f32 Y0 = j - t;
        f32 Z0 = k - t;
        f32 x0 = xin - X0;
        f32 y0 = yin - Y0;
        f32 z0 = zin - Z0;
        
        i32 i1, j1, k1, i2, j2, k2;
        
        if (x0 >= y0) {
            if (y0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
            else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; }
            else { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; }
        } else {
            if (y0 < z0) { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; }
            else if (x0 < z0) { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; }
            else { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
        }
        
        f32 x1 = x0 - i1 + G3;
        f32 y1 = y0 - j1 + G3;
        f32 z1 = z0 - k1 + G3;
        f32 x2 = x0 - i2 + 2.0f * G3;
        f32 y2 = y0 - j2 + 2.0f * G3;
        f32 z2 = z0 - k2 + 2.0f * G3;
        f32 x3 = x0 - 1.0f + 3.0f * G3;
        f32 y3 = y0 - 1.0f + 3.0f * G3;
        f32 z3 = z0 - 1.0f + 3.0f * G3;
        
        i32 ii = i & 255;
        i32 jj = j & 255;
        i32 kk = k & 255;
        
        i32 gi0 = perm_mod12_[ii + perm_[jj + perm_[kk]]];
        i32 gi1 = perm_mod12_[ii + i1 + perm_[jj + j1 + perm_[kk + k1]]];
        i32 gi2 = perm_mod12_[ii + i2 + perm_[jj + j2 + perm_[kk + k2]]];
        i32 gi3 = perm_mod12_[ii + 1 + perm_[jj + 1 + perm_[kk + 1]]];
        
        f32 t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
        if (t0 < 0) n0 = 0.0f;
        else {
            t0 *= t0;
            n0 = t0 * t0 * dot(grad3[gi0], x0, y0, z0);
        }
        
        f32 t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
        if (t1 < 0) n1 = 0.0f;
        else {
            t1 *= t1;
            n1 = t1 * t1 * dot(grad3[gi1], x1, y1, z1);
        }
        
        f32 t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
        if (t2 < 0) n2 = 0.0f;
        else {
            t2 *= t2;
            n2 = t2 * t2 * dot(grad3[gi2], x2, y2, z2);
        }
        
        f32 t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
        if (t3 < 0) n3 = 0.0f;
        else {
            t3 *= t3;
            n3 = t3 * t3 * dot(grad3[gi3], x3, y3, z3);
        }
        
        return 32.0f * (n0 + n1 + n2 + n3);
    }
};

/**
 * @brief Worley Noise Implementation (Cellular/Voronoi noise)
 */
class WorleyNoise : public NoiseBase {
public:
    explicit WorleyNoise(u64 seed = 0)
        : seed_(seed), frequency_(1.0f), amplitude_(1.0f),
          octaves_(1), persistence_(0.5f), lacunarity_(2.0f),
          distance_type_(DistanceType::EUCLIDEAN) {
        rng_.seed(seed);
    }
    
    enum class DistanceType : u8 {
        EUCLIDEAN = 0,
        MANHATTAN,
        CHEBYSHEV,
        MINKOWSKI
    };
    
    f32 sample_1d(f32 x) override {
        return sample_2d(x, 0.0f);
    }
    
    f32 sample_2d(f32 x, f32 y) override {
        f32 value = 0.0f;
        f32 current_amplitude = amplitude_;
        f32 current_frequency = frequency_;
        
        for (u32 i = 0; i < octaves_; ++i) {
            value += worley_2d(x * current_frequency, y * current_frequency) * current_amplitude;
            current_amplitude *= persistence_;
            current_frequency *= lacunarity_;
        }
        
        return value;
    }
    
    f32 sample_3d(f32 x, f32 y, f32 z) override {
        f32 value = 0.0f;
        f32 current_amplitude = amplitude_;
        f32 current_frequency = frequency_;
        
        for (u32 i = 0; i < octaves_; ++i) {
            value += worley_3d(x * current_frequency, y * current_frequency, z * current_frequency) * current_amplitude;
            current_amplitude *= persistence_;
            current_frequency *= lacunarity_;
        }
        
        return value;
    }
    
    void set_seed(u64 seed) override {
        seed_ = seed;
        rng_.seed(seed);
    }
    
    void set_frequency(f32 frequency) override { frequency_ = frequency; }
    void set_amplitude(f32 amplitude) override { amplitude_ = amplitude; }
    void set_octaves(u32 octaves) override { octaves_ = std::min(octaves, 8u); }
    void set_persistence(f32 persistence) override { persistence_ = persistence; }
    void set_lacunarity(f32 lacunarity) override { lacunarity_ = lacunarity; }
    
    void set_distance_type(DistanceType type) { distance_type_ = type; }
    
    NoiseType get_type() const override { return NoiseType::WORLEY; }
    
private:
    u64 seed_;
    f32 frequency_;
    f32 amplitude_;
    u32 octaves_;
    f32 persistence_;
    f32 lacunarity_;
    DistanceType distance_type_;
    mutable std::mt19937_64 rng_;
    
    f32 distance_function(f32 dx, f32 dy) const {
        switch (distance_type_) {
            case DistanceType::EUCLIDEAN:
                return std::sqrt(dx * dx + dy * dy);
            case DistanceType::MANHATTAN:
                return std::abs(dx) + std::abs(dy);
            case DistanceType::CHEBYSHEV:
                return std::max(std::abs(dx), std::abs(dy));
            case DistanceType::MINKOWSKI:
                return std::pow(std::pow(std::abs(dx), 3.0f) + std::pow(std::abs(dy), 3.0f), 1.0f / 3.0f);
        }
        return 0.0f;
    }
    
    f32 distance_function(f32 dx, f32 dy, f32 dz) const {
        switch (distance_type_) {
            case DistanceType::EUCLIDEAN:
                return std::sqrt(dx * dx + dy * dy + dz * dz);
            case DistanceType::MANHATTAN:
                return std::abs(dx) + std::abs(dy) + std::abs(dz);
            case DistanceType::CHEBYSHEV:
                return std::max({std::abs(dx), std::abs(dy), std::abs(dz)});
            case DistanceType::MINKOWSKI:
                return std::pow(std::pow(std::abs(dx), 3.0f) + std::pow(std::abs(dy), 3.0f) + std::pow(std::abs(dz), 3.0f), 1.0f / 3.0f);
        }
        return 0.0f;
    }
    
    f32 worley_2d(f32 x, f32 y) const {
        i32 cellX = static_cast<i32>(std::floor(x));
        i32 cellY = static_cast<i32>(std::floor(y));
        
        f32 min_distance = std::numeric_limits<f32>::max();
        
        // Check surrounding cells
        for (i32 dx = -1; dx <= 1; ++dx) {
            for (i32 dy = -1; dy <= 1; ++dy) {
                i32 currentCellX = cellX + dx;
                i32 currentCellY = cellY + dy;
                
                // Generate random point in cell
                u64 cellSeed = hash_2d(currentCellX, currentCellY);
                std::mt19937_64 cellRng(cellSeed);
                std::uniform_real_distribution<f32> dis(0.0f, 1.0f);
                
                f32 pointX = currentCellX + dis(cellRng);
                f32 pointY = currentCellY + dis(cellRng);
                
                f32 distance = distance_function(x - pointX, y - pointY);
                min_distance = std::min(min_distance, distance);
            }
        }
        
        return min_distance;
    }
    
    f32 worley_3d(f32 x, f32 y, f32 z) const {
        i32 cellX = static_cast<i32>(std::floor(x));
        i32 cellY = static_cast<i32>(std::floor(y));
        i32 cellZ = static_cast<i32>(std::floor(z));
        
        f32 min_distance = std::numeric_limits<f32>::max();
        
        // Check surrounding cells
        for (i32 dx = -1; dx <= 1; ++dx) {
            for (i32 dy = -1; dy <= 1; ++dy) {
                for (i32 dz = -1; dz <= 1; ++dz) {
                    i32 currentCellX = cellX + dx;
                    i32 currentCellY = cellY + dy;
                    i32 currentCellZ = cellZ + dz;
                    
                    // Generate random point in cell
                    u64 cellSeed = hash_3d(currentCellX, currentCellY, currentCellZ);
                    std::mt19937_64 cellRng(cellSeed);
                    std::uniform_real_distribution<f32> dis(0.0f, 1.0f);
                    
                    f32 pointX = currentCellX + dis(cellRng);
                    f32 pointY = currentCellY + dis(cellRng);
                    f32 pointZ = currentCellZ + dis(cellRng);
                    
                    f32 distance = distance_function(x - pointX, y - pointY, z - pointZ);
                    min_distance = std::min(min_distance, distance);
                }
            }
        }
        
        return min_distance;
    }
    
    u64 hash_2d(i32 x, i32 y) const {
        // Simple hash function for 2D coordinates
        u64 hash = seed_;
        hash ^= static_cast<u64>(x) * 0x9e3779b9;
        hash ^= static_cast<u64>(y) * 0x85ebca6b;
        hash ^= hash >> 13;
        hash *= 0xc2b2ae35;
        hash ^= hash >> 16;
        return hash;
    }
    
    u64 hash_3d(i32 x, i32 y, i32 z) const {
        // Simple hash function for 3D coordinates
        u64 hash = seed_;
        hash ^= static_cast<u64>(x) * 0x9e3779b9;
        hash ^= static_cast<u64>(y) * 0x85ebca6b;
        hash ^= static_cast<u64>(z) * 0xc2b2ae35;
        hash ^= hash >> 13;
        hash *= 0x85ebca6b;
        hash ^= hash >> 16;
        return hash;
    }
};

/**
 * @brief Noise Factory - Creates noise generators based on type
 */
class NoiseFactory {
public:
    static std::unique_ptr<NoiseBase> create_noise(const NoiseGenerator& config) {
        std::unique_ptr<NoiseBase> noise;
        
        switch (config.type) {
            case NoiseType::PERLIN:
                noise = std::make_unique<PerlinNoise>(config.seed);
                break;
            case NoiseType::SIMPLEX:
                noise = std::make_unique<SimplexNoise>(config.seed);
                break;
            case NoiseType::WORLEY:
                noise = std::make_unique<WorleyNoise>(config.seed);
                break;
            default:
                noise = std::make_unique<PerlinNoise>(config.seed);
                break;
        }
        
        if (noise) {
            noise->set_frequency(config.frequency);
            noise->set_amplitude(config.amplitude);
            noise->set_octaves(config.octaves);
            noise->set_persistence(config.persistence);
            noise->set_lacunarity(config.lacunarity);
        }
        
        return noise;
    }
    
    // Convenience functions for common noise types
    static std::unique_ptr<NoiseBase> create_perlin(u64 seed = 0, f32 frequency = 1.0f, u32 octaves = 4) {
        NoiseGenerator config;
        config.type = NoiseType::PERLIN;
        config.seed = seed;
        config.frequency = frequency;
        config.octaves = octaves;
        return create_noise(config);
    }
    
    static std::unique_ptr<NoiseBase> create_simplex(u64 seed = 0, f32 frequency = 1.0f, u32 octaves = 4) {
        NoiseGenerator config;
        config.type = NoiseType::SIMPLEX;
        config.seed = seed;
        config.frequency = frequency;
        config.octaves = octaves;
        return create_noise(config);
    }
    
    static std::unique_ptr<NoiseBase> create_worley(u64 seed = 0, f32 frequency = 1.0f) {
        NoiseGenerator config;
        config.type = NoiseType::WORLEY;
        config.seed = seed;
        config.frequency = frequency;
        config.octaves = 1; // Worley typically uses 1 octave
        return create_noise(config);
    }
};

/**
 * @brief Utility functions for common procedural generation patterns
 */
namespace noise_utils {

// Generate height map using noise
inline Grid2D<f32> generate_heightmap(const NoiseGenerator& config, u32 width, u32 height) {
    auto noise = NoiseFactory::create_noise(config);
    Grid2D<f32> heightmap(width, height);
    
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            f32 sample_x = static_cast<f32>(x) / width;
            f32 sample_y = static_cast<f32>(y) / height;
            heightmap(x, y) = noise->sample_2d(sample_x, sample_y);
        }
    }
    
    return heightmap;
}

// Generate terrain using multiple noise layers
inline Grid2D<f32> generate_terrain(u32 width, u32 height, u64 seed = 0) {
    Grid2D<f32> terrain(width, height);
    
    // Base terrain
    auto base_noise = NoiseFactory::create_perlin(seed, 0.01f, 6);
    
    // Detail noise
    auto detail_noise = NoiseFactory::create_perlin(seed + 1, 0.1f, 3);
    
    // Ridge noise
    auto ridge_noise = NoiseFactory::create_simplex(seed + 2, 0.05f, 4);
    
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            f32 sample_x = static_cast<f32>(x);
            f32 sample_y = static_cast<f32>(y);
            
            f32 base = base_noise->sample_2d(sample_x, sample_y) * 0.7f;
            f32 detail = detail_noise->sample_2d(sample_x, sample_y) * 0.2f;
            f32 ridge = std::abs(ridge_noise->sample_2d(sample_x, sample_y)) * 0.1f;
            
            terrain(x, y) = base + detail + ridge;
        }
    }
    
    return terrain;
}

// Generate cave system using Worley noise
inline Grid2D<bool> generate_caves(u32 width, u32 height, u64 seed = 0, f32 threshold = 0.4f) {
    auto worley = NoiseFactory::create_worley(seed, 0.02f);
    Grid2D<bool> caves(width, height);
    
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            f32 sample_x = static_cast<f32>(x);
            f32 sample_y = static_cast<f32>(y);
            
            f32 value = worley->sample_2d(sample_x, sample_y);
            caves(x, y) = value < threshold; // Cave if below threshold
        }
    }
    
    return caves;
}

} // namespace noise_utils

} // namespace ecscope::ai