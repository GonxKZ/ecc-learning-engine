#pragma once

#include "physics_math.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

namespace ecscope::physics {

// Material properties for physics simulation
struct PhysicsMaterial {
    std::string name;
    
    // Basic properties
    Real density = 1.0f;           // kg/m³ (or kg/m² for 2D)
    Real friction = 0.3f;          // Static friction coefficient (0.0 = frictionless, 1.0+ = high friction)
    Real dynamic_friction = 0.2f;  // Kinetic friction coefficient (usually lower than static)
    Real restitution = 0.2f;       // Bounciness (0.0 = perfectly inelastic, 1.0 = perfectly elastic)
    
    // Advanced properties
    Real linear_damping = 0.01f;   // Linear velocity damping per second
    Real angular_damping = 0.01f;  // Angular velocity damping per second
    Real surface_velocity = 0.0f;  // Surface velocity for conveyor belt effects
    
    // Thermal properties (for advanced simulations)
    Real thermal_conductivity = 1.0f;
    Real specific_heat = 1000.0f;
    Real melting_point = 1000.0f;
    
    // Audio properties (for sound simulation)
    Real sound_velocity = 340.0f;  // Speed of sound through material
    Real sound_absorption = 0.1f;  // Sound absorption coefficient
    
    // Visual properties (for rendering)
    Vec3 color = Vec3(0.5f, 0.5f, 0.5f);
    Real roughness = 0.5f;
    Real metallic = 0.0f;
    Real transparency = 0.0f;
    
    // Material flags
    bool is_sensor = false;        // Ghost collision (no physical response)
    bool is_one_way = false;       // Only collides from one direction
    Vec3 one_way_direction = Vec3::unit_y();
    
    PhysicsMaterial() = default;
    
    PhysicsMaterial(const std::string& material_name, Real density_val, 
                   Real friction_val, Real restitution_val)
        : name(material_name), density(density_val), friction(friction_val), 
          restitution(restitution_val) {
        dynamic_friction = friction * 0.8f;  // Reasonable default
    }
    
    // Material combination rules for collisions
    struct CombinedProperties {
        Real friction;
        Real dynamic_friction;
        Real restitution;
        Real surface_velocity;
        bool is_sensor;
    };
    
    static CombinedProperties combine(const PhysicsMaterial& a, const PhysicsMaterial& b) {
        CombinedProperties result;
        
        // Friction: geometric mean (good for most cases)
        result.friction = std::sqrt(a.friction * b.friction);
        result.dynamic_friction = std::sqrt(a.dynamic_friction * b.dynamic_friction);
        
        // Restitution: use maximum (more bouncy materials dominate)
        result.restitution = std::max(a.restitution, b.restitution);
        
        // Surface velocity: sum (for conveyor belt effects)
        result.surface_velocity = a.surface_velocity + b.surface_velocity;
        
        // Sensor: either material is a sensor
        result.is_sensor = a.is_sensor || b.is_sensor;
        
        return result;
    }
};

// Predefined common materials
namespace Materials {
    inline PhysicsMaterial Steel() {
        PhysicsMaterial mat("Steel", 7850.0f, 0.7f, 0.2f);
        mat.thermal_conductivity = 50.0f;
        mat.sound_velocity = 5960.0f;
        mat.color = Vec3(0.7f, 0.7f, 0.8f);
        mat.metallic = 1.0f;
        mat.roughness = 0.2f;
        return mat;
    }
    
    inline PhysicsMaterial Wood() {
        PhysicsMaterial mat("Wood", 600.0f, 0.5f, 0.3f);
        mat.linear_damping = 0.02f;
        mat.angular_damping = 0.03f;
        mat.thermal_conductivity = 0.1f;
        mat.sound_velocity = 4000.0f;
        mat.sound_absorption = 0.3f;
        mat.color = Vec3(0.6f, 0.4f, 0.2f);
        mat.roughness = 0.8f;
        return mat;
    }
    
    inline PhysicsMaterial Rubber() {
        PhysicsMaterial mat("Rubber", 900.0f, 0.8f, 0.9f);
        mat.dynamic_friction = 0.7f;
        mat.linear_damping = 0.05f;
        mat.angular_damping = 0.05f;
        mat.sound_absorption = 0.8f;
        mat.color = Vec3(0.2f, 0.2f, 0.2f);
        mat.roughness = 0.9f;
        return mat;
    }
    
    inline PhysicsMaterial Ice() {
        PhysicsMaterial mat("Ice", 917.0f, 0.05f, 0.1f);
        mat.dynamic_friction = 0.02f;
        mat.thermal_conductivity = 2.0f;
        mat.melting_point = 273.0f;
        mat.color = Vec3(0.8f, 0.9f, 1.0f);
        mat.transparency = 0.8f;
        mat.roughness = 0.1f;
        return mat;
    }
    
    inline PhysicsMaterial Glass() {
        PhysicsMaterial mat("Glass", 2500.0f, 0.4f, 0.1f);
        mat.sound_velocity = 5640.0f;
        mat.color = Vec3(0.9f, 0.9f, 0.9f);
        mat.transparency = 0.9f;
        mat.roughness = 0.05f;
        return mat;
    }
    
    inline PhysicsMaterial Concrete() {
        PhysicsMaterial mat("Concrete", 2400.0f, 0.6f, 0.1f);
        mat.linear_damping = 0.1f;
        mat.angular_damping = 0.1f;
        mat.sound_absorption = 0.4f;
        mat.color = Vec3(0.7f, 0.7f, 0.6f);
        mat.roughness = 0.9f;
        return mat;
    }
    
    inline PhysicsMaterial Water() {
        PhysicsMaterial mat("Water", 1000.0f, 0.0f, 0.0f);
        mat.linear_damping = 2.0f;    // High damping for fluid
        mat.angular_damping = 2.0f;
        mat.thermal_conductivity = 0.6f;
        mat.sound_velocity = 1482.0f;
        mat.color = Vec3(0.2f, 0.4f, 0.8f);
        mat.transparency = 0.7f;
        return mat;
    }
    
    inline PhysicsMaterial Sensor() {
        PhysicsMaterial mat("Sensor", 0.0f, 0.0f, 0.0f);
        mat.is_sensor = true;
        mat.color = Vec3(1.0f, 1.0f, 0.0f);
        mat.transparency = 0.5f;
        return mat;
    }
    
    inline PhysicsMaterial Bouncy() {
        PhysicsMaterial mat("Bouncy", 500.0f, 0.3f, 1.2f);  // Super elastic
        mat.color = Vec3(1.0f, 0.2f, 1.0f);
        mat.roughness = 0.1f;
        return mat;
    }
    
    inline PhysicsMaterial Conveyor(Real belt_speed = 1.0f) {
        PhysicsMaterial mat("Conveyor", 1000.0f, 0.4f, 0.1f);
        mat.surface_velocity = belt_speed;
        mat.color = Vec3(0.3f, 0.3f, 0.3f);
        return mat;
    }
}

// Material property interpolation for smooth transitions
class MaterialInterpolator {
public:
    static PhysicsMaterial lerp(const PhysicsMaterial& a, const PhysicsMaterial& b, Real t) {
        t = clamp(t, 0.0f, 1.0f);
        
        PhysicsMaterial result;
        result.name = a.name + "_to_" + b.name;
        
        result.density = ecscope::physics::lerp(a.density, b.density, t);
        result.friction = ecscope::physics::lerp(a.friction, b.friction, t);
        result.dynamic_friction = ecscope::physics::lerp(a.dynamic_friction, b.dynamic_friction, t);
        result.restitution = ecscope::physics::lerp(a.restitution, b.restitution, t);
        result.linear_damping = ecscope::physics::lerp(a.linear_damping, b.linear_damping, t);
        result.angular_damping = ecscope::physics::lerp(a.angular_damping, b.angular_damping, t);
        result.surface_velocity = ecscope::physics::lerp(a.surface_velocity, b.surface_velocity, t);
        
        result.thermal_conductivity = ecscope::physics::lerp(a.thermal_conductivity, b.thermal_conductivity, t);
        result.specific_heat = ecscope::physics::lerp(a.specific_heat, b.specific_heat, t);
        result.melting_point = ecscope::physics::lerp(a.melting_point, b.melting_point, t);
        
        result.sound_velocity = ecscope::physics::lerp(a.sound_velocity, b.sound_velocity, t);
        result.sound_absorption = ecscope::physics::lerp(a.sound_absorption, b.sound_absorption, t);
        
        result.color = ecscope::physics::lerp(a.color, b.color, t);
        result.roughness = ecscope::physics::lerp(a.roughness, b.roughness, t);
        result.metallic = ecscope::physics::lerp(a.metallic, b.metallic, t);
        result.transparency = ecscope::physics::lerp(a.transparency, b.transparency, t);
        
        // Boolean properties - use threshold
        result.is_sensor = t < 0.5f ? a.is_sensor : b.is_sensor;
        result.is_one_way = t < 0.5f ? a.is_one_way : b.is_one_way;
        result.one_way_direction = ecscope::physics::lerp(a.one_way_direction, b.one_way_direction, t);
        
        return result;
    }
};

// Material manager for efficient storage and lookup
class MaterialManager {
private:
    std::unordered_map<std::string, std::unique_ptr<PhysicsMaterial>> materials;
    std::unordered_map<uint32_t, std::string> id_to_name;
    uint32_t next_id = 1;
    
    // Cache for combined materials to avoid repeated calculations
    mutable std::unordered_map<uint64_t, PhysicsMaterial::CombinedProperties> combination_cache;
    
public:
    MaterialManager() {
        // Register default materials
        register_material(std::make_unique<PhysicsMaterial>(Materials::Steel()));
        register_material(std::make_unique<PhysicsMaterial>(Materials::Wood()));
        register_material(std::make_unique<PhysicsMaterial>(Materials::Rubber()));
        register_material(std::make_unique<PhysicsMaterial>(Materials::Ice()));
        register_material(std::make_unique<PhysicsMaterial>(Materials::Glass()));
        register_material(std::make_unique<PhysicsMaterial>(Materials::Concrete()));
        register_material(std::make_unique<PhysicsMaterial>(Materials::Water()));
        register_material(std::make_unique<PhysicsMaterial>(Materials::Sensor()));
        register_material(std::make_unique<PhysicsMaterial>(Materials::Bouncy()));
    }
    
    uint32_t register_material(std::unique_ptr<PhysicsMaterial> material) {
        uint32_t id = next_id++;
        std::string name = material->name;
        materials[name] = std::move(material);
        id_to_name[id] = name;
        return id;
    }
    
    const PhysicsMaterial* get_material(const std::string& name) const {
        auto it = materials.find(name);
        return it != materials.end() ? it->second.get() : nullptr;
    }
    
    const PhysicsMaterial* get_material(uint32_t id) const {
        auto it = id_to_name.find(id);
        if (it != id_to_name.end()) {
            return get_material(it->second);
        }
        return nullptr;
    }
    
    uint32_t get_material_id(const std::string& name) const {
        for (const auto& [id, mat_name] : id_to_name) {
            if (mat_name == name) {
                return id;
            }
        }
        return 0;
    }
    
    bool remove_material(const std::string& name) {
        auto it = materials.find(name);
        if (it != materials.end()) {
            // Find and remove from id map
            for (auto id_it = id_to_name.begin(); id_it != id_to_name.end(); ++id_it) {
                if (id_it->second == name) {
                    id_to_name.erase(id_it);
                    break;
                }
            }
            materials.erase(it);
            clear_combination_cache();
            return true;
        }
        return false;
    }
    
    // Get combined properties for two materials with caching
    PhysicsMaterial::CombinedProperties get_combined_properties(
        const std::string& material_a, const std::string& material_b) const {
        
        // Create cache key
        uint64_t key = std::hash<std::string>{}(material_a) ^ 
                      (std::hash<std::string>{}(material_b) << 1);
        
        auto cache_it = combination_cache.find(key);
        if (cache_it != combination_cache.end()) {
            return cache_it->second;
        }
        
        const PhysicsMaterial* mat_a = get_material(material_a);
        const PhysicsMaterial* mat_b = get_material(material_b);
        
        if (mat_a && mat_b) {
            auto combined = PhysicsMaterial::combine(*mat_a, *mat_b);
            combination_cache[key] = combined;
            return combined;
        }
        
        // Return default properties if materials not found
        return PhysicsMaterial::CombinedProperties{};
    }
    
    std::vector<std::string> get_material_names() const {
        std::vector<std::string> names;
        names.reserve(materials.size());
        for (const auto& [name, material] : materials) {
            names.push_back(name);
        }
        return names;
    }
    
    size_t get_material_count() const {
        return materials.size();
    }
    
    void clear_combination_cache() {
        combination_cache.clear();
    }
    
    // Create a custom material builder
    class MaterialBuilder {
    private:
        PhysicsMaterial material;
        
    public:
        MaterialBuilder(const std::string& name) {
            material.name = name;
        }
        
        MaterialBuilder& density(Real value) { material.density = value; return *this; }
        MaterialBuilder& friction(Real value) { material.friction = value; return *this; }
        MaterialBuilder& dynamic_friction(Real value) { material.dynamic_friction = value; return *this; }
        MaterialBuilder& restitution(Real value) { material.restitution = value; return *this; }
        MaterialBuilder& linear_damping(Real value) { material.linear_damping = value; return *this; }
        MaterialBuilder& angular_damping(Real value) { material.angular_damping = value; return *this; }
        MaterialBuilder& surface_velocity(Real value) { material.surface_velocity = value; return *this; }
        MaterialBuilder& color(const Vec3& value) { material.color = value; return *this; }
        MaterialBuilder& roughness(Real value) { material.roughness = value; return *this; }
        MaterialBuilder& metallic(Real value) { material.metallic = value; return *this; }
        MaterialBuilder& transparency(Real value) { material.transparency = value; return *this; }
        MaterialBuilder& sensor(bool value = true) { material.is_sensor = value; return *this; }
        MaterialBuilder& one_way(const Vec3& direction) { 
            material.is_one_way = true; 
            material.one_way_direction = direction.normalized(); 
            return *this; 
        }
        
        std::unique_ptr<PhysicsMaterial> build() {
            return std::make_unique<PhysicsMaterial>(std::move(material));
        }
    };
    
    static MaterialBuilder create(const std::string& name) {
        return MaterialBuilder(name);
    }
    
    // Debug information
    struct MaterialStats {
        size_t total_materials;
        size_t cached_combinations;
        Real average_density;
        Real average_friction;
        Real average_restitution;
    };
    
    MaterialStats get_stats() const {
        MaterialStats stats{};
        stats.total_materials = materials.size();
        stats.cached_combinations = combination_cache.size();
        
        if (!materials.empty()) {
            Real total_density = 0, total_friction = 0, total_restitution = 0;
            for (const auto& [name, material] : materials) {
                total_density += material->density;
                total_friction += material->friction;
                total_restitution += material->restitution;
            }
            stats.average_density = total_density / materials.size();
            stats.average_friction = total_friction / materials.size();
            stats.average_restitution = total_restitution / materials.size();
        }
        
        return stats;
    }
};

// Global material manager instance
inline MaterialManager& get_material_manager() {
    static MaterialManager instance;
    return instance;
}

// Convenience functions
inline const PhysicsMaterial* get_material(const std::string& name) {
    return get_material_manager().get_material(name);
}

inline PhysicsMaterial::CombinedProperties combine_materials(
    const std::string& material_a, const std::string& material_b) {
    return get_material_manager().get_combined_properties(material_a, material_b);
}

inline uint32_t register_material(std::unique_ptr<PhysicsMaterial> material) {
    return get_material_manager().register_material(std::move(material));
}

} // namespace ecscope::physics