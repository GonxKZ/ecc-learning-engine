/**
 * @file materials.cpp
 * @brief Material System Implementation
 * 
 * Advanced material system with PBR support and property management.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "ecscope/rendering/materials.hpp"
#include <iostream>
#include <algorithm>

namespace ecscope::rendering {

// Material property implementations
MaterialProperty::MaterialProperty(const std::string& name, MaterialPropertyType type)
    : name_(name), type_(type) {
}

const std::string& MaterialProperty::get_name() const {
    return name_;
}

MaterialPropertyType MaterialProperty::get_type() const {
    return type_;
}

// Float property
FloatProperty::FloatProperty(const std::string& name, float value)
    : MaterialProperty(name, MaterialPropertyType::Float), value_(value) {
}

float FloatProperty::get_value() const {
    return value_;
}

void FloatProperty::set_value(float value) {
    value_ = value;
}

// Vector3 property
Vector3Property::Vector3Property(const std::string& name, const std::array<float, 3>& value)
    : MaterialProperty(name, MaterialPropertyType::Vector3), value_(value) {
}

const std::array<float, 3>& Vector3Property::get_value() const {
    return value_;
}

void Vector3Property::set_value(const std::array<float, 3>& value) {
    value_ = value;
}

// Vector4 property
Vector4Property::Vector4Property(const std::string& name, const std::array<float, 4>& value)
    : MaterialProperty(name, MaterialPropertyType::Vector4), value_(value) {
}

const std::array<float, 4>& Vector4Property::get_value() const {
    return value_;
}

void Vector4Property::set_value(const std::array<float, 4>& value) {
    value_ = value;
}

// Texture property
TextureProperty::TextureProperty(const std::string& name, TextureHandle texture)
    : MaterialProperty(name, MaterialPropertyType::Texture), texture_(texture) {
}

TextureHandle TextureProperty::get_texture() const {
    return texture_;
}

void TextureProperty::set_texture(TextureHandle texture) {
    texture_ = texture;
}

// Material implementation
Material::Material(const std::string& name) : name_(name) {
}

const std::string& Material::get_name() const {
    return name_;
}

void Material::add_property(std::unique_ptr<MaterialProperty> property) {
    if (property) {
        properties_[property->get_name()] = std::move(property);
    }
}

MaterialProperty* Material::get_property(const std::string& name) const {
    auto it = properties_.find(name);
    return (it != properties_.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> Material::get_property_names() const {
    std::vector<std::string> names;
    names.reserve(properties_.size());
    
    for (const auto& [name, property] : properties_) {
        names.push_back(name);
    }
    
    return names;
}

void Material::remove_property(const std::string& name) {
    properties_.erase(name);
}

// PBR Material implementation
PBRMaterial::PBRMaterial(const std::string& name) : Material(name) {
    // Initialize standard PBR properties
    add_property(std::make_unique<Vector3Property>("albedo", std::array<float, 3>{1.0f, 1.0f, 1.0f}));
    add_property(std::make_unique<FloatProperty>("metallic", 0.0f));
    add_property(std::make_unique<FloatProperty>("roughness", 0.5f));
    add_property(std::make_unique<FloatProperty>("ao", 1.0f));
    add_property(std::make_unique<Vector3Property>("emission", std::array<float, 3>{0.0f, 0.0f, 0.0f}));
    add_property(std::make_unique<FloatProperty>("alpha", 1.0f));
}

void PBRMaterial::set_albedo(const std::array<float, 3>& albedo) {
    if (auto prop = dynamic_cast<Vector3Property*>(get_property("albedo"))) {
        prop->set_value(albedo);
    }
}

std::array<float, 3> PBRMaterial::get_albedo() const {
    if (auto prop = dynamic_cast<const Vector3Property*>(get_property("albedo"))) {
        return prop->get_value();
    }
    return {1.0f, 1.0f, 1.0f};
}

void PBRMaterial::set_metallic(float metallic) {
    if (auto prop = dynamic_cast<FloatProperty*>(get_property("metallic"))) {
        prop->set_value(std::clamp(metallic, 0.0f, 1.0f));
    }
}

float PBRMaterial::get_metallic() const {
    if (auto prop = dynamic_cast<const FloatProperty*>(get_property("metallic"))) {
        return prop->get_value();
    }
    return 0.0f;
}

void PBRMaterial::set_roughness(float roughness) {
    if (auto prop = dynamic_cast<FloatProperty*>(get_property("roughness"))) {
        prop->set_value(std::clamp(roughness, 0.0f, 1.0f));
    }
}

float PBRMaterial::get_roughness() const {
    if (auto prop = dynamic_cast<const FloatProperty*>(get_property("roughness"))) {
        return prop->get_value();
    }
    return 0.5f;
}

void PBRMaterial::set_albedo_texture(TextureHandle texture) {
    auto existing = get_property("albedo_texture");
    if (existing) {
        if (auto tex_prop = dynamic_cast<TextureProperty*>(existing)) {
            tex_prop->set_texture(texture);
        }
    } else {
        add_property(std::make_unique<TextureProperty>("albedo_texture", texture));
    }
}

void PBRMaterial::set_normal_texture(TextureHandle texture) {
    auto existing = get_property("normal_texture");
    if (existing) {
        if (auto tex_prop = dynamic_cast<TextureProperty*>(existing)) {
            tex_prop->set_texture(texture);
        }
    } else {
        add_property(std::make_unique<TextureProperty>("normal_texture", texture));
    }
}

void PBRMaterial::set_metallic_roughness_texture(TextureHandle texture) {
    auto existing = get_property("metallic_roughness_texture");
    if (existing) {
        if (auto tex_prop = dynamic_cast<TextureProperty*>(existing)) {
            tex_prop->set_texture(texture);
        }
    } else {
        add_property(std::make_unique<TextureProperty>("metallic_roughness_texture", texture));
    }
}

// Material Manager implementation
MaterialManager::MaterialManager() = default;
MaterialManager::~MaterialManager() = default;

MaterialHandle MaterialManager::create_material(const std::string& name, MaterialType type) {
    uint64_t id = next_material_id_.fetch_add(1);
    
    std::unique_ptr<Material> material;
    switch (type) {
        case MaterialType::Basic:
            material = std::make_unique<Material>(name);
            break;
        case MaterialType::PBR:
            material = std::make_unique<PBRMaterial>(name);
            break;
        default:
            material = std::make_unique<Material>(name);
            break;
    }
    
    materials_[id] = std::move(material);
    return MaterialHandle{id};
}

void MaterialManager::destroy_material(MaterialHandle handle) {
    if (handle.is_valid()) {
        materials_.erase(handle.id());
    }
}

Material* MaterialManager::get_material(MaterialHandle handle) const {
    if (!handle.is_valid()) {
        return nullptr;
    }
    
    auto it = materials_.find(handle.id());
    return (it != materials_.end()) ? it->second.get() : nullptr;
}

std::vector<MaterialHandle> MaterialManager::get_all_materials() const {
    std::vector<MaterialHandle> handles;
    handles.reserve(materials_.size());
    
    for (const auto& [id, material] : materials_) {
        handles.emplace_back(id);
    }
    
    return handles;
}

size_t MaterialManager::get_material_count() const {
    return materials_.size();
}

} // namespace ecscope::rendering