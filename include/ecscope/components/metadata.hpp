#pragma once

#include "reflection.hpp"
#include "properties.hpp"
#include "../foundation/concepts.hpp"
#include "../core/types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <optional>
#include <functional>
#include <chrono>
#include <type_traits>
#include <typeindex>

/**
 * @file metadata.hpp
 * @brief Component metadata and documentation system
 * 
 * This file implements a comprehensive metadata system that provides:
 * - Rich component and property documentation
 * - Component categorization and tagging
 * - Hierarchical component relationships
 * - Usage examples and best practices
 * - Performance characteristics metadata
 * - Version and compatibility information
 * - Dependency tracking and analysis
 * - Auto-generated documentation
 * 
 * Key Features:
 * - Compile-time metadata generation
 * - Runtime metadata queries and introspection
 * - Documentation generation from metadata
 * - Component usage analytics
 * - Deprecation warnings and migration paths
 * - Custom attribute system for extensibility
 */

namespace ecscope::components {

/// @brief Component category for organization
enum class ComponentCategory : std::uint16_t {
    Unknown = 0,
    Transform,      ///< Position, rotation, scale
    Rendering,      ///< Graphics, materials, meshes
    Physics,        ///< Collision, dynamics, forces
    Audio,          ///< Sound, music, effects
    Input,          ///< User input handling
    AI,             ///< Artificial intelligence, pathfinding
    Animation,      ///< Skeletal, procedural animation
    Networking,     ///< Network synchronization
    UI,             ///< User interface elements
    Logic,          ///< Game logic, state machines
    Data,           ///< Pure data containers
    System,         ///< System-specific components
    Debug,          ///< Debugging and profiling
    Utility,        ///< General utility components
    Custom = 0x8000 ///< Custom categories start here
};

/// @brief Component complexity level for performance guidance
enum class ComponentComplexity : std::uint8_t {
    Trivial,        ///< Simple POD types, no side effects
    Simple,         ///< Basic logic, minimal dependencies
    Moderate,       ///< Some complexity, few dependencies
    Complex,        ///< Significant logic, multiple dependencies
    Heavy           ///< Resource-intensive, many dependencies
};

/// @brief Component lifecycle stage information
enum class ComponentLifecycle : std::uint8_t {
    Experimental,   ///< Under development, API may change
    Preview,        ///< Feature-complete but not stable
    Stable,         ///< Production-ready, stable API
    Mature,         ///< Well-tested, optimized
    Legacy,         ///< Still supported but deprecated
    Deprecated      ///< Will be removed in future version
};

/// @brief Performance characteristics of a component
struct PerformanceInfo {
    std::size_t memory_usage{0};        ///< Typical memory usage in bytes
    std::uint32_t cpu_cost{0};          ///< Relative CPU cost (0-100)
    std::uint32_t cache_friendliness{0}; ///< Cache performance (0-100)
    bool is_simd_optimized{false};      ///< Uses SIMD instructions
    bool is_thread_safe{false};         ///< Can be accessed from multiple threads
    bool is_lock_free{false};           ///< Uses lock-free algorithms
    std::chrono::nanoseconds typical_access_time{0}; ///< Typical access time
    
    /// @brief Create performance info for type
    template<typename T>
    static PerformanceInfo create() {
        PerformanceInfo info;
        info.memory_usage = sizeof(T);
        info.cache_friendliness = sizeof(T) <= 64 ? 100 : 50; // Estimate based on cache line size
        info.is_simd_optimized = SimdComponent<T>;
        info.is_thread_safe = requires { T::is_thread_safe; } && T::is_thread_safe;
        info.is_lock_free = requires { T::is_lock_free; } && T::is_lock_free;
        return info;
    }
};

/// @brief Version information for compatibility tracking
struct VersionInfo {
    std::uint16_t major{1};
    std::uint16_t minor{0};
    std::uint16_t patch{0};
    std::string pre_release{};
    std::string build_metadata{};
    
    /// @brief Compare versions
    int compare(const VersionInfo& other) const noexcept {
        if (major != other.major) return major - other.major;
        if (minor != other.minor) return minor - other.minor;
        if (patch != other.patch) return patch - other.patch;
        return pre_release.compare(other.pre_release);
    }
    
    bool operator==(const VersionInfo& other) const noexcept { return compare(other) == 0; }
    bool operator<(const VersionInfo& other) const noexcept { return compare(other) < 0; }
    bool operator>(const VersionInfo& other) const noexcept { return compare(other) > 0; }
    
    /// @brief Convert to string
    std::string to_string() const {
        std::string version = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
        if (!pre_release.empty()) {
            version += "-" + pre_release;
        }
        if (!build_metadata.empty()) {
            version += "+" + build_metadata;
        }
        return version;
    }
    
    /// @brief Parse from string
    static VersionInfo from_string(const std::string& version_str);
};

/// @brief Usage example for documentation
struct UsageExample {
    std::string title;
    std::string description;
    std::string code;
    std::vector<std::string> tags;
    ComponentComplexity complexity{ComponentComplexity::Simple};
    
    UsageExample(std::string title_, std::string desc, std::string code_)
        : title(std::move(title_)), description(std::move(desc)), code(std::move(code_)) {}
};

/// @brief Migration information for deprecated components
struct MigrationInfo {
    std::string deprecated_reason;
    std::string replacement_component;
    std::string migration_guide;
    VersionInfo deprecated_since;
    VersionInfo removed_in;
    std::vector<std::string> breaking_changes;
    
    bool is_deprecated() const noexcept { return !deprecated_reason.empty(); }
    bool will_be_removed() const noexcept { return removed_in.major > 0; }
};

/// @brief Component relationship information
struct ComponentRelationship {
    std::type_index related_type;
    std::string relationship_type;  // "requires", "conflicts", "enhances", "replaces"
    std::string description;
    bool is_hard_requirement{false}; // If true, component cannot exist without related component
};

/// @brief Component metadata container
class ComponentMetadata {
public:
    ComponentMetadata(std::string name, std::type_index type_index)
        : name_(std::move(name)), type_index_(type_index), 
          creation_time_(std::chrono::system_clock::now()) {}
    
    /// @brief Get component name
    const std::string& name() const noexcept { return name_; }
    
    /// @brief Get type index
    std::type_index type_index() const noexcept { return type_index_; }
    
    /// @brief Set description
    ComponentMetadata& set_description(std::string description) {
        description_ = std::move(description);
        return *this;
    }
    
    /// @brief Get description
    const std::string& description() const noexcept { return description_; }
    
    /// @brief Set category
    ComponentMetadata& set_category(ComponentCategory category) {
        category_ = category;
        return *this;
    }
    
    /// @brief Get category
    ComponentCategory category() const noexcept { return category_; }
    
    /// @brief Set complexity
    ComponentMetadata& set_complexity(ComponentComplexity complexity) {
        complexity_ = complexity;
        return *this;
    }
    
    /// @brief Get complexity
    ComponentComplexity complexity() const noexcept { return complexity_; }
    
    /// @brief Set lifecycle stage
    ComponentMetadata& set_lifecycle(ComponentLifecycle lifecycle) {
        lifecycle_ = lifecycle;
        return *this;
    }
    
    /// @brief Get lifecycle stage
    ComponentLifecycle lifecycle() const noexcept { return lifecycle_; }
    
    /// @brief Set performance info
    ComponentMetadata& set_performance_info(PerformanceInfo info) {
        performance_info_ = info;
        return *this;
    }
    
    /// @brief Get performance info
    const PerformanceInfo& performance_info() const noexcept { return performance_info_; }
    
    /// @brief Set version info
    ComponentMetadata& set_version(VersionInfo version) {
        version_ = version;
        return *this;
    }
    
    /// @brief Get version info
    const VersionInfo& version() const noexcept { return version_; }
    
    /// @brief Set author information
    ComponentMetadata& set_author(std::string author, std::string email = "") {
        author_ = std::move(author);
        author_email_ = std::move(email);
        return *this;
    }
    
    /// @brief Get author
    const std::string& author() const noexcept { return author_; }
    
    /// @brief Get author email
    const std::string& author_email() const noexcept { return author_email_; }
    
    /// @brief Add tag
    ComponentMetadata& add_tag(std::string tag) {
        tags_.emplace(std::move(tag));
        return *this;
    }
    
    /// @brief Remove tag
    ComponentMetadata& remove_tag(const std::string& tag) {
        tags_.erase(tag);
        return *this;
    }
    
    /// @brief Check if has tag
    bool has_tag(const std::string& tag) const {
        return tags_.contains(tag);
    }
    
    /// @brief Get all tags
    const std::unordered_set<std::string>& tags() const noexcept { return tags_; }
    
    /// @brief Add usage example
    ComponentMetadata& add_example(UsageExample example) {
        examples_.emplace_back(std::move(example));
        return *this;
    }
    
    /// @brief Get usage examples
    const std::vector<UsageExample>& examples() const noexcept { return examples_; }
    
    /// @brief Set migration info
    ComponentMetadata& set_migration_info(MigrationInfo migration) {
        migration_info_ = std::move(migration);
        return *this;
    }
    
    /// @brief Get migration info
    const MigrationInfo& migration_info() const noexcept { return migration_info_; }
    
    /// @brief Check if deprecated
    bool is_deprecated() const noexcept { return migration_info_.is_deprecated(); }
    
    /// @brief Add relationship
    ComponentMetadata& add_relationship(ComponentRelationship relationship) {
        relationships_.emplace_back(std::move(relationship));
        return *this;
    }
    
    /// @brief Get relationships
    const std::vector<ComponentRelationship>& relationships() const noexcept { return relationships_; }
    
    /// @brief Get relationships by type
    std::vector<const ComponentRelationship*> get_relationships_by_type(const std::string& type) const {
        std::vector<const ComponentRelationship*> result;
        for (const auto& rel : relationships_) {
            if (rel.relationship_type == type) {
                result.push_back(&rel);
            }
        }
        return result;
    }
    
    /// @brief Add custom attribute
    ComponentMetadata& set_attribute(const std::string& name, std::string value) {
        attributes_[name] = std::move(value);
        return *this;
    }
    
    /// @brief Get custom attribute
    const std::string* get_attribute(const std::string& name) const {
        auto it = attributes_.find(name);
        return it != attributes_.end() ? &it->second : nullptr;
    }
    
    /// @brief Get all attributes
    const std::unordered_map<std::string, std::string>& attributes() const noexcept {
        return attributes_;
    }
    
    /// @brief Get creation time
    std::chrono::system_clock::time_point creation_time() const noexcept { return creation_time_; }
    
    /// @brief Set documentation URL
    ComponentMetadata& set_documentation_url(std::string url) {
        documentation_url_ = std::move(url);
        return *this;
    }
    
    /// @brief Get documentation URL
    const std::string& documentation_url() const noexcept { return documentation_url_; }
    
    /// @brief Set source file information
    ComponentMetadata& set_source_file(std::string file, std::uint32_t line = 0) {
        source_file_ = std::move(file);
        source_line_ = line;
        return *this;
    }
    
    /// @brief Get source file
    const std::string& source_file() const noexcept { return source_file_; }
    
    /// @brief Get source line
    std::uint32_t source_line() const noexcept { return source_line_; }

private:
    std::string name_;
    std::type_index type_index_;
    std::string description_;
    ComponentCategory category_{ComponentCategory::Unknown};
    ComponentComplexity complexity_{ComponentComplexity::Simple};
    ComponentLifecycle lifecycle_{ComponentLifecycle::Stable};
    PerformanceInfo performance_info_{};
    VersionInfo version_{};
    std::string author_;
    std::string author_email_;
    std::unordered_set<std::string> tags_;
    std::vector<UsageExample> examples_;
    MigrationInfo migration_info_{};
    std::vector<ComponentRelationship> relationships_;
    std::unordered_map<std::string, std::string> attributes_;
    std::chrono::system_clock::time_point creation_time_;
    std::string documentation_url_;
    std::string source_file_;
    std::uint32_t source_line_{0};
};

/// @brief Property metadata for enhanced documentation
class PropertyMetadata {
public:
    PropertyMetadata(std::string name, std::string type_name)
        : name_(std::move(name)), type_name_(std::move(type_name)) {}
    
    /// @brief Get property name
    const std::string& name() const noexcept { return name_; }
    
    /// @brief Get type name
    const std::string& type_name() const noexcept { return type_name_; }
    
    /// @brief Set description
    PropertyMetadata& set_description(std::string description) {
        description_ = std::move(description);
        return *this;
    }
    
    /// @brief Get description
    const std::string& description() const noexcept { return description_; }
    
    /// @brief Set unit of measurement
    PropertyMetadata& set_unit(std::string unit) {
        unit_ = std::move(unit);
        return *this;
    }
    
    /// @brief Get unit
    const std::string& unit() const noexcept { return unit_; }
    
    /// @brief Set default value description
    PropertyMetadata& set_default_value(std::string default_val) {
        default_value_ = std::move(default_val);
        return *this;
    }
    
    /// @brief Get default value
    const std::string& default_value() const noexcept { return default_value_; }
    
    /// @brief Set range information
    PropertyMetadata& set_range(std::string min_val, std::string max_val) {
        range_min_ = std::move(min_val);
        range_max_ = std::move(max_val);
        return *this;
    }
    
    /// @brief Get range minimum
    const std::string& range_min() const noexcept { return range_min_; }
    
    /// @brief Get range maximum
    const std::string& range_max() const noexcept { return range_max_; }
    
    /// @brief Set tooltip text
    PropertyMetadata& set_tooltip(std::string tooltip) {
        tooltip_ = std::move(tooltip);
        return *this;
    }
    
    /// @brief Get tooltip
    const std::string& tooltip() const noexcept { return tooltip_; }
    
    /// @brief Set editor hints
    PropertyMetadata& set_editor_hint(std::string hint) {
        editor_hints_.emplace_back(std::move(hint));
        return *this;
    }
    
    /// @brief Get editor hints
    const std::vector<std::string>& editor_hints() const noexcept { return editor_hints_; }
    
    /// @brief Add validation rule description
    PropertyMetadata& add_validation_rule(std::string rule) {
        validation_rules_.emplace_back(std::move(rule));
        return *this;
    }
    
    /// @brief Get validation rules
    const std::vector<std::string>& validation_rules() const noexcept { return validation_rules_; }

private:
    std::string name_;
    std::string type_name_;
    std::string description_;
    std::string unit_;
    std::string default_value_;
    std::string range_min_;
    std::string range_max_;
    std::string tooltip_;
    std::vector<std::string> editor_hints_;
    std::vector<std::string> validation_rules_;
};

/// @brief Metadata registry for comprehensive component documentation
class MetadataRegistry {
public:
    /// @brief Singleton access
    static MetadataRegistry& instance() {
        static MetadataRegistry registry;
        return registry;
    }
    
    /// @brief Register component metadata
    template<typename T>
    ComponentMetadata& register_metadata(const std::string& name = typeid(T).name()) {
        std::unique_lock lock(mutex_);
        
        const auto type_index = std::type_index(typeid(T));
        auto [it, inserted] = component_metadata_.emplace(type_index, ComponentMetadata(name, type_index));
        
        if (inserted) {
            // Auto-populate performance info
            it->second.set_performance_info(PerformanceInfo::create<T>());
        }
        
        return it->second;
    }
    
    /// @brief Get component metadata
    template<typename T>
    const ComponentMetadata* get_metadata() const {
        std::shared_lock lock(mutex_);
        
        const auto type_index = std::type_index(typeid(T));
        auto it = component_metadata_.find(type_index);
        return it != component_metadata_.end() ? &it->second : nullptr;
    }
    
    /// @brief Get component metadata by type index
    const ComponentMetadata* get_metadata(std::type_index type_index) const {
        std::shared_lock lock(mutex_);
        
        auto it = component_metadata_.find(type_index);
        return it != component_metadata_.end() ? &it->second : nullptr;
    }
    
    /// @brief Get component metadata by name
    const ComponentMetadata* get_metadata(const std::string& name) const {
        std::shared_lock lock(mutex_);
        
        for (const auto& [type_index, metadata] : component_metadata_) {
            if (metadata.name() == name) {
                return &metadata;
            }
        }
        return nullptr;
    }
    
    /// @brief Register property metadata
    void register_property_metadata(std::type_index type_index, PropertyMetadata metadata) {
        std::unique_lock lock(mutex_);
        property_metadata_[type_index].emplace_back(std::move(metadata));
    }
    
    /// @brief Get property metadata
    std::vector<const PropertyMetadata*> get_property_metadata(std::type_index type_index) const {
        std::shared_lock lock(mutex_);
        
        auto it = property_metadata_.find(type_index);
        if (it == property_metadata_.end()) {
            return {};
        }
        
        std::vector<const PropertyMetadata*> result;
        result.reserve(it->second.size());
        for (const auto& prop_meta : it->second) {
            result.push_back(&prop_meta);
        }
        return result;
    }
    
    /// @brief Get components by category
    std::vector<const ComponentMetadata*> get_components_by_category(ComponentCategory category) const {
        std::shared_lock lock(mutex_);
        
        std::vector<const ComponentMetadata*> result;
        for (const auto& [type_index, metadata] : component_metadata_) {
            if (metadata.category() == category) {
                result.push_back(&metadata);
            }
        }
        return result;
    }
    
    /// @brief Get components by lifecycle stage
    std::vector<const ComponentMetadata*> get_components_by_lifecycle(ComponentLifecycle lifecycle) const {
        std::shared_lock lock(mutex_);
        
        std::vector<const ComponentMetadata*> result;
        for (const auto& [type_index, metadata] : component_metadata_) {
            if (metadata.lifecycle() == lifecycle) {
                result.push_back(&metadata);
            }
        }
        return result;
    }
    
    /// @brief Get components with tag
    std::vector<const ComponentMetadata*> get_components_with_tag(const std::string& tag) const {
        std::shared_lock lock(mutex_);
        
        std::vector<const ComponentMetadata*> result;
        for (const auto& [type_index, metadata] : component_metadata_) {
            if (metadata.has_tag(tag)) {
                result.push_back(&metadata);
            }
        }
        return result;
    }
    
    /// @brief Get deprecated components
    std::vector<const ComponentMetadata*> get_deprecated_components() const {
        std::shared_lock lock(mutex_);
        
        std::vector<const ComponentMetadata*> result;
        for (const auto& [type_index, metadata] : component_metadata_) {
            if (metadata.is_deprecated()) {
                result.push_back(&metadata);
            }
        }
        return result;
    }
    
    /// @brief Get all registered metadata
    std::vector<const ComponentMetadata*> get_all_metadata() const {
        std::shared_lock lock(mutex_);
        
        std::vector<const ComponentMetadata*> result;
        result.reserve(component_metadata_.size());
        for (const auto& [type_index, metadata] : component_metadata_) {
            result.push_back(&metadata);
        }
        return result;
    }
    
    /// @brief Get metadata count
    std::size_t metadata_count() const {
        std::shared_lock lock(mutex_);
        return component_metadata_.size();
    }
    
    /// @brief Clear all metadata
    void clear() {
        std::unique_lock lock(mutex_);
        component_metadata_.clear();
        property_metadata_.clear();
    }
    
    /// @brief Generate documentation
    struct DocumentationOptions {
        bool include_deprecated{true};
        bool include_experimental{true};
        bool include_examples{true};
        bool include_relationships{true};
        bool include_performance_info{true};
        std::string format{"markdown"};  // "markdown", "html", "xml"
    };
    
    std::string generate_documentation(const DocumentationOptions& options = {}) const;

private:
    MetadataRegistry() = default;
    
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::type_index, ComponentMetadata> component_metadata_;
    std::unordered_map<std::type_index, std::vector<PropertyMetadata>> property_metadata_;
};

/// @brief Metadata builder for fluent API
template<typename T>
class MetadataBuilder {
public:
    MetadataBuilder(const std::string& name = typeid(T).name()) {
        metadata_ = &MetadataRegistry::instance().register_metadata<T>(name);
    }
    
    /// @brief Set description
    MetadataBuilder& description(const std::string& desc) {
        if (metadata_) metadata_->set_description(desc);
        return *this;
    }
    
    /// @brief Set category
    MetadataBuilder& category(ComponentCategory cat) {
        if (metadata_) metadata_->set_category(cat);
        return *this;
    }
    
    /// @brief Set complexity
    MetadataBuilder& complexity(ComponentComplexity comp) {
        if (metadata_) metadata_->set_complexity(comp);
        return *this;
    }
    
    /// @brief Set lifecycle stage
    MetadataBuilder& lifecycle(ComponentLifecycle lc) {
        if (metadata_) metadata_->set_lifecycle(lc);
        return *this;
    }
    
    /// @brief Set version
    MetadataBuilder& version(std::uint16_t major, std::uint16_t minor = 0, std::uint16_t patch = 0) {
        if (metadata_) {
            VersionInfo version;
            version.major = major;
            version.minor = minor;
            version.patch = patch;
            metadata_->set_version(version);
        }
        return *this;
    }
    
    /// @brief Set author
    MetadataBuilder& author(const std::string& name, const std::string& email = "") {
        if (metadata_) metadata_->set_author(name, email);
        return *this;
    }
    
    /// @brief Add tag
    MetadataBuilder& tag(const std::string& tag_name) {
        if (metadata_) metadata_->add_tag(tag_name);
        return *this;
    }
    
    /// @brief Add example
    MetadataBuilder& example(const std::string& title, const std::string& desc, const std::string& code) {
        if (metadata_) metadata_->add_example(UsageExample(title, desc, code));
        return *this;
    }
    
    /// @brief Mark as deprecated
    MetadataBuilder& deprecated(const std::string& reason, const std::string& replacement = "") {
        if (metadata_) {
            MigrationInfo migration;
            migration.deprecated_reason = reason;
            migration.replacement_component = replacement;
            metadata_->set_migration_info(std::move(migration));
        }
        return *this;
    }
    
    /// @brief Add relationship
    MetadataBuilder& requires_component(std::type_index type, const std::string& desc = "") {
        if (metadata_) {
            ComponentRelationship rel;
            rel.related_type = type;
            rel.relationship_type = "requires";
            rel.description = desc;
            rel.is_hard_requirement = true;
            metadata_->add_relationship(std::move(rel));
        }
        return *this;
    }
    
    /// @brief Set source location
    MetadataBuilder& source(const std::string& file, std::uint32_t line = 0) {
        if (metadata_) metadata_->set_source_file(file, line);
        return *this;
    }
    
    /// @brief Get the metadata object
    ComponentMetadata* get() const { return metadata_; }

private:
    ComponentMetadata* metadata_{nullptr};
};

/// @brief Helper function to create metadata builder
template<typename T>
MetadataBuilder<T> metadata(const std::string& name = typeid(T).name()) {
    return MetadataBuilder<T>(name);
}

/// @brief Convenience macros for metadata registration
#define ECSCOPE_COMPONENT_METADATA(Type, ...) \
    namespace { \
        struct Type##_MetadataRegistrar { \
            Type##_MetadataRegistrar() { \
                register_metadata(); \
            } \
            void register_metadata(); \
        }; \
        static Type##_MetadataRegistrar Type##_metadata_registrar_instance; \
    } \
    void Type##_MetadataRegistrar::register_metadata()

#define ECSCOPE_DESCRIBE_COMPONENT(description_text) \
    ::ecscope::components::metadata<decltype(*this)>().description(description_text)

}  // namespace ecscope::components