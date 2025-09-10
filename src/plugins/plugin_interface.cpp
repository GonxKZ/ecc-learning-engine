#include "ecscope/plugins/plugin_interface.hpp"
#include <sstream>
#include <regex>

namespace ecscope::plugins {

    bool PluginVersion::is_compatible(const PluginVersion& other) const noexcept {
        // Major version must match exactly
        if (major != other.major) {
            return false;
        }
        
        // Minor version can be equal or newer
        if (minor < other.minor) {
            return false;
        }
        
        // Patch version doesn't affect compatibility
        return true;
    }
    
    std::string PluginVersion::to_string() const {
        std::ostringstream oss;
        oss << major << "." << minor << "." << patch;
        if (!pre_release.empty()) {
            oss << "-" << pre_release;
        }
        return oss.str();
    }
    
    bool PluginVersion::operator==(const PluginVersion& other) const noexcept {
        return major == other.major && 
               minor == other.minor && 
               patch == other.patch && 
               pre_release == other.pre_release;
    }
    
    bool PluginVersion::operator<(const PluginVersion& other) const noexcept {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        if (patch != other.patch) return patch < other.patch;
        
        // Pre-release versions are less than normal versions
        if (pre_release.empty() && !other.pre_release.empty()) return false;
        if (!pre_release.empty() && other.pre_release.empty()) return true;
        
        return pre_release < other.pre_release;
    }
    
    bool PluginVersion::operator>(const PluginVersion& other) const noexcept {
        return other < *this;
    }
    
    bool PluginMetadata::is_valid() const noexcept {
        // Check required fields
        if (name.empty() || display_name.empty()) {
            return false;
        }
        
        // Validate name format (alphanumeric and underscores/hyphens only)
        std::regex name_pattern("^[a-zA-Z0-9_-]+$");
        if (!std::regex_match(name, name_pattern)) {
            return false;
        }
        
        // Check version validity
        if (version.major == 0 && version.minor == 0 && version.patch == 0) {
            return false;
        }
        
        // Validate memory limit (reasonable bounds)
        if (memory_limit == 0 || memory_limit > (1ULL << 32)) { // Max 4GB
            return false;
        }
        
        // Validate CPU time limit (reasonable bounds)
        if (cpu_time_limit == 0 || cpu_time_limit > 60000) { // Max 60 seconds
            return false;
        }
        
        // Check dependency names are valid
        for (const auto& dep : dependencies) {
            if (dep.name.empty() || !std::regex_match(dep.name, name_pattern)) {
                return false;
            }
        }
        
        return true;
    }

} // namespace ecscope::plugins