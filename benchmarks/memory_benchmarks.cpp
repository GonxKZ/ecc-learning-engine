#include <benchmark/benchmark.h>
#include <ecscope/memory/memory_manager.hpp>
#include <ecscope/memory/allocators.hpp>
#include <ecscope/memory/memory_utils.hpp>
#include <vector>
#include <random>
#include <memory>

using namespace ecscope::memory;

// ==== ALLOCATION BENCHMARKS ====

// Benchmark different allocation strategies
static void BM_AllocationStrategy_Fastest(benchmark::State& state) {
    auto& manager = MemoryManager::instance();
    MemoryPolicy policy;
    policy.strategy = AllocationStrategy::FASTEST;
    policy.enable_tracking = false;
    
    size_t size = state.range(0);
    
    for (auto _ : state) {
        void* ptr = manager.allocate(size, policy);
        benchmark::DoNotOptimize(ptr);
        if (ptr) manager.deallocate(ptr, size, policy);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
}

static void BM_AllocationStrategy_MostEfficient(benchmark::State& state) {
    auto& manager = MemoryManager::instance();
    MemoryPolicy policy;
    policy.strategy = AllocationStrategy::MOST_EFFICIENT;
    policy.enable_tracking = false;
    
    size_t size = state.range(0);
    
    for (auto _ : state) {
        void* ptr = manager.allocate(size, policy);
        benchmark::DoNotOptimize(ptr);
        if (ptr) manager.deallocate(ptr, size, policy);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
}

static void BM_AllocationStrategy_SizeSegregated(benchmark::State& state) {
    auto& manager = MemoryManager::instance();
    MemoryPolicy policy;
    policy.strategy = AllocationStrategy::SIZE_SEGREGATED;
    policy.enable_tracking = false;
    
    size_t size = state.range(0);
    
    for (auto _ : state) {
        void* ptr = manager.allocate(size, policy);
        benchmark::DoNotOptimize(ptr);
        if (ptr) manager.deallocate(ptr, size, policy);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
}

static void BM_AllocationStrategy_ThreadLocal(benchmark::State& state) {
    auto& manager = MemoryManager::instance();
    MemoryPolicy policy;
    policy.strategy = AllocationStrategy::THREAD_LOCAL;
    policy.enable_tracking = false;
    
    size_t size = state.range(0);
    
    for (auto _ : state) {
        void* ptr = manager.allocate(size, policy);
        benchmark::DoNotOptimize(ptr);
        if (ptr) manager.deallocate(ptr, size, policy);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
}

// Benchmark standard malloc/free for comparison
static void BM_StandardMalloc(benchmark::State& state) {
    size_t size = state.range(0);
    
    for (auto _ : state) {
        void* ptr = std::malloc(size);
        benchmark::DoNotOptimize(ptr);
        if (ptr) std::free(ptr);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
}

// Register allocation benchmarks with various sizes
BENCHMARK(BM_AllocationStrategy_Fastest)->Range(8, 8<<20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_AllocationStrategy_MostEfficient)->Range(8, 8<<20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_AllocationStrategy_SizeSegregated)->Range(8, 8<<20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_AllocationStrategy_ThreadLocal)->Range(8, 8<<20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StandardMalloc)->Range(8, 8<<20)->Unit(benchmark::kNanosecond);

// ==== ALLOCATOR-SPECIFIC BENCHMARKS ====

static void BM_LinearAllocator(benchmark::State& state) {
    size_t size = state.range(0);
    LinearAllocator allocator(64 * 1024 * 1024); // 64MB
    
    for (auto _ : state) {
        void* ptr = allocator.allocate(size);
        benchmark::DoNotOptimize(ptr);
        
        // Reset when full to continue benchmarking
        if (!ptr) {
            allocator.reset();
            ptr = allocator.allocate(size);
        }
    }
    
    state.SetBytesProcessed(state.iterations() * size);
}

static void BM_ObjectPool(benchmark::State& state) {
    struct TestObject { uint64_t data[8]; }; // 64-byte object
    ObjectPool<TestObject> pool(10000);
    
    for (auto _ : state) {
        TestObject* obj = pool.allocate();
        benchmark::DoNotOptimize(obj);
        if (obj) pool.deallocate(obj);
    }
    
    state.SetBytesProcessed(state.iterations() * sizeof(TestObject));
}

static void BM_LockFreeAllocator(benchmark::State& state) {
    LockFreeAllocator<64> allocator(64 * 1024 * 1024); // 64MB, 64-byte blocks
    
    for (auto _ : state) {
        void* ptr = allocator.allocate();
        benchmark::DoNotOptimize(ptr);
        if (ptr) allocator.deallocate(ptr);
    }
    
    state.SetBytesProcessed(state.iterations() * 64);
}

BENCHMARK(BM_LinearAllocator)->Range(8, 8<<12)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ObjectPool)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_LockFreeAllocator)->Unit(benchmark::kNanosecond);

// ==== SIMD OPERATION BENCHMARKS ====

static void BM_SIMDCopy(benchmark::State& state) {
    size_t size = state.range(0);
    void* src = std::aligned_alloc(32, size);
    void* dst = std::aligned_alloc(32, size);
    
    // Fill source with data
    std::memset(src, 0xAB, size);
    
    for (auto _ : state) {
        SIMDMemoryOps::fast_copy(dst, src, size);
        benchmark::DoNotOptimize(dst);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    std::free(src);
    std::free(dst);
}

static void BM_StandardMemcpy(benchmark::State& state) {
    size_t size = state.range(0);
    void* src = std::aligned_alloc(32, size);
    void* dst = std::aligned_alloc(32, size);
    
    std::memset(src, 0xAB, size);
    
    for (auto _ : state) {
        std::memcpy(dst, src, size);
        benchmark::DoNotOptimize(dst);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    std::free(src);
    std::free(dst);
}

static void BM_SIMDSet(benchmark::State& state) {
    size_t size = state.range(0);
    void* dst = std::aligned_alloc(32, size);
    
    for (auto _ : state) {
        SIMDMemoryOps::fast_set(dst, 0xCD, size);
        benchmark::DoNotOptimize(dst);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    std::free(dst);
}

static void BM_StandardMemset(benchmark::State& state) {
    size_t size = state.range(0);
    void* dst = std::aligned_alloc(32, size);
    
    for (auto _ : state) {
        std::memset(dst, 0xCD, size);
        benchmark::DoNotOptimize(dst);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    std::free(dst);
}

static void BM_SIMDCompare(benchmark::State& state) {
    size_t size = state.range(0);
    void* buf1 = std::aligned_alloc(32, size);
    void* buf2 = std::aligned_alloc(32, size);
    
    std::memset(buf1, 0xAB, size);
    std::memset(buf2, 0xAB, size);
    
    for (auto _ : state) {
        int result = SIMDMemoryOps::fast_compare(buf1, buf2, size);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    std::free(buf1);
    std::free(buf2);
}

static void BM_StandardMemcmp(benchmark::State& state) {
    size_t size = state.range(0);
    void* buf1 = std::aligned_alloc(32, size);
    void* buf2 = std::aligned_alloc(32, size);
    
    std::memset(buf1, 0xAB, size);
    std::memset(buf2, 0xAB, size);
    
    for (auto _ : state) {
        int result = std::memcmp(buf1, buf2, size);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    std::free(buf1);
    std::free(buf2);
}

// Register SIMD benchmarks
BENCHMARK(BM_SIMDCopy)->Range(1<<6, 1<<20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StandardMemcpy)->Range(1<<6, 1<<20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_SIMDSet)->Range(1<<6, 1<<20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StandardMemset)->Range(1<<6, 1<<20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_SIMDCompare)->Range(1<<6, 1<<20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StandardMemcmp)->Range(1<<6, 1<<20)->Unit(benchmark::kNanosecond);

// ==== MULTITHREADED BENCHMARKS ====

static void BM_ThreadSafeAllocation(benchmark::State& state) {
    auto& manager = MemoryManager::instance();
    MemoryPolicy policy;
    policy.strategy = AllocationStrategy::THREAD_LOCAL;
    policy.enable_tracking = false;
    
    const size_t size = 64;
    
    for (auto _ : state) {
        void* ptr = manager.allocate(size, policy);
        benchmark::DoNotOptimize(ptr);
        if (ptr) manager.deallocate(ptr, size, policy);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
}

static void BM_LockFreeMultithreaded(benchmark::State& state) {
    static LockFreeAllocator<64> allocator(64 * 1024 * 1024);
    
    for (auto _ : state) {
        void* ptr = allocator.allocate();
        benchmark::DoNotOptimize(ptr);
        if (ptr) allocator.deallocate(ptr);
    }
    
    state.SetBytesProcessed(state.iterations() * 64);
}

BENCHMARK(BM_ThreadSafeAllocation)->Threads(1)->Threads(2)->Threads(4)->Threads(8)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_LockFreeMultithreaded)->Threads(1)->Threads(2)->Threads(4)->Threads(8)->Unit(benchmark::kNanosecond);

// ==== REAL-WORLD SCENARIO BENCHMARKS ====

// Simulate game entity allocation/deallocation patterns
static void BM_GameEntityPattern(benchmark::State& state) {
    struct Entity {
        uint64_t id;
        float position[3];
        float velocity[3];
        uint32_t components;
    };
    
    auto& manager = MemoryManager::instance();
    MemoryPolicy policy;
    policy.strategy = AllocationStrategy::SIZE_SEGREGATED;
    policy.enable_tracking = false;
    
    std::vector<Entity*> entities;
    entities.reserve(1000);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (auto _ : state) {
        // Allocate entities (70% probability)
        if (dis(gen) < 0.7 && entities.size() < 1000) {
            Entity* entity = manager.allocate_object<Entity>(policy);
            if (entity) {
                entity->id = entities.size();
                entities.push_back(entity);
            }
        }
        // Deallocate entities (30% probability)
        else if (!entities.empty()) {
            size_t index = static_cast<size_t>(dis(gen) * entities.size());
            Entity* entity = entities[index];
            entities.erase(entities.begin() + index);
            manager.deallocate_object(entity, policy);
        }
        
        benchmark::DoNotOptimize(entities);
    }
    
    // Cleanup remaining entities
    for (Entity* entity : entities) {
        manager.deallocate_object(entity, policy);
    }
    
    state.SetBytesProcessed(state.iterations() * sizeof(Entity));
}

// Simulate string allocation patterns
static void BM_StringAllocationPattern(benchmark::State& state) {
    auto& manager = MemoryManager::instance();
    MemoryPolicy policy;
    policy.strategy = AllocationStrategy::BALANCED;
    policy.enable_tracking = false;
    
    std::vector<char*> strings;
    strings.reserve(100);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> size_dist(16, 256);
    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    
    for (auto _ : state) {
        // Allocate strings (60% probability)
        if (prob_dist(gen) < 0.6 && strings.size() < 100) {
            size_t size = size_dist(gen);
            char* str = manager.allocate_array<char>(size, policy);
            if (str) {
                // Fill with dummy data
                std::memset(str, 'A' + (strings.size() % 26), size - 1);
                str[size - 1] = '\0';
                strings.push_back(str);
            }
        }
        // Deallocate strings (40% probability)
        else if (!strings.empty()) {
            size_t index = static_cast<size_t>(prob_dist(gen) * strings.size());
            char* str = strings[index];
            size_t len = std::strlen(str) + 1;
            strings.erase(strings.begin() + index);
            manager.deallocate_array(str, len, policy);
        }
        
        benchmark::DoNotOptimize(strings);
    }
    
    // Cleanup remaining strings
    for (char* str : strings) {
        size_t len = std::strlen(str) + 1;
        manager.deallocate_array(str, len, policy);
    }
}

// Simulate large temporary buffer allocations (like audio/video processing)
static void BM_LargeBufferPattern(benchmark::State& state) {
    auto& manager = MemoryManager::instance();
    MemoryPolicy policy;
    policy.strategy = AllocationStrategy::FASTEST;
    policy.enable_tracking = false;
    
    constexpr size_t BUFFER_SIZE = 64 * 1024; // 64KB buffers
    
    for (auto _ : state) {
        // Allocate buffer
        uint8_t* buffer = manager.allocate_array<uint8_t>(BUFFER_SIZE, policy);
        if (buffer) {
            // Simulate processing (memory access pattern)
            for (size_t i = 0; i < BUFFER_SIZE; i += 64) {
                buffer[i] = static_cast<uint8_t>(i & 0xFF);
                benchmark::DoNotOptimize(buffer[i]);
            }
            
            manager.deallocate_array(buffer, BUFFER_SIZE, policy);
        }
    }
    
    state.SetBytesProcessed(state.iterations() * BUFFER_SIZE);
}

BENCHMARK(BM_GameEntityPattern)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StringAllocationPattern)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_LargeBufferPattern)->Unit(benchmark::kMicrosecond);

// ==== FRAGMENTATION BENCHMARK ====

static void BM_FragmentationResistance(benchmark::State& state) {
    auto& manager = MemoryManager::instance();
    MemoryPolicy policy;
    policy.strategy = AllocationStrategy::SIZE_SEGREGATED;
    policy.enable_tracking = false;
    
    std::vector<void*> ptrs;
    ptrs.reserve(1000);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> size_dist(16, 1024);
    
    // Pre-allocate to create fragmentation
    for (int i = 0; i < 500; ++i) {
        void* ptr = manager.allocate(size_dist(gen), policy);
        if (ptr) ptrs.push_back(ptr);
    }
    
    // Free every other allocation to create holes
    for (size_t i = 1; i < ptrs.size(); i += 2) {
        manager.deallocate(ptrs[i]);
        ptrs[i] = nullptr;
    }
    
    for (auto _ : state) {
        // Try to allocate in fragmented space
        size_t size = size_dist(gen);
        void* ptr = manager.allocate(size, policy);
        benchmark::DoNotOptimize(ptr);
        if (ptr) manager.deallocate(ptr, size, policy);
    }
    
    // Cleanup
    for (void* ptr : ptrs) {
        if (ptr) manager.deallocate(ptr);
    }
}

BENCHMARK(BM_FragmentationResistance)->Unit(benchmark::kNanosecond);

// ==== MEMORY PRESSURE BENCHMARK ====

static void BM_MemoryPressureHandling(benchmark::State& state) {
    auto& manager = MemoryManager::instance();
    MemoryPolicy policy;
    policy.strategy = AllocationStrategy::BALANCED;
    policy.enable_tracking = false;
    policy.enable_automatic_cleanup = true;
    
    std::vector<void*> long_lived;
    std::vector<void*> short_lived;
    
    long_lived.reserve(100);
    short_lived.reserve(50);
    
    // Create memory pressure with long-lived allocations
    for (int i = 0; i < 100; ++i) {
        void* ptr = manager.allocate(1024 * 1024, policy); // 1MB each
        if (ptr) long_lived.push_back(ptr);
    }
    
    for (auto _ : state) {
        // Allocate short-lived memory under pressure
        for (int i = 0; i < 50; ++i) {
            void* ptr = manager.allocate(64 * 1024, policy); // 64KB each
            if (ptr) short_lived.push_back(ptr);
        }
        
        // Free short-lived memory
        for (void* ptr : short_lived) {
            manager.deallocate(ptr, 64 * 1024, policy);
        }
        short_lived.clear();
        
        benchmark::DoNotOptimize(short_lived);
    }
    
    // Cleanup long-lived memory
    for (void* ptr : long_lived) {
        manager.deallocate(ptr, 1024 * 1024, policy);
    }
}

BENCHMARK(BM_MemoryPressureHandling)->Unit(benchmark::kMicrosecond);

// Custom main function to initialize the memory manager
int main(int argc, char** argv) {
    // Initialize memory manager with benchmarking-optimized settings
    MemoryPolicy policy;
    policy.enable_tracking = false;         // Disable for pure performance
    policy.enable_leak_detection = false;   // Disable for pure performance
    policy.prefer_simd_operations = true;
    policy.enable_automatic_cleanup = true;
    
    MemoryManager::instance().initialize(policy);
    
    // Print system capabilities
    std::cout << "Memory Management System Benchmarks\n";
    std::cout << "====================================\n\n";
    
    std::cout << "System Configuration:\n";
    std::cout << "  Hardware threads: " << std::thread::hardware_concurrency() << "\n";
    std::cout << "  Cache line size: " << get_cache_line_size() << " bytes\n";
    
    if (SIMDMemoryOps::has_sse2()) std::cout << "  SSE2: Available\n";
    if (SIMDMemoryOps::has_avx2()) std::cout << "  AVX2: Available\n";
    if (SIMDMemoryOps::has_avx512()) std::cout << "  AVX512: Available\n";
    
    auto& topology = NUMATopology::instance();
    std::cout << "  NUMA nodes: " << topology.get_num_nodes() << "\n";
    
    std::cout << "\nRunning benchmarks...\n\n";
    
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    
    // Print final memory statistics
    auto metrics = MemoryManager::instance().get_performance_metrics();
    std::cout << "\nFinal Memory Statistics:\n";
    std::cout << "  Peak allocated bytes: " << metrics.peak_allocated_bytes << "\n";
    std::cout << "  Total allocations: " << metrics.total_allocations << "\n";
    std::cout << "  Failed allocations: " << metrics.failed_allocations << "\n";
    
    if (metrics.total_allocations > 0) {
        double success_rate = 1.0 - (static_cast<double>(metrics.failed_allocations) / metrics.total_allocations);
        std::cout << "  Success rate: " << (success_rate * 100.0) << "%\n";
    }
    
    ::benchmark::Shutdown();
    return 0;
}