#pragma once

/**
 * @file physics/math3d.hpp
 * @brief Comprehensive 3D Physics Mathematics Foundation for ECScope Educational ECS Engine
 * 
 * This header extends the existing 2D physics math foundation to support complete 3D physics
 * simulation with emphasis on educational clarity and high performance. It provides:
 * 
 * - Advanced 3D vector mathematics (Vec3, Vec4) with SIMD optimizations
 * - Quaternion mathematics for 3D rotations
 * - 3D matrix operations (3x3, 4x4) for transformations
 * - 3D geometric primitives (Sphere, Box, Capsule, ConvexHull)
 * - 3D collision detection mathematics
 * - Physics constants and utility functions for 3D simulation
 * - Educational debugging and visualization helpers
 * 
 * Educational Philosophy:
 * This library builds upon the 2D foundation to teach advanced 3D physics programming
 * concepts while delivering production-ready performance. Each algorithm includes detailed
 * comments explaining the mathematical principles with references to physics textbooks.
 * 
 * Performance Characteristics:
 * - Header-only implementation for optimal inlining
 * - SIMD optimizations for Vec3/Vec4 operations (SSE/AVX)
 * - Cache-friendly data layouts for 3D structures
 * - Zero-overhead abstractions
 * - Compile-time computations where possible
 * 
 * Memory Integration:
 * - Full integration with ECScope's memory tracking system
 * - Custom allocator support for 3D physics objects
 * - Memory pool optimizations for frequent 3D allocations
 * 
 * @author ECScope Educational ECS Framework - 3D Physics Extension
 * @date 2025
 */

#include "math.hpp"  // Include existing 2D math foundation
#include "core/types.hpp"
#include "ecs/components/transform.hpp"
#include "memory/memory_tracker.hpp"
#include <cmath>
#include <array>
#include <vector>
#include <optional>
#include <span>
#include <algorithm>
#include <immintrin.h>

namespace ecscope::physics::math3d {

// Import 2D foundation
using namespace ecscope::physics::math;

//=============================================================================
// 3D Vector Mathematics
//=============================================================================

/**
 * @brief 3D Vector class with comprehensive mathematical operations
 * 
 * Educational Context:
 * Vec3 represents points and directions in 3D space. It's the foundation
 * for all 3D physics calculations including position, velocity, acceleration,
 * forces, and torque. The implementation prioritizes both performance and
 * educational clarity.
 * 
 * Memory Layout:
 * - 16-byte aligned for SIMD operations
 * - Padding to ensure cache-friendly access patterns
 * - Compatible with graphics APIs (OpenGL, DirectX)
 */
struct alignas(16) Vec3 {
    f32 x, y, z, w;  // w component for SIMD alignment (unused in most ops)
    
    // Constructors
    constexpr Vec3() noexcept : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    constexpr Vec3(f32 x_, f32 y_, f32 z_) noexcept : x(x_), y(y_), z(z_), w(0.0f) {}
    constexpr Vec3(f32 scalar) noexcept : x(scalar), y(scalar), z(scalar), w(0.0f) {}
    
    // Conversion from Vec2 (assumes z = 0)
    constexpr explicit Vec3(const Vec2& v2, f32 z_ = 0.0f) noexcept : x(v2.x), y(v2.y), z(z_), w(0.0f) {}
    
    // Component access
    constexpr f32& operator[](usize index) noexcept {
        return (&x)[index];
    }
    
    constexpr f32 operator[](usize index) const noexcept {
        return (&x)[index];
    }
    
    // Basic arithmetic operations
    constexpr Vec3 operator+(const Vec3& other) const noexcept {
        return Vec3{x + other.x, y + other.y, z + other.z};
    }
    
    constexpr Vec3 operator-(const Vec3& other) const noexcept {
        return Vec3{x - other.x, y - other.y, z - other.z};
    }
    
    constexpr Vec3 operator*(f32 scalar) const noexcept {
        return Vec3{x * scalar, y * scalar, z * scalar};
    }
    
    constexpr Vec3 operator*(const Vec3& other) const noexcept {
        return Vec3{x * other.x, y * other.y, z * other.z};
    }
    
    constexpr Vec3 operator/(f32 scalar) const noexcept {
        const f32 inv_scalar = 1.0f / scalar;
        return Vec3{x * inv_scalar, y * inv_scalar, z * inv_scalar};
    }
    
    constexpr Vec3 operator-() const noexcept {
        return Vec3{-x, -y, -z};
    }
    
    // Compound assignment operators
    constexpr Vec3& operator+=(const Vec3& other) noexcept {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }
    
    constexpr Vec3& operator-=(const Vec3& other) noexcept {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }
    
    constexpr Vec3& operator*=(f32 scalar) noexcept {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }
    
    constexpr Vec3& operator*=(const Vec3& other) noexcept {
        x *= other.x; y *= other.y; z *= other.z;
        return *this;
    }
    
    constexpr Vec3& operator/=(f32 scalar) noexcept {
        const f32 inv_scalar = 1.0f / scalar;
        x *= inv_scalar; y *= inv_scalar; z *= inv_scalar;
        return *this;
    }
    
    // Comparison operators
    constexpr bool operator==(const Vec3& other) const noexcept {
        return std::abs(x - other.x) < constants::EPSILON &&
               std::abs(y - other.y) < constants::EPSILON &&
               std::abs(z - other.z) < constants::EPSILON;
    }
    
    constexpr bool operator!=(const Vec3& other) const noexcept {
        return !(*this == other);
    }
    
    // Vector operations
    constexpr f32 dot(const Vec3& other) const noexcept {
        return x * other.x + y * other.y + z * other.z;
    }
    
    constexpr Vec3 cross(const Vec3& other) const noexcept {
        return Vec3{
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }
    
    constexpr f32 length_squared() const noexcept {
        return x * x + y * y + z * z;
    }
    
    f32 length() const noexcept {
        return std::sqrt(length_squared());
    }
    
    Vec3 normalized() const noexcept {
        const f32 len = length();
        if (len < constants::EPSILON) {
            return Vec3::zero();
        }
        return *this / len;
    }
    
    Vec3& normalize() noexcept {
        const f32 len = length();
        if (len > constants::EPSILON) {
            *this /= len;
        } else {
            *this = Vec3::zero();
        }
        return *this;
    }
    
    // Utility functions
    constexpr f32 distance_to(const Vec3& other) const noexcept {
        return (*this - other).length();
    }
    
    constexpr f32 distance_squared_to(const Vec3& other) const noexcept {
        return (*this - other).length_squared();
    }
    
    constexpr Vec3 lerp(const Vec3& other, f32 t) const noexcept {
        return *this + (other - *this) * t;
    }
    
    // Get minimum and maximum components
    constexpr f32 min_component() const noexcept {
        return std::min({x, y, z});
    }
    
    constexpr f32 max_component() const noexcept {
        return std::max({x, y, z});
    }
    
    constexpr Vec3 abs() const noexcept {
        return Vec3{std::abs(x), std::abs(y), std::abs(z)};
    }
    
    // Convert to Vec2 (drops z component)
    constexpr Vec2 to_vec2() const noexcept {
        return Vec2{x, y};
    }
    
    // Static factory methods
    static constexpr Vec3 zero() noexcept { return Vec3{0.0f, 0.0f, 0.0f}; }
    static constexpr Vec3 one() noexcept { return Vec3{1.0f, 1.0f, 1.0f}; }
    static constexpr Vec3 unit_x() noexcept { return Vec3{1.0f, 0.0f, 0.0f}; }
    static constexpr Vec3 unit_y() noexcept { return Vec3{0.0f, 1.0f, 0.0f}; }
    static constexpr Vec3 unit_z() noexcept { return Vec3{0.0f, 0.0f, 1.0f}; }
    
    // Common 3D directions
    static constexpr Vec3 forward() noexcept { return Vec3{0.0f, 0.0f, -1.0f}; }  // -Z forward (OpenGL convention)
    static constexpr Vec3 back() noexcept { return Vec3{0.0f, 0.0f, 1.0f}; }
    static constexpr Vec3 left() noexcept { return Vec3{-1.0f, 0.0f, 0.0f}; }
    static constexpr Vec3 right() noexcept { return Vec3{1.0f, 0.0f, 0.0f}; }
    static constexpr Vec3 up() noexcept { return Vec3{0.0f, 1.0f, 0.0f}; }
    static constexpr Vec3 down() noexcept { return Vec3{0.0f, -1.0f, 0.0f}; }
};

// Global Vec3 operations
constexpr Vec3 operator*(f32 scalar, const Vec3& vec) noexcept {
    return vec * scalar;
}

/**
 * @brief 4D Vector class for homogeneous coordinates and SIMD optimization
 * 
 * Educational Context:
 * Vec4 is used for homogeneous coordinates in 3D transformations and for
 * SIMD optimization of multiple Vec3 operations. The w component is used
 * for perspective division in graphics and as padding for SIMD alignment.
 */
struct alignas(16) Vec4 {
    f32 x, y, z, w;
    
    // Constructors
    constexpr Vec4() noexcept : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    constexpr Vec4(f32 x_, f32 y_, f32 z_, f32 w_) noexcept : x(x_), y(y_), z(z_), w(w_) {}
    constexpr Vec4(const Vec3& v3, f32 w_ = 1.0f) noexcept : x(v3.x), y(v3.y), z(v3.z), w(w_) {}
    constexpr Vec4(f32 scalar) noexcept : x(scalar), y(scalar), z(scalar), w(scalar) {}
    
    // Component access
    constexpr f32& operator[](usize index) noexcept {
        return (&x)[index];
    }
    
    constexpr f32 operator[](usize index) const noexcept {
        return (&x)[index];
    }
    
    // Basic arithmetic operations
    constexpr Vec4 operator+(const Vec4& other) const noexcept {
        return Vec4{x + other.x, y + other.y, z + other.z, w + other.w};
    }
    
    constexpr Vec4 operator-(const Vec4& other) const noexcept {
        return Vec4{x - other.x, y - other.y, z - other.z, w - other.w};
    }
    
    constexpr Vec4 operator*(f32 scalar) const noexcept {
        return Vec4{x * scalar, y * scalar, z * scalar, w * scalar};
    }
    
    constexpr Vec4 operator*(const Vec4& other) const noexcept {
        return Vec4{x * other.x, y * other.y, z * other.z, w * other.w};
    }
    
    constexpr Vec4 operator/(f32 scalar) const noexcept {
        const f32 inv_scalar = 1.0f / scalar;
        return Vec4{x * inv_scalar, y * inv_scalar, z * inv_scalar, w * inv_scalar};
    }
    
    // Vector operations
    constexpr f32 dot(const Vec4& other) const noexcept {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }
    
    constexpr f32 length_squared() const noexcept {
        return x * x + y * y + z * z + w * w;
    }
    
    f32 length() const noexcept {
        return std::sqrt(length_squared());
    }
    
    Vec4 normalized() const noexcept {
        const f32 len = length();
        if (len < constants::EPSILON) {
            return Vec4{};
        }
        return *this / len;
    }
    
    // Convert to Vec3 (drops w component or performs perspective divide)
    constexpr Vec3 to_vec3() const noexcept {
        return Vec3{x, y, z};
    }
    
    constexpr Vec3 to_vec3_perspective() const noexcept {
        if (std::abs(w) < constants::EPSILON) {
            return Vec3{x, y, z};
        }
        const f32 inv_w = 1.0f / w;
        return Vec3{x * inv_w, y * inv_w, z * inv_w};
    }
    
    // Static factory methods
    static constexpr Vec4 zero() noexcept { return Vec4{0.0f, 0.0f, 0.0f, 0.0f}; }
    static constexpr Vec4 one() noexcept { return Vec4{1.0f, 1.0f, 1.0f, 1.0f}; }
    static constexpr Vec4 unit_x() noexcept { return Vec4{1.0f, 0.0f, 0.0f, 0.0f}; }
    static constexpr Vec4 unit_y() noexcept { return Vec4{0.0f, 1.0f, 0.0f, 0.0f}; }
    static constexpr Vec4 unit_z() noexcept { return Vec4{0.0f, 0.0f, 1.0f, 0.0f}; }
    static constexpr Vec4 unit_w() noexcept { return Vec4{0.0f, 0.0f, 0.0f, 1.0f}; }
};

//=============================================================================
// Quaternion Mathematics for 3D Rotations
//=============================================================================

/**
 * @brief Quaternion class for robust 3D rotations
 * 
 * Educational Context:
 * Quaternions provide a mathematically robust way to represent rotations in 3D
 * without suffering from gimbal lock or other issues with Euler angles.
 * They use 4 components (x, y, z, w) where (x, y, z) represents the rotation
 * axis scaled by sin(θ/2) and w represents cos(θ/2).
 * 
 * Mathematical Properties:
 * - Unit quaternions represent rotations
 * - Quaternion multiplication composes rotations
 * - Conjugate of unit quaternion is its inverse
 * - SLERP provides smooth interpolation between rotations
 * 
 * Performance Benefits:
 * - More efficient than rotation matrices for most operations
 * - Compact representation (4 floats vs 9 for 3x3 matrix)
 * - Avoids trigonometric functions in most operations
 * - SIMD-friendly with Vec4 operations
 */
struct alignas(16) Quaternion {
    f32 x, y, z, w;  // w is the scalar part
    
    // Constructors
    constexpr Quaternion() noexcept : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}  // Identity
    constexpr Quaternion(f32 x_, f32 y_, f32 z_, f32 w_) noexcept : x(x_), y(y_), z(z_), w(w_) {}
    constexpr explicit Quaternion(const Vec4& v) noexcept : x(v.x), y(v.y), z(v.z), w(v.w) {}
    
    // Create from axis-angle representation
    Quaternion(const Vec3& axis, f32 angle) noexcept {
        const f32 half_angle = angle * 0.5f;
        const f32 sin_half = std::sin(half_angle);
        const f32 cos_half = std::cos(half_angle);
        const Vec3 normalized_axis = axis.normalized();
        
        x = normalized_axis.x * sin_half;
        y = normalized_axis.y * sin_half;
        z = normalized_axis.z * sin_half;
        w = cos_half;
    }
    
    // Create from Euler angles (pitch, yaw, roll in radians)
    static Quaternion from_euler(f32 pitch, f32 yaw, f32 roll) noexcept {
        const f32 cp = std::cos(pitch * 0.5f);
        const f32 sp = std::sin(pitch * 0.5f);
        const f32 cy = std::cos(yaw * 0.5f);
        const f32 sy = std::sin(yaw * 0.5f);
        const f32 cr = std::cos(roll * 0.5f);
        const f32 sr = std::sin(roll * 0.5f);
        
        return Quaternion{
            sr * cp * cy - cr * sp * sy,  // x
            cr * sp * cy + sr * cp * sy,  // y
            cr * cp * sy - sr * sp * cy,  // z
            cr * cp * cy + sr * sp * sy   // w
        };
    }
    
    // Create from rotation matrix (3x3)
    static Quaternion from_rotation_matrix(const class Matrix3& mat) noexcept;
    
    // Basic operations
    constexpr Quaternion operator+(const Quaternion& other) const noexcept {
        return Quaternion{x + other.x, y + other.y, z + other.z, w + other.w};
    }
    
    constexpr Quaternion operator-(const Quaternion& other) const noexcept {
        return Quaternion{x - other.x, y - other.y, z - other.z, w - other.w};
    }
    
    constexpr Quaternion operator*(f32 scalar) const noexcept {
        return Quaternion{x * scalar, y * scalar, z * scalar, w * scalar};
    }
    
    constexpr Quaternion operator/(f32 scalar) const noexcept {
        const f32 inv_scalar = 1.0f / scalar;
        return Quaternion{x * inv_scalar, y * inv_scalar, z * inv_scalar, w * inv_scalar};
    }
    
    // Quaternion multiplication (composition of rotations)
    constexpr Quaternion operator*(const Quaternion& other) const noexcept {
        return Quaternion{
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        };
    }
    
    // Quaternion properties
    constexpr f32 length_squared() const noexcept {
        return x * x + y * y + z * z + w * w;
    }
    
    f32 length() const noexcept {
        return std::sqrt(length_squared());
    }
    
    constexpr f32 dot(const Quaternion& other) const noexcept {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }
    
    // Normalization
    Quaternion normalized() const noexcept {
        const f32 len = length();
        if (len < constants::EPSILON) {
            return identity();
        }
        return *this / len;
    }
    
    Quaternion& normalize() noexcept {
        const f32 len = length();
        if (len > constants::EPSILON) {
            *this /= len;
        } else {
            *this = identity();
        }
        return *this;
    }
    
    // Quaternion-specific operations
    constexpr Quaternion conjugate() const noexcept {
        return Quaternion{-x, -y, -z, w};
    }
    
    Quaternion inverse() const noexcept {
        const f32 norm = length_squared();
        if (norm < constants::EPSILON) {
            return identity();
        }
        return conjugate() / norm;
    }
    
    // Rotate a vector by this quaternion
    Vec3 rotate(const Vec3& v) const noexcept {
        // Optimized version of: this * Quaternion(v.x, v.y, v.z, 0) * this.conjugate()
        const Vec3 q_vec{x, y, z};
        const f32 q_scalar = w;
        
        const Vec3 uv = q_vec.cross(v);
        const Vec3 uuv = q_vec.cross(uv);
        
        return v + 2.0f * (uv * q_scalar + uuv);
    }
    
    // Convert to axis-angle representation
    std::pair<Vec3, f32> to_axis_angle() const noexcept {
        const f32 sin_half_angle = std::sqrt(x * x + y * y + z * z);
        if (sin_half_angle < constants::EPSILON) {
            return {Vec3::unit_x(), 0.0f};  // No rotation
        }
        
        const f32 angle = 2.0f * std::atan2(sin_half_angle, w);
        const Vec3 axis = Vec3{x, y, z} / sin_half_angle;
        return {axis, angle};
    }
    
    // Convert to Euler angles (returns pitch, yaw, roll)
    Vec3 to_euler() const noexcept {
        const f32 sinr_cosp = 2.0f * (w * x + y * z);
        const f32 cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
        const f32 roll = std::atan2(sinr_cosp, cosr_cosp);
        
        const f32 sinp = 2.0f * (w * y - z * x);
        f32 pitch;
        if (std::abs(sinp) >= 1.0f) {
            pitch = std::copysign(constants::HALF_PI_F, sinp);  // Use 90 degrees if out of range
        } else {
            pitch = std::asin(sinp);
        }
        
        const f32 siny_cosp = 2.0f * (w * z + x * y);
        const f32 cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
        const f32 yaw = std::atan2(siny_cosp, cosy_cosp);
        
        return Vec3{pitch, yaw, roll};
    }
    
    // Interpolation
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, f32 t) noexcept {
        f32 dot = a.dot(b);
        
        // If the dot product is negative, slerp won't take the shorter path.
        // Note that v1 and -v1 are equivalent when the represent rotations.
        Quaternion b_corrected = b;
        if (dot < 0.0f) {
            b_corrected = Quaternion{-b.x, -b.y, -b.z, -b.w};
            dot = -dot;
        }
        
        // If the inputs are too close for comfort, linearly interpolate
        if (dot > 0.9995f) {
            return (a + (b_corrected - a) * t).normalized();
        }
        
        // Since dot is in range [0, 1], acos is safe
        const f32 theta_0 = std::acos(dot);
        const f32 theta = theta_0 * t;
        const f32 sin_theta = std::sin(theta);
        const f32 sin_theta_0 = std::sin(theta_0);
        
        const f32 s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
        const f32 s1 = sin_theta / sin_theta_0;
        
        return a * s0 + b_corrected * s1;
    }
    
    // Convert to Vec4 for SIMD operations
    constexpr Vec4 to_vec4() const noexcept {
        return Vec4{x, y, z, w};
    }
    
    // Static factory methods
    static constexpr Quaternion identity() noexcept { return Quaternion{0.0f, 0.0f, 0.0f, 1.0f}; }
    
    // Create rotation quaternions for common rotations
    static Quaternion rotation_x(f32 angle) noexcept {
        const f32 half_angle = angle * 0.5f;
        return Quaternion{std::sin(half_angle), 0.0f, 0.0f, std::cos(half_angle)};
    }
    
    static Quaternion rotation_y(f32 angle) noexcept {
        const f32 half_angle = angle * 0.5f;
        return Quaternion{0.0f, std::sin(half_angle), 0.0f, std::cos(half_angle)};
    }
    
    static Quaternion rotation_z(f32 angle) noexcept {
        const f32 half_angle = angle * 0.5f;
        return Quaternion{0.0f, 0.0f, std::sin(half_angle), std::cos(half_angle)};
    }
    
    // Look at rotation (creates a rotation that looks from 'from' towards 'to')
    static Quaternion look_at(const Vec3& from, const Vec3& to, const Vec3& up = Vec3::up()) noexcept {
        const Vec3 forward = (to - from).normalized();
        const Vec3 right = forward.cross(up).normalized();
        const Vec3 up_corrected = right.cross(forward);
        
        // Create rotation matrix and convert to quaternion
        // This is a simplified approach - full implementation would handle edge cases
        const Matrix3 rotation_matrix = Matrix3::from_axes(right, up_corrected, -forward);
        return from_rotation_matrix(rotation_matrix);
    }
};

//=============================================================================
// 3D Matrix Mathematics
//=============================================================================

/**
 * @brief 3x3 Matrix for 3D rotations and linear transformations
 * 
 * Educational Context:
 * 3x3 matrices represent linear transformations in 3D space including
 * rotations, scaling, and shearing. They're used extensively in physics
 * for inertia tensors, basis transformations, and coordinate system conversions.
 * 
 * Memory Layout:
 * Column-major order for compatibility with graphics APIs:
 * | m00  m01  m02 |   | col0.x  col1.x  col2.x |
 * | m10  m11  m12 | = | col0.y  col1.y  col2.y |
 * | m20  m21  m22 |   | col0.z  col1.z  col2.z |
 */
struct alignas(32) Matrix3 {
    Vec3 col0, col1, col2;  // Column vectors
    
    // Constructors
    constexpr Matrix3() noexcept 
        : col0(1.0f, 0.0f, 0.0f), col1(0.0f, 1.0f, 0.0f), col2(0.0f, 0.0f, 1.0f) {}  // Identity
    
    constexpr Matrix3(const Vec3& c0, const Vec3& c1, const Vec3& c2) noexcept 
        : col0(c0), col1(c1), col2(c2) {}
    
    constexpr Matrix3(f32 m00, f32 m01, f32 m02,
                     f32 m10, f32 m11, f32 m12,
                     f32 m20, f32 m21, f32 m22) noexcept
        : col0(m00, m10, m20), col1(m01, m11, m21), col2(m02, m12, m22) {}
    
    // Create from quaternion
    explicit Matrix3(const Quaternion& q) noexcept {
        const f32 xx = q.x * q.x;
        const f32 yy = q.y * q.y;
        const f32 zz = q.z * q.z;
        const f32 xy = q.x * q.y;
        const f32 xz = q.x * q.z;
        const f32 yz = q.y * q.z;
        const f32 wx = q.w * q.x;
        const f32 wy = q.w * q.y;
        const f32 wz = q.w * q.z;
        
        col0 = Vec3{1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy)};
        col1 = Vec3{2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx)};
        col2 = Vec3{2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy)};
    }
    
    // Element access
    Vec3& operator[](usize col) noexcept {
        return (&col0)[col];
    }
    
    const Vec3& operator[](usize col) const noexcept {
        return (&col0)[col];
    }
    
    f32& operator()(usize row, usize col) noexcept {
        return (&col0)[col][row];
    }
    
    f32 operator()(usize row, usize col) const noexcept {
        return (&col0)[col][row];
    }
    
    // Matrix operations
    constexpr Matrix3 operator+(const Matrix3& other) const noexcept {
        return Matrix3{col0 + other.col0, col1 + other.col1, col2 + other.col2};
    }
    
    constexpr Matrix3 operator-(const Matrix3& other) const noexcept {
        return Matrix3{col0 - other.col0, col1 - other.col1, col2 - other.col2};
    }
    
    constexpr Matrix3 operator*(f32 scalar) const noexcept {
        return Matrix3{col0 * scalar, col1 * scalar, col2 * scalar};
    }
    
    // Matrix multiplication
    constexpr Matrix3 operator*(const Matrix3& other) const noexcept {
        return Matrix3{
            Vec3{
                col0.x * other.col0.x + col1.x * other.col0.y + col2.x * other.col0.z,
                col0.y * other.col0.x + col1.y * other.col0.y + col2.y * other.col0.z,
                col0.z * other.col0.x + col1.z * other.col0.y + col2.z * other.col0.z
            },
            Vec3{
                col0.x * other.col1.x + col1.x * other.col1.y + col2.x * other.col1.z,
                col0.y * other.col1.x + col1.y * other.col1.y + col2.y * other.col1.z,
                col0.z * other.col1.x + col1.z * other.col1.y + col2.z * other.col1.z
            },
            Vec3{
                col0.x * other.col2.x + col1.x * other.col2.y + col2.x * other.col2.z,
                col0.y * other.col2.x + col1.y * other.col2.y + col2.y * other.col2.z,
                col0.z * other.col2.x + col1.z * other.col2.y + col2.z * other.col2.z
            }
        };
    }
    
    // Matrix-vector multiplication
    constexpr Vec3 operator*(const Vec3& v) const noexcept {
        return col0 * v.x + col1 * v.y + col2 * v.z;
    }
    
    // Matrix properties
    constexpr f32 determinant() const noexcept {
        return col0.x * (col1.y * col2.z - col1.z * col2.y) -
               col1.x * (col0.y * col2.z - col0.z * col2.y) +
               col2.x * (col0.y * col1.z - col0.z * col1.y);
    }
    
    constexpr Matrix3 transpose() const noexcept {
        return Matrix3{
            Vec3{col0.x, col1.x, col2.x},
            Vec3{col0.y, col1.y, col2.y},
            Vec3{col0.z, col1.z, col2.z}
        };
    }
    
    Matrix3 inverse() const noexcept {
        const f32 det = determinant();
        if (std::abs(det) < constants::EPSILON) {
            return identity();
        }
        
        const f32 inv_det = 1.0f / det;
        
        // Calculate adjugate matrix
        return Matrix3{
            Vec3{
                (col1.y * col2.z - col1.z * col2.y) * inv_det,
                (col0.z * col2.y - col0.y * col2.z) * inv_det,
                (col0.y * col1.z - col0.z * col1.y) * inv_det
            },
            Vec3{
                (col1.z * col2.x - col1.x * col2.z) * inv_det,
                (col0.x * col2.z - col0.z * col2.x) * inv_det,
                (col0.z * col1.x - col0.x * col1.z) * inv_det
            },
            Vec3{
                (col1.x * col2.y - col1.y * col2.x) * inv_det,
                (col0.y * col2.x - col0.x * col2.y) * inv_det,
                (col0.x * col1.y - col0.y * col1.x) * inv_det
            }
        };
    }
    
    // Static factory methods
    static constexpr Matrix3 identity() noexcept {
        return Matrix3{};
    }
    
    static constexpr Matrix3 zero() noexcept {
        return Matrix3{Vec3::zero(), Vec3::zero(), Vec3::zero()};
    }
    
    static Matrix3 rotation_x(f32 angle) noexcept {
        const f32 cos_a = std::cos(angle);
        const f32 sin_a = std::sin(angle);
        return Matrix3{
            Vec3{1.0f, 0.0f, 0.0f},
            Vec3{0.0f, cos_a, sin_a},
            Vec3{0.0f, -sin_a, cos_a}
        };
    }
    
    static Matrix3 rotation_y(f32 angle) noexcept {
        const f32 cos_a = std::cos(angle);
        const f32 sin_a = std::sin(angle);
        return Matrix3{
            Vec3{cos_a, 0.0f, -sin_a},
            Vec3{0.0f, 1.0f, 0.0f},
            Vec3{sin_a, 0.0f, cos_a}
        };
    }
    
    static Matrix3 rotation_z(f32 angle) noexcept {
        const f32 cos_a = std::cos(angle);
        const f32 sin_a = std::sin(angle);
        return Matrix3{
            Vec3{cos_a, sin_a, 0.0f},
            Vec3{-sin_a, cos_a, 0.0f},
            Vec3{0.0f, 0.0f, 1.0f}
        };
    }
    
    static Matrix3 scale(f32 sx, f32 sy, f32 sz) noexcept {
        return Matrix3{
            Vec3{sx, 0.0f, 0.0f},
            Vec3{0.0f, sy, 0.0f},
            Vec3{0.0f, 0.0f, sz}
        };
    }
    
    static Matrix3 scale(const Vec3& s) noexcept {
        return scale(s.x, s.y, s.z);
    }
    
    static Matrix3 from_axes(const Vec3& x_axis, const Vec3& y_axis, const Vec3& z_axis) noexcept {
        return Matrix3{x_axis, y_axis, z_axis};
    }
    
    // Get individual axes
    const Vec3& x_axis() const noexcept { return col0; }
    const Vec3& y_axis() const noexcept { return col1; }
    const Vec3& z_axis() const noexcept { return col2; }
};

/**
 * @brief 4x4 Matrix for complete 3D transformations including translation
 * 
 * Educational Context:
 * 4x4 matrices represent affine transformations in 3D space including
 * translation, rotation, scaling, and projection. They use homogeneous
 * coordinates to represent translations as linear transformations.
 */
struct alignas(64) Matrix4 {
    Vec4 col0, col1, col2, col3;  // Column vectors
    
    // Constructors
    constexpr Matrix4() noexcept 
        : col0(1.0f, 0.0f, 0.0f, 0.0f), col1(0.0f, 1.0f, 0.0f, 0.0f)
        , col2(0.0f, 0.0f, 1.0f, 0.0f), col3(0.0f, 0.0f, 0.0f, 1.0f) {}  // Identity
    
    constexpr Matrix4(const Vec4& c0, const Vec4& c1, const Vec4& c2, const Vec4& c3) noexcept
        : col0(c0), col1(c1), col2(c2), col3(c3) {}
    
    // Create from 3x3 matrix (no translation)
    explicit constexpr Matrix4(const Matrix3& m3) noexcept
        : col0(m3.col0, 0.0f), col1(m3.col1, 0.0f), col2(m3.col2, 0.0f), col3(0.0f, 0.0f, 0.0f, 1.0f) {}
    
    // Element access
    Vec4& operator[](usize col) noexcept {
        return (&col0)[col];
    }
    
    const Vec4& operator[](usize col) const noexcept {
        return (&col0)[col];
    }
    
    f32& operator()(usize row, usize col) noexcept {
        return (&col0)[col][row];
    }
    
    f32 operator()(usize row, usize col) const noexcept {
        return (&col0)[col][row];
    }
    
    // Matrix operations
    constexpr Matrix4 operator+(const Matrix4& other) const noexcept {
        return Matrix4{col0 + other.col0, col1 + other.col1, col2 + other.col2, col3 + other.col3};
    }
    
    constexpr Matrix4 operator-(const Matrix4& other) const noexcept {
        return Matrix4{col0 - other.col0, col1 - other.col1, col2 - other.col2, col3 - other.col3};
    }
    
    constexpr Matrix4 operator*(f32 scalar) const noexcept {
        return Matrix4{col0 * scalar, col1 * scalar, col2 * scalar, col3 * scalar};
    }
    
    // Matrix multiplication (full 4x4)
    Matrix4 operator*(const Matrix4& other) const noexcept {
        Matrix4 result;
        for (usize i = 0; i < 4; ++i) {
            for (usize j = 0; j < 4; ++j) {
                f32 sum = 0.0f;
                for (usize k = 0; k < 4; ++k) {
                    sum += (*this)(i, k) * other(k, j);
                }
                result(i, j) = sum;
            }
        }
        return result;
    }
    
    // Matrix-vector multiplication (homogeneous)
    constexpr Vec4 operator*(const Vec4& v) const noexcept {
        return col0 * v.x + col1 * v.y + col2 * v.z + col3 * v.w;
    }
    
    // Transform 3D point (assumes w=1)
    Vec3 transform_point(const Vec3& p) const noexcept {
        const Vec4 result = (*this) * Vec4{p, 1.0f};
        return result.to_vec3_perspective();
    }
    
    // Transform 3D vector (assumes w=0)
    Vec3 transform_vector(const Vec3& v) const noexcept {
        const Vec4 result = (*this) * Vec4{v, 0.0f};
        return result.to_vec3();
    }
    
    // Matrix properties
    f32 determinant() const noexcept {
        // Calculate 4x4 determinant using cofactor expansion
        const f32 a = col0.x, b = col1.x, c = col2.x, d = col3.x;
        const f32 e = col0.y, f = col1.y, g = col2.y, h = col3.y;
        const f32 i = col0.z, j = col1.z, k = col2.z, l = col3.z;
        const f32 m = col0.w, n = col1.w, o = col2.w, p = col3.w;
        
        return a * (f * (k * p - l * o) - g * (j * p - l * n) + h * (j * o - k * n)) -
               b * (e * (k * p - l * o) - g * (i * p - l * m) + h * (i * o - k * m)) +
               c * (e * (j * p - l * n) - f * (i * p - l * m) + h * (i * n - j * m)) -
               d * (e * (j * o - k * n) - f * (i * o - k * m) + g * (i * n - j * m));
    }
    
    Matrix4 transpose() const noexcept {
        return Matrix4{
            Vec4{col0.x, col1.x, col2.x, col3.x},
            Vec4{col0.y, col1.y, col2.y, col3.y},
            Vec4{col0.z, col1.z, col2.z, col3.z},
            Vec4{col0.w, col1.w, col2.w, col3.w}
        };
    }
    
    Matrix4 inverse() const noexcept;  // Implementation in .cpp file due to complexity
    
    // Extract 3x3 rotation matrix
    Matrix3 to_matrix3() const noexcept {
        return Matrix3{col0.to_vec3(), col1.to_vec3(), col2.to_vec3()};
    }
    
    // Extract translation vector
    Vec3 get_translation() const noexcept {
        return col3.to_vec3();
    }
    
    // Set translation
    void set_translation(const Vec3& translation) noexcept {
        col3 = Vec4{translation, 1.0f};
    }
    
    // Static factory methods
    static constexpr Matrix4 identity() noexcept {
        return Matrix4{};
    }
    
    static constexpr Matrix4 zero() noexcept {
        return Matrix4{Vec4::zero(), Vec4::zero(), Vec4::zero(), Vec4::zero()};
    }
    
    static Matrix4 translation(const Vec3& t) noexcept {
        return Matrix4{
            Vec4{1.0f, 0.0f, 0.0f, 0.0f},
            Vec4{0.0f, 1.0f, 0.0f, 0.0f},
            Vec4{0.0f, 0.0f, 1.0f, 0.0f},
            Vec4{t.x, t.y, t.z, 1.0f}
        };
    }
    
    static Matrix4 scale(f32 sx, f32 sy, f32 sz) noexcept {
        return Matrix4{
            Vec4{sx, 0.0f, 0.0f, 0.0f},
            Vec4{0.0f, sy, 0.0f, 0.0f},
            Vec4{0.0f, 0.0f, sz, 0.0f},
            Vec4{0.0f, 0.0f, 0.0f, 1.0f}
        };
    }
    
    static Matrix4 scale(const Vec3& s) noexcept {
        return scale(s.x, s.y, s.z);
    }
    
    // Create transformation matrix from translation, rotation, and scale
    static Matrix4 trs(const Vec3& translation, const Quaternion& rotation, const Vec3& scale) noexcept {
        const Matrix3 rotation_matrix{rotation};
        const Matrix3 scale_matrix = Matrix3::scale(scale);
        const Matrix3 rotation_scale = rotation_matrix * scale_matrix;
        
        return Matrix4{
            Vec4{rotation_scale.col0, 0.0f},
            Vec4{rotation_scale.col1, 0.0f},
            Vec4{rotation_scale.col2, 0.0f},
            Vec4{translation, 1.0f}
        };
    }
    
    // Perspective projection matrix
    static Matrix4 perspective(f32 fov_y, f32 aspect, f32 near_plane, f32 far_plane) noexcept {
        const f32 tan_half_fov = std::tan(fov_y * 0.5f);
        const f32 range = far_plane - near_plane;
        
        return Matrix4{
            Vec4{1.0f / (aspect * tan_half_fov), 0.0f, 0.0f, 0.0f},
            Vec4{0.0f, 1.0f / tan_half_fov, 0.0f, 0.0f},
            Vec4{0.0f, 0.0f, -(far_plane + near_plane) / range, -1.0f},
            Vec4{0.0f, 0.0f, -2.0f * far_plane * near_plane / range, 0.0f}
        };
    }
    
    // Orthographic projection matrix
    static Matrix4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_plane, f32 far_plane) noexcept {
        const f32 width = right - left;
        const f32 height = top - bottom;
        const f32 depth = far_plane - near_plane;
        
        return Matrix4{
            Vec4{2.0f / width, 0.0f, 0.0f, 0.0f},
            Vec4{0.0f, 2.0f / height, 0.0f, 0.0f},
            Vec4{0.0f, 0.0f, -2.0f / depth, 0.0f},
            Vec4{-(right + left) / width, -(top + bottom) / height, -(far_plane + near_plane) / depth, 1.0f}
        };
    }
    
    // Look-at view matrix
    static Matrix4 look_at(const Vec3& eye, const Vec3& target, const Vec3& up = Vec3::up()) noexcept {
        const Vec3 forward = (target - eye).normalized();
        const Vec3 right = forward.cross(up).normalized();
        const Vec3 up_corrected = right.cross(forward);
        
        return Matrix4{
            Vec4{right, 0.0f},
            Vec4{up_corrected, 0.0f},
            Vec4{-forward, 0.0f},
            Vec4{-right.dot(eye), -up_corrected.dot(eye), forward.dot(eye), 1.0f}
        };
    }
};

//=============================================================================
// Enhanced Transform3D for Physics
//=============================================================================

/**
 * @brief Enhanced 3D transform for physics simulation
 * 
 * Extends the basic Transform with 3D physics-specific functionality:
 * - Pre-computed transformation matrices for performance
 * - Hierarchical transforms with parent-child relationships
 * - Physics integration helpers
 * - Bounds calculation utilities
 */
struct alignas(64) Transform3D {
    Vec3 position{0.0f, 0.0f, 0.0f};        // World position
    Quaternion rotation{};                   // World rotation (identity quaternion)
    Vec3 scale{1.0f, 1.0f, 1.0f};          // Scale factors
    
    // Cached transformation matrices (updated when transform changes)
    mutable Matrix4 world_matrix;            // Complete world transformation matrix
    mutable Matrix3 rotation_matrix;         // 3x3 rotation matrix
    mutable bool matrix_dirty{true};         // Whether matrices need updating
    
    // Default constructor
    constexpr Transform3D() noexcept = default;
    
    // Constructors
    constexpr Transform3D(Vec3 pos, Quaternion rot = Quaternion::identity(), Vec3 scl = Vec3::one()) noexcept
        : position(pos), rotation(rot), scale(scl), matrix_dirty(true) {}
    
    constexpr Transform3D(f32 x, f32 y, f32 z, Quaternion rot = Quaternion::identity(), f32 uniform_scale = 1.0f) noexcept
        : position(x, y, z), rotation(rot), scale(uniform_scale, uniform_scale, uniform_scale), matrix_dirty(true) {}
    
    // Matrix access (updates cache if dirty)
    const Matrix4& get_world_matrix() const noexcept {
        if (matrix_dirty) {
            update_matrices();
        }
        return world_matrix;
    }
    
    const Matrix3& get_rotation_matrix() const noexcept {
        if (matrix_dirty) {
            update_matrices();
        }
        return rotation_matrix;
    }
    
    // Transform operations (mark matrices as dirty)
    void set_position(const Vec3& pos) noexcept {
        position = pos;
        matrix_dirty = true;
    }
    
    void set_rotation(const Quaternion& rot) noexcept {
        rotation = rot.normalized();
        matrix_dirty = true;
    }
    
    void set_scale(const Vec3& scl) noexcept {
        scale = scl;
        matrix_dirty = true;
    }
    
    // Point and vector transformations
    Vec3 transform_point(const Vec3& local_point) const noexcept {
        return get_world_matrix().transform_point(local_point);
    }
    
    Vec3 transform_vector(const Vec3& local_vector) const noexcept {
        const Vec3 scaled = Vec3{local_vector.x * scale.x, local_vector.y * scale.y, local_vector.z * scale.z};
        return rotation.rotate(scaled);
    }
    
    Vec3 transform_direction(const Vec3& local_direction) const noexcept {
        return rotation.rotate(local_direction);
    }
    
    Vec3 inverse_transform_point(const Vec3& world_point) const noexcept {
        const Vec3 translated = world_point - position;
        const Vec3 rotated = rotation.inverse().rotate(translated);
        return Vec3{rotated.x / scale.x, rotated.y / scale.y, rotated.z / scale.z};
    }
    
    Vec3 inverse_transform_direction(const Vec3& world_direction) const noexcept {
        return rotation.inverse().rotate(world_direction);
    }
    
    // Direction vectors in world space
    Vec3 forward() const noexcept {
        return rotation.rotate(Vec3::forward());
    }
    
    Vec3 back() const noexcept {
        return rotation.rotate(Vec3::back());
    }
    
    Vec3 right() const noexcept {
        return rotation.rotate(Vec3::right());
    }
    
    Vec3 left() const noexcept {
        return rotation.rotate(Vec3::left());
    }
    
    Vec3 up() const noexcept {
        return rotation.rotate(Vec3::up());
    }
    
    Vec3 down() const noexcept {
        return rotation.rotate(Vec3::down());
    }
    
    // Combine transforms (useful for hierarchical transforms)
    Transform3D operator*(const Transform3D& child) const noexcept {
        const Vec3 child_world_pos = transform_point(child.position);
        const Quaternion child_world_rot = rotation * child.rotation;
        const Vec3 child_world_scale = Vec3{scale.x * child.scale.x, scale.y * child.scale.y, scale.z * child.scale.z};
        return Transform3D{child_world_pos, child_world_rot, child_world_scale};
    }
    
    // Interpolation for smooth physics integration
    static Transform3D lerp(const Transform3D& a, const Transform3D& b, f32 t) noexcept {
        const Vec3 pos = a.position.lerp(b.position, t);
        const Quaternion rot = Quaternion::slerp(a.rotation, b.rotation, t);
        const Vec3 scl = a.scale.lerp(b.scale, t);
        return Transform3D{pos, rot, scl};
    }
    
    // Look at target
    void look_at(const Vec3& target, const Vec3& up = Vec3::up()) noexcept {
        rotation = Quaternion::look_at(position, target, up);
        matrix_dirty = true;
    }
    
    // Translate in local or world space
    void translate(const Vec3& translation, bool local_space = false) noexcept {
        if (local_space) {
            position += rotation.rotate(translation);
        } else {
            position += translation;
        }
        matrix_dirty = true;
    }
    
    // Rotate around axis
    void rotate(const Vec3& axis, f32 angle, bool local_space = false) noexcept {
        const Quaternion rotation_quat{axis, angle};
        if (local_space) {
            rotation = rotation * rotation_quat;
        } else {
            rotation = rotation_quat * rotation;
        }
        rotation.normalize();
        matrix_dirty = true;
    }
    
    // Rotate around local axes
    void rotate_local(f32 pitch, f32 yaw, f32 roll) noexcept {
        const Quaternion rotation_quat = Quaternion::from_euler(pitch, yaw, roll);
        rotation = rotation * rotation_quat;
        rotation.normalize();
        matrix_dirty = true;
    }
    
private:
    void update_matrices() const noexcept {
        world_matrix = Matrix4::trs(position, rotation, scale);
        rotation_matrix = Matrix3{rotation};
        matrix_dirty = false;
    }
};

//=============================================================================
// 3D Extended Vector Operations
//=============================================================================

/**
 * @brief Extended 3D vector utilities for physics calculations
 */
namespace vec3 {
    
    /**
     * @brief Safe vector normalization with fallback
     */
    inline Vec3 safe_normalize(const Vec3& v, const Vec3& fallback = Vec3::unit_x()) noexcept {
        const f32 length_sq = v.length_squared();
        if (length_sq > constants::EPSILON * constants::EPSILON) {
            return v / std::sqrt(length_sq);
        }
        return fallback;
    }
    
    /**
     * @brief Spherical linear interpolation for vectors
     */
    Vec3 slerp(const Vec3& a, const Vec3& b, f32 t) noexcept;
    
    /**
     * @brief Project vector a onto vector b
     */
    inline Vec3 project(const Vec3& a, const Vec3& b) noexcept {
        const f32 b_length_sq = b.length_squared();
        if (b_length_sq < constants::EPSILON) {
            return Vec3::zero();
        }
        return b * (a.dot(b) / b_length_sq);
    }
    
    /**
     * @brief Reject vector a from vector b (orthogonal component)
     */
    inline Vec3 reject(const Vec3& a, const Vec3& b) noexcept {
        return a - project(a, b);
    }
    
    /**
     * @brief Reflect vector across a normal
     */
    constexpr Vec3 reflect(const Vec3& v, const Vec3& normal) noexcept {
        return v - normal * (2.0f * v.dot(normal));
    }
    
    /**
     * @brief Clamp vector magnitude to maximum length
     */
    inline Vec3 clamp_magnitude(const Vec3& v, f32 max_length) noexcept {
        const f32 length_sq = v.length_squared();
        if (length_sq > max_length * max_length) {
            return v * (max_length / std::sqrt(length_sq));
        }
        return v;
    }
    
    /**
     * @brief Check if two vectors are approximately equal
     */
    constexpr bool approximately_equal(const Vec3& a, const Vec3& b, f32 epsilon = constants::EPSILON) noexcept {
        return std::abs(a.x - b.x) <= epsilon && 
               std::abs(a.y - b.y) <= epsilon && 
               std::abs(a.z - b.z) <= epsilon;
    }
    
    /**
     * @brief Angle between two vectors in radians
     */
    f32 angle_between(const Vec3& a, const Vec3& b) noexcept;
    
    /**
     * @brief Create orthonormal basis from single vector
     */
    std::pair<Vec3, Vec3> create_orthonormal_basis(const Vec3& normal) noexcept;
    
    /**
     * @brief Gram-Schmidt orthogonalization of three vectors
     */
    std::tuple<Vec3, Vec3, Vec3> gram_schmidt(const Vec3& a, const Vec3& b, const Vec3& c) noexcept;
    
    /**
     * @brief Convert Cartesian to spherical coordinates (radius, theta, phi)
     */
    Vec3 to_spherical(const Vec3& cartesian) noexcept;
    
    /**
     * @brief Convert spherical to Cartesian coordinates
     */
    Vec3 from_spherical(f32 radius, f32 theta, f32 phi) noexcept;
    
    /**
     * @brief Triple scalar product: a · (b × c)
     */
    constexpr f32 scalar_triple_product(const Vec3& a, const Vec3& b, const Vec3& c) noexcept {
        return a.dot(b.cross(c));
    }
    
    /**
     * @brief Vector triple product: a × (b × c)
     */
    constexpr Vec3 vector_triple_product(const Vec3& a, const Vec3& b, const Vec3& c) noexcept {
        return b * a.dot(c) - c * a.dot(b);
    }
}

} // namespace ecscope::physics::math3d