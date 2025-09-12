/**
 * @file help_integration.cpp
 * @brief Implementation of help system integration with ECScope UI
 */

#include "ecscope/gui/help_integration.hpp"
#include <sstream>
#include <algorithm>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace ecscope::gui::help {

// =============================================================================
// HELP INTEGRATION MANAGER
// =============================================================================

HelpIntegration& HelpIntegration::Get() {
    static HelpIntegration instance;
    return instance;
}

void HelpIntegration::Initialize() {
    // Register all component help contexts
    RegisterDashboardHelp();
    RegisterECSInspectorHelp();
    RegisterRenderingHelp();
    RegisterPhysicsHelp();
    RegisterAudioHelp();
    RegisterNetworkingHelp();
    RegisterAssetPipelineHelp();
    RegisterDebugToolsHelp();
    RegisterPluginManagerHelp();
    RegisterScriptingHelp();
    RegisterResponsiveDesignHelp();
}

void HelpIntegration::RegisterDashboardHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Dashboard main window
    provider.RegisterContext("dashboard_main", {
        "dashboard_main",
        "The main dashboard provides an overview of your project and quick access to all major systems. "
        "Use the navigation bar to switch between different views and the status bar to monitor system performance.",
        {"project_management", "navigation", "performance_monitoring"},
        "tutorial_dashboard_basics",
        UserLevel::Beginner
    });
    
    // Project panel
    provider.RegisterContext("dashboard_project", {
        "dashboard_project",
        "Manage your project settings, configure build options, and access project-wide preferences. "
        "The project panel is your central hub for project configuration.",
        {"project_settings", "build_configuration", "version_control"},
        "tutorial_project_setup",
        UserLevel::Beginner
    });
    
    // Quick actions
    provider.RegisterContext("dashboard_quickactions", {
        "dashboard_quickactions",
        "Quick actions provide one-click access to frequently used operations. "
        "Customize this panel by right-clicking to add or remove actions.",
        {"shortcuts", "customization", "workflow_optimization"},
        std::nullopt,
        UserLevel::Intermediate
    });
    
    // Statistics panel
    provider.RegisterContext("dashboard_stats", {
        "dashboard_stats",
        "Real-time statistics display current engine performance metrics, resource usage, "
        "and system health. Click on any metric for detailed analysis.",
        {"performance", "profiling", "optimization"},
        "tutorial_performance_monitoring",
        UserLevel::Intermediate
    });
}

void HelpIntegration::RegisterECSInspectorHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Entity hierarchy
    provider.RegisterContext("ecs_hierarchy", {
        "ecs_hierarchy",
        "The entity hierarchy displays all entities in your scene in a tree structure. "
        "Drag and drop to reparent entities, right-click for context menu options.",
        {"entities", "scene_management", "hierarchy"},
        "tutorial_ecs_basics",
        UserLevel::Beginner
    });
    
    // Component list
    provider.RegisterContext("ecs_components", {
        "ecs_components",
        "View and edit all components attached to the selected entity. "
        "Click '+' to add new components, 'x' to remove them. Components can be enabled/disabled individually.",
        {"components", "entity_editing", "component_system"},
        "tutorial_component_system",
        UserLevel::Beginner
    });
    
    // Systems panel
    provider.RegisterContext("ecs_systems", {
        "ecs_systems",
        "Manage and monitor all active systems in your ECS. Systems can be enabled/disabled, "
        "reordered for execution priority, and profiled for performance.",
        {"systems", "execution_order", "system_profiling"},
        "tutorial_ecs_systems",
        UserLevel::Intermediate
    });
    
    // Query builder
    provider.RegisterContext("ecs_query", {
        "ecs_query",
        "Build complex queries to filter entities based on component combinations. "
        "Use the visual query builder or write queries directly in the advanced mode.",
        {"queries", "filtering", "entity_selection"},
        "tutorial_advanced_queries",
        UserLevel::Advanced
    });
    
    // Archetype viewer
    provider.RegisterContext("ecs_archetypes", {
        "ecs_archetypes",
        "View memory layout and performance characteristics of different entity archetypes. "
        "This advanced view helps optimize component combinations for cache efficiency.",
        {"archetypes", "memory_optimization", "cache_efficiency"},
        std::nullopt,
        UserLevel::Expert
    });
}

void HelpIntegration::RegisterRenderingHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Viewport
    provider.RegisterContext("rendering_viewport", {
        "rendering_viewport",
        "The main rendering viewport. Use mouse to navigate: Left-click drag to rotate, "
        "middle-click drag to pan, scroll to zoom. Press 'F' to focus on selected object.",
        {"viewport_navigation", "camera_controls", "view_modes"},
        "tutorial_viewport_navigation",
        UserLevel::Beginner
    });
    
    // Render settings
    provider.RegisterContext("rendering_settings", {
        "rendering_settings",
        "Configure global rendering settings including resolution, anti-aliasing, "
        "shadows, and post-processing effects. Changes apply in real-time.",
        {"graphics_settings", "quality_presets", "performance_tuning"},
        "tutorial_graphics_settings",
        UserLevel::Intermediate
    });
    
    // Shader editor
    provider.RegisterContext("rendering_shaders", {
        "rendering_shaders",
        "Edit and hot-reload shaders with syntax highlighting and error checking. "
        "The shader graph provides a visual alternative to code-based shader creation.",
        {"shaders", "material_editing", "shader_graph"},
        "tutorial_shader_creation",
        UserLevel::Advanced
    });
    
    // Material editor
    provider.RegisterContext("rendering_materials", {
        "rendering_materials",
        "Create and edit materials with real-time preview. Drag textures from the asset browser, "
        "adjust parameters with sliders, and see changes instantly in the viewport.",
        {"materials", "textures", "pbr_workflow"},
        "tutorial_material_creation",
        UserLevel::Intermediate
    });
    
    // Lighting panel
    provider.RegisterContext("rendering_lighting", {
        "rendering_lighting",
        "Configure scene lighting including directional, point, and spot lights. "
        "Adjust global illumination settings and bake lightmaps for optimal performance.",
        {"lighting", "global_illumination", "lightmapping"},
        "tutorial_lighting_setup",
        UserLevel::Intermediate
    });
}

void HelpIntegration::RegisterPhysicsHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Physics world settings
    provider.RegisterContext("physics_world", {
        "physics_world",
        "Configure global physics settings including gravity, simulation rate, and solver iterations. "
        "Higher iterations provide more accurate simulations at the cost of performance.",
        {"physics_settings", "simulation", "solver_configuration"},
        "tutorial_physics_basics",
        UserLevel::Intermediate
    });
    
    // Collision layers
    provider.RegisterContext("physics_layers", {
        "physics_layers",
        "Define collision layers and their interaction matrix. Objects on different layers "
        "can be configured to collide or ignore each other for optimized physics queries.",
        {"collision_layers", "layer_matrix", "collision_filtering"},
        "tutorial_collision_setup",
        UserLevel::Intermediate
    });
    
    // Rigid body editor
    provider.RegisterContext("physics_rigidbody", {
        "physics_rigidbody",
        "Configure rigid body properties including mass, drag, and constraints. "
        "Use the constraint editor to create joints and complex mechanical systems.",
        {"rigid_bodies", "constraints", "joints"},
        "tutorial_rigidbody_setup",
        UserLevel::Intermediate
    });
    
    // Physics debugger
    provider.RegisterContext("physics_debug", {
        "physics_debug",
        "Visualize physics simulation with collision shapes, contact points, and forces. "
        "Enable different visualization modes to debug specific physics issues.",
        {"physics_debugging", "collision_visualization", "force_display"},
        std::nullopt,
        UserLevel::Advanced
    });
}

void HelpIntegration::RegisterAudioHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Audio mixer
    provider.RegisterContext("audio_mixer", {
        "audio_mixer",
        "The master audio mixer controls volume levels and effects for different audio channels. "
        "Create custom mix groups and apply real-time effects chains.",
        {"audio_mixing", "volume_control", "audio_groups"},
        "tutorial_audio_mixing",
        UserLevel::Intermediate
    });
    
    // 3D audio settings
    provider.RegisterContext("audio_3d", {
        "audio_3d",
        "Configure 3D spatial audio including doppler effect, distance attenuation, and reverb zones. "
        "The 3D visualizer shows audio source positions and propagation.",
        {"spatial_audio", "3d_sound", "audio_zones"},
        "tutorial_3d_audio",
        UserLevel::Intermediate
    });
    
    // Audio source editor
    provider.RegisterContext("audio_source", {
        "audio_source",
        "Edit audio source properties including pitch, volume, and looping settings. "
        "Preview changes in real-time and configure trigger conditions.",
        {"audio_sources", "sound_properties", "audio_triggers"},
        std::nullopt,
        UserLevel::Beginner
    });
    
    // Effects rack
    provider.RegisterContext("audio_effects", {
        "audio_effects",
        "Apply and configure audio effects including reverb, echo, distortion, and filters. "
        "Chain multiple effects and save presets for reuse.",
        {"audio_effects", "dsp", "effect_chains"},
        "tutorial_audio_effects",
        UserLevel::Advanced
    });
}

void HelpIntegration::RegisterNetworkingHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Network manager
    provider.RegisterContext("network_manager", {
        "network_manager",
        "Manage network connections, configure server/client settings, and monitor network traffic. "
        "The connection panel shows all active connections with latency and bandwidth metrics.",
        {"networking", "multiplayer", "connection_management"},
        "tutorial_networking_basics",
        UserLevel::Advanced
    });
    
    // Replication settings
    provider.RegisterContext("network_replication", {
        "network_replication",
        "Configure which components and entities are replicated across the network. "
        "Set update rates and priority levels for optimized bandwidth usage.",
        {"replication", "network_sync", "bandwidth_optimization"},
        "tutorial_replication_setup",
        UserLevel::Advanced
    });
    
    // Network profiler
    provider.RegisterContext("network_profiler", {
        "network_profiler",
        "Analyze network performance with detailed packet inspection and bandwidth graphs. "
        "Identify bottlenecks and optimize network traffic patterns.",
        {"network_profiling", "packet_analysis", "bandwidth_monitoring"},
        std::nullopt,
        UserLevel::Expert
    });
}

void HelpIntegration::RegisterAssetPipelineHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Asset browser
    provider.RegisterContext("assets_browser", {
        "assets_browser",
        "Browse and manage all project assets. Drag assets into the scene or inspector to use them. "
        "Right-click for import/export options and asset-specific operations.",
        {"asset_management", "file_browser", "asset_import"},
        "tutorial_asset_management",
        UserLevel::Beginner
    });
    
    // Import settings
    provider.RegisterContext("assets_import", {
        "assets_import",
        "Configure import settings for different asset types. These settings determine "
        "how assets are processed and optimized when imported into the project.",
        {"import_settings", "asset_processing", "optimization"},
        "tutorial_asset_importing",
        UserLevel::Intermediate
    });
    
    // Asset preview
    provider.RegisterContext("assets_preview", {
        "assets_preview",
        "Preview assets before importing or using them. The preview panel supports "
        "textures, models, audio, and other asset types with format-specific controls.",
        {"asset_preview", "file_inspection", "format_support"},
        std::nullopt,
        UserLevel::Beginner
    });
    
    // Asset dependencies
    provider.RegisterContext("assets_dependencies", {
        "assets_dependencies",
        "View and manage asset dependencies to understand relationships between assets. "
        "This helps identify unused assets and optimize build size.",
        {"dependencies", "asset_relationships", "build_optimization"},
        std::nullopt,
        UserLevel::Advanced
    });
}

void HelpIntegration::RegisterDebugToolsHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Performance profiler
    provider.RegisterContext("debug_profiler", {
        "debug_profiler",
        "Profile CPU and GPU performance to identify bottlenecks. The flame graph shows "
        "hierarchical time spent in different functions and systems.",
        {"profiling", "performance_analysis", "optimization"},
        "tutorial_profiling",
        UserLevel::Intermediate
    });
    
    // Memory analyzer
    provider.RegisterContext("debug_memory", {
        "debug_memory",
        "Analyze memory usage and detect leaks. The memory map shows allocation patterns "
        "and helps identify memory fragmentation issues.",
        {"memory_profiling", "leak_detection", "memory_optimization"},
        "tutorial_memory_analysis",
        UserLevel::Advanced
    });
    
    // Console
    provider.RegisterContext("debug_console", {
        "debug_console",
        "Execute commands and view log output. Type 'help' for available commands. "
        "The console supports auto-completion and command history.",
        {"console", "commands", "logging"},
        std::nullopt,
        UserLevel::Intermediate
    });
    
    // Frame debugger
    provider.RegisterContext("debug_frame", {
        "debug_frame",
        "Step through individual draw calls and render passes to debug rendering issues. "
        "Inspect textures, buffers, and shader states at each step.",
        {"frame_debugging", "render_debugging", "gpu_debugging"},
        "tutorial_frame_debugging",
        UserLevel::Expert
    });
}

void HelpIntegration::RegisterPluginManagerHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Plugin list
    provider.RegisterContext("plugins_list", {
        "plugins_list",
        "View and manage installed plugins. Enable/disable plugins, check for updates, "
        "and access plugin-specific settings and documentation.",
        {"plugins", "extensions", "plugin_management"},
        "tutorial_plugin_system",
        UserLevel::Intermediate
    });
    
    // Plugin marketplace
    provider.RegisterContext("plugins_marketplace", {
        "plugins_marketplace",
        "Browse and install plugins from the community marketplace. Filter by category, "
        "rating, and compatibility. Read reviews before installing.",
        {"marketplace", "plugin_discovery", "community_content"},
        std::nullopt,
        UserLevel::Beginner
    });
    
    // Plugin development
    provider.RegisterContext("plugins_development", {
        "plugins_development",
        "Create custom plugins using the plugin SDK. Access API documentation, "
        "templates, and debugging tools for plugin development.",
        {"plugin_development", "sdk", "api"},
        "tutorial_plugin_creation",
        UserLevel::Expert
    });
}

void HelpIntegration::RegisterScriptingHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Script editor
    provider.RegisterContext("scripting_editor", {
        "scripting_editor",
        "Write and edit scripts with syntax highlighting, auto-completion, and error checking. "
        "Press Ctrl+Space for IntelliSense, F5 to run, and F9 to set breakpoints.",
        {"scripting", "code_editor", "intellisense"},
        "tutorial_scripting_basics",
        UserLevel::Intermediate
    });
    
    // Script debugger
    provider.RegisterContext("scripting_debugger", {
        "scripting_debugger",
        "Debug scripts with breakpoints, step execution, and variable inspection. "
        "The call stack shows the execution flow and local variables.",
        {"debugging", "breakpoints", "script_debugging"},
        "tutorial_script_debugging",
        UserLevel::Intermediate
    });
    
    // Hot reload
    provider.RegisterContext("scripting_hotreload", {
        "scripting_hotreload",
        "Enable hot reload to see script changes without restarting. "
        "The system preserves state where possible during reload.",
        {"hot_reload", "live_coding", "rapid_iteration"},
        std::nullopt,
        UserLevel::Advanced
    });
    
    // Script bindings
    provider.RegisterContext("scripting_bindings", {
        "scripting_bindings",
        "Configure script bindings to expose engine functionality to scripts. "
        "Create custom bindings for your C++ classes and functions.",
        {"bindings", "script_api", "interop"},
        "tutorial_script_bindings",
        UserLevel::Expert
    });
}

void HelpIntegration::RegisterResponsiveDesignHelp() {
    auto& provider = HelpSystem::Get().GetContextProvider();
    
    // Layout manager
    provider.RegisterContext("responsive_layout", {
        "responsive_layout",
        "Design responsive layouts that adapt to different screen sizes and orientations. "
        "Use the device simulator to preview layouts on various devices.",
        {"responsive_design", "layout", "adaptive_ui"},
        "tutorial_responsive_design",
        UserLevel::Intermediate
    });
    
    // DPI scaling
    provider.RegisterContext("responsive_dpi", {
        "responsive_dpi",
        "Configure DPI scaling for high-resolution displays. The system automatically "
        "adjusts UI elements to maintain consistent physical size across devices.",
        {"dpi_scaling", "high_dpi", "resolution_independence"},
        std::nullopt,
        UserLevel::Intermediate
    });
    
    // Device preview
    provider.RegisterContext("responsive_preview", {
        "responsive_preview",
        "Preview your UI on different device profiles including phones, tablets, and desktops. "
        "Create custom device profiles for specific target hardware.",
        {"device_preview", "testing", "multi_device"},
        std::nullopt,
        UserLevel::Beginner
    });
}

void HelpIntegration::AddHelpButton(const std::string& helpTopic) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::SameLine(ImGui::GetWindowWidth() - 30);
    if (ImGui::SmallButton("?")) {
        HelpSystem::Get().ShowHelp(helpTopic);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Click for help on this topic");
    }
#endif
}

void HelpIntegration::AddContextMenuHelp(const std::string& helpTopic) {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Help")) {
            HelpSystem::Get().ShowHelp(helpTopic);
        }
        ImGui::EndPopup();
    }
#endif
}

void HelpIntegration::ShowInlineHelp(const std::string& text) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(text.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
#endif
}

void HelpIntegration::MonitorUserActivity() {
    // Update activity tracker
    m_tracker.idleTime += ImGui::GetIO().DeltaTime;
    
    // Reset idle time on any input
    if (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered()) {
        m_tracker.idleTime = 0.0f;
    }
    
    // Check if user might need help
    if (m_proactiveHelpEnabled && m_tracker.idleTime > m_helpSuggestionThreshold) {
        // Suggest help based on current context
        if (m_tracker.errorCount > 3) {
            ShowNotification("Need help? Press F1 for assistance", 5.0f);
            m_tracker.errorCount = 0;
        }
    }
}

// =============================================================================
// UI HELP WIDGETS
// =============================================================================

void HelpIcon(const std::string& helpText, const std::string& helpTopic) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(helpText.c_str());
        if (!helpTopic.empty()) {
            ImGui::Separator();
            ImGui::TextDisabled("Click for more information");
        }
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
        
        if (!helpTopic.empty() && ImGui::IsItemClicked()) {
            HelpSystem::Get().ShowHelp(helpTopic);
        }
    }
#endif
}

bool HelpButton(const std::string& helpTopic) {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::SmallButton("?")) {
        HelpSystem::Get().ShowHelp(helpTopic);
        return true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Get help on this topic");
    }
#endif
    return false;
}

void HelpText(const std::string& text) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopStyleColor();
#endif
}

void HelpSection(const std::string& title, const std::string& content) {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        HelpText(content);
        ImGui::Unindent();
    }
#endif
}

bool TutorialButton(const std::string& tutorialId, const std::string& label) {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Button(label.c_str())) {
        HelpSystem::Get().StartTutorial(tutorialId);
        return true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Start interactive tutorial");
    }
#endif
    return false;
}

void TutorialProgress() {
#ifdef ECSCOPE_HAS_IMGUI
    // This would show progress of active tutorial
    // Implementation depends on tutorial system state
#endif
}

void HotkeyReminder(const std::string& action, const std::string& keys) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("%s", action.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", keys.c_str());
#endif
}

void FirstTimeHint(const std::string& id, const std::string& hint) {
#ifdef ECSCOPE_HAS_IMGUI
    static std::unordered_set<std::string> shownHints;
    
    if (shownHints.find(id) == shownHints.end()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
        ImGui::TextWrapped("Hint: %s", hint.c_str());
        ImGui::PopStyleColor();
        
        if (ImGui::SmallButton("Got it!")) {
            shownHints.insert(id);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Don't show again")) {
            shownHints.insert(id);
            // Save preference
        }
    }
#endif
}

// =============================================================================
// SMART HELP SUGGESTIONS
// =============================================================================

std::vector<SmartHelpSuggestions::Suggestion> SmartHelpSuggestions::GetSuggestions() {
    return GenerateSuggestions();
}

void SmartHelpSuggestions::TrackAction(const std::string& action) {
    m_recentActions.push_back(action);
    if (m_recentActions.size() > 50) {
        m_recentActions.erase(m_recentActions.begin());
    }
}

void SmartHelpSuggestions::TrackError(const std::string& error) {
    m_recentErrors.push_back(error);
    if (m_recentErrors.size() > 20) {
        m_recentErrors.erase(m_recentErrors.begin());
    }
}

void SmartHelpSuggestions::ProvideFeedback(const Suggestion& suggestion, bool helpful) {
    // Adjust weights based on feedback
    if (helpful) {
        m_suggestionWeights[suggestion.helpTopic] += 0.1f;
    } else {
        m_suggestionWeights[suggestion.helpTopic] -= 0.05f;
    }
}

float SmartHelpSuggestions::CalculateRelevance(const std::string& topic) {
    float relevance = 0.5f; // Base relevance
    
    // Check if topic relates to recent errors
    for (const auto& error : m_recentErrors) {
        if (error.find(topic) != std::string::npos) {
            relevance += 0.3f;
        }
    }
    
    // Check if topic relates to recent actions
    for (const auto& action : m_recentActions) {
        if (action.find(topic) != std::string::npos) {
            relevance += 0.1f;
        }
    }
    
    // Apply learned weights
    auto it = m_suggestionWeights.find(topic);
    if (it != m_suggestionWeights.end()) {
        relevance *= (1.0f + it->second);
    }
    
    return std::min(1.0f, relevance);
}

std::vector<SmartHelpSuggestions::Suggestion> SmartHelpSuggestions::GenerateSuggestions() {
    std::vector<Suggestion> suggestions;
    
    // Analyze recent errors for common patterns
    std::unordered_map<std::string, int> errorPatterns;
    for (const auto& error : m_recentErrors) {
        // Simple pattern extraction (would be more sophisticated in production)
        if (error.find("shader") != std::string::npos) {
            errorPatterns["shader"]++;
        }
        if (error.find("memory") != std::string::npos) {
            errorPatterns["memory"]++;
        }
        if (error.find("network") != std::string::npos) {
            errorPatterns["network"]++;
        }
    }
    
    // Generate suggestions based on patterns
    for (const auto& [pattern, count] : errorPatterns) {
        if (count > 2) {
            Suggestion suggestion;
            suggestion.title = "Troubleshoot " + pattern + " issues";
            suggestion.description = "You've encountered multiple " + pattern + 
                                    " errors. This guide can help resolve them.";
            suggestion.helpTopic = "troubleshoot_" + pattern;
            suggestion.tutorialId = "tutorial_fix_" + pattern;
            suggestion.relevance = CalculateRelevance(pattern);
            suggestions.push_back(suggestion);
        }
    }
    
    // Sort by relevance
    std::sort(suggestions.begin(), suggestions.end(),
              [](const auto& a, const auto& b) { return a.relevance > b.relevance; });
    
    // Limit to top 5 suggestions
    if (suggestions.size() > 5) {
        suggestions.resize(5);
    }
    
    return suggestions;
}

// =============================================================================
// INTERACTIVE ONBOARDING
// =============================================================================

void InteractiveOnboarding::Start() {
    m_active = true;
    m_currentStep = OnboardingStep::Welcome;
}

void InteractiveOnboarding::NextStep() {
    switch (m_currentStep) {
        case OnboardingStep::Welcome:
            m_currentStep = OnboardingStep::ProfileSetup;
            break;
        case OnboardingStep::ProfileSetup:
            m_currentStep = OnboardingStep::InterfaceTour;
            break;
        case OnboardingStep::InterfaceTour:
            m_currentStep = OnboardingStep::FirstProject;
            break;
        case OnboardingStep::FirstProject:
            m_currentStep = OnboardingStep::BasicConcepts;
            break;
        case OnboardingStep::BasicConcepts:
            m_currentStep = OnboardingStep::CustomizeWorkspace;
            break;
        case OnboardingStep::CustomizeWorkspace:
            m_currentStep = OnboardingStep::Complete;
            break;
        case OnboardingStep::Complete:
            m_active = false;
            break;
    }
}

void InteractiveOnboarding::Skip() {
    m_active = false;
}

void InteractiveOnboarding::RenderWelcome() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Welcome to ECScope Game Engine!");
    ImGui::Separator();
    ImGui::TextWrapped("Let's get you started with a quick setup process that will "
                      "personalize your experience and introduce you to the main features.");
    
    if (ImGui::Button("Get Started")) {
        NextStep();
    }
    ImGui::SameLine();
    if (ImGui::Button("Skip Setup")) {
        Skip();
    }
#endif
}

void InteractiveOnboarding::RenderProfileSetup() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Tell us about yourself");
    ImGui::Separator();
    
    const char* levels[] = { "Beginner", "Intermediate", "Advanced", "Expert" };
    int currentLevel = static_cast<int>(m_selectedLevel);
    if (ImGui::Combo("Experience Level", &currentLevel, levels, IM_ARRAYSIZE(levels))) {
        m_selectedLevel = static_cast<UserLevel>(currentLevel);
    }
    
    ImGui::Checkbox("I have experience with other game engines", &m_hasExperience);
    
    ImGui::Text("What are you interested in?");
    static bool interestedInProgramming = false;
    static bool interestedInArt = false;
    static bool interestedInDesign = false;
    static bool interestedInAudio = false;
    
    ImGui::Checkbox("Programming", &interestedInProgramming);
    ImGui::Checkbox("Art & Graphics", &interestedInArt);
    ImGui::Checkbox("Game Design", &interestedInDesign);
    ImGui::Checkbox("Audio", &interestedInAudio);
    
    if (ImGui::Button("Continue")) {
        // Save preferences
        if (interestedInProgramming) m_interests.push_back("programming");
        if (interestedInArt) m_interests.push_back("art");
        if (interestedInDesign) m_interests.push_back("design");
        if (interestedInAudio) m_interests.push_back("audio");
        
        HelpSystem::Get().SetUserLevel(m_selectedLevel);
        NextStep();
    }
#endif
}

void InteractiveOnboarding::RenderInterfaceTour() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Interface Overview");
    ImGui::Separator();
    ImGui::TextWrapped("Let's take a quick tour of the main interface elements.");
    
    // Highlight different UI areas
    static int tourStep = 0;
    const char* tourDescriptions[] = {
        "This is the main menu bar where you access all major functions.",
        "The dashboard provides quick access to common tasks and project overview.",
        "The viewport is where you see and interact with your game world.",
        "The inspector shows properties of selected objects.",
        "The asset browser manages all your project files and resources."
    };
    
    if (tourStep < IM_ARRAYSIZE(tourDescriptions)) {
        ImGui::TextWrapped("%s", tourDescriptions[tourStep]);
        
        if (ImGui::Button("Next")) {
            tourStep++;
            if (tourStep >= IM_ARRAYSIZE(tourDescriptions)) {
                NextStep();
                tourStep = 0;
            }
        }
    }
#endif
}

void InteractiveOnboarding::RenderFirstProject() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Create Your First Project");
    ImGui::Separator();
    ImGui::TextWrapped("Let's create a simple project to get you started.");
    
    static char projectName[256] = "MyFirstGame";
    ImGui::InputText("Project Name", projectName, sizeof(projectName));
    
    static int templateIndex = 0;
    const char* templates[] = { "Empty", "2D Platformer", "3D First Person", "Top-Down RPG" };
    ImGui::Combo("Template", &templateIndex, templates, IM_ARRAYSIZE(templates));
    
    if (ImGui::Button("Create Project")) {
        // Create project
        NextStep();
    }
#endif
}

void InteractiveOnboarding::RenderBasicConcepts() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Basic Concepts");
    ImGui::Separator();
    
    static int conceptIndex = 0;
    const char* concepts[] = {
        "Entities are the basic building blocks of your game world.",
        "Components define the properties and behaviors of entities.",
        "Systems process entities with specific component combinations.",
        "The ECS architecture provides flexibility and performance."
    };
    
    if (conceptIndex < IM_ARRAYSIZE(concepts)) {
        ImGui::TextWrapped("%s", concepts[conceptIndex]);
        
        if (ImGui::Button("Next Concept")) {
            conceptIndex++;
            if (conceptIndex >= IM_ARRAYSIZE(concepts)) {
                NextStep();
                conceptIndex = 0;
            }
        }
    }
#endif
}

void InteractiveOnboarding::RenderCustomization() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Customize Your Workspace");
    ImGui::Separator();
    ImGui::TextWrapped("You can customize the interface to match your workflow.");
    
    static int themeIndex = 0;
    const char* themes[] = { "Dark", "Light", "Blue", "Custom" };
    ImGui::Combo("Color Theme", &themeIndex, themes, IM_ARRAYSIZE(themes));
    
    static float uiScale = 1.0f;
    ImGui::SliderFloat("UI Scale", &uiScale, 0.5f, 2.0f);
    
    ImGui::Text("Dock windows by dragging their title bars.");
    ImGui::Text("Right-click title bars for more options.");
    
    if (ImGui::Button("Finish Setup")) {
        NextStep();
    }
#endif
}

void InteractiveOnboarding::RenderCompletion() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Setup Complete!");
    ImGui::Separator();
    ImGui::TextWrapped("You're all set! Here are some recommended next steps:");
    
    ImGui::BulletText("Complete the beginner tutorials");
    ImGui::BulletText("Explore the example projects");
    ImGui::BulletText("Check out the documentation");
    ImGui::BulletText("Join the community forums");
    
    if (ImGui::Button("Start Using ECScope")) {
        m_active = false;
    }
#endif
}

// Additional help system implementations would continue here...

} // namespace ecscope::gui::help