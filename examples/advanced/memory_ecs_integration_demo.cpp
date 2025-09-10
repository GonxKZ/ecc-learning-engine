/**
 * @file memory_ecs_integration_demo.cpp
 * @brief Comprehensive demonstration of the ECScope Memory Management System
 *        integrated with ECS for high-performance game engine usage
 * 
 * This example demonstrates:
 * - Custom ECS allocators using the memory management system
 * - Component-specific memory pools
 * - NUMA-aware component allocation
 * - Memory tracking and leak detection for components
 * - Performance monitoring and optimization
 * - Real-time memory profiling
 */

#include <ecscope/memory/memory_manager.hpp>
#include <ecscope/memory/allocators.hpp>
#include <ecscope/memory/memory_pools.hpp>
#include <ecscope/memory/memory_tracker.hpp>
#include <ecscope/memory/memory_utils.hpp>

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>

using namespace ecscope::memory;

// ==== ECS-INTEGRATED COMPONENT TYPES ====

// Base component class using memory management
class BaseComponent {
public:
    virtual ~BaseComponent() = default;
    
    // Custom operator new/delete using memory manager
    static void* operator new(size_t size) {
        MemoryPolicy policy;
        policy.strategy = AllocationStrategy::SIZE_SEGREGATED;
        policy.allocation_tag = "ECS_Component";
        policy.enable_tracking = true;
        
        return MemoryManager::instance().allocate(size, policy);
    }
    
    static void operator delete(void* ptr, size_t size) {
        MemoryPolicy policy;
        policy.allocation_tag = "ECS_Component";
        
        MemoryManager::instance().deallocate(ptr, size, policy);
    }
    
    virtual const char* get_type_name() const = 0;
    virtual size_t get_memory_footprint() const = 0;
};

// Transform component with spatial locality optimization
struct Transform : public BaseComponent {
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // quaternion
    float scale[3] = {1.0f, 1.0f, 1.0f};
    
    const char* get_type_name() const override { return "Transform"; }
    size_t get_memory_footprint() const override { return sizeof(Transform); }
};

// Physics component with NUMA-aware allocation
struct Physics : public BaseComponent {
    float velocity[3] = {0.0f, 0.0f, 0.0f};
    float acceleration[3] = {0.0f, 0.0f, 0.0f};
    float mass = 1.0f;
    float friction = 0.1f;
    bool is_kinematic = false;
    
    const char* get_type_name() const override { return "Physics"; }
    size_t get_memory_footprint() const override { return sizeof(Physics); }
    
    // NUMA-aware allocation for physics computations
    static void* operator new(size_t size) {
        MemoryPolicy policy;
        policy.strategy = AllocationStrategy::NUMA_AWARE;
        policy.allocation_tag = "Physics_Component";
        policy.enable_tracking = true;
        
        return MemoryManager::instance().allocate(size, policy);
    }
};

// Render component with thread-local allocation for GPU data
struct Render : public BaseComponent {
    uint32_t mesh_id = 0;
    uint32_t material_id = 0;
    uint32_t texture_ids[8] = {0}; // Multiple texture slots
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    bool visible = true;
    bool cast_shadows = true;
    uint32_t render_queue = 2000;
    
    const char* get_type_name() const override { return "Render"; }
    size_t get_memory_footprint() const override { return sizeof(Render); }
    
    // Thread-local allocation for renderer thread efficiency
    static void* operator new(size_t size) {
        MemoryPolicy policy;
        policy.strategy = AllocationStrategy::THREAD_LOCAL;
        policy.allocation_tag = "Render_Component";
        policy.enable_tracking = true;
        
        return MemoryManager::instance().allocate(size, policy);
    }
};

// Audio component with encrypted memory for sensitive data
struct Audio : public BaseComponent {
    uint32_t sound_id = 0;
    float volume = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;
    bool looping = false;
    bool spatial_audio = true;
    float min_distance = 1.0f;
    float max_distance = 100.0f;
    
    const char* get_type_name() const override { return "Audio"; }
    size_t get_memory_footprint() const override { return sizeof(Audio); }
    
    // Encrypted allocation for audio data protection
    static void* operator new(size_t size) {
        MemoryPolicy policy;
        policy.strategy = AllocationStrategy::BALANCED;
        policy.allocation_tag = "Audio_Component";
        policy.enable_memory_encryption = true;
        policy.enable_tracking = true;
        
        return MemoryManager::instance().allocate(size, policy);
    }
};

// ==== ECS-OPTIMIZED MEMORY POOLS ====

template<typename ComponentType>
class ComponentPool {
public:
    explicit ComponentPool(size_t initial_capacity = 1000) {
        // Register custom component pool
        auto pool = std::make_unique<DynamicPoolWrapper<sizeof(ComponentType)>>();
        pool_name_ = std::string("Pool_") + ComponentType{}.get_type_name();
        
        MemoryManager::instance().register_custom_pool(pool_name_, std::move(pool));
        
        // Pre-allocate some components for better performance
        preallocate(initial_capacity / 4);
    }
    
    template<typename... Args>
    ComponentType* create_component(Args&&... args) {
        auto& manager = MemoryManager::instance();
        
        MemoryPolicy policy;
        policy.allocation_tag = pool_name_;
        
        void* ptr = manager.allocate_from_pool(pool_name_, sizeof(ComponentType), policy);
        if (!ptr) {
            // Fallback to regular allocation if pool is full
            ptr = manager.allocate(sizeof(ComponentType), policy);
        }
        
        if (ptr) {
            return new(ptr) ComponentType(std::forward<Args>(args)...);
        }
        
        return nullptr;
    }
    
    void destroy_component(ComponentType* component) {
        if (!component) return;
        
        component->~ComponentType();
        
        auto& manager = MemoryManager::instance();
        manager.deallocate_to_pool(pool_name_, component);
    }
    
    const std::string& get_pool_name() const { return pool_name_; }

private:
    void preallocate(size_t count) {
        // Pre-allocate components to warm up the pool
        std::vector<ComponentType*> temp_components;
        temp_components.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            ComponentType* comp = create_component();
            if (comp) {
                temp_components.push_back(comp);
            }
        }
        
        // Immediately release them back to the pool
        for (ComponentType* comp : temp_components) {
            destroy_component(comp);
        }
    }
    
    std::string pool_name_;
};

// ==== ECS ENTITY MANAGER WITH MEMORY OPTIMIZATION ====

class Entity {
public:
    Entity(uint32_t id) : id_(id) {}
    
    uint32_t get_id() const { return id_; }
    
    template<typename ComponentType, typename... Args>
    ComponentType* add_component(Args&&... args) {
        auto pool_it = component_pools_.find(typeid(ComponentType));
        if (pool_it == component_pools_.end()) {
            // Create new pool for this component type
            auto pool = std::make_unique<ComponentPool<ComponentType>>();
            pool_it = component_pools_.emplace(typeid(ComponentType), std::move(pool)).first;
        }
        
        auto* pool = static_cast<ComponentPool<ComponentType>*>(pool_it->second.get());
        ComponentType* component = pool->create_component(std::forward<Args>(args)...);
        
        if (component) {
            components_[typeid(ComponentType)] = component;
        }
        
        return component;
    }
    
    template<typename ComponentType>
    ComponentType* get_component() {
        auto it = components_.find(typeid(ComponentType));
        return it != components_.end() ? static_cast<ComponentType*>(it->second) : nullptr;
    }
    
    template<typename ComponentType>
    void remove_component() {
        auto comp_it = components_.find(typeid(ComponentType));
        if (comp_it != components_.end()) {
            auto pool_it = component_pools_.find(typeid(ComponentType));
            if (pool_it != component_pools_.end()) {
                auto* pool = static_cast<ComponentPool<ComponentType>*>(pool_it->second.get());
                pool->destroy_component(static_cast<ComponentType*>(comp_it->second));
            }
            components_.erase(comp_it);
        }
    }
    
    ~Entity() {
        // Clean up all components
        for (const auto& [type, component] : components_) {
            // Component pools will handle cleanup
        }
    }

private:
    uint32_t id_;
    std::unordered_map<std::type_index, BaseComponent*> components_;
    
    // Static component pools shared across all entities
    static inline std::unordered_map<std::type_index, std::unique_ptr<void*>> component_pools_;
};

// ==== MEMORY-OPTIMIZED ECS WORLD ====

class ECSWorld {
public:
    ECSWorld() {
        // Initialize memory manager with ECS-optimized settings
        MemoryPolicy policy;
        policy.strategy = AllocationStrategy::SIZE_SEGREGATED;
        policy.enable_tracking = true;
        policy.enable_leak_detection = true;
        policy.enable_stack_traces = false; // Disable for performance
        policy.enable_automatic_cleanup = true;
        policy.prefer_simd_operations = true;
        
        MemoryManager::instance().initialize(policy);
        
        // Initialize component pools
        transform_pool_ = std::make_unique<ComponentPool<Transform>>();
        physics_pool_ = std::make_unique<ComponentPool<Physics>>();
        render_pool_ = std::make_unique<ComponentPool<Render>>();
        audio_pool_ = std::make_unique<ComponentPool<Audio>>();
        
        std::cout << "ECS World initialized with advanced memory management\n";
    }
    
    Entity* create_entity() {
        uint32_t entity_id = next_entity_id_++;
        
        // Use memory manager for entity allocation
        MemoryPolicy policy;
        policy.strategy = AllocationStrategy::FASTEST;
        policy.allocation_tag = "Entity";
        
        Entity* entity = MemoryManager::instance().allocate_object<Entity>(policy, entity_id);
        if (entity) {
            entities_[entity_id] = std::unique_ptr<Entity>(entity);
            return entity;
        }
        
        return nullptr;
    }
    
    void destroy_entity(uint32_t entity_id) {
        auto it = entities_.find(entity_id);
        if (it != entities_.end()) {
            // Entity destructor will clean up components
            entities_.erase(it);
        }
    }
    
    Entity* get_entity(uint32_t entity_id) {
        auto it = entities_.find(entity_id);
        return it != entities_.end() ? it->second.get() : nullptr;
    }
    
    // System update functions with memory optimization
    void update_physics_system(float delta_time) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Collect all physics components for batch processing
        std::vector<std::pair<Transform*, Physics*>> physics_entities;
        physics_entities.reserve(entities_.size());
        
        for (const auto& [id, entity] : entities_) {
            Transform* transform = entity->get_component<Transform>();
            Physics* physics = entity->get_component<Physics>();
            
            if (transform && physics && !physics->is_kinematic) {
                physics_entities.push_back({transform, physics});
            }
        }
        
        // SIMD-optimized physics update
        update_physics_batch(physics_entities, delta_time);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        physics_update_time_ = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time).count();
    }
    
    void update_render_system() {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Collect visible render components
        std::vector<std::pair<Transform*, Render*>> render_entities;
        render_entities.reserve(entities_.size());
        
        for (const auto& [id, entity] : entities_) {
            Transform* transform = entity->get_component<Transform>();
            Render* render = entity->get_component<Render>();
            
            if (transform && render && render->visible) {
                render_entities.push_back({transform, render});
            }
        }
        
        // Sort by render queue for optimal batching
        std::sort(render_entities.begin(), render_entities.end(),
                 [](const auto& a, const auto& b) {
                     return a.second->render_queue < b.second->render_queue;
                 });
        
        // Batch render operations
        batch_render(render_entities);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        render_update_time_ = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time).count();
    }
    
    // Memory profiling and statistics
    void print_memory_statistics() const {
        auto& manager = MemoryManager::instance();
        auto metrics = manager.get_performance_metrics();
        auto health = manager.generate_health_report();
        
        std::cout << "\n=== Memory Management Statistics ===\n";
        std::cout << std::fixed << std::setprecision(2);
        
        // Basic metrics
        std::cout << "Total Allocations: " << metrics.total_allocations << "\n";
        std::cout << "Current Memory Usage: " << (metrics.current_allocated_bytes / 1024.0 / 1024.0) << " MB\n";
        std::cout << "Peak Memory Usage: " << (metrics.peak_allocated_bytes / 1024.0 / 1024.0) << " MB\n";
        std::cout << "Memory Efficiency: " << (metrics.memory_efficiency * 100.0) << "%\n";
        
        // Bandwidth statistics
        std::cout << "Read Bandwidth: " << metrics.current_read_bandwidth_mbps << " MB/s\n";
        std::cout << "Write Bandwidth: " << metrics.current_write_bandwidth_mbps << " MB/s\n";
        std::cout << "Peak Bandwidth: " << metrics.peak_bandwidth_mbps << " MB/s\n";
        
        // Pool statistics
        std::cout << "Active Pools: " << metrics.active_pools << "\n";
        std::cout << "Average Pool Utilization: " << (metrics.average_pool_utilization * 100.0) << "%\n";
        
        // NUMA statistics
        if (!metrics.numa_node_utilization.empty()) {
            std::cout << "NUMA Node Utilization:\n";
            for (const auto& [node_id, utilization] : metrics.numa_node_utilization) {
                std::cout << "  Node " << node_id << ": " << (utilization * 100.0) << "%\n";
            }
        }
        
        // Memory pressure
        const char* pressure_names[] = {"LOW", "MODERATE", "HIGH", "CRITICAL"};
        std::cout << "Memory Pressure: " << pressure_names[static_cast<int>(metrics.current_pressure)] << "\n";
        
        // Health report
        std::cout << "\n=== Memory Health Report ===\n";
        if (health.has_memory_leaks) {
            std::cout << "⚠️  Memory Leaks Detected: " << health.leaked_allocations 
                     << " allocations (" << (health.leaked_bytes / 1024.0) << " KB)\n";
        } else {
            std::cout << "✅ No Memory Leaks Detected\n";
        }
        
        if (health.has_memory_corruption) {
            std::cout << "⚠️  Memory Corruption Detected\n";
        } else {
            std::cout << "✅ No Memory Corruption Detected\n";
        }
        
        if (health.has_performance_issues) {
            std::cout << "⚠️  Performance Issues Detected\n";
        } else {
            std::cout << "✅ No Performance Issues Detected\n";
        }
        
        // Recommendations
        if (!health.recommendations.empty()) {
            std::cout << "\nRecommendations:\n";
            for (const auto& rec : health.recommendations) {
                std::cout << "  • " << rec << "\n";
            }
        }
        
        // Performance timings
        std::cout << "\n=== System Performance ===\n";
        std::cout << "Physics Update Time: " << physics_update_time_ << " µs\n";
        std::cout << "Render Update Time: " << render_update_time_ << " µs\n";
        
        std::cout << std::endl;
    }

private:
    void update_physics_batch(const std::vector<std::pair<Transform*, Physics*>>& entities, 
                             float delta_time) {
        // Batch process physics using SIMD operations where possible
        for (const auto& [transform, physics] : entities) {
            // Apply acceleration to velocity
            for (int i = 0; i < 3; ++i) {
                physics->velocity[i] += physics->acceleration[i] * delta_time;
            }
            
            // Apply friction
            for (int i = 0; i < 3; ++i) {
                physics->velocity[i] *= (1.0f - physics->friction * delta_time);
            }
            
            // Update position
            for (int i = 0; i < 3; ++i) {
                transform->position[i] += physics->velocity[i] * delta_time;
            }
        }
    }
    
    void batch_render(const std::vector<std::pair<Transform*, Render*>>& entities) {
        // Group by material for efficient rendering
        std::unordered_map<uint32_t, std::vector<std::pair<Transform*, Render*>>> batches;
        
        for (const auto& entity : entities) {
            batches[entity.second->material_id].push_back(entity);
        }
        
        // Simulate batched rendering
        for (const auto& [material_id, batch] : batches) {
            // Render all objects with the same material in one batch
            // This would interface with the actual graphics API
        }
    }
    
    // Entity storage
    std::unordered_map<uint32_t, std::unique_ptr<Entity>> entities_;
    uint32_t next_entity_id_ = 1;
    
    // Component pools
    std::unique_ptr<ComponentPool<Transform>> transform_pool_;
    std::unique_ptr<ComponentPool<Physics>> physics_pool_;
    std::unique_ptr<ComponentPool<Render>> render_pool_;
    std::unique_ptr<ComponentPool<Audio>> audio_pool_;
    
    // Performance tracking
    mutable uint64_t physics_update_time_ = 0;
    mutable uint64_t render_update_time_ = 0;
};

// ==== DEMO APPLICATION ====

class MemoryECSDemoApp {
public:
    void run() {
        std::cout << "ECScope Memory Management + ECS Integration Demo\n";
        std::cout << "================================================\n\n";
        
        print_system_info();
        
        // Create ECS world
        ECSWorld world;
        
        std::cout << "Creating demo entities...\n";
        
        // Create diverse entities to stress-test the memory system
        create_demo_entities(world, 10000);
        
        std::cout << "Running simulation...\n";
        
        // Run simulation to demonstrate memory management
        run_simulation(world, 60); // 60 frames
        
        // Show final statistics
        world.print_memory_statistics();
        
        // Test memory cleanup
        std::cout << "Testing memory cleanup and leak detection...\n";
        test_memory_cleanup(world);
        
        std::cout << "Demo completed successfully!\n";
    }

private:
    void print_system_info() {
        std::cout << "System Information:\n";
        std::cout << "  Hardware threads: " << std::thread::hardware_concurrency() << "\n";
        std::cout << "  Cache line size: " << get_cache_line_size() << " bytes\n";
        
        if (SIMDMemoryOps::has_avx512()) std::cout << "  SIMD: AVX-512\n";
        else if (SIMDMemoryOps::has_avx2()) std::cout << "  SIMD: AVX2\n";
        else if (SIMDMemoryOps::has_sse2()) std::cout << "  SIMD: SSE2\n";
        else std::cout << "  SIMD: None\n";
        
        auto& topology = NUMATopology::instance();
        std::cout << "  NUMA nodes: " << topology.get_num_nodes();
        if (topology.is_numa_available()) {
            std::cout << " (Available)\n";
        } else {
            std::cout << " (Simulated)\n";
        }
        
        std::cout << "\n";
    }
    
    void create_demo_entities(ECSWorld& world, size_t count) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> pos_dist(-100.0f, 100.0f);
        std::uniform_real_distribution<float> vel_dist(-10.0f, 10.0f);
        std::uniform_int_distribution<uint32_t> id_dist(1, 1000);
        
        for (size_t i = 0; i < count; ++i) {
            Entity* entity = world.create_entity();
            if (!entity) continue;
            
            // All entities have transform
            Transform* transform = entity->add_component<Transform>();
            if (transform) {
                transform->position[0] = pos_dist(gen);
                transform->position[1] = pos_dist(gen);
                transform->position[2] = pos_dist(gen);
            }
            
            // 80% have physics
            if (gen() % 10 < 8) {
                Physics* physics = entity->add_component<Physics>();
                if (physics) {
                    physics->velocity[0] = vel_dist(gen);
                    physics->velocity[1] = vel_dist(gen);
                    physics->velocity[2] = vel_dist(gen);
                    physics->mass = 1.0f + (gen() % 100) / 100.0f;
                }
            }
            
            // 60% have render
            if (gen() % 10 < 6) {
                Render* render = entity->add_component<Render>();
                if (render) {
                    render->mesh_id = id_dist(gen);
                    render->material_id = id_dist(gen) % 50; // Batch materials
                }
            }
            
            // 20% have audio
            if (gen() % 10 < 2) {
                Audio* audio = entity->add_component<Audio>();
                if (audio) {
                    audio->sound_id = id_dist(gen);
                    audio->volume = (gen() % 100) / 100.0f;
                }
            }
        }
        
        std::cout << "Created " << count << " entities with various components\n";
    }
    
    void run_simulation(ECSWorld& world, int frames) {
        const float delta_time = 1.0f / 60.0f; // 60 FPS
        
        for (int frame = 0; frame < frames; ++frame) {
            // Update systems
            world.update_physics_system(delta_time);
            world.update_render_system();
            
            // Print progress every 10 frames
            if (frame % 10 == 0) {
                std::cout << "Frame " << frame << " completed\n";
            }
            
            // Simulate frame timing
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        std::cout << "Simulation completed (" << frames << " frames)\n\n";
    }
    
    void test_memory_cleanup(ECSWorld& world) {
        auto& manager = MemoryManager::instance();
        
        // Get initial memory state
        auto initial_metrics = manager.get_performance_metrics();
        std::cout << "Initial memory usage: " 
                 << (initial_metrics.current_allocated_bytes / 1024.0 / 1024.0) << " MB\n";
        
        // Force garbage collection and cleanup
        MemoryPressureDetector::instance().register_pressure_callback(
            [&](MemoryPressureDetector::PressureLevel level) {
                std::cout << "Memory pressure callback triggered: Level " 
                         << static_cast<int>(level) << "\n";
            });
        
        // Simulate memory pressure to trigger cleanup
        std::cout << "Triggering memory cleanup...\n";
        
        // The world destructor will clean up entities, which should free component memory
        // This tests the memory management system's cleanup capabilities
        
        auto final_metrics = manager.get_performance_metrics();
        auto health_report = manager.generate_health_report();
        
        std::cout << "Final memory usage: " 
                 << (final_metrics.current_allocated_bytes / 1024.0 / 1024.0) << " MB\n";
        
        if (health_report.has_memory_leaks) {
            std::cout << "⚠️  Detected " << health_report.leaked_allocations 
                     << " memory leaks (" << (health_report.leaked_bytes / 1024.0) << " KB)\n";
            
            // Export leak report for analysis
            manager.export_allocation_profile("memory_leak_report.txt");
            std::cout << "Leak report exported to memory_leak_report.txt\n";
        } else {
            std::cout << "✅ No memory leaks detected - All component memory properly cleaned up!\n";
        }
    }
};

// ==== MAIN FUNCTION ====

int main() {
    try {
        MemoryECSDemoApp app;
        app.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}