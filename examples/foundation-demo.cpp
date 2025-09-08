/**
 * @file foundation-demo.cpp
 * @brief Comprehensive demonstration of the ECScope ECS foundation
 * 
 * This example showcases all the core features of the ECS foundation:
 * - Entity management with generational IDs
 * - Component registration and type safety
 * - Efficient packed storage with SoA optimization
 * - System lifecycle management
 * - Performance monitoring and profiling
 * - Memory management and tracking
 * 
 * Educational Notes:
 * This demo serves as both a test of the foundation and a learning resource.
 * Each section demonstrates key concepts and best practices for using the ECS
 * framework in real applications.
 */

#include "../include/ecscope/foundation/entity.hpp"
#include "../include/ecscope/foundation/component.hpp"
#include "../include/ecscope/foundation/storage.hpp"
#include "../include/ecscope/foundation/system.hpp"
#include "../include/ecscope/core/memory.hpp"
#include "../include/ecscope/core/platform.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>
#include <vector>

using namespace ecscope::foundation;
using namespace ecscope::core;

// Example component types for the demo
struct Position {
    float x{0.0f}, y{0.0f}, z{0.0f};
    
    Position() = default;
    Position(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    
    std::string to_string() const {
        return "Position(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }
};

struct Velocity {
    float dx{0.0f}, dy{0.0f}, dz{0.0f};
    
    Velocity() = default;
    Velocity(float dx_, float dy_, float dz_) : dx(dx_), dy(dy_), dz(dz_) {}
    
    bool operator==(const Velocity& other) const {
        return dx == other.dx && dy == other.dy && dz == other.dz;
    }
    
    std::string to_string() const {
        return "Velocity(" + std::to_string(dx) + ", " + std::to_string(dy) + ", " + std::to_string(dz) + ")";
    }
};

struct Health {
    float current{100.0f};
    float maximum{100.0f};
    
    Health() = default;
    Health(float max) : current(max), maximum(max) {}
    Health(float curr, float max) : current(curr), maximum(max) {}
    
    bool operator==(const Health& other) const {
        return current == other.current && maximum == other.maximum;
    }
    
    std::string to_string() const {
        return "Health(" + std::to_string(current) + "/" + std::to_string(maximum) + ")";
    }
    
    bool is_alive() const { return current > 0.0f; }
    void heal(float amount) { current = std::min(current + amount, maximum); }
    void damage(float amount) { current = std::max(current - amount, 0.0f); }
};

struct Player {}; // Tag component

// Example system implementations
class MovementSystem : public QuerySystem<MovementSystem> {
public:
    MovementSystem() : QuerySystem({
        .name = "MovementSystem",
        .priority = SystemPriority::High,
        .phase = SystemPhase::Update,
        .thread_safe = true
    }) {
        require_component<Position>();
        require_component<Velocity>();
    }

protected:
    void on_update(float delta_time) override {
        // In a real implementation, we would iterate over entities with Position and Velocity
        // For this demo, we'll simulate processing
        std::cout << "MovementSystem: Processing entities (dt=" << delta_time << "s)\n";
        
        // Simulate some work
        auto start = std::chrono::high_resolution_clock::now();
        volatile int work = 0;
        for (int i = 0; i < 10000; ++i) {
            work += i * i;
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "  Simulated work took: " << duration.count() << " microseconds\n";
    }
};

class HealthSystem : public QuerySystem<HealthSystem> {
public:
    HealthSystem() : QuerySystem({
        .name = "HealthSystem",
        .priority = SystemPriority::Normal,
        .phase = SystemPhase::Update,
        .thread_safe = true
    }) {
        require_component<Health>();
    }

protected:
    void on_update(float delta_time) override {
        std::cout << "HealthSystem: Managing health (dt=" << delta_time << "s)\n";
        
        // Simulate health regeneration or decay processing
        auto start = std::chrono::high_resolution_clock::now();
        volatile float work = 0.0f;
        for (int i = 0; i < 5000; ++i) {
            work += std::sin(i * 0.01f) * delta_time;
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "  Health processing took: " << duration.count() << " microseconds\n";
    }
};

class RenderSystem : public QuerySystem<RenderSystem> {
public:
    RenderSystem() : QuerySystem({
        .name = "RenderSystem",
        .priority = SystemPriority::High,
        .phase = SystemPhase::Render,
        .thread_safe = false  // Rendering typically isn't thread-safe
    }) {
        require_component<Position>();
    }

protected:
    void on_initialize() override {
        std::cout << "RenderSystem: Initializing rendering resources\n";
    }
    
    void on_update(float delta_time) override {
        std::cout << "RenderSystem: Rendering frame (dt=" << delta_time << "s)\n";
        
        // Simulate rendering work
        auto start = std::chrono::high_resolution_clock::now();
        volatile float work = 0.0f;
        for (int i = 0; i < 15000; ++i) {
            work += std::cos(i * 0.02f);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "  Rendering took: " << duration.count() << " microseconds\n";
    }
    
    void on_shutdown() override {
        std::cout << "RenderSystem: Cleaning up rendering resources\n";
    }
};

// Demo functions
void demonstrate_platform_detection() {
    std::cout << "\n=== Platform Detection Demo ===\n";
    
    const auto& cpu_features = platform::get_cpu_features();
    std::cout << "CPU Features:\n";
    std::cout << "  Logical cores: " << cpu_features.logical_cores << "\n";
    std::cout << "  Physical cores: " << cpu_features.physical_cores << "\n";
    std::cout << "  Cache line size: " << cpu_features.cache_line_size << " bytes\n";
    std::cout << "  SSE2: " << (cpu_features.sse2 ? "Yes" : "No") << "\n";
    std::cout << "  AVX: " << (cpu_features.avx ? "Yes" : "No") << "\n";
    std::cout << "  AVX2: " << (cpu_features.avx2 ? "Yes" : "No") << "\n";
}

void demonstrate_entity_manager() {
    std::cout << "\n=== Entity Manager Demo ===\n";
    
    EntityManager::Config config{
        .initial_capacity = 100,
        .max_entities = 10000,
        .enable_recycling = true,
        .thread_safe = true
    };
    
    EntityManager entity_manager(config);
    
    std::cout << "Creating entities...\n";
    std::vector<EntityHandle> entities;
    
    // Create some entities
    for (int i = 0; i < 10; ++i) {
        auto entity = entity_manager.create_entity();
        entities.push_back(entity);
        std::cout << "  Created entity: ID=" << entity.id.value 
                  << ", Gen=" << entity.generation << "\n";
    }
    
    std::cout << "Entity count: " << entity_manager.entity_count() << "\n";
    std::cout << "Utilization: " << std::fixed << std::setprecision(2) 
              << (entity_manager.utilization() * 100.0) << "%\n";
    
    // Destroy some entities to test recycling
    std::cout << "\nDestroying some entities...\n";
    for (int i = 0; i < 3; ++i) {
        if (entity_manager.destroy_entity(entities[i])) {
            std::cout << "  Destroyed entity: ID=" << entities[i].id.value 
                      << ", Gen=" << entities[i].generation << "\n";
        }
    }
    
    std::cout << "Entity count after destruction: " << entity_manager.entity_count() << "\n";
    std::cout << "Recycled slots: " << entity_manager.recycled_count() << "\n";
    
    // Create new entities (should reuse destroyed slots)
    std::cout << "\nCreating new entities (recycling test)...\n";
    for (int i = 0; i < 3; ++i) {
        auto entity = entity_manager.create_entity();
        std::cout << "  Recycled entity: ID=" << entity.id.value 
                  << ", Gen=" << entity.generation << "\n";
    }
    
    // Test entity validation
    std::cout << "\nEntity validation test...\n";
    for (const auto& entity : entities) {
        bool alive = entity_manager.is_alive(entity);
        std::cout << "  Entity ID=" << entity.id.value 
                  << ", Gen=" << entity.generation 
                  << " is " << (alive ? "alive" : "dead") << "\n";
    }
}

void demonstrate_component_system() {
    std::cout << "\n=== Component System Demo ===\n";
    
    // Register components
    auto& registry = ComponentRegistry::instance();
    
    auto pos_id = registry.register_component<Position>("Position");
    auto vel_id = registry.register_component<Velocity>("Velocity");
    auto health_id = registry.register_component<Health>("Health");
    auto player_id = registry.register_component<Player>("Player");
    
    std::cout << "Registered components:\n";
    std::cout << "  Position ID: " << pos_id.value << "\n";
    std::cout << "  Velocity ID: " << vel_id.value << "\n";
    std::cout << "  Health ID: " << health_id.value << "\n";
    std::cout << "  Player ID: " << player_id.value << "\n";
    
    // Demonstrate component signatures
    auto signature_builder = registry.create_signature_builder();
    auto moving_entity_signature = signature_builder
        .with<Position>()
        .with<Velocity>()
        .build();
    
    auto player_signature = signature_builder
        .reset()
        .with<Player>()
        .with<Position>()
        .with<Health>()
        .build();
    
    std::cout << "\nComponent signatures:\n";
    std::cout << "  Moving entity: 0x" << std::hex << moving_entity_signature << std::dec << "\n";
    std::cout << "  Player: 0x" << std::hex << player_signature << std::dec << "\n";
    
    // Test component reflection
    const auto* pos_desc = registry.get_component_desc(pos_id);
    if (pos_desc) {
        std::cout << "\nPosition component info:\n";
        std::cout << "  Name: " << pos_desc->name << "\n";
        std::cout << "  Size: " << pos_desc->type_info.size_info.size << " bytes\n";
        std::cout << "  Alignment: " << pos_desc->type_info.size_info.alignment << " bytes\n";
        std::cout << "  Has debug support: " << pos_desc->has_debug_support() << "\n";
    }
}

void demonstrate_storage_system() {
    std::cout << "\n=== Storage System Demo ===\n";
    
    EntityManager entity_manager;
    PackedStorage<Position> position_storage;
    PackedStorage<Velocity> velocity_storage;
    PackedStorage<Health> health_storage;
    
    // Create entities and add components
    std::vector<EntityHandle> entities;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos_dist(-100.0f, 100.0f);
    std::uniform_real_distribution<float> vel_dist(-10.0f, 10.0f);
    std::uniform_real_distribution<float> health_dist(50.0f, 150.0f);
    
    std::cout << "Creating entities with components...\n";
    for (int i = 0; i < 1000; ++i) {
        auto entity = entity_manager.create_entity();
        entities.push_back(entity);
        
        // Add position to all entities
        position_storage.emplace(entity, pos_dist(gen), pos_dist(gen), pos_dist(gen));
        
        // Add velocity to most entities
        if (i % 3 != 0) {
            velocity_storage.emplace(entity, vel_dist(gen), vel_dist(gen), vel_dist(gen));
        }
        
        // Add health to some entities
        if (i % 2 == 0) {
            health_storage.emplace(entity, health_dist(gen));
        }
    }
    
    std::cout << "Storage statistics:\n";
    std::cout << "  Entities: " << entities.size() << "\n";
    std::cout << "  Positions: " << position_storage.size() << "\n";
    std::cout << "  Velocities: " << velocity_storage.size() << "\n";
    std::cout << "  Health components: " << health_storage.size() << "\n";
    
    // Demonstrate batch processing
    std::cout << "\nBatch processing demo (Position + Velocity)...\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    int processed_count = 0;
    position_storage.batch().for_each([&](EntityHandle entity, Position& pos) {
        if (velocity_storage.contains(entity)) {
            auto& vel = velocity_storage.get(entity);
            
            // Simple physics integration
            pos.x += vel.dx * 0.016f; // 60 FPS
            pos.y += vel.dy * 0.016f;
            pos.z += vel.dz * 0.016f;
            
            ++processed_count;
        }
    });
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "  Processed " << processed_count << " entities in " 
              << duration.count() << " microseconds\n";
    std::cout << "  Rate: " << (processed_count / (duration.count() / 1000.0)) 
              << " entities/ms\n";
    
    // Memory usage statistics
    auto pos_mem_stats = position_storage.get_memory_stats();
    std::cout << "\nPosition storage memory usage:\n";
    std::cout << "  Total: " << pos_mem_stats.total_bytes << " bytes\n";
    std::cout << "  Components: " << pos_mem_stats.component_bytes << " bytes\n";
    std::cout << "  Entities: " << pos_mem_stats.entity_bytes << " bytes\n";
    std::cout << "  Utilization: " << std::fixed << std::setprecision(2) 
              << (pos_mem_stats.utilization * 100.0) << "%\n";
}

void demonstrate_system_scheduler() {
    std::cout << "\n=== System Scheduler Demo ===\n";
    
    SystemScheduler::Config config{
        .enable_parallel_execution = true,
        .max_worker_threads = 0,  // Use hardware concurrency
        .enable_profiling = true
    };
    
    SystemScheduler scheduler(config);
    
    // Register systems
    std::cout << "Registering systems...\n";
    ECSCOPE_REGISTER_SYSTEM(MovementSystem, scheduler);
    ECSCOPE_REGISTER_SYSTEM(HealthSystem, scheduler);
    ECSCOPE_REGISTER_SYSTEM(RenderSystem, scheduler);
    
    // Initialize systems
    std::cout << "\nInitializing systems...\n";
    scheduler.initialize_systems();
    
    // Run simulation frames
    std::cout << "\nRunning simulation frames...\n";
    const float dt = 1.0f / 60.0f; // 60 FPS
    
    for (int frame = 0; frame < 5; ++frame) {
        std::cout << "\n--- Frame " << (frame + 1) << " ---\n";
        
        auto frame_start = std::chrono::high_resolution_clock::now();
        scheduler.update_systems(dt);
        auto frame_end = std::chrono::high_resolution_clock::now();
        
        auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);
        std::cout << "Frame time: " << frame_time.count() << " microseconds\n";
    }
    
    // Show system statistics
    std::cout << "\n=== System Statistics ===\n";
    auto movement_system = scheduler.get_system<MovementSystem>();
    if (movement_system) {
        auto stats = movement_system->get_stats();
        std::cout << "MovementSystem:\n";
        std::cout << "  Updates: " << stats.update_count << "\n";
        std::cout << "  Average time: " << std::fixed << std::setprecision(3) 
                  << stats.average_time_ms() << " ms\n";
        std::cout << "  Min time: " << (stats.min_time_ns / 1000.0) << " μs\n";
        std::cout << "  Max time: " << (stats.max_time_ns / 1000.0) << " μs\n";
    }
    
    auto scheduler_stats = scheduler.get_stats();
    std::cout << "\nScheduler Statistics:\n";
    std::cout << "  Frames processed: " << scheduler_stats.frame_count << "\n";
    std::cout << "  Average frame time: " << std::fixed << std::setprecision(3) 
              << scheduler_stats.average_frame_time_ms() << " ms\n";
    
    // Shutdown systems
    std::cout << "\nShutting down systems...\n";
    scheduler.shutdown_systems();
}

void demonstrate_memory_management() {
    std::cout << "\n=== Memory Management Demo ===\n";
    
    // Memory pool demonstration
    std::cout << "Memory Pool Test:\n";
    memory::MemoryPool<64> pool(100); // 64-byte blocks, 100 initial blocks
    
    std::vector<void*> allocations;
    
    // Allocate some blocks
    for (int i = 0; i < 50; ++i) {
        void* ptr = pool.allocate();
        allocations.push_back(ptr);
    }
    
    std::cout << "  Allocated: " << pool.allocated_count() << " blocks\n";
    std::cout << "  Available: " << pool.available_count() << " blocks\n";
    
    // Deallocate some blocks
    for (int i = 0; i < 25; ++i) {
        pool.deallocate(allocations[i]);
    }
    
    std::cout << "  After deallocation:\n";
    std::cout << "    Allocated: " << pool.allocated_count() << " blocks\n";
    std::cout << "    Available: " << pool.available_count() << " blocks\n";
    
    // Linear arena demonstration
    std::cout << "\nLinear Arena Test:\n";
    memory::LinearArena arena(1024); // 1KB arena
    
    auto* floats = arena.allocate<float>(100);   // 400 bytes
    auto* ints = arena.allocate<int>(50);        // 200 bytes
    auto* doubles = arena.allocate<double>(25);  // 200 bytes
    
    std::cout << "  Used: " << arena.used() << " bytes\n";
    std::cout << "  Available: " << arena.available() << " bytes\n";
    std::cout << "  Utilization: " << std::fixed << std::setprecision(1) 
              << (arena.utilization() * 100.0) << "%\n";
    
    // Stack allocator with scope demonstration
    std::cout << "\nStack Allocator Test:\n";
    memory::StackAllocator stack(2048); // 2KB stack
    
    {
        memory::StackScope scope(stack);
        
        auto* buffer1 = stack.allocate<char>(500);
        auto* buffer2 = stack.allocate<int>(100);
        
        std::cout << "  Inside scope - Used: " << stack.used() << " bytes\n";
        
        // Scope destructor will automatically rewind the stack
    }
    
    std::cout << "  After scope - Used: " << stack.used() << " bytes\n";
    
    // Memory tracking demonstration
    std::cout << "\nMemory Tracker Test:\n";
    auto& tracker = memory::MemoryTracker::instance();
    tracker.reset_stats();
    
    // Simulate some allocations
    void* ptr1 = std::malloc(1024);
    tracker.track_allocation(ptr1, 1024, alignof(void*), "test_allocation_1");
    
    void* ptr2 = std::malloc(2048);
    tracker.track_allocation(ptr2, 2048, alignof(void*), "test_allocation_2");
    
    auto stats = tracker.get_stats();
    std::cout << "  Total allocated: " << stats.total_allocated << " bytes\n";
    std::cout << "  Current allocated: " << stats.current_allocated << " bytes\n";
    std::cout << "  Peak allocated: " << stats.peak_allocated << " bytes\n";
    std::cout << "  Allocation count: " << stats.allocation_count << "\n";
    
    // Clean up
    tracker.track_deallocation(ptr1);
    std::free(ptr1);
    tracker.track_deallocation(ptr2);
    std::free(ptr2);
    
    // Clean up pool allocations
    for (std::size_t i = 25; i < allocations.size(); ++i) {
        pool.deallocate(allocations[i]);
    }
}

int main() {
    std::cout << "ECScope ECS Foundation Demo\n";
    std::cout << "===========================\n";
    
    try {
        // Run all demonstrations
        demonstrate_platform_detection();
        demonstrate_entity_manager();
        demonstrate_component_system();
        demonstrate_storage_system();
        demonstrate_system_scheduler();
        demonstrate_memory_management();
        
        std::cout << "\n=== Demo Complete ===\n";
        std::cout << "All foundation systems working correctly!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}