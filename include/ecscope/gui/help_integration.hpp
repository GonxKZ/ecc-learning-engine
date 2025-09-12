/**
 * @file help_integration.hpp
 * @brief Integration layer between help system and ECScope UI components
 * 
 * This module provides seamless integration of the help system with all
 * existing ECScope UI components, enabling context-sensitive help throughout
 * the engine interface.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "help_system.hpp"
#include "dashboard.hpp"
#include "ecs_inspector.hpp"
#include "rendering_ui.hpp"
#include "audio_ui.hpp"
#include "network_ui.hpp"
#include "asset_pipeline_ui.hpp"
#include "debug_tools_ui.hpp"
#include "plugin_manager_ui.hpp"
#include "scripting_ui.hpp"
#include "responsive_design.hpp"

#include <functional>
#include <string>
#include <vector>

namespace ecscope::gui::help {

// =============================================================================
// HELP INTEGRATION MANAGER
// =============================================================================

/**
 * @brief Manages integration of help system with UI components
 */
class HelpIntegration {
public:
    static HelpIntegration& Get();
    
    /**
     * @brief Initialize help integration for all UI systems
     */
    void Initialize();
    
    /**
     * @brief Register help contexts for dashboard components
     */
    void RegisterDashboardHelp();
    
    /**
     * @brief Register help contexts for ECS inspector
     */
    void RegisterECSInspectorHelp();
    
    /**
     * @brief Register help contexts for rendering UI
     */
    void RegisterRenderingHelp();
    
    /**
     * @brief Register help contexts for physics UI
     */
    void RegisterPhysicsHelp();
    
    /**
     * @brief Register help contexts for audio UI
     */
    void RegisterAudioHelp();
    
    /**
     * @brief Register help contexts for networking UI
     */
    void RegisterNetworkingHelp();
    
    /**
     * @brief Register help contexts for asset pipeline
     */
    void RegisterAssetPipelineHelp();
    
    /**
     * @brief Register help contexts for debug tools
     */
    void RegisterDebugToolsHelp();
    
    /**
     * @brief Register help contexts for plugin manager
     */
    void RegisterPluginManagerHelp();
    
    /**
     * @brief Register help contexts for scripting environment
     */
    void RegisterScriptingHelp();
    
    /**
     * @brief Register help contexts for responsive design
     */
    void RegisterResponsiveDesignHelp();
    
    /**
     * @brief Add help button to any ImGui window
     */
    void AddHelpButton(const std::string& helpTopic);
    
    /**
     * @brief Add context menu help option
     */
    void AddContextMenuHelp(const std::string& helpTopic);
    
    /**
     * @brief Show inline help hint
     */
    void ShowInlineHelp(const std::string& text);
    
    /**
     * @brief Check if user needs assistance (stuck detection)
     */
    void MonitorUserActivity();
    
private:
    HelpIntegration() = default;
    
    // Activity tracking for proactive help
    struct ActivityTracker {
        std::string currentView;
        float timeInView = 0.0f;
        int errorCount = 0;
        int undoCount = 0;
        float idleTime = 0.0f;
        std::vector<std::string> recentActions;
    };
    
    ActivityTracker m_tracker;
    bool m_proactiveHelpEnabled = true;
    float m_helpSuggestionThreshold = 30.0f; // seconds
};

// =============================================================================
// UI HELP WIDGETS
// =============================================================================

/**
 * @brief Help icon widget with tooltip
 */
void HelpIcon(const std::string& helpText, const std::string& helpTopic = "");

/**
 * @brief Question mark button that opens help
 */
bool HelpButton(const std::string& helpTopic);

/**
 * @brief Inline help text with formatting
 */
void HelpText(const std::string& text);

/**
 * @brief Collapsible help section
 */
void HelpSection(const std::string& title, const std::string& content);

/**
 * @brief Tutorial launcher button
 */
bool TutorialButton(const std::string& tutorialId, const std::string& label = "Start Tutorial");

/**
 * @brief Progress indicator for active tutorial
 */
void TutorialProgress();

/**
 * @brief Hotkey reminder widget
 */
void HotkeyReminder(const std::string& action, const std::string& keys);

/**
 * @brief First-time user hint
 */
void FirstTimeHint(const std::string& id, const std::string& hint);

// =============================================================================
// SMART HELP SUGGESTIONS
// =============================================================================

/**
 * @brief AI-powered help suggestion system
 */
class SmartHelpSuggestions {
public:
    struct Suggestion {
        std::string title;
        std::string description;
        std::string helpTopic;
        std::string tutorialId;
        float relevance;
    };
    
    /**
     * @brief Analyze user behavior and suggest help
     */
    std::vector<Suggestion> GetSuggestions();
    
    /**
     * @brief Track user action for pattern analysis
     */
    void TrackAction(const std::string& action);
    
    /**
     * @brief Track error for help suggestions
     */
    void TrackError(const std::string& error);
    
    /**
     * @brief Learn from user feedback
     */
    void ProvideFeedback(const Suggestion& suggestion, bool helpful);
    
private:
    std::vector<std::string> m_recentActions;
    std::vector<std::string> m_recentErrors;
    std::unordered_map<std::string, float> m_suggestionWeights;
    
    float CalculateRelevance(const std::string& topic);
    std::vector<Suggestion> GenerateSuggestions();
};

// =============================================================================
// INTERACTIVE ONBOARDING
// =============================================================================

/**
 * @brief Interactive onboarding flow for new users
 */
class InteractiveOnboarding {
public:
    enum class OnboardingStep {
        Welcome,
        ProfileSetup,
        InterfaceTour,
        FirstProject,
        BasicConcepts,
        CustomizeWorkspace,
        Complete
    };
    
    void Start();
    void NextStep();
    void Skip();
    
    void RenderWelcome();
    void RenderProfileSetup();
    void RenderInterfaceTour();
    void RenderFirstProject();
    void RenderBasicConcepts();
    void RenderCustomization();
    void RenderCompletion();
    
    bool IsActive() const { return m_active; }
    OnboardingStep GetCurrentStep() const { return m_currentStep; }
    
private:
    bool m_active = false;
    OnboardingStep m_currentStep = OnboardingStep::Welcome;
    
    // User preferences collected during onboarding
    UserLevel m_selectedLevel = UserLevel::Beginner;
    std::vector<std::string> m_interests;
    std::string m_primaryUseCase;
    bool m_hasExperience = false;
};

// =============================================================================
// DOCUMENTATION GENERATOR
// =============================================================================

/**
 * @brief Generates documentation from code and UI
 */
class DocumentationGenerator {
public:
    /**
     * @brief Generate markdown documentation for a component
     */
    std::string GenerateComponentDocs(const std::string& componentName);
    
    /**
     * @brief Generate API reference from code
     */
    std::string GenerateAPIReference(const std::string& className);
    
    /**
     * @brief Generate tutorial from recorded actions
     */
    std::string GenerateTutorialFromRecording(const std::vector<std::string>& actions);
    
    /**
     * @brief Export documentation to various formats
     */
    void ExportDocumentation(const std::string& format, const std::string& outputPath);
    
private:
    std::string FormatAsMarkdown(const std::string& content);
    std::string FormatAsHTML(const std::string& content);
    std::string FormatAsPDF(const std::string& content);
};

// =============================================================================
// MULTILANGUAGE SUPPORT
// =============================================================================

/**
 * @brief Manages help content in multiple languages
 */
class MultiLanguageHelp {
public:
    /**
     * @brief Set the current language
     */
    void SetLanguage(const std::string& langCode);
    
    /**
     * @brief Get translated help text
     */
    std::string GetTranslated(const std::string& key);
    
    /**
     * @brief Load language pack
     */
    void LoadLanguagePack(const std::string& path);
    
    /**
     * @brief Get available languages
     */
    std::vector<std::string> GetAvailableLanguages();
    
    /**
     * @brief Check if language is supported
     */
    bool IsLanguageSupported(const std::string& langCode);
    
private:
    std::string m_currentLanguage = "en";
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_translations;
};

// =============================================================================
// VIDEO TUTORIAL PLAYER
// =============================================================================

/**
 * @brief Embedded video tutorial player
 */
class VideoTutorialPlayer {
public:
    void LoadVideo(const std::string& videoPath);
    void Play();
    void Pause();
    void Stop();
    void Seek(float time);
    
    void SetSpeed(float speed);
    void SetVolume(float volume);
    
    void Render();
    
    bool IsPlaying() const { return m_playing; }
    float GetDuration() const { return m_duration; }
    float GetCurrentTime() const { return m_currentTime; }
    
private:
    std::string m_videoPath;
    bool m_playing = false;
    float m_duration = 0.0f;
    float m_currentTime = 0.0f;
    float m_playbackSpeed = 1.0f;
    float m_volume = 1.0f;
    
    // Video decoding would be handled by external library
    void* m_videoContext = nullptr;
};

// =============================================================================
// HELP METRICS AND ANALYTICS
// =============================================================================

/**
 * @brief Tracks help system usage and effectiveness
 */
class HelpMetrics {
public:
    struct Metric {
        std::string contentId;
        int views;
        float avgTimeSpent;
        float completionRate;
        float helpfulnessRating;
    };
    
    void TrackView(const std::string& contentId);
    void TrackCompletion(const std::string& contentId);
    void TrackRating(const std::string& contentId, float rating);
    
    std::vector<Metric> GetTopContent(int count = 10);
    std::vector<Metric> GetLeastEffective(int count = 10);
    
    void GenerateReport(const std::string& outputPath);
    
private:
    std::unordered_map<std::string, Metric> m_metrics;
    
    void UpdateMetric(const std::string& contentId, 
                      std::function<void(Metric&)> updater);
};

// =============================================================================
// COLLABORATIVE HELP
// =============================================================================

/**
 * @brief Community-driven help features
 */
class CollaborativeHelp {
public:
    struct UserContribution {
        std::string id;
        std::string author;
        std::string title;
        std::string content;
        int upvotes;
        int downvotes;
        std::vector<std::string> tags;
    };
    
    void SubmitContribution(const UserContribution& contribution);
    std::vector<UserContribution> GetContributions(const std::string& topic);
    void VoteContribution(const std::string& id, bool upvote);
    void ReportContribution(const std::string& id, const std::string& reason);
    
    void ConnectToCommunity();
    void SyncContributions();
    
private:
    std::vector<UserContribution> m_contributions;
    bool m_connected = false;
};

// =============================================================================
// ACCESSIBILITY FEATURES
// =============================================================================

/**
 * @brief Accessibility features for help system
 */
class AccessibilityHelp {
public:
    void EnableScreenReader();
    void SetFontSize(float size);
    void SetHighContrast(bool enabled);
    void EnableKeyboardNavigation();
    
    void ReadAloud(const std::string& text);
    void ShowSubtitles(bool show);
    
    std::string GetAccessibleDescription(const std::string& uiElement);
    
private:
    bool m_screenReaderEnabled = false;
    bool m_highContrastMode = false;
    float m_fontSize = 1.0f;
    bool m_keyboardNavEnabled = true;
    bool m_subtitlesEnabled = false;
};

} // namespace ecscope::gui::help