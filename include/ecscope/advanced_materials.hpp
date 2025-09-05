#pragma once

/**
 * @file advanced_materials.hpp
 * @brief Advanced Material System for ECScope Physics Engine
 * 
 * This header implements a comprehensive material system that extends the basic
 * PhysicsMaterial with advanced properties for realistic physics simulation.
 * Includes temperature-dependent properties, anisotropic materials, composites,
 * and educational material science concepts.
 * 
 * Key Features:
 * - Temperature-dependent material properties
 * - Anisotropic materials (different properties in different directions)
 * - Composite materials with multiple phases
 * - Plastic deformation and damage modeling
 * - Thermal expansion and conduction
 * - Electromagnetic properties
 * - Educational visualization of material behavior
 * - Real-world material database
 * 
 * Educational Goals:
 * - Demonstrate materials science principles
 * - Show relationship between microstructure and properties
 * - Visualize stress-strain relationships
 * - Illustrate failure mechanisms and fracture
 * - Compare different material classes (metals, polymers, ceramics)
 * 
 * Performance Features:
 * - Efficient material property lookup tables
 * - SIMD-optimized property calculations
 * - Cache-friendly data structures
 * - Minimal computational overhead
 * 
 * @author ECScope Educational Physics Engine
 * @date 2024
 */

#include "physics.hpp"
#include "soft_body_physics.hpp"
#include "fluid_simulation.hpp"
#include "../ecs/component.hpp"
#include "../memory/pool.hpp"
#include <array>
#include <vector>
#include <functional>
#include <string_view>

namespace ecscope::physics::materials {

// Import necessary types
using namespace ecscope::physics::math;
using namespace ecscope::physics::components;

//=============================================================================
// Material Property Tensors
//=============================================================================

/**
 * @brief 2x2 tensor for anisotropic material properties
 * 
 * Represents material properties that vary with direction.
 * Used for elastic moduli, thermal conductivity, electrical conductivity, etc.
 */
struct alignas(16) MaterialTensor2D {
    // Tensor components in matrix form:
    // [xx xy]
    // [yx yy]
    f32 xx, xy, yx, yy;
    
    constexpr MaterialTensor2D() noexcept : xx(1.0f), xy(0.0f), yx(0.0f), yy(1.0f) {}
    
    constexpr MaterialTensor2D(f32 diagonal_value) noexcept 
        : xx(diagonal_value), xy(0.0f), yx(0.0f), yy(diagonal_value) {}
    
    constexpr MaterialTensor2D(f32 xx_, f32 xy_, f32 yx_, f32 yy_) noexcept 
        : xx(xx_), xy(xy_), yx(yx_), yy(yy_) {}
    
    /** @brief Create isotropic tensor (same value in all directions) */
    static constexpr MaterialTensor2D isotropic(f32 value) noexcept {
        return MaterialTensor2D(value);
    }
    
    /** @brief Create orthotropic tensor (different values along x and y axes) */
    static constexpr MaterialTensor2D orthotropic(f32 x_value, f32 y_value) noexcept {
        return MaterialTensor2D(x_value, 0.0f, 0.0f, y_value);
    }
    
    /** @brief Rotate tensor by angle (radians) */
    MaterialTensor2D rotated(f32 angle) const noexcept {
        f32 c = std::cos(angle);
        f32 s = std::sin(angle);
        
        // Rotation matrix multiplication: R^T * T * R
        f32 new_xx = c*c*xx + s*s*yy + 2*c*s*xy;
        f32 new_yy = s*s*xx + c*c*yy - 2*c*s*xy;
        f32 new_xy = c*s*(yy - xx) + (c*c - s*s)*xy;
        f32 new_yx = new_xy; // Symmetric tensor
        
        return MaterialTensor2D(new_xx, new_xy, new_yx, new_yy);
    }
    
    /** @brief Get property value in a specific direction */
    f32 value_in_direction(const Vec2& direction) const noexcept {
        Vec2 n = direction.normalized();
        return xx*n.x*n.x + yy*n.y*n.y + (xy + yx)*n.x*n.y;
    }
    
    /** @brief Get principal values (eigenvalues) */
    std::pair<f32, f32> get_principal_values() const noexcept {
        f32 trace = xx + yy;
        f32 det = xx*yy - xy*yx;
        f32 discriminant = std::sqrt(trace*trace - 4*det);
        
        f32 lambda1 = (trace + discriminant) * 0.5f;
        f32 lambda2 = (trace - discriminant) * 0.5f;
        
        return {std::max(lambda1, lambda2), std::min(lambda1, lambda2)};
    }
    
    /** @brief Get anisotropy ratio (max/min principal values) */
    f32 get_anisotropy_ratio() const noexcept {
        auto [max_val, min_val] = get_principal_values();
        return (min_val > 1e-6f) ? max_val / min_val : 1e6f;
    }
};

//=============================================================================
// Temperature-Dependent Properties
//=============================================================================

/**
 * @brief Temperature-dependent material property
 * 
 * Represents how a material property changes with temperature.
 * Uses polynomial interpolation for efficient evaluation.
 */
template<usize MaxCoefficients = 4>
struct TemperatureDependentProperty {
    /** @brief Polynomial coefficients: property = c0 + c1*T + c2*T^2 + c3*T^3 + ... */
    std::array<f32, MaxCoefficients> coefficients{0.0f};
    
    /** @brief Valid temperature range */
    f32 min_temperature{0.0f};   // Kelvin
    f32 max_temperature{1e6f};   // Kelvin
    
    /** @brief Reference temperature for coefficients */
    f32 reference_temperature{293.15f}; // Room temperature
    
    constexpr TemperatureDependentProperty() noexcept = default;
    
    /** @brief Constant property (temperature-independent) */
    explicit constexpr TemperatureDependentProperty(f32 constant_value) noexcept {
        coefficients[0] = constant_value;
    }
    
    /** @brief Linear temperature dependence */
    constexpr TemperatureDependentProperty(f32 ref_value, f32 temp_coeff) noexcept {
        coefficients[0] = ref_value;
        coefficients[1] = temp_coeff;
    }
    
    /** @brief Evaluate property at given temperature */
    f32 evaluate(f32 temperature) const noexcept {
        // Clamp temperature to valid range
        temperature = std::clamp(temperature, min_temperature, max_temperature);
        
        // Normalize temperature relative to reference
        f32 delta_t = temperature - reference_temperature;
        
        // Evaluate polynomial using Horner's method
        f32 result = coefficients[MaxCoefficients - 1];
        for (int i = MaxCoefficients - 2; i >= 0; --i) {
            result = result * delta_t + coefficients[i];
        }
        
        return std::max(0.0f, result); // Ensure positive physical properties
    }
    
    /** @brief Get temperature derivative (rate of change) */
    f32 temperature_derivative(f32 temperature) const noexcept {
        if (MaxCoefficients < 2) return 0.0f;
        
        temperature = std::clamp(temperature, min_temperature, max_temperature);
        f32 delta_t = temperature - reference_temperature;
        
        // Derivative of polynomial
        f32 result = (MaxCoefficients - 1) * coefficients[MaxCoefficients - 1];
        for (int i = MaxCoefficients - 2; i >= 1; --i) {
            result = result * delta_t + i * coefficients[i];
        }
        
        return result;
    }
    
    /** @brief Create property for steel (temperature-dependent Young's modulus) */
    static TemperatureDependentProperty create_steel_youngs_modulus() noexcept {
        TemperatureDependentProperty prop;
        prop.coefficients[0] = 200e9f;      // 200 GPa at room temperature
        prop.coefficients[1] = -4e7f;       // Decreases with temperature
        prop.min_temperature = 200.0f;      // 200 K minimum
        prop.max_temperature = 2000.0f;     // 2000 K maximum
        return prop;
    }
    
    /** @brief Create property for aluminum thermal expansion */
    static TemperatureDependentProperty create_aluminum_thermal_expansion() noexcept {
        TemperatureDependentProperty prop;
        prop.coefficients[0] = 23e-6f;      // 23 μm/m/K at room temperature
        prop.coefficients[1] = 1e-8f;       // Slight increase with temperature
        prop.min_temperature = 100.0f;
        prop.max_temperature = 900.0f;      // Below melting point
        return prop;
    }
};

//=============================================================================
// Advanced Material Properties
//=============================================================================

/**
 * @brief Comprehensive advanced material definition
 * 
 * Extends basic PhysicsMaterial with advanced properties for realistic
 * simulation of complex material behavior.
 */
struct alignas(64) AdvancedMaterial {
    //-------------------------------------------------------------------------
    // Basic Properties (from PhysicsMaterial)
    //-------------------------------------------------------------------------
    
    /** @brief Base physics material */
    PhysicsMaterial base_material;
    
    //-------------------------------------------------------------------------
    // Mechanical Properties (Advanced)
    //-------------------------------------------------------------------------
    
    /** @brief Elastic modulus tensor (anisotropic) */
    MaterialTensor2D elastic_modulus{200e9f}; // Default: 200 GPa isotropic
    
    /** @brief Shear modulus tensor */
    MaterialTensor2D shear_modulus{80e9f};    // Default: 80 GPa isotropic
    
    /** @brief Poisson's ratio tensor */
    MaterialTensor2D poissons_ratio{0.3f};    // Default: 0.3 isotropic
    
    /** @brief Yield strength (onset of plastic deformation) */
    TemperatureDependentProperty<3> yield_strength{250e6f}; // 250 MPa
    
    /** @brief Ultimate tensile strength (fracture stress) */
    TemperatureDependentProperty<3> ultimate_strength{400e6f}; // 400 MPa
    
    /** @brief Fatigue limit (infinite life stress) */
    f32 fatigue_limit{200e6f}; // 200 MPa
    
    /** @brief Fracture toughness (resistance to crack propagation) */
    f32 fracture_toughness{50e6f}; // 50 MPa√m
    
    /** @brief Hardness (resistance to indentation) */
    f32 hardness{2e9f}; // 2 GPa
    
    //-------------------------------------------------------------------------
    // Thermal Properties
    //-------------------------------------------------------------------------
    
    /** @brief Thermal conductivity tensor */
    MaterialTensor2D thermal_conductivity{50.0f}; // 50 W/m/K
    
    /** @brief Specific heat capacity */
    TemperatureDependentProperty<3> specific_heat{500.0f}; // 500 J/kg/K
    
    /** @brief Thermal expansion coefficient tensor */
    MaterialTensor2D thermal_expansion{12e-6f}; // 12 μm/m/K
    
    /** @brief Melting temperature */
    f32 melting_temperature{1800.0f}; // 1800 K
    
    /** @brief Glass transition temperature (for polymers) */
    f32 glass_transition_temperature{350.0f}; // 350 K
    
    /** @brief Thermal diffusivity */
    f32 thermal_diffusivity{1e-5f}; // m²/s
    
    /** @brief Emissivity (for radiation) */
    f32 emissivity{0.8f}; // 0.8 (dimensionless)
    
    //-------------------------------------------------------------------------
    // Electrical Properties
    //-------------------------------------------------------------------------
    
    /** @brief Electrical conductivity tensor */
    MaterialTensor2D electrical_conductivity{1e7f}; // 10^7 S/m
    
    /** @brief Dielectric constant */
    f32 dielectric_constant{1.0f};
    
    /** @brief Magnetic permeability */
    f32 magnetic_permeability{1.0f}; // Relative permeability
    
    /** @brief Electrical resistivity */
    TemperatureDependentProperty<2> resistivity{1e-7f}; // 10^-7 Ω⋅m
    
    //-------------------------------------------------------------------------
    // Optical Properties
    //-------------------------------------------------------------------------
    
    /** @brief Refractive index */
    f32 refractive_index{1.5f};
    
    /** @brief Absorption coefficient */
    f32 absorption_coefficient{0.1f};
    
    /** @brief Reflectance */
    f32 reflectance{0.1f};
    
    /** @brief Transparency */
    f32 transparency{0.0f};
    
    //-------------------------------------------------------------------------
    // Damage and Failure Properties
    //-------------------------------------------------------------------------
    
    /** @brief Damage parameters for progressive failure */
    struct {
        f32 damage_threshold{0.8f};     ///< Stress ratio to start damage
        f32 damage_rate{0.1f};          ///< Rate of damage accumulation
        f32 critical_damage{0.95f};     ///< Damage level for complete failure
        f32 healing_rate{0.0f};         ///< Self-healing rate (if any)
    } damage_model;
    
    /** @brief Fatigue parameters */
    struct {
        f32 stress_life_exponent{-0.1f}; ///< S-N curve exponent
        f32 strain_life_exponent{-0.5f}; ///< ε-N curve exponent
        u32 endurance_limit{1e6};        ///< Cycles to infinite life
    } fatigue_model;
    
    /** @brief Fracture mechanics parameters */
    struct {
        f32 critical_stress_intensity{20e6f}; ///< Critical SIF (Pa√m)
        f32 crack_growth_rate{1e-12f};        ///< da/dN coefficient
        f32 crack_growth_exponent{3.0f};      ///< Paris law exponent
    } fracture_model;
    
    //-------------------------------------------------------------------------
    // Microstructure Properties
    //-------------------------------------------------------------------------
    
    /** @brief Grain size (affects strength via Hall-Petch relation) */
    f32 grain_size{10e-6f}; // 10 μm
    
    /** @brief Porosity fraction */
    f32 porosity{0.0f}; // 0% porosity
    
    /** @brief Texture coefficient (crystallographic texture) */
    f32 texture_coefficient{1.0f}; // Random texture
    
    /** @brief Preferred orientation (for anisotropic materials) */
    f32 preferred_orientation{0.0f}; // Radians
    
    //-------------------------------------------------------------------------
    // Composite Material Properties
    //-------------------------------------------------------------------------
    
    /** @brief Volume fraction of reinforcement (for composites) */
    f32 reinforcement_fraction{0.0f};
    
    /** @brief Aspect ratio of reinforcement (fibers, particles) */
    f32 reinforcement_aspect_ratio{1.0f};
    
    /** @brief Interface strength (matrix-reinforcement bond) */
    f32 interface_strength{50e6f}; // 50 MPa
    
    //-------------------------------------------------------------------------
    // Environmental Effects
    //-------------------------------------------------------------------------
    
    /** @brief Corrosion rate in different environments */
    struct {
        f32 air_corrosion_rate{1e-9f};      ///< mm/year in air
        f32 water_corrosion_rate{1e-8f};    ///< mm/year in water
        f32 acid_corrosion_rate{1e-6f};     ///< mm/year in acid
    } corrosion_rates;
    
    /** @brief UV degradation rate */
    f32 uv_degradation_rate{1e-8f}; // Property loss per J/m² UV exposure
    
    /** @brief Moisture absorption coefficient */
    f32 moisture_absorption{0.01f}; // % weight gain at 100% humidity
    
    //-------------------------------------------------------------------------
    // Material Classification
    //-------------------------------------------------------------------------
    
    /** @brief Material class enumeration */
    enum class MaterialClass : u8 {
        Metal,              ///< Metallic materials
        Polymer,            ///< Polymeric materials  
        Ceramic,            ///< Ceramic materials
        Composite,          ///< Composite materials
        Semiconductor,      ///< Semiconductor materials
        Biomaterial,        ///< Biological materials
        Smart              ///< Smart/functional materials
    } material_class{MaterialClass::Metal};
    
    /** @brief Material subclass for more specific identification */
    enum class MaterialSubclass : u8 {
        // Metals
        Steel, Aluminum, Titanium, Copper, 
        // Polymers
        Thermoplastic, Thermoset, Elastomer,
        // Ceramics
        Oxide, Nitride, Carbide, Glass,
        // Composites
        FiberReinforced, ParticleReinforced, Layered,
        // Others
        Unknown
    } material_subclass{MaterialSubclass::Steel};
    
    //-------------------------------------------------------------------------
    // Behavior Flags
    //-------------------------------------------------------------------------
    
    union {
        u32 flags;
        struct {
            u32 is_anisotropic : 1;           ///< Has directional properties
            u32 is_temperature_dependent : 1; ///< Properties vary with temperature
            u32 is_strain_rate_sensitive : 1; ///< Properties depend on loading rate
            u32 is_composite : 1;             ///< Multi-phase composite material
            u32 has_phase_transitions : 1;    ///< Can undergo phase changes
            u32 is_viscoelastic : 1;          ///< Shows time-dependent behavior
            u32 is_plastic : 1;               ///< Can deform plastically
            u32 is_brittle : 1;               ///< Fails without significant plastic deformation
            u32 is_ductile : 1;               ///< Shows significant plastic deformation
            u32 is_fatigue_sensitive : 1;     ///< Susceptible to fatigue failure
            u32 is_corrosion_resistant : 1;   ///< Resistant to chemical attack
            u32 is_conductive : 1;            ///< Electrically conductive
            u32 is_magnetic : 1;              ///< Shows magnetic behavior
            u32 is_transparent : 1;           ///< Optically transparent
            u32 is_smart_material : 1;        ///< Shape memory, piezoelectric, etc.
            u32 reserved : 17;
        };
    } material_flags{0};
    
    //-------------------------------------------------------------------------
    // Material Identification
    //-------------------------------------------------------------------------
    
    /** @brief Material name for identification */
    char name[32]{"Generic Material"};
    
    /** @brief Material designation/standard (e.g., "AISI 1020", "6061-T6") */
    char designation[16]{"GENERIC"};
    
    /** @brief Unique material ID */
    u32 material_id{0};
    
    /** @brief Material database version */
    u16 database_version{1};
    
    //-------------------------------------------------------------------------
    // Factory Methods for Common Materials
    //-------------------------------------------------------------------------
    
    /** @brief Create structural steel (AISI 1020) */
    static AdvancedMaterial create_structural_steel() noexcept;
    
    /** @brief Create aluminum alloy (6061-T6) */
    static AdvancedMaterial create_aluminum_6061() noexcept;
    
    /** @brief Create titanium alloy (Ti-6Al-4V) */
    static AdvancedMaterial create_titanium_alloy() noexcept;
    
    /** @brief Create engineering polymer (Nylon 6,6) */
    static AdvancedMaterial create_nylon66() noexcept;
    
    /** @brief Create structural ceramic (Alumina) */
    static AdvancedMaterial create_alumina() noexcept;
    
    /** @brief Create carbon fiber composite */
    static AdvancedMaterial create_carbon_fiber_composite() noexcept;
    
    /** @brief Create concrete */
    static AdvancedMaterial create_concrete() noexcept;
    
    /** @brief Create wood (generic hardwood) */
    static AdvancedMaterial create_hardwood() noexcept;
    
    /** @brief Create rubber (natural) */
    static AdvancedMaterial create_rubber() noexcept;
    
    /** @brief Create glass (soda-lime) */
    static AdvancedMaterial create_glass() noexcept;
    
    //-------------------------------------------------------------------------
    // Property Access Methods
    //-------------------------------------------------------------------------
    
    /** @brief Get Young's modulus in specific direction at temperature */
    f32 get_youngs_modulus(const Vec2& direction, f32 temperature = 293.15f) const noexcept {
        f32 base_modulus = elastic_modulus.value_in_direction(direction);
        if (material_flags.is_temperature_dependent) {
            // Apply temperature correction (simplified)
            f32 temp_factor = 1.0f - (temperature - 293.15f) * 1e-4f; // -0.01%/K typical
            base_modulus *= std::max(0.1f, temp_factor);
        }
        return base_modulus;
    }
    
    /** @brief Get yield strength at temperature and strain rate */
    f32 get_yield_strength(f32 temperature = 293.15f, f32 strain_rate = 1e-3f) const noexcept {
        f32 base_yield = yield_strength.evaluate(temperature);
        
        if (material_flags.is_strain_rate_sensitive) {
            // Johnson-Cook strain rate effect
            f32 rate_factor = 1.0f + 0.05f * std::log10(strain_rate / 1e-3f);
            base_yield *= std::max(0.5f, rate_factor);
        }
        
        return base_yield;
    }
    
    /** @brief Get thermal conductivity in specific direction */
    f32 get_thermal_conductivity(const Vec2& direction) const noexcept {
        return thermal_conductivity.value_in_direction(direction);
    }
    
    /** @brief Calculate thermal stress from temperature change */
    f32 calculate_thermal_stress(f32 delta_temperature, const Vec2& direction) const noexcept {
        f32 alpha = thermal_expansion.value_in_direction(direction);
        f32 E = get_youngs_modulus(direction);
        return E * alpha * delta_temperature;
    }
    
    /** @brief Check if material has failed under given stress */
    bool has_failed(f32 stress, f32 temperature = 293.15f) const noexcept {
        f32 failure_stress = get_yield_strength(temperature);
        if (material_flags.is_brittle) {
            return stress > failure_stress; // Immediate failure
        } else {
            return stress > ultimate_strength.evaluate(temperature);
        }
    }
    
    /** @brief Calculate damage from stress history */
    f32 calculate_damage(f32 max_stress, u32 cycles, f32 temperature = 293.15f) const noexcept {
        if (!material_flags.is_fatigue_sensitive) return 0.0f;
        
        f32 yield = get_yield_strength(temperature);
        f32 stress_ratio = max_stress / yield;
        
        if (stress_ratio < damage_model.damage_threshold) return 0.0f;
        
        // Simplified fatigue damage accumulation
        f32 damage_per_cycle = damage_model.damage_rate * 
                              std::pow(stress_ratio, -fatigue_model.stress_life_exponent);
        
        return std::min(1.0f, damage_per_cycle * cycles);
    }
    
    /** @brief Get material stiffness matrix for finite element analysis */
    std::array<f32, 4> get_stiffness_matrix(f32 temperature = 293.15f) const noexcept {
        f32 E1 = elastic_modulus.xx;
        f32 E2 = elastic_modulus.yy;
        f32 G12 = shear_modulus.xy;
        f32 nu12 = poissons_ratio.xy;
        f32 nu21 = poissons_ratio.yx;
        
        // Apply temperature effects
        if (material_flags.is_temperature_dependent) {
            f32 temp_factor = 1.0f - (temperature - 293.15f) * 1e-4f;
            E1 *= temp_factor;
            E2 *= temp_factor;
            G12 *= temp_factor;
        }
        
        f32 denom = 1.0f - nu12 * nu21;
        
        return {
            E1 / denom,              // C11
            nu21 * E1 / denom,       // C12
            nu12 * E2 / denom,       // C21
            E2 / denom               // C22
        };
    }
    
    /** @brief Update properties based on current state */
    void update_properties(f32 temperature, f32 damage_level) noexcept {
        // Apply damage to properties
        if (damage_level > 0.0f) {
            f32 degradation = 1.0f - damage_level;
            elastic_modulus.xx *= degradation;
            elastic_modulus.yy *= degradation;
            shear_modulus.xx *= degradation;
            shear_modulus.yy *= degradation;
        }
        
        // Update base material properties
        base_material.density = base_material.density * (1.0f - damage_level * 0.1f);
    }
    
    //-------------------------------------------------------------------------
    // Validation and Utility Methods
    //-------------------------------------------------------------------------
    
    /** @brief Check if material properties are physically reasonable */
    bool is_valid() const noexcept {
        return base_material.is_valid() &&
               elastic_modulus.xx > 0.0f && elastic_modulus.yy > 0.0f &&
               shear_modulus.xx > 0.0f && shear_modulus.yy > 0.0f &&
               yield_strength.evaluate(293.15f) > 0.0f &&
               melting_temperature > 0.0f &&
               thermal_conductivity.xx >= 0.0f &&
               specific_heat.evaluate(293.15f) > 0.0f;
    }
    
    /** @brief Get material description for educational display */
    const char* get_material_description() const noexcept {
        switch (material_class) {
            case MaterialClass::Metal: return "Metallic Material";
            case MaterialClass::Polymer: return "Polymeric Material";
            case MaterialClass::Ceramic: return "Ceramic Material";
            case MaterialClass::Composite: return "Composite Material";
            case MaterialClass::Semiconductor: return "Semiconductor Material";
            case MaterialClass::Biomaterial: return "Biological Material";
            case MaterialClass::Smart: return "Smart Material";
            default: return "Unknown Material";
        }
    }
    
    /** @brief Calculate material cost index (simplified) */
    f32 get_material_cost_index() const noexcept {
        // Simplified cost model based on material class and properties
        f32 base_cost = 1.0f;
        
        switch (material_class) {
            case MaterialClass::Metal: base_cost = 2.0f; break;
            case MaterialClass::Polymer: base_cost = 1.0f; break;
            case MaterialClass::Ceramic: base_cost = 3.0f; break;
            case MaterialClass::Composite: base_cost = 5.0f; break;
            case MaterialClass::Smart: base_cost = 10.0f; break;
            default: base_cost = 1.0f;
        }
        
        // Adjust for performance
        f32 strength_factor = yield_strength.evaluate(293.15f) / 250e6f;
        f32 stiffness_factor = elastic_modulus.xx / 200e9f;
        
        return base_cost * (1.0f + 0.5f * strength_factor + 0.3f * stiffness_factor);
    }
    
    /** @brief Generate comprehensive material report */
    std::string generate_material_report() const noexcept;
    
    /** @brief Compare two materials for selection */
    f32 compare_material_performance(const AdvancedMaterial& other, 
                                   const std::array<f32, 8>& weight_factors) const noexcept;
};

//=============================================================================
// Material Database and Management
//=============================================================================

/**
 * @brief Material database for managing and accessing materials
 */
class MaterialDatabase {
private:
    std::vector<AdvancedMaterial> materials_;
    std::unordered_map<std::string, u32> name_to_id_;
    std::unordered_map<u32, u32> id_to_index_;
    
public:
    MaterialDatabase() = default;
    
    /** @brief Add material to database */
    u32 add_material(const AdvancedMaterial& material) {
        u32 index = static_cast<u32>(materials_.size());
        materials_.push_back(material);
        
        // Update lookup tables
        name_to_id_[material.name] = material.material_id;
        id_to_index_[material.material_id] = index;
        
        return material.material_id;
    }
    
    /** @brief Get material by ID */
    const AdvancedMaterial* get_material(u32 material_id) const noexcept {
        auto it = id_to_index_.find(material_id);
        return (it != id_to_index_.end()) ? &materials_[it->second] : nullptr;
    }
    
    /** @brief Get material by name */
    const AdvancedMaterial* get_material(std::string_view name) const noexcept {
        auto it = name_to_id_.find(std::string(name));
        if (it != name_to_id_.end()) {
            return get_material(it->second);
        }
        return nullptr;
    }
    
    /** @brief Find materials by class */
    std::vector<const AdvancedMaterial*> find_materials_by_class(
        AdvancedMaterial::MaterialClass material_class) const noexcept {
        
        std::vector<const AdvancedMaterial*> result;
        for (const auto& material : materials_) {
            if (material.material_class == material_class) {
                result.push_back(&material);
            }
        }
        return result;
    }
    
    /** @brief Initialize with common engineering materials */
    void initialize_standard_materials() {
        add_material(AdvancedMaterial::create_structural_steel());
        add_material(AdvancedMaterial::create_aluminum_6061());
        add_material(AdvancedMaterial::create_titanium_alloy());
        add_material(AdvancedMaterial::create_nylon66());
        add_material(AdvancedMaterial::create_alumina());
        add_material(AdvancedMaterial::create_carbon_fiber_composite());
        add_material(AdvancedMaterial::create_concrete());
        add_material(AdvancedMaterial::create_hardwood());
        add_material(AdvancedMaterial::create_rubber());
        add_material(AdvancedMaterial::create_glass());
    }
    
    /** @brief Get number of materials in database */
    usize size() const noexcept { return materials_.size(); }
    
    /** @brief Clear all materials */
    void clear() noexcept {
        materials_.clear();
        name_to_id_.clear();
        id_to_index_.clear();
    }
};

} // namespace ecscope::physics::materials