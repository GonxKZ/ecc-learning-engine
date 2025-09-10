#include <benchmark/benchmark.h>
#include "ecscope/physics/physics_world.hpp"
#include "ecscope/physics/collision_detection.hpp"
#include <random>
#include <memory>

namespace ecscope::physics::benchmark {

// Global setup for benchmarks
class PhysicsBenchmarkFixture : public ::benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        world_config = PhysicsWorldConfig{};
        world_config.gravity = Vec3(0, -9.81f, 0);
        world_config.time_step = 1.0f / 60.0f;
        world_config.velocity_iterations = 8;
        world_config.position_iterations = 3;
        world_config.enable_multithreading = true;
        
        world = std::make_unique<PhysicsWorld>(world_config);
        
        // Initialize random number generator
        gen.seed(42); // Fixed seed for reproducible results
    }
    
    void TearDown(const ::benchmark::State& state) override {
        world.reset();
    }
    
protected:
    PhysicsWorldConfig world_config;
    std::unique_ptr<PhysicsWorld> world;
    std::mt19937 gen;
    
    void create_sphere_scene(size_t count, Real world_size = 100.0f) {
        std::uniform_real_distribution<Real> pos_dist(-world_size/2, world_size/2);
        std::uniform_real_distribution<Real> vel_dist(-5.0f, 5.0f);
        
        for (size_t i = 0; i < count; ++i) {
            SphereShape shape(0.5f + static_cast<Real>(i % 10) * 0.1f); // Varied sizes
            Material material{0.5f, 0.3f, 1.0f};
            
            Vec3 position(pos_dist(gen), pos_dist(gen) + 20.0f, pos_dist(gen));
            Vec3 velocity(vel_dist(gen), vel_dist(gen), vel_dist(gen));
            
            uint32_t body_id = world->create_dynamic_body_3d(position, Quaternion::identity(), shape, material);
            world->set_body_velocity_3d(body_id, velocity);
        }
    }
    
    void create_box_scene(size_t count, Real world_size = 100.0f) {
        std::uniform_real_distribution<Real> pos_dist(-world_size/2, world_size/2);
        std::uniform_real_distribution<Real> size_dist(0.5f, 2.0f);
        
        for (size_t i = 0; i < count; ++i) {
            Real size = size_dist(gen);
            BoxShape3D shape(Vec3(size, size, size));
            Material material{0.5f, 0.3f, 1.0f};
            
            Vec3 position(pos_dist(gen), pos_dist(gen) + 20.0f, pos_dist(gen));
            
            uint32_t body_id = world->create_dynamic_body_3d(position, Quaternion::identity(), shape, material);
        }
    }
};

// Benchmark broad phase collision detection performance
BENCHMARK_DEFINE_F(PhysicsBenchmarkFixture, BroadPhase_1000_Spheres)(benchmark::State& state) {
    create_sphere_scene(1000);
    
    for (auto _ : state) {
        world->step(world_config.time_step);
        benchmark::DoNotOptimize(world->get_stats());
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.counters["Bodies"] = 1000;
    state.counters["FPS"] = benchmark::Counter(1.0 / (state.mean_time_per_iteration() / 1e9), benchmark::Counter::kIsRate);
}
BENCHMARK_REGISTER_F(PhysicsBenchmarkFixture, BroadPhase_1000_Spheres);

BENCHMARK_DEFINE_F(PhysicsBenchmarkFixture, BroadPhase_5000_Spheres)(benchmark::State& state) {
    create_sphere_scene(5000);
    
    for (auto _ : state) {
        world->step(world_config.time_step);
        benchmark::DoNotOptimize(world->get_stats());
    }
    
    state.SetItemsProcessed(state.iterations() * 5000);
    state.counters["Bodies"] = 5000;
    state.counters["FPS"] = benchmark::Counter(1.0 / (state.mean_time_per_iteration() / 1e9), benchmark::Counter::kIsRate);
}
BENCHMARK_REGISTER_F(PhysicsBenchmarkFixture, BroadPhase_5000_Spheres);

BENCHMARK_DEFINE_F(PhysicsBenchmarkFixture, BroadPhase_10000_Spheres)(benchmark::State& state) {
    create_sphere_scene(10000);
    
    for (auto _ : state) {
        world->step(world_config.time_step);
        benchmark::DoNotOptimize(world->get_stats());
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
    state.counters["Bodies"] = 10000;
    state.counters["FPS"] = benchmark::Counter(1.0 / (state.mean_time_per_iteration() / 1e9), benchmark::Counter::kIsRate);
}
BENCHMARK_REGISTER_F(PhysicsBenchmarkFixture, BroadPhase_10000_Spheres);

// Benchmark narrow phase collision detection
BENCHMARK_DEFINE_F(PhysicsBenchmarkFixture, NarrowPhase_Spheres)(benchmark::State& state) {
    const size_t count = state.range(0);
    create_sphere_scene(count, 50.0f); // Smaller world for more collisions
    
    for (auto _ : state) {
        world->step(world_config.time_step);
        PhysicsStats stats = world->get_stats();
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetItemsProcessed(state.iterations() * count);
    state.counters["Bodies"] = count;
}
BENCHMARK_REGISTER_F(PhysicsBenchmarkFixture, NarrowPhase_Spheres)->Range(100, 2000)->Unit(benchmark::kMicrosecond);

BENCHMARK_DEFINE_F(PhysicsBenchmarkFixture, NarrowPhase_Boxes)(benchmark::State& state) {
    const size_t count = state.range(0);
    create_box_scene(count, 50.0f);
    
    for (auto _ : state) {
        world->step(world_config.time_step);
        PhysicsStats stats = world->get_stats();
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetItemsProcessed(state.iterations() * count);
    state.counters["Bodies"] = count;
}
BENCHMARK_REGISTER_F(PhysicsBenchmarkFixture, NarrowPhase_Boxes)->Range(100, 1000)->Unit(benchmark::kMicrosecond);

// Benchmark constraint solver performance
BENCHMARK_DEFINE_F(PhysicsBenchmarkFixture, ConstraintSolver_Stack)(benchmark::State& state) {
    const size_t count = state.range(0);
    
    // Create a stack of boxes (lots of contacts)
    for (size_t i = 0; i < count; ++i) {
        BoxShape3D shape(Vec3(1, 0.5f, 1));
        Material material{0.8f, 0.1f, 1.0f}; // High friction, low restitution
        
        Vec3 position(0, static_cast<Real>(i) * 1.1f, 0);
        uint32_t body_id = world->create_dynamic_body_3d(position, Quaternion::identity(), shape, material);
    }
    
    // Let it settle for a few steps
    for (int i = 0; i < 10; ++i) {
        world->step(world_config.time_step);
    }
    
    for (auto _ : state) {
        world->step(world_config.time_step);
        PhysicsStats stats = world->get_stats();
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK_REGISTER_F(PhysicsBenchmarkFixture, ConstraintSolver_Stack)->Range(10, 200);

// Benchmark different collision detection algorithms
static void BM_SphereCollisionDetection(benchmark::State& state) {
    SphereShape sphere_a(1.0f);
    SphereShape sphere_b(1.0f);
    Transform3D transform_a(Vec3(0, 0, 0), Quaternion::identity());
    Transform3D transform_b(Vec3(1.5f, 0, 0), Quaternion::identity());
    
    for (auto _ : state) {
        ContactManifold manifold(1, 2);
        bool result = test_sphere_sphere_optimized(sphere_a, transform_a, sphere_b, transform_b, manifold);
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(manifold);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SphereCollisionDetection)->Unit(benchmark::kNanosecond);

static void BM_GJKCollisionDetection(benchmark::State& state) {
    BoxShape3D box_a(Vec3(1, 1, 1));
    BoxShape3D box_b(Vec3(1, 1, 1));
    Transform3D transform_a(Vec3(0, 0, 0), Quaternion::identity());
    Transform3D transform_b(Vec3(1.5f, 0, 0), Quaternion::identity());
    
    for (auto _ : state) {
        Simplex simplex;
        bool result = GJK::intersects(box_a, transform_a, box_b, transform_b, simplex);
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(simplex);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GJKCollisionDetection)->Unit(benchmark::kNanosecond);

static void BM_EPAContactGeneration(benchmark::State& state) {
    BoxShape3D box_a(Vec3(1, 1, 1));
    BoxShape3D box_b(Vec3(1, 1, 1));
    Transform3D transform_a(Vec3(0, 0, 0), Quaternion::identity());
    Transform3D transform_b(Vec3(1.5f, 0, 0), Quaternion::identity());
    
    // Pre-compute GJK result
    Simplex simplex;
    bool collision = GJK::intersects(box_a, transform_a, box_b, transform_b, simplex);
    assert(collision);
    
    for (auto _ : state) {
        ContactManifold manifold = EPA::get_contact_manifold(box_a, transform_a, box_b, transform_b, simplex);
        benchmark::DoNotOptimize(manifold);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EPAContactGeneration)->Unit(benchmark::kMicrosecond);

// Benchmark spatial hash performance
static void BM_SpatialHashInsertion(benchmark::State& state) {
    const size_t count = state.range(0);
    auto broad_phase = create_optimal_broad_phase(count, 100.0f);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<Real> pos_dist(-50.0f, 50.0f);
    
    std::vector<RigidBody3D> bodies(count);
    std::vector<SphereShape> shapes(count, SphereShape(1.0f));
    
    for (size_t i = 0; i < count; ++i) {
        bodies[i].id = static_cast<uint32_t>(i);
        bodies[i].transform.position = Vec3(pos_dist(gen), pos_dist(gen), pos_dist(gen));
    }
    
    for (auto _ : state) {
        broad_phase->clear();
        for (size_t i = 0; i < count; ++i) {
            broad_phase->add_body_3d(bodies[i], shapes[i]);
        }
        benchmark::DoNotOptimize(broad_phase.get());
    }
    
    state.SetItemsProcessed(state.iterations() * count);
    state.counters["Objects/sec"] = benchmark::Counter(count, benchmark::Counter::kIsIterationInvariantRate);
}
BENCHMARK(BM_SpatialHashInsertion)->Range(1000, 50000);

static void BM_SpatialHashPairGeneration(benchmark::State& state) {
    const size_t count = state.range(0);
    auto broad_phase = create_optimal_broad_phase(count, 50.0f); // Smaller world for more pairs
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<Real> pos_dist(-25.0f, 25.0f);
    
    // Pre-populate spatial hash
    for (size_t i = 0; i < count; ++i) {
        RigidBody3D body;
        body.id = static_cast<uint32_t>(i);
        body.transform.position = Vec3(pos_dist(gen), pos_dist(gen), pos_dist(gen));
        
        SphereShape shape(1.0f);
        broad_phase->add_body_3d(body, shape);
    }
    
    for (auto _ : state) {
        broad_phase->find_collision_pairs_3d();
        const auto& pairs = broad_phase->get_collision_pairs();
        benchmark::DoNotOptimize(pairs);
    }
    
    state.SetItemsProcessed(state.iterations() * count);
    BroadPhaseStats stats = broad_phase->get_stats();
    state.counters["Pairs"] = stats.total_pairs;
    state.counters["Efficiency"] = stats.efficiency_ratio;
}
BENCHMARK(BM_SpatialHashPairGeneration)->Range(1000, 10000);

// Memory allocation benchmarks
static void BM_PhysicsWorldCreation(benchmark::State& state) {
    PhysicsWorldConfig config;
    
    for (auto _ : state) {
        auto world = std::make_unique<PhysicsWorld>(config);
        benchmark::DoNotOptimize(world.get());
    }
}
BENCHMARK(BM_PhysicsWorldCreation);

// Threaded vs single-threaded comparison
BENCHMARK_DEFINE_F(PhysicsBenchmarkFixture, SingleThreaded_1000_Bodies)(benchmark::State& state) {
    world_config.enable_multithreading = false;
    world = std::make_unique<PhysicsWorld>(world_config);
    create_sphere_scene(1000);
    
    for (auto _ : state) {
        world->step(world_config.time_step);
    }
    
    state.counters["FPS"] = benchmark::Counter(1.0 / (state.mean_time_per_iteration() / 1e9), benchmark::Counter::kIsRate);
}
BENCHMARK_REGISTER_F(PhysicsBenchmarkFixture, SingleThreaded_1000_Bodies);

BENCHMARK_DEFINE_F(PhysicsBenchmarkFixture, MultiThreaded_1000_Bodies)(benchmark::State& state) {
    world_config.enable_multithreading = true;
    world = std::make_unique<PhysicsWorld>(world_config);
    create_sphere_scene(1000);
    
    for (auto _ : state) {
        world->step(world_config.time_step);
    }
    
    state.counters["FPS"] = benchmark::Counter(1.0 / (state.mean_time_per_iteration() / 1e9), benchmark::Counter::kIsRate);
}
BENCHMARK_REGISTER_F(PhysicsBenchmarkFixture, MultiThreaded_1000_Bodies);

} // namespace ecscope::physics::benchmark

// Custom main function for benchmarks
int main(int argc, char** argv) {
    std::cout << "=== ECScope Physics Engine Performance Benchmarks ===\n";
    std::cout << "Measuring performance for production-ready 2D/3D physics\n";
    std::cout << "Target: 10,000+ bodies at 60fps (16.67ms per frame)\n\n";
    
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    std::cout << "\n=== Benchmark Suite Complete ===\n";
    std::cout << "Key Performance Indicators:\n";
    std::cout << "- Sub-millisecond broad phase for 10K+ objects\n";
    std::cout << "- 60+ FPS with 5000+ active bodies\n";
    std::cout << "- Multi-threaded speedup for large scenes\n";
    std::cout << "- Memory-efficient collision detection\n";
    
    return 0;
}