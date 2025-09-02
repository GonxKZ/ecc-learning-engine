#pragma once

#include "../component.hpp"
#include "core/types.hpp"
#include <cmath>

namespace ecscope::ecs::components {

// 2D Vector for position, scale, etc.
struct Vec2 {
    f32 x{0.0f};
    f32 y{0.0f};
    
    constexpr Vec2() = default;
    constexpr Vec2(f32 x_, f32 y_) noexcept : x(x_), y(y_) {}
    
    // Basic arithmetic operations
    constexpr Vec2 operator+(const Vec2& other) const noexcept {
        return Vec2{x + other.x, y + other.y};
    }
    
    constexpr Vec2 operator-(const Vec2& other) const noexcept {
        return Vec2{x - other.x, y - other.y};
    }
    
    constexpr Vec2 operator*(f32 scalar) const noexcept {
        return Vec2{x * scalar, y * scalar};
    }
    
    constexpr Vec2 operator/(f32 scalar) const noexcept {
        return Vec2{x / scalar, y / scalar};
    }
    
    constexpr Vec2& operator+=(const Vec2& other) noexcept {
        x += other.x;
        y += other.y;
        return *this;
    }
    
    constexpr Vec2& operator-=(const Vec2& other) noexcept {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    
    constexpr Vec2& operator*=(f32 scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    
    constexpr Vec2& operator/=(f32 scalar) noexcept {
        x /= scalar;
        y /= scalar;
        return *this;
    }
    
    // Utility functions
    constexpr f32 length_squared() const noexcept {
        return x * x + y * y;
    }
    
    f32 length() const noexcept {
        return std::sqrt(length_squared());
    }
    
    Vec2 normalized() const noexcept {
        f32 len = length();
        return len > 0.0f ? (*this / len) : Vec2{0.0f, 0.0f};
    }
    
    constexpr f32 dot(const Vec2& other) const noexcept {
        return x * other.x + y * other.y;
    }
    
    // Static utility vectors
    static constexpr Vec2 zero() noexcept { return Vec2{0.0f, 0.0f}; }
    static constexpr Vec2 one() noexcept { return Vec2{1.0f, 1.0f}; }
    static constexpr Vec2 up() noexcept { return Vec2{0.0f, 1.0f}; }
    static constexpr Vec2 down() noexcept { return Vec2{0.0f, -1.0f}; }
    static constexpr Vec2 left() noexcept { return Vec2{-1.0f, 0.0f}; }
    static constexpr Vec2 right() noexcept { return Vec2{1.0f, 0.0f}; }
};

// Transform component - fundamental for spatial representation
struct Transform : ComponentBase {
    Vec2 position{0.0f, 0.0f};    // World position
    f32 rotation{0.0f};           // Rotation in radians
    Vec2 scale{1.0f, 1.0f};       // Scale factors
    
    constexpr Transform() = default;
    
    constexpr Transform(Vec2 pos, f32 rot = 0.0f, Vec2 scl = Vec2::one()) noexcept
        : position(pos), rotation(rot), scale(scl) {}
    
    constexpr Transform(f32 x, f32 y, f32 rot = 0.0f, f32 scale_uniform = 1.0f) noexcept
        : position(x, y), rotation(rot), scale(scale_uniform, scale_uniform) {}
    
    // Transform operations
    Vec2 transform_point(const Vec2& local_point) const noexcept {
        // Simple 2D transformation: scale, rotate, translate
        f32 cos_r = std::cos(rotation);
        f32 sin_r = std::sin(rotation);
        
        // Apply scale
        Vec2 scaled = Vec2{local_point.x * scale.x, local_point.y * scale.y};
        
        // Apply rotation
        Vec2 rotated = Vec2{
            scaled.x * cos_r - scaled.y * sin_r,
            scaled.x * sin_r + scaled.y * cos_r
        };
        
        // Apply translation
        return rotated + position;
    }
    
    Vec2 inverse_transform_point(const Vec2& world_point) const noexcept {
        // Inverse transformation: untranslate, unrotate, unscale
        Vec2 translated = world_point - position;
        
        // Inverse rotation
        f32 cos_r = std::cos(-rotation);
        f32 sin_r = std::sin(-rotation);
        Vec2 rotated = Vec2{
            translated.x * cos_r - translated.y * sin_r,
            translated.x * sin_r + translated.y * cos_r
        };
        
        // Inverse scale
        return Vec2{rotated.x / scale.x, rotated.y / scale.y};
    }
    
    // Get forward direction (useful for movement)
    Vec2 forward() const noexcept {
        return Vec2{std::cos(rotation), std::sin(rotation)};
    }
    
    // Get right direction
    Vec2 right() const noexcept {
        return Vec2{-std::sin(rotation), std::cos(rotation)};
    }
    
    // Translate by offset
    void translate(const Vec2& offset) noexcept {
        position += offset;
    }
    
    // Rotate by additional angle
    void rotate(f32 angle_radians) noexcept {
        rotation += angle_radians;
        // Normalize to [-π, π] range
        while (rotation > 3.14159f) rotation -= 6.28318f;
        while (rotation < -3.14159f) rotation += 6.28318f;
    }
    
    // Scale uniformly
    void scale_uniform(f32 factor) noexcept {
        scale *= factor;
    }
    
    // Utility functions for common transformations
    static constexpr Transform identity() noexcept {
        return Transform{};
    }
    
    static constexpr Transform at_position(Vec2 pos) noexcept {
        return Transform{pos};
    }
    
    static constexpr Transform with_rotation(f32 rot) noexcept {
        return Transform{Vec2::zero(), rot};
    }
    
    static constexpr Transform with_scale(Vec2 scl) noexcept {
        return Transform{Vec2::zero(), 0.0f, scl};
    }
};

// Verify that Transform satisfies our Component concept
static_assert(Component<Transform>, "Transform must satisfy Component concept");
static_assert(Component<Vec2>, "Vec2 must satisfy Component concept");

} // namespace ecscope::ecs::components

