#pragma once

#include "core/types.hpp"
#include "core/log.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "signature.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstring>

namespace ecscope::ecs {

// Forward declarations
class Registry;

// Chunk size for archetype storage (can be tuned for cache performance)
constexpr usize DEFAULT_CHUNK_SIZE = 16384; // 16KB chunks

// Type-erased component array for SoA (Structure of Arrays) storage
class ComponentArray {
private:
    void* data_;                    // Raw memory for components
    usize element_size_;            // Size of each component
    usize element_alignment_;       // Alignment requirement
    usize capacity_;                // Number of elements that can be stored
    usize size_;                    // Current number of elements
    const char* component_name_;    // For debugging
    
public:
    ComponentArray(usize element_size, usize alignment, const char* name = nullptr)
        : data_(nullptr)
        , element_size_(element_size)
        , element_alignment_(alignment)
        , capacity_(0)
        , size_(0)
        , component_name_(name) {
        reserve(DEFAULT_CHUNK_SIZE / element_size_); // Default initial capacity
    }
    
    ~ComponentArray() {
        if (data_) {
            std::free(data_);
        }
    }
    
    // Non-copyable but movable
    ComponentArray(const ComponentArray&) = delete;
    ComponentArray& operator=(const ComponentArray&) = delete;
    
    ComponentArray(ComponentArray&& other) noexcept
        : data_(other.data_)
        , element_size_(other.element_size_)
        , element_alignment_(other.element_alignment_)
        , capacity_(other.capacity_)
        , size_(other.size_)
        , component_name_(other.component_name_) {
        other.data_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
    }
    
    ComponentArray& operator=(ComponentArray&& other) noexcept {
        if (this != &other) {
            if (data_) {
                std::free(data_);
            }
            
            data_ = other.data_;
            element_size_ = other.element_size_;
            element_alignment_ = other.element_alignment_;
            capacity_ = other.capacity_;
            size_ = other.size_;
            component_name_ = other.component_name_;
            
            other.data_ = nullptr;
            other.capacity_ = 0;
            other.size_ = 0;
        }
        return *this;
    }
    
    // Reserve capacity
    void reserve(usize new_capacity) {
        if (new_capacity <= capacity_) return;
        
        void* new_data = std::aligned_alloc(element_alignment_, new_capacity * element_size_);
        if (!new_data) {
            LOG_ERROR("Failed to allocate memory for ComponentArray");
            return;
        }
        
        if (data_ && size_ > 0) {
            std::memcpy(new_data, data_, size_ * element_size_);
        }
        
        if (data_) {
            std::free(data_);
        }
        
        data_ = new_data;
        capacity_ = new_capacity;
    }
    
    // Add component (assumes T is the correct type)
    template<Component T>
    void push_back(const T& component) {
        static_assert(std::is_trivially_copyable_v<T>);
        
        if (sizeof(T) != element_size_) {
            LOG_ERROR("Component size mismatch");
            return;
        }
        
        if (size_ >= capacity_) {
            reserve(capacity_ * 2); // Double capacity
        }
        
        std::memcpy(static_cast<byte*>(data_) + (size_ * element_size_), &component, element_size_);
        ++size_;
    }
    
    // Get component at index (assumes T is the correct type)
    template<Component T>
    T* get(usize index) {
        if (index >= size_ || sizeof(T) != element_size_) {
            return nullptr;
        }
        return reinterpret_cast<T*>(static_cast<byte*>(data_) + (index * element_size_));
    }
    
    template<Component T>
    const T* get(usize index) const {
        if (index >= size_ || sizeof(T) != element_size_) {
            return nullptr;
        }
        return reinterpret_cast<const T*>(static_cast<const byte*>(data_) + (index * element_size_));
    }
    
    // Remove element by index (swap with last)
    void remove_swap_back(usize index) {
        if (index >= size_) return;
        
        if (index != size_ - 1) {
            // Swap with last element
            void* target = static_cast<byte*>(data_) + (index * element_size_);
            void* last = static_cast<byte*>(data_) + ((size_ - 1) * element_size_);
            
            // Use temporary buffer for swap
            byte temp[element_size_];
            std::memcpy(temp, target, element_size_);
            std::memcpy(target, last, element_size_);
            std::memcpy(last, temp, element_size_);
        }
        
        --size_;
    }
    
    // Clear all elements
    void clear() {
        size_ = 0;
    }
    
    // Access raw data
    void* data() { return data_; }
    const void* data() const { return data_; }
    
    // Size and capacity
    usize size() const { return size_; }
    usize capacity() const { return capacity_; }
    usize element_size() const { return element_size_; }
    bool empty() const { return size_ == 0; }
    
    // Memory usage
    usize memory_usage() const {
        return capacity_ * element_size_;
    }
    
    // Component name (for debugging)
    const char* name() const { return component_name_; }
};

// Archetype: stores entities with the same component signature using SoA
class Archetype {
private:
    ComponentSignature signature_;                                      // Which components this archetype has
    std::vector<Entity> entities_;                                      // Packed array of entities
    std::unordered_map<core::ComponentID, ComponentArray> components_; // Component arrays (SoA)
    std::vector<ComponentInfo> component_infos_;                        // Metadata for components
    
public:
    explicit Archetype(const ComponentSignature& signature)
        : signature_(signature) {
        
        // Pre-allocate entities vector
        entities_.reserve(DEFAULT_CHUNK_SIZE / sizeof(Entity));
    }
    
    // Add component type to this archetype
    template<Component T>
    void add_component_type() {
        auto id = component_id<T>();
        if (components_.find(id) == components_.end()) {
            components_.emplace(id, ComponentArray(sizeof(T), alignof(T), typeid(T).name()));
            component_infos_.push_back(ComponentInfo::create<T>());
            signature_.set<T>();
        }
    }
    
    // Create entity with components
    template<Component... Components>
    Entity create_entity(Components&&... components) {
        Entity entity = create_entity();
        
        // Add each component
        (add_component_to_entity(entity, std::forward<Components>(components)), ...);
        
        return entity;
    }
    
    // Create entity without components (components added separately)
    Entity create_entity() {
        Entity entity = ecs::create_entity();
        entities_.push_back(entity);
        return entity;
    }
    
    // Add component to existing entity
    template<Component T>
    bool add_component_to_entity(Entity entity, const T& component) {
        // Find entity index
        auto it = std::find(entities_.begin(), entities_.end(), entity);
        if (it == entities_.end()) {
            LOG_WARN("Entity not found in archetype");
            return false;
        }
        
        usize index = it - entities_.begin();
        auto comp_id = component_id<T>();
        
        auto comp_it = components_.find(comp_id);
        if (comp_it != components_.end()) {
            // Component array exists, add to it
            comp_it->second.push_back(component);
            return true;
        } else {
            LOG_WARN("Component type not registered in this archetype");
            return false;
        }
    }
    
    // Get component from entity
    template<Component T>
    T* get_component(Entity entity) {
        auto it = std::find(entities_.begin(), entities_.end(), entity);
        if (it == entities_.end()) {
            return nullptr;
        }
        
        usize index = it - entities_.begin();
        auto comp_id = component_id<T>();
        
        auto comp_it = components_.find(comp_id);
        if (comp_it != components_.end()) {
            return comp_it->second.template get<T>(index);
        }
        
        return nullptr;
    }
    
    template<Component T>
    const T* get_component(Entity entity) const {
        auto it = std::find(entities_.begin(), entities_.end(), entity);
        if (it == entities_.end()) {
            return nullptr;
        }
        
        usize index = it - entities_.begin();
        auto comp_id = component_id<T>();
        
        auto comp_it = components_.find(comp_id);
        if (comp_it != components_.end()) {
            return comp_it->second.template get<T>(index);
        }
        
        return nullptr;
    }
    
    // Remove entity
    bool remove_entity(Entity entity) {
        auto it = std::find(entities_.begin(), entities_.end(), entity);
        if (it == entities_.end()) {
            return false;
        }
        
        usize index = it - entities_.begin();
        
        // Remove from all component arrays using swap-back
        for (auto& [id, component_array] : components_) {
            component_array.remove_swap_back(index);
        }
        
        // Remove entity using swap-back
        if (index != entities_.size() - 1) {
            std::swap(*it, entities_.back());
        }
        entities_.pop_back();
        
        return true;
    }
    
    // Get entity count
    usize entity_count() const {
        return entities_.size();
    }
    
    // Check if archetype is empty
    bool empty() const {
        return entities_.empty();
    }
    
    // Get signature
    const ComponentSignature& signature() const {
        return signature_;
    }
    
    // Check if archetype has component type
    template<Component T>
    bool has_component() const {
        return signature_.has<T>();
    }
    
    // Get all entities (for iteration)
    const std::vector<Entity>& entities() const {
        return entities_;
    }
    
    // Get component array for iteration
    template<Component T>
    ComponentArray* get_component_array() {
        auto id = component_id<T>();
        auto it = components_.find(id);
        return it != components_.end() ? &it->second : nullptr;
    }
    
    template<Component T>
    const ComponentArray* get_component_array() const {
        auto id = component_id<T>();
        auto it = components_.find(id);
        return it != components_.end() ? &it->second : nullptr;
    }
    
    // Memory usage information
    usize memory_usage() const {
        usize total = sizeof(*this);
        total += entities_.capacity() * sizeof(Entity);
        
        for (const auto& [id, array] : components_) {
            total += array.memory_usage();
        }
        
        return total;
    }
    
    // Component information
    const std::vector<ComponentInfo>& component_infos() const {
        return component_infos_;
    }
    
    // Debug information
    void debug_print() const {
        LOG_INFO("Archetype with " + std::to_string(entities_.size()) + " entities");
        LOG_INFO("Signature: " + signature_.to_string());
        for (const auto& info : component_infos_) {
            LOG_INFO("Component: " + std::string(info.name) + " (size=" + std::to_string(info.size) + ")");
        }
    }
};

// Factory functions for creating optimized archetypes
std::unique_ptr<Archetype> create_arena_archetype(
    const ComponentSignature& signature,
    memory::ArenaAllocator& arena,
    bool enable_tracking = true);
    
std::unique_ptr<Archetype> create_pmr_archetype(
    const ComponentSignature& signature,
    std::pmr::memory_resource& resource,
    bool enable_tracking = true);

} // namespace ecscope::ecs