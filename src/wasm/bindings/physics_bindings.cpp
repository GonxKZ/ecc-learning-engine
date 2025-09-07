// =============================================================================
// ECScope Physics JavaScript Bindings using Embind
// =============================================================================

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <memory>
#include <vector>
#include <string>

// ECScope physics includes
#include "ecscope/physics.hpp"
#include "ecscope/math.hpp"
#include "ecscope/world.hpp"
#include "ecscope/collision.hpp"

using namespace emscripten;
using namespace ecscope;

// =============================================================================
// WEBASSEMBLY PHYSICS WORLD WRAPPER
// =============================================================================

class WasmPhysicsWorld {
private:
    std::unique_ptr<physics::World> world_;
    std::vector<uint32_t> body_ids_;

public:
    WasmPhysicsWorld() : world_(std::make_unique<physics::World>()) {
        // Configure world for web performance
        world_->setGravity({0.0f, -9.81f});
        world_->setTimeStep(1.0f / 60.0f);  // 60 FPS target
    }

    // World configuration
    void setGravity(float x, float y) {
        world_->setGravity({x, y});
    }

    val getGravity() {
        auto gravity = world_->getGravity();
        val result = val::object();
        result.set("x", gravity.x);
        result.set("y", gravity.y);
        return result;
    }

    void setTimeStep(float time_step) {
        world_->setTimeStep(time_step);
    }

    float getTimeStep() const {
        return world_->getTimeStep();
    }

    // Body management
    uint32_t createStaticBody(float x, float y, float width, float height) {
        physics::BodyDef body_def;
        body_def.type = physics::BodyType::Static;
        body_def.position = {x, y};
        
        auto body = world_->createBody(body_def);
        
        physics::BoxShape box;
        box.width = width;
        box.height = height;
        
        physics::FixtureDef fixture_def;
        fixture_def.shape = &box;
        fixture_def.density = 1.0f;
        fixture_def.friction = 0.3f;
        fixture_def.restitution = 0.5f;
        
        body->createFixture(fixture_def);
        
        uint32_t id = body->getId();
        body_ids_.push_back(id);
        return id;
    }

    uint32_t createDynamicBody(float x, float y, float width, float height, float density = 1.0f) {
        physics::BodyDef body_def;
        body_def.type = physics::BodyType::Dynamic;
        body_def.position = {x, y};
        
        auto body = world_->createBody(body_def);
        
        physics::BoxShape box;
        box.width = width;
        box.height = height;
        
        physics::FixtureDef fixture_def;
        fixture_def.shape = &box;
        fixture_def.density = density;
        fixture_def.friction = 0.3f;
        fixture_def.restitution = 0.3f;
        
        body->createFixture(fixture_def);
        
        uint32_t id = body->getId();
        body_ids_.push_back(id);
        return id;
    }

    uint32_t createCircleBody(float x, float y, float radius, bool is_static = false, float density = 1.0f) {
        physics::BodyDef body_def;
        body_def.type = is_static ? physics::BodyType::Static : physics::BodyType::Dynamic;
        body_def.position = {x, y};
        
        auto body = world_->createBody(body_def);
        
        physics::CircleShape circle;
        circle.radius = radius;
        
        physics::FixtureDef fixture_def;
        fixture_def.shape = &circle;
        fixture_def.density = density;
        fixture_def.friction = 0.3f;
        fixture_def.restitution = 0.6f;
        
        body->createFixture(fixture_def);
        
        uint32_t id = body->getId();
        body_ids_.push_back(id);
        return id;
    }

    void destroyBody(uint32_t body_id) {
        auto body = world_->getBody(body_id);
        if (body) {
            world_->destroyBody(body);
            body_ids_.erase(std::remove(body_ids_.begin(), body_ids_.end(), body_id), body_ids_.end());
        }
    }

    // Body properties
    val getBodyPosition(uint32_t body_id) {
        auto body = world_->getBody(body_id);
        if (body) {
            auto pos = body->getPosition();
            val result = val::object();
            result.set("x", pos.x);
            result.set("y", pos.y);
            return result;
        }
        return val::null();
    }

    void setBodyPosition(uint32_t body_id, float x, float y) {
        auto body = world_->getBody(body_id);
        if (body) {
            body->setPosition({x, y});
        }
    }

    val getBodyVelocity(uint32_t body_id) {
        auto body = world_->getBody(body_id);
        if (body) {
            auto vel = body->getLinearVelocity();
            val result = val::object();
            result.set("x", vel.x);
            result.set("y", vel.y);
            return result;
        }
        return val::null();
    }

    void setBodyVelocity(uint32_t body_id, float x, float y) {
        auto body = world_->getBody(body_id);
        if (body) {
            body->setLinearVelocity({x, y});
        }
    }

    float getBodyAngle(uint32_t body_id) {
        auto body = world_->getBody(body_id);
        return body ? body->getAngle() : 0.0f;
    }

    void setBodyAngle(uint32_t body_id, float angle) {
        auto body = world_->getBody(body_id);
        if (body) {
            body->setAngle(angle);
        }
    }

    float getBodyAngularVelocity(uint32_t body_id) {
        auto body = world_->getBody(body_id);
        return body ? body->getAngularVelocity() : 0.0f;
    }

    void setBodyAngularVelocity(uint32_t body_id, float velocity) {
        auto body = world_->getBody(body_id);
        if (body) {
            body->setAngularVelocity(velocity);
        }
    }

    // Forces and impulses
    void applyForce(uint32_t body_id, float force_x, float force_y, float point_x, float point_y) {
        auto body = world_->getBody(body_id);
        if (body) {
            body->applyForce({force_x, force_y}, {point_x, point_y});
        }
    }

    void applyImpulse(uint32_t body_id, float impulse_x, float impulse_y, float point_x, float point_y) {
        auto body = world_->getBody(body_id);
        if (body) {
            body->applyLinearImpulse({impulse_x, impulse_y}, {point_x, point_y});
        }
    }

    void applyTorque(uint32_t body_id, float torque) {
        auto body = world_->getBody(body_id);
        if (body) {
            body->applyTorque(torque);
        }
    }

    // Simulation
    void step(float time_step) {
        world_->step(time_step, 8, 3);  // velocity iterations, position iterations
    }

    void step() {
        world_->step();  // Use default time step
    }

    // Queries and information
    val getAllBodies() {
        val bodies = val::array();
        int index = 0;
        
        for (uint32_t id : body_ids_) {
            auto body = world_->getBody(id);
            if (body) {
                val body_info = val::object();
                body_info.set("id", id);
                body_info.set("position", getBodyPosition(id));
                body_info.set("velocity", getBodyVelocity(id));
                body_info.set("angle", getBodyAngle(id));
                body_info.set("angularVelocity", getBodyAngularVelocity(id));
                body_info.set("type", static_cast<int>(body->getType()));
                bodies.set(index++, body_info);
            }
        }
        
        return bodies;
    }

    size_t getBodyCount() const {
        return body_ids_.size();
    }

    // Performance and statistics
    val getWorldStatistics() {
        val stats = val::object();
        stats.set("bodyCount", getBodyCount());
        stats.set("contactCount", world_->getContactCount());
        stats.set("jointCount", world_->getJointCount());
        
        // Performance metrics
        auto profile = world_->getProfile();
        val performance = val::object();
        performance.set("stepTime", profile.step);
        performance.set("collideTime", profile.collide);
        performance.set("solveTime", profile.solve);
        performance.set("broadphaseTime", profile.broadphase);
        stats.set("performance", performance);
        
        return stats;
    }

    // Collision detection
    val queryAABB(float min_x, float min_y, float max_x, float max_y) {
        val bodies = val::array();
        int index = 0;
        
        physics::AABB aabb;
        aabb.lower_bound = {min_x, min_y};
        aabb.upper_bound = {max_x, max_y};
        
        world_->queryAABB([&](physics::Body* body) -> bool {
            val body_info = val::object();
            body_info.set("id", body->getId());
            body_info.set("position", getBodyPosition(body->getId()));
            bodies.set(index++, body_info);
            return true;  // Continue query
        }, aabb);
        
        return bodies;
    }

    // Demo scenarios
    void createBoxStack(int count, float x, float y, float box_size) {
        for (int i = 0; i < count; ++i) {
            createDynamicBody(x, y + i * (box_size + 0.1f), box_size, box_size, 1.0f);
        }
    }

    void createPyramid(int base_count, float x, float y, float box_size) {
        for (int row = 0; row < base_count; ++row) {
            int boxes_in_row = base_count - row;
            float row_width = boxes_in_row * box_size;
            float start_x = x - row_width * 0.5f + box_size * 0.5f;
            
            for (int col = 0; col < boxes_in_row; ++col) {
                createDynamicBody(
                    start_x + col * box_size,
                    y + row * (box_size + 0.05f),
                    box_size,
                    box_size,
                    1.0f
                );
            }
        }
    }

    void explode(float center_x, float center_y, float force, float radius) {
        for (uint32_t id : body_ids_) {
            auto body = world_->getBody(id);
            if (body && body->getType() == physics::BodyType::Dynamic) {
                auto pos = body->getPosition();
                float dx = pos.x - center_x;
                float dy = pos.y - center_y;
                float distance = std::sqrt(dx * dx + dy * dy);
                
                if (distance < radius && distance > 0.1f) {
                    float force_magnitude = force / (distance * distance);
                    float force_x = (dx / distance) * force_magnitude;
                    float force_y = (dy / distance) * force_magnitude;
                    
                    applyImpulse(id, force_x, force_y, pos.x, pos.y);
                }
            }
        }
    }

    void reset() {
        // Destroy all bodies
        for (uint32_t id : body_ids_) {
            auto body = world_->getBody(id);
            if (body) {
                world_->destroyBody(body);
            }
        }
        body_ids_.clear();
    }
};

// =============================================================================
// WEBASSEMBLY PHYSICS PERFORMANCE MONITOR
// =============================================================================

class WasmPhysicsPerformanceMonitor {
private:
    std::chrono::high_resolution_clock::time_point step_start_;
    std::vector<double> step_times_;
    size_t max_samples_ = 300;

public:
    void beginStep() {
        step_start_ = std::chrono::high_resolution_clock::now();
    }

    void endStep() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - step_start_).count();
        
        step_times_.push_back(duration);
        if (step_times_.size() > max_samples_) {
            step_times_.erase(step_times_.begin());
        }
    }

    val getStatistics() {
        val stats = val::object();
        
        if (!step_times_.empty()) {
            double sum = 0.0;
            double min_time = step_times_[0];
            double max_time = step_times_[0];
            
            for (double time : step_times_) {
                sum += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }
            
            stats.set("averageStepTime", sum / step_times_.size());
            stats.set("minStepTime", min_time);
            stats.set("maxStepTime", max_time);
            stats.set("totalSamples", step_times_.size());
            
            // Calculate target performance
            double target_time = 1000.0 / 60.0;  // 60 FPS target
            stats.set("targetStepTime", target_time);
            stats.set("performanceRatio", target_time / (sum / step_times_.size()));
        } else {
            stats.set("averageStepTime", 0.0);
            stats.set("minStepTime", 0.0);
            stats.set("maxStepTime", 0.0);
            stats.set("totalSamples", 0);
        }
        
        return stats;
    }

    void reset() {
        step_times_.clear();
    }
};

// =============================================================================
// EMBIND BINDINGS
// =============================================================================

EMSCRIPTEN_BINDINGS(ecscope_physics) {
    // Physics world bindings
    class_<WasmPhysicsWorld>("PhysicsWorld")
        .constructor<>()
        .function("setGravity", &WasmPhysicsWorld::setGravity)
        .function("getGravity", &WasmPhysicsWorld::getGravity)
        .function("setTimeStep", &WasmPhysicsWorld::setTimeStep)
        .function("getTimeStep", &WasmPhysicsWorld::getTimeStep)
        .function("createStaticBody", &WasmPhysicsWorld::createStaticBody)
        .function("createDynamicBody", &WasmPhysicsWorld::createDynamicBody)
        .function("createCircleBody", &WasmPhysicsWorld::createCircleBody)
        .function("destroyBody", &WasmPhysicsWorld::destroyBody)
        .function("getBodyPosition", &WasmPhysicsWorld::getBodyPosition)
        .function("setBodyPosition", &WasmPhysicsWorld::setBodyPosition)
        .function("getBodyVelocity", &WasmPhysicsWorld::getBodyVelocity)
        .function("setBodyVelocity", &WasmPhysicsWorld::setBodyVelocity)
        .function("getBodyAngle", &WasmPhysicsWorld::getBodyAngle)
        .function("setBodyAngle", &WasmPhysicsWorld::setBodyAngle)
        .function("getBodyAngularVelocity", &WasmPhysicsWorld::getBodyAngularVelocity)
        .function("setBodyAngularVelocity", &WasmPhysicsWorld::setBodyAngularVelocity)
        .function("applyForce", &WasmPhysicsWorld::applyForce)
        .function("applyImpulse", &WasmPhysicsWorld::applyImpulse)
        .function("applyTorque", &WasmPhysicsWorld::applyTorque)
        .function("step", select_overload<void()>(&WasmPhysicsWorld::step))
        .function("stepWithTime", select_overload<void(float)>(&WasmPhysicsWorld::step))
        .function("getAllBodies", &WasmPhysicsWorld::getAllBodies)
        .function("getBodyCount", &WasmPhysicsWorld::getBodyCount)
        .function("getWorldStatistics", &WasmPhysicsWorld::getWorldStatistics)
        .function("queryAABB", &WasmPhysicsWorld::queryAABB)
        .function("createBoxStack", &WasmPhysicsWorld::createBoxStack)
        .function("createPyramid", &WasmPhysicsWorld::createPyramid)
        .function("explode", &WasmPhysicsWorld::explode)
        .function("reset", &WasmPhysicsWorld::reset);

    // Physics performance monitor bindings
    class_<WasmPhysicsPerformanceMonitor>("PhysicsPerformanceMonitor")
        .constructor<>()
        .function("beginStep", &WasmPhysicsPerformanceMonitor::beginStep)
        .function("endStep", &WasmPhysicsPerformanceMonitor::endStep)
        .function("getStatistics", &WasmPhysicsPerformanceMonitor::getStatistics)
        .function("reset", &WasmPhysicsPerformanceMonitor::reset);
}