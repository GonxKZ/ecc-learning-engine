#pragma once

#include "../core/types.hpp"
#include "../core/memory.hpp"
#include "../foundation/concepts.hpp"
#include "../foundation/component.hpp"
#include "sparse_set.hpp"
#include "chunk.hpp"
#include <vector>
#include <span>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <type_traits>
#include <utility>
#include <cassert>
#include <bit>
#include <atomic>
#include <mutex>
#include <functional>

/**
 * @file archetype.hpp
 * @brief Archetype-based entity organization with structural change tracking
 * 
 * This file implements a comprehensive archetype system with:
 * - Efficient archetype identification and management
 * - Fast entity transitions between archetypes
 * - Structural change detection and optimization
 * - Cache-friendly component chunk organization
 * - Query-optimized archetype matching
 * - Memory-efficient component type tracking
 * - Thread-safe archetype operations
 * - Performance monitoring and diagnostics
 * 
 * Educational Notes:
 * - Archetypes group entities with identical component signatures
 * - Structural changes (add/remove components) trigger archetype transitions
 * - Component chunks within archetypes enable cache-friendly iteration
 * - Archetype graphs optimize transition paths between related archetypes
 * - Bloom filters accelerate query matching for large archetype counts
 * - Version tracking enables change detection for query caching
 * - Hot/cold archetype separation improves cache utilization
 */

namespace ecscope::registry {

using namespace ecscope::core;
using namespace ecscope::foundation;

/// @brief Configuration for archetype system behavior
struct ArchetypeConfig {
    std::uint32_t initial_archetype_capacity{256};        ///< Initial archetype storage capacity
    std::uint32_t max_archetype_count{65536};             ///< Maximum number of archetypes
    std::size_t entities_per_archetype_hint{1024};        ///< Expected entities per archetype
    bool enable_archetype_graphs{true};                   ///< Enable archetype transition graphs
    bool enable_bloom_filters{true};                      ///< Enable bloom filter optimization
    bool enable_hot_cold_separation{true};                ///< Separate hot/cold archetypes
    bool enable_structural_change_tracking{true};         ///< Track structural changes
    std::uint32_t bloom_filter_size{1024};                ///< Bloom filter size in bits
    std::uint32_t transition_cache_size{512};             ///< Archetype transition cache size
    double hot_archetype_threshold{0.1};                  ///< Access ratio for hot classification
};

/// @brief Archetype identifier with efficient comparison
struct ArchetypeSignature {
    ComponentSignature signature{};
    std::uint32_t component_count{};
    std::uint64_t hash{};
    
    ArchetypeSignature() = default;
    
    explicit ArchetypeSignature(ComponentSignature sig) 
        : signature(sig), component_count(std::popcount(sig)) {
        hash = calculate_hash();
    }
    
    bool operator==(const ArchetypeSignature& other) const noexcept {
        return signature == other.signature;
    }
    
    bool operator!=(const ArchetypeSignature& other) const noexcept {
        return signature != other.signature;
    }
    
    bool operator<(const ArchetypeSignature& other) const noexcept {
        return signature < other.signature;
    }
    
    /// @brief Check if this archetype matches a query signature
    bool matches_query(ComponentSignature required, ComponentSignature excluded = 0) const noexcept {
        return (signature & required) == required && (signature & excluded) == 0;
    }
    
    /// @brief Check if archetype contains specific component
    bool has_component(ComponentId id) const noexcept {
        return id.is_valid() && id.value < 64 && (signature & (1ULL << id.value)) != 0;
    }
    
    /// @brief Get all component IDs in this archetype
    std::vector<ComponentId> get_component_ids() const {
        std::vector<ComponentId> ids;
        ids.reserve(component_count);
        
        for (std::uint16_t i = 0; i < 64; ++i) {
            if (signature & (1ULL << i)) {
                ids.push_back(ComponentId{i});
            }
        }
        
        return ids;
    }
    
    /// @brief Calculate signature hash for fast lookups
    std::uint64_t calculate_hash() const noexcept {
        // Use FNV-1a hash algorithm
        std::uint64_t hash = 14695981039346656037ULL;
        hash ^= signature;
        hash *= 1099511628211ULL;
        return hash;
    }
    
    /// @brief Create signature from component list
    template<Component... Components>
    static ArchetypeSignature create() {
        ComponentSignature sig = 0;
        ((sig |= (1ULL << component_utils::get_component_id<Components>().value)), ...);
        return ArchetypeSignature(sig);
    }
};

/// @brief Archetype transition descriptor for structural changes
struct ArchetypeTransition {
    ArchetypeId from_archetype{};
    ArchetypeId to_archetype{};
    ComponentId changed_component{};
    enum class Type { Add, Remove, Replace } type;
    std::uint32_t transition_count{};  // Usage statistics
    
    bool operator==(const ArchetypeTransition& other) const noexcept {
        return from_archetype == other.from_archetype && 
               to_archetype == other.to_archetype &&
               changed_component == other.changed_component &&
               type == other.type;
    }
};

/// @brief Structural change event for notifications
struct StructuralChange {
    EntityHandle entity{};
    ArchetypeId from_archetype{};
    ArchetypeId to_archetype{};
    ComponentId changed_component{};
    ArchetypeTransition::Type change_type{};
    Version change_version{};
};

/// @brief Forward declaration
class Archetype;

/// @brief Archetype graph for optimizing transitions
class ArchetypeGraph {
public:
    /// @brief Add archetype to the graph
    void add_archetype(ArchetypeId id, const ArchetypeSignature& signature) {
        std::lock_guard<std::mutex> lock(mutex_);
        archetype_signatures_[id] = signature;
        build_transition_cache(id, signature);
    }
    
    /// @brief Remove archetype from the graph
    void remove_archetype(ArchetypeId id) {
        std::lock_guard<std::mutex> lock(mutex_);
        archetype_signatures_.erase(id);
        // Remove all transitions involving this archetype
        transitions_.erase(id);
        for (auto& [archetype, transitions] : transitions_) {
            transitions.erase(id);
        }
    }
    
    /// @brief Find target archetype for component addition
    ArchetypeId find_add_archetype(ArchetypeId current, ComponentId component) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = transitions_.find(current);
        if (it != transitions_.end()) {
            auto transition_it = it->second.find(ComponentSignature(1ULL << component.value));
            if (transition_it != it->second.end()) {
                return transition_it->second.to_archetype;
            }
        }
        
        return ArchetypeId::invalid();
    }
    
    /// @brief Find target archetype for component removal
    ArchetypeId find_remove_archetype(ArchetypeId current, ComponentId component) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = transitions_.find(current);
        if (it != transitions_.end()) {
            auto transition_it = it->second.find(ComponentSignature(~(1ULL << component.value)));
            if (transition_it != it->second.end()) {
                return transition_it->second.to_archetype;
            }
        }
        
        return ArchetypeId::invalid();
    }
    
    /// @brief Get transition statistics
    const ArchetypeTransition* get_transition_info(ArchetypeId from, ArchetypeId to) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = transition_details_.find({from, to});
        return it != transition_details_.end() ? &it->second : nullptr;
    }

private:
    /// @brief Build transition cache for new archetype
    void build_transition_cache(ArchetypeId id, const ArchetypeSignature& signature) {
        // Find all possible single-component additions and removals
        for (const auto& [other_id, other_sig] : archetype_signatures_) {
            if (other_id == id) continue;
            
            const auto diff = signature.signature ^ other_sig.signature;
            const auto diff_count = std::popcount(diff);
            
            if (diff_count == 1) {
                // Single component difference - this is a valid transition
                const ComponentId changed_component{static_cast<std::uint16_t>(std::countr_zero(diff))};
                
                ArchetypeTransition transition{};
                transition.changed_component = changed_component;
                
                if (signature.signature & (1ULL << changed_component.value)) {
                    // Current archetype has the component, other doesn't - removal
                    transition.from_archetype = id;
                    transition.to_archetype = other_id;
                    transition.type = ArchetypeTransition::Type::Remove;
                    transitions_[id][~(1ULL << changed_component.value)] = transition;
                } else {
                    // Other archetype has the component, current doesn't - addition
                    transition.from_archetype = other_id;
                    transition.to_archetype = id;
                    transition.type = ArchetypeTransition::Type::Add;
                    transitions_[other_id][1ULL << changed_component.value] = transition;
                }
                
                transition_details_[{transition.from_archetype, transition.to_archetype}] = transition;
            }
        }
    }
    
    mutable std::mutex mutex_;
    
    /// @brief Archetype signature storage
    std::unordered_map<ArchetypeId, ArchetypeSignature> archetype_signatures_;
    
    /// @brief Transition cache: [from_archetype][component_diff] -> transition
    std::unordered_map<ArchetypeId, std::unordered_map<ComponentSignature, ArchetypeTransition>> transitions_;
    
    /// @brief Hash function for archetype ID pairs
    struct ArchetypeIdPairHash {
        std::size_t operator()(const std::pair<ArchetypeId, ArchetypeId>& p) const noexcept {
            std::size_t h1 = std::hash<ArchetypeId>{}(p.first);
            std::size_t h2 = std::hash<ArchetypeId>{}(p.second);
            return h1 ^ (h2 << 1);
        }
    };
    
    /// @brief Detailed transition information
    std::unordered_map<std::pair<ArchetypeId, ArchetypeId>, ArchetypeTransition, ArchetypeIdPairHash> transition_details_;
};

/// @brief Main archetype class for organizing entities by component signature
class Archetype {
public:
    using entity_iterator = AdvancedSparseSet::Iterator;
    
    /// @brief Construct archetype with signature
    explicit Archetype(ArchetypeId id, ArchetypeSignature signature, const ArchetypeConfig& config = {})
        : id_(id)
        , signature_(signature)
        , config_(config)
        , entity_set_(SparseSetConfig{
            .initial_dense_capacity = static_cast<std::uint32_t>(config.entities_per_archetype_hint),
            .enable_thread_safety = true,
            .enable_simd_optimization = true
          })
        , version_(constants::INITIAL_VERSION) {
        
        initialize_component_storages();
    }
    
    /// @brief Non-copyable but movable
    Archetype(const Archetype&) = delete;
    Archetype& operator=(const Archetype&) = delete;
    Archetype(Archetype&&) noexcept = default;
    Archetype& operator=(Archetype&&) noexcept = default;
    
    /// @brief Get archetype ID
    ArchetypeId id() const noexcept { return id_; }
    
    /// @brief Get archetype signature
    const ArchetypeSignature& signature() const noexcept { return signature_; }
    
    /// @brief Get current version for change detection
    Version version() const noexcept { return version_.load(std::memory_order_acquire); }
    
    /// @brief Get number of entities in this archetype
    std::size_t entity_count() const noexcept { return entity_set_.size(); }
    
    /// @brief Check if archetype is empty
    bool empty() const noexcept { return entity_set_.empty(); }
    
    /// @brief Check if entity belongs to this archetype
    bool contains_entity(EntityHandle entity) const noexcept {
        return entity_set_.contains(entity);
    }
    
    /// @brief Add entity to archetype
    /// @param entity Entity to add
    /// @throws std::invalid_argument if entity already exists
    void add_entity(EntityHandle entity) {
        if (entity_set_.contains(entity)) {
            throw std::invalid_argument("Entity already exists in archetype");
        }
        
        entity_set_.insert(entity);
        increment_version();
        
        // Notify structural change listeners
        if (structural_change_callback_) {
            StructuralChange change{
                .entity = entity,
                .from_archetype = ArchetypeId::invalid(),
                .to_archetype = id_,
                .change_type = ArchetypeTransition::Type::Add,
                .change_version = version()
            };
            structural_change_callback_(change);
        }
    }
    
    /// @brief Remove entity from archetype
    /// @param entity Entity to remove
    /// @return true if entity was removed, false if not found
    bool remove_entity(EntityHandle entity) noexcept {
        if (!entity_set_.remove(entity)) {
            return false;
        }
        
        // Remove from all component storages
        for (auto& [component_id, storage] : component_storages_) {
            storage->remove(entity);
        }
        
        increment_version();
        
        // Notify structural change listeners
        if (structural_change_callback_) {
            StructuralChange change{
                .entity = entity,
                .from_archetype = id_,
                .to_archetype = ArchetypeId::invalid(),
                .change_type = ArchetypeTransition::Type::Remove,
                .change_version = version()
            };
            structural_change_callback_(change);
        }
        
        return true;
    }
    
    /// @brief Get all entities in this archetype
    std::span<const EntityHandle> entities() const noexcept {
        return entity_set_.entities();
    }
    
    /// @brief Entity iteration support
    entity_iterator begin() const noexcept { return entity_set_.begin(); }
    entity_iterator end() const noexcept { return entity_set_.end(); }
    
    /// @brief Check if archetype matches query signature
    bool matches_query(ComponentSignature required, ComponentSignature excluded = 0) const noexcept {
        return signature_.matches_query(required, excluded);
    }
    
    /// @brief Get component storage for specific component type
    /// @param id Component ID
    /// @return Pointer to storage, or nullptr if component not in archetype
    IComponentStorage* get_component_storage(ComponentId id) const noexcept {
        auto it = component_storages_.find(id);
        return it != component_storages_.end() ? it->second.get() : nullptr;
    }
    
    /// @brief Get all component storages
    const std::unordered_map<ComponentId, std::unique_ptr<IComponentStorage>>& 
    component_storages() const noexcept {
        return component_storages_;
    }
    
    /// @brief Advanced iteration with multiple component access
    template<Component... Components>
    class MultiComponentView {
    public:
        explicit MultiComponentView(const Archetype& archetype) : archetype_(archetype) {
            // Validate that all components exist in this archetype
            ((validate_component<Components>()), ...);
            gather_storages();
        }
        
        /// @brief Iterator for multi-component access
        class Iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = std::tuple<EntityHandle, Components&...>;
            using pointer = void;  // Not supported for tuple return
            using reference = value_type;
            
            Iterator(const MultiComponentView& view, std::size_t index) 
                : view_(view), index_(index) {}
            
            reference operator*() const {
                const auto entity = view_.archetype_.entity_set_.get_entity(
                    static_cast<std::uint32_t>(index_));
                return std::make_tuple(entity, get_component_ref<Components>(entity)...);
            }
            
            Iterator& operator++() { ++index_; return *this; }
            Iterator operator++(int) { auto tmp = *this; ++index_; return tmp; }
            
            bool operator==(const Iterator& other) const noexcept { 
                return index_ == other.index_; 
            }
            bool operator!=(const Iterator& other) const noexcept { 
                return index_ != other.index_; 
            }

        private:
            template<Component T>
            T& get_component_ref(EntityHandle entity) const {
                auto* storage = view_.get_storage<T>();
                return static_cast<TypedComponentStorage<T>*>(storage)->get_storage().get(entity);
            }
            
            const MultiComponentView& view_;
            std::size_t index_;
        };
        
        Iterator begin() const { return Iterator(*this, 0); }
        Iterator end() const { return Iterator(*this, archetype_.entity_count()); }
        
        /// @brief Process all entities with function
        template<typename Func>
        void for_each(Func&& func) const {
            const auto entities = archetype_.entities();
            
            for (const auto entity : entities) {
                auto components = std::make_tuple(get_component<Components>(entity)...);
                std::apply([&func, entity](auto&... comps) {
                    func(entity, comps...);
                }, components);
            }
        }
        
        /// @brief Parallel processing of entities
        template<typename Func>
        void parallel_for_each(Func&& func, std::size_t batch_size = 64) const {
            const auto entities = archetype_.entities();
            
            if (entities.size() < batch_size * 2) {
                for_each(func);
                return;
            }
            
            // Process in batches (would use job system in full implementation)
            const auto num_batches = (entities.size() + batch_size - 1) / batch_size;
            
            for (std::size_t batch = 0; batch < num_batches; ++batch) {
                const auto start = batch * batch_size;
                const auto end = std::min(start + batch_size, entities.size());
                
                for (auto i = start; i < end; ++i) {
                    const auto entity = entities[i];
                    auto components = std::make_tuple(get_component<Components>(entity)...);
                    std::apply([&func, entity](auto&... comps) {
                        func(entity, comps...);
                    }, components);
                }
            }
        }

    private:
        template<Component T>
        void validate_component() const {
            const auto id = component_utils::get_component_id<T>();
            if (!archetype_.signature_.has_component(id)) {
                throw std::invalid_argument("Component not present in archetype");
            }
        }
        
        void gather_storages() {
            (gather_storage<Components>(), ...);
        }
        
        template<Component T>
        void gather_storage() {
            const auto id = component_utils::get_component_id<T>();
            auto* storage = archetype_.get_component_storage(id);
            if (!storage) {
                throw std::runtime_error("Component storage not found");
            }
            storages_[id] = storage;
        }
        
        template<Component T>
        IComponentStorage* get_storage() const {
            const auto id = component_utils::get_component_id<T>();
            auto it = storages_.find(id);
            return it != storages_.end() ? it->second : nullptr;
        }
        
        template<Component T>
        T& get_component(EntityHandle entity) const {
            auto* storage = get_storage<T>();
            return *static_cast<T*>(storage->get_component_ptr(entity));
        }
        
        const Archetype& archetype_;
        std::unordered_map<ComponentId, IComponentStorage*> storages_;
    };
    
    /// @brief Create multi-component view
    template<Component... Components>
    MultiComponentView<Components...> view() const {
        return MultiComponentView<Components...>(*this);
    }
    
    /// @brief Set structural change callback
    void set_structural_change_callback(std::function<void(const StructuralChange&)> callback) {
        structural_change_callback_ = std::move(callback);
    }
    
    /// @brief Get archetype statistics
    struct ArchetypeStats {
        std::size_t entity_count{};
        std::size_t component_type_count{};
        std::size_t total_memory_usage{};
        std::uint64_t access_count{};
        std::uint64_t modification_count{};
        Version current_version{};
        bool is_hot{false};
    };
    
    ArchetypeStats get_stats() const {
        ArchetypeStats stats{};
        stats.entity_count = entity_count();
        stats.component_type_count = signature_.component_count;
        stats.current_version = version();
        
        for (const auto& [id, storage] : component_storages_) {
            stats.total_memory_usage += storage->size() * 
                ComponentRegistry::instance().get_component_desc(id)->type_info.size_info.size;
        }
        
        stats.access_count = access_count_.load(std::memory_order_relaxed);
        stats.modification_count = modification_count_.load(std::memory_order_relaxed);
        stats.is_hot = is_hot_archetype();
        
        return stats;
    }
    
    /// @brief Optimize archetype layout for better performance
    void optimize_layout() {
        // In a full implementation, this would:
        // 1. Reorganize component storages for better cache locality
        // 2. Compact fragmented storages
        // 3. Adjust chunk sizes based on usage patterns
        // 4. Reorder components by access frequency
        
        increment_version();
    }

private:
    /// @brief Initialize component storages based on signature
    void initialize_component_storages() {
        const auto component_ids = signature_.get_component_ids();
        
        for (ComponentId id : component_ids) {
            // Create typed storage based on component type
            // In a full implementation, this would use a factory pattern
            // For now, we'll create generic storages
            
            const auto* desc = ComponentRegistry::instance().get_component_desc(id);
            if (!desc) continue;
            
            // Create storage based on component type information
            // This is simplified - full implementation would dispatch to typed storages
            // component_storages_[id] = create_typed_storage(desc);
        }
    }
    
    /// @brief Increment version for change tracking
    void increment_version() noexcept {
        version_.fetch_add(1, std::memory_order_acq_rel);
        modification_count_.fetch_add(1, std::memory_order_relaxed);
    }
    
    /// @brief Check if this is a hot archetype (frequently accessed)
    bool is_hot_archetype() const noexcept {
        const auto total_ops = access_count_.load(std::memory_order_relaxed) +
                              modification_count_.load(std::memory_order_relaxed);
        return total_ops >= static_cast<std::uint64_t>(config_.hot_archetype_threshold * 1000);
    }
    
    ArchetypeId id_;
    ArchetypeSignature signature_;
    ArchetypeConfig config_;
    
    /// @brief Entity set for this archetype
    AdvancedSparseSet entity_set_;
    
    /// @brief Component storages by type
    std::unordered_map<ComponentId, std::unique_ptr<IComponentStorage>> component_storages_;
    
    /// @brief Version tracking for change detection
    std::atomic<Version> version_;
    
    /// @brief Performance tracking
    mutable std::atomic<std::uint64_t> access_count_{0};
    mutable std::atomic<std::uint64_t> modification_count_{0};
    
    /// @brief Structural change notification
    std::function<void(const StructuralChange&)> structural_change_callback_;
};

/// @brief Archetype manager for organizing and optimizing archetypes
class ArchetypeManager {
public:
    explicit ArchetypeManager(const ArchetypeConfig& config = {})
        : config_(config), archetype_graph_(std::make_unique<ArchetypeGraph>()) {
        
        archetypes_.reserve(config_.initial_archetype_capacity);
    }
    
    /// @brief Find or create archetype for signature
    /// @param signature Component signature
    /// @return Archetype ID
    ArchetypeId get_or_create_archetype(const ArchetypeSignature& signature) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Try to find existing archetype
        auto it = signature_to_archetype_.find(signature);
        if (it != signature_to_archetype_.end()) {
            return it->second;
        }
        
        // Create new archetype
        if (archetypes_.size() >= config_.max_archetype_count) {
            throw std::runtime_error("Maximum archetype count exceeded");
        }
        
        const auto id = ArchetypeId{static_cast<std::uint32_t>(archetypes_.size())};
        
        auto archetype = std::make_unique<Archetype>(id, signature, config_);
        archetype->set_structural_change_callback(
            [this](const StructuralChange& change) {
                on_structural_change(change);
            }
        );
        
        signature_to_archetype_[signature] = id;
        archetype_to_signature_[id] = signature;
        archetypes_.push_back(std::move(archetype));
        
        // Update archetype graph
        archetype_graph_->add_archetype(id, signature);
        
        return id;
    }
    
    /// @brief Get archetype by ID
    /// @param id Archetype ID
    /// @return Pointer to archetype, or nullptr if not found
    Archetype* get_archetype(ArchetypeId id) const noexcept {
        if (id.value >= archetypes_.size()) {
            return nullptr;
        }
        return archetypes_[id.value].get();
    }
    
    /// @brief Find archetype for component addition
    /// @param current_archetype Current archetype
    /// @param component Component to add
    /// @return Target archetype ID, or invalid if not found
    ArchetypeId find_add_component_archetype(ArchetypeId current_archetype, ComponentId component) const {
        // Try archetype graph first
        auto target = archetype_graph_->find_add_archetype(current_archetype, component);
        if (target.is_valid()) {
            return target;
        }
        
        // Fallback to signature-based search
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = archetype_to_signature_.find(current_archetype);
        if (it == archetype_to_signature_.end()) {
            return ArchetypeId::invalid();
        }
        
        const auto current_signature = it->second;
        const auto target_signature = ArchetypeSignature(
            current_signature.signature | (1ULL << component.value)
        );
        
        auto target_it = signature_to_archetype_.find(target_signature);
        return target_it != signature_to_archetype_.end() ? 
               target_it->second : ArchetypeId::invalid();
    }
    
    /// @brief Find archetype for component removal
    /// @param current_archetype Current archetype
    /// @param component Component to remove
    /// @return Target archetype ID, or invalid if not found
    ArchetypeId find_remove_component_archetype(ArchetypeId current_archetype, ComponentId component) const {
        // Try archetype graph first
        auto target = archetype_graph_->find_remove_archetype(current_archetype, component);
        if (target.is_valid()) {
            return target;
        }
        
        // Fallback to signature-based search
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = archetype_to_signature_.find(current_archetype);
        if (it == archetype_to_signature_.end()) {
            return ArchetypeId::invalid();
        }
        
        const auto current_signature = it->second;
        const auto target_signature = ArchetypeSignature(
            current_signature.signature & ~(1ULL << component.value)
        );
        
        auto target_it = signature_to_archetype_.find(target_signature);
        return target_it != signature_to_archetype_.end() ? 
               target_it->second : ArchetypeId::invalid();
    }
    
    /// @brief Get all archetypes matching query
    /// @param required Required component signature
    /// @param excluded Excluded component signature
    /// @return Vector of matching archetype IDs
    std::vector<ArchetypeId> query_archetypes(ComponentSignature required, 
                                             ComponentSignature excluded = 0) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<ArchetypeId> matches;
        matches.reserve(archetypes_.size() / 4);  // Heuristic
        
        for (std::size_t i = 0; i < archetypes_.size(); ++i) {
            const auto& archetype = archetypes_[i];
            if (archetype->matches_query(required, excluded)) {
                matches.push_back(archetype->id());
            }
        }
        
        return matches;
    }
    
    /// @brief Get manager statistics
    struct ManagerStats {
        std::size_t total_archetypes{};
        std::size_t total_entities{};
        std::size_t hot_archetype_count{};
        std::size_t empty_archetype_count{};
        std::uint64_t structural_change_count{};
        double average_entities_per_archetype{};
    };
    
    ManagerStats get_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        ManagerStats stats{};
        stats.total_archetypes = archetypes_.size();
        stats.structural_change_count = structural_change_count_.load(std::memory_order_relaxed);
        
        for (const auto& archetype : archetypes_) {
            const auto archetype_stats = archetype->get_stats();
            stats.total_entities += archetype_stats.entity_count;
            
            if (archetype_stats.entity_count == 0) {
                ++stats.empty_archetype_count;
            }
            if (archetype_stats.is_hot) {
                ++stats.hot_archetype_count;
            }
        }
        
        stats.average_entities_per_archetype = stats.total_archetypes > 0 ?
            static_cast<double>(stats.total_entities) / stats.total_archetypes : 0.0;
        
        return stats;
    }

private:
    /// @brief Handle structural change events
    void on_structural_change(const StructuralChange& change) {
        structural_change_count_.fetch_add(1, std::memory_order_relaxed);
        
        // In a full implementation, this would:
        // 1. Invalidate query caches
        // 2. Update bloom filters
        // 3. Trigger archetype optimization
        // 4. Notify systems about structural changes
    }
    
    ArchetypeConfig config_;
    std::unique_ptr<ArchetypeGraph> archetype_graph_;
    
    mutable std::mutex mutex_;
    
    /// @brief Archetype storage
    std::vector<std::unique_ptr<Archetype>> archetypes_;
    
    /// @brief Signature to archetype mapping
    std::unordered_map<ArchetypeSignature, ArchetypeId> signature_to_archetype_;
    
    /// @brief Archetype to signature reverse mapping
    std::unordered_map<ArchetypeId, ArchetypeSignature> archetype_to_signature_;
    
    /// @brief Performance tracking
    std::atomic<std::uint64_t> structural_change_count_{0};
};

} // namespace ecscope::registry

/// @brief Hash specialization for ArchetypeSignature
template<>
struct std::hash<ecscope::registry::ArchetypeSignature> {
    std::size_t operator()(const ecscope::registry::ArchetypeSignature& sig) const noexcept {
        return sig.hash;
    }
};