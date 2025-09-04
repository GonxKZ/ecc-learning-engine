#include "math3d.hpp"
#include <algorithm>
#include <cmath>

namespace ecscope::physics::math3d {

//=============================================================================
// Quaternion Implementation
//=============================================================================

Quaternion Quaternion::from_rotation_matrix(const Matrix3& mat) noexcept {
    // Shepperd's method for converting rotation matrix to quaternion
    const f32 trace = mat.col0.x + mat.col1.y + mat.col2.z;
    
    if (trace > 0.0f) {
        const f32 s = std::sqrt(trace + 1.0f) * 2.0f; // s = 4 * w
        return Quaternion{
            (mat.col1.z - mat.col2.y) / s,
            (mat.col2.x - mat.col0.z) / s,
            (mat.col0.y - mat.col1.x) / s,
            0.25f * s
        }.normalized();
    } else if (mat.col0.x > mat.col1.y && mat.col0.x > mat.col2.z) {
        const f32 s = std::sqrt(1.0f + mat.col0.x - mat.col1.y - mat.col2.z) * 2.0f; // s = 4 * x
        return Quaternion{
            0.25f * s,
            (mat.col1.x + mat.col0.y) / s,
            (mat.col2.x + mat.col0.z) / s,
            (mat.col1.z - mat.col2.y) / s
        }.normalized();
    } else if (mat.col1.y > mat.col2.z) {
        const f32 s = std::sqrt(1.0f + mat.col1.y - mat.col0.x - mat.col2.z) * 2.0f; // s = 4 * y
        return Quaternion{
            (mat.col1.x + mat.col0.y) / s,
            0.25f * s,
            (mat.col2.y + mat.col1.z) / s,
            (mat.col2.x - mat.col0.z) / s
        }.normalized();
    } else {
        const f32 s = std::sqrt(1.0f + mat.col2.z - mat.col0.x - mat.col1.y) * 2.0f; // s = 4 * z
        return Quaternion{
            (mat.col2.x + mat.col0.z) / s,
            (mat.col2.y + mat.col1.z) / s,
            0.25f * s,
            (mat.col0.y - mat.col1.x) / s
        }.normalized();
    }
}

//=============================================================================
// Matrix4 Implementation
//=============================================================================

Matrix4 Matrix4::inverse() const noexcept {
    // Calculate inverse using Gauss-Jordan elimination
    // This is a comprehensive implementation for educational purposes
    
    const f32 det = determinant();
    if (std::abs(det) < constants::EPSILON) {
        return identity(); // Return identity if matrix is singular
    }
    
    const f32 inv_det = 1.0f / det;
    
    // Calculate cofactor matrix
    Matrix4 cofactor;
    
    // Row 0
    cofactor(0, 0) = ((*this)(1,1) * ((*this)(2,2) * (*this)(3,3) - (*this)(2,3) * (*this)(3,2)) -
                     (*this)(1,2) * ((*this)(2,1) * (*this)(3,3) - (*this)(2,3) * (*this)(3,1)) +
                     (*this)(1,3) * ((*this)(2,1) * (*this)(3,2) - (*this)(2,2) * (*this)(3,1))) * inv_det;
    
    cofactor(0, 1) = -((*this)(0,1) * ((*this)(2,2) * (*this)(3,3) - (*this)(2,3) * (*this)(3,2)) -
                      (*this)(0,2) * ((*this)(2,1) * (*this)(3,3) - (*this)(2,3) * (*this)(3,1)) +
                      (*this)(0,3) * ((*this)(2,1) * (*this)(3,2) - (*this)(2,2) * (*this)(3,1))) * inv_det;
    
    cofactor(0, 2) = ((*this)(0,1) * ((*this)(1,2) * (*this)(3,3) - (*this)(1,3) * (*this)(3,2)) -
                     (*this)(0,2) * ((*this)(1,1) * (*this)(3,3) - (*this)(1,3) * (*this)(3,1)) +
                     (*this)(0,3) * ((*this)(1,1) * (*this)(3,2) - (*this)(1,2) * (*this)(3,1))) * inv_det;
    
    cofactor(0, 3) = -((*this)(0,1) * ((*this)(1,2) * (*this)(2,3) - (*this)(1,3) * (*this)(2,2)) -
                      (*this)(0,2) * ((*this)(1,1) * (*this)(2,3) - (*this)(1,3) * (*this)(2,1)) +
                      (*this)(0,3) * ((*this)(1,1) * (*this)(2,2) - (*this)(1,2) * (*this)(2,1))) * inv_det;
    
    // Row 1
    cofactor(1, 0) = -((*this)(1,0) * ((*this)(2,2) * (*this)(3,3) - (*this)(2,3) * (*this)(3,2)) -
                      (*this)(1,2) * ((*this)(2,0) * (*this)(3,3) - (*this)(2,3) * (*this)(3,0)) +
                      (*this)(1,3) * ((*this)(2,0) * (*this)(3,2) - (*this)(2,2) * (*this)(3,0))) * inv_det;
    
    cofactor(1, 1) = ((*this)(0,0) * ((*this)(2,2) * (*this)(3,3) - (*this)(2,3) * (*this)(3,2)) -
                     (*this)(0,2) * ((*this)(2,0) * (*this)(3,3) - (*this)(2,3) * (*this)(3,0)) +
                     (*this)(0,3) * ((*this)(2,0) * (*this)(3,2) - (*this)(2,2) * (*this)(3,0))) * inv_det;
    
    cofactor(1, 2) = -((*this)(0,0) * ((*this)(1,2) * (*this)(3,3) - (*this)(1,3) * (*this)(3,2)) -
                      (*this)(0,2) * ((*this)(1,0) * (*this)(3,3) - (*this)(1,3) * (*this)(3,0)) +
                      (*this)(0,3) * ((*this)(1,0) * (*this)(3,2) - (*this)(1,2) * (*this)(3,0))) * inv_det;
    
    cofactor(1, 3) = ((*this)(0,0) * ((*this)(1,2) * (*this)(2,3) - (*this)(1,3) * (*this)(2,2)) -
                     (*this)(0,2) * ((*this)(1,0) * (*this)(2,3) - (*this)(1,3) * (*this)(2,0)) +
                     (*this)(0,3) * ((*this)(1,0) * (*this)(2,2) - (*this)(1,2) * (*this)(2,0))) * inv_det;
    
    // Row 2
    cofactor(2, 0) = ((*this)(1,0) * ((*this)(2,1) * (*this)(3,3) - (*this)(2,3) * (*this)(3,1)) -
                     (*this)(1,1) * ((*this)(2,0) * (*this)(3,3) - (*this)(2,3) * (*this)(3,0)) +
                     (*this)(1,3) * ((*this)(2,0) * (*this)(3,1) - (*this)(2,1) * (*this)(3,0))) * inv_det;
    
    cofactor(2, 1) = -((*this)(0,0) * ((*this)(2,1) * (*this)(3,3) - (*this)(2,3) * (*this)(3,1)) -
                      (*this)(0,1) * ((*this)(2,0) * (*this)(3,3) - (*this)(2,3) * (*this)(3,0)) +
                      (*this)(0,3) * ((*this)(2,0) * (*this)(3,1) - (*this)(2,1) * (*this)(3,0))) * inv_det;
    
    cofactor(2, 2) = ((*this)(0,0) * ((*this)(1,1) * (*this)(3,3) - (*this)(1,3) * (*this)(3,1)) -
                     (*this)(0,1) * ((*this)(1,0) * (*this)(3,3) - (*this)(1,3) * (*this)(3,0)) +
                     (*this)(0,3) * ((*this)(1,0) * (*this)(3,1) - (*this)(1,1) * (*this)(3,0))) * inv_det;
    
    cofactor(2, 3) = -((*this)(0,0) * ((*this)(1,1) * (*this)(2,3) - (*this)(1,3) * (*this)(2,1)) -
                      (*this)(0,1) * ((*this)(1,0) * (*this)(2,3) - (*this)(1,3) * (*this)(2,0)) +
                      (*this)(0,3) * ((*this)(1,0) * (*this)(2,1) - (*this)(1,1) * (*this)(2,0))) * inv_det;
    
    // Row 3
    cofactor(3, 0) = -((*this)(1,0) * ((*this)(2,1) * (*this)(3,2) - (*this)(2,2) * (*this)(3,1)) -
                      (*this)(1,1) * ((*this)(2,0) * (*this)(3,2) - (*this)(2,2) * (*this)(3,0)) +
                      (*this)(1,2) * ((*this)(2,0) * (*this)(3,1) - (*this)(2,1) * (*this)(3,0))) * inv_det;
    
    cofactor(3, 1) = ((*this)(0,0) * ((*this)(2,1) * (*this)(3,2) - (*this)(2,2) * (*this)(3,1)) -
                     (*this)(0,1) * ((*this)(2,0) * (*this)(3,2) - (*this)(2,2) * (*this)(3,0)) +
                     (*this)(0,2) * ((*this)(2,0) * (*this)(3,1) - (*this)(2,1) * (*this)(3,0))) * inv_det;
    
    cofactor(3, 2) = -((*this)(0,0) * ((*this)(1,1) * (*this)(3,2) - (*this)(1,2) * (*this)(3,1)) -
                      (*this)(0,1) * ((*this)(1,0) * (*this)(3,2) - (*this)(1,2) * (*this)(3,0)) +
                      (*this)(0,2) * ((*this)(1,0) * (*this)(3,1) - (*this)(1,1) * (*this)(3,0))) * inv_det;
    
    cofactor(3, 3) = ((*this)(0,0) * ((*this)(1,1) * (*this)(2,2) - (*this)(1,2) * (*this)(2,1)) -
                     (*this)(0,1) * ((*this)(1,0) * (*this)(2,2) - (*this)(1,2) * (*this)(2,0)) +
                     (*this)(0,2) * ((*this)(1,0) * (*this)(2,1) - (*this)(1,1) * (*this)(2,0))) * inv_det;
    
    return cofactor;
}

//=============================================================================
// Extended Vec3 Operations Implementation
//=============================================================================

namespace vec3 {

Vec3 slerp(const Vec3& a, const Vec3& b, f32 t) noexcept {
    const f32 dot = std::clamp(a.dot(b), -1.0f, 1.0f);
    
    // If vectors are nearly parallel, use linear interpolation
    if (std::abs(dot) > 0.9995f) {
        return a.lerp(b, t).normalized();
    }
    
    const f32 theta = std::acos(std::abs(dot));
    const f32 sin_theta = std::sin(theta);
    
    const f32 a_factor = std::sin((1.0f - t) * theta) / sin_theta;
    const f32 b_factor = std::sin(t * theta) / sin_theta;
    
    Vec3 result = a * a_factor + b * b_factor;
    
    // Handle negative dot product case
    if (dot < 0.0f) {
        result = a * a_factor - b * b_factor;
    }
    
    return result.normalized();
}

f32 angle_between(const Vec3& a, const Vec3& b) noexcept {
    const f32 a_length = a.length();
    const f32 b_length = b.length();
    
    if (a_length < constants::EPSILON || b_length < constants::EPSILON) {
        return 0.0f;
    }
    
    const f32 cos_angle = std::clamp(a.dot(b) / (a_length * b_length), -1.0f, 1.0f);
    return std::acos(cos_angle);
}

std::pair<Vec3, Vec3> create_orthonormal_basis(const Vec3& normal) noexcept {
    const Vec3 n = normal.normalized();
    
    // Choose an arbitrary vector not parallel to normal
    Vec3 arbitrary = Vec3::unit_x();
    if (std::abs(n.dot(arbitrary)) > 0.9f) {
        arbitrary = Vec3::unit_y();
    }
    
    const Vec3 tangent = n.cross(arbitrary).normalized();
    const Vec3 bitangent = n.cross(tangent);
    
    return {tangent, bitangent};
}

std::tuple<Vec3, Vec3, Vec3> gram_schmidt(const Vec3& a, const Vec3& b, const Vec3& c) noexcept {
    // Gram-Schmidt orthogonalization process
    const Vec3 u1 = a.normalized();
    
    const Vec3 u2_unnormalized = b - project(b, u1);
    const Vec3 u2 = safe_normalize(u2_unnormalized, Vec3::unit_y());
    
    const Vec3 u3_unnormalized = c - project(c, u1) - project(c, u2);
    const Vec3 u3 = safe_normalize(u3_unnormalized, u1.cross(u2));
    
    return {u1, u2, u3};
}

Vec3 to_spherical(const Vec3& cartesian) noexcept {
    const f32 radius = cartesian.length();
    if (radius < constants::EPSILON) {
        return Vec3::zero();
    }
    
    const f32 theta = std::atan2(cartesian.y, cartesian.x);  // Azimuthal angle
    const f32 phi = std::acos(std::clamp(cartesian.z / radius, -1.0f, 1.0f));  // Polar angle
    
    return Vec3{radius, theta, phi};
}

Vec3 from_spherical(f32 radius, f32 theta, f32 phi) noexcept {
    const f32 sin_phi = std::sin(phi);
    return Vec3{
        radius * sin_phi * std::cos(theta),
        radius * sin_phi * std::sin(theta),
        radius * std::cos(phi)
    };
}

} // namespace vec3

} // namespace ecscope::physics::math3d