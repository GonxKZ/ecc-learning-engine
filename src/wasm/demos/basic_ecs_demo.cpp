// =============================================================================
// ECScope Basic ECS Demo - WebAssembly
// =============================================================================

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <iostream>
#include <memory>
#include <vector>
#include <random>

// ECScope includes
#include "../wasm_core.hpp"
#include "ecscope/registry.hpp"
#include "ecscope/entity.hpp"
#include "ecscope/component.hpp"

using namespace emscripten;
using namespace ecscope;

// =============================================================================
// DEMO COMPONENTS
// =============================================================================

struct Position {
    float x, y, z;
    Position(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

struct Velocity {
    float x, y, z;
    Velocity(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

struct Color {
    float r, g, b, a;
    Color(float r = 1, float g = 1, float b = 1, float a = 1) : r(r), g(g), b(b), a(a) {}
};

struct Lifetime {
    float remaining;
    float total;
    Lifetime(float time = 5.0f) : remaining(time), total(time) {}
};

struct Particle {
    float size;
    Particle(float s = 1.0f) : size(s) {}
};

// =============================================================================
// BASIC ECS DEMO CLASS
// =============================================================================

class BasicECSDemo {
private:
    std::unique_ptr<Registry> registry_;
    std::vector<Entity> entities_;
    std::mt19937 random_gen_;
    std::uniform_real_distribution<float> pos_dist_;
    std::uniform_real_distribution<float> vel_dist_;
    std::uniform_real_distribution<float> color_dist_;
    
    bool is_running_ = false;
    float canvas_width_ = 800.0f;
    float canvas_height_ = 600.0f;
    
    // Demo statistics
    int particles_created_ = 0;
    int particles_destroyed_ = 0;
    float total_time_ = 0.0f;

public:
    BasicECSDemo() 
        : registry_(std::make_unique<Registry>())
        , random_gen_(std::random_device{}())
        , pos_dist_(0.0f, 1.0f)
        , vel_dist_(-100.0f, 100.0f)
        , color_dist_(0.3f, 1.0f) {
        
        EM_ASM({
            console.log('BasicECSDemo initialized');
        });
    }
    
    void setCanvasSize(float width, float height) {
        canvas_width_ = width;
        canvas_height_ = height;
    }
    
    void start() {
        is_running_ = true;
        total_time_ = 0.0f;
        
        // Create initial particles
        createParticleBurst(50, canvas_width_ / 2, canvas_height_ / 2);
        
        EM_ASM({
            console.log('BasicECSDemo started');
        });
    }
    
    void stop() {
        is_running_ = false;
        
        EM_ASM({
            console.log('BasicECSDemo stopped');
        });
    }
    
    void reset() {
        // Remove all entities
        for (auto entity : entities_) {
            if (registry_->valid(entity)) {
                registry_->destroy(entity);
            }
        }
        entities_.clear();
        
        particles_created_ = 0;
        particles_destroyed_ = 0;
        total_time_ = 0.0f;
        
        EM_ASM({
            console.log('BasicECSDemo reset');
        });
    }
    
    void update(float delta_time) {
        if (!is_running_) return;
        
        total_time_ += delta_time;
        
        // Update movement system
        updateMovementSystem(delta_time);
        
        // Update lifetime system
        updateLifetimeSystem(delta_time);
        
        // Spawn new particles occasionally
        if (static_cast<int>(total_time_ * 2) % 3 == 0 && entities_.size() < 200) {
            if (static_cast<int>(total_time_ * 10) % 10 == 0) {
                createRandomParticle();
            }
        }
        
        // Create bursts periodically
        if (static_cast<int>(total_time_) % 5 == 0 && static_cast<int>(total_time_ * 10) % 10 == 0) {
            createParticleBurst(20, 
                pos_dist_(random_gen_) * canvas_width_,
                pos_dist_(random_gen_) * canvas_height_);
        }
    }
    
    void createRandomParticle() {
        Entity entity = registry_->create();
        
        // Position
        float x = pos_dist_(random_gen_) * canvas_width_;
        float y = pos_dist_(random_gen_) * canvas_height_;
        registry_->emplace<Position>(entity, x, y, 0.0f);
        
        // Velocity
        float vx = vel_dist_(random_gen_);
        float vy = vel_dist_(random_gen_);
        registry_->emplace<Velocity>(entity, vx, vy, 0.0f);
        
        // Color
        float r = color_dist_(random_gen_);
        float g = color_dist_(random_gen_);
        float b = color_dist_(random_gen_);
        registry_->emplace<Color>(entity, r, g, b, 1.0f);
        
        // Lifetime
        float lifetime = 2.0f + pos_dist_(random_gen_) * 8.0f;
        registry_->emplace<Lifetime>(entity, lifetime);
        
        // Particle properties
        float size = 2.0f + pos_dist_(random_gen_) * 8.0f;
        registry_->emplace<Particle>(entity, size);
        
        entities_.push_back(entity);
        particles_created_++;
    }
    
    void createParticleBurst(int count, float center_x, float center_y) {
        for (int i = 0; i < count; ++i) {
            Entity entity = registry_->create();
            
            // Position around center
            float angle = pos_dist_(random_gen_) * 2 * 3.14159f;
            float radius = pos_dist_(random_gen_) * 50.0f;
            float x = center_x + cos(angle) * radius;
            float y = center_y + sin(angle) * radius;
            registry_->emplace<Position>(entity, x, y, 0.0f);
            
            // Velocity radiating outward
            float speed = 50.0f + pos_dist_(random_gen_) * 100.0f;
            float vx = cos(angle) * speed;
            float vy = sin(angle) * speed;
            registry_->emplace<Velocity>(entity, vx, vy, 0.0f);
            
            // Color based on position
            float r = 0.5f + pos_dist_(random_gen_) * 0.5f;
            float g = 0.3f + pos_dist_(random_gen_) * 0.4f;
            float b = 0.8f + pos_dist_(random_gen_) * 0.2f;
            registry_->emplace<Color>(entity, r, g, b, 1.0f);
            
            // Random lifetime
            float lifetime = 3.0f + pos_dist_(random_gen_) * 5.0f;
            registry_->emplace<Lifetime>(entity, lifetime);
            
            // Particle size
            float size = 1.0f + pos_dist_(random_gen_) * 6.0f;
            registry_->emplace<Particle>(entity, size);
            
            entities_.push_back(entity);
            particles_created_++;
        }
    }
    
    void createExplosion(float x, float y) {
        createParticleBurst(30, x, y);
    }
    
private:
    void updateMovementSystem(float delta_time) {
        auto view = registry_->view<Position, Velocity>();
        
        for (auto entity : view) {
            auto& pos = view.get<Position>(entity);
            const auto& vel = view.get<Velocity>(entity);
            
            pos.x += vel.x * delta_time;
            pos.y += vel.y * delta_time;
            
            // Bounce off edges
            if (pos.x < 0 || pos.x > canvas_width_) {
                auto& velocity = view.get<Velocity>(entity);
                velocity.x = -velocity.x;
                pos.x = std::clamp(pos.x, 0.0f, canvas_width_);
            }
            
            if (pos.y < 0 || pos.y > canvas_height_) {
                auto& velocity = view.get<Velocity>(entity);
                velocity.y = -velocity.y;
                pos.y = std::clamp(pos.y, 0.0f, canvas_height_);
            }
        }
    }
    
    void updateLifetimeSystem(float delta_time) {
        std::vector<Entity> to_remove;
        
        auto view = registry_->view<Lifetime, Color>();
        
        for (auto entity : view) {
            auto& lifetime = view.get<Lifetime>(entity);
            auto& color = view.get<Color>(entity);
            
            lifetime.remaining -= delta_time;
            
            // Fade out based on remaining lifetime
            color.a = lifetime.remaining / lifetime.total;
            
            if (lifetime.remaining <= 0.0f) {
                to_remove.push_back(entity);
            }
        }
        
        // Remove expired entities
        for (auto entity : to_remove) {
            registry_->destroy(entity);
            
            // Remove from entities list
            entities_.erase(std::remove(entities_.begin(), entities_.end(), entity), entities_.end());
            particles_destroyed_++;
        }
    }

public:
    // JavaScript API
    val getParticleData() {
        val particles = val::array();
        
        auto view = registry_->view<Position, Color, Particle>();
        int index = 0;
        
        for (auto entity : view) {
            const auto& pos = view.get<Position>(entity);
            const auto& color = view.get<Color>(entity);
            const auto& particle = view.get<Particle>(entity);
            
            val p = val::object();
            p.set("x", pos.x);
            p.set("y", pos.y);
            p.set("size", particle.size);
            p.set("r", color.r);
            p.set("g", color.g);
            p.set("b", color.b);
            p.set("a", color.a);
            
            particles.set(index++, p);
        }
        
        return particles;
    }
    
    val getStatistics() {
        val stats = val::object();
        stats.set("activeParticles", entities_.size());
        stats.set("totalCreated", particles_created_);
        stats.set("totalDestroyed", particles_destroyed_);
        stats.set("entityCount", registry_->size());
        stats.set("archetypeCount", registry_->archetype_count());
        stats.set("memoryUsage", registry_->memory_usage());
        stats.set("totalTime", total_time_);
        stats.set("isRunning", is_running_);
        return stats;
    }
    
    size_t getEntityCount() const {
        return registry_->size();
    }
    
    size_t getActiveParticleCount() const {
        return entities_.size();
    }
    
    bool isRunning() const {
        return is_running_;
    }
};

// =============================================================================
// EMBIND BINDINGS
// =============================================================================

EMSCRIPTEN_BINDINGS(basic_ecs_demo) {
    class_<BasicECSDemo>("BasicECSDemo")
        .constructor<>()
        .function("setCanvasSize", &BasicECSDemo::setCanvasSize)
        .function("start", &BasicECSDemo::start)
        .function("stop", &BasicECSDemo::stop)
        .function("reset", &BasicECSDemo::reset)
        .function("update", &BasicECSDemo::update)
        .function("createRandomParticle", &BasicECSDemo::createRandomParticle)
        .function("createParticleBurst", &BasicECSDemo::createParticleBurst)
        .function("createExplosion", &BasicECSDemo::createExplosion)
        .function("getParticleData", &BasicECSDemo::getParticleData)
        .function("getStatistics", &BasicECSDemo::getStatistics)
        .function("getEntityCount", &BasicECSDemo::getEntityCount)
        .function("getActiveParticleCount", &BasicECSDemo::getActiveParticleCount)
        .function("isRunning", &BasicECSDemo::isRunning);
}

// =============================================================================
// MAIN FUNCTION FOR DEMO
// =============================================================================

int main() {
    EM_ASM({
        console.log('Basic ECS Demo WebAssembly module loaded');
        
        // Notify JavaScript that the demo is ready
        if (window.ECScope && window.ECScope.onBasicECSDemoReady) {
            window.ECScope.onBasicECSDemoReady();
        }
    });
    
    return 0;
}