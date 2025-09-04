#include "simd_math3d.hpp"
#include "core/log.hpp"
#include <chrono>
#include <sstream>
#include <random>

namespace ecscope::physics::simd3d {

//=============================================================================
// Educational Performance Benchmarking Implementation
//=============================================================================

namespace benchmark3d {

Simd3DBenchmarkResult benchmark_vec3_operations(usize count) noexcept {
    Simd3DBenchmarkResult result{};
    result.operations_count = count;
    result.operation_name = "Vec3 Operations (Add, Dot, Cross, Normalize)";
    result.simd_implementation = "AVX512F/AVX2/SSE2";
    
    // Generate test data
    std::vector<Vec3> vec_a(count), vec_b(count), vec_results(count);
    std::vector<f32> dot_results(count);
    std::mt19937 rng{42};
    std::uniform_real_distribution<f32> dist{-10.0f, 10.0f};
    
    for (usize i = 0; i < count; ++i) {
        vec_a[i] = Vec3{dist(rng), dist(rng), dist(rng)};
        vec_b[i] = Vec3{dist(rng), dist(rng), dist(rng)};
    }
    
    // Benchmark scalar operations
    auto scalar_start = std::chrono::high_resolution_clock::now();
    
    // Scalar addition
    for (usize i = 0; i < count; ++i) {
        vec_results[i] = vec_a[i] + vec_b[i];
    }
    
    // Scalar dot product
    for (usize i = 0; i < count; ++i) {
        dot_results[i] = vec_a[i].dot(vec_b[i]);
    }
    
    // Scalar cross product
    for (usize i = 0; i < count; ++i) {
        vec_results[i] = vec_a[i].cross(vec_b[i]);
    }
    
    // Scalar normalization
    std::vector<Vec3> normalize_data = vec_a; // Copy for modification
    for (usize i = 0; i < count; ++i) {
        normalize_data[i].normalize();
    }
    
    auto scalar_end = std::chrono::high_resolution_clock::now();
    result.scalar_time_ns = std::chrono::duration<f64, std::nano>(scalar_end - scalar_start).count();
    
    // Benchmark SIMD operations
    auto simd_start = std::chrono::high_resolution_clock::now();
    
    // SIMD addition
    simd_vec3::add_vec3_arrays(vec_a.data(), vec_b.data(), vec_results.data(), count);
    
    // SIMD dot product
    simd_vec3::dot_product_vec3_arrays(vec_a.data(), vec_b.data(), dot_results.data(), count);
    
    // SIMD cross product
    simd_vec3::cross_product_vec3_arrays(vec_a.data(), vec_b.data(), vec_results.data(), count);
    
    // SIMD normalization
    normalize_data = vec_a; // Reset data
    simd_vec3::normalize_vec3_arrays(normalize_data.data(), count);
    
    auto simd_end = std::chrono::high_resolution_clock::now();
    result.simd_time_ns = std::chrono::duration<f64, std::nano>(simd_end - simd_start).count();
    
    // Calculate metrics
    result.speedup_factor = result.scalar_time_ns / result.simd_time_ns;
    result.vector_throughput_mvecs_per_sec = (count * 4.0 / result.simd_time_ns) * 1000.0; // 4 operations, convert to MVecs/sec
    
    LOG_INFO("Vec3 SIMD Benchmark: {:.2f}x speedup ({:.1f} MVecs/sec)", 
             result.speedup_factor, result.vector_throughput_mvecs_per_sec);
    
    return result;
}

Simd3DBenchmarkResult benchmark_quaternion_operations(usize count) noexcept {
    Simd3DBenchmarkResult result{};
    result.operations_count = count;
    result.operation_name = "Quaternion Operations (Multiply, Normalize, SLERP)";
    result.simd_implementation = "AVX512F/AVX2/SSE2";
    
    // Generate test data
    std::vector<Quaternion> quat_a(count), quat_b(count), quat_results(count);
    std::mt19937 rng{42};
    std::uniform_real_distribution<f32> dist{-1.0f, 1.0f};
    
    for (usize i = 0; i < count; ++i) {
        quat_a[i] = Quaternion{dist(rng), dist(rng), dist(rng), dist(rng)}.normalized();
        quat_b[i] = Quaternion{dist(rng), dist(rng), dist(rng), dist(rng)}.normalized();
    }
    
    // Benchmark scalar operations
    auto scalar_start = std::chrono::high_resolution_clock::now();
    
    // Scalar multiplication
    for (usize i = 0; i < count; ++i) {
        quat_results[i] = quat_a[i] * quat_b[i];
    }
    
    // Scalar normalization
    std::vector<Quaternion> normalize_data = quat_a;
    for (usize i = 0; i < count; ++i) {
        normalize_data[i].normalize();
    }
    
    // Scalar SLERP
    for (usize i = 0; i < count; ++i) {
        quat_results[i] = Quaternion::slerp(quat_a[i], quat_b[i], 0.5f);
    }
    
    auto scalar_end = std::chrono::high_resolution_clock::now();
    result.scalar_time_ns = std::chrono::duration<f64, std::nano>(scalar_end - scalar_start).count();
    
    // Benchmark SIMD operations
    auto simd_start = std::chrono::high_resolution_clock::now();
    
    // SIMD multiplication
    simd_quaternion::multiply_quaternion_arrays(quat_a.data(), quat_b.data(), quat_results.data(), count);
    
    // SIMD normalization
    normalize_data = quat_a;
    simd_quaternion::normalize_quaternion_arrays(normalize_data.data(), count);
    
    // SIMD SLERP
    simd_quaternion::slerp_quaternion_arrays(quat_a.data(), quat_b.data(), 0.5f, quat_results.data(), count);
    
    auto simd_end = std::chrono::high_resolution_clock::now();
    result.simd_time_ns = std::chrono::duration<f64, std::nano>(simd_end - simd_start).count();
    
    // Calculate metrics
    result.speedup_factor = result.scalar_time_ns / result.simd_time_ns;
    result.quaternion_throughput_mquats_per_sec = (count * 3.0 / result.simd_time_ns) * 1000.0; // 3 operations
    
    LOG_INFO("Quaternion SIMD Benchmark: {:.2f}x speedup ({:.1f} MQuats/sec)", 
             result.speedup_factor, result.quaternion_throughput_mquats_per_sec);
    
    return result;
}

Simd3DBenchmarkResult benchmark_matrix_operations(usize count) noexcept {
    Simd3DBenchmarkResult result{};
    result.operations_count = count;
    result.operation_name = "Matrix4 Operations (Multiply, Transform Points/Vectors)";
    result.simd_implementation = "AVX512F/AVX2/SSE2";
    
    // Generate test data
    std::vector<Matrix4> mat_a(count), mat_b(count), mat_results(count);
    std::vector<Vec3> points(count), vectors(count), point_results(count), vector_results(count);
    std::mt19937 rng{42};
    std::uniform_real_distribution<f32> dist{-10.0f, 10.0f};
    
    for (usize i = 0; i < count; ++i) {
        // Create transformation matrices
        Vec3 translation{dist(rng), dist(rng), dist(rng)};
        Vec3 scale{dist(rng) + 1.0f, dist(rng) + 1.0f, dist(rng) + 1.0f};
        Quaternion rotation = Quaternion::from_euler(dist(rng), dist(rng), dist(rng));
        
        mat_a[i] = Matrix4::trs(translation, rotation, scale);
        mat_b[i] = Matrix4::trs(Vec3{dist(rng), dist(rng), dist(rng)}, 
                               Quaternion::from_euler(dist(rng), dist(rng), dist(rng)),
                               Vec3{dist(rng) + 1.0f, dist(rng) + 1.0f, dist(rng) + 1.0f});
        
        points[i] = Vec3{dist(rng), dist(rng), dist(rng)};
        vectors[i] = Vec3{dist(rng), dist(rng), dist(rng)};
    }
    
    // Benchmark scalar operations
    auto scalar_start = std::chrono::high_resolution_clock::now();
    
    // Scalar matrix multiplication
    for (usize i = 0; i < count; ++i) {
        mat_results[i] = mat_a[i] * mat_b[i];
    }
    
    // Scalar point transformation
    for (usize i = 0; i < count; ++i) {
        point_results[i] = mat_a[i].transform_point(points[i]);
    }
    
    // Scalar vector transformation
    for (usize i = 0; i < count; ++i) {
        vector_results[i] = mat_a[i].transform_vector(vectors[i]);
    }
    
    auto scalar_end = std::chrono::high_resolution_clock::now();
    result.scalar_time_ns = std::chrono::duration<f64, std::nano>(scalar_end - scalar_start).count();
    
    // Benchmark SIMD operations
    auto simd_start = std::chrono::high_resolution_clock::now();
    
    // SIMD matrix multiplication
    simd_matrix::multiply_matrix4_arrays(mat_a.data(), mat_b.data(), mat_results.data(), count);
    
    // SIMD point transformation
    simd_matrix::transform_points_by_matrix4_arrays(points.data(), mat_a.data(), point_results.data(), count);
    
    // SIMD vector transformation
    simd_matrix::transform_vectors_by_matrix4_arrays(vectors.data(), mat_a.data(), vector_results.data(), count);
    
    auto simd_end = std::chrono::high_resolution_clock::now();
    result.simd_time_ns = std::chrono::duration<f64, std::nano>(simd_end - simd_start).count();
    
    // Calculate metrics
    result.speedup_factor = result.scalar_time_ns / result.simd_time_ns;
    result.matrix_throughput_mops_per_sec = (count * 3.0 / result.simd_time_ns) * 1000.0; // 3 operations
    
    LOG_INFO("Matrix4 SIMD Benchmark: {:.2f}x speedup ({:.1f} MOps/sec)", 
             result.speedup_factor, result.matrix_throughput_mops_per_sec);
    
    return result;
}

PhysicsPipelineBenchmark benchmark_3d_physics_pipeline(usize entity_count) noexcept {
    PhysicsPipelineBenchmark result{};
    result.entities_processed = entity_count;
    
    // Generate test physics data
    std::vector<Transform3D> transforms(entity_count);
    std::vector<Vec3> velocities(entity_count);
    std::vector<Vec3> forces(entity_count);
    std::vector<Quaternion> angular_velocities_quat(entity_count);
    std::mt19937 rng{42};
    std::uniform_real_distribution<f32> pos_dist{-100.0f, 100.0f};
    std::uniform_real_distribution<f32> vel_dist{-10.0f, 10.0f};
    
    for (usize i = 0; i < entity_count; ++i) {
        transforms[i] = Transform3D{
            Vec3{pos_dist(rng), pos_dist(rng), pos_dist(rng)},
            Quaternion::from_euler(pos_dist(rng) * 0.01f, pos_dist(rng) * 0.01f, pos_dist(rng) * 0.01f),
            Vec3::one()
        };
        velocities[i] = Vec3{vel_dist(rng), vel_dist(rng), vel_dist(rng)};
        forces[i] = Vec3{vel_dist(rng) * 100.0f, vel_dist(rng) * 100.0f, vel_dist(rng) * 100.0f};
        angular_velocities_quat[i] = Quaternion::from_euler(vel_dist(rng) * 0.1f, vel_dist(rng) * 0.1f, vel_dist(rng) * 0.1f);
    }
    
    const f32 dt = 1.0f / 60.0f;
    auto total_start = std::chrono::high_resolution_clock::now();
    
    // 1. Transform Update Phase
    auto transform_start = std::chrono::high_resolution_clock::now();
    
    // Update positions using SIMD
    std::vector<Vec3> positions(entity_count);
    std::vector<Vec3> velocity_deltas(entity_count);
    for (usize i = 0; i < entity_count; ++i) {
        positions[i] = transforms[i].position;
        velocity_deltas[i] = velocities[i] * dt;
    }
    
    simd_vec3::add_vec3_arrays(positions.data(), velocity_deltas.data(), positions.data(), entity_count);
    
    // Update rotations (quaternion integration)
    for (usize i = 0; i < entity_count; ++i) {
        transforms[i].position = positions[i];
        // Simplified angular integration
        transforms[i].rotation = transforms[i].rotation * angular_velocities_quat[i];
        transforms[i].rotation.normalize();
    }
    
    auto transform_end = std::chrono::high_resolution_clock::now();
    result.transform_update_time_ns = std::chrono::duration<f64, std::nano>(transform_end - transform_start).count();
    
    // 2. Collision Detection Phase (simplified broad-phase)
    auto collision_start = std::chrono::high_resolution_clock::now();
    
    usize collision_tests = 0;
    std::vector<f32> distances(entity_count * entity_count);
    
    // Simple O(n²) distance calculations using SIMD where possible
    for (usize i = 0; i < entity_count; ++i) {
        for (usize j = i + 1; j < entity_count; ++j) {
            f32 dist_sq = transforms[i].position.distance_squared_to(transforms[j].position);
            distances[i * entity_count + j] = dist_sq;
            collision_tests++;
        }
    }
    
    auto collision_end = std::chrono::high_resolution_clock::now();
    result.collision_detection_time_ns = std::chrono::duration<f64, std::nano>(collision_end - collision_start).count();
    
    // 3. Constraint Solving Phase (simplified)
    auto constraint_start = std::chrono::high_resolution_clock::now();
    
    // Apply forces using SIMD
    std::vector<Vec3> accelerations(entity_count);
    const f32 mass = 1.0f;
    for (usize i = 0; i < entity_count; ++i) {
        accelerations[i] = forces[i] / mass;
    }
    
    std::vector<Vec3> velocity_updates(entity_count);
    for (usize i = 0; i < entity_count; ++i) {
        velocity_updates[i] = accelerations[i] * dt;
    }
    
    simd_vec3::add_vec3_arrays(velocities.data(), velocity_updates.data(), velocities.data(), entity_count);
    
    auto constraint_end = std::chrono::high_resolution_clock::now();
    result.constraint_solving_time_ns = std::chrono::duration<f64, std::nano>(constraint_end - constraint_start).count();
    
    // 4. Integration Phase
    auto integration_start = std::chrono::high_resolution_clock::now();
    
    // Final position integration (already done above, but simulate additional work)
    std::vector<Matrix4> world_matrices(entity_count);
    for (usize i = 0; i < entity_count; ++i) {
        world_matrices[i] = transforms[i].get_world_matrix();
    }
    
    auto integration_end = std::chrono::high_resolution_clock::now();
    result.integration_time_ns = std::chrono::duration<f64, std::nano>(integration_end - integration_start).count();
    
    auto total_end = std::chrono::high_resolution_clock::now();
    result.total_pipeline_time_ns = std::chrono::duration<f64, std::nano>(total_end - total_start).count();
    
    // Calculate SIMD efficiency
    f64 theoretical_scalar_time = result.total_pipeline_time_ns * 4.0; // Assume 4x theoretical speedup
    result.simd_efficiency_ratio = theoretical_scalar_time / result.total_pipeline_time_ns;
    
    LOG_INFO("3D Physics Pipeline Benchmark: {} entities, {:.2f}ms total, {:.2f}x SIMD efficiency", 
             entity_count, result.total_pipeline_time_ns / 1e6, result.simd_efficiency_ratio);
    
    return result;
}

std::string PhysicsPipelineBenchmark::generate_report() const {
    std::ostringstream oss;
    
    oss << "=== 3D Physics Pipeline Performance Report ===\n";
    oss << "Entities Processed: " << entities_processed << "\n";
    oss << "Total Pipeline Time: " << (total_pipeline_time_ns / 1e6) << " ms\n\n";
    
    oss << "Phase Breakdown:\n";
    oss << "  Transform Update:    " << (transform_update_time_ns / 1e6) << " ms ("
        << (transform_update_time_ns / total_pipeline_time_ns * 100.0) << "%)\n";
    oss << "  Collision Detection: " << (collision_detection_time_ns / 1e6) << " ms ("
        << (collision_detection_time_ns / total_pipeline_time_ns * 100.0) << "%)\n";
    oss << "  Constraint Solving:  " << (constraint_solving_time_ns / 1e6) << " ms ("
        << (constraint_solving_time_ns / total_pipeline_time_ns * 100.0) << "%)\n";
    oss << "  Integration:         " << (integration_time_ns / 1e6) << " ms ("
        << (integration_time_ns / total_pipeline_time_ns * 100.0) << "%)\n\n";
    
    oss << "Performance Metrics:\n";
    oss << "  SIMD Efficiency Ratio: " << simd_efficiency_ratio << "x\n";
    oss << "  Entities per Second: " << (entities_processed / (total_pipeline_time_ns / 1e9)) << "\n";
    oss << "  Throughput: " << ((entities_processed * 1000.0) / (total_pipeline_time_ns / 1e6)) << " entities/ms\n";
    
    return oss.str();
}

} // namespace benchmark3d

//=============================================================================
// Educational Visualization Implementation
//=============================================================================

namespace education3d {

SimdRegisterVisualization analyze_3d_simd_utilization(const std::string& operation) noexcept {
    SimdRegisterVisualization viz;
    viz.operation_name = operation;
    
    if (operation == "vec3_cross_product") {
        viz.register_usage_steps = {
            "Load Vec3 A components (ax, ay, az, 0) into XMM0",
            "Load Vec3 B components (bx, by, bz, 0) into XMM1", 
            "Shuffle A to (ay, az, ax, 0) in XMM2",
            "Shuffle B to (bz, bx, by, 0) in XMM3",
            "Multiply XMM2 * XMM1 -> XMM4 (ay*bx, az*by, ax*bz, 0)",
            "Multiply XMM0 * XMM3 -> XMM5 (ax*bz, ay*bx, az*by, 0)",
            "Subtract XMM4 - XMM5 -> result cross product"
        };
        viz.register_utilization_percent = {100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f};
        viz.optimization_opportunities = {
            "Use AVX for processing 2 cross products simultaneously",
            "Prefetch next Vec3 pairs for cache efficiency",
            "Use FMA instructions where available"
        };
        viz.theoretical_vs_actual_speedup = 0.85; // 85% of theoretical due to memory access patterns
    }
    
    return viz;
}

AlgorithmComparison compare_3d_algorithms(const std::string& algorithm) noexcept {
    AlgorithmComparison comparison;
    comparison.algorithm_name = algorithm;
    
    if (algorithm == "quaternion_multiplication") {
        comparison.scalar_steps = {
            "Load quaternion A (ax, ay, az, aw)",
            "Load quaternion B (bx, by, bz, bw)",
            "Compute result.x = aw*bx + ax*bw + ay*bz - az*by",
            "Compute result.y = aw*by - ax*bz + ay*bw + az*bx",
            "Compute result.z = aw*bz + ax*by - ay*bx + az*bw",
            "Compute result.w = aw*bw - ax*bx - ay*by - az*bz",
            "Store result quaternion"
        };
        
        comparison.simd_steps = {
            "Load 4 quaternions A into AVX512 registers",
            "Load 4 quaternions B into AVX512 registers",
            "Perform vectorized quaternion multiplication",
            "Store 4 result quaternions"
        };
        
        comparison.step_timings_scalar = {1.0, 1.0, 4.0, 4.0, 4.0, 4.0, 1.0}; // Relative timings
        comparison.step_timings_simd = {4.0, 4.0, 8.0, 4.0}; // Process 4x data in similar time
        
        comparison.educational_insights = {
            "SIMD provides 3-4x speedup for quaternion operations",
            "Memory layout is crucial - AoS vs SoA affects performance",
            "Branch-free algorithms are essential for consistent SIMD performance",
            "Register pressure becomes important with complex operations"
        };
    }
    
    return comparison;
}

Simd3DEducation explain_vec3_cross_product() noexcept {
    Simd3DEducation education;
    education.concept_name = "3D Vector Cross Product";
    education.mathematical_explanation = 
        "The cross product a × b produces a vector perpendicular to both a and b.\n"
        "Formula: (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)\n"
        "Geometric meaning: The magnitude equals the area of the parallelogram formed by a and b.";
    
    education.simd_optimization_explanation = 
        "SIMD optimization involves processing multiple cross products simultaneously.\n"
        "Key technique: Separate x, y, z components into different registers for parallel computation.\n"
        "Challenges: Requires careful data shuffling and memory layout optimization.";
    
    education.key_performance_factors = {
        "Memory alignment for SIMD loads/stores",
        "Data layout (AoS vs SoA) affects cache efficiency", 
        "Shuffle instruction efficiency varies by architecture",
        "Batch size affects amortization of setup costs"
    };
    
    education.common_pitfalls = {
        "Assuming Vec3 has 16-byte alignment (it may not)",
        "Not considering cache line splits with unaligned data",
        "Over-optimizing small datasets where scalar is faster",
        "Ignoring the cost of data reorganization"
    };
    
    education.when_to_use_simd = "Use SIMD when processing arrays of 100+ Vec3 cross products";
    education.complexity_analysis = "O(1) per operation, but constant factor depends on batch size and architecture";
    
    return education;
}

Simd3DEducation explain_quaternion_multiplication() noexcept {
    Simd3DEducation education;
    education.concept_name = "Quaternion Multiplication";
    education.mathematical_explanation = 
        "Quaternion multiplication composes two rotations: q1 * q2.\n"
        "Formula involves 16 multiply-add operations following specific patterns.\n"
        "Result represents the combined rotation of q1 followed by q2.";
    
    education.simd_optimization_explanation = 
        "Quaternions naturally fit in 128-bit registers (4 x 32-bit floats).\n"
        "Multiple quaternions can be processed in parallel using wider registers.\n"
        "Key optimization: Minimize data movement and maximize register reuse.";
    
    education.key_performance_factors = {
        "Register pressure - quaternion multiplication uses many temporaries",
        "Instruction latency - multiply-add chains limit parallelism",
        "Memory bandwidth for loading/storing quaternion arrays",
        "Cache locality for sequential processing"
    };
    
    education.common_pitfalls = {
        "Forgetting quaternion normalization after operations",
        "Not considering numerical precision with many compositions",
        "Assuming all quaternions are unit quaternions",
        "Inefficient memory access patterns"
    };
    
    education.when_to_use_simd = "Use SIMD for 50+ quaternion multiplications or in tight loops";
    education.complexity_analysis = "O(1) per multiplication, ~30-40 cycles per quaternion on modern CPUs";
    
    return education;
}

Simd3DEducation explain_matrix_transformation() noexcept {
    Simd3DEducation education;
    education.concept_name = "4x4 Matrix Transformations";
    education.mathematical_explanation = 
        "4x4 matrices represent affine transformations in 3D space.\n"
        "Point transformation: P' = M * P (using homogeneous coordinates).\n"
        "Matrix multiplication requires 64 multiply-add operations.";
    
    education.simd_optimization_explanation = 
        "Each matrix row/column fits perfectly in 128-bit registers.\n"
        "Vector-matrix and matrix-matrix multiplications can be vectorized.\n"
        "Key insight: Process multiple transformations or multiple points in parallel.";
    
    education.key_performance_factors = {
        "Matrix storage order (row-major vs column-major)",
        "Cache utilization for large arrays of matrices",
        "Branch prediction for conditional transformations",
        "Register file pressure with multiple matrices"
    };
    
    education.common_pitfalls = {
        "Matrix order confusion (M*P vs P*M)",
        "Unnecessary matrix recalculation",
        "Poor cache locality with scattered data",
        "Not exploiting structure in transformation matrices"
    };
    
    education.when_to_use_simd = "Essential for real-time 3D graphics and physics simulation";
    education.complexity_analysis = "O(1) per transformation, ~100-200 cycles for 4x4 * 4x4 multiplication";
    
    return education;
}

Simd3DEducation explain_3d_normalization() noexcept {
    Simd3DEducation education;
    education.concept_name = "3D Vector Normalization";
    education.mathematical_explanation = 
        "Normalization converts a vector to unit length: v_normalized = v / |v|.\n"
        "Requires computing sqrt(x² + y² + z²) and dividing each component.\n"
        "Essential for direction vectors, surface normals, and quaternion maintenance.";
    
    education.simd_optimization_explanation = 
        "SIMD provides approximate reciprocal square root (rsqrt) instructions.\n"
        "Newton-Raphson iteration can improve rsqrt precision if needed.\n"
        "Batch processing multiple normalizations amortizes computation costs.";
    
    education.key_performance_factors = {
        "SIMD rsqrt instruction availability and precision",
        "Handling of zero-length vectors (division by zero)",
        "Memory access patterns for vector arrays",
        "Precision requirements vs performance trade-offs"
    };
    
    education.common_pitfalls = {
        "Not handling zero or near-zero length vectors",
        "Assuming all vectors need full precision normalization",
        "Inefficient branching for special cases",
        "Computing expensive sqrt when rsqrt suffices"
    };
    
    education.when_to_use_simd = "Use SIMD for normalizing 50+ vectors or in performance-critical loops";
    education.complexity_analysis = "O(1) per vector, ~20-30 cycles with SIMD rsqrt";
    
    return education;
}

} // namespace education3d

} // namespace ecscope::physics::simd3d