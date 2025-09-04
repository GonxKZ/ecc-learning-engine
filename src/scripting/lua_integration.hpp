#pragma once

/**
 * @file scripting/lua_integration.hpp
 * @brief Advanced LuaJIT Integration for ECScope ECS Engine
 * 
 * This system provides high-performance Lua scripting with educational focus:
 * 
 * Key Features:
 * - LuaJIT integration for maximum performance (JIT compilation)
 * - Lua coroutine support for complex game logic and AI
 * - Automatic binding generation for ECS components and systems
 * - Custom memory allocator integration with ECScope
 * - Hot-reload support with state preservation
 * - Advanced debugging and profiling tools
 * - Educational visualization of script execution
 * 
 * Architecture:
 * - Sol2-style binding generation with type safety
 * - Coroutine scheduler integrated with job system
 * - Custom Lua allocator using ECScope memory pools
 * - Stack trace analysis and debugging support
 * - Performance monitoring with flame graph generation
 * 
 * Performance Benefits:
 * - LuaJIT provides near-C performance for hot paths
 * - Zero-allocation function calls for frequent operations
 * - Vectorized operations through FFI integration
 * - Memory pool allocation reduces GC pressure
 * - Coroutine-based async operations
 * 
 * Educational Features:
 * - Real-time performance profiling with hotspot analysis
 * - Memory usage visualization and leak detection
 * - Coroutine state visualization
 * - Interactive debugging with live state inspection
 * - Bytecode analysis and optimization suggestions
 * 
 * @author ECScope Educational ECS Framework - Scripting System  
 * @date 2025
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "ecs/entity.hpp"
#include "ecs/component.hpp"
#include "memory/lockfree_allocators.hpp"
#include "job_system/work_stealing_job_system.hpp"
#include <lua.hpp>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <span>
#include <queue>
#include <coroutine>

namespace ecscope::scripting::lua {

//=============================================================================
// Forward Declarations
//=============================================================================

class LuaEngine;
class LuaModule;
class LuaCoroutineScheduler;
class ComponentBinding;
class SystemBinding;
class LuaProfiler;
class LuaDebugger;

//=============================================================================
// Lua State Management
//=============================================================================

/**
 * @brief RAII wrapper for Lua state with automatic cleanup
 */
class LuaStateWrapper {
private:
    lua_State* L_;
    bool owns_state_;
    
public:
    explicit LuaStateWrapper(lua_State* L = nullptr, bool owns = true) 
        : L_(L), owns_state_(owns) {
        if (!L_ && owns) {
            L_ = luaL_newstate();
        }
    }
    
    ~LuaStateWrapper() {
        if (L_ && owns_state_) {
            lua_close(L_);
        }
    }
    
    // Non-copyable but movable
    LuaStateWrapper(const LuaStateWrapper&) = delete;
    LuaStateWrapper& operator=(const LuaStateWrapper&) = delete;
    
    LuaStateWrapper(LuaStateWrapper&& other) noexcept 
        : L_(other.L_), owns_state_(other.owns_state_) {
        other.L_ = nullptr;
        other.owns_state_ = false;
    }
    
    LuaStateWrapper& operator=(LuaStateWrapper&& other) noexcept {
        if (this != &other) {
            if (L_ && owns_state_) {
                lua_close(L_);
            }
            L_ = other.L_;
            owns_state_ = other.owns_state_;
            other.L_ = nullptr;
            other.owns_state_ = false;
        }
        return *this;
    }
    
    lua_State* get() const noexcept { return L_; }
    lua_State* release() noexcept {
        owns_state_ = false;
        return L_;
    }
    
    bool is_valid() const noexcept { return L_ != nullptr; }
    operator bool() const noexcept { return is_valid(); }
    
    // Stack management helpers
    struct StackGuard {
        lua_State* L;
        int initial_top;
        
        StackGuard(lua_State* state) : L(state), initial_top(lua_gettop(L)) {}
        ~StackGuard() { lua_settop(L, initial_top); }
    };
    
    StackGuard create_stack_guard() { return StackGuard{L_}; }
    
    // Common operations
    bool load_string(const std::string& code, const std::string& name = "chunk") {
        return luaL_loadbuffer(L_, code.c_str(), code.length(), name.c_str()) == LUA_OK;
    }
    
    bool load_file(const std::string& filepath) {
        return luaL_loadfile(L_, filepath.c_str()) == LUA_OK;
    }
    
    bool pcall(int nargs = 0, int nresults = LUA_MULTRET) {
        return lua_pcall(L_, nargs, nresults, 0) == LUA_OK;
    }
    
    std::string get_error() {
        if (lua_isstring(L_, -1)) {
            std::string error = lua_tostring(L_, -1);
            lua_pop(L_, 1);
            return error;
        }
        return "Unknown Lua error";
    }
    
    // Type checking and conversion
    bool is_nil(int index = -1) const { return lua_isnil(L_, index); }
    bool is_boolean(int index = -1) const { return lua_isboolean(L_, index); }
    bool is_number(int index = -1) const { return lua_isnumber(L_, index); }
    bool is_string(int index = -1) const { return lua_isstring(L_, index); }
    bool is_table(int index = -1) const { return lua_istable(L_, index); }
    bool is_function(int index = -1) const { return lua_isfunction(L_, index); }
    bool is_userdata(int index = -1) const { return lua_isuserdata(L_, index); }
    
    bool to_boolean(int index = -1) const { return lua_toboolean(L_, index); }
    lua_Number to_number(int index = -1) const { return lua_tonumber(L_, index); }
    std::string to_string(int index = -1) const {
        size_t len;
        const char* str = lua_tolstring(L_, index, &len);
        return str ? std::string(str, len) : std::string{};
    }
    
    void push_nil() { lua_pushnil(L_); }
    void push_boolean(bool value) { lua_pushboolean(L_, value); }
    void push_number(lua_Number value) { lua_pushnumber(L_, value); }
    void push_string(const std::string& value) { lua_pushlstring(L_, value.c_str(), value.length()); }
    void push_function(lua_CFunction func) { lua_pushcfunction(L_, func); }
    
    // Global access
    void set_global(const std::string& name) { lua_setglobal(L_, name.c_str()); }
    void get_global(const std::string& name) { lua_getglobal(L_, name.c_str()); }
    
    // Table operations
    void create_table(int narr = 0, int nrec = 0) { lua_createtable(L_, narr, nrec); }
    void set_field(const std::string& key, int table_index = -2) { lua_setfield(L_, table_index, key.c_str()); }
    void get_field(const std::string& key, int table_index = -1) { lua_getfield(L_, table_index, key.c_str()); }
};

//=============================================================================
// Memory Management Integration
//=============================================================================

/**
 * @brief Custom Lua allocator using ECScope memory system
 */
class LuaMemoryManager {
private:
    memory::AdvancedMemorySystem& memory_system_;
    std::atomic<usize> total_allocated_{0};
    std::atomic<usize> total_freed_{0};
    std::atomic<usize> peak_memory_{0};
    
    // Allocation tracking
    struct AllocationInfo {
        usize size;
        std::chrono::high_resolution_clock::time_point timestamp;
        const char* what;  // Lua allocation type description
    };
    
    mutable std::mutex allocation_map_mutex_;
    std::unordered_map<void*, AllocationInfo> allocation_map_;
    
public:
    explicit LuaMemoryManager(memory::AdvancedMemorySystem& memory_system)
        : memory_system_(memory_system) {}
    
    void* allocate(void* ud, void* ptr, size_t osize, size_t nsize) {
        (void)ud;  // Unused user data
        
        if (nsize == 0) {
            // Free operation
            if (ptr) {
                deallocate_impl(ptr, osize);
            }
            return nullptr;
        }
        
        if (ptr == nullptr) {
            // New allocation
            return allocate_impl(nsize, "new");
        }
        
        // Reallocation
        void* new_ptr = allocate_impl(nsize, "realloc");
        if (new_ptr && ptr) {
            std::memcpy(new_ptr, ptr, std::min(osize, nsize));
            deallocate_impl(ptr, osize);
        }
        return new_ptr;
    }
    
    // Statistics
    struct Statistics {
        usize total_allocated;
        usize total_freed;
        usize current_allocated;
        usize peak_memory;
        usize active_allocations;
        f64 memory_efficiency;
    };
    
    Statistics get_statistics() const {
        std::lock_guard<std::mutex> lock(allocation_map_mutex_);
        usize allocated = total_allocated_.load(std::memory_order_relaxed);
        usize freed = total_freed_.load(std::memory_order_relaxed);
        
        return Statistics{
            .total_allocated = allocated,
            .total_freed = freed,
            .current_allocated = allocated - freed,
            .peak_memory = peak_memory_.load(std::memory_order_relaxed),
            .active_allocations = allocation_map_.size(),
            .memory_efficiency = memory_system_.get_fragmentation_ratio()
        };
    }
    
    std::vector<AllocationInfo> get_allocation_report() const {
        std::lock_guard<std::mutex> lock(allocation_map_mutex_);
        std::vector<AllocationInfo> report;
        report.reserve(allocation_map_.size());
        
        for (const auto& [ptr, info] : allocation_map_) {
            report.push_back(info);
        }
        
        return report;
    }

private:
    void* allocate_impl(size_t size, const char* what) {
        void* ptr = memory_system_.allocate(size, alignof(std::max_align_t));
        if (ptr) {
            usize current = total_allocated_.fetch_add(size, std::memory_order_relaxed) + size;
            
            // Update peak memory
            usize peak = peak_memory_.load(std::memory_order_relaxed);
            while (current > peak && 
                   !peak_memory_.compare_exchange_weak(peak, current, std::memory_order_relaxed)) {
                // Retry until successful
            }
            
            // Track allocation
            {
                std::lock_guard<std::mutex> lock(allocation_map_mutex_);
                allocation_map_[ptr] = AllocationInfo{
                    .size = size,
                    .timestamp = std::chrono::high_resolution_clock::now(),
                    .what = what
                };
            }
        }
        return ptr;
    }
    
    void deallocate_impl(void* ptr, size_t size) {
        {
            std::lock_guard<std::mutex> lock(allocation_map_mutex_);
            allocation_map_.erase(ptr);
        }
        
        memory_system_.deallocate(ptr);
        total_freed_.fetch_add(size, std::memory_order_relaxed);
    }
};

// Global Lua allocator function
extern "C" {
    static LuaMemoryManager* g_lua_memory_manager = nullptr;
    
    static void* lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize) {
        return g_lua_memory_manager->allocate(ud, ptr, osize, nsize);
    }
}

//=============================================================================
// Coroutine System
//=============================================================================

/**
 * @brief Lua coroutine wrapper with advanced scheduling
 */
class LuaCoroutine {
public:
    enum class State {
        Created,
        Running,  
        Suspended,
        Dead,
        Error
    };
    
private:
    lua_State* coroutine_state_;
    LuaStateWrapper main_state_;  // Keep reference to main state
    State state_;
    std::string name_;
    u32 coroutine_id_;
    
    // Execution context
    std::chrono::high_resolution_clock::time_point creation_time_;
    std::chrono::high_resolution_clock::time_point last_resume_time_;
    std::chrono::microseconds total_execution_time_{0};
    u32 resume_count_{0};
    
    // Error handling
    std::string last_error_;
    
    // Scheduling
    std::chrono::high_resolution_clock::time_point wake_time_;
    bool should_wake_on_time_{false};
    
public:
    LuaCoroutine(lua_State* main_state, const std::string& name, u32 id)
        : main_state_(main_state, false)
        , state_(State::Created)
        , name_(name)
        , coroutine_id_(id)
        , creation_time_(std::chrono::high_resolution_clock::now()) {
        
        coroutine_state_ = lua_newthread(main_state);
        if (!coroutine_state_) {
            state_ = State::Error;
            last_error_ = "Failed to create Lua coroutine";
        }
    }
    
    ~LuaCoroutine() = default;
    
    // Non-copyable but movable
    LuaCoroutine(const LuaCoroutine&) = delete;
    LuaCoroutine& operator=(const LuaCoroutine&) = delete;
    LuaCoroutine(LuaCoroutine&&) = default;
    LuaCoroutine& operator=(LuaCoroutine&&) = default;
    
    bool load_code(const std::string& code) {
        if (state_ != State::Created) return false;
        
        if (luaL_loadbuffer(coroutine_state_, code.c_str(), code.length(), name_.c_str()) != LUA_OK) {
            last_error_ = lua_tostring(coroutine_state_, -1);
            lua_pop(coroutine_state_, 1);
            state_ = State::Error;
            return false;
        }
        
        return true;
    }
    
    bool load_file(const std::string& filepath) {
        if (state_ != State::Created) return false;
        
        if (luaL_loadfile(coroutine_state_, filepath.c_str()) != LUA_OK) {
            last_error_ = lua_tostring(coroutine_state_, -1);
            lua_pop(coroutine_state_, 1);
            state_ = State::Error;
            return false;
        }
        
        return true;
    }
    
    enum class ResumeResult {
        Success,
        Yield,
        Finished,
        Error
    };
    
    ResumeResult resume(int nargs = 0) {
        if (state_ == State::Dead || state_ == State::Error) {
            return ResumeResult::Error;
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        last_resume_time_ = start_time;
        state_ = State::Running;
        resume_count_++;
        
        int result = lua_resume(coroutine_state_, nullptr, nargs);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        total_execution_time_ += std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        switch (result) {
            case LUA_OK:
                state_ = State::Dead;
                return ResumeResult::Finished;
                
            case LUA_YIELD:
                state_ = State::Suspended;
                return ResumeResult::Yield;
                
            default:
                state_ = State::Error;
                if (lua_isstring(coroutine_state_, -1)) {
                    last_error_ = lua_tostring(coroutine_state_, -1);
                    lua_pop(coroutine_state_, 1);
                } else {
                    last_error_ = "Unknown coroutine error";
                }
                return ResumeResult::Error;
        }
    }
    
    // Sleep functionality
    void sleep_for(std::chrono::milliseconds duration) {
        wake_time_ = std::chrono::high_resolution_clock::now() + duration;
        should_wake_on_time_ = true;
    }
    
    bool should_resume() const {
        if (state_ != State::Suspended) return false;
        if (!should_wake_on_time_) return true;
        
        return std::chrono::high_resolution_clock::now() >= wake_time_;
    }
    
    // Accessors
    State state() const noexcept { return state_; }
    const std::string& name() const noexcept { return name_; }
    u32 id() const noexcept { return coroutine_id_; }
    const std::string& last_error() const noexcept { return last_error_; }
    lua_State* get_state() const noexcept { return coroutine_state_; }
    
    // Statistics
    struct Statistics {
        std::chrono::high_resolution_clock::time_point creation_time;
        std::chrono::high_resolution_clock::time_point last_resume_time;
        std::chrono::microseconds total_execution_time;
        u32 resume_count;
        State current_state;
        bool has_error;
        std::string error_message;
    };
    
    Statistics get_statistics() const {
        return Statistics{
            .creation_time = creation_time_,
            .last_resume_time = last_resume_time_,
            .total_execution_time = total_execution_time_,
            .resume_count = resume_count_,
            .current_state = state_,
            .has_error = state_ == State::Error,
            .error_message = last_error_
        };
    }
};

/**
 * @brief Advanced coroutine scheduler with job system integration
 */
class LuaCoroutineScheduler {
private:
    std::vector<std::unique_ptr<LuaCoroutine>> coroutines_;
    std::queue<u32> ready_queue_;
    std::vector<u32> sleeping_coroutines_;
    std::atomic<u32> next_coroutine_id_{1};
    
    mutable std::mutex scheduler_mutex_;
    job_system::JobSystem* job_system_;
    
    // Statistics
    std::atomic<u64> total_resumes_{0};
    std::atomic<u64> total_yields_{0};
    std::atomic<u64> total_errors_{0};
    
public:
    explicit LuaCoroutineScheduler(job_system::JobSystem* job_system = nullptr)
        : job_system_(job_system) {}
    
    u32 create_coroutine(lua_State* main_state, const std::string& name) {
        u32 id = next_coroutine_id_.fetch_add(1, std::memory_order_relaxed);
        
        auto coroutine = std::make_unique<LuaCoroutine>(main_state, name, id);
        
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        // Find empty slot or add to end
        bool found_slot = false;
        for (usize i = 0; i < coroutines_.size(); ++i) {
            if (!coroutines_[i]) {
                coroutines_[i] = std::move(coroutine);
                found_slot = true;
                break;
            }
        }
        
        if (!found_slot) {
            coroutines_.push_back(std::move(coroutine));
        }
        
        return id;
    }
    
    LuaCoroutine* get_coroutine(u32 id) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        for (auto& coroutine : coroutines_) {
            if (coroutine && coroutine->id() == id) {
                return coroutine.get();
            }
        }
        
        return nullptr;
    }
    
    bool start_coroutine(u32 id, const std::string& code_or_file, bool is_file = false) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        LuaCoroutine* coroutine = find_coroutine_unsafe(id);
        if (!coroutine) return false;
        
        bool loaded = is_file ? 
            coroutine->load_file(code_or_file) :
            coroutine->load_code(code_or_file);
        
        if (loaded) {
            ready_queue_.push(id);
            return true;
        }
        
        return false;
    }
    
    void schedule_coroutine(u32 id) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        LuaCoroutine* coroutine = find_coroutine_unsafe(id);
        if (coroutine && coroutine->state() == LuaCoroutine::State::Suspended && 
            coroutine->should_resume()) {
            ready_queue_.push(id);
        }
    }
    
    void update() {
        std::vector<u32> ready_coroutines;
        
        // Collect ready coroutines
        {
            std::lock_guard<std::mutex> lock(scheduler_mutex_);
            
            // Check sleeping coroutines
            for (auto it = sleeping_coroutines_.begin(); it != sleeping_coroutines_.end();) {
                LuaCoroutine* coroutine = find_coroutine_unsafe(*it);
                if (coroutine && coroutine->should_resume()) {
                    ready_queue_.push(*it);
                    it = sleeping_coroutines_.erase(it);
                } else {
                    ++it;
                }
            }
            
            // Collect from ready queue
            while (!ready_queue_.empty()) {
                ready_coroutines.push_back(ready_queue_.front());
                ready_queue_.pop();
            }
        }
        
        // Execute coroutines (possibly in parallel)
        if (job_system_) {
            execute_coroutines_parallel(ready_coroutines);
        } else {
            execute_coroutines_sequential(ready_coroutines);
        }
    }
    
    void remove_coroutine(u32 id) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        for (auto& coroutine : coroutines_) {
            if (coroutine && coroutine->id() == id) {
                coroutine.reset();
                break;
            }
        }
        
        // Remove from sleeping list
        sleeping_coroutines_.erase(
            std::remove(sleeping_coroutines_.begin(), sleeping_coroutines_.end(), id),
            sleeping_coroutines_.end());
    }
    
    // Statistics
    struct Statistics {
        usize total_coroutines;
        usize active_coroutines;
        usize suspended_coroutines;
        usize dead_coroutines;
        usize error_coroutines;
        u64 total_resumes;
        u64 total_yields;
        u64 total_errors;
        f64 average_execution_time_ms;
    };
    
    Statistics get_statistics() const {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        usize active = 0, suspended = 0, dead = 0, error = 0;
        f64 total_execution_ms = 0.0;
        usize execution_count = 0;
        
        for (const auto& coroutine : coroutines_) {
            if (!coroutine) continue;
            
            switch (coroutine->state()) {
                case LuaCoroutine::State::Running:
                case LuaCoroutine::State::Created:
                    ++active;
                    break;
                case LuaCoroutine::State::Suspended:
                    ++suspended;
                    break;
                case LuaCoroutine::State::Dead:
                    ++dead;
                    break;
                case LuaCoroutine::State::Error:
                    ++error;
                    break;
            }
            
            auto stats = coroutine->get_statistics();
            total_execution_ms += std::chrono::duration<f64, std::milli>(stats.total_execution_time).count();
            if (stats.resume_count > 0) {
                ++execution_count;
            }
        }
        
        return Statistics{
            .total_coroutines = coroutines_.size(),
            .active_coroutines = active,
            .suspended_coroutines = suspended,
            .dead_coroutines = dead,
            .error_coroutines = error,
            .total_resumes = total_resumes_.load(std::memory_order_relaxed),
            .total_yields = total_yields_.load(std::memory_order_relaxed),
            .total_errors = total_errors_.load(std::memory_order_relaxed),
            .average_execution_time_ms = execution_count > 0 ? total_execution_ms / execution_count : 0.0
        };
    }
    
    std::vector<LuaCoroutine::Statistics> get_coroutine_details() const {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        std::vector<LuaCoroutine::Statistics> details;
        for (const auto& coroutine : coroutines_) {
            if (coroutine) {
                details.push_back(coroutine->get_statistics());
            }
        }
        
        return details;
    }

private:
    LuaCoroutine* find_coroutine_unsafe(u32 id) {
        for (auto& coroutine : coroutines_) {
            if (coroutine && coroutine->id() == id) {
                return coroutine.get();
            }
        }
        return nullptr;
    }
    
    void execute_coroutines_sequential(const std::vector<u32>& coroutine_ids) {
        for (u32 id : coroutine_ids) {
            execute_coroutine(id);
        }
    }
    
    void execute_coroutines_parallel(const std::vector<u32>& coroutine_ids) {
        std::vector<job_system::JobID> jobs;
        
        for (u32 id : coroutine_ids) {
            std::string job_name = "LuaCoroutine_" + std::to_string(id);
            
            auto job_id = job_system_->submit_job(job_name, [this, id]() {
                execute_coroutine(id);
            }, job_system::JobPriority::Normal, job_system::JobAffinity::WorkerThread);
            
            jobs.push_back(job_id);
        }
        
        // Wait for all coroutines to complete
        job_system_->wait_for_batch(jobs);
    }
    
    void execute_coroutine(u32 id) {
        LuaCoroutine* coroutine = nullptr;
        
        {
            std::lock_guard<std::mutex> lock(scheduler_mutex_);
            coroutine = find_coroutine_unsafe(id);
        }
        
        if (!coroutine) return;
        
        auto result = coroutine->resume();
        
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        total_resumes_.fetch_add(1, std::memory_order_relaxed);
        
        switch (result) {
            case LuaCoroutine::ResumeResult::Success:
            case LuaCoroutine::ResumeResult::Finished:
                // Coroutine completed
                break;
                
            case LuaCoroutine::ResumeResult::Yield:
                total_yields_.fetch_add(1, std::memory_order_relaxed);
                if (coroutine->should_resume()) {
                    ready_queue_.push(id);  // Reschedule immediately
                } else {
                    sleeping_coroutines_.push_back(id);  // Put to sleep
                }
                break;
                
            case LuaCoroutine::ResumeResult::Error:
                total_errors_.fetch_add(1, std::memory_order_relaxed);
                LOG_ERROR("Lua coroutine {} error: {}", coroutine->name(), coroutine->last_error());
                break;
        }
    }
};

//=============================================================================
// Component Binding System  
//=============================================================================

/**
 * @brief Automatic Lua binding generation for ECS components
 */
class ComponentBinding {
private:
    struct ComponentDescriptor {
        std::string name;
        usize size;
        usize alignment;
        core::ComponentID component_id;
        
        // Lua userdata metatable name
        std::string metatable_name;
        
        // Field descriptors
        struct FieldDescriptor {
            std::string name;
            usize offset;
            std::string type_name;
            std::function<void(lua_State*, const void*)> push_to_lua;
            std::function<bool(lua_State*, void*, int)> get_from_lua;
        };
        std::vector<FieldDescriptor> fields;
        
        // Constructor/destructor
        std::function<void(void*)> constructor;
        std::function<void(void*)> destructor;
    };
    
    std::unordered_map<core::ComponentID, ComponentDescriptor> components_;
    std::unordered_map<std::string, core::ComponentID> name_to_id_;
    
public:
    template<typename Component>
    void register_component(lua_State* L, const std::string& name) {
        static_assert(ecs::Component<Component>, "Type must satisfy Component concept");
        
        ComponentDescriptor desc;
        desc.name = name;
        desc.size = sizeof(Component);
        desc.alignment = alignof(Component);
        desc.component_id = ecs::component_id<Component>();
        desc.metatable_name = name + "_mt";
        
        // Register fields (simplified - would use reflection)
        register_component_fields<Component>(desc);
        
        // Constructor/destructor
        desc.constructor = [](void* ptr) { new(ptr) Component{}; };
        desc.destructor = [](void* ptr) { static_cast<Component*>(ptr)->~Component(); };
        
        components_[desc.component_id] = desc;
        name_to_id_[name] = desc.component_id;
        
        // Create Lua metatable for this component type
        create_lua_metatable(L, desc);
        
        LOG_INFO("Registered Lua component binding: {}", name);
    }
    
    int create_component(lua_State* L, const std::string& component_name) {
        auto it = name_to_id_.find(component_name);
        if (it == name_to_id_.end()) {
            luaL_error(L, "Unknown component: %s", component_name.c_str());
            return 0;
        }
        
        const auto& desc = components_[it->second];
        
        // Allocate userdata
        void* userdata = lua_newuserdata(L, desc.size);
        
        // Initialize component
        desc.constructor(userdata);
        
        // Set metatable
        luaL_getmetatable(L, desc.metatable_name.c_str());
        lua_setmetatable(L, -2);
        
        return 1;  // Return component instance
    }
    
    // Lua C functions for component operations
    static int component_index(lua_State* L) {
        // Get field value: component.field_name
        const char* field_name = luaL_checkstring(L, 2);
        void* userdata = luaL_checkudata(L, 1, nullptr);  // Would check specific metatable
        
        // Find component type and field
        // Implementation would look up component type and access field
        
        lua_pushnil(L);  // Placeholder
        return 1;
    }
    
    static int component_newindex(lua_State* L) {
        // Set field value: component.field_name = value
        const char* field_name = luaL_checkstring(L, 2);
        void* userdata = luaL_checkudata(L, 1, nullptr);
        
        // Implementation would set field value with type checking
        return 0;
    }
    
    static int component_gc(lua_State* L) {
        // Garbage collection handler
        void* userdata = luaL_checkudata(L, 1, nullptr);
        
        // Call component destructor
        // Implementation would identify component type and call destructor
        
        return 0;
    }
    
    static int component_tostring(lua_State* L) {
        // String representation
        void* userdata = luaL_checkudata(L, 1, nullptr);
        
        lua_pushstring(L, "ECScope Component");  // Placeholder
        return 1;
    }

private:
    template<typename Component>
    void register_component_fields(ComponentDescriptor& desc) {
        // Simplified field registration - would use reflection system
        if constexpr (requires { std::declval<Component>().x; std::declval<Component>().y; }) {
            ComponentDescriptor::FieldDescriptor x_field;
            x_field.name = "x";
            x_field.offset = offsetof(Component, x);
            x_field.type_name = "float";
            x_field.push_to_lua = [](lua_State* L, const void* ptr) {
                const auto* comp = static_cast<const Component*>(ptr);
                lua_pushnumber(L, static_cast<lua_Number>(comp->x));
            };
            x_field.get_from_lua = [](lua_State* L, void* ptr, int index) {
                if (!lua_isnumber(L, index)) return false;
                auto* comp = static_cast<Component*>(ptr);
                comp->x = static_cast<decltype(comp->x)>(lua_tonumber(L, index));
                return true;
            };
            desc.fields.push_back(x_field);
            
            // Similar for y field
            ComponentDescriptor::FieldDescriptor y_field;
            y_field.name = "y";
            y_field.offset = offsetof(Component, y);
            y_field.type_name = "float";
            y_field.push_to_lua = [](lua_State* L, const void* ptr) {
                const auto* comp = static_cast<const Component*>(ptr);
                lua_pushnumber(L, static_cast<lua_Number>(comp->y));
            };
            y_field.get_from_lua = [](lua_State* L, void* ptr, int index) {
                if (!lua_isnumber(L, index)) return false;
                auto* comp = static_cast<Component*>(ptr);
                comp->y = static_cast<decltype(comp->y)>(lua_tonumber(L, index));
                return true;
            };
            desc.fields.push_back(y_field);
        }
        
        if constexpr (requires { std::declval<Component>().z; }) {
            ComponentDescriptor::FieldDescriptor z_field;
            z_field.name = "z";
            z_field.offset = offsetof(Component, z);
            z_field.type_name = "float";
            z_field.push_to_lua = [](lua_State* L, const void* ptr) {
                const auto* comp = static_cast<const Component*>(ptr);
                lua_pushnumber(L, static_cast<lua_Number>(comp->z));
            };
            z_field.get_from_lua = [](lua_State* L, void* ptr, int index) {
                if (!lua_isnumber(L, index)) return false;
                auto* comp = static_cast<Component*>(ptr);
                comp->z = static_cast<decltype(comp->z)>(lua_tonumber(L, index));
                return true;
            };
            desc.fields.push_back(z_field);
        }
    }
    
    void create_lua_metatable(lua_State* L, const ComponentDescriptor& desc) {
        luaL_newmetatable(L, desc.metatable_name.c_str());
        
        // Set metamethods
        lua_pushcfunction(L, component_index);
        lua_setfield(L, -2, "__index");
        
        lua_pushcfunction(L, component_newindex);
        lua_setfield(L, -2, "__newindex");
        
        lua_pushcfunction(L, component_gc);
        lua_setfield(L, -2, "__gc");
        
        lua_pushcfunction(L, component_tostring);
        lua_setfield(L, -2, "__tostring");
        
        // Pop metatable
        lua_pop(L, 1);
    }
};

} // namespace ecscope::scripting::lua