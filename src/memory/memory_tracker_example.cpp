/**
 * @file memory_tracker_example.cpp
 * @brief Example demonstrating integration of Memory Tracker with ECScope allocators
 * 
 * This example shows how to integrate the Memory Tracker with various allocators
 * in the ECScope ECS engine, providing educational insights into memory usage patterns.
 * 
 * Key Integration Examples:
 * - Arena allocator integration with tracking
 * - Pool allocator integration with tracking
 * - Custom allocator tracking setup
 * - Real-time memory analysis and reporting
 * - Leak detection demonstration
 * - Performance impact measurement
 * 
 * Educational Value:
 * - Clear demonstration of tracking overhead
 * - Examples of memory usage patterns
 * - Integration with existing allocator infrastructure
 * - Real-world usage scenarios for ECS engines
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "memory_tracker.hpp"
#include "arena.hpp"
// #include "pool.hpp"  // Uncomment when needed
#include "core/time.hpp"
#include "core/log.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <thread>
#include <chrono>

using namespace ecscope::memory;
using namespace ecscope::core;

/**
 * @brief Example Arena Allocator with Memory Tracker integration
 */
class TrackedArenaAllocator {
private:
    ArenaAllocator arena_;
    u32 allocator_id_;
    static std::atomic<u32> next_id_;
    
public:
    explicit TrackedArenaAllocator(usize size, const std::string& name = "TrackedArena")
        : arena_(size, name, false)  // Disable arena's internal tracking
        , allocator_id_(next_id_++) {
        
        LOG_INFO("Created tracked arena allocator: " + name);
    }
    
    void* allocate(usize size, usize alignment = alignof(std::max_align_t), 
                   AllocationCategory category = AllocationCategory::Unknown) {
        void* ptr = arena_.allocate(size, alignment);
        if (ptr) {
            // Track the allocation with the Memory Tracker
            tracker::track_alloc(ptr, size, size, alignment, category, 
                               AllocatorType::Arena, arena_.name().c_str(), allocator_id_);
        }
        return ptr;
    }
    
    // Arena allocators don't support individual deallocation
    void reset() {
        // Get all active allocations for this allocator and mark them as deallocated
        auto& tracker = MemoryTracker::instance();
        auto allocations = tracker.get_active_allocations();
        
        for (const auto& allocation : allocations) {
            if (allocation.allocator_id == allocator_id_ && 
                allocation.allocator_type == AllocatorType::Arena) {
                tracker::track_dealloc(allocation.address, AllocatorType::Arena,
                                     arena_.name().c_str(), allocator_id_);
            }
        }
        
        arena_.reset();
    }
    
    const ArenaStats& stats() const { return arena_.stats(); }
    const std::string& name() const { return arena_.name(); }
};

std::atomic<u32> TrackedArenaAllocator::next_id_{1};

/**
 * @brief Simulate ECS component allocations
 */
struct Transform {
    float x, y, z;
    float rotation;
    
    Transform(float x = 0, float y = 0, float z = 0, float rot = 0) 
        : x(x), y(y), z(z), rotation(rot) {}
};

struct Velocity {
    float vx, vy, vz;
    
    Velocity(float vx = 0, float vy = 0, float vz = 0) 
        : vx(vx), vy(vy), vz(vz) {}
};

struct RenderMesh {
    std::vector<float> vertices;
    std::vector<u32> indices;
    
    RenderMesh(usize vertex_count) {
        vertices.resize(vertex_count * 3); // x, y, z per vertex
        indices.resize(vertex_count);
        
        // Fill with dummy data
        for (usize i = 0; i < vertices.size(); ++i) {
            vertices[i] = static_cast<float>(i) * 0.1f;
        }
        for (usize i = 0; i < indices.size(); ++i) {
            indices[i] = static_cast<u32>(i);
        }
    }
};

/**
 * @brief Demonstrate basic memory tracking functionality
 */
void demonstrate_basic_tracking() {
    std::cout << "=== Basic Memory Tracking Demo ===\n";
    
    // Initialize memory tracker with custom configuration
    TrackerConfig config;
    config.enable_tracking = true;
    config.enable_call_stacks = false;  // Disable for better performance in demo
    config.enable_access_tracking = false;  // Disable for demo
    config.enable_heat_mapping = true;
    config.enable_leak_detection = true;
    config.max_tracked_allocations = 10000;
    config.sampling_rate = 1.0;  // Track everything
    
    MemoryTracker::initialize(config);
    
    // Create a tracked arena allocator
    TrackedArenaAllocator arena(64 * KB, "ECS_Components");
    
    // Allocate some ECS components
    std::vector<Transform*> transforms;
    std::vector<Velocity*> velocities;
    
    const usize num_entities = 1000;
    
    std::cout << "Allocating " << num_entities << " entities...\n";
    
    for (usize i = 0; i < num_entities; ++i) {
        // Allocate Transform component
        Transform* transform = static_cast<Transform*>(
            arena.allocate(sizeof(Transform), alignof(Transform), 
                          AllocationCategory::ECS_Components));
        if (transform) {
            new(transform) Transform(static_cast<float>(i), 0.0f, 0.0f, 0.0f);
            transforms.push_back(transform);
        }
        
        // Allocate Velocity component (every other entity)
        if (i % 2 == 0) {
            Velocity* velocity = static_cast<Velocity*>(
                arena.allocate(sizeof(Velocity), alignof(Velocity), 
                              AllocationCategory::ECS_Components));
            if (velocity) {
                new(velocity) Velocity(1.0f, 0.0f, 0.0f);
                velocities.push_back(velocity);
            }
        }
    }
    
    // Get and display statistics
    auto global_stats = MemoryTracker::instance().get_global_stats();
    auto category_stats = MemoryTracker::instance().get_category_stats(AllocationCategory::ECS_Components);
    
    std::cout << "Global Memory Stats:\n";
    std::cout << "  Total allocated: " << global_stats.total_allocated << " bytes\n";
    std::cout << "  Peak allocated: " << global_stats.peak_allocated << " bytes\n";
    std::cout << "  Active allocations: " << global_stats.current_allocations << "\n";
    std::cout << "  Total allocations ever: " << global_stats.total_allocations_ever << "\n";
    
    std::cout << "ECS Components Category Stats:\n";
    std::cout << "  Current allocated: " << category_stats.current_allocated << " bytes\n";
    std::cout << "  Peak allocated: " << category_stats.peak_allocated << " bytes\n";
    std::cout << "  Active allocations: " << category_stats.current_allocations << "\n";
    std::cout << "  Average allocation size: " << category_stats.average_allocation_size << " bytes\n";
    std::cout << "  Waste ratio: " << (category_stats.waste_ratio * 100.0) << "%\n";
    
    // Demonstrate size distribution analysis
    auto size_dist = MemoryTracker::instance().get_size_distribution();
    std::cout << "Size Distribution (top 5 buckets):\n";
    for (usize i = 0; i < std::min(5ul, SizeDistribution::BUCKET_COUNT); ++i) {
        const auto& bucket = size_dist.buckets[i];
        if (bucket.allocation_count > 0) {
            std::cout << "  " << bucket.min_size << "-" << bucket.max_size 
                     << " bytes: " << bucket.allocation_count << " allocations ("
                     << std::fixed << std::setprecision(1) << bucket.percentage << "%)\n";
        }
    }
    
    // Clean up
    arena.reset();
    
    std::cout << "Arena reset completed.\n\n";
}

/**
 * @brief Demonstrate memory leak detection
 */
void demonstrate_leak_detection() {
    std::cout << "=== Memory Leak Detection Demo ===\n";
    
    TrackedArenaAllocator arena(32 * KB, "LeakTest");
    
    // Allocate some objects and "forget" to deallocate them
    std::cout << "Allocating objects for leak test...\n";
    
    for (int i = 0; i < 10; ++i) {
        void* ptr = arena.allocate(64, alignof(std::max_align_t), 
                                 AllocationCategory::Temporary);
        (void)ptr; // Intentionally "leak" by not tracking the pointer
    }
    
    // Wait a bit to make the allocations "old"
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check for leaks
    auto leaks = MemoryTracker::instance().detect_leaks(0.05, 0.5); // 50ms old, 50% confidence
    
    std::cout << "Detected " << leaks.size() << " potential leaks:\n";
    for (const auto& leak : leaks) {
        std::cout << "  Address: " << leak.allocation.address 
                 << ", Size: " << leak.allocation.size 
                 << " bytes, Age: " << std::fixed << std::setprecision(3) << leak.age 
                 << "s, Score: " << std::setprecision(2) << leak.leak_score << "\n";
    }
    
    // Clean up properly
    arena.reset();
    std::cout << "Cleaned up leak test.\n\n";
}

/**
 * @brief Demonstrate performance analysis
 */
void demonstrate_performance_analysis() {
    std::cout << "=== Performance Analysis Demo ===\n";
    
    TrackedArenaAllocator arena(1 * MB, "PerformanceTest");
    
    // Measure allocation performance with tracking
    Timer timer;
    const usize num_allocations = 10000;
    
    std::cout << "Performing " << num_allocations << " allocations with tracking...\n";
    
    timer.start();
    for (usize i = 0; i < num_allocations; ++i) {
        usize size = 32 + (i % 256); // Varying sizes
        void* ptr = arena.allocate(size, alignof(std::max_align_t), 
                                 AllocationCategory::ECS_Core);
        (void)ptr;
    }
    f64 tracked_time = timer.elapsed_milliseconds();
    
    std::cout << "Tracked allocations completed in " << std::fixed << std::setprecision(3) 
             << tracked_time << " ms\n";
    
    // Get performance metrics
    auto global_stats = MemoryTracker::instance().get_global_stats();
    std::cout << "Performance Metrics:\n";
    std::cout << "  Average allocation time: " << (global_stats.average_allocation_time * 1000000.0) << " µs\n";
    std::cout << "  Allocation rate: " << global_stats.allocation_rate << " allocs/sec\n";
    std::cout << "  Estimated cache miss rate: " << std::setprecision(1) 
             << (MemoryTracker::instance().estimate_cache_miss_rate() * 100.0) << "%\n";
    std::cout << "  Estimated memory bandwidth: " << std::setprecision(2)
             << (MemoryTracker::instance().estimate_memory_bandwidth_usage() / (1024 * 1024)) << " MB/s\n";
    
    // Memory pressure analysis
    auto pressure = MemoryTracker::instance().get_memory_pressure();
    std::cout << "Memory Pressure: " << pressure.level_string() 
             << " (usage ratio: " << std::setprecision(1) << (pressure.memory_usage_ratio * 100.0) << "%)\n";
    
    arena.reset();
    std::cout << "Performance test completed.\n\n";
}

/**
 * @brief Demonstrate timeline analysis
 */
void demonstrate_timeline_analysis() {
    std::cout << "=== Timeline Analysis Demo ===\n";
    
    TrackedArenaAllocator arena(512 * KB, "TimelineTest");
    
    std::cout << "Simulating allocation patterns over time...\n";
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<usize> size_dist(16, 512);
    std::uniform_int_distribution<int> delay_dist(1, 10);
    
    // Simulate allocation patterns
    for (int phase = 0; phase < 3; ++phase) {
        std::cout << "Phase " << (phase + 1) << ": ";
        
        AllocationCategory category;
        switch (phase) {
            case 0:
                category = AllocationCategory::ECS_Components;
                std::cout << "Entity creation\n";
                break;
            case 1:
                category = AllocationCategory::Renderer_Meshes;
                std::cout << "Mesh loading\n";
                break;
            case 2:
                category = AllocationCategory::Audio_Buffers;
                std::cout << "Audio streaming\n";
                break;
            default:
                category = AllocationCategory::Unknown;
                break;
        }
        
        for (int i = 0; i < 50; ++i) {
            usize size = size_dist(rng);
            void* ptr = arena.allocate(size, alignof(std::max_align_t), category);
            (void)ptr;
            
            // Small delay to spread allocations over time
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(rng)));
        }
    }
    
    // Get timeline data
    auto timeline = MemoryTracker::instance().get_allocation_timeline();
    
    std::cout << "Timeline Analysis (last 10 time slots):\n";
    std::cout << "Time\t\tAllocs\tDeallocs\tNet Bytes\n";
    
    usize start_idx = (timeline.size() >= 10) ? timeline.size() - 10 : 0;
    for (usize i = start_idx; i < timeline.size(); ++i) {
        const auto& slot = timeline[i];
        if (slot.allocations > 0 || slot.deallocations > 0) {
            i64 net_bytes = static_cast<i64>(slot.bytes_allocated) - static_cast<i64>(slot.bytes_deallocated);
            std::cout << std::fixed << std::setprecision(1) << slot.start_time << "s\t\t"
                     << slot.allocations << "\t" << slot.deallocations << "\t"
                     << std::showpos << net_bytes << std::noshowpos << "\n";
        }
    }
    
    arena.reset();
    std::cout << "Timeline analysis completed.\n\n";
}

/**
 * @brief Export tracking data for offline analysis
 */
void demonstrate_data_export() {
    std::cout << "=== Data Export Demo ===\n";
    
    // Export various data formats
    std::cout << "Exporting tracking data...\n";
    
    MemoryTracker::instance().export_to_json("memory_tracking_report.json");
    MemoryTracker::instance().export_timeline_csv("allocation_timeline.csv");
    MemoryTracker::instance().export_heat_map_data("memory_heat_map.csv");
    
    std::cout << "Data exported to:\n";
    std::cout << "  - memory_tracking_report.json\n";
    std::cout << "  - allocation_timeline.csv\n";
    std::cout << "  - memory_heat_map.csv\n\n";
}

/**
 * @brief Main example function
 */
int main() {
    std::cout << "ECScope Memory Tracker Integration Example\n";
    std::cout << "==========================================\n\n";
    
    try {
        // Run all demonstrations
        demonstrate_basic_tracking();
        demonstrate_leak_detection();
        demonstrate_performance_analysis();
        demonstrate_timeline_analysis();
        demonstrate_data_export();
        
        std::cout << "All demonstrations completed successfully!\n";
        std::cout << "Check the exported files for detailed memory analysis data.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error during demonstration: " << e.what() << std::endl;
        return 1;
    }
    
    // Shutdown the memory tracker
    MemoryTracker::shutdown();
    
    return 0;
}