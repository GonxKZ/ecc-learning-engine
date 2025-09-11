/**
 * @file audio_raytracing_benchmark.cpp
 * @brief Performance benchmark for audio ray tracing system
 * 
 * This benchmark tests the performance of the audio ray tracing system under
 * various conditions and provides detailed performance analysis.
 */

#include "ecscope/audio/audio_raytracing.hpp"
#include "ecscope/audio/audio_debug.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace ecscope::audio;

class AudioRayTracingBenchmark {
public:
    struct BenchmarkResults {
        std::string test_name;
        uint32_t num_rays;
        uint32_t num_geometry;
        float scene_complexity;
        
        double setup_time_ms;
        double tracing_time_ms;
        double average_ray_time_us;
        uint32_t rays_per_second;
        
        uint32_t intersections_tested;
        uint32_t intersections_found;
        float intersection_ratio;
        
        size_t memory_usage_mb;
        float cpu_usage_percent;
        
        bool success;
        std::string error_message;
    };
    
    AudioRayTracingBenchmark() : m_random_engine(std::chrono::steady_clock::now().time_since_epoch().count()) {
        std::cout << "Audio Ray Tracing Performance Benchmark\n";
        std::cout << "======================================\n\n";
    }
    
    void run_all_benchmarks() {
        std::vector<BenchmarkResults> results;
        
        std::cout << "Running comprehensive audio ray tracing benchmarks...\n\n";
        
        // Basic performance tests
        results.push_back(benchmark_basic_ray_tracing());
        results.push_back(benchmark_high_ray_count());
        results.push_back(benchmark_complex_geometry());
        
        // Scalability tests
        results.push_back(benchmark_scalability_rays());
        results.push_back(benchmark_scalability_geometry());
        
        // Quality vs performance trade-offs
        results.push_back(benchmark_quality_levels());
        results.push_back(benchmark_frequency_bands());
        
        // Acceleration structure tests
        results.push_back(benchmark_bvh_performance());
        results.push_back(benchmark_octree_performance());
        
        // Real-time performance tests
        results.push_back(benchmark_realtime_performance());
        results.push_back(benchmark_dynamic_scenes());
        
        // Memory usage tests
        results.push_back(benchmark_memory_usage());
        
        // Generate comprehensive report
        generate_benchmark_report(results);
        export_results_to_csv(results, "audio_raytracing_benchmark_results.csv");
        
        std::cout << "\nBenchmark completed! Results saved to audio_raytracing_benchmark_results.csv\n";
    }

private:
    BenchmarkResults benchmark_basic_ray_tracing() {
        std::cout << "Running basic ray tracing benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "Basic Ray Tracing";
        result.num_rays = 1000;
        result.num_geometry = 100;
        result.scene_complexity = 1.0f;
        result.success = false;
        
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Setup ray tracer
            AudioRayTracer ray_tracer;
            AudioRayTracer::TracingParameters params;
            params.num_rays = result.num_rays;
            params.max_bounces = 5;
            params.min_energy_threshold = 0.001f;
            
            ray_tracer.initialize(params);
            
            // Create simple scene
            auto geometry = create_simple_scene(result.num_geometry);
            ray_tracer.set_scene_geometry(geometry);
            ray_tracer.build_bvh_acceleration_structure();
            
            auto setup_end_time = std::chrono::high_resolution_clock::now();
            result.setup_time_ms = std::chrono::duration<double, std::milli>(setup_end_time - start_time).count();
            
            // Perform ray tracing
            Vector3f source_pos{0.0f, 1.0f, 0.0f};
            Vector3f listener_pos{5.0f, 1.0f, 0.0f};
            
            auto tracing_start = std::chrono::high_resolution_clock::now();
            
            auto impulse_response = ray_tracer.trace_impulse_response(source_pos, listener_pos, AudioListener{});
            
            auto tracing_end = std::chrono::high_resolution_clock::now();
            result.tracing_time_ms = std::chrono::duration<double, std::milli>(tracing_end - tracing_start).count();
            
            // Calculate performance metrics
            result.average_ray_time_us = (result.tracing_time_ms * 1000.0) / result.num_rays;
            result.rays_per_second = static_cast<uint32_t>((result.num_rays * 1000.0) / result.tracing_time_ms);
            
            auto stats = ray_tracer.get_tracing_statistics();
            result.intersections_tested = stats.intersections_tested;
            result.intersections_found = stats.intersections_found;
            result.intersection_ratio = static_cast<float>(stats.intersections_found) / stats.intersections_tested;
            result.memory_usage_mb = static_cast<size_t>(stats.memory_usage_mb);
            
            result.success = true;
            
        } catch (const std::exception& e) {
            result.error_message = e.what();
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_high_ray_count() {
        std::cout << "Running high ray count benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "High Ray Count";
        result.num_rays = 50000;
        result.num_geometry = 500;
        result.scene_complexity = 3.0f;
        result.success = false;
        
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            AudioRayTracer ray_tracer;
            AudioRayTracer::TracingParameters params;
            params.num_rays = result.num_rays;
            params.max_bounces = 8;
            params.min_energy_threshold = 0.0001f;
            params.thread_count = std::thread::hardware_concurrency();
            
            ray_tracer.initialize(params);
            
            auto geometry = create_complex_scene(result.num_geometry);
            ray_tracer.set_scene_geometry(geometry);
            ray_tracer.build_bvh_acceleration_structure();
            
            auto setup_end_time = std::chrono::high_resolution_clock::now();
            result.setup_time_ms = std::chrono::duration<double, std::milli>(setup_end_time - start_time).count();
            
            Vector3f source_pos{0.0f, 2.0f, 0.0f};
            Vector3f listener_pos{10.0f, 2.0f, 5.0f};
            
            auto tracing_start = std::chrono::high_resolution_clock::now();
            
            auto impulse_response = ray_tracer.trace_impulse_response(source_pos, listener_pos, AudioListener{});
            
            auto tracing_end = std::chrono::high_resolution_clock::now();
            result.tracing_time_ms = std::chrono::duration<double, std::milli>(tracing_end - tracing_start).count();
            
            result.average_ray_time_us = (result.tracing_time_ms * 1000.0) / result.num_rays;
            result.rays_per_second = static_cast<uint32_t>((result.num_rays * 1000.0) / result.tracing_time_ms);
            
            auto stats = ray_tracer.get_tracing_statistics();
            result.intersections_tested = stats.intersections_tested;
            result.intersections_found = stats.intersections_found;
            result.intersection_ratio = static_cast<float>(stats.intersections_found) / stats.intersections_tested;
            result.memory_usage_mb = static_cast<size_t>(stats.memory_usage_mb);
            
            result.success = true;
            
        } catch (const std::exception& e) {
            result.error_message = e.what();
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_complex_geometry() {
        std::cout << "Running complex geometry benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "Complex Geometry";
        result.num_rays = 10000;
        result.num_geometry = 2000;
        result.scene_complexity = 5.0f;
        result.success = false;
        
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            AudioRayTracer ray_tracer;
            AudioRayTracer::TracingParameters params;
            params.num_rays = result.num_rays;
            params.max_bounces = 10;
            params.enable_diffraction = true;
            params.enable_transmission = true;
            params.enable_scattering = true;
            
            ray_tracer.initialize(params);
            
            auto geometry = create_highly_complex_scene(result.num_geometry);
            ray_tracer.set_scene_geometry(geometry);
            ray_tracer.build_bvh_acceleration_structure();
            
            auto setup_end_time = std::chrono::high_resolution_clock::now();
            result.setup_time_ms = std::chrono::duration<double, std::milli>(setup_end_time - start_time).count();
            
            Vector3f source_pos{0.0f, 1.5f, 0.0f};
            Vector3f listener_pos{8.0f, 1.5f, 8.0f};
            
            auto tracing_start = std::chrono::high_resolution_clock::now();
            
            auto impulse_response = ray_tracer.trace_impulse_response(source_pos, listener_pos, AudioListener{});
            
            auto tracing_end = std::chrono::high_resolution_clock::now();
            result.tracing_time_ms = std::chrono::duration<double, std::milli>(tracing_end - tracing_start).count();
            
            result.average_ray_time_us = (result.tracing_time_ms * 1000.0) / result.num_rays;
            result.rays_per_second = static_cast<uint32_t>((result.num_rays * 1000.0) / result.tracing_time_ms);
            
            auto stats = ray_tracer.get_tracing_statistics();
            result.intersections_tested = stats.intersections_tested;
            result.intersections_found = stats.intersections_found;
            result.intersection_ratio = static_cast<float>(stats.intersections_found) / stats.intersections_tested;
            result.memory_usage_mb = static_cast<size_t>(stats.memory_usage_mb);
            
            result.success = true;
            
        } catch (const std::exception& e) {
            result.error_message = e.what();
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_scalability_rays() {
        std::cout << "Running ray scalability benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "Ray Scalability";
        result.success = true;
        
        std::vector<uint32_t> ray_counts = {1000, 5000, 10000, 25000, 50000, 100000};
        std::vector<double> times;
        
        for (uint32_t ray_count : ray_counts) {
            try {
                AudioRayTracer ray_tracer;
                AudioRayTracer::TracingParameters params;
                params.num_rays = ray_count;
                params.max_bounces = 5;
                
                ray_tracer.initialize(params);
                
                auto geometry = create_simple_scene(200);
                ray_tracer.set_scene_geometry(geometry);
                ray_tracer.build_bvh_acceleration_structure();
                
                Vector3f source_pos{0.0f, 1.0f, 0.0f};
                Vector3f listener_pos{5.0f, 1.0f, 0.0f};
                
                auto start = std::chrono::high_resolution_clock::now();
                auto impulse_response = ray_tracer.trace_impulse_response(source_pos, listener_pos, AudioListener{});
                auto end = std::chrono::high_resolution_clock::now();
                
                double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
                times.push_back(time_ms);
                
                std::cout << "    " << ray_count << " rays: " << std::fixed << std::setprecision(2) 
                         << time_ms << " ms (" << (ray_count / time_ms) << " rays/ms)\n";
                
            } catch (const std::exception& e) {
                result.success = false;
                result.error_message = "Failed at " + std::to_string(ray_count) + " rays: " + e.what();
                break;
            }
        }
        
        if (result.success && times.size() > 1) {
            // Calculate scalability factor (should be roughly linear)
            double first_time_per_ray = times[0] / ray_counts[0];
            double last_time_per_ray = times.back() / ray_counts.back();
            result.scene_complexity = static_cast<float>(last_time_per_ray / first_time_per_ray);
            
            result.tracing_time_ms = times.back();
            result.num_rays = ray_counts.back();
            result.average_ray_time_us = (result.tracing_time_ms * 1000.0) / result.num_rays;
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_scalability_geometry() {
        std::cout << "Running geometry scalability benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "Geometry Scalability";
        result.success = true;
        
        std::vector<uint32_t> geometry_counts = {100, 500, 1000, 2000, 5000};
        
        for (uint32_t geom_count : geometry_counts) {
            try {
                auto start = std::chrono::high_resolution_clock::now();
                
                AudioRayTracer ray_tracer;
                AudioRayTracer::TracingParameters params;
                params.num_rays = 5000;
                params.max_bounces = 5;
                
                ray_tracer.initialize(params);
                
                auto geometry = create_simple_scene(geom_count);
                ray_tracer.set_scene_geometry(geometry);
                ray_tracer.build_bvh_acceleration_structure();
                
                auto setup_end = std::chrono::high_resolution_clock::now();
                double setup_time = std::chrono::duration<double, std::milli>(setup_end - start).count();
                
                Vector3f source_pos{0.0f, 1.0f, 0.0f};
                Vector3f listener_pos{5.0f, 1.0f, 0.0f};
                
                auto trace_start = std::chrono::high_resolution_clock::now();
                auto impulse_response = ray_tracer.trace_impulse_response(source_pos, listener_pos, AudioListener{});
                auto trace_end = std::chrono::high_resolution_clock::now();
                
                double trace_time = std::chrono::duration<double, std::milli>(trace_end - trace_start).count();
                
                std::cout << "    " << geom_count << " objects: Setup " << std::fixed << std::setprecision(2) 
                         << setup_time << " ms, Trace " << trace_time << " ms\n";
                
                if (geom_count == geometry_counts.back()) {
                    result.setup_time_ms = setup_time;
                    result.tracing_time_ms = trace_time;
                    result.num_geometry = geom_count;
                }
                
            } catch (const std::exception& e) {
                result.success = false;
                result.error_message = "Failed at " + std::to_string(geom_count) + " objects: " + e.what();
                break;
            }
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_quality_levels() {
        std::cout << "Running quality levels benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "Quality Levels";
        result.success = true;
        
        std::vector<int> quality_levels = {1, 3, 5, 7, 10};
        
        for (int quality : quality_levels) {
            try {
                AudioRayTracer ray_tracer;
                AudioRayTracer::TracingParameters params;
                params.num_rays = 1000 * quality;  // Scale rays with quality
                params.max_bounces = 2 + quality;   // More bounces for higher quality
                params.min_energy_threshold = 0.001f / quality;  // Lower threshold for higher quality
                
                ray_tracer.initialize(params);
                
                auto geometry = create_simple_scene(300);
                ray_tracer.set_scene_geometry(geometry);
                ray_tracer.build_bvh_acceleration_structure();
                
                Vector3f source_pos{0.0f, 1.0f, 0.0f};
                Vector3f listener_pos{5.0f, 1.0f, 0.0f};
                
                auto start = std::chrono::high_resolution_clock::now();
                auto impulse_response = ray_tracer.trace_impulse_response(source_pos, listener_pos, AudioListener{});
                auto end = std::chrono::high_resolution_clock::now();
                
                double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
                
                std::cout << "    Quality " << quality << ": " << std::fixed << std::setprecision(2) 
                         << time_ms << " ms (" << params.num_rays << " rays)\n";
                
                if (quality == quality_levels.back()) {
                    result.tracing_time_ms = time_ms;
                    result.num_rays = params.num_rays;
                    result.scene_complexity = static_cast<float>(quality);
                }
                
            } catch (const std::exception& e) {
                result.success = false;
                result.error_message = "Failed at quality " + std::to_string(quality) + ": " + e.what();
                break;
            }
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_frequency_bands() {
        std::cout << "Running frequency bands benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "Frequency Bands";
        result.num_rays = 5000;
        result.num_geometry = 300;
        result.success = false;
        
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            AudioRayTracer ray_tracer;
            AudioRayTracer::TracingParameters params;
            params.num_rays = result.num_rays;
            params.max_bounces = 6;
            params.frequency_bands = 10;
            params.min_frequency = 20.0f;
            params.max_frequency = 20000.0f;
            params.use_multiband_tracing = true;
            
            ray_tracer.initialize(params);
            
            auto geometry = create_frequency_test_scene();
            ray_tracer.set_scene_geometry(geometry);
            ray_tracer.build_bvh_acceleration_structure();
            
            auto setup_end_time = std::chrono::high_resolution_clock::now();
            result.setup_time_ms = std::chrono::duration<double, std::milli>(setup_end_time - start_time).count();
            
            Vector3f source_pos{0.0f, 1.0f, 0.0f};
            Vector3f listener_pos{6.0f, 1.0f, 0.0f};
            
            auto tracing_start = std::chrono::high_resolution_clock::now();
            
            auto impulse_response = ray_tracer.trace_impulse_response(source_pos, listener_pos, AudioListener{});
            
            auto tracing_end = std::chrono::high_resolution_clock::now();
            result.tracing_time_ms = std::chrono::duration<double, std::milli>(tracing_end - tracing_start).count();
            
            result.scene_complexity = 2.5f;  // Frequency processing adds complexity
            result.average_ray_time_us = (result.tracing_time_ms * 1000.0) / result.num_rays;
            
            result.success = true;
            
        } catch (const std::exception& e) {
            result.error_message = e.what();
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_bvh_performance() {
        std::cout << "Running BVH acceleration benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "BVH Acceleration";
        result.num_rays = 10000;
        result.num_geometry = 1000;
        result.scene_complexity = 2.0f;
        result.success = false;
        
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Test with BVH
            AudioRayTracer ray_tracer;
            AudioRayTracer::TracingParameters params;
            params.num_rays = result.num_rays;
            params.max_bounces = 7;
            params.enable_spatial_hashing = false;  // Use BVH only
            
            ray_tracer.initialize(params);
            
            auto geometry = create_complex_scene(result.num_geometry);
            ray_tracer.set_scene_geometry(geometry);
            ray_tracer.build_bvh_acceleration_structure();
            
            auto setup_end_time = std::chrono::high_resolution_clock::now();
            result.setup_time_ms = std::chrono::duration<double, std::milli>(setup_end_time - start_time).count();
            
            Vector3f source_pos{0.0f, 2.0f, 0.0f};
            Vector3f listener_pos{8.0f, 2.0f, 4.0f};
            
            auto tracing_start = std::chrono::high_resolution_clock::now();
            
            auto impulse_response = ray_tracer.trace_impulse_response(source_pos, listener_pos, AudioListener{});
            
            auto tracing_end = std::chrono::high_resolution_clock::now();
            result.tracing_time_ms = std::chrono::duration<double, std::milli>(tracing_end - tracing_start).count();
            
            result.average_ray_time_us = (result.tracing_time_ms * 1000.0) / result.num_rays;
            result.rays_per_second = static_cast<uint32_t>((result.num_rays * 1000.0) / result.tracing_time_ms);
            
            auto stats = ray_tracer.get_tracing_statistics();
            result.intersections_tested = stats.intersections_tested;
            result.intersections_found = stats.intersections_found;
            result.intersection_ratio = static_cast<float>(stats.intersections_found) / stats.intersections_tested;
            result.memory_usage_mb = static_cast<size_t>(stats.memory_usage_mb);
            
            result.success = true;
            
        } catch (const std::exception& e) {
            result.error_message = e.what();
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_octree_performance() {
        std::cout << "Running Octree acceleration benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "Octree Acceleration";
        result.num_rays = 10000;
        result.num_geometry = 1000;
        result.scene_complexity = 2.0f;
        result.success = false;
        
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Test with Octree
            AudioRayTracer ray_tracer;
            AudioRayTracer::TracingParameters params;
            params.num_rays = result.num_rays;
            params.max_bounces = 7;
            params.enable_spatial_hashing = true;  // Use octree/spatial hashing
            
            ray_tracer.initialize(params);
            
            auto geometry = create_complex_scene(result.num_geometry);
            ray_tracer.set_scene_geometry(geometry);
            ray_tracer.build_octree_acceleration_structure();
            
            auto setup_end_time = std::chrono::high_resolution_clock::now();
            result.setup_time_ms = std::chrono::duration<double, std::milli>(setup_end_time - start_time).count();
            
            Vector3f source_pos{0.0f, 2.0f, 0.0f};
            Vector3f listener_pos{8.0f, 2.0f, 4.0f};
            
            auto tracing_start = std::chrono::high_resolution_clock::now();
            
            auto impulse_response = ray_tracer.trace_impulse_response(source_pos, listener_pos, AudioListener{});
            
            auto tracing_end = std::chrono::high_resolution_clock::now();
            result.tracing_time_ms = std::chrono::duration<double, std::milli>(tracing_end - tracing_start).count();
            
            result.average_ray_time_us = (result.tracing_time_ms * 1000.0) / result.num_rays;
            result.rays_per_second = static_cast<uint32_t>((result.num_rays * 1000.0) / result.tracing_time_ms);
            
            auto stats = ray_tracer.get_tracing_statistics();
            result.intersections_tested = stats.intersections_tested;
            result.intersections_found = stats.intersections_found;
            result.intersection_ratio = static_cast<float>(stats.intersections_found) / stats.intersections_tested;
            result.memory_usage_mb = static_cast<size_t>(stats.memory_usage_mb);
            
            result.success = true;
            
        } catch (const std::exception& e) {
            result.error_message = e.what();
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_realtime_performance() {
        std::cout << "Running real-time performance benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "Real-time Performance";
        result.num_rays = 2000;  // Reduced for real-time
        result.num_geometry = 300;
        result.scene_complexity = 1.5f;
        result.success = false;
        
        try {
            AudioRayTracer ray_tracer;
            AudioRayTracer::TracingParameters params;
            params.num_rays = result.num_rays;
            params.max_bounces = 4;  // Reduced for real-time
            params.min_energy_threshold = 0.005f;  // Higher threshold for speed
            params.max_rays_per_frame = result.num_rays / 4;  // Distribute over frames
            
            ray_tracer.initialize(params);
            
            auto geometry = create_simple_scene(result.num_geometry);
            ray_tracer.set_scene_geometry(geometry);
            ray_tracer.build_bvh_acceleration_structure();
            
            // Simulate real-time updates
            Vector3f source_pos{0.0f, 1.0f, 0.0f};
            Vector3f listener_pos{3.0f, 1.0f, 0.0f};
            
            ray_tracer.start_realtime_tracing(source_pos, listener_pos);
            
            std::vector<double> frame_times;
            const int num_frames = 100;
            
            for (int frame = 0; frame < num_frames; ++frame) {
                auto frame_start = std::chrono::high_resolution_clock::now();
                
                ray_tracer.update_realtime_tracing(0.016f);  // 60 FPS target
                
                auto frame_end = std::chrono::high_resolution_clock::now();
                double frame_time = std::chrono::duration<double, std::milli>(frame_end - frame_start).count();
                frame_times.push_back(frame_time);
            }
            
            ray_tracer.stop_realtime_tracing();
            
            // Calculate statistics
            double total_time = std::accumulate(frame_times.begin(), frame_times.end(), 0.0);
            double avg_frame_time = total_time / num_frames;
            double max_frame_time = *std::max_element(frame_times.begin(), frame_times.end());
            
            result.tracing_time_ms = avg_frame_time;
            result.setup_time_ms = max_frame_time;  // Store max as "setup time"
            
            // Check if it meets real-time requirements (16.67ms for 60 FPS)
            bool realtime_capable = avg_frame_time < 16.67 && max_frame_time < 33.33;  // Max 2 frames
            
            std::cout << "    Average frame time: " << std::fixed << std::setprecision(2) 
                     << avg_frame_time << " ms\n";
            std::cout << "    Maximum frame time: " << max_frame_time << " ms\n";
            std::cout << "    Real-time capable (60 FPS): " << (realtime_capable ? "YES" : "NO") << "\n";
            
            result.success = realtime_capable;
            if (!realtime_capable) {
                result.error_message = "Failed to meet real-time performance requirements";
            }
            
        } catch (const std::exception& e) {
            result.error_message = e.what();
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_dynamic_scenes() {
        std::cout << "Running dynamic scenes benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "Dynamic Scenes";
        result.num_rays = 3000;
        result.num_geometry = 400;
        result.scene_complexity = 2.5f;
        result.success = false;
        
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            AudioRayTracer ray_tracer;
            AudioRayTracer::TracingParameters params;
            params.num_rays = result.num_rays;
            params.max_bounces = 5;
            
            ray_tracer.initialize(params);
            
            auto geometry = create_simple_scene(result.num_geometry);
            ray_tracer.set_scene_geometry(geometry);
            ray_tracer.build_bvh_acceleration_structure();
            
            auto setup_end_time = std::chrono::high_resolution_clock::now();
            result.setup_time_ms = std::chrono::duration<double, std::milli>(setup_end_time - start_time).count();
            
            // Simulate dynamic scene updates
            std::vector<double> update_times;
            const int num_updates = 20;
            
            for (int update = 0; update < num_updates; ++update) {
                // Modify some geometry
                auto modified_geometry = geometry;
                for (size_t i = 0; i < modified_geometry.size() / 4; ++i) {
                    // Move some objects randomly
                    Vector3f offset{
                        std::uniform_real_distribution<float>(-1.0f, 1.0f)(m_random_engine),
                        0.0f,
                        std::uniform_real_distribution<float>(-1.0f, 1.0f)(m_random_engine)
                    };
                    
                    for (auto& vertex : modified_geometry[i * 4].vertices) {
                        vertex = vertex + offset;
                    }
                }
                
                auto update_start = std::chrono::high_resolution_clock::now();
                
                ray_tracer.set_scene_geometry(modified_geometry);
                ray_tracer.build_bvh_acceleration_structure();
                
                Vector3f source_pos{0.0f, 1.0f, static_cast<float>(update * 0.5f)};
                Vector3f listener_pos{5.0f, 1.0f, static_cast<float>(update * 0.3f)};
                
                auto impulse_response = ray_tracer.trace_impulse_response(source_pos, listener_pos, AudioListener{});
                
                auto update_end = std::chrono::high_resolution_clock::now();
                double update_time = std::chrono::duration<double, std::milli>(update_end - update_start).count();
                update_times.push_back(update_time);
            }
            
            double total_update_time = std::accumulate(update_times.begin(), update_times.end(), 0.0);
            result.tracing_time_ms = total_update_time / num_updates;
            
            result.success = true;
            
        } catch (const std::exception& e) {
            result.error_message = e.what();
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    BenchmarkResults benchmark_memory_usage() {
        std::cout << "Running memory usage benchmark...\n";
        
        BenchmarkResults result;
        result.test_name = "Memory Usage";
        result.success = true;
        
        std::vector<uint32_t> geometry_sizes = {100, 500, 1000, 2000, 5000};
        
        for (uint32_t geom_size : geometry_sizes) {
            try {
                AudioRayTracer ray_tracer;
                AudioRayTracer::TracingParameters params;
                params.num_rays = 5000;
                params.max_bounces = 5;
                
                ray_tracer.initialize(params);
                
                auto geometry = create_simple_scene(geom_size);
                ray_tracer.set_scene_geometry(geometry);
                ray_tracer.build_bvh_acceleration_structure();
                
                auto stats = ray_tracer.get_tracing_statistics();
                
                std::cout << "    " << geom_size << " objects: " << std::fixed << std::setprecision(1) 
                         << stats.memory_usage_mb << " MB\n";
                
                if (geom_size == geometry_sizes.back()) {
                    result.num_geometry = geom_size;
                    result.memory_usage_mb = static_cast<size_t>(stats.memory_usage_mb);
                }
                
            } catch (const std::exception& e) {
                result.success = false;
                result.error_message = "Failed at " + std::to_string(geom_size) + " objects: " + e.what();
                break;
            }
        }
        
        std::cout << "  " << result.test_name << ": " << (result.success ? "PASSED" : "FAILED") << "\n";
        return result;
    }
    
    // Helper functions for creating test scenes
    std::vector<AcousticGeometry> create_simple_scene(uint32_t num_objects) {
        std::vector<AcousticGeometry> geometry;
        
        for (uint32_t i = 0; i < num_objects; ++i) {
            AcousticGeometry geom;
            geom.type = AcousticGeometry::Type::TRIANGLE;
            
            // Create random triangle
            Vector3f base{
                std::uniform_real_distribution<float>(-10.0f, 10.0f)(m_random_engine),
                std::uniform_real_distribution<float>(0.0f, 5.0f)(m_random_engine),
                std::uniform_real_distribution<float>(-10.0f, 10.0f)(m_random_engine)
            };
            
            geom.vertices = {
                base,
                base + Vector3f{1.0f, 0.0f, 0.0f},
                base + Vector3f{0.5f, 1.0f, 0.5f}
            };
            
            geom.indices = {0, 1, 2};
            geom.material = AcousticMaterial::concrete();
            
            geometry.push_back(geom);
        }
        
        return geometry;
    }
    
    std::vector<AcousticGeometry> create_complex_scene(uint32_t num_objects) {
        std::vector<AcousticGeometry> geometry;
        
        // Mix of different geometry types
        for (uint32_t i = 0; i < num_objects; ++i) {
            AcousticGeometry geom;
            
            int type = i % 4;
            switch (type) {
                case 0:
                    geom.type = AcousticGeometry::Type::TRIANGLE;
                    geom.material = AcousticMaterial::concrete();
                    break;
                case 1:
                    geom.type = AcousticGeometry::Type::QUAD;
                    geom.material = AcousticMaterial::wood();
                    break;
                case 2:
                    geom.type = AcousticGeometry::Type::SPHERE;
                    geom.material = AcousticMaterial::metal();
                    break;
                case 3:
                    geom.type = AcousticGeometry::Type::BOX;
                    geom.material = AcousticMaterial::glass();
                    break;
            }
            
            // Random positioning
            Vector3f center{
                std::uniform_real_distribution<float>(-15.0f, 15.0f)(m_random_engine),
                std::uniform_real_distribution<float>(0.0f, 8.0f)(m_random_engine),
                std::uniform_real_distribution<float>(-15.0f, 15.0f)(m_random_engine)
            };
            
            geom.center = center;
            
            geometry.push_back(geom);
        }
        
        return geometry;
    }
    
    std::vector<AcousticGeometry> create_highly_complex_scene(uint32_t num_objects) {
        std::vector<AcousticGeometry> geometry;
        
        // Create a complex architectural scene
        for (uint32_t i = 0; i < num_objects; ++i) {
            AcousticGeometry geom;
            geom.type = AcousticGeometry::Type::TRIANGLE;
            
            // Vary materials and properties significantly
            std::vector<AcousticMaterial> materials = {
                AcousticMaterial::concrete(),
                AcousticMaterial::wood(),
                AcousticMaterial::carpet(),
                AcousticMaterial::glass(),
                AcousticMaterial::metal(),
                AcousticMaterial::fabric()
            };
            
            geom.material = materials[i % materials.size()];
            
            // Create more complex geometry with varied sizes
            float scale = std::uniform_real_distribution<float>(0.1f, 3.0f)(m_random_engine);
            Vector3f base{
                std::uniform_real_distribution<float>(-20.0f, 20.0f)(m_random_engine),
                std::uniform_real_distribution<float>(0.0f, 10.0f)(m_random_engine),
                std::uniform_real_distribution<float>(-20.0f, 20.0f)(m_random_engine)
            };
            
            geom.vertices = {
                base,
                base + Vector3f{scale, 0.0f, 0.0f},
                base + Vector3f{scale * 0.5f, scale, scale * 0.5f}
            };
            
            geom.indices = {0, 1, 2};
            
            geometry.push_back(geom);
        }
        
        return geometry;
    }
    
    std::vector<AcousticGeometry> create_frequency_test_scene() {
        std::vector<AcousticGeometry> geometry;
        
        // Create scene with materials that have frequency-dependent properties
        for (int i = 0; i < 300; ++i) {
            AcousticGeometry geom;
            geom.type = AcousticGeometry::Type::QUAD;
            
            // Create material with frequency-dependent absorption
            geom.material = AcousticMaterial::concrete();
            geom.material.frequencies = {125, 250, 500, 1000, 2000, 4000, 8000};
            geom.material.absorption_spectrum = {0.1f, 0.15f, 0.25f, 0.4f, 0.6f, 0.8f, 0.9f};
            geom.material.scattering_spectrum = {0.1f, 0.15f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
            
            Vector3f center{
                std::uniform_real_distribution<float>(-12.0f, 12.0f)(m_random_engine),
                std::uniform_real_distribution<float>(0.0f, 6.0f)(m_random_engine),
                std::uniform_real_distribution<float>(-12.0f, 12.0f)(m_random_engine)
            };
            
            geom.center = center;
            geometry.push_back(geom);
        }
        
        return geometry;
    }
    
    void generate_benchmark_report(const std::vector<BenchmarkResults>& results) {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "AUDIO RAY TRACING BENCHMARK REPORT\n";
        std::cout << std::string(80, '=') << "\n\n";
        
        // Summary table
        std::cout << std::left << std::setw(25) << "Test Name" 
                  << std::setw(12) << "Status"
                  << std::setw(12) << "Rays"
                  << std::setw(12) << "Geometry"
                  << std::setw(15) << "Time (ms)"
                  << std::setw(15) << "Rays/sec"
                  << "Memory (MB)\n";
        std::cout << std::string(100, '-') << "\n";
        
        for (const auto& result : results) {
            std::cout << std::left << std::setw(25) << result.test_name
                      << std::setw(12) << (result.success ? "PASSED" : "FAILED")
                      << std::setw(12) << result.num_rays
                      << std::setw(12) << result.num_geometry
                      << std::setw(15) << std::fixed << std::setprecision(2) << result.tracing_time_ms
                      << std::setw(15) << result.rays_per_second
                      << result.memory_usage_mb << "\n";
        }
        
        std::cout << "\n";
        
        // Performance analysis
        size_t passed_tests = std::count_if(results.begin(), results.end(), 
                                          [](const BenchmarkResults& r) { return r.success; });
        
        std::cout << "PERFORMANCE ANALYSIS:\n";
        std::cout << "  Tests Passed: " << passed_tests << "/" << results.size() << "\n";
        
        if (passed_tests > 0) {
            auto max_rays_result = std::max_element(results.begin(), results.end(),
                [](const BenchmarkResults& a, const BenchmarkResults& b) {
                    return a.success && b.success ? a.num_rays < b.num_rays : !a.success;
                });
            
            if (max_rays_result != results.end() && max_rays_result->success) {
                std::cout << "  Maximum rays traced: " << max_rays_result->num_rays 
                         << " (" << max_rays_result->test_name << ")\n";
            }
            
            auto fastest_result = std::min_element(results.begin(), results.end(),
                [](const BenchmarkResults& a, const BenchmarkResults& b) {
                    return a.success && b.success ? a.average_ray_time_us < b.average_ray_time_us : !a.success;
                });
            
            if (fastest_result != results.end() && fastest_result->success) {
                std::cout << "  Fastest ray processing: " << std::fixed << std::setprecision(3) 
                         << fastest_result->average_ray_time_us << " μs/ray"
                         << " (" << fastest_result->test_name << ")\n";
            }
        }
        
        std::cout << "\n";
        
        // Failed tests details
        bool has_failures = false;
        for (const auto& result : results) {
            if (!result.success) {
                if (!has_failures) {
                    std::cout << "FAILED TESTS:\n";
                    has_failures = true;
                }
                std::cout << "  " << result.test_name << ": " << result.error_message << "\n";
            }
        }
        
        if (has_failures) {
            std::cout << "\n";
        }
        
        // Recommendations
        std::cout << "RECOMMENDATIONS:\n";
        
        auto realtime_test = std::find_if(results.begin(), results.end(),
            [](const BenchmarkResults& r) { return r.test_name == "Real-time Performance"; });
        
        if (realtime_test != results.end()) {
            if (realtime_test->success) {
                std::cout << "  ✓ System is capable of real-time ray tracing\n";
            } else {
                std::cout << "  ✗ System needs optimization for real-time performance\n";
                std::cout << "    - Consider reducing ray count or quality settings\n";
                std::cout << "    - Enable multi-threading if not already enabled\n";
                std::cout << "    - Use spatial acceleration structures\n";
            }
        }
        
        // Check for memory issues
        auto memory_intensive = std::find_if(results.begin(), results.end(),
            [](const BenchmarkResults& r) { return r.memory_usage_mb > 500; });
        
        if (memory_intensive != results.end()) {
            std::cout << "  ⚠ High memory usage detected (>" << memory_intensive->memory_usage_mb << " MB)\n";
            std::cout << "    - Consider using streaming or level-of-detail techniques\n";
        }
        
        std::cout << "\n" << std::string(80, '=') << "\n";
    }
    
    void export_results_to_csv(const std::vector<BenchmarkResults>& results, const std::string& filename) {
        std::ofstream csv_file(filename);
        
        // CSV header
        csv_file << "Test Name,Status,Rays,Geometry,Complexity,Setup Time (ms),Tracing Time (ms),"
                 << "Average Ray Time (μs),Rays/sec,Intersections Tested,Intersections Found,"
                 << "Intersection Ratio,Memory (MB),Error Message\n";
        
        // CSV data
        for (const auto& result : results) {
            csv_file << "\"" << result.test_name << "\","
                     << (result.success ? "PASSED" : "FAILED") << ","
                     << result.num_rays << ","
                     << result.num_geometry << ","
                     << std::fixed << std::setprecision(2) << result.scene_complexity << ","
                     << result.setup_time_ms << ","
                     << result.tracing_time_ms << ","
                     << std::setprecision(3) << result.average_ray_time_us << ","
                     << result.rays_per_second << ","
                     << result.intersections_tested << ","
                     << result.intersections_found << ","
                     << std::setprecision(4) << result.intersection_ratio << ","
                     << result.memory_usage_mb << ","
                     << "\"" << result.error_message << "\"\n";
        }
        
        csv_file.close();
    }

private:
    std::mt19937 m_random_engine;
};

int main() {
    try {
        AudioRayTracingBenchmark benchmark;
        benchmark.run_all_benchmarks();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Benchmark failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Benchmark failed with unknown exception" << std::endl;
        return 1;
    }
}