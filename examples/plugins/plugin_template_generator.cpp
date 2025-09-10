/**
 * @file plugin_template_generator.cpp
 * @brief Plugin template generator utility
 * 
 * This utility generates plugin templates with boilerplate code to help
 * developers get started quickly with ECScope plugin development.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <filesystem>
#include <algorithm>
#include <regex>

class PluginTemplateGenerator {
public:
    enum class PluginType {
        Basic,
        Rendering,
        ECS,
        System,
        GUI
    };
    
    struct PluginConfig {
        std::string name;
        std::string display_name;
        std::string description;
        std::string author;
        std::string namespace_name;
        PluginType type{PluginType::Basic};
        bool needs_rendering{false};
        bool needs_ecs{false};
        bool needs_assets{false};
        bool needs_gui{false};
        bool needs_networking{false};
        bool has_update_loop{false};
        bool has_gui_window{false};
        std::vector<std::string> custom_permissions;
        std::vector<std::string> dependencies;
    };

public:
    bool generate_plugin_template(const PluginConfig& config, const std::string& output_dir) {
        if (!validate_config(config)) {
            return false;
        }
        
        // Create directory structure
        if (!create_directory_structure(config, output_dir)) {
            return false;
        }
        
        // Generate files
        if (!generate_header_file(config, output_dir)) return false;
        if (!generate_source_file(config, output_dir)) return false;
        if (!generate_cmake_file(config, output_dir)) return false;
        if (!generate_manifest_file(config, output_dir)) return false;
        if (!generate_readme_file(config, output_dir)) return false;
        
        std::cout << "Plugin template generated successfully in: " << output_dir << std::endl;
        return true;
    }

private:
    bool validate_config(const PluginConfig& config) {
        if (config.name.empty()) {
            std::cerr << "Error: Plugin name cannot be empty" << std::endl;
            return false;
        }
        
        // Check for valid identifier name
        std::regex name_pattern("^[a-zA-Z][a-zA-Z0-9_]*$");
        if (!std::regex_match(config.name, name_pattern)) {
            std::cerr << "Error: Plugin name must be a valid identifier" << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool create_directory_structure(const PluginConfig& config, const std::string& output_dir) {
        try {
            std::filesystem::create_directories(output_dir + "/src");
            std::filesystem::create_directories(output_dir + "/include");
            std::filesystem::create_directories(output_dir + "/resources");
            std::filesystem::create_directories(output_dir + "/tests");
            std::filesystem::create_directories(output_dir + "/docs");
            return true;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directories: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool generate_header_file(const PluginConfig& config, const std::string& output_dir) {
        std::string filename = output_dir + "/include/" + config.name + "_plugin.hpp";
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not create header file: " << filename << std::endl;
            return false;
        }
        
        std::string class_name = to_pascal_case(config.name) + "Plugin";
        
        file << "#pragma once\n\n";
        file << "#include <ecscope/plugins/sdk/plugin_sdk.hpp>\n";
        
        if (config.needs_rendering) {
            file << "#include <ecscope/plugins/rendering_integration.hpp>\n";
        }
        if (config.needs_ecs) {
            file << "#include <ecscope/plugins/ecs_integration.hpp>\n";
        }
        
        file << "\n";
        
        if (!config.namespace_name.empty()) {
            file << "namespace " << config.namespace_name << " {\n\n";
        }
        
        file << "/**\n";
        file << " * @brief " << config.display_name << "\n";
        file << " * \n";
        file << " * " << config.description << "\n";
        file << " */\n";
        file << "class " << class_name << " : public ecscope::plugins::sdk::PluginBase {\n";
        file << "public:\n";
        file << "    " << class_name << "();\n";
        file << "    virtual ~" << class_name << "() = default;\n\n";
        
        file << "    // Plugin metadata\n";
        file << "    static const ecscope::plugins::PluginMetadata& get_static_metadata();\n\n";
        
        file << "protected:\n";
        file << "    // Plugin lifecycle\n";
        file << "    bool on_initialize() override;\n";
        file << "    void on_shutdown() override;\n";
        
        if (config.has_update_loop) {
            file << "    void update(double delta_time) override;\n";
        }
        
        file << "    void on_pause() override;\n";
        file << "    void on_resume() override;\n";
        file << "    void on_configure(const std::map<std::string, std::string>& config) override;\n\n";
        
        if (config.needs_ecs) {
            file << "    // ECS integration\n";
            file << "    void setup_ecs_components();\n";
            file << "    void setup_ecs_systems();\n\n";
        }
        
        if (config.needs_rendering) {
            file << "    // Rendering integration\n";
            file << "    void setup_shaders();\n";
            file << "    void setup_render_passes();\n\n";
        }
        
        if (config.has_gui_window) {
            file << "    // GUI\n";
            file << "    void render_main_window();\n";
            file << "    void render_settings_window();\n\n";
        }
        
        file << "private:\n";
        file << "    // Plugin state\n";
        file << "    bool initialized_{false};\n";
        
        if (config.has_update_loop) {
            file << "    double update_time_{0.0};\n";
        }
        
        if (config.needs_rendering) {
            file << "    \n";
            file << "    // Rendering resources\n";
            file << "    // std::unique_ptr<ecscope::plugins::PluginRenderingHelper> rendering_helper_;\n";
        }
        
        if (config.needs_ecs) {
            file << "    \n";
            file << "    // ECS resources\n";
            file << "    // std::unique_ptr<ecscope::plugins::PluginECSHelper> ecs_helper_;\n";
        }
        
        file << "};\n\n";
        
        if (!config.namespace_name.empty()) {
            file << "} // namespace " << config.namespace_name << "\n\n";
        }
        
        // Plugin export macros
        std::string full_class_name = config.namespace_name.empty() ? 
            class_name : config.namespace_name + "::" + class_name;
        
        file << "// Plugin export\n";
        file << "DECLARE_PLUGIN(" << full_class_name << ", \"" << config.name << "\", \"1.0.0\")\n";
        file << "DECLARE_PLUGIN_API_VERSION()\n";
        
        file.close();
        std::cout << "Generated header file: " << filename << std::endl;
        return true;
    }
    
    bool generate_source_file(const PluginConfig& config, const std::string& output_dir) {
        std::string filename = output_dir + "/src/" + config.name + "_plugin.cpp";
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not create source file: " << filename << std::endl;
            return false;
        }
        
        std::string class_name = to_pascal_case(config.name) + "Plugin";
        
        file << "#include \"" << config.name << "_plugin.hpp\"\n";
        file << "#include <iostream>\n\n";
        
        if (!config.namespace_name.empty()) {
            file << "namespace " << config.namespace_name << " {\n\n";
        }
        
        // Constructor
        file << class_name << "::" << class_name << "()\n";
        file << "    : PluginBase(\"" << config.name << "\", {1, 0, 0}) {\n";
        file << "    \n";
        file << "    // Set plugin metadata\n";
        file << "    set_display_name(\"" << config.display_name << "\");\n";
        file << "    set_description(\"" << config.description << "\");\n";
        file << "    set_author(\"" << config.author << "\");\n";
        file << "    set_license(\"MIT\");\n";
        file << "    \n";
        file << "    // Add tags\n";
        
        if (config.type == PluginType::Basic) file << "    add_tag(\"basic\");\n";
        if (config.type == PluginType::Rendering) file << "    add_tag(\"rendering\");\n";
        if (config.type == PluginType::ECS) file << "    add_tag(\"ecs\");\n";
        if (config.type == PluginType::System) file << "    add_tag(\"system\");\n";
        if (config.type == PluginType::GUI) file << "    add_tag(\"gui\");\n";
        
        file << "    \n";
        file << "    // Set priority\n";
        file << "    set_priority(ecscope::plugins::PluginPriority::Normal);\n";
        file << "}\n\n";
        
        // Static metadata function
        file << "const ecscope::plugins::PluginMetadata& " << class_name << "::get_static_metadata() {\n";
        file << "    static ecscope::plugins::PluginMetadata metadata;\n";
        file << "    if (metadata.name.empty()) {\n";
        file << "        metadata.name = \"" << config.name << "\";\n";
        file << "        metadata.display_name = \"" << config.display_name << "\";\n";
        file << "        metadata.description = \"" << config.description << "\";\n";
        file << "        metadata.author = \"" << config.author << "\";\n";
        file << "        metadata.version = {1, 0, 0};\n";
        file << "        metadata.license = \"MIT\";\n";
        file << "        metadata.sandbox_required = true;\n";
        
        if (config.needs_rendering) {
            file << "        metadata.memory_limit = 1024 * 1024 * 100; // 100MB for rendering\n";
        } else {
            file << "        metadata.memory_limit = 1024 * 1024 * 50;  // 50MB\n";
        }
        
        file << "        metadata.cpu_time_limit = 100; // 100ms\n";
        file << "        \n";
        file << "        // Required permissions\n";
        file << "        metadata.required_permissions.push_back(\"PluginCommunication\");\n";
        
        if (config.needs_rendering) file << "        metadata.required_permissions.push_back(\"RenderingAccess\");\n";
        if (config.needs_ecs) file << "        metadata.required_permissions.push_back(\"ECCoreAccess\");\n";
        if (config.needs_assets) file << "        metadata.required_permissions.push_back(\"AssetAccess\");\n";
        if (config.needs_gui) file << "        metadata.required_permissions.push_back(\"GuiAccess\");\n";
        
        for (const auto& perm : config.custom_permissions) {
            file << "        metadata.required_permissions.push_back(\"" << perm << "\");\n";
        }
        
        file << "    }\n";
        file << "    return metadata;\n";
        file << "}\n\n";
        
        // Lifecycle methods
        file << "bool " << class_name << "::on_initialize() {\n";
        file << "    log_info(\"Initializing " << config.display_name << "\");\n";
        file << "    \n";
        file << "    // Request necessary permissions\n";
        
        std::vector<std::string> all_permissions = {"PluginCommunication"};
        if (config.needs_rendering) all_permissions.push_back("RenderingAccess");
        if (config.needs_ecs) all_permissions.push_back("ECCoreAccess");
        if (config.needs_assets) all_permissions.push_back("AssetAccess");
        if (config.needs_gui) all_permissions.push_back("GuiAccess");
        
        for (const auto& perm : all_permissions) {
            file << "    if (!request_permission(ecscope::plugins::Permission::" << perm << ", \n";
            file << "                          \"Required for plugin functionality\")) {\n";
            file << "        log_error(\"Failed to get " << perm << " permission\");\n";
            file << "        return false;\n";
            file << "    }\n";
        }
        
        file << "    \n";
        
        if (config.needs_ecs) {
            file << "    // Setup ECS integration\n";
            file << "    setup_ecs_components();\n";
            file << "    setup_ecs_systems();\n";
            file << "    \n";
        }
        
        if (config.needs_rendering) {
            file << "    // Setup rendering\n";
            file << "    setup_shaders();\n";
            file << "    setup_render_passes();\n";
            file << "    \n";
        }
        
        if (config.has_gui_window) {
            file << "    // Setup GUI\n";
            file << "    add_gui_window(\"Main Window\", [this]() { render_main_window(); });\n";
            file << "    add_gui_window(\"Settings\", [this]() { render_settings_window(); });\n";
            file << "    \n";
        }
        
        file << "    initialized_ = true;\n";
        file << "    log_info(\"" << config.display_name << " initialized successfully\");\n";
        file << "    return true;\n";
        file << "}\n\n";
        
        file << "void " << class_name << "::on_shutdown() {\n";
        file << "    log_info(\"Shutting down " << config.display_name << "\");\n";
        file << "    initialized_ = false;\n";
        file << "    log_info(\"" << config.display_name << " shutdown complete\");\n";
        file << "}\n\n";
        
        if (config.has_update_loop) {
            file << "void " << class_name << "::update(double delta_time) {\n";
            file << "    if (!initialized_) return;\n";
            file << "    \n";
            file << "    update_time_ += delta_time;\n";
            file << "    \n";
            file << "    // TODO: Add update logic here\n";
            file << "}\n\n";
        }
        
        file << "void " << class_name << "::on_pause() {\n";
        file << "    log_info(\"" << config.display_name << " paused\");\n";
        file << "}\n\n";
        
        file << "void " << class_name << "::on_resume() {\n";
        file << "    log_info(\"" << config.display_name << " resumed\");\n";
        file << "}\n\n";
        
        file << "void " << class_name << "::on_configure(const std::map<std::string, std::string>& config) {\n";
        file << "    log_info(\"" << config.display_name << " configuration updated\");\n";
        file << "    \n";
        file << "    for (const auto& [key, value] : config) {\n";
        file << "        log_debug(\"Config: \" + key + \" = \" + value);\n";
        file << "        set_config(key, value);\n";
        file << "    }\n";
        file << "}\n\n";
        
        // Additional methods based on config
        if (config.needs_ecs) {
            generate_ecs_methods(file, class_name);
        }
        
        if (config.needs_rendering) {
            generate_rendering_methods(file, class_name);
        }
        
        if (config.has_gui_window) {
            generate_gui_methods(file, class_name);
        }
        
        if (!config.namespace_name.empty()) {
            file << "} // namespace " << config.namespace_name << "\n";
        }
        
        file.close();
        std::cout << "Generated source file: " << filename << std::endl;
        return true;
    }
    
    void generate_ecs_methods(std::ofstream& file, const std::string& class_name) {
        file << "void " << class_name << "::setup_ecs_components() {\n";
        file << "    log_info(\"Setting up ECS components\");\n";
        file << "    \n";
        file << "    // TODO: Register custom components here\n";
        file << "    // register_component<YourComponent>(\"YourComponent\");\n";
        file << "}\n\n";
        
        file << "void " << class_name << "::setup_ecs_systems() {\n";
        file << "    log_info(\"Setting up ECS systems\");\n";
        file << "    \n";
        file << "    // TODO: Register custom systems here\n";
        file << "    // register_system<YourSystem>(\"YourSystem\");\n";
        file << "}\n\n";
    }
    
    void generate_rendering_methods(std::ofstream& file, const std::string& class_name) {
        file << "void " << class_name << "::setup_shaders() {\n";
        file << "    log_info(\"Setting up shaders\");\n";
        file << "    \n";
        file << "    // TODO: Create and register shaders\n";
        file << "    // create_shader(\"my_shader\", vertex_source, fragment_source);\n";
        file << "}\n\n";
        
        file << "void " << class_name << "::setup_render_passes() {\n";
        file << "    log_info(\"Setting up render passes\");\n";
        file << "    \n";
        file << "    // TODO: Add custom render passes\n";
        file << "    // add_render_pass(\"my_pass\", [this](auto* renderer) {\n";
        file << "    //     // Custom rendering logic\n";
        file << "    // });\n";
        file << "}\n\n";
    }
    
    void generate_gui_methods(std::ofstream& file, const std::string& class_name) {
        file << "void " << class_name << "::render_main_window() {\n";
        file << "    // TODO: Implement main window GUI\n";
        file << "    // This would use ImGui calls in a real implementation\n";
        file << "}\n\n";
        
        file << "void " << class_name << "::render_settings_window() {\n";
        file << "    // TODO: Implement settings window GUI\n";
        file << "    // This would use ImGui calls in a real implementation\n";
        file << "}\n\n";
    }
    
    bool generate_cmake_file(const PluginConfig& config, const std::string& output_dir) {
        std::string filename = output_dir + "/CMakeLists.txt";
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not create CMake file: " << filename << std::endl;
            return false;
        }
        
        file << "cmake_minimum_required(VERSION 3.16)\n";
        file << "project(" << config.name << "_plugin)\n\n";
        
        file << "# Set C++ standard\n";
        file << "set(CMAKE_CXX_STANDARD 17)\n";
        file << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
        
        file << "# Find ECScope plugin framework\n";
        file << "find_package(ECScope REQUIRED COMPONENTS Core Plugins";
        if (config.needs_rendering) file << " Rendering";
        if (config.needs_ecs) file << " ECS";
        if (config.needs_assets) file << " Assets";
        if (config.needs_gui) file << " GUI";
        file << ")\n\n";
        
        file << "# Plugin sources\n";
        file << "set(PLUGIN_SOURCES\n";
        file << "    src/" << config.name << "_plugin.cpp\n";
        file << ")\n\n";
        
        file << "# Plugin headers\n";
        file << "set(PLUGIN_HEADERS\n";
        file << "    include/" << config.name << "_plugin.hpp\n";
        file << ")\n\n";
        
        file << "# Create plugin library\n";
        file << "add_library(" << config.name << "_plugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})\n\n";
        
        file << "# Link with ECScope\n";
        file << "target_link_libraries(" << config.name << "_plugin\n";
        file << "    ECScope::Core\n";
        file << "    ECScope::Plugins\n";
        if (config.needs_rendering) file << "    ECScope::Rendering\n";
        if (config.needs_ecs) file << "    ECScope::ECS\n";
        if (config.needs_assets) file << "    ECScope::Assets\n";
        if (config.needs_gui) file << "    ECScope::GUI\n";
        file << ")\n\n";
        
        file << "# Include directories\n";
        file << "target_include_directories(" << config.name << "_plugin\n";
        file << "    PRIVATE include\n";
        file << "    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}\n";
        file << ")\n\n";
        
        file << "# Set plugin properties\n";
        file << "set_target_properties(" << config.name << "_plugin PROPERTIES\n";
        file << "    OUTPUT_NAME \"" << config.name << "\"\n";
        file << "    PREFIX \"\"\n";
        file << "    SUFFIX \".ecplugin\"\n";
        file << "    RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_BINARY_DIR}/plugins\"\n";
        file << "    LIBRARY_OUTPUT_DIRECTORY \"${CMAKE_BINARY_DIR}/plugins\"\n";
        file << ")\n\n";
        
        file << "# Install plugin\n";
        file << "install(TARGETS " << config.name << "_plugin\n";
        file << "    LIBRARY DESTINATION plugins\n";
        file << "    RUNTIME DESTINATION plugins\n";
        file << ")\n\n";
        
        file.close();
        std::cout << "Generated CMake file: " << filename << std::endl;
        return true;
    }
    
    bool generate_manifest_file(const PluginConfig& config, const std::string& output_dir) {
        std::string filename = output_dir + "/plugin.json.in";
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not create manifest file: " << filename << std::endl;
            return false;
        }
        
        file << "{\n";
        file << "    \"name\": \"" << config.name << "\",\n";
        file << "    \"display_name\": \"" << config.display_name << "\",\n";
        file << "    \"description\": \"" << config.description << "\",\n";
        file << "    \"author\": \"" << config.author << "\",\n";
        file << "    \"version\": \"1.0.0\",\n";
        file << "    \"api_version\": \"1.0\",\n";
        file << "    \"license\": \"MIT\",\n";
        file << "    \"website\": \"\",\n";
        
        file << "    \"tags\": [";
        bool first = true;
        if (config.type == PluginType::Basic) { file << (first ? "" : ", ") << "\"basic\""; first = false; }
        if (config.type == PluginType::Rendering) { file << (first ? "" : ", ") << "\"rendering\""; first = false; }
        if (config.type == PluginType::ECS) { file << (first ? "" : ", ") << "\"ecs\""; first = false; }
        if (config.type == PluginType::System) { file << (first ? "" : ", ") << "\"system\""; first = false; }
        if (config.type == PluginType::GUI) { file << (first ? "" : ", ") << "\"gui\""; first = false; }
        file << "],\n";
        
        file << "    \"requirements\": {\n";
        file << "        \"engine_version_min\": \"1.0.0\",\n";
        file << "        \"engine_version_max\": \"2.0.0\",\n";
        
        if (config.needs_rendering) {
            file << "        \"memory_limit\": 104857600,\n"; // 100MB
        } else {
            file << "        \"memory_limit\": 52428800,\n"; // 50MB
        }
        
        file << "        \"cpu_time_limit\": 100,\n";
        file << "        \"sandbox_required\": true\n";
        file << "    },\n";
        
        file << "    \"permissions\": [\n";
        file << "        \"PluginCommunication\"";
        if (config.needs_rendering) file << ",\n        \"RenderingAccess\"";
        if (config.needs_ecs) file << ",\n        \"ECCoreAccess\"";
        if (config.needs_assets) file << ",\n        \"AssetAccess\"";
        if (config.needs_gui) file << ",\n        \"GuiAccess\"";
        file << "\n    ],\n";
        
        file << "    \"dependencies\": [";
        first = true;
        for (const auto& dep : config.dependencies) {
            file << (first ? "" : ",") << "\n        {\"name\": \"" << dep << "\", \"version\": \"1.0.0\"}";
            first = false;
        }
        file << "\n    ]\n";
        
        file << "}\n";
        
        file.close();
        std::cout << "Generated manifest file: " << filename << std::endl;
        return true;
    }
    
    bool generate_readme_file(const PluginConfig& config, const std::string& output_dir) {
        std::string filename = output_dir + "/README.md";
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not create README file: " << filename << std::endl;
            return false;
        }
        
        file << "# " << config.display_name << "\n\n";
        file << config.description << "\n\n";
        
        file << "## Features\n\n";
        if (config.needs_rendering) file << "- Custom rendering integration\n";
        if (config.needs_ecs) file << "- ECS components and systems\n";
        if (config.needs_assets) file << "- Asset management\n";
        if (config.needs_gui) file << "- GUI interface\n";
        if (config.has_update_loop) file << "- Real-time updates\n";
        
        file << "\n## Building\n\n";
        file << "```bash\n";
        file << "mkdir build\n";
        file << "cd build\n";
        file << "cmake ..\n";
        file << "make\n";
        file << "```\n\n";
        
        file << "## Installation\n\n";
        file << "Copy the generated `.ecplugin` file to your ECScope plugins directory.\n\n";
        
        file << "## Configuration\n\n";
        file << "The plugin accepts the following configuration options:\n\n";
        file << "- `enabled`: Enable/disable the plugin (default: true)\n";
        file << "- Add your configuration options here...\n\n";
        
        file << "## Usage\n\n";
        file << "Describe how to use your plugin here.\n\n";
        
        file << "## License\n\n";
        file << "MIT License - see LICENSE file for details.\n\n";
        
        file << "## Author\n\n";
        file << config.author << "\n";
        
        file.close();
        std::cout << "Generated README file: " << filename << std::endl;
        return true;
    }
    
    std::string to_pascal_case(const std::string& input) {
        std::string result;
        bool capitalize_next = true;
        
        for (char c : input) {
            if (c == '_') {
                capitalize_next = true;
            } else if (capitalize_next) {
                result += std::toupper(c);
                capitalize_next = false;
            } else {
                result += c;
            }
        }
        
        return result;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "ECScope Plugin Template Generator\n";
        std::cout << "Usage: " << argv[0] << " [plugin_name] [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --type [basic|rendering|ecs|system|gui]  Plugin type (default: basic)\n";
        std::cout << "  --author [name]                          Plugin author\n";
        std::cout << "  --description [text]                     Plugin description\n";
        std::cout << "  --namespace [name]                       C++ namespace\n";
        std::cout << "  --output-dir [path]                      Output directory\n";
        std::cout << "  --rendering                              Enable rendering support\n";
        std::cout << "  --ecs                                    Enable ECS support\n";
        std::cout << "  --assets                                 Enable asset support\n";
        std::cout << "  --gui                                    Enable GUI support\n";
        std::cout << "  --update-loop                            Enable update loop\n";
        return 1;
    }
    
    PluginTemplateGenerator generator;
    PluginTemplateGenerator::PluginConfig config;
    
    config.name = argv[1];
    config.display_name = config.name + " Plugin";
    config.description = "A custom ECScope plugin";
    config.author = "Plugin Developer";
    
    std::string output_dir = "./" + config.name + "_plugin";
    
    // Parse command line arguments
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--type" && i + 1 < argc) {
            std::string type = argv[++i];
            if (type == "basic") config.type = PluginTemplateGenerator::PluginType::Basic;
            else if (type == "rendering") config.type = PluginTemplateGenerator::PluginType::Rendering;
            else if (type == "ecs") config.type = PluginTemplateGenerator::PluginType::ECS;
            else if (type == "system") config.type = PluginTemplateGenerator::PluginType::System;
            else if (type == "gui") config.type = PluginTemplateGenerator::PluginType::GUI;
        } else if (arg == "--author" && i + 1 < argc) {
            config.author = argv[++i];
        } else if (arg == "--description" && i + 1 < argc) {
            config.description = argv[++i];
        } else if (arg == "--namespace" && i + 1 < argc) {
            config.namespace_name = argv[++i];
        } else if (arg == "--output-dir" && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (arg == "--rendering") {
            config.needs_rendering = true;
        } else if (arg == "--ecs") {
            config.needs_ecs = true;
        } else if (arg == "--assets") {
            config.needs_assets = true;
        } else if (arg == "--gui") {
            config.needs_gui = true;
        } else if (arg == "--update-loop") {
            config.has_update_loop = true;
        }
    }
    
    // Auto-configure based on type
    if (config.type == PluginTemplateGenerator::PluginType::Rendering) {
        config.needs_rendering = true;
        config.needs_assets = true;
        config.has_update_loop = true;
    } else if (config.type == PluginTemplateGenerator::PluginType::ECS) {
        config.needs_ecs = true;
        config.has_update_loop = true;
    } else if (config.type == PluginTemplateGenerator::PluginType::GUI) {
        config.needs_gui = true;
    }
    
    if (!generator.generate_plugin_template(config, output_dir)) {
        std::cerr << "Failed to generate plugin template" << std::endl;
        return 1;
    }
    
    std::cout << "\nPlugin template generated successfully!\n";
    std::cout << "Next steps:\n";
    std::cout << "1. cd " << output_dir << "\n";
    std::cout << "2. mkdir build && cd build\n";
    std::cout << "3. cmake ..\n";
    std::cout << "4. make\n";
    
    return 0;
}