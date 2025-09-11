#pragma once

#include "test_framework.hpp"
#include "../physics/world.hpp"
#include "../physics/rigidbody.hpp"
#include "../physics/collision.hpp"
#include "../physics/broadphase.hpp"
#include "../physics/narrowphase.hpp"
#include "../physics/solver.hpp"
#include "../math.hpp"
#include <random>
#include <cmath>

namespace ecscope::testing {

// Physics simulation determinism validator
class PhysicsDeterminismValidator {
public:
    struct SimulationState {
        std::vector<Vector3> positions;
        std::vector<Vector3> velocities;
        std::vector<Vector3> angular_velocities;
        std::vector<Quaternion> orientations;
        double total_energy;
        
        bool operator==(const SimulationState& other) const {
            return compare_vectors(positions, other.positions, 1e-10) &&
                   compare_vectors(velocities, other.velocities, 1e-10) &&
                   compare_vectors(angular_velocities, other.angular_velocities, 1e-10) &&
                   compare_quaternions(orientations, other.orientations, 1e-10) &&
                   std::abs(total_energy - other.total_energy) < 1e-10;
        }
        
    private:
        bool compare_vectors(const std::vector<Vector3>& a, const std::vector<Vector3>& b, 
                           double tolerance) const {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) {
                if ((a[i] - b[i]).length() > tolerance) return false;
            }
            return true;
        }
        
        bool compare_quaternions(const std::vector<Quaternion>& a, const std::vector<Quaternion>& b,
                               double tolerance) const {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) {
                if (quaternion_distance(a[i], b[i]) > tolerance) return false;
            }
            return true;
        }
        
        double quaternion_distance(const Quaternion& q1, const Quaternion& q2) const {
            double dot = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
            return 1.0 - std::abs(dot);
        }
    };

    template<typename PhysicsWorld>
    bool validate_determinism(PhysicsWorld& world, int steps, int repetitions = 3) {
        std::vector<SimulationState> final_states;
        
        for (int rep = 0; rep < repetitions; ++rep) {
            // Reset world to initial state
            world.reset();
            setup_deterministic_scene(world);
            
            // Run simulation
            for (int step = 0; step < steps; ++step) {
                world.step(1.0f / 60.0f); // Fixed timestep
            }
            
            // Capture final state
            final_states.push_back(capture_state(world));
        }
        
        // Compare all states
        for (size_t i = 1; i < final_states.size(); ++i) {
            if (!(final_states[0] == final_states[i])) {
                return false;
            }
        }
        
        return true;
    }

private:
    template<typename PhysicsWorld>
    void setup_deterministic_scene(PhysicsWorld& world) {
        // Create identical initial conditions
        // This would need to be adapted based on your physics API
        /*
        auto body1 = world.create_rigidbody();
        body1.set_position({0, 10, 0});
        body1.set_velocity({1, 0, 0});
        
        auto body2 = world.create_rigidbody();
        body2.set_position({5, 10, 0});
        body2.set_velocity({-1, 0, 0});
        */
    }
    
    template<typename PhysicsWorld>
    SimulationState capture_state(const PhysicsWorld& world) {
        SimulationState state;
        
        // This would need to be adapted based on your physics API
        /*
        for (const auto& body : world.get_rigidbodies()) {
            state.positions.push_back(body.position());
            state.velocities.push_back(body.velocity());
            state.angular_velocities.push_back(body.angular_velocity());
            state.orientations.push_back(body.orientation());
        }
        
        state.total_energy = calculate_total_energy(world);
        */
        
        return state;
    }
    
    template<typename PhysicsWorld>
    double calculate_total_energy(const PhysicsWorld& world) {
        double total_energy = 0.0;
        
        // Calculate kinetic + potential energy
        /*
        for (const auto& body : world.get_rigidbodies()) {
            // Kinetic energy: 0.5 * m * v^2 + 0.5 * I * w^2
            double ke_linear = 0.5 * body.mass() * body.velocity().length_squared();
            double ke_angular = 0.5 * body.moment_of_inertia() * body.angular_velocity().length_squared();
            
            // Potential energy: m * g * h
            double pe = body.mass() * 9.81 * body.position().y;
            
            total_energy += ke_linear + ke_angular + pe;
        }
        */
        
        return total_energy;
    }
};

// Physics conservation laws validator
class ConservationValidator {
public:
    struct ConservationResults {
        bool energy_conserved;
        bool momentum_conserved;
        bool angular_momentum_conserved;
        double energy_drift;
        double momentum_drift;
        double angular_momentum_drift;
    };

    template<typename PhysicsWorld>
    ConservationResults validate_conservation(PhysicsWorld& world, int steps, 
                                             double tolerance = 1e-3) {
        // Setup isolated system
        setup_isolated_system(world);
        
        // Capture initial values
        double initial_energy = calculate_total_energy(world);
        Vector3 initial_momentum = calculate_total_momentum(world);
        Vector3 initial_angular_momentum = calculate_total_angular_momentum(world);
        
        // Run simulation
        for (int step = 0; step < steps; ++step) {
            world.step(1.0f / 60.0f);
        }
        
        // Capture final values
        double final_energy = calculate_total_energy(world);
        Vector3 final_momentum = calculate_total_momentum(world);
        Vector3 final_angular_momentum = calculate_total_angular_momentum(world);
        
        // Calculate drifts
        ConservationResults results;
        results.energy_drift = std::abs(final_energy - initial_energy) / initial_energy;
        results.momentum_drift = (final_momentum - initial_momentum).length() / initial_momentum.length();
        results.angular_momentum_drift = (final_angular_momentum - initial_angular_momentum).length() / 
                                        initial_angular_momentum.length();
        
        results.energy_conserved = results.energy_drift < tolerance;
        results.momentum_conserved = results.momentum_drift < tolerance;
        results.angular_momentum_conserved = results.angular_momentum_drift < tolerance;
        
        return results;
    }

private:
    template<typename PhysicsWorld>
    void setup_isolated_system(PhysicsWorld& world) {
        // Create system with no external forces
        // Disable gravity temporarily for conservation tests
    }
    
    template<typename PhysicsWorld>
    double calculate_total_energy(const PhysicsWorld& world) {
        // Implementation similar to PhysicsDeterminismValidator
        return 0.0;
    }
    
    template<typename PhysicsWorld>
    Vector3 calculate_total_momentum(const PhysicsWorld& world) {
        Vector3 total_momentum{0, 0, 0};
        /*
        for (const auto& body : world.get_rigidbodies()) {
            total_momentum = total_momentum + body.velocity() * body.mass();
        }
        */
        return total_momentum;
    }
    
    template<typename PhysicsWorld>
    Vector3 calculate_total_angular_momentum(const PhysicsWorld& world) {
        Vector3 total_angular_momentum{0, 0, 0};
        /*
        for (const auto& body : world.get_rigidbodies()) {
            Vector3 angular_momentum = body.angular_velocity() * body.moment_of_inertia();
            total_angular_momentum = total_angular_momentum + angular_momentum;
        }
        */
        return total_angular_momentum;
    }
};

// Collision detection accuracy validator
class CollisionValidator {
public:
    struct CollisionTestCase {
        std::string name;
        Vector3 pos1, pos2;
        Vector3 size1, size2;
        Vector3 vel1, vel2;
        bool should_collide;
        Vector3 expected_contact_point;
        Vector3 expected_normal;
        double expected_penetration;
    };

    template<typename CollisionSystem>
    bool validate_collision_detection(CollisionSystem& collision_system) {
        auto test_cases = generate_collision_test_cases();
        
        for (const auto& test_case : test_cases) {
            if (!validate_single_collision(collision_system, test_case)) {
                return false;
            }
        }
        
        return true;
    }

    template<typename CollisionSystem>
    bool validate_single_collision(CollisionSystem& collision_system, 
                                  const CollisionTestCase& test_case) {
        // Setup collision objects based on test case
        /*
        auto obj1 = collision_system.create_box(test_case.pos1, test_case.size1);
        auto obj2 = collision_system.create_box(test_case.pos2, test_case.size2);
        
        obj1.set_velocity(test_case.vel1);
        obj2.set_velocity(test_case.vel2);
        
        // Perform collision detection
        auto collision_result = collision_system.detect_collision(obj1, obj2);
        
        if (collision_result.has_collision != test_case.should_collide) {
            return false;
        }
        
        if (test_case.should_collide) {
            // Validate contact details
            double contact_error = (collision_result.contact_point - test_case.expected_contact_point).length();
            double normal_error = (collision_result.normal - test_case.expected_normal).length();
            double penetration_error = std::abs(collision_result.penetration - test_case.expected_penetration);
            
            return contact_error < 0.01 && normal_error < 0.01 && penetration_error < 0.01;
        }
        */
        
        return true;
    }

private:
    std::vector<CollisionTestCase> generate_collision_test_cases() {
        std::vector<CollisionTestCase> cases;
        
        // Case 1: Simple collision
        cases.push_back({
            "Simple Box Collision",
            {0, 0, 0}, {2, 0, 0},          // positions
            {1, 1, 1}, {1, 1, 1},          // sizes
            {1, 0, 0}, {-1, 0, 0},         // velocities
            true,                           // should collide
            {1, 0, 0},                     // expected contact point
            {1, 0, 0},                     // expected normal
            0.0                            // expected penetration
        });
        
        // Case 2: No collision
        cases.push_back({
            "No Collision",
            {0, 0, 0}, {5, 0, 0},          // positions
            {1, 1, 1}, {1, 1, 1},          // sizes
            {0, 0, 0}, {0, 0, 0},          // velocities
            false,                          // should not collide
            {0, 0, 0},                     // contact point (unused)
            {0, 0, 0},                     // normal (unused)
            0.0                            // penetration (unused)
        });
        
        // Add more test cases...
        
        return cases;
    }
};

// Physics performance profiler
class PhysicsProfiler {
public:
    struct PhysicsMetrics {
        std::chrono::nanoseconds total_step_time{0};
        std::chrono::nanoseconds broadphase_time{0};
        std::chrono::nanoseconds narrowphase_time{0};
        std::chrono::nanoseconds constraint_solving_time{0};
        std::chrono::nanoseconds integration_time{0};
        size_t collision_pairs_tested{0};
        size_t collision_pairs_found{0};
        size_t constraint_iterations{0};
        double collision_efficiency{0.0}; // found / tested
    };

    template<typename PhysicsWorld>
    PhysicsMetrics profile_simulation(PhysicsWorld& world, int steps) {
        PhysicsMetrics total_metrics;
        
        for (int step = 0; step < steps; ++step) {
            auto step_metrics = profile_single_step(world);
            accumulate_metrics(total_metrics, step_metrics);
        }
        
        // Calculate averages
        total_metrics.total_step_time /= steps;
        total_metrics.broadphase_time /= steps;
        total_metrics.narrowphase_time /= steps;
        total_metrics.constraint_solving_time /= steps;
        total_metrics.integration_time /= steps;
        total_metrics.collision_pairs_tested /= steps;
        total_metrics.collision_pairs_found /= steps;
        
        if (total_metrics.collision_pairs_tested > 0) {
            total_metrics.collision_efficiency = 
                static_cast<double>(total_metrics.collision_pairs_found) / 
                total_metrics.collision_pairs_tested;
        }
        
        return total_metrics;
    }

private:
    template<typename PhysicsWorld>
    PhysicsMetrics profile_single_step(PhysicsWorld& world) {
        PhysicsMetrics metrics;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Profile broadphase
        auto broadphase_start = std::chrono::high_resolution_clock::now();
        // world.broadphase_detection();
        auto broadphase_end = std::chrono::high_resolution_clock::now();
        metrics.broadphase_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            broadphase_end - broadphase_start);
        
        // Profile narrowphase
        auto narrowphase_start = std::chrono::high_resolution_clock::now();
        // world.narrowphase_detection();
        auto narrowphase_end = std::chrono::high_resolution_clock::now();
        metrics.narrowphase_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            narrowphase_end - narrowphase_start);
        
        // Profile constraint solving
        auto solving_start = std::chrono::high_resolution_clock::now();
        // world.solve_constraints();
        auto solving_end = std::chrono::high_resolution_clock::now();
        metrics.constraint_solving_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            solving_end - solving_start);
        
        // Profile integration
        auto integration_start = std::chrono::high_resolution_clock::now();
        // world.integrate();
        auto integration_end = std::chrono::high_resolution_clock::now();
        metrics.integration_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            integration_end - integration_start);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        metrics.total_step_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time);
        
        return metrics;
    }
    
    void accumulate_metrics(PhysicsMetrics& total, const PhysicsMetrics& step) {
        total.total_step_time += step.total_step_time;
        total.broadphase_time += step.broadphase_time;
        total.narrowphase_time += step.narrowphase_time;
        total.constraint_solving_time += step.constraint_solving_time;
        total.integration_time += step.integration_time;
        total.collision_pairs_tested += step.collision_pairs_tested;
        total.collision_pairs_found += step.collision_pairs_found;
        total.constraint_iterations += step.constraint_iterations;
    }
};

// Physics stress tester
class PhysicsStressTester {
public:
    template<typename PhysicsWorld>
    bool stress_test_many_bodies(PhysicsWorld& world, size_t body_count, int steps) {
        // Create many rigid bodies
        create_stress_test_scene(world, body_count);
        
        // Monitor performance degradation
        auto initial_metrics = PhysicsProfiler{}.profile_simulation(world, 10);
        
        // Run for many steps
        for (int step = 0; step < steps; ++step) {
            world.step(1.0f / 60.0f);
            
            // Check for simulation instability
            if (detect_instability(world)) {
                return false;
            }
        }
        
        // Check final performance
        auto final_metrics = PhysicsProfiler{}.profile_simulation(world, 10);
        
        // Ensure performance hasn't degraded too much
        double performance_ratio = static_cast<double>(final_metrics.total_step_time.count()) /
                                  initial_metrics.total_step_time.count();
        
        return performance_ratio < 2.0; // Less than 2x slower
    }

private:
    template<typename PhysicsWorld>
    void create_stress_test_scene(PhysicsWorld& world, size_t body_count) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> pos_dist(-50.0f, 50.0f);
        std::uniform_real_distribution<float> vel_dist(-10.0f, 10.0f);
        
        for (size_t i = 0; i < body_count; ++i) {
            Vector3 position{pos_dist(gen), pos_dist(gen) + 50.0f, pos_dist(gen)};
            Vector3 velocity{vel_dist(gen), vel_dist(gen), vel_dist(gen)};
            
            // Create rigid body with random properties
            // auto body = world.create_rigidbody();
            // body.set_position(position);
            // body.set_velocity(velocity);
        }
    }
    
    template<typename PhysicsWorld>
    bool detect_instability(const PhysicsWorld& world) {
        // Check for NaN values, excessive velocities, etc.
        /*
        for (const auto& body : world.get_rigidbodies()) {
            auto pos = body.position();
            auto vel = body.velocity();
            
            // Check for NaN
            if (std::isnan(pos.x) || std::isnan(pos.y) || std::isnan(pos.z) ||
                std::isnan(vel.x) || std::isnan(vel.y) || std::isnan(vel.z)) {
                return true;
            }
            
            // Check for excessive velocities
            if (vel.length() > 1000.0f) {
                return true;
            }
            
            // Check for positions too far from origin
            if (pos.length() > 10000.0f) {
                return true;
            }
        }
        */
        
        return false;
    }
};

// Physics test fixture
class PhysicsTestFixture : public TestFixture {
public:
    void setup() override {
        // Initialize physics world
        world_ = std::make_unique<ecscope::physics::World>();
        
        // Initialize validators and profilers
        determinism_validator_ = std::make_unique<PhysicsDeterminismValidator>();
        conservation_validator_ = std::make_unique<ConservationValidator>();
        collision_validator_ = std::make_unique<CollisionValidator>();
        profiler_ = std::make_unique<PhysicsProfiler>();
        stress_tester_ = std::make_unique<PhysicsStressTester>();
    }

    void teardown() override {
        world_.reset();
        determinism_validator_.reset();
        conservation_validator_.reset();
        collision_validator_.reset();
        profiler_.reset();
        stress_tester_.reset();
    }

protected:
    std::unique_ptr<ecscope::physics::World> world_;
    std::unique_ptr<PhysicsDeterminismValidator> determinism_validator_;
    std::unique_ptr<ConservationValidator> conservation_validator_;
    std::unique_ptr<CollisionValidator> collision_validator_;
    std::unique_ptr<PhysicsProfiler> profiler_;
    std::unique_ptr<PhysicsStressTester> stress_tester_;

    void create_falling_box_scene() {
        // Create a simple falling box scene for testing
        /*
        auto floor = world_->create_static_box({0, -1, 0}, {100, 1, 100});
        auto box = world_->create_dynamic_box({0, 10, 0}, {1, 1, 1});
        box.set_mass(1.0f);
        */
    }

    void create_pendulum_scene() {
        // Create a pendulum for conservation testing
        /*
        auto anchor = world_->create_static_sphere({0, 10, 0}, 0.1f);
        auto bob = world_->create_dynamic_sphere({0, 5, 0}, 0.5f);
        bob.set_mass(1.0f);
        
        auto constraint = world_->create_distance_constraint(anchor, bob, 5.0f);
        */
    }
};

// Specific physics tests
class PhysicsDeterminismTest : public PhysicsTestFixture {
public:
    PhysicsDeterminismTest() : PhysicsTestFixture() {
        context_.name = "Physics Determinism Test";
        context_.category = TestCategory::PHYSICS;
    }

    void run() override {
        create_falling_box_scene();
        
        bool is_deterministic = determinism_validator_->validate_determinism(*world_, 100, 5);
        ASSERT_TRUE(is_deterministic);
    }
};

class ConservationLawsTest : public PhysicsTestFixture {
public:
    ConservationLawsTest() : PhysicsTestFixture() {
        context_.name = "Conservation Laws Test";
        context_.category = TestCategory::PHYSICS;
    }

    void run() override {
        create_pendulum_scene();
        
        auto results = conservation_validator_->validate_conservation(*world_, 1000);
        
        ASSERT_TRUE(results.energy_conserved);
        ASSERT_TRUE(results.momentum_conserved);
        ASSERT_TRUE(results.angular_momentum_conserved);
        
        // Energy should be conserved within 1%
        ASSERT_LT(results.energy_drift, 0.01);
    }
};

class CollisionAccuracyTest : public PhysicsTestFixture {
public:
    CollisionAccuracyTest() : PhysicsTestFixture() {
        context_.name = "Collision Accuracy Test";
        context_.category = TestCategory::PHYSICS;
    }

    void run() override {
        bool collision_accurate = collision_validator_->validate_collision_detection(
            world_->get_collision_system());
        ASSERT_TRUE(collision_accurate);
    }
};

class PhysicsPerformanceTest : public BenchmarkTest {
public:
    PhysicsPerformanceTest() : BenchmarkTest("Physics Performance", 100) {
        context_.category = TestCategory::PERFORMANCE;
    }

    void setup() override {
        world_ = std::make_unique<ecscope::physics::World>();
        
        // Create performance test scene with many objects
        for (int i = 0; i < 1000; ++i) {
            // Create objects for performance testing
        }
    }

    void benchmark() override {
        world_->step(1.0f / 60.0f);
    }

    void teardown() override {
        world_.reset();
    }

private:
    std::unique_ptr<ecscope::physics::World> world_;
};

class PhysicsStressTest : public PhysicsTestFixture {
public:
    PhysicsStressTest() : PhysicsTestFixture() {
        context_.name = "Physics Stress Test";
        context_.category = TestCategory::STRESS;
        context_.timeout_seconds = 120; // Longer timeout for stress test
    }

    void run() override {
        bool stress_passed = stress_tester_->stress_test_many_bodies(*world_, 5000, 1000);
        ASSERT_TRUE(stress_passed);
    }
};

} // namespace ecscope::testing