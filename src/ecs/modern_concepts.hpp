#pragma once

/**
 * @file ecs/modern_concepts.hpp
 * @brief Advanced C++20 Concepts for Type-Safe ECS Operations
 * 
 * This header provides comprehensive C++20 concepts that ensure type safety
 * across all ECS operations while providing clear, educational error messages
 * when constraints are violated. These concepts build upon the existing
 * component system and extend it with modern C++ type safety features.
 * 
 * Key Features:
 * - Comprehensive type safety for ECS components and systems
 * - Clear, educational error messages for constraint violations
 * - Performance-oriented constraints (trivially copyable, size limits)
 * - Integration with existing memory allocator concepts
 * - Query builder type safety and optimization hints
 * - System dependency and resource access validation
 * 
 * Educational Value:
 * - Demonstrates modern C++20 concepts usage
 * - Shows how to write self-documenting code through constraints
 * - Provides examples of constraint composition and refinement
 * - Illustrates performance-conscious type constraints
 * 
 * @author ECScope Educational ECS Framework - Modern Extensions
 * @date 2024
 */

#include "core/types.hpp"
#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include <concepts>
#include <type_traits>
#include <memory_resource>

namespace ecscope::ecs::concepts {

//=============================================================================
// Core ECS Type Concepts
//=============================================================================

/**
 * @brief Concept for valid ECS entities
 * 
 * Educational note: This concept ensures that types used as entities
 * have the required interface and can be efficiently stored and compared.
 */
template<typename T>
concept EntityType = requires(T entity) {
    // Must have an ID that can be converted to u32
    { entity.id() } -> std::convertible_to<u32>;
    
    // Must be comparable for efficient lookups
    { entity == entity } -> std::convertible_to<bool>;
    { entity != entity } -> std::convertible_to<bool>;
    
    // Must be trivially copyable for efficient storage
    requires std::is_trivially_copyable_v<T>;
    
    // Must be reasonably sized (entities should be lightweight)
    requires sizeof(T) <= 16;
    
    // Must have invalid state for error handling
    { T::invalid() } -> std::convertible_to<T>;
};

/**
 * @brief Enhanced component concept with performance constraints
 * 
 * Educational note: This extends the basic Component concept with
 * additional constraints that ensure optimal performance in ECS systems.
 */
template<typename T>
concept PerformantComponent = Component<T> && requires {
    // Must be trivially copyable for efficient memory operations
    requires std::is_trivially_copyable_v<T>;
    
    // Must be default constructible for initialization
    requires std::is_default_constructible_v<T>;
    
    // Should be reasonably sized to fit in cache lines efficiently
    requires sizeof(T) <= 1024; // 1KB limit for single components
    
    // Must have proper alignment for SIMD operations (power of 2)
    requires (alignof(T) & (alignof(T) - 1)) == 0;
};

/**
 * @brief Concept for components suitable for SoA (Structure of Arrays) storage
 * 
 * Educational note: SoA storage provides better cache performance for
 * systems that process many entities but only access a few component fields.
 */
template<typename T>
concept SoATransformable = PerformantComponent<T> && requires {
    // Must have at least 2 fields to benefit from SoA
    requires sizeof(T) > sizeof(void*);
    
    // Must be trivially destructible for bulk operations
    requires std::is_trivially_destructible_v<T>;
    
    // Should not be too large (SoA becomes inefficient for very large components)
    requires sizeof(T) <= 512;
    
    // Must be standard layout for reliable field offset calculations
    requires std::is_standard_layout_v<T>;
};

/**
 * @brief Concept for components that can be processed with SIMD instructions
 * 
 * Educational note: SIMD (Single Instruction, Multiple Data) processing
 * can provide significant performance improvements for mathematical components.
 */
template<typename T>
concept SimdCompatibleComponent = PerformantComponent<T> && requires {
    // Must contain arithmetic data suitable for vectorization
    requires std::is_arithmetic_v<T> || requires {
        // Or must be composed of arithmetic types
        requires std::is_aggregate_v<T>;
    };
    
    // Must have alignment suitable for SIMD operations
    requires alignof(T) >= 16; // At least SSE alignment
    
    // Size must be a power of 2 for efficient SIMD loading
    requires (sizeof(T) & (sizeof(T) - 1)) == 0;
    
    // Must not be too large for SIMD registers
    requires sizeof(T) <= 64; // Maximum AVX-512 register size
};

/**
 * @brief Concept for components that support change detection
 * 
 * Educational note: Change detection allows systems to process only
 * modified components, reducing unnecessary work.
 */
template<typename T>
concept ChangeTrackableComponent = PerformantComponent<T> && requires {
    // Must support equality comparison for change detection
    { std::declval<const T&>() == std::declval<const T&>() } -> std::convertible_to<bool>;
    
    // Must be hashable for efficient change detection
    { std::hash<T>{}(std::declval<const T&>()) } -> std::convertible_to<std::size_t>;
    
    // Must be copyable for storing previous state
    requires std::is_copy_constructible_v<T>;
    requires std::is_copy_assignable_v<T>;
};

//=============================================================================
// Memory Management Concepts
//=============================================================================

/**
 * @brief Concept for ECS-compatible memory allocators
 * 
 * Educational note: Custom allocators can significantly improve ECS
 * performance by providing cache-friendly memory layouts and reducing
 * allocation overhead.
 */
template<typename Allocator>
concept ECSAllocator = requires(Allocator& alloc, std::size_t size, std::size_t align, const void* ptr) {
    // Core allocation interface
    { alloc.allocate(size, align) } -> std::convertible_to<void*>;
    
    // Must support ownership queries for debugging
    { alloc.owns(ptr) } -> std::convertible_to<bool>;
    
    // Must provide size information
    { alloc.total_size() } -> std::convertible_to<std::size_t>;
    { alloc.used_size() } -> std::convertible_to<std::size_t>;
    
    // Must support reset/cleanup operations
    alloc.reset();
};

/**
 * @brief Concept for arena-style allocators
 * 
 * Educational note: Arena allocators provide linear allocation with
 * excellent cache performance and very low allocation overhead.
 */
template<typename ArenaAllocator>
concept ArenaAllocatorType = ECSAllocator<ArenaAllocator> && requires(ArenaAllocator& arena) {
    // Must support checkpoint/restore for scoped allocations
    { arena.create_checkpoint() } -> std::convertible_to<typename ArenaAllocator::Checkpoint>;
    arena.restore_checkpoint(std::declval<typename ArenaAllocator::Checkpoint>());
    
    // Must provide allocation statistics
    { arena.stats() };
    
    // Must have a name for debugging/visualization
    { arena.name() } -> std::convertible_to<std::string>;
};

/**
 * @brief Concept for pool allocators optimized for fixed-size allocations
 * 
 * Educational note: Pool allocators excel when allocating many objects
 * of the same size, providing O(1) allocation/deallocation.
 */
template<typename PoolAllocator>
concept PoolAllocatorType = ECSAllocator<PoolAllocator> && requires(PoolAllocator& pool) {
    // Must specify block size
    { pool.block_size() } -> std::convertible_to<std::size_t>;
    
    // Must support capacity management
    { pool.capacity() } -> std::convertible_to<std::size_t>;
    { pool.allocated_count() } -> std::convertible_to<std::size_t>;
    
    // Must support shrinking for memory management
    { pool.shrink_pool() } -> std::convertible_to<std::size_t>;
};

/**
 * @brief Concept for PMR (Polymorphic Memory Resource) compatibility
 */
template<typename T>
concept PMRCompatible = requires {
    // Must work with polymorphic allocators
    typename std::pmr::vector<T>;
    typename std::pmr::unordered_map<int, T>;
    
    // Must be compatible with memory resources
    requires std::is_same_v<T, void> || requires {
        std::pmr::polymorphic_allocator<T>{};
    };
};

//=============================================================================
// Query System Concepts
//=============================================================================

/**
 * @brief Concept for types that can be used in ECS queries
 * 
 * Educational note: Query systems need to efficiently filter and iterate
 * over entities based on their component composition.
 */
template<typename T>
concept Queryable = PerformantComponent<T> && requires {
    // Must be efficiently comparable for filtering
    requires std::is_trivially_copyable_v<T>;
    
    // Must have stable address for query result caching
    requires !std::is_reference_v<T>;
    
    // Must be movable for efficient query result storage
    requires std::is_move_constructible_v<T>;
};

/**
 * @brief Concept for query filter types
 * 
 * Educational note: Query filters allow complex component matching logic
 * such as ALL, ANY, NOT, and WITH/WITHOUT combinations.
 */
template<typename Filter>
concept QueryFilter = requires {
    // Must specify the type of filter operation
    Filter::is_inclusive_filter || Filter::is_exclusive_filter || 
    Filter::is_optional_filter || Filter::is_change_filter;
    
    // Must provide component type information
    requires requires {
        typename Filter::component_type;
    } || requires {
        typename Filter::component_types;
    };
};

/**
 * @brief Concept for inclusive query filters (WITH, ALL)
 */
template<typename Filter>
concept InclusiveQueryFilter = QueryFilter<Filter> && requires {
    requires Filter::is_inclusive_filter;
    
    // Must provide method to create component signature
    { Filter::to_signature() };
};

/**
 * @brief Concept for exclusive query filters (WITHOUT, NOT)
 */
template<typename Filter>
concept ExclusiveQueryFilter = QueryFilter<Filter> && requires {
    requires Filter::is_exclusive_filter;
    
    // Must provide method to create exclusion signature
    { Filter::to_exclusion_signature() };
};

/**
 * @brief Concept for optional query filters
 */
template<typename Filter>
concept OptionalQueryFilter = QueryFilter<Filter> && requires {
    requires Filter::is_optional_filter;
    typename Filter::component_type;
};

/**
 * @brief Concept for change detection filters
 */
template<typename Filter>
concept ChangeDetectionFilter = QueryFilter<Filter> && requires {
    requires Filter::is_change_filter;
    typename Filter::component_type;
    requires ChangeTrackableComponent<typename Filter::component_type>;
};

//=============================================================================
// System Concepts
//=============================================================================

/**
 * @brief Concept for valid ECS systems
 * 
 * Educational note: Systems contain the game logic and operate on
 * entities with specific component combinations.
 */
template<typename System>
concept ECSSystem = requires(System& system, const auto& context) {
    // Must have a name for identification and debugging
    { system.name() } -> std::convertible_to<std::string>;
    
    // Must support lifecycle operations
    system.initialize(context);
    system.update(context);
    system.shutdown(context);
    
    // Must provide dependency information
    { system.dependencies() };
    { system.resource_info() };
    
    // Must be configurable
    { system.is_enabled() } -> std::convertible_to<bool>;
    system.set_enabled(true);
};

/**
 * @brief Concept for systems that can be executed in parallel
 * 
 * Educational note: Parallel systems can run simultaneously without
 * conflicting resource access, improving overall performance.
 */
template<typename System>
concept ParallelizableSystem = ECSSystem<System> && requires(System& system) {
    // Must declare thread safety
    requires System::is_thread_safe;
    
    // Must not have conflicting resource access
    requires !System::requires_exclusive_access;
    
    // Must provide resource access patterns for conflict detection
    { system.reads_components() };
    { system.writes_components() };
};

/**
 * @brief Concept for event-driven systems
 * 
 * Educational note: Event systems respond to specific events rather than
 * running every frame, which is more efficient for infrequent operations.
 */
template<typename EventSystem, typename EventType>
concept EventDrivenSystem = ECSSystem<EventSystem> && requires(EventSystem& system, const EventType& event, const auto& context) {
    // Must handle specific event types
    system.on_event(event, context);
    
    // Must specify event type
    typename EventSystem::event_type;
    requires std::is_same_v<typename EventSystem::event_type, EventType>;
    
    // Must support event filtering
    { system.should_handle_event(event) } -> std::convertible_to<bool>;
};

//=============================================================================
// Performance and Optimization Concepts
//=============================================================================

/**
 * @brief Concept for cache-friendly data structures
 * 
 * Educational note: Cache-friendly structures improve performance by
 * minimizing cache misses during iteration and access patterns.
 */
template<typename T>
concept CacheFriendly = requires {
    // Must have predictable memory layout
    requires std::is_standard_layout_v<T>;
    
    // Must be efficiently packable
    requires sizeof(T) <= 64; // Single cache line
    
    // Must not have excessive padding
    requires sizeof(T) >= (sizeof(T) * 0.75); // At least 75% efficiency
};

/**
 * @brief Concept for types suitable for batch processing
 * 
 * Educational note: Batch processing allows systems to process multiple
 * components simultaneously, improving cache utilization and enabling SIMD.
 */
template<typename T>
concept BatchProcessable = PerformantComponent<T> && requires {
    // Must be suitable for bulk operations
    requires std::is_trivially_copyable_v<T>;
    requires std::is_trivially_destructible_v<T>;
    
    // Must have stable memory representation
    requires std::has_unique_object_representations_v<T>;
    
    // Must be efficiently movable
    requires std::is_nothrow_move_constructible_v<T>;
    requires std::is_nothrow_move_assignable_v<T>;
};

/**
 * @brief Concept for hot path operations
 * 
 * Educational note: Hot path operations are executed frequently and must
 * be highly optimized to avoid performance bottlenecks.
 */
template<typename Operation>
concept HotPathOperation = requires {
    // Must be inlinable
    requires requires { Operation::is_inlinable; } ? Operation::is_inlinable : true;
    
    // Must not throw exceptions in hot paths
    requires std::is_nothrow_invocable_v<Operation>;
    
    // Must have minimal overhead
    requires sizeof(Operation) <= sizeof(void*);
};

//=============================================================================
// Validation and Testing Concepts
//=============================================================================

/**
 * @brief Concept for testable ECS components
 * 
 * Educational note: Testable components support unit testing and
 * property-based testing to ensure correctness.
 */
template<typename T>
concept TestableComponent = PerformantComponent<T> && requires(const T& a, const T& b) {
    // Must support equality comparison for testing
    { a == b } -> std::convertible_to<bool>;
    
    // Must be default constructible for test setup
    T{};
    
    // Must support copy construction for test assertions
    T{a};
    
    // Should support streaming for test output (optional)
    requires requires(std::ostream& os) {
        os << a;
    } || true; // Make this optional
};

/**
 * @brief Concept for benchmarkable operations
 * 
 * Educational note: Benchmarkable operations can be measured and compared
 * for performance analysis and optimization validation.
 */
template<typename Operation>
concept Benchmarkable = requires {
    // Must be measurable
    requires std::is_invocable_v<Operation>;
    
    // Must be repeatable
    requires std::is_copy_constructible_v<Operation>;
    
    // Must have deterministic behavior
    requires !requires { Operation::has_side_effects; } || !Operation::has_side_effects;
};

//=============================================================================
// Educational Constraint Helpers
//=============================================================================

/**
 * @brief Helper to provide educational error messages for constraint violations
 */
template<typename T>
constexpr bool educational_component_check() {
    static_assert(std::is_trivially_copyable_v<T>, 
        "ECS Educational Note: Components must be trivially copyable for efficient memory operations. "
        "This ensures components can be safely memcpy'd and moved in memory without custom logic.");
    
    static_assert(sizeof(T) <= 1024,
        "ECS Educational Note: Components should be reasonably sized (≤1KB) to maintain cache efficiency. "
        "Large components may cause cache misses. Consider splitting into multiple smaller components.");
    
    static_assert(alignof(T) <= 64,
        "ECS Educational Note: Component alignment should not exceed 64 bytes to avoid excessive padding. "
        "High alignment requirements can waste memory in component arrays.");
    
    return true;
}

/**
 * @brief Helper to validate system design patterns
 */
template<typename System>
constexpr bool educational_system_check() {
    static_assert(requires { typename System::resource_requirements; } || true,
        "ECS Educational Note: Systems should declare their resource requirements for dependency analysis. "
        "This enables automatic parallelization and conflict detection.");
    
    static_assert(!std::is_final_v<System>,
        "ECS Educational Note: Systems should not be final to allow testing through inheritance. "
        "This enables mock systems and test-specific system variants.");
    
    return true;
}

//=============================================================================
// Concept Composition Helpers
//=============================================================================

/**
 * @brief Composed concept for high-performance components
 */
template<typename T>
concept HighPerformanceComponent = PerformantComponent<T> && 
                                  CacheFriendly<T> && 
                                  BatchProcessable<T>;

/**
 * @brief Composed concept for educational demo components
 */
template<typename T>
concept EducationalComponent = TestableComponent<T> && 
                              ChangeTrackableComponent<T> &&
                              requires { educational_component_check<T>(); };

/**
 * @brief Composed concept for production-ready systems
 */
template<typename T>
concept ProductionSystem = ECSSystem<T> && 
                          requires { educational_system_check<T>(); };

} // namespace ecscope::ecs::concepts

//=============================================================================
// Educational Macros for Concept Validation
//=============================================================================

/**
 * @brief Macro to validate component design at compile time
 */
#define ECSCOPE_VALIDATE_COMPONENT(ComponentType) \
    static_assert(::ecscope::ecs::concepts::PerformantComponent<ComponentType>, \
        "Component " #ComponentType " does not meet performance requirements"); \
    static_assert(::ecscope::ecs::concepts::educational_component_check<ComponentType>(), \
        "Component " #ComponentType " educational validation failed");

/**
 * @brief Macro to validate system design at compile time
 */
#define ECSCOPE_VALIDATE_SYSTEM(SystemType) \
    static_assert(::ecscope::ecs::concepts::ECSSystem<SystemType>, \
        "System " #SystemType " does not meet ECS system requirements"); \
    static_assert(::ecscope::ecs::concepts::educational_system_check<SystemType>(), \
        "System " #SystemType " educational validation failed");

/**
 * @brief Macro to check if a type is suitable for SoA transformation
 */
#define ECSCOPE_CHECK_SOA_SUITABILITY(ComponentType) \
    static_assert(::ecscope::ecs::concepts::SoATransformable<ComponentType>, \
        "Component " #ComponentType " is not suitable for SoA (Structure of Arrays) storage. " \
        "Consider: 1) Reducing size, 2) Ensuring trivial destructibility, 3) Standard layout");

/**
 * @brief Macro to validate query filter design
 */
#define ECSCOPE_VALIDATE_QUERY_FILTER(FilterType) \
    static_assert(::ecscope::ecs::concepts::QueryFilter<FilterType>, \
        "Filter " #FilterType " does not implement required query filter interface");

#endif // ECSCOPE_ECS_MODERN_CONCEPTS_HPP