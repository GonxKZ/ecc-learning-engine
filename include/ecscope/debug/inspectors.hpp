#pragma once

#include "debug_system.hpp"
#include "profilers.hpp"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <any>

namespace ECScope::Debug {

// Forward declarations
class ECSRegistry;
class World;
class AssetManager;

/**
 * @brief Entity inspector with live component editing
 */
class EntityInspector : public Inspector {
public:
    struct ComponentInfo {
        std::string name;
        std::type_index type_id;
        void* data;
        size_t size;
        bool is_const;
        std::function<void()> render_func;
        std::function<bool()> edit_func;
    };
    
    struct EntityInfo {
        uint32_t id;
        std::string name;
        bool active;
        std::vector<ComponentInfo> components;
        std::vector<uint32_t> children;
        uint32_t parent = 0;
        
        // Metadata
        size_t memory_footprint = 0;
        std::string archetype;
        uint32_t archetype_index = 0;
    };

    explicit EntityInspector(const std::string& name);
    ~EntityInspector() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Entity selection
    void SelectEntity(uint32_t entity_id);
    void ClearSelection();
    uint32_t GetSelectedEntity() const { return m_selected_entity; }
    
    // Component editing
    template<typename T>
    void RegisterComponentEditor(std::function<bool(T&)> edit_func);
    
    template<typename T>
    void RegisterComponentRenderer(std::function<void(const T&)> render_func);

    // ECS integration
    void SetECSRegistry(std::shared_ptr<void> registry) { m_ecs_registry = registry; }
    void RefreshEntityData();

private:
    uint32_t m_selected_entity = 0;
    EntityInfo m_current_entity;
    std::shared_ptr<void> m_ecs_registry;
    
    // Component editor registry
    std::unordered_map<std::type_index, std::function<bool(void*)>> m_component_editors;
    std::unordered_map<std::type_index, std::function<void(const void*)>> m_component_renderers;
    
    // UI state
    bool m_show_memory_info = true;
    bool m_show_archetype_info = true;
    bool m_show_relationships = true;
    std::string m_search_filter;
    
    void RenderEntitySelector();
    void RenderEntityInfo();
    void RenderComponentList();
    void RenderComponentEditor(ComponentInfo& component);
    void RenderEntityHierarchy();
    void RenderEntityMemoryInfo();
    
    void PopulateEntityInfo(uint32_t entity_id);
    std::vector<uint32_t> GetAllEntities();
    std::string GetEntityName(uint32_t entity_id);
};

/**
 * @brief System performance inspector with timing analysis
 */
class SystemInspector : public Inspector {
public:
    struct SystemInfo {
        std::string name;
        std::type_index type_id;
        bool enabled;
        uint32_t execution_order;
        
        // Performance data
        double last_update_time_ms = 0.0;
        double average_update_time_ms = 0.0;
        double total_time_ms = 0.0;
        size_t update_count = 0;
        
        // Resource usage
        size_t entities_processed = 0;
        size_t memory_allocated = 0;
        
        // Dependencies
        std::vector<std::string> dependencies;
        std::vector<std::string> dependents;
        
        // Query information
        std::string query_signature;
        size_t matched_entities = 0;
    };
    
    struct SystemGroupInfo {
        std::string name;
        std::vector<std::string> systems;
        double total_time_ms = 0.0;
        bool parallel_execution = false;
    };

    explicit SystemInspector(const std::string& name, std::shared_ptr<CPUProfiler> profiler);
    ~SystemInspector() override;

    void Update(float deltaTime) override;
    void Render() override;

    // System management
    void RegisterSystem(const std::string& name, std::type_index type_id);
    void UnregisterSystem(const std::string& name);
    void UpdateSystemInfo(const std::string& name, const SystemInfo& info);
    
    // System groups
    void RegisterSystemGroup(const std::string& group_name, const std::vector<std::string>& systems);
    void SetSystemGroupParallel(const std::string& group_name, bool parallel);

private:
    std::shared_ptr<CPUProfiler> m_profiler;
    std::unordered_map<std::string, SystemInfo> m_systems;
    std::unordered_map<std::string, SystemGroupInfo> m_system_groups;
    
    std::string m_selected_system;
    bool m_show_performance_graph = true;
    bool m_show_dependency_graph = true;
    bool m_show_only_active = false;
    
    void RenderSystemList();
    void RenderSystemDetails();
    void RenderPerformanceGraph();
    void RenderDependencyGraph();
    void RenderSystemControls();
    
    std::vector<float> GetSystemPerformanceHistory(const std::string& system_name);
};

/**
 * @brief Asset inspector with dependency graphs
 */
class AssetInspector : public Inspector {
public:
    struct AssetInfo {
        std::string path;
        std::string type;
        std::string status; // Loading, Loaded, Failed, Cached
        
        size_t file_size = 0;
        size_t memory_size = 0;
        size_t ref_count = 0;
        
        double load_time_ms = 0.0;
        std::chrono::system_clock::time_point last_accessed;
        std::chrono::system_clock::time_point created_time;
        
        // Dependencies
        std::vector<std::string> dependencies;
        std::vector<std::string> dependents;
        
        // Metadata
        std::unordered_map<std::string, std::string> metadata;
        std::string error_message;
    };
    
    enum class AssetFilter {
        All,
        Loaded,
        Loading,
        Failed,
        Unused,
        HighMemory,
        SlowLoading
    };

    explicit AssetInspector(const std::string& name, std::shared_ptr<AssetProfiler> profiler);
    ~AssetInspector() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Asset management
    void RegisterAsset(const std::string& path, const AssetInfo& info);
    void UpdateAssetInfo(const std::string& path, const AssetInfo& info);
    void RemoveAsset(const std::string& path);
    
    // Asset system integration
    void SetAssetManager(std::shared_ptr<void> asset_manager) { m_asset_manager = asset_manager; }

private:
    std::shared_ptr<AssetProfiler> m_profiler;
    std::shared_ptr<void> m_asset_manager;
    std::unordered_map<std::string, AssetInfo> m_assets;
    
    std::string m_selected_asset;
    AssetFilter m_current_filter = AssetFilter::All;
    std::string m_search_query;
    bool m_show_dependency_graph = true;
    bool m_show_memory_usage = true;
    
    void RenderAssetList();
    void RenderAssetDetails();
    void RenderDependencyGraph();
    void RenderAssetFilters();
    void RenderMemoryUsage();
    void RenderLoadingPerformance();
    
    std::vector<std::string> GetFilteredAssets() const;
    bool PassesFilter(const AssetInfo& asset) const;
};

/**
 * @brief Memory inspector with allocation trees
 */
class MemoryInspector : public Inspector {
public:
    struct AllocationNode {
        void* address;
        size_t size;
        std::string tag;
        std::string callstack;
        std::chrono::system_clock::time_point timestamp;
        
        // Tree structure
        std::vector<std::unique_ptr<AllocationNode>> children;
        AllocationNode* parent = nullptr;
        
        // Analysis
        bool is_leak = false;
        bool is_large_allocation = false;
        size_t total_children_size = 0;
    };
    
    struct MemoryPool {
        std::string name;
        size_t total_size = 0;
        size_t used_size = 0;
        size_t free_size = 0;
        size_t block_count = 0;
        size_t free_blocks = 0;
        double fragmentation = 0.0;
        
        std::vector<std::pair<size_t, bool>> blocks; // size, is_free
    };

    explicit MemoryInspector(const std::string& name, std::shared_ptr<MemoryProfiler> profiler);
    ~MemoryInspector() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Memory pool management
    void RegisterMemoryPool(const std::string& name, const MemoryPool& pool);
    void UpdateMemoryPool(const std::string& name, const MemoryPool& pool);

private:
    std::shared_ptr<MemoryProfiler> m_profiler;
    std::unique_ptr<AllocationNode> m_allocation_tree;
    std::unordered_map<std::string, MemoryPool> m_memory_pools;
    
    enum class ViewMode {
        AllocationTree,
        MemoryPools,
        LeakDetection,
        MemoryMap,
        Statistics
    } m_view_mode = ViewMode::AllocationTree;
    
    std::string m_selected_tag;
    size_t m_min_allocation_size = 0;
    bool m_show_callstacks = true;
    bool m_group_by_tag = true;
    
    void RenderViewModeSelector();
    void RenderAllocationTree();
    void RenderMemoryPools();
    void RenderLeakDetection();
    void RenderMemoryMap();
    void RenderStatistics();
    
    void BuildAllocationTree();
    void RenderAllocationNode(const AllocationNode& node, int depth = 0);
    void RenderMemoryPool(const std::string& name, const MemoryPool& pool);
};

/**
 * @brief Shader inspector with reflection data
 */
class ShaderInspector : public Inspector {
public:
    struct ShaderInfo {
        std::string name;
        std::string vertex_path;
        std::string fragment_path;
        std::string geometry_path;
        std::string compute_path;
        
        bool compiled = false;
        bool linked = false;
        std::string compile_log;
        std::string link_log;
        
        // Reflection data
        struct Uniform {
            std::string name;
            std::string type;
            int location;
            size_t size;
            std::any default_value;
            std::any current_value;
        };
        
        struct Attribute {
            std::string name;
            std::string type;
            int location;
            size_t size;
        };
        
        std::vector<Uniform> uniforms;
        std::vector<Attribute> attributes;
        
        // Performance data
        size_t instruction_count = 0;
        size_t texture_samples = 0;
        double compile_time_ms = 0.0;
        double average_gpu_time_ms = 0.0;
    };

    explicit ShaderInspector(const std::string& name);
    ~ShaderInspector() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Shader management
    void RegisterShader(const std::string& name, const ShaderInfo& info);
    void UpdateShaderInfo(const std::string& name, const ShaderInfo& info);
    void RemoveShader(const std::string& name);

private:
    std::unordered_map<std::string, ShaderInfo> m_shaders;
    std::string m_selected_shader;
    
    bool m_show_source_code = true;
    bool m_show_uniforms = true;
    bool m_show_attributes = true;
    bool m_show_performance = true;
    
    void RenderShaderList();
    void RenderShaderDetails();
    void RenderShaderSource();
    void RenderUniformEditor();
    void RenderShaderPerformance();
    
    void RenderUniformValue(ShaderInfo::Uniform& uniform);
    std::string GetShaderSource(const std::string& path);
};

/**
 * @brief Job system inspector with fiber states
 */
class JobSystemInspector : public Inspector {
public:
    struct WorkerThread {
        uint32_t id;
        std::string name;
        bool active = false;
        size_t jobs_completed = 0;
        size_t jobs_stolen = 0;
        double cpu_utilization = 0.0;
        
        std::string current_job;
        double job_start_time = 0.0;
    };
    
    struct JobInfo {
        uint32_t id;
        std::string name;
        std::string category;
        
        enum class Status {
            Pending,
            Running,
            Completed,
            Failed,
            Cancelled
        } status = Status::Pending;
        
        uint32_t worker_id = 0;
        uint32_t priority = 0;
        
        double submit_time = 0.0;
        double start_time = 0.0;
        double end_time = 0.0;
        
        std::vector<uint32_t> dependencies;
        std::vector<uint32_t> dependents;
        
        size_t memory_usage = 0;
        std::string error_message;
    };
    
    struct FiberInfo {
        uint32_t id;
        std::string name;
        
        enum class State {
            Ready,
            Running,
            Waiting,
            Suspended,
            Completed
        } state = State::Ready;
        
        void* stack_ptr = nullptr;
        size_t stack_size = 0;
        uint32_t worker_id = 0;
        
        std::string wait_reason;
        double switch_count = 0;
    };

    explicit JobSystemInspector(const std::string& name);
    ~JobSystemInspector() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Data updates
    void UpdateWorkerThreads(const std::vector<WorkerThread>& threads);
    void UpdateJobs(const std::vector<JobInfo>& jobs);
    void UpdateFibers(const std::vector<FiberInfo>& fibers);

private:
    std::vector<WorkerThread> m_worker_threads;
    std::vector<JobInfo> m_jobs;
    std::vector<FiberInfo> m_fibers;
    
    enum class ViewMode {
        Overview,
        WorkerThreads,
        JobQueue,
        FiberStates,
        Dependencies,
        Performance
    } m_view_mode = ViewMode::Overview;
    
    uint32_t m_selected_job = 0;
    uint32_t m_selected_fiber = 0;
    
    void RenderViewModeSelector();
    void RenderOverview();
    void RenderWorkerThreads();
    void RenderJobQueue();
    void RenderFiberStates();
    void RenderDependencyGraph();
    void RenderPerformanceMetrics();
    
    void RenderJobInfo(const JobInfo& job);
    void RenderFiberInfo(const FiberInfo& fiber);
    void RenderWorkerInfo(const WorkerThread& worker);
};

// Template implementations
template<typename T>
void EntityInspector::RegisterComponentEditor(std::function<bool(T&)> edit_func) {
    m_component_editors[std::type_index(typeid(T))] = [edit_func](void* data) {
        return edit_func(*static_cast<T*>(data));
    };
}

template<typename T>
void EntityInspector::RegisterComponentRenderer(std::function<void(const T&)> render_func) {
    m_component_renderers[std::type_index(typeid(T))] = [render_func](const void* data) {
        render_func(*static_cast<const T*>(data));
    };
}

} // namespace ECScope::Debug