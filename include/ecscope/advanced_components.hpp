#pragma once

/**
 * @file advanced_components.hpp
 * @brief Advanced ECS Component Patterns and Types
 * 
 * This comprehensive system provides sophisticated component patterns that
 * demonstrate advanced ECS techniques for educational purposes. It showcases
 * different component design patterns and their trade-offs.
 * 
 * Key Educational Features:
 * - Tag components for zero-size entity marking
 * - Singleton components for global state management
 * - Component variants using std::variant
 * - Component serialization and deserialization framework
 * - Component factories and builders
 * - Component validation and constraints
 * - Component versioning and migration
 * - Component reflection and introspection
 * 
 * Component Pattern Types:
 * - Tag Components: Zero-size markers for entity classification
 * - Data Components: Traditional data storage components
 * - Singleton Components: Global shared state components
 * - Variant Components: Type-safe unions of different component types
 * - Reference Components: Components that reference other entities
 * - Event Components: Components that trigger events
 * - Temporal Components: Components with lifecycle and expiration
 * 
 * Advanced Features:
 * - Component composition and inheritance patterns
 * - Component dependency injection
 * - Component validation and schema enforcement
 * - Component change tracking and dirty flagging
 * - Component serialization with versioning
 * - Component hot-reloading and live editing
 * - Component performance profiling and optimization
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "memory/arena.hpp"
#include "memory/memory_tracker.hpp"
#include <variant>
#include <optional>
#include <any>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <memory>
#include <typeindex>
#include <chrono>
#include <concepts>

namespace ecscope::ecs {

// Forward declarations
class Registry;
class ComponentManager;

/**
 * @brief Component pattern categories for classification
 */
enum class ComponentPattern : u8 {
    Data = 0,               // Traditional data storage
    Tag,                    // Zero-size marker
    Singleton,              // Global shared state
    Variant,                // Type-safe union
    Reference,              // Entity reference
    Event,                  // Event trigger
    Temporal,               // Has lifecycle/expiration
    Composite,              // Composed of other components
    Factory,                // Creates other components
    Proxy                   // Proxy to external data
};

/**
 * @brief Component lifecycle state
 */
enum class ComponentState : u8 {
    Uninitialized = 0,      // Not yet initialized
    Active,                 // Active and valid
    Disabled,               // Temporarily disabled
    Expired,                // Has expired (temporal components)
    Invalid,                // Invalid/corrupted
    Destroying             // Being destroyed
};

/**
 * @brief Component metadata for advanced features
 */
struct ComponentMetadata {
    std::string name;                   // Human-readable name
    std::string description;            // Component description
    ComponentPattern pattern;           // Pattern type
    ComponentState state;               // Current state
    u32 version;                       // Component version
    f64 creation_time;                 // When created
    f64 last_modified_time;            // Last modification
    f64 expiration_time;               // When expires (if temporal)
    
    // Validation
    std::vector<std::string> required_components;  // Dependencies
    std::vector<std::string> conflicting_components; // Conflicts
    std::function<bool(const void*)> validator;    // Custom validation
    
    // Serialization
    std::function<std::string(const void*)> serializer;     // Serialize to string
    std::function<void*(const std::string&)> deserializer;  // Deserialize from string
    
    // Change tracking
    bool is_dirty;                     // Has been modified
    u64 change_flags;                  // Specific changes
    std::function<void()> change_callback; // Called on changes
    
    ComponentMetadata() noexcept
        : pattern(ComponentPattern::Data)
        , state(ComponentState::Uninitialized)
        , version(1)
        , creation_time(0.0)
        , last_modified_time(0.0)
        , expiration_time(0.0)
        , is_dirty(false)
        , change_flags(0) {}
};

/**
 * @brief Base interface for advanced component features
 */
class IAdvancedComponent {
public:
    virtual ~IAdvancedComponent() = default;
    
    // Lifecycle
    virtual bool initialize() { return true; }
    virtual void update(f64 delta_time) {}
    virtual void shutdown() {}
    
    // State management
    virtual ComponentState get_state() const = 0;
    virtual void set_state(ComponentState state) = 0;
    
    // Validation
    virtual bool is_valid() const { return get_state() != ComponentState::Invalid; }
    virtual std::vector<std::string> validate() const { return {}; }
    
    // Change tracking
    virtual bool is_dirty() const = 0;
    virtual void mark_clean() = 0;
    virtual void mark_dirty() = 0;
    virtual u64 get_change_flags() const = 0;
    
    // Serialization
    virtual std::string serialize() const = 0;
    virtual bool deserialize(const std::string& data) = 0;
    
    // Metadata
    virtual ComponentMetadata& metadata() = 0;
    virtual const ComponentMetadata& metadata() const = 0;
    
    // Type information
    virtual std::type_index type_index() const = 0;
    virtual const char* type_name() const = 0;
};

/**
 * @brief CRTP base for advanced components
 */
template<typename Derived>
class AdvancedComponent : public IAdvancedComponent {
private:
    ComponentMetadata metadata_;
    ComponentState state_;
    bool is_dirty_;
    u64 change_flags_;
    
protected:
    // Change flag helpers
    void set_change_flag(u8 flag_bit) {
        change_flags_ |= (1ULL << flag_bit);
        mark_dirty();
    }
    
    void clear_change_flag(u8 flag_bit) {
        change_flags_ &= ~(1ULL << flag_bit);
    }
    
    bool has_change_flag(u8 flag_bit) const {
        return (change_flags_ & (1ULL << flag_bit)) != 0;
    }
    
public:
    AdvancedComponent() 
        : state_(ComponentState::Uninitialized)
        , is_dirty_(true)
        , change_flags_(0) {
        
        metadata_.name = typeid(Derived).name();
        metadata_.creation_time = std::chrono::duration<f64>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    
    // IAdvancedComponent implementation
    ComponentState get_state() const override { return state_; }
    void set_state(ComponentState state) override { 
        if (state_ != state) {
            state_ = state; 
            mark_dirty();
        }
    }
    
    bool is_dirty() const override { return is_dirty_; }
    void mark_clean() override { 
        is_dirty_ = false; 
        change_flags_ = 0;
    }
    void mark_dirty() override { 
        is_dirty_ = true; 
        metadata_.last_modified_time = std::chrono::duration<f64>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        if (metadata_.change_callback) {
            metadata_.change_callback();
        }
    }
    u64 get_change_flags() const override { return change_flags_; }
    
    ComponentMetadata& metadata() override { return metadata_; }
    const ComponentMetadata& metadata() const override { return metadata_; }
    
    std::type_index type_index() const override { return std::type_index(typeid(Derived)); }
    const char* type_name() const override { return typeid(Derived).name(); }
    
    // Default serialization (can be overridden)
    std::string serialize() const override {
        if (metadata_.serializer) {
            return metadata_.serializer(static_cast<const Derived*>(this));
        }
        return "{}";  // Empty JSON object
    }
    
    bool deserialize(const std::string& data) override {
        if (metadata_.deserializer) {
            try {
                void* result = metadata_.deserializer(data);
                if (result) {
                    *static_cast<Derived*>(this) = *static_cast<Derived*>(result);
                    return true;
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Component deserialization failed: {}", e.what());
            }
        }
        return false;
    }
    
    // Convenience methods for derived classes
protected:
    void set_pattern(ComponentPattern pattern) { metadata_.pattern = pattern; }
    void set_description(const std::string& desc) { metadata_.description = desc; }
    void set_expiration_time(f64 time) { metadata_.expiration_time = time; }
    
    bool is_expired() const {
        if (metadata_.expiration_time <= 0.0) return false;
        f64 current_time = std::chrono::duration<f64>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        return current_time >= metadata_.expiration_time;
    }
};

/**
 * @brief Tag component pattern - zero-size markers
 * 
 * Tag components are used to mark entities with specific characteristics
 * without storing any data. They are memory-efficient and perfect for
 * entity classification and filtering.
 */
template<typename Tag>
struct TagComponent : public AdvancedComponent<TagComponent<Tag>> {
    static_assert(std::is_empty_v<Tag>, "Tag must be an empty type");
    
    TagComponent() {
        this->set_pattern(ComponentPattern::Tag);
        this->set_description("Tag component for entity classification");
        this->set_state(ComponentState::Active);
    }
    
    // Tag components are always valid
    std::vector<std::string> validate() const override { return {}; }
    
    // Tag-specific methods
    static constexpr bool is_tag() { return true; }
    static constexpr usize size() { return 0; }
    
    // Serialization for tags is trivial
    std::string serialize() const override {
        return "{\"tag\":\"" + std::string(typeid(Tag).name()) + "\"}";
    }
    
    bool deserialize(const std::string& data) override {
        // Tags don't need deserialization data
        return true;
    }
};

// Common tag types
struct PlayerTag {};
struct EnemyTag {};
struct NPCTag {};
struct DeadTag {};
struct DisabledTag {};
struct SelectedTag {};
struct VisibleTag {};
struct CollidableTag {};
struct StaticTag {};
struct DynamicTag {};

// Convenience aliases for common tags
using Player = TagComponent<PlayerTag>;
using Enemy = TagComponent<EnemyTag>;
using NPC = TagComponent<NPCTag>;
using Dead = TagComponent<DeadTag>;
using Disabled = TagComponent<DisabledTag>;
using Selected = TagComponent<SelectedTag>;
using Visible = TagComponent<VisibleTag>;
using Collidable = TagComponent<CollidableTag>;
using Static = TagComponent<StaticTag>;
using Dynamic = TagComponent<DynamicTag>;

/**
 * @brief Singleton component pattern - global shared state
 * 
 * Singleton components maintain global state that is shared across
 * the entire ECS system. Only one instance exists per component type.
 */
template<typename Data>
class SingletonComponent : public AdvancedComponent<SingletonComponent<Data>> {
private:
    static inline std::optional<Data> instance_;
    static inline std::mutex instance_mutex_;
    static inline bool is_initialized_;
    
public:
    SingletonComponent() {
        this->set_pattern(ComponentPattern::Singleton);
        this->set_description("Singleton component for global state");
        
        std::lock_guard lock(instance_mutex_);
        if (!is_initialized_) {
            instance_ = Data{};
            is_initialized_ = true;
            this->set_state(ComponentState::Active);
        }
    }
    
    // Access to singleton data
    static Data& get() {
        std::lock_guard lock(instance_mutex_);
        if (!instance_.has_value()) {
            instance_ = Data{};
        }
        return instance_.value();
    }
    
    static const Data& get_const() {
        return get();
    }
    
    static void set(const Data& data) {
        std::lock_guard lock(instance_mutex_);
        instance_ = data;
    }
    
    static void reset() {
        std::lock_guard lock(instance_mutex_);
        instance_.reset();
        is_initialized_ = false;
    }
    
    static bool exists() {
        std::lock_guard lock(instance_mutex_);
        return instance_.has_value();
    }
    
    // Component interface
    std::vector<std::string> validate() const override {
        std::vector<std::string> errors;
        if (!exists()) {
            errors.push_back("Singleton instance not initialized");
        }
        return errors;
    }
    
    bool initialize() override {
        if (!exists()) {
            std::lock_guard lock(instance_mutex_);
            instance_ = Data{};
            is_initialized_ = true;
        }
        this->set_state(ComponentState::Active);
        return true;
    }
    
    void shutdown() override {
        reset();
        this->set_state(ComponentState::Uninitialized);
    }
    
    std::string serialize() const override {
        if (this->metadata().serializer && exists()) {
            return this->metadata().serializer(&get());
        }
        return "{}";
    }
    
    bool deserialize(const std::string& data) override {
        if (this->metadata().deserializer) {
            try {
                void* result = this->metadata().deserializer(data);
                if (result) {
                    set(*static_cast<Data*>(result));
                    return true;
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Singleton component deserialization failed: {}", e.what());
            }
        }
        return false;
    }
};

/**
 * @brief Variant component pattern - type-safe unions
 * 
 * Variant components can hold one of several different types,
 * providing type-safe storage for components that can have
 * different representations or behaviors.
 */
template<typename... Types>
class VariantComponent : public AdvancedComponent<VariantComponent<Types...>> {
private:
    std::variant<Types...> data_;
    
public:
    using VariantType = std::variant<Types...>;
    static constexpr usize type_count = sizeof...(Types);
    
    VariantComponent() {
        this->set_pattern(ComponentPattern::Variant);
        this->set_description("Variant component for type-safe unions");
        
        // Initialize with first type's default constructor
        if constexpr (sizeof...(Types) > 0) {
            data_ = std::get<0>(std::variant<Types...>{});
        }
        this->set_state(ComponentState::Active);
    }
    
    template<typename T>
    explicit VariantComponent(const T& value) 
        : data_(value) {
        static_assert((std::is_same_v<T, Types> || ...), "Type must be one of the variant types");
        this->set_pattern(ComponentPattern::Variant);
        this->set_state(ComponentState::Active);
    }
    
    // Access methods
    template<typename T>
    T& get() {
        static_assert((std::is_same_v<T, Types> || ...), "Type must be one of the variant types");
        if (!holds<T>()) {
            throw std::bad_variant_access{};
        }
        return std::get<T>(data_);
    }
    
    template<typename T>
    const T& get() const {
        static_assert((std::is_same_v<T, Types> || ...), "Type must be one of the variant types");
        if (!holds<T>()) {
            throw std::bad_variant_access{};
        }
        return std::get<T>(data_);
    }
    
    template<typename T>
    T* get_if() noexcept {
        static_assert((std::is_same_v<T, Types> || ...), "Type must be one of the variant types");
        return std::get_if<T>(&data_);
    }
    
    template<typename T>
    const T* get_if() const noexcept {
        static_assert((std::is_same_v<T, Types> || ...), "Type must be one of the variant types");
        return std::get_if<T>(&data_);
    }
    
    template<typename T>
    bool holds() const noexcept {
        static_assert((std::is_same_v<T, Types> || ...), "Type must be one of the variant types");
        return std::holds_alternative<T>(data_);
    }
    
    template<typename T>
    void set(const T& value) {
        static_assert((std::is_same_v<T, Types> || ...), "Type must be one of the variant types");
        if (!holds<T>() || get<T>() != value) {
            data_ = value;
            this->mark_dirty();
        }
    }
    
    template<typename T, typename... Args>
    T& emplace(Args&&... args) {
        static_assert((std::is_same_v<T, Types> || ...), "Type must be one of the variant types");
        T& result = data_.template emplace<T>(std::forward<Args>(args)...);
        this->mark_dirty();
        return result;
    }
    
    // Visitor pattern support
    template<typename Visitor>
    auto visit(Visitor&& visitor) {
        return std::visit(std::forward<Visitor>(visitor), data_);
    }
    
    template<typename Visitor>
    auto visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), data_);
    }
    
    // Type information
    usize index() const noexcept {
        return data_.index();
    }
    
    std::string current_type_name() const {
        return visit([](const auto& value) -> std::string {
            return typeid(value).name();
        });
    }
    
    // Component interface
    std::vector<std::string> validate() const override {
        std::vector<std::string> errors;
        
        // Validate that variant holds a valid alternative
        if (data_.valueless_by_exception()) {
            errors.push_back("Variant is in exceptional state");
        }
        
        return errors;
    }
    
    std::string serialize() const override {
        return visit([this](const auto& value) -> std::string {
            if (this->metadata().serializer) {
                return this->metadata().serializer(&value);
            }
            return "{\"type\":\"" + std::string(typeid(value).name()) + "\"}";
        });
    }
    
    bool deserialize(const std::string& data) override {
        // Variant deserialization is complex and type-dependent
        // This would need type registry for proper implementation
        LOG_WARN("Variant component deserialization not fully implemented");
        return false;
    }
};

/**
 * @brief Reference component pattern - entity references
 * 
 * Reference components store references to other entities,
 * providing a way to create relationships and associations
 * between entities in the ECS system.
 */
template<typename RefData = void>
class ReferenceComponent : public AdvancedComponent<ReferenceComponent<RefData>> {
private:
    Entity target_entity_;
    std::conditional_t<std::is_void_v<RefData>, std::monostate, RefData> data_;
    
public:
    ReferenceComponent() 
        : target_entity_(Entity::invalid()) {
        this->set_pattern(ComponentPattern::Reference);
        this->set_description("Reference component for entity associations");
        this->set_state(ComponentState::Uninitialized);
    }
    
    explicit ReferenceComponent(Entity target) 
        : target_entity_(target) {
        this->set_pattern(ComponentPattern::Reference);
        this->set_state(target.is_valid() ? ComponentState::Active : ComponentState::Invalid);
    }
    
    ReferenceComponent(Entity target, const RefData& data) 
        requires (!std::is_void_v<RefData>)
        : target_entity_(target), data_(data) {
        this->set_pattern(ComponentPattern::Reference);
        this->set_state(target.is_valid() ? ComponentState::Active : ComponentState::Invalid);
    }
    
    // Access methods
    Entity target() const noexcept { return target_entity_; }
    
    void set_target(Entity entity) {
        if (target_entity_ != entity) {
            target_entity_ = entity;
            this->set_state(entity.is_valid() ? ComponentState::Active : ComponentState::Invalid);
            this->mark_dirty();
        }
    }
    
    bool has_target() const noexcept { 
        return target_entity_.is_valid(); 
    }
    
    // Data access (if RefData is not void)
    template<typename T = RefData>
    const T& data() const requires (!std::is_void_v<T>) {
        return data_;
    }
    
    template<typename T = RefData>
    T& data() requires (!std::is_void_v<T>) {
        this->mark_dirty();
        return data_;
    }
    
    template<typename T = RefData>
    void set_data(const T& new_data) requires (!std::is_void_v<T>) {
        if (data_ != new_data) {
            data_ = new_data;
            this->mark_dirty();
        }
    }
    
    // Component interface
    std::vector<std::string> validate() const override {
        std::vector<std::string> errors;
        
        if (!target_entity_.is_valid()) {
            errors.push_back("Reference target is invalid");
        }
        
        return errors;
    }
    
    std::string serialize() const override {
        std::ostringstream oss;
        oss << "{\"target\":" << target_entity_.id();
        
        if constexpr (!std::is_void_v<RefData>) {
            if (this->metadata().serializer) {
                oss << ",\"data\":" << this->metadata().serializer(&data_);
            }
        }
        
        oss << "}";
        return oss.str();
    }
    
    bool deserialize(const std::string& data) override {
        // Simplified deserialization - would need proper JSON parsing
        LOG_WARN("Reference component deserialization not fully implemented");
        return false;
    }
};

// Common reference component types
using EntityReference = ReferenceComponent<void>;
using OwnerReference = ReferenceComponent<std::string>;  // With owner name
using ParentReference = ReferenceComponent<void>;
using ChildReference = ReferenceComponent<u32>;  // With child index

/**
 * @brief Temporal component pattern - components with lifecycle
 * 
 * Temporal components have a limited lifespan and can expire
 * automatically. They are useful for temporary effects, buffs,
 * debuffs, and other time-limited behaviors.
 */
template<typename Data>
class TemporalComponent : public AdvancedComponent<TemporalComponent<Data>> {
private:
    Data data_;
    f64 duration_;
    f64 remaining_time_;
    bool auto_destroy_;
    std::function<void()> expiration_callback_;
    
public:
    TemporalComponent(const Data& data, f64 duration, bool auto_destroy = true)
        : data_(data), duration_(duration), remaining_time_(duration), auto_destroy_(auto_destroy) {
        this->set_pattern(ComponentPattern::Temporal);
        this->set_description("Temporal component with expiration");
        this->set_expiration_time(this->metadata().creation_time + duration);
        this->set_state(ComponentState::Active);
    }
    
    // Data access
    const Data& data() const { return data_; }
    Data& data() { 
        this->mark_dirty(); 
        return data_; 
    }
    
    void set_data(const Data& new_data) {
        if (data_ != new_data) {
            data_ = new_data;
            this->mark_dirty();
        }
    }
    
    // Time management
    f64 duration() const noexcept { return duration_; }
    f64 remaining_time() const noexcept { return remaining_time_; }
    f64 elapsed_time() const noexcept { return duration_ - remaining_time_; }
    f64 progress() const noexcept { 
        return duration_ > 0.0 ? (elapsed_time() / duration_) : 1.0; 
    }
    
    bool is_expired() const noexcept { 
        return remaining_time_ <= 0.0; 
    }
    
    bool should_auto_destroy() const noexcept { 
        return auto_destroy_ && is_expired(); 
    }
    
    void set_expiration_callback(std::function<void()> callback) {
        expiration_callback_ = std::move(callback);
    }
    
    // Reset/extend time
    void reset_timer() {
        remaining_time_ = duration_;
        this->set_state(ComponentState::Active);
        this->mark_dirty();
    }
    
    void extend_time(f64 additional_time) {
        remaining_time_ += additional_time;
        if (remaining_time_ > 0.0 && this->get_state() == ComponentState::Expired) {
            this->set_state(ComponentState::Active);
        }
        this->mark_dirty();
    }
    
    void set_remaining_time(f64 time) {
        remaining_time_ = std::max(0.0, time);
        this->set_state(remaining_time_ > 0.0 ? ComponentState::Active : ComponentState::Expired);
        this->mark_dirty();
    }
    
    // Component interface
    void update(f64 delta_time) override {
        if (this->get_state() == ComponentState::Active && remaining_time_ > 0.0) {
            remaining_time_ -= delta_time;
            this->mark_dirty();
            
            if (remaining_time_ <= 0.0) {
                remaining_time_ = 0.0;
                this->set_state(ComponentState::Expired);
                
                if (expiration_callback_) {
                    expiration_callback_();
                }
            }
        }
    }
    
    std::vector<std::string> validate() const override {
        std::vector<std::string> errors;
        
        if (duration_ < 0.0) {
            errors.push_back("Duration cannot be negative");
        }
        
        if (remaining_time_ < 0.0) {
            errors.push_back("Remaining time cannot be negative");
        }
        
        return errors;
    }
    
    std::string serialize() const override {
        std::ostringstream oss;
        oss << "{\"duration\":" << duration_ 
            << ",\"remaining\":" << remaining_time_
            << ",\"auto_destroy\":" << (auto_destroy_ ? "true" : "false");
        
        if (this->metadata().serializer) {
            oss << ",\"data\":" << this->metadata().serializer(&data_);
        }
        
        oss << "}";
        return oss.str();
    }
};

/**
 * @brief Event component pattern - components that trigger events
 * 
 * Event components are used to trigger events or notifications
 * when they are added, modified, or removed from entities.
 * They are useful for reactive programming patterns.
 */
template<typename EventData>
class EventComponent : public AdvancedComponent<EventComponent<EventData>> {
private:
    EventData event_data_;
    bool is_consumed_;
    std::vector<std::function<void(const EventData&)>> handlers_;
    
public:
    explicit EventComponent(const EventData& data)
        : event_data_(data), is_consumed_(false) {
        this->set_pattern(ComponentPattern::Event);
        this->set_description("Event component for reactive programming");
        this->set_state(ComponentState::Active);
    }
    
    // Event data access
    const EventData& data() const { return event_data_; }
    
    void set_data(const EventData& data) {
        if (event_data_ != data) {
            event_data_ = data;
            is_consumed_ = false;  // Reset consumed flag
            this->mark_dirty();
            trigger_event();
        }
    }
    
    // Event handling
    bool is_consumed() const noexcept { return is_consumed_; }
    
    void consume() {
        is_consumed_ = true;
        this->set_state(ComponentState::Disabled);
        this->mark_dirty();
    }
    
    void reset() {
        is_consumed_ = false;
        this->set_state(ComponentState::Active);
        this->mark_dirty();
    }
    
    void add_handler(std::function<void(const EventData&)> handler) {
        handlers_.push_back(std::move(handler));
    }
    
    void clear_handlers() {
        handlers_.clear();
    }
    
    // Trigger event manually
    void trigger_event() {
        if (!is_consumed_) {
            for (const auto& handler : handlers_) {
                try {
                    handler(event_data_);
                } catch (const std::exception& e) {
                    LOG_ERROR("Event handler exception: {}", e.what());
                }
            }
        }
    }
    
    // Component interface
    bool initialize() override {
        this->set_state(ComponentState::Active);
        trigger_event();  // Trigger event on initialization
        return true;
    }
    
    std::vector<std::string> validate() const override {
        // Events are generally always valid
        return {};
    }
    
    std::string serialize() const override {
        std::ostringstream oss;
        oss << "{\"consumed\":" << (is_consumed_ ? "true" : "false");
        
        if (this->metadata().serializer) {
            oss << ",\"data\":" << this->metadata().serializer(&event_data_);
        }
        
        oss << "}";
        return oss.str();
    }
};

/**
 * @brief Component factory for creating components with specific patterns
 */
class ComponentFactory {
private:
    std::unordered_map<std::string, std::function<std::unique_ptr<IAdvancedComponent>()>> creators_;
    std::unordered_map<std::type_index, std::string> type_names_;
    
public:
    // Register component creators
    template<typename ComponentType>
    void register_component(const std::string& name) {
        creators_[name] = []() { 
            return std::make_unique<ComponentType>(); 
        };
        type_names_[std::type_index(typeid(ComponentType))] = name;
    }
    
    template<typename ComponentType, typename... Args>
    void register_component_with_args(const std::string& name, Args... default_args) {
        creators_[name] = [default_args...]() { 
            return std::make_unique<ComponentType>(default_args...); 
        };
        type_names_[std::type_index(typeid(ComponentType))] = name;
    }
    
    // Create components
    std::unique_ptr<IAdvancedComponent> create(const std::string& name) const {
        auto it = creators_.find(name);
        if (it != creators_.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    template<typename ComponentType>
    std::unique_ptr<ComponentType> create_typed(const std::string& name) const {
        auto component = create(name);
        return std::unique_ptr<ComponentType>(dynamic_cast<ComponentType*>(component.release()));
    }
    
    // Information
    std::vector<std::string> get_registered_names() const {
        std::vector<std::string> names;
        names.reserve(creators_.size());
        for (const auto& [name, creator] : creators_) {
            names.push_back(name);
        }
        return names;
    }
    
    template<typename ComponentType>
    std::string get_type_name() const {
        auto it = type_names_.find(std::type_index(typeid(ComponentType)));
        return it != type_names_.end() ? it->second : "";
    }
    
    bool is_registered(const std::string& name) const {
        return creators_.find(name) != creators_.end();
    }
};

/**
 * @brief Component manager for advanced component patterns
 */
class AdvancedComponentManager {
private:
    std::unordered_map<Entity, std::vector<std::unique_ptr<IAdvancedComponent>>> entity_components_;
    std::unordered_map<std::type_index, std::vector<Entity>> components_by_type_;
    ComponentFactory factory_;
    
    // Memory management
    std::unique_ptr<memory::ArenaAllocator> component_arena_;
    u32 allocator_id_;
    
    // Statistics
    mutable std::unordered_map<std::type_index, usize> component_counts_;
    mutable usize total_components_;
    
public:
    explicit AdvancedComponentManager(usize arena_size = 4 * 1024 * 1024);  // 4MB
    ~AdvancedComponentManager();
    
    // Component management
    template<typename ComponentType, typename... Args>
    ComponentType* add_component(Entity entity, Args&&... args) {
        auto component = std::make_unique<ComponentType>(std::forward<Args>(args)...);
        ComponentType* component_ptr = component.get();
        
        entity_components_[entity].push_back(std::move(component));
        components_by_type_[std::type_index(typeid(ComponentType))].push_back(entity);
        
        component_ptr->initialize();
        return component_ptr;
    }
    
    template<typename ComponentType>
    bool remove_component(Entity entity) {
        auto entity_it = entity_components_.find(entity);
        if (entity_it == entity_components_.end()) {
            return false;
        }
        
        auto& components = entity_it->second;
        auto component_it = std::find_if(components.begin(), components.end(),
            [](const auto& comp) {
                return comp->type_index() == std::type_index(typeid(ComponentType));
            });
        
        if (component_it != components.end()) {
            (*component_it)->shutdown();
            components.erase(component_it);
            
            // Remove from type index
            auto& type_entities = components_by_type_[std::type_index(typeid(ComponentType))];
            auto type_it = std::find(type_entities.begin(), type_entities.end(), entity);
            if (type_it != type_entities.end()) {
                type_entities.erase(type_it);
            }
            
            return true;
        }
        
        return false;
    }
    
    template<typename ComponentType>
    ComponentType* get_component(Entity entity) {
        auto entity_it = entity_components_.find(entity);
        if (entity_it == entity_components_.end()) {
            return nullptr;
        }
        
        for (const auto& component : entity_it->second) {
            if (component->type_index() == std::type_index(typeid(ComponentType))) {
                return dynamic_cast<ComponentType*>(component.get());
            }
        }
        
        return nullptr;
    }
    
    template<typename ComponentType>
    bool has_component(Entity entity) const {
        return get_component<ComponentType>(entity) != nullptr;
    }
    
    // Bulk operations
    void update_all_components(f64 delta_time);
    void validate_all_components() const;
    void cleanup_expired_components();
    
    // Component factory access
    ComponentFactory& factory() { return factory_; }
    const ComponentFactory& factory() const { return factory_; }
    
    // Entity management
    void remove_all_components(Entity entity);
    std::vector<IAdvancedComponent*> get_all_components(Entity entity);
    
    // Query operations
    template<typename ComponentType>
    std::vector<Entity> get_entities_with_component() const {
        auto it = components_by_type_.find(std::type_index(typeid(ComponentType)));
        return it != components_by_type_.end() ? it->second : std::vector<Entity>{};
    }
    
    // Statistics
    usize get_total_component_count() const;
    std::unordered_map<std::string, usize> get_component_type_counts() const;
    usize get_memory_usage() const;
    
    // Serialization
    std::string serialize_entity_components(Entity entity) const;
    bool deserialize_entity_components(Entity entity, const std::string& data);
    
private:
    static u32 next_allocator_id();
    static std::atomic<u32> allocator_id_counter_;
};

// Global component manager instance
AdvancedComponentManager& get_advanced_component_manager();

// Utility functions for component pattern detection
template<typename T>
constexpr bool is_tag_component_v = std::is_base_of_v<TagComponent<T>, T> || 
                                   (std::is_empty_v<T> && std::is_standard_layout_v<T>);

template<typename T>
constexpr bool is_singleton_component_v = requires { typename T::SingletonType; };

template<typename T>
constexpr bool is_variant_component_v = requires { typename T::VariantType; };

template<typename T>
constexpr bool is_reference_component_v = std::is_base_of_v<ReferenceComponent<>, T>;

template<typename T>
constexpr bool is_temporal_component_v = requires(T t) { t.is_expired(); t.remaining_time(); };

template<typename T>
constexpr bool is_event_component_v = requires(T t) { t.is_consumed(); t.trigger_event(); };

// Component pattern detection functions
template<typename T>
ComponentPattern detect_component_pattern() {
    if constexpr (is_tag_component_v<T>) return ComponentPattern::Tag;
    else if constexpr (is_singleton_component_v<T>) return ComponentPattern::Singleton;
    else if constexpr (is_variant_component_v<T>) return ComponentPattern::Variant;
    else if constexpr (is_reference_component_v<T>) return ComponentPattern::Reference;
    else if constexpr (is_temporal_component_v<T>) return ComponentPattern::Temporal;
    else if constexpr (is_event_component_v<T>) return ComponentPattern::Event;
    else return ComponentPattern::Data;
}

} // namespace ecscope::ecs