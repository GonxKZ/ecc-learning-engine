#pragma once

#include "../core/types.hpp"
#include "../core/memory.hpp"
#include "../core/platform.hpp"
#include "concepts.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "storage.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string_view>
#include <chrono>
#include <functional>
#include <algorithm>
#include <mutex>

/**
 * @file system.hpp
 * @brief System base classes with lifecycle management
 * 
 * This file implements a comprehensive system framework with:
 * - System lifecycle management (initialize, update, shutdown)
 * - Dependency declaration and resolution
 * - Performance monitoring integration
 * - Resource management
 * - Thread-safe system execution
 * - System priority and phase scheduling
 * - Query-based entity iteration
 * 
 * Educational Notes:
 * - Systems operate on entities that match specific component patterns
 * - System dependencies ensure proper execution order
 * - Performance monitoring helps identify bottlenecks
 * - Resource management prevents leaks and provides cleanup
 * - Thread safety allows parallel system execution where safe
 * - Query objects abstract complex entity filtering logic
 */

namespace ecscope::foundation {

using namespace ecscope::core;
using namespace ecscope::core::platform;

/// @brief Base system interface
class ISystem {
public:
    /// @brief System lifecycle states
    enum class State {
        Created,      ///< System created but not initialized
        Initialized,  ///< System initialized and ready
        Running,      ///< System actively running
        Paused,       ///< System paused (not updating)
        Shutdown      ///< System shut down
    };
    
    /// @brief System execution statistics
    struct ExecutionStats {
        std::uint64_t update_count{0};
        std::uint64_t total_time_ns{0};
        std::uint64_t min_time_ns{UINT64_MAX};
        std::uint64_t max_time_ns{0};
        std::uint64_t last_time_ns{0};
        
        double average_time_ms() const noexcept {
            return update_count > 0 ? 
                (static_cast<double>(total_time_ns) / update_count) / 1'000'000.0 : 0.0;
        }
        
        void record_execution(std::uint64_t time_ns) noexcept {
            ++update_count;
            total_time_ns += time_ns;
            min_time_ns = std::min(min_time_ns, time_ns);
            max_time_ns = std::max(max_time_ns, time_ns);
            last_time_ns = time_ns;
        }
        
        void reset() noexcept {
            *this = ExecutionStats{};
        }
    };
    
    virtual ~ISystem() = default;
    
    /// @brief Get system name
    virtual std::string_view name() const = 0;
    
    /// @brief Get system ID
    virtual SystemId id() const = 0;
    
    /// @brief Get system priority
    virtual SystemPriority priority() const = 0;
    
    /// @brief Get system execution phase
    virtual SystemPhase phase() const = 0;
    
    /// @brief Get current state
    virtual State state() const = 0;
    
    /// @brief Initialize system
    virtual void initialize() = 0;
    
    /// @brief Update system
    virtual void update(float delta_time) = 0;
    
    /// @brief Shutdown system
    virtual void shutdown() = 0;
    
    /// @brief Pause system execution
    virtual void pause() = 0;
    
    /// @brief Resume system execution
    virtual void resume() = 0;
    
    /// @brief Get system dependencies (systems that must run before this one)
    virtual std::vector<SystemId> dependencies() const = 0;
    
    /// @brief Get system conflicts (systems that cannot run concurrently)
    virtual std::vector<SystemId> conflicts() const = 0;
    
    /// @brief Check if system can run in parallel with others
    virtual bool is_thread_safe() const = 0;
    
    /// @brief Get execution statistics
    virtual ExecutionStats get_stats() const = 0;
    
    /// @brief Reset execution statistics
    virtual void reset_stats() = 0;
    
    /// @brief Get required component signature
    virtual ComponentSignature required_components() const = 0;
    
    /// @brief Get excluded component signature
    virtual ComponentSignature excluded_components() const = 0;
};

/// @brief Base system implementation
template<System T>
class SystemBase : public ISystem {
public:
    /// @brief System configuration
    struct Config {
        std::string_view name{};
        SystemPriority priority{SystemPriority::Normal};
        SystemPhase phase{SystemPhase::Update};
        bool thread_safe{false};
        bool enable_profiling{true};
    };
    
    explicit SystemBase(const Config& config)
        : config_(config)
        , id_(generate_system_id())
        , state_(State::Created) {
    }
    
    virtual ~SystemBase() {
        if (state_ != State::Shutdown) {
            shutdown();
        }
    }
    
    // ISystem interface
    std::string_view name() const override { return config_.name; }
    SystemId id() const override { return id_; }
    SystemPriority priority() const override { return config_.priority; }
    SystemPhase phase() const override { return config_.phase; }
    State state() const override { return state_; }
    bool is_thread_safe() const override { return config_.thread_safe; }
    
    void initialize() override {
        if (state_ != State::Created) {
            return;
        }
        
        HighResolutionTimer timer;
        
        try {
            on_initialize();
            state_ = State::Initialized;
        } catch (...) {
            state_ = State::Shutdown;
            throw;
        }
        
        if (config_.enable_profiling) {
            initialization_time_ns_ = timer.elapsed_nanoseconds();
        }
    }
    
    void update(float delta_time) override {
        if (state_ != State::Initialized && state_ != State::Running) {
            return;
        }
        
        HighResolutionTimer timer;
        
        state_ = State::Running;
        on_update(delta_time);
        
        if (config_.enable_profiling) {
            stats_.record_execution(timer.elapsed_nanoseconds());
        }
    }
    
    void shutdown() override {
        if (state_ == State::Shutdown) {
            return;
        }
        
        try {
            on_shutdown();
        } catch (...) {
            // Log error but don't throw during shutdown
        }
        
        state_ = State::Shutdown;
    }
    
    void pause() override {
        if (state_ == State::Running) {
            state_ = State::Paused;
            on_pause();
        }
    }
    
    void resume() override {
        if (state_ == State::Paused) {
            state_ = State::Running;
            on_resume();
        }
    }
    
    ExecutionStats get_stats() const override { return stats_; }
    void reset_stats() override { stats_.reset(); }
    
    std::vector<SystemId> dependencies() const override {
        return dependencies_;
    }
    
    std::vector<SystemId> conflicts() const override {
        return conflicts_;
    }
    
    ComponentSignature required_components() const override {
        return required_components_;
    }
    
    ComponentSignature excluded_components() const override {
        return excluded_components_;
    }
    
    /// @brief Get initialization time
    std::uint64_t initialization_time_ns() const { return initialization_time_ns_; }

protected:
    /// @brief Override to implement system initialization
    virtual void on_initialize() {}
    
    /// @brief Override to implement system update logic
    virtual void on_update(float delta_time) = 0;
    
    /// @brief Override to implement system shutdown
    virtual void on_shutdown() {}
    
    /// @brief Override to implement system pause behavior
    virtual void on_pause() {}
    
    /// @brief Override to implement system resume behavior
    virtual void on_resume() {}
    
    /// @brief Add system dependency
    void add_dependency(SystemId dependency) {
        dependencies_.push_back(dependency);
    }
    
    /// @brief Add system conflict
    void add_conflict(SystemId conflict) {
        conflicts_.push_back(conflict);
    }
    
    /// @brief Set required components
    void set_required_components(ComponentSignature signature) {
        required_components_ = signature;
    }
    
    /// @brief Set excluded components
    void set_excluded_components(ComponentSignature signature) {
        excluded_components_ = signature;
    }
    
    /// @brief Add required component type
    template<Component ComponentType>
    void require_component() {
        const auto id = component_utils::get_component_id<ComponentType>();
        required_components_ = ComponentRegistry::add_component_to_signature(required_components_, id);
    }
    
    /// @brief Add excluded component type
    template<Component ComponentType>
    void exclude_component() {
        const auto id = component_utils::get_component_id<ComponentType>();
        excluded_components_ = ComponentRegistry::add_component_to_signature(excluded_components_, id);
    }

private:
    static SystemId generate_system_id() {
        static std::atomic<std::uint16_t> next_id{0};
        return SystemId{next_id++};
    }
    
    Config config_;
    SystemId id_;
    State state_;
    ExecutionStats stats_;
    std::uint64_t initialization_time_ns_{0};
    
    std::vector<SystemId> dependencies_;
    std::vector<SystemId> conflicts_;
    ComponentSignature required_components_{0};
    ComponentSignature excluded_components_{0};
};

/// @brief Query-based system for entity processing
template<System T>
class QuerySystem : public SystemBase<T> {
public:
    using Base = SystemBase<T>;
    using Config = typename Base::Config;
    
    explicit QuerySystem(const Config& config) : Base(config) {}
    
    /// @brief Entity query interface
    class EntityQuery {
    public:
        virtual ~EntityQuery() = default;
        virtual std::span<const EntityHandle> get_entities() = 0;
        virtual bool matches_entity(EntityHandle entity) = 0;
        virtual ComponentSignature get_required_signature() const = 0;
        virtual ComponentSignature get_excluded_signature() const = 0;
    };
    
    /// @brief Simple component query
    template<Component... Required>
    class ComponentQuery : public EntityQuery {
    public:
        ComponentQuery() {
            required_signature_ = component_utils::create_signature<Required...>();
        }
        
        std::span<const EntityHandle> get_entities() override {
            // This would be implemented with access to the world/registry
            // For now, return empty span
            return {};
        }
        
        bool matches_entity(EntityHandle entity) override {
            // This would check entity's component signature against required signature
            // Implementation depends on world/registry access
            return false;
        }
        
        ComponentSignature get_required_signature() const override {
            return required_signature_;
        }
        
        ComponentSignature get_excluded_signature() const override {
            return excluded_signature_;
        }
        
        /// @brief Add excluded component type
        template<Component ExcludedType>
        ComponentQuery& exclude() {
            const auto id = component_utils::get_component_id<ExcludedType>();
            excluded_signature_ = ComponentRegistry::add_component_to_signature(excluded_signature_, id);
            return *this;
        }

    private:
        ComponentSignature required_signature_{0};
        ComponentSignature excluded_signature_{0};
    };
    
protected:
    /// @brief Process entities matching query
    /// @param query Entity query to process
    /// @param processor Function to call for each matching entity
    template<typename Func>
    void process_entities(EntityQuery& query, Func&& processor) {
        const auto entities = query.get_entities();
        
        for (EntityHandle entity : entities) {
            if (query.matches_entity(entity)) {
                processor(entity);
            }
        }
    }
    
    /// @brief Process entities in batches for better cache performance
    /// @param query Entity query to process
    /// @param processor Function to call for each batch
    /// @param batch_size Number of entities per batch
    template<typename Func>
    void process_entities_batched(EntityQuery& query, Func&& processor, std::uint32_t batch_size = 64) {
        const auto entities = query.get_entities();
        
        for (std::size_t i = 0; i < entities.size(); i += batch_size) {
            const auto batch_end = std::min(i + batch_size, entities.size());
            const auto batch = entities.subspan(i, batch_end - i);
            
            // Prefetch next batch
            if (batch_end < entities.size()) {
                const auto next_batch_size = std::min(batch_size, entities.size() - batch_end);
                for (std::uint32_t j = 0; j < next_batch_size; ++j) {
                    prefetch_read(&entities[batch_end + j]);
                }
            }
            
            processor(batch);
        }
    }
};

/// @brief System scheduler for managing system execution
class SystemScheduler {
public:
    /// @brief Scheduler configuration
    struct Config {
        bool enable_parallel_execution{true};
        std::uint32_t max_worker_threads{0};  // 0 = hardware_concurrency
        bool enable_profiling{true};
        bool strict_dependencies{true};
    };
    
    explicit SystemScheduler(const Config& config = {});
    
    /// @brief Register a system
    void register_system(std::unique_ptr<ISystem> system);
    
    /// @brief Unregister a system
    bool unregister_system(SystemId id);
    
    /// @brief Initialize all systems
    void initialize_systems();
    
    /// @brief Update all systems for one frame
    void update_systems(float delta_time);
    
    /// @brief Shutdown all systems
    void shutdown_systems();
    
    /// @brief Pause all systems
    void pause_systems();
    
    /// @brief Resume all systems
    void resume_systems();
    
    /// @brief Get system by ID
    ISystem* get_system(SystemId id) const;
    
    /// @brief Get system by type
    template<typename T>
    T* get_system() const {
        for (auto& [phase_systems] : systems_by_phase_) {
            for (auto& system : phase_systems.second) {
                if (auto* typed = dynamic_cast<T*>(system.get())) {
                    return typed;
                }
            }
        }
        return nullptr;
    }
    
    /// @brief Get systems by phase
    std::vector<ISystem*> get_systems_by_phase(SystemPhase phase) const;
    
    /// @brief Get execution statistics
    struct SchedulerStats {
        std::uint64_t frame_count{0};
        std::uint64_t total_frame_time_ns{0};
        std::uint64_t min_frame_time_ns{UINT64_MAX};
        std::uint64_t max_frame_time_ns{0};
        std::uint64_t last_frame_time_ns{0};
        std::uint32_t active_systems{0};
        std::uint32_t parallel_executions{0};
        
        double average_frame_time_ms() const noexcept {
            return frame_count > 0 ? 
                (static_cast<double>(total_frame_time_ns) / frame_count) / 1'000'000.0 : 0.0;
        }
    };
    
    SchedulerStats get_stats() const { return stats_; }
    void reset_stats() { stats_ = SchedulerStats{}; }

private:
    /// @brief Build system execution order based on dependencies
    void build_execution_order();
    
    /// @brief Check for circular dependencies
    bool has_circular_dependencies() const;
    
    /// @brief Execute systems in a phase
    void execute_phase(SystemPhase phase, float delta_time);
    
    /// @brief Execute systems in parallel where possible
    void execute_parallel(std::vector<ISystem*>& systems, float delta_time);
    
    Config config_;
    
    /// @brief Systems organized by execution phase
    std::unordered_map<SystemPhase, std::vector<std::unique_ptr<ISystem>>> systems_by_phase_;
    
    /// @brief System lookup by ID
    std::unordered_map<SystemId, ISystem*> system_lookup_;
    
    /// @brief Execution order for each phase
    std::unordered_map<SystemPhase, std::vector<ISystem*>> execution_order_;
    
    /// @brief Thread safety
    mutable std::mutex mutex_;
    
    /// @brief Statistics
    SchedulerStats stats_;
    
    /// @brief System state
    bool initialized_{false};
    bool running_{false};
};

/// @brief System registration helper
template<System T>
class SystemRegistrar {
public:
    template<typename... Args>
    static void register_system(SystemScheduler& scheduler, Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        scheduler.register_system(std::move(system));
    }
};

/// @brief Convenience macro for system registration
#define ECSCOPE_REGISTER_SYSTEM(SystemType, Scheduler, ...) \
    ::ecscope::foundation::SystemRegistrar<SystemType>::register_system(Scheduler, __VA_ARGS__)

}  // namespace ecscope::foundation