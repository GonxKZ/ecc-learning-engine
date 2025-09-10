#pragma once

#include <string>

/**
 * @brief CMake utilities for plugin development
 * 
 * This header provides constants and utilities that work with the 
 * ECScope plugin CMake module to simplify plugin build configuration.
 */

namespace ecscope::plugins::sdk::cmake {
    
    // Plugin build configuration constants
    constexpr const char* PLUGIN_API_VERSION = "1.0";
    constexpr const char* MINIMUM_CMAKE_VERSION = "3.16";
    constexpr const char* MINIMUM_CXX_STANDARD = "17";
    
    // Default plugin directories
    constexpr const char* DEFAULT_PLUGIN_SOURCE_DIR = "src";
    constexpr const char* DEFAULT_PLUGIN_INCLUDE_DIR = "include";
    constexpr const char* DEFAULT_PLUGIN_RESOURCE_DIR = "resources";
    constexpr const char* DEFAULT_PLUGIN_OUTPUT_DIR = "plugins";
    
    // Plugin manifest constants
    constexpr const char* PLUGIN_MANIFEST_FILENAME = "plugin.json";
    constexpr const char* PLUGIN_CMAKE_CONFIG = "PluginConfig.cmake.in";
    
    /**
     * @brief Generate CMakeLists.txt template for a plugin
     */
    std::string generate_cmakelists_template(
        const std::string& plugin_name,
        const std::string& plugin_version = "1.0.0",
        bool needs_rendering = false,
        bool needs_assets = false,
        bool needs_gui = false,
        bool needs_networking = false
    );
    
    /**
     * @brief Generate plugin config template
     */
    std::string generate_plugin_config_template(
        const std::string& plugin_name,
        const std::string& description = "",
        const std::string& author = ""
    );
    
    /**
     * @brief Generate plugin manifest template
     */
    std::string generate_plugin_manifest_template(
        const std::string& plugin_name,
        const std::string& plugin_version = "1.0.0",
        const std::string& description = "",
        const std::string& author = ""
    );
    
} // namespace ecscope::plugins::sdk::cmake

// CMake template strings
namespace ecscope::plugins::sdk::cmake::templates {
    
    constexpr const char* BASIC_CMAKELISTS_TEMPLATE = R"(
cmake_minimum_required(VERSION 3.16)
project({PLUGIN_NAME}_plugin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ECScope plugin framework
find_package(ECScope REQUIRED COMPONENTS Core Plugins{OPTIONAL_COMPONENTS})

# Plugin sources
set(PLUGIN_SOURCES
    src/{PLUGIN_NAME}_plugin.cpp
    # Add more source files here
)

# Plugin headers
set(PLUGIN_HEADERS
    include/{PLUGIN_NAME}_plugin.hpp
    # Add more header files here
)

# Create plugin library
add_library({PLUGIN_NAME}_plugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

# Link with ECScope
target_link_libraries({PLUGIN_NAME}_plugin 
    ECScope::Core 
    ECScope::Plugins{OPTIONAL_LINKS}
)

# Include directories
target_include_directories({PLUGIN_NAME}_plugin 
    PRIVATE include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set plugin properties
set_target_properties({PLUGIN_NAME}_plugin PROPERTIES
    OUTPUT_NAME "{PLUGIN_NAME}"
    PREFIX ""
    SUFFIX ".ecplugin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)

# Copy plugin manifest
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/plugin.json.in"
    "${CMAKE_BINARY_DIR}/plugins/{PLUGIN_NAME}/plugin.json"
    @ONLY
)

# Copy resources if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/" 
         DESTINATION "${CMAKE_BINARY_DIR}/plugins/{PLUGIN_NAME}/resources/")
endif()

# Install plugin
install(TARGETS {PLUGIN_NAME}_plugin
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)

install(FILES "${CMAKE_BINARY_DIR}/plugins/{PLUGIN_NAME}/plugin.json"
    DESTINATION plugins/{PLUGIN_NAME}
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    install(DIRECTORY resources/
        DESTINATION plugins/{PLUGIN_NAME}/resources
    )
endif()
)";

    constexpr const char* PLUGIN_HEADER_TEMPLATE = R"(
#pragma once

#include <ecscope/plugins/sdk/plugin_sdk.hpp>

class {PLUGIN_CLASS} : public ecscope::plugins::sdk::PluginBase {
public:
    {PLUGIN_CLASS}();
    virtual ~{PLUGIN_CLASS}() = default;
    
    // Plugin metadata
    static const ecscope::plugins::PluginMetadata& get_static_metadata() {
        static ecscope::plugins::PluginMetadata metadata = create_metadata();
        return metadata;
    }
    
protected:
    // Plugin lifecycle
    bool on_initialize() override;
    void on_shutdown() override;
    void update(double delta_time) override;
    
    // Event handlers
    void on_event(const std::string& event_name, const std::map<std::string, std::string>& params) override;
    
    // Message handlers
    std::string handle_message(const std::string& message, const std::map<std::string, std::string>& params) override;
    
    // Configuration
    void on_configure(const std::map<std::string, std::string>& config) override;
    
private:
    static ecscope::plugins::PluginMetadata create_metadata();
    
    // Plugin-specific members
    bool initialized_{false};
};

// Plugin export declaration
DECLARE_PLUGIN({PLUGIN_CLASS}, "{PLUGIN_NAME}", "{PLUGIN_VERSION}")
DECLARE_PLUGIN_API_VERSION()
)";

    constexpr const char* PLUGIN_SOURCE_TEMPLATE = R"(
#include "{PLUGIN_NAME}_plugin.hpp"
#include <iostream>

{PLUGIN_CLASS}::{PLUGIN_CLASS}() 
    : PluginBase("{PLUGIN_NAME}", {PLUGIN_VERSION_STRUCT}) {
    
    // Set plugin metadata
    set_display_name("{PLUGIN_DISPLAY_NAME}");
    set_description("{PLUGIN_DESCRIPTION}");
    set_author("{PLUGIN_AUTHOR}");
    set_license("MIT"); // Change as needed
    
    // Add tags
    add_tag("example");
    
    // Set priority if needed
    // set_priority(ecscope::plugins::PluginPriority::Normal);
}

bool {PLUGIN_CLASS}::on_initialize() {
    log_info("Initializing {PLUGIN_NAME} plugin");
    
    // Request necessary permissions
    if (!request_permission(ecscope::plugins::Permission::PluginCommunication, "For inter-plugin messaging")) {
        log_error("Failed to get communication permission");
        return false;
    }
    
    // Subscribe to events if needed
    // subscribe_to_event("engine.update", [this](const auto& params) {
    //     // Handle engine update event
    // });
    
    // Set message handlers
    // set_message_handler("ping", [this](const auto& params) -> std::string {
    //     return "pong";
    // });
    
    initialized_ = true;
    log_info("{PLUGIN_NAME} plugin initialized successfully");
    return true;
}

void {PLUGIN_CLASS}::on_shutdown() {
    log_info("Shutting down {PLUGIN_NAME} plugin");
    
    // Clean up resources
    initialized_ = false;
    
    log_info("{PLUGIN_NAME} plugin shutdown complete");
}

void {PLUGIN_CLASS}::update(double delta_time) {
    if (!initialized_) return;
    
    // Update plugin logic here
    // This is called every frame if the plugin is active
}

void {PLUGIN_CLASS}::on_event(const std::string& event_name, const std::map<std::string, std::string>& params) {
    log_debug("Received event: " + event_name);
    
    // Handle specific events
    if (event_name == "engine.update") {
        // Handle engine update
    } else if (event_name == "plugin.message") {
        // Handle plugin message event
    }
}

std::string {PLUGIN_CLASS}::handle_message(const std::string& message, const std::map<std::string, std::string>& params) {
    log_debug("Received message: " + message);
    
    if (message == "ping") {
        return "pong";
    } else if (message == "status") {
        return initialized_ ? "running" : "stopped";
    } else if (message == "info") {
        return get_metadata().display_name + " v" + get_metadata().version.to_string();
    }
    
    return ""; // Unknown message
}

void {PLUGIN_CLASS}::on_configure(const std::map<std::string, std::string>& config) {
    log_info("Configuring {PLUGIN_NAME} plugin");
    
    // Handle configuration changes
    for (const auto& [key, value] : config) {
        log_debug("Config: " + key + " = " + value);
        set_config(key, value);
    }
}

ecscope::plugins::PluginMetadata {PLUGIN_CLASS}::create_metadata() {
    ecscope::plugins::PluginMetadata metadata;
    
    metadata.name = "{PLUGIN_NAME}";
    metadata.display_name = "{PLUGIN_DISPLAY_NAME}";
    metadata.description = "{PLUGIN_DESCRIPTION}";
    metadata.author = "{PLUGIN_AUTHOR}";
    metadata.version = {PLUGIN_VERSION_STRUCT};
    metadata.license = "MIT";
    
    // Set resource limits
    metadata.memory_limit = 1024 * 1024 * 50; // 50MB
    metadata.cpu_time_limit = 100; // 100ms
    metadata.sandbox_required = true;
    
    // Add required permissions
    metadata.required_permissions.push_back("PluginCommunication");
    
    // Add dependencies if needed
    // metadata.dependencies.emplace_back("core_plugin", ecscope::plugins::PluginVersion{1, 0, 0});
    
    return metadata;
}
)";

    constexpr const char* PLUGIN_MANIFEST_TEMPLATE = R"(
{
    "name": "{PLUGIN_NAME}",
    "display_name": "{PLUGIN_DISPLAY_NAME}",
    "description": "{PLUGIN_DESCRIPTION}",
    "author": "{PLUGIN_AUTHOR}",
    "version": "{PLUGIN_VERSION}",
    "api_version": "1.0",
    "license": "MIT",
    "website": "",
    "tags": ["example"],
    
    "requirements": {
        "engine_version_min": "1.0.0",
        "engine_version_max": "2.0.0",
        "memory_limit": 52428800,
        "cpu_time_limit": 100,
        "sandbox_required": true
    },
    
    "permissions": [
        "PluginCommunication"
    ],
    
    "dependencies": [],
    
    "resources": {
        "textures": [],
        "shaders": [],
        "models": [],
        "audio": [],
        "scripts": []
    },
    
    "build": {
        "cmake_minimum_version": "3.16",
        "cxx_standard": "17",
        "output_name": "{PLUGIN_NAME}.ecplugin"
    }
}
)";

} // namespace ecscope::plugins::sdk::cmake::templates