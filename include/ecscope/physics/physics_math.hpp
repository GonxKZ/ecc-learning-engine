#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include <immintrin.h>
#include <cassert>

namespace ecscope::physics {

// Precision control for deterministic simulation
using Real = float;  // Can be changed to double for higher precision
constexpr Real PHYSICS_EPSILON = 1e-6f;
constexpr Real PI = 3.14159265359f;

// SIMD-aligned vector classes for high performance
struct alignas(16) Vec2 {
    Real x, y;
    
    Vec2() : x(0), y(0) {}
    Vec2(Real x, Real y) : x(x), y(y) {}
    
    // Arithmetic operations
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(Real scalar) const { return {x * scalar, y * scalar}; }
    Vec2 operator/(Real scalar) const { return {x / scalar, y / scalar}; }
    
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    Vec2& operator*=(Real scalar) { x *= scalar; y *= scalar; return *this; }
    Vec2& operator/=(Real scalar) { x /= scalar; y /= scalar; return *this; }
    
    // Vector operations
    Real dot(const Vec2& other) const { return x * other.x + y * other.y; }
    Real cross(const Vec2& other) const { return x * other.y - y * other.x; }
    Real length_squared() const { return x * x + y * y; }
    Real length() const { return std::sqrt(length_squared()); }
    
    Vec2 normalized() const {
        Real len = length();
        return len > PHYSICS_EPSILON ? *this / len : Vec2{0, 0};
    }
    
    Vec2 perpendicular() const { return {-y, x}; }
    
    static Vec2 zero() { return {0, 0}; }
    static Vec2 unit_x() { return {1, 0}; }
    static Vec2 unit_y() { return {0, 1}; }
};

struct alignas(16) Vec3 {
    Real x, y, z;
    Real w; // Padding for 16-byte alignment
    
    Vec3() : x(0), y(0), z(0), w(0) {}
    Vec3(Real x, Real y, Real z) : x(x), y(y), z(z), w(0) {}
    
    // Arithmetic operations
    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator*(Real scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    Vec3 operator/(Real scalar) const { return {x / scalar, y / scalar, z / scalar}; }
    
    Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vec3& operator-=(const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vec3& operator*=(Real scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    Vec3& operator/=(Real scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }
    
    // Vector operations
    Real dot(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }
    Vec3 cross(const Vec3& other) const {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }
    
    Real length_squared() const { return x * x + y * y + z * z; }
    Real length() const { return std::sqrt(length_squared()); }
    
    Vec3 normalized() const {
        Real len = length();
        return len > PHYSICS_EPSILON ? *this / len : Vec3{0, 0, 0};
    }
    
    static Vec3 zero() { return {0, 0, 0}; }
    static Vec3 unit_x() { return {1, 0, 0}; }
    static Vec3 unit_y() { return {0, 1, 0}; }
    static Vec3 unit_z() { return {0, 0, 1}; }
};

// Quaternion for 3D rotations
struct alignas(16) Quaternion {
    Real x, y, z, w;
    
    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(Real x, Real y, Real z, Real w) : x(x), y(y), z(z), w(w) {}
    
    static Quaternion from_axis_angle(const Vec3& axis, Real angle) {
        Real half_angle = angle * 0.5f;
        Real sin_half = std::sin(half_angle);
        Vec3 norm_axis = axis.normalized();
        return {
            norm_axis.x * sin_half,
            norm_axis.y * sin_half,
            norm_axis.z * sin_half,
            std::cos(half_angle)
        };
    }
    
    Quaternion operator*(const Quaternion& other) const {
        return {
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        };
    }
    
    Vec3 rotate_vector(const Vec3& v) const {
        // v' = q * v * q^-1 (optimized version)
        Vec3 u{x, y, z};
        Real s = w;
        
        return u * (2.0f * u.dot(v)) + 
               v * (s * s - u.dot(u)) + 
               u.cross(v) * (2.0f * s);
    }
    
    Quaternion normalized() const {
        Real len = std::sqrt(x * x + y * y + z * z + w * w);
        return len > PHYSICS_EPSILON ? Quaternion{x/len, y/len, z/len, w/len} : Quaternion{};
    }
    
    static Quaternion identity() { return {0, 0, 0, 1}; }
};

// 3x3 Matrix for inertia tensors and rotations
struct alignas(16) Mat3 {
    std::array<Real, 9> data;
    
    Mat3() { std::fill(data.begin(), data.end(), 0); }
    
    Real& operator()(int row, int col) { return data[row * 3 + col]; }
    const Real& operator()(int row, int col) const { return data[row * 3 + col]; }
    
    Vec3 operator*(const Vec3& v) const {
        return {
            data[0] * v.x + data[1] * v.y + data[2] * v.z,
            data[3] * v.x + data[4] * v.y + data[5] * v.z,
            data[6] * v.x + data[7] * v.y + data[8] * v.z
        };
    }
    
    Mat3 operator*(const Mat3& other) const {
        Mat3 result;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                for (int k = 0; k < 3; ++k) {
                    result(i, j) += (*this)(i, k) * other(k, j);
                }
            }
        }
        return result;
    }
    
    Mat3 transposed() const {
        Mat3 result;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result(i, j) = (*this)(j, i);
            }
        }
        return result;
    }
    
    Mat3 inverse() const {
        // Compute determinant
        Real det = data[0] * (data[4] * data[8] - data[5] * data[7]) -
                   data[1] * (data[3] * data[8] - data[5] * data[6]) +
                   data[2] * (data[3] * data[7] - data[4] * data[6]);
        
        if (std::abs(det) < PHYSICS_EPSILON) {
            return identity(); // Return identity if singular
        }
        
        Real inv_det = 1.0f / det;
        Mat3 result;
        
        result.data[0] = (data[4] * data[8] - data[5] * data[7]) * inv_det;
        result.data[1] = (data[2] * data[7] - data[1] * data[8]) * inv_det;
        result.data[2] = (data[1] * data[5] - data[2] * data[4]) * inv_det;
        result.data[3] = (data[5] * data[6] - data[3] * data[8]) * inv_det;
        result.data[4] = (data[0] * data[8] - data[2] * data[6]) * inv_det;
        result.data[5] = (data[2] * data[3] - data[0] * data[5]) * inv_det;
        result.data[6] = (data[3] * data[7] - data[4] * data[6]) * inv_det;
        result.data[7] = (data[1] * data[6] - data[0] * data[7]) * inv_det;
        result.data[8] = (data[0] * data[4] - data[1] * data[3]) * inv_det;
        
        return result;
    }
    
    static Mat3 identity() {
        Mat3 result;
        result.data[0] = result.data[4] = result.data[8] = 1.0f;
        return result;
    }
    
    static Mat3 from_quaternion(const Quaternion& q) {
        Mat3 result;
        Real xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        Real xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        Real wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
        
        result.data[0] = 1.0f - 2.0f * (yy + zz);
        result.data[1] = 2.0f * (xy - wz);
        result.data[2] = 2.0f * (xz + wy);
        result.data[3] = 2.0f * (xy + wz);
        result.data[4] = 1.0f - 2.0f * (xx + zz);
        result.data[5] = 2.0f * (yz - wx);
        result.data[6] = 2.0f * (xz - wy);
        result.data[7] = 2.0f * (yz + wx);
        result.data[8] = 1.0f - 2.0f * (xx + yy);
        
        return result;
    }
};

// Transform for rigid bodies
struct Transform2D {
    Vec2 position;
    Real rotation;
    
    Transform2D() : position(0, 0), rotation(0) {}
    Transform2D(const Vec2& pos, Real rot) : position(pos), rotation(rot) {}
    
    Vec2 transform_point(const Vec2& point) const {
        Real cos_r = std::cos(rotation);
        Real sin_r = std::sin(rotation);
        return {
            position.x + point.x * cos_r - point.y * sin_r,
            position.y + point.x * sin_r + point.y * cos_r
        };
    }
    
    Vec2 transform_vector(const Vec2& vector) const {
        Real cos_r = std::cos(rotation);
        Real sin_r = std::sin(rotation);
        return {
            vector.x * cos_r - vector.y * sin_r,
            vector.x * sin_r + vector.y * cos_r
        };
    }
};

struct Transform3D {
    Vec3 position;
    Quaternion rotation;
    
    Transform3D() : position(0, 0, 0), rotation(0, 0, 0, 1) {}
    Transform3D(const Vec3& pos, const Quaternion& rot) : position(pos), rotation(rot) {}
    
    Vec3 transform_point(const Vec3& point) const {
        return position + rotation.rotate_vector(point);
    }
    
    Vec3 transform_vector(const Vec3& vector) const {
        return rotation.rotate_vector(vector);
    }
    
    Mat3 rotation_matrix() const {
        return Mat3::from_quaternion(rotation);
    }
};

// Utility functions for physics calculations
inline Real clamp(Real value, Real min_val, Real max_val) {
    return std::max(min_val, std::min(value, max_val));
}

inline Real lerp(Real a, Real b, Real t) {
    return a + t * (b - a);
}

inline Vec2 lerp(const Vec2& a, const Vec2& b, Real t) {
    return {lerp(a.x, b.x, t), lerp(a.y, b.y, t)};
}

inline Vec3 lerp(const Vec3& a, const Vec3& b, Real t) {
    return {lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t)};
}

// SIMD-optimized vector operations for bulk calculations
#ifdef __AVX__
inline void bulk_add_vectors_2d(const Vec2* a, const Vec2* b, Vec2* result, size_t count) {
    size_t simd_count = count & ~3; // Process 4 Vec2s at a time
    
    for (size_t i = 0; i < simd_count; i += 4) {
        __m256 va = _mm256_load_ps(reinterpret_cast<const float*>(&a[i]));
        __m256 vb = _mm256_load_ps(reinterpret_cast<const float*>(&b[i]));
        __m256 vresult = _mm256_add_ps(va, vb);
        _mm256_store_ps(reinterpret_cast<float*>(&result[i]), vresult);
    }
    
    // Handle remaining elements
    for (size_t i = simd_count; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
}
#endif

} // namespace ecscope::physics