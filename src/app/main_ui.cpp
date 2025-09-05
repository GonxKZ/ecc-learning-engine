#include "core/types.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "core/time.hpp"
#include "core/log.hpp"

// ECS includes
#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "ecs/components/transform.hpp"

// UI includes
#include "renderer/window.hpp"
#include "ui/overlay.hpp"
#include "ui/panels/panel_ecs_inspector.hpp"
#include "ui/panels/panel_memory.hpp"
#include "ui/panels/panel_stats.hpp"

// Interactive Learning System includes
#include "learning/interactive_learning_integration.hpp"

#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>

using namespace ecscope::core;
using namespace ecscope::ecs;
using namespace ecscope::ecs::components;
using namespace ecscope::renderer;
using namespace ecscope::ui;

// Application state
struct AppState {
    bool running{true};
    bool demo_running{false};
    std::vector<Entity> demo_entities;
    
    // Demo settings
    usize entity_count{1000};
    f32 movement_speed{50.0f};
    bool animate_entities{true};
    
    // Timing
    f64 last_demo_update{0.0};
    f64 demo_update_interval{1.0 / 60.0}; // 60 fps demo updates
};

// Function prototypes
bool initialize_graphics();
void shutdown_graphics();
void create_demo_entities(AppState& state);
void update_demo_entities(AppState& state, f64 delta_time);
void handle_events(AppState& state);
void render_frame(AppState& state, f64 delta_time);
void render_demo_controls(AppState& state);

// Helper function to get registry as shared pointer for learning integration
std::shared_ptr<ecs::Registry> get_registry_ptr() {
    static std::shared_ptr<ecs::Registry> registry_ptr;
    if (!registry_ptr) {
        // Create a shared pointer wrapper around the global registry
        registry_ptr = std::shared_ptr<ecs::Registry>(&get_registry(), [](ecs::Registry*){});
    }
    return registry_ptr;
}

int main() {
    // Welcome message
    LOG_INFO("ECScope v0.1.0 - Educational ECS Engine with UI");
    LOG_INFO("Memory Observatory & Data Layout Laboratory");
    LOG_INFO("Built with C++20 + SDL2 + ImGui");
    
    // Show instrumentation status
#ifdef ECSCOPE_ENABLE_INSTRUMENTATION
    LOG_INFO("Instrumentation: ENABLED");
#else
    LOG_INFO("Instrumentation: DISABLED");
#endif

#ifdef ECSCOPE_HAS_GRAPHICS
    LOG_INFO("Graphics: ENABLED");
#else
    LOG_INFO("Graphics: DISABLED - falling back to console demo");
    // Fall back to console-only version
    system("./ecscope_app");
    return 0;
#endif

    // Initialize core systems
    performance_profiler::initialize();
    memory_tracker::initialize();
    
    // Initialize Interactive Learning System
    auto& learning_integration = ecscope::learning::get_learning_integration();
    
    // Initialize graphics
    if (!initialize_graphics()) {
        LOG_ERROR("Failed to initialize graphics system");
        return -1;
    }
    
    AppState app_state;
    
    // Create initial demo entities
    create_demo_entities(app_state);
    
    // Main application loop
    auto& delta_time_tracker = delta_time();
    delta_time_tracker.update(); // Initialize
    
    LOG_INFO("=== Starting UI Application Loop ===");
    
    while (app_state.running) {
        PROFILE_FUNCTION();
        
        // Update timing
        delta_time_tracker.update();
        f64 frame_delta = delta_time_tracker.delta_seconds();
        
        // Handle events
        handle_events(app_state);
        
        // Update demo entities
        if (app_state.demo_running && app_state.animate_entities) {
            update_demo_entities(app_state, frame_delta);
        }
        
        // Render frame
        render_frame(app_state, frame_delta);
        
        // Update performance stats
        auto& stats_panel = *static_cast<PerformanceStatsPanel*>(
            get_ui_overlay().get_panel("Performance Stats"));
        if (stats_panel.is_visible()) {
            stats_panel.record_frame_time(frame_delta * 1000.0);
        }
    }
    
    LOG_INFO("=== Shutting Down ===");
    
    // Cleanup
    shutdown_graphics();
    performance_profiler::shutdown();
    memory_tracker::shutdown();
    
    LOG_INFO("ECScope shutdown complete");
    return 0;
}

bool initialize_graphics() {
    PROFILE_FUNCTION();
    
    // Create main window
    WindowConfig config;
    config.title = "ECScope - ECS Engine Observatory";
    config.width = 1400;
    config.height = 900;
    config.resizable = true;
    config.vsync = true;
    
    auto& window = get_main_window();
    if (auto result = window.create(); result.is_err()) {
        LOG_ERROR("Failed to create window");
        return false;
    }
    
    // Initialize UI overlay
    auto& ui = get_ui_overlay();
    if (auto result = ui.initialize(window); result.is_err()) {
        LOG_ERROR("Failed to initialize UI overlay");
        return false;
    }
    
    // Create UI panels
    auto* ecs_inspector = ui.add_panel<ECSInspectorPanel>();
    auto* memory_observer = ui.add_panel<MemoryObserverPanel>();
    auto* stats_panel = ui.add_panel<PerformanceStatsPanel>();
    
    // Initialize Interactive Learning System with UI
    auto& learning_integration = ecscope::learning::get_learning_integration();
    learning_integration.initialize(ui, get_registry_ptr());
    
    // Configure panels
    ecs_inspector->set_visible(true);
    memory_observer->set_visible(false); // Start hidden
    stats_panel->set_visible(true);
    stats_panel->set_target_fps(60.0);
    
    LOG_INFO("Graphics system initialized successfully");
    return true;
}

void shutdown_graphics() {
    get_ui_overlay().shutdown();
    get_main_window().destroy();
}

void create_demo_entities(AppState& state) {
    PROFILE_SCOPE("CreateDemoEntities");
    
    auto& registry = get_registry();
    state.demo_entities.clear();
    state.demo_entities.reserve(state.entity_count);
    
    // Random number generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<f32> pos_dist(-500.0f, 500.0f);
    std::uniform_real_distribution<f32> rot_dist(0.0f, 6.28318f); // 0 to 2π
    std::uniform_real_distribution<f32> scale_dist(0.5f, 2.0f);
    
    Timer creation_timer;
    
    for (usize i = 0; i < state.entity_count; ++i) {
        Transform transform{
            Vec2{pos_dist(gen), pos_dist(gen)}, // Random position
            rot_dist(gen),                      // Random rotation  
            Vec2{scale_dist(gen), scale_dist(gen)} // Random scale
        };
        
        Entity entity = registry.create_entity(std::move(transform));
        state.demo_entities.push_back(entity);
        
        // Add velocity to some entities for animation
        if (i % 3 == 0) { // Every 3rd entity gets velocity
            Vec2 velocity{pos_dist(gen) * 0.1f, pos_dist(gen) * 0.1f};
            registry.add_component<Vec2>(entity, velocity);
        }
    }
    
    f64 creation_time = creation_timer.elapsed_milliseconds();
    
    std::ostringstream oss;
    oss << "Created " << state.entity_count << " demo entities in " 
        << creation_time << " ms";
    LOG_INFO(oss.str());
    
    state.demo_running = true;
}

void update_demo_entities(AppState& state, f64 delta_time) {
    PROFILE_SCOPE("UpdateDemoEntities");
    
    state.last_demo_update += delta_time;
    if (state.last_demo_update < state.demo_update_interval) {
        return;
    }
    
    auto& registry = get_registry();
    
    // Update entities with both Transform and Vec2 (velocity)
    registry.for_each<Transform, Vec2>([&](Entity entity, Transform& transform, Vec2& velocity) {
        (void)entity; // Unused in this simple update
        
        // Simple movement
        transform.translate(velocity * state.movement_speed * static_cast<f32>(state.last_demo_update));
        
        // Bounce off screen boundaries (simple wrapping)
        if (transform.position.x < -600.0f || transform.position.x > 600.0f) {
            velocity.x = -velocity.x;
        }
        if (transform.position.y < -400.0f || transform.position.y > 400.0f) {
            velocity.y = -velocity.y;
        }
        
        // Gentle rotation
        transform.rotate(0.5f * static_cast<f32>(state.last_demo_update));
    });
    
    state.last_demo_update = 0.0;
}

void handle_events(AppState& state) {
    PROFILE_SCOPE("HandleEvents");
    
    auto& window = get_main_window();
    
    WindowEvent event = window.poll_event();
    switch (event) {
        case WindowEvent::Close:
            state.running = false;
            break;
            
        case WindowEvent::Resize:
            // Window resize handled automatically
            break;
            
        default:
            break;
    }
    
    // Pass events to UI
    get_ui_overlay().handle_window_event(event);
}

void render_frame(AppState& state, f64 delta_time) {
    PROFILE_SCOPE("RenderFrame");
    
    auto& window = get_main_window();
    auto& ui = get_ui_overlay();
    
    // Clear screen
    window.clear(0.1f, 0.1f, 0.15f, 1.0f); // Dark blue background
    
    // Begin UI frame
    ui.begin_frame();
    
    // Update UI
    ui.update(delta_time);
    
    // Render custom demo controls window
    render_demo_controls(state);
    
    // Render UI
    ui.render();
    
    // End UI frame
    ui.end_frame();
    
    // Present
    window.swap_buffers();
    window.update_stats(delta_time * 1000.0); // Convert to milliseconds
}

void render_demo_controls(AppState& state) {
#ifdef ECSCOPE_HAS_GRAPHICS
    PROFILE_SCOPE("RenderDemoControls");
    
    if (ImGui::Begin("ECS Demo Controls")) {
        // Demo status
        ImGui::Text("Demo Status: %s", state.demo_running ? "Running" : "Stopped");
        
        if (ImGui::Button(state.demo_running ? "Stop Demo" : "Start Demo")) {
            state.demo_running = !state.demo_running;
        }
        
        ImGui::Separator();
        
        // Entity count control
        int entity_count = static_cast<int>(state.entity_count);
        if (ImGui::SliderInt("Entity Count", &entity_count, 100, 10000)) {
            state.entity_count = static_cast<usize>(entity_count);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Recreate Entities")) {
            create_demo_entities(state);
        }
        
        // Movement controls
        ImGui::SliderFloat("Movement Speed", &state.movement_speed, 0.0f, 200.0f);
        ImGui::Checkbox("Animate Entities", &state.animate_entities);
        
        ImGui::Separator();
        
        // Registry statistics
        auto& registry = get_registry();
        ImGui::Text("Registry Statistics:");
        ImGui::Text("  Total Entities: %zu", registry.total_entities_created());
        ImGui::Text("  Active Entities: %zu", registry.active_entities());
        ImGui::Text("  Archetypes: %zu", registry.archetype_count());
        ImGui::Text("  Memory Usage: %.2f MB", 
                   registry.memory_usage() / (1024.0 * 1024.0));
        
        // Performance info
        ImGui::Separator();
        ImGui::Text("Performance:");
        auto& delta_time_tracker = delta_time();
        ImGui::Text("  FPS: %.1f", delta_time_tracker.fps());
        ImGui::Text("  Frame Time: %.2f ms", delta_time_tracker.delta_milliseconds());
        
        // Memory tracking controls
        ImGui::Separator();
        ImGui::Text("Memory Tracking:");
        if (ImGui::Button("Force Memory Update")) {
            // Trigger memory panel update
            auto* memory_panel = static_cast<MemoryObserverPanel*>(
                get_ui_overlay().get_panel("Memory Observer"));
            if (memory_panel) {
                memory_panel->set_visible(true);
            }
        }
        
        // Entity selection
        ImGui::Separator();
        ImGui::Text("Entity Inspector:");
        
        if (!state.demo_entities.empty()) {
            static int selected_entity_index = 0;
            if (ImGui::SliderInt("Select Entity", &selected_entity_index, 
                               0, static_cast<int>(state.demo_entities.size() - 1))) {
                // Update ECS inspector selection
                auto* ecs_panel = static_cast<ECSInspectorPanel*>(
                    get_ui_overlay().get_panel("ECS Inspector"));
                if (ecs_panel) {
                    ecs_panel->select_entity(state.demo_entities[selected_entity_index]);
                }
            }
            
            // Show selected entity info
            Entity selected = state.demo_entities[selected_entity_index];
            ImGui::Text("Selected: Entity %u (Gen %u)", 
                       selected.index, selected.generation);
            
            // Show component info
            if (auto* transform = registry.get_component<Transform>(selected)) {
                ImGui::Text("  Position: (%.1f, %.1f)", 
                           transform->position.x, transform->position.y);
                ImGui::Text("  Rotation: %.2f°", 
                           transform->rotation * 180.0f / 3.14159f);
            }
            
            if (registry.get_component<Vec2>(selected)) {
                ImGui::Text("  Has Velocity: Yes");
            }
        }
        
        // Quick actions
        ImGui::Separator();
        if (ImGui::Button("Clear All Entities")) {
            auto& registry = get_registry();
            for (Entity entity : state.demo_entities) {
                registry.destroy_entity(entity);
            }
            state.demo_entities.clear();
            state.demo_running = false;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Show All Panels")) {
            get_ui_overlay().get_panel("ECS Inspector")->set_visible(true);
            get_ui_overlay().get_panel("Memory Observer")->set_visible(true);
            get_ui_overlay().get_panel("Performance Stats")->set_visible(true);
        }
        
        ImGui::Separator();
        
        // Interactive Learning System controls
        ImGui::Text("🎓 Interactive Learning System:");
        
        if (ImGui::Button("Start ECS Tutorial")) {
            ecscope::learning::learning_integration::quick_start_ecs_tutorial();
        }
        ImGui::SameLine();
        if (ImGui::Button("Performance Analysis")) {
            ecscope::learning::learning_integration::quick_start_performance_analysis();
        }
        
        if (ImGui::Button("Debug Practice")) {
            ecscope::learning::learning_integration::quick_start_debugging_practice();
        }
        ImGui::SameLine();
        if (ImGui::Button("Take Quiz")) {
            ecscope::learning::learning_integration::quick_start_adaptive_quiz("ECS Basics");
        }
        
        if (ImGui::Button("Show Learning Panels")) {
            auto& learning_integration = ecscope::learning::get_learning_integration();
            learning_integration.show_learning_panels(true);
        }
        ImGui::SameLine();
        if (ImGui::Button("Hide Learning Panels")) {
            auto& learning_integration = ecscope::learning::get_learning_integration();
            learning_integration.hide_learning_panels();
        }
        
    }
    ImGui::End();
#endif
}