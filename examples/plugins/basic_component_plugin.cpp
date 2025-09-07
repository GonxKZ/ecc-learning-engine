/**
 * @file examples/plugins/basic_component_plugin.cpp
 * @brief Educational Basic Component Plugin Example
 * 
 * This plugin demonstrates the fundamental concepts of creating custom ECS components
 * in ECScope. It shows how to register components, handle lifecycle events, and
 * provide educational documentation.
 * 
 * Learning Objectives:
 * - Understanding ECS component architecture
 * - Plugin lifecycle management
 * - Component registration and usage
 * - Memory management in plugins
 * - Educational plugin development patterns
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin/plugin_api.hpp"
#include "plugin/plugin_core.hpp"
#include "ecs/component.hpp"
#include <memory>
#include <string>
#include <vector>

using namespace ecscope;
using namespace ecscope::plugin;

//=============================================================================
// Custom Components Definition
//=============================================================================

/**
 * @brief Simple health component for educational purposes
 * 
 * This component demonstrates basic component structure with:
 * - Simple data members
 * - Default initialization
 * - String representation for debugging
 */
struct HealthComponent : public ecs::Component<HealthComponent> {
    f32 current_health{100.0f};
    f32 max_health{100.0f};
    f32 regeneration_rate{1.0f};
    bool is_invulnerable{false};
    
    /**
     * @brief Default constructor
     */
    HealthComponent() = default;
    
    /**
     * @brief Constructor with values
     */
    HealthComponent(f32 max_hp, f32 regen_rate = 1.0f) 
        : current_health(max_hp), max_health(max_hp), regeneration_rate(regen_rate) {}
    
    /**
     * @brief Check if entity is alive
     */
    bool is_alive() const {
        return current_health > 0.0f;
    }
    
    /**
     * @brief Apply damage to health
     */
    void take_damage(f32 damage) {
        if (!is_invulnerable) {
            current_health = std::max(0.0f, current_health - damage);
        }
    }
    
    /**
     * @brief Heal the entity
     */
    void heal(f32 amount) {
        current_health = std::min(max_health, current_health + amount);
    }
    
    /**
     * @brief Get health percentage (0-1)
     */
    f32 get_health_percentage() const {
        return max_health > 0.0f ? current_health / max_health : 0.0f;
    }
    
    /**
     * @brief String representation for debugging
     */
    std::string to_string() const {
        return "HealthComponent{HP: " + std::to_string(current_health) + 
               "/" + std::to_string(max_health) + 
               ", Regen: " + std::to_string(regeneration_rate) + "}";
    }
};

/**
 * @brief Experience and leveling component
 * 
 * Demonstrates more complex component with:
 * - Computed properties
 * - Event triggers (level up)
 * - Complex state management
 */
struct ExperienceComponent : public ecs::Component<ExperienceComponent> {
    u32 current_level{1};
    u32 current_experience{0};
    u32 experience_to_next_level{100};
    f32 experience_multiplier{1.0f};
    
    // Level progression settings
    static constexpr u32 MAX_LEVEL = 100;
    static constexpr f32 LEVEL_SCALING_FACTOR = 1.5f;
    
    /**
     * @brief Default constructor
     */
    ExperienceComponent() = default;
    
    /**
     * @brief Constructor with starting level
     */
    explicit ExperienceComponent(u32 starting_level) : current_level(starting_level) {
        calculate_experience_requirements();
    }
    
    /**
     * @brief Add experience points
     */
    bool add_experience(u32 exp_points) {
        u32 adjusted_exp = static_cast<u32>(exp_points * experience_multiplier);
        current_experience += adjusted_exp;
        
        bool leveled_up = false;
        while (current_experience >= experience_to_next_level && current_level < MAX_LEVEL) {
            current_experience -= experience_to_next_level;
            current_level++;
            calculate_experience_requirements();
            leveled_up = true;
        }
        
        return leveled_up;
    }
    
    /**
     * @brief Get experience progress to next level (0-1)
     */
    f32 get_level_progress() const {
        if (current_level >= MAX_LEVEL) return 1.0f;
        return static_cast<f32>(current_experience) / experience_to_next_level;
    }
    
    /**
     * @brief Get total experience earned
     */
    u32 get_total_experience() const {
        u32 total = current_experience;
        for (u32 level = 1; level < current_level; ++level) {
            total += calculate_experience_for_level(level);
        }
        return total;
    }
    
    /**
     * @brief String representation
     */
    std::string to_string() const {
        return "ExperienceComponent{Level: " + std::to_string(current_level) + 
               ", EXP: " + std::to_string(current_experience) + 
               "/" + std::to_string(experience_to_next_level) + "}";
    }

private:
    void calculate_experience_requirements() {
        experience_to_next_level = calculate_experience_for_level(current_level);
    }
    
    u32 calculate_experience_for_level(u32 level) const {
        return static_cast<u32>(100 * std::pow(LEVEL_SCALING_FACTOR, level - 1));
    }
};

/**
 * @brief Inventory component with educational slot management
 */
struct InventoryComponent : public ecs::Component<InventoryComponent> {
    struct Item {
        std::string name;
        u32 quantity{1};
        f32 weight{0.0f};
        std::string description;
        
        Item() = default;
        Item(const std::string& n, u32 q = 1, f32 w = 0.0f, const std::string& desc = "")
            : name(n), quantity(q), weight(w), description(desc) {}
    };
    
    std::vector<Item> items;
    u32 max_slots{20};
    f32 max_weight{100.0f};
    f32 current_weight{0.0f};
    
    /**
     * @brief Default constructor
     */
    InventoryComponent() = default;
    
    /**
     * @brief Constructor with capacity
     */
    InventoryComponent(u32 slots, f32 weight_limit) 
        : max_slots(slots), max_weight(weight_limit) {}
    
    /**
     * @brief Add item to inventory
     */
    bool add_item(const Item& item) {
        // Check weight limit
        if (current_weight + item.weight > max_weight) {
            return false; // Too heavy
        }
        
        // Try to stack with existing item
        for (auto& existing_item : items) {
            if (existing_item.name == item.name) {
                existing_item.quantity += item.quantity;
                current_weight += item.weight;
                return true;
            }
        }
        
        // Add as new item if we have slots
        if (items.size() < max_slots) {
            items.push_back(item);
            current_weight += item.weight;
            return true;
        }
        
        return false; // No space
    }
    
    /**
     * @brief Remove item from inventory
     */
    bool remove_item(const std::string& item_name, u32 quantity = 1) {
        for (auto it = items.begin(); it != items.end(); ++it) {
            if (it->name == item_name) {
                if (it->quantity <= quantity) {
                    current_weight -= it->weight;
                    items.erase(it);
                } else {
                    it->quantity -= quantity;
                    current_weight -= it->weight * (static_cast<f32>(quantity) / it->quantity);
                }
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Get item count
     */
    u32 get_item_count(const std::string& item_name) const {
        for (const auto& item : items) {
            if (item.name == item_name) {
                return item.quantity;
            }
        }
        return 0;
    }
    
    /**
     * @brief Check if inventory is full
     */
    bool is_full() const {
        return items.size() >= max_slots;
    }
    
    /**
     * @brief Get weight usage percentage
     */
    f32 get_weight_usage() const {
        return current_weight / max_weight;
    }
    
    /**
     * @brief String representation
     */
    std::string to_string() const {
        return "InventoryComponent{Items: " + std::to_string(items.size()) + 
               "/" + std::to_string(max_slots) + 
               ", Weight: " + std::to_string(current_weight) + 
               "/" + std::to_string(max_weight) + "}";
    }
};

//=============================================================================
// Plugin Implementation
//=============================================================================

/**
 * @brief Basic Component Plugin class
 * 
 * This plugin demonstrates how to create and register custom ECS components
 * with the ECScope engine. It includes educational features and best practices.
 */
class BasicComponentPlugin : public IPlugin {
private:
    PluginMetadata metadata_;
    std::unique_ptr<PluginAPI> api_;
    PluginStats stats_;
    
    // Educational tracking
    std::vector<std::string> component_usage_examples_;
    u32 components_created_{0};

public:
    /**
     * @brief Constructor
     */
    BasicComponentPlugin() {
        initialize_metadata();
        initialize_educational_content();
    }
    
    /**
     * @brief Get plugin metadata
     */
    const PluginMetadata& get_metadata() const override {
        return metadata_;
    }
    
    /**
     * @brief Initialize plugin
     */
    bool initialize() override {
        if (!api_) {
            LOG_ERROR("Plugin API not available during initialization");
            return false;
        }
        
        // Register our custom components
        if (!register_components()) {
            LOG_ERROR("Failed to register plugin components");
            return false;
        }
        
        // Set up educational examples
        setup_educational_examples();
        
        // Add learning notes
        add_educational_content();
        
        LOG_INFO("BasicComponentPlugin initialized successfully");
        LOG_INFO("Registered components: HealthComponent, ExperienceComponent, InventoryComponent");
        
        return true;
    }
    
    /**
     * @brief Shutdown plugin
     */
    void shutdown() override {
        LOG_INFO("BasicComponentPlugin shutting down");
        LOG_INFO("Components created during session: {}", components_created_);
        
        // Cleanup would happen here
        // In a real plugin, you might save statistics, cleanup resources, etc.
    }
    
    /**
     * @brief Update plugin (called every frame)
     */
    void update(f64 delta_time) override {
        // This plugin doesn't need per-frame updates
        // But this is where you'd put any continuous processing
        
        // Update statistics
        auto current_time = std::chrono::system_clock::now();
        stats_.last_activity = current_time;
        stats_.average_frame_time_ms = delta_time;
    }
    
    /**
     * @brief Handle plugin events
     */
    void handle_event(const PluginEvent& event) override {
        switch (event.type) {
            case PluginEventType::ComponentAdded:
                handle_component_added_event(event);
                break;
            case PluginEventType::EntityCreated:
                handle_entity_created_event(event);
                break;
            case PluginEventType::ConfigurationChanged:
                handle_configuration_changed_event(event);
                break;
            default:
                break;
        }
    }
    
    /**
     * @brief Get plugin configuration
     */
    std::unordered_map<std::string, std::string> get_config() const override {
        return {
            {"max_health_default", "100.0"},
            {"experience_multiplier", "1.0"},
            {"inventory_default_slots", "20"},
            {"inventory_default_weight_limit", "100.0"},
            {"enable_educational_mode", "true"},
            {"log_component_creation", "true"}
        };
    }
    
    /**
     * @brief Set plugin configuration
     */
    void set_config(const std::unordered_map<std::string, std::string>& config) override {
        // Handle configuration updates
        for (const auto& [key, value] : config) {
            api_->set_config(key, value);
            LOG_INFO("Configuration updated: {} = {}", key, value);
        }
    }
    
    /**
     * @brief Validate plugin state
     */
    bool validate() const override {
        // Check that our components are properly registered
        auto& registry = api_->get_registry();
        
        bool health_registered = registry.has_service("HealthComponent");
        bool experience_registered = registry.has_service("ExperienceComponent");
        bool inventory_registered = registry.has_service("InventoryComponent");
        
        if (!health_registered || !experience_registered || !inventory_registered) {
            LOG_WARN("Some components are not properly registered");
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief Get plugin statistics
     */
    PluginStats get_stats() const override {
        auto current_stats = stats_;
        current_stats.total_function_calls = components_created_;
        return current_stats;
    }
    
    /**
     * @brief Educational: Explain plugin functionality
     */
    std::string explain_functionality() const override {
        return R"(
=== Basic Component Plugin Educational Overview ===

This plugin demonstrates fundamental ECS component creation in ECScope.

Key Concepts Demonstrated:
1. Component Structure - How to define ECS components with data and methods
2. Plugin Lifecycle - Proper initialization, update, and shutdown patterns
3. Registration Process - How plugins register components with the engine
4. Educational Integration - Providing learning resources and examples

Components Provided:
• HealthComponent - Basic health/damage system with regeneration
• ExperienceComponent - Level progression with configurable scaling  
• InventoryComponent - Item management with weight and slot limits

Learning Objectives:
- Understand ECS architecture and component design
- Learn plugin development best practices
- See real-world examples of component implementation
- Practice with event handling and configuration

This plugin serves as a template for creating your own component plugins.
Study the code to understand the patterns and adapt them for your needs.
        )";
    }
    
    /**
     * @brief Educational: Get learning resources
     */
    std::vector<std::string> get_learning_resources() const override {
        return {
            "Component Design Patterns Guide",
            "ECS Architecture Fundamentals",
            "Plugin Development Tutorial",
            "Memory Management in Components",
            "Event-Driven Component Communication",
            "Performance Optimization for Components",
            "Component Testing Strategies"
        };
    }
    
    /**
     * @brief Set API reference (called by plugin manager)
     */
    void set_api(std::unique_ptr<PluginAPI> api) {
        api_ = std::move(api);
    }

private:
    /**
     * @brief Initialize plugin metadata
     */
    void initialize_metadata() {
        metadata_.name = "BasicComponentPlugin";
        metadata_.display_name = "Basic Component Examples";
        metadata_.description = "Educational plugin demonstrating fundamental ECS component creation";
        metadata_.version = PluginVersion(1, 0, 0);
        metadata_.author = "ECScope Educational Framework";
        metadata_.license = "MIT";
        metadata_.category = PluginCategory::Educational;
        metadata_.priority = PluginPriority::Normal;
        
        metadata_.is_educational = true;
        metadata_.educational_purpose = "Demonstrate basic ECS component development patterns";
        metadata_.learning_objectives = {
            "Understand ECS component architecture",
            "Learn plugin development lifecycle",
            "Practice component registration and usage",
            "See real-world component examples"
        };
        metadata_.difficulty_level = "beginner";
        
        metadata_.min_engine_version = PluginVersion(1, 0, 0);
        metadata_.supported_platforms = {"Windows", "Linux", "macOS"};
    }
    
    /**
     * @brief Register plugin components
     */
    bool register_components() {
        auto& ecs = api_->get_ecs();
        
        // Register HealthComponent
        if (!ecs.register_component<HealthComponent>("HealthComponent", 
            "Basic health component with damage and regeneration", true)) {
            LOG_ERROR("Failed to register HealthComponent");
            return false;
        }
        
        // Register ExperienceComponent
        if (!ecs.register_component<ExperienceComponent>("ExperienceComponent",
            "Experience and leveling component with configurable progression", true)) {
            LOG_ERROR("Failed to register ExperienceComponent");
            return false;
        }
        
        // Register InventoryComponent
        if (!ecs.register_component<InventoryComponent>("InventoryComponent",
            "Item inventory with weight and slot management", true)) {
            LOG_ERROR("Failed to register InventoryComponent");
            return false;
        }
        
        LOG_INFO("Successfully registered all plugin components");
        return true;
    }
    
    /**
     * @brief Set up educational examples
     */
    void setup_educational_examples() {
        // Add code examples to the API context
        api_->add_code_example("Creating a Health Entity", R"cpp(
// Create an entity with health component
auto entity = api.get_ecs().create_entity<HealthComponent>(100.0f, 2.0f);

// Access and modify health
auto* health = api.get_ecs().get_component<HealthComponent>(entity);
if (health) {
    health->take_damage(25.0f);
    std::cout << "Health: " << health->current_health << std::endl;
}
        )cpp");
        
        api_->add_code_example("Experience System Usage", R"cpp(
// Create character with experience component
auto character = api.get_ecs().create_entity<ExperienceComponent>(1);

// Add experience and check for level up
auto* exp = api.get_ecs().get_component<ExperienceComponent>(character);
if (exp) {
    bool leveled_up = exp->add_experience(150);
    if (leveled_up) {
        std::cout << "Level up! Now level " << exp->current_level << std::endl;
    }
}
        )cpp");
        
        api_->add_code_example("Inventory Management", R"cpp(
// Create entity with inventory
auto player = api.get_ecs().create_entity<InventoryComponent>(30, 150.0f);

// Add items to inventory
auto* inventory = api.get_ecs().get_component<InventoryComponent>(player);
if (inventory) {
    InventoryComponent::Item sword{"Iron Sword", 1, 5.0f, "A sturdy iron sword"};
    if (inventory->add_item(sword)) {
        std::cout << "Item added successfully!" << std::endl;
    }
}
        )cpp");
    }
    
    /**
     * @brief Add educational content
     */
    void add_educational_content() {
        api_->add_learning_note("ECS components should be data-focused with minimal logic");
        api_->add_learning_note("Use composition over inheritance in component design");
        api_->add_learning_note("Keep components small and focused on a single responsibility");
        api_->add_learning_note("Consider memory layout and cache performance in component design");
        
        api_->explain_concept("Data-Oriented Design", 
            "ECS components store data contiguously in memory for better cache performance. "
            "This is why components should primarily contain data with minimal methods.");
        
        api_->explain_concept("Component Composition",
            "Instead of complex inheritance hierarchies, ECS uses component composition. "
            "Entities get behavior by combining multiple focused components.");
    }
    
    /**
     * @brief Initialize educational content
     */
    void initialize_educational_content() {
        component_usage_examples_ = {
            "Health system for RPG characters",
            "Damage system for combat mechanics",
            "Experience and leveling for character progression",
            "Inventory management for item collection",
            "Status effects and buffs/debuffs"
        };
    }
    
    /**
     * @brief Handle component added event
     */
    void handle_component_added_event(const PluginEvent& event) {
        components_created_++;
        
        if (api_->get_config("log_component_creation") == "true") {
            LOG_INFO("Component added to entity {}", event.entity_a);
        }
    }
    
    /**
     * @brief Handle entity created event
     */
    void handle_entity_created_event(const PluginEvent& event) {
        // Track entity creation for educational statistics
        stats_.total_events_handled++;
    }
    
    /**
     * @brief Handle configuration changed event
     */
    void handle_configuration_changed_event(const PluginEvent& event) {
        LOG_INFO("Configuration changed, updating plugin settings");
        // Reload configuration-dependent settings
    }
};

//=============================================================================
// Plugin Entry Points (C-style exports for dynamic loading)
//=============================================================================

extern "C" {

/**
 * @brief Create plugin instance
 */
PLUGIN_EXPORT IPlugin* create_plugin() {
    try {
        return new BasicComponentPlugin();
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create BasicComponentPlugin: {}", e.what());
        return nullptr;
    }
}

/**
 * @brief Destroy plugin instance
 */
PLUGIN_EXPORT void destroy_plugin(IPlugin* plugin) {
    if (plugin) {
        delete plugin;
    }
}

/**
 * @brief Get plugin metadata as JSON
 */
PLUGIN_EXPORT const char* get_plugin_info() {
    return R"json({
        "name": "BasicComponentPlugin",
        "display_name": "Basic Component Examples", 
        "description": "Educational plugin demonstrating fundamental ECS component creation",
        "version": "1.0.0",
        "author": "ECScope Educational Framework",
        "license": "MIT",
        "category": "Educational",
        "is_educational": true,
        "difficulty_level": "beginner",
        "learning_objectives": [
            "Understand ECS component architecture",
            "Learn plugin development lifecycle", 
            "Practice component registration and usage",
            "See real-world component examples"
        ],
        "components": [
            "HealthComponent",
            "ExperienceComponent", 
            "InventoryComponent"
        ],
        "min_engine_version": "1.0.0",
        "supported_platforms": ["Windows", "Linux", "macOS"]
    })json";
}

/**
 * @brief Get plugin API version
 */
PLUGIN_EXPORT u32 get_plugin_version() {
    return PLUGIN_API_VERSION;
}

/**
 * @brief Validate plugin before loading
 */
PLUGIN_EXPORT bool validate_plugin() {
    // Perform any pre-load validation here
    return true;
}

} // extern "C"