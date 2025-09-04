#pragma once

#include "core/types.hpp"
#include "core/id.hpp"
#include "component.hpp"
#include <bitset>
#include <vector>
#include <algorithm>

namespace ecscope::ecs {

// Maximum number of component types supported (can be increased if needed)
constexpr usize MAX_COMPONENTS = 64;

// Component signature using bitset for fast operations
class ComponentSignature {
private:
    std::bitset<MAX_COMPONENTS> bits_;
    
public:
    ComponentSignature() = default;
    
    // Set a component bit
    template<Component T>
    void set() noexcept {
        auto id = component_id<T>();
        if (id.value() < MAX_COMPONENTS) {
            bits_.set(id.value());
        }
    }
    
    void set(core::ComponentID id) noexcept {
        if (id.value() < MAX_COMPONENTS) {
            bits_.set(id.value());
        }
    }
    
    // Clear a component bit
    template<Component T>
    void reset() noexcept {
        auto id = component_id<T>();
        if (id.value() < MAX_COMPONENTS) {
            bits_.reset(id.value());
        }
    }
    
    void reset(core::ComponentID id) noexcept {
        if (id.value() < MAX_COMPONENTS) {
            bits_.reset(id.value());
        }
    }
    
    // Test if a component is present
    template<Component T>
    bool has() const noexcept {
        auto id = component_id<T>();
        return id.value() < MAX_COMPONENTS && bits_.test(id.value());
    }
    
    bool has(core::ComponentID id) const noexcept {
        return id.value() < MAX_COMPONENTS && bits_.test(id.value());
    }
    
    // Get the raw bitset
    const std::bitset<MAX_COMPONENTS>& bits() const noexcept {
        return bits_;
    }
    
    // Check if signature is empty
    bool empty() const noexcept {
        return bits_.none();
    }
    
    // Count number of components
    usize count() const noexcept {
        return bits_.count();
    }
    
    // Bitwise operations
    ComponentSignature operator|(const ComponentSignature& other) const noexcept {
        ComponentSignature result;
        result.bits_ = bits_ | other.bits_;
        return result;
    }
    
    ComponentSignature operator&(const ComponentSignature& other) const noexcept {
        ComponentSignature result;
        result.bits_ = bits_ & other.bits_;
        return result;
    }
    
    ComponentSignature operator^(const ComponentSignature& other) const noexcept {
        ComponentSignature result;
        result.bits_ = bits_ ^ other.bits_;
        return result;
    }
    
    ComponentSignature operator~() const noexcept {
        ComponentSignature result;
        result.bits_ = ~bits_;
        return result;
    }
    
    // Assignment operators
    ComponentSignature& operator|=(const ComponentSignature& other) noexcept {
        bits_ |= other.bits_;
        return *this;
    }
    
    ComponentSignature& operator&=(const ComponentSignature& other) noexcept {
        bits_ &= other.bits_;
        return *this;
    }
    
    ComponentSignature& operator^=(const ComponentSignature& other) noexcept {
        bits_ ^= other.bits_;
        return *this;
    }
    
    // Comparison operators
    bool operator==(const ComponentSignature& other) const noexcept {
        return bits_ == other.bits_;
    }
    
    bool operator!=(const ComponentSignature& other) const noexcept {
        return bits_ != other.bits_;
    }
    
    bool operator<(const ComponentSignature& other) const noexcept {
        // For use in ordered containers (maps, sets)
        // Lexicographic comparison of bitsets
        for (usize i = 0; i < MAX_COMPONENTS; ++i) {
            if (bits_[i] != other.bits_[i]) {
                return other.bits_[i]; // other has bit set where we don't
            }
        }
        return false; // Equal
    }
    
    // Check if this signature is a subset of another
    bool is_subset_of(const ComponentSignature& other) const noexcept {
        return (bits_ & other.bits_) == bits_;
    }
    
    // Check if this signature is a superset of another  
    bool is_superset_of(const ComponentSignature& other) const noexcept {
        return other.is_subset_of(*this);
    }
    
    // Check if signatures have any components in common
    bool intersects(const ComponentSignature& other) const noexcept {
        return (bits_ & other.bits_).any();
    }
    
    // Hash for use in unordered containers
    struct Hash {
        usize operator()(const ComponentSignature& signature) const noexcept {
            return std::hash<std::bitset<MAX_COMPONENTS>>{}(signature.bits_);
        }
    };
    
    // Get list of component IDs in this signature (for iteration)
    std::vector<core::ComponentID> to_component_ids() const {
        std::vector<core::ComponentID> result;
        for (usize i = 0; i < MAX_COMPONENTS; ++i) {
            if (bits_.test(i)) {
                result.emplace_back(static_cast<u32>(i));
            }
        }
        return result;
    }
    
    // String representation for debugging
    std::string to_string() const {
        return bits_.to_string();
    }
};

// Signature builder utility for creating signatures from multiple components
template<Component... Components>
ComponentSignature make_signature() noexcept {
    ComponentSignature signature;
    (signature.set<Components>(), ...); // C++17 fold expression
    return signature;
}

// Check if signature matches a component list
template<Component... Components>
bool signature_matches(const ComponentSignature& signature) noexcept {
    auto required = make_signature<Components...>();
    return signature.is_superset_of(required);
}

} // namespace ecscope::ecs

// Hash specialization for ComponentSignature
namespace std {
    template<>
    struct hash<ecscope::ecs::ComponentSignature> {
        usize operator()(const ecscope::ecs::ComponentSignature& signature) const noexcept {
            return ecscope::ecs::ComponentSignature::Hash{}(signature);
        }
    };
}