/**
 * @file visual_ecs_inspector_demo.cpp
 * @brief Comprehensive demonstration of the Visual ECS Inspector
 * 
 * This demo shows how to integrate and use the Visual ECS Inspector with a
 * real ECS system, including archetype visualization, system profiling,
 * memory analysis, and sparse set monitoring.
 */

#include "ecscope/visual_ecs_inspector.hpp"
#include "ecscope/sparse_set_visualizer.hpp"
#include "ecscope/memory_tracker.hpp"
#include "ecs/registry.hpp"
#include "ecs/system.hpp"
#include "overlay.hpp"
#include "core/log.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <random>

using namespace ecscope;

// Example components for demonstration
struct Transform {
    f32 x, y, z;
    f32 rotation;
    
    Transform(f32 x = 0, f32 y = 0, f32 z = 0, f32 rot = 0)
        : x(x), y(y), z(z), rotation(rot) {}
};

struct Velocity {
    f32 dx, dy, dz;
    
    Velocity(f32 dx = 0, f32 dy = 0, f32 dz = 0)
        : dx(dx), dy(dy), dz(dz) {}
};

struct Renderable {
    u32 mesh_id;
    u32 texture_id;
    bool visible;
    
    Renderable(u32 mesh = 0, u32 texture = 0, bool vis = true)
        : mesh_id(mesh), texture_id(texture), visible(vis) {}
};

struct Health {
    f32 current;
    f32 maximum;
    
    Health(f32 max = 100.0f) : current(max), maximum(max) {}
};

struct AI {
    enum class State { Idle, Patrol, Chase, Attack } state;
    f32 detection_radius;
    f32 attack_range;
    
    AI(State s = State::Idle, f32 detect = 10.0f, f32 attack = 2.0f)
        : state(s), detection_radius(detect), attack_range(attack) {}
};

// Example systems
class MovementSystem : public ecs::UpdateSystem {
public:
    MovementSystem() : UpdateSystem("Movement System") {
        reads<Transform>().writes<Transform>().reads<Velocity>();
    }
    
    void update(const ecs::SystemContext& context) override {
        TRACK_SPARSE_SET_ITERATE("Transform");
        TRACK_SPARSE_SET_ITERATE("Velocity");
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        context.registry().for_each<Transform, Velocity>([&](ecs::Entity entity, Transform& transform, const Velocity& velocity) {
            TRACK_SPARSE_SET_ACCESS("Transform", &transform, sizeof(Transform), true, 
                                   visualization::SparseSetAccessPattern::Sequential);
            TRACK_SPARSE_SET_ACCESS("Velocity", const_cast<Velocity*>(&velocity), sizeof(Velocity), false,
                                   visualization::SparseSetAccessPattern::Sequential);
            
            // Simple movement update
            transform.x += velocity.dx * static_cast<f32>(context.delta_time());
            transform.y += velocity.dy * static_cast<f32>(context.delta_time());
            transform.z += velocity.dz * static_cast<f32>(context.delta_time());
        });
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::micro>(end_time - start_time).count();
        
        // Simulate some processing time for demonstration
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
};

class RenderSystem : public ecs::RenderSystem {
public:
    RenderSystem() : RenderSystem("Render System") {
        reads<Transform>().reads<Renderable>();
        set_time_budget(0.008); // 8ms budget for 120 FPS
    }
    
    void update(const ecs::SystemContext& context) override {
        TRACK_SPARSE_SET_ITERATE("Transform");
        TRACK_SPARSE_SET_ITERATE("Renderable");
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        usize rendered_count = 0;
        
        context.registry().for_each<Transform, Renderable>([&](ecs::Entity entity, const Transform& transform, const Renderable& renderable) {
            if (renderable.visible) {
                // Simulate rendering work
                TRACK_SPARSE_SET_ACCESS("Transform", const_cast<Transform*>(&transform), sizeof(Transform), false,
                                       visualization::SparseSetAccessPattern::Sequential);
                TRACK_SPARSE_SET_ACCESS("Renderable", const_cast<Renderable*>(&renderable), sizeof(Renderable), false,
                                       visualization::SparseSetAccessPattern::Sequential);
                
                // Simulate GPU command submission
                ++rendered_count;
            }
        });
        
        // Simulate variable rendering time based on number of objects
        auto base_time = std::chrono::microseconds(500); // Base rendering overhead
        auto per_object_time = std::chrono::microseconds(rendered_count * 10); // 10μs per object
        std::this_thread::sleep_for(base_time + per_object_time);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::micro>(end_time - start_time).count();
        
        if (duration > time_budget() * 1000000) { // Convert seconds to microseconds
            LOG_WARN("Render system exceeded time budget: {:.2f}μs > {:.2f}μs", 
                    duration, time_budget() * 1000000);
        }
    }
};

class AISystem : public ecs::UpdateSystem {
public:
    AISystem() : UpdateSystem("AI System") {
        reads<Transform>().writes<AI>().reads<Health>();
        set_time_budget(0.005); // 5ms budget
    }
    
    void update(const ecs::SystemContext& context) override {
        TRACK_SPARSE_SET_ITERATE("AI");
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<f32> dis(0.0f, 1.0f);
        
        context.registry().for_each<Transform, AI, Health>([&](ecs::Entity entity, const Transform& transform, AI& ai, const Health& health) {
            TRACK_SPARSE_SET_ACCESS("AI", &ai, sizeof(AI), true,
                                   visualization::SparseSetAccessPattern::Random);
            
            // Simple AI state machine
            if (health.current <= 0) {
                return; // Dead entities don't think
            }
            
            switch (ai.state) {
                case AI::State::Idle:
                    if (dis(gen) < 0.01f) { // 1% chance per frame to start patrolling
                        ai.state = AI::State::Patrol;
                    }
                    break;
                    
                case AI::State::Patrol:
                    if (dis(gen) < 0.005f) { // 0.5% chance to return to idle
                        ai.state = AI::State::Idle;
                    }
                    break;
                    
                case AI::State::Chase:
                    if (dis(gen) < 0.02f) { // 2% chance to lose target
                        ai.state = AI::State::Patrol;
                    }
                    break;
                    
                case AI::State::Attack:
                    if (dis(gen) < 0.1f) { // 10% chance to finish attacking
                        ai.state = AI::State::Chase;
                    }
                    break;
            }
            
            // Simulate AI computation time
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        });
    }
};

// Demo application class
class VisualECSInspectorDemo {
private:
    std::unique_ptr<ecs::Registry> registry_;
    std::unique_ptr<ecs::SystemManager> system_manager_;
    std::unique_ptr<ui::Overlay> overlay_;
    std::unique_ptr<ui::VisualECSInspector> inspector_;
    
    // Demo state
    bool running_;
    f64 last_time_;
    u32 frame_count_;
    
public:
    VisualECSInspectorDemo() 
        : running_(false), last_time_(0.0), frame_count_(0) {}
    
    bool initialize() {
        LOG_INFO("Initializing Visual ECS Inspector Demo");
        
        // Initialize memory tracking
        memory::TrackerConfig tracker_config;
        tracker_config.enable_tracking = true;
        tracker_config.enable_access_tracking = true;
        tracker_config.enable_heat_mapping = true;
        tracker_config.enable_leak_detection = true;
        memory::MemoryTracker::initialize(tracker_config);
        
        // Initialize sparse set analyzer
        visualization::GlobalSparseSetAnalyzer::initialize();
        
        // Create ECS registry with memory tracking
        ecs::AllocatorConfig ecs_config = ecs::AllocatorConfig::create_educational_focused();
        registry_ = std::make_unique<ecs::Registry>(ecs_config, \"Demo_Registry\");
        
        // Create system manager\n        system_manager_ = std::make_unique<ecs::SystemManager>(registry_.get());\n        \n        // Register systems\n        system_manager_->add_system<MovementSystem>();\n        system_manager_->add_system<RenderSystem>();\n        system_manager_->add_system<AISystem>();\n        \n        // Create UI overlay\n        overlay_ = std::make_unique<ui::Overlay>();\n        \n        // Create and register visual ECS inspector\n        inspector_ = ui::create_visual_ecs_inspector();\n        inspector_->show_archetype_graph(true);\n        inspector_->show_system_profiler(true);\n        inspector_->show_memory_visualizer(true);\n        inspector_->show_entity_browser(true);\n        inspector_->show_sparse_set_view(true);\n        inspector_->show_performance_timeline(true);\n        inspector_->show_educational_hints(true);\n        \n        overlay_->add_panel(std::move(inspector_));\n        \n        // Register sparse sets for visualization\n        auto& analyzer = visualization::GlobalSparseSetAnalyzer::instance();\n        analyzer.register_sparse_set(\"Transform\", 1000);\n        analyzer.register_sparse_set(\"Velocity\", 500);\n        analyzer.register_sparse_set(\"Renderable\", 800);\n        analyzer.register_sparse_set(\"Health\", 300);\n        analyzer.register_sparse_set(\"AI\", 100);\n        \n        // Initialize systems\n        system_manager_->initialize_all_systems();\n        \n        LOG_INFO(\"Demo initialized successfully\");\n        return true;\n    }\n    \n    void create_demo_entities() {\n        LOG_INFO(\"Creating demo entities\");\n        \n        std::random_device rd;\n        std::mt19937 gen(rd());\n        std::uniform_real_distribution<f32> pos_dis(-50.0f, 50.0f);\n        std::uniform_real_distribution<f32> vel_dis(-5.0f, 5.0f);\n        std::uniform_int_distribution<u32> mesh_dis(0, 9);\n        std::uniform_int_distribution<u32> texture_dis(0, 15);\n        std::uniform_real_distribution<f32> health_dis(50.0f, 150.0f);\n        \n        // Create various entity archetypes for interesting visualization\n        \n        // Archetype 1: Moving rendered objects (Transform + Velocity + Renderable)\n        for (int i = 0; i < 100; ++i) {\n            registry_->create_entity(\n                Transform(pos_dis(gen), pos_dis(gen), pos_dis(gen)),\n                Velocity(vel_dis(gen), vel_dis(gen), vel_dis(gen)),\n                Renderable(mesh_dis(gen), texture_dis(gen), true)\n            );\n        }\n        \n        // Archetype 2: Static rendered objects (Transform + Renderable)\n        for (int i = 0; i < 50; ++i) {\n            registry_->create_entity(\n                Transform(pos_dis(gen), pos_dis(gen), pos_dis(gen)),\n                Renderable(mesh_dis(gen), texture_dis(gen), true)\n            );\n        }\n        \n        // Archetype 3: AI entities with health (Transform + AI + Health + Renderable)\n        for (int i = 0; i < 25; ++i) {\n            registry_->create_entity(\n                Transform(pos_dis(gen), pos_dis(gen), pos_dis(gen)),\n                AI(AI::State::Idle, 10.0f, 2.0f),\n                Health(health_dis(gen)),\n                Renderable(mesh_dis(gen), texture_dis(gen), true)\n            );\n        }\n        \n        // Archetype 4: Moving AI entities (all components)\n        for (int i = 0; i < 15; ++i) {\n            registry_->create_entity(\n                Transform(pos_dis(gen), pos_dis(gen), pos_dis(gen)),\n                Velocity(vel_dis(gen) * 0.5f, vel_dis(gen) * 0.5f, vel_dis(gen) * 0.5f),\n                AI(AI::State::Patrol, 15.0f, 3.0f),\n                Health(health_dis(gen)),\n                Renderable(mesh_dis(gen), texture_dis(gen), true)\n            );\n        }\n        \n        // Archetype 5: Invisible moving objects (Transform + Velocity)\n        for (int i = 0; i < 30; ++i) {\n            registry_->create_entity(\n                Transform(pos_dis(gen), pos_dis(gen), pos_dis(gen)),\n                Velocity(vel_dis(gen), vel_dis(gen), vel_dis(gen))\n            );\n        }\n        \n        LOG_INFO(\"Created {} entities across {} archetypes\", \n                registry_->active_entities(), registry_->archetype_count());\n    }\n    \n    void run_frame(f64 delta_time) {\n        // Update ECS systems\n        system_manager_->execute_frame(delta_time);\n        \n        // Update inspector data from real systems\n        if (inspector_) {\n            ui::visual_inspector_integration::update_from_registry(*inspector_, *registry_);\n            ui::visual_inspector_integration::update_from_system_manager(*inspector_, *system_manager_);\n            ui::visual_inspector_integration::update_from_memory_tracker(*inspector_, memory::MemoryTracker::instance());\n        }\n        \n        // Update sparse set analyzer\n        auto& analyzer = visualization::GlobalSparseSetAnalyzer::instance();\n        analyzer.analyze_all();\n        \n        // Update UI\n        if (overlay_) {\n            overlay_->update(delta_time);\n        }\n        \n        ++frame_count_;\n        \n        // Periodically create and destroy entities to show archetype dynamics\n        if (frame_count_ % 300 == 0) { // Every 5 seconds at 60 FPS\n            // Create some new entities\n            std::random_device rd;\n            std::mt19937 gen(rd());\n            std::uniform_real_distribution<f32> pos_dis(-20.0f, 20.0f);\n            std::uniform_real_distribution<f32> vel_dis(-2.0f, 2.0f);\n            \n            for (int i = 0; i < 5; ++i) {\n                registry_->create_entity(\n                    Transform(pos_dis(gen), pos_dis(gen), pos_dis(gen)),\n                    Velocity(vel_dis(gen), vel_dis(gen), vel_dis(gen)),\n                    Renderable(0, 0, true)\n                );\n            }\n        }\n        \n        // Show memory pressure demonstration\n        if (frame_count_ % 600 == 0) { // Every 10 seconds\n            auto pressure = memory::tracker::get_pressure_level();\n            if (pressure != memory::MemoryPressure::Level::Low) {\n                LOG_WARN(\"Memory pressure detected: {}\", static_cast<int>(pressure));\n            }\n        }\n    }\n    \n    void render() {\n        if (overlay_) {\n            overlay_->render();\n        }\n    }\n    \n    void shutdown() {\n        LOG_INFO(\"Shutting down Visual ECS Inspector Demo\");\n        \n        // Export analysis data\n        if (inspector_) {\n            inspector_->export_archetype_data(\"demo_archetype_analysis.json\");\n            inspector_->export_system_performance(\"demo_system_performance.csv\");\n            inspector_->export_memory_analysis(\"demo_memory_analysis.json\");\n            inspector_->export_performance_timeline(\"demo_performance_timeline.csv\");\n        }\n        \n        auto& analyzer = visualization::GlobalSparseSetAnalyzer::instance();\n        analyzer.export_analysis_report(\"demo_sparse_set_analysis.md\");\n        \n        // Shutdown systems\n        if (system_manager_) {\n            system_manager_->shutdown_all_systems();\n        }\n        \n        // Cleanup\n        inspector_.reset();\n        overlay_.reset();\n        system_manager_.reset();\n        registry_.reset();\n        \n        // Shutdown global systems\n        visualization::GlobalSparseSetAnalyzer::shutdown();\n        memory::MemoryTracker::shutdown();\n        \n        LOG_INFO(\"Demo shutdown complete\");\n    }\n    \n    // Accessors\n    bool is_running() const { return running_; }\n    void set_running(bool running) { running_ = running; }\n    const ecs::Registry& registry() const { return *registry_; }\n    const ecs::SystemManager& system_manager() const { return *system_manager_; }\n    ui::VisualECSInspector* inspector() { return inspector_.get(); }\n};\n\n// Main demo function\nint main() {\n    // Initialize logging\n    core::log::initialize();\n    LOG_INFO(\"Starting Visual ECS Inspector Demo\");\n    \n    VisualECSInspectorDemo demo;\n    \n    if (!demo.initialize()) {\n        LOG_ERROR(\"Failed to initialize demo\");\n        return -1;\n    }\n    \n    demo.create_demo_entities();\n    demo.set_running(true);\n    \n    LOG_INFO(\"Demo initialized. Visual ECS Inspector features:\");\n    LOG_INFO(\"  - Real-time archetype visualization with entity relationships\");\n    LOG_INFO(\"  - System execution profiling with bottleneck detection\");\n    LOG_INFO(\"  - Memory allocation tracking and visualization\");\n    LOG_INFO(\"  - Interactive entity browser with live editing\");\n    LOG_INFO(\"  - Sparse set storage analysis with cache locality metrics\");\n    LOG_INFO(\"  - Performance timeline with frame-by-frame analysis\");\n    LOG_INFO(\"  - Educational tooltips and ECS concept explanations\");\n    \n    // Main loop simulation\n    f64 last_time = 0.0;\n    const f64 target_frame_time = 1.0 / 60.0; // 60 FPS target\n    \n    for (u32 frame = 0; frame < 3600 && demo.is_running(); ++frame) { // Run for 60 seconds\n        f64 current_time = frame * target_frame_time;\n        f64 delta_time = current_time - last_time;\n        \n        demo.run_frame(delta_time);\n        \n        // Simulate frame rendering\n        demo.render();\n        \n        last_time = current_time;\n        \n        // Print status every 5 seconds\n        if (frame % 300 == 0) {\n            LOG_INFO(\"Demo running: Frame {}, Time: {:.1f}s, Entities: {}, Archetypes: {}\",\n                    frame, current_time, demo.registry().active_entities(), \n                    demo.registry().archetype_count());\n        }\n    }\n    \n    // Show final statistics\n    LOG_INFO(\"Demo completed successfully!\");\n    LOG_INFO(\"Final statistics:\");\n    LOG_INFO(\"  - Total entities: {}\", demo.registry().total_entities_created());\n    LOG_INFO(\"  - Active entities: {}\", demo.registry().active_entities());\n    LOG_INFO(\"  - Total archetypes: {}\", demo.registry().archetype_count());\n    \n    if (auto* inspector = demo.inspector()) {\n        LOG_INFO(\"  - Archetype nodes: {}\", inspector->archetype_nodes().size());\n        LOG_INFO(\"  - System nodes: {}\", inspector->system_nodes().size());\n        \n        if (auto* memory_data = inspector->memory_data()) {\n            LOG_INFO(\"  - Memory allocations tracked: {}\", memory_data->blocks.size());\n            LOG_INFO(\"  - Total memory usage: {} KB\", memory_data->total_allocated / 1024);\n            LOG_INFO(\"  - Memory fragmentation: {:.1f}%\", memory_data->fragmentation_ratio * 100.0);\n            LOG_INFO(\"  - Cache hit rate: {:.1f}%\", memory_data->cache_hit_rate * 100.0);\n        }\n    }\n    \n    demo.shutdown();\n    \n    LOG_INFO(\"Visual ECS Inspector Demo completed successfully!\");\n    LOG_INFO(\"Check the exported files for detailed analysis data:\");\n    LOG_INFO(\"  - demo_archetype_analysis.json: Archetype relationship data\");\n    LOG_INFO(\"  - demo_system_performance.csv: System execution metrics\");\n    LOG_INFO(\"  - demo_memory_analysis.json: Memory allocation analysis\");\n    LOG_INFO(\"  - demo_performance_timeline.csv: Frame-by-frame performance data\");\n    LOG_INFO(\"  - demo_sparse_set_analysis.md: Sparse set storage analysis\");\n    \n    return 0;\n}