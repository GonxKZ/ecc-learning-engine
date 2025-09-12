/**
 * @file help_system.hpp
 * @brief ECScope Comprehensive Help System and Interactive Tutorials
 * 
 * Professional-grade help system providing context-sensitive help, interactive
 * tutorials, searchable documentation, and guided tours for the ECScope game
 * engine. Integrates seamlessly with Dear ImGui for a native experience.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <optional>
#include <queue>
#include <regex>
#include <variant>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

namespace ecscope::gui::help {

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

class HelpArticle;
class Tutorial;
class TutorialStep;
class HelpTooltip;
class GuidedTour;
class SearchEngine;
class VideoTutorial;

// =============================================================================
// ENUMERATIONS & TYPES
// =============================================================================

/**
 * @brief Help content types
 */
enum class HelpContentType {
    Article,        // Written documentation
    Tutorial,       // Interactive step-by-step guide
    Video,          // Video-like guided tour
    Tooltip,        // Quick contextual help
    FAQ,            // Frequently asked questions
    Troubleshoot,   // Problem-solving guide
    Reference       // API/feature reference
};

/**
 * @brief User proficiency levels for adaptive help
 */
enum class UserLevel {
    Beginner,       // New to game engines
    Intermediate,   // Familiar with basic concepts
    Advanced,       // Power user
    Expert          // Engine developer
};

/**
 * @brief Tutorial interaction types
 */
enum class TutorialAction {
    Click,          // Click on UI element
    Drag,           // Drag operation
    Type,           // Text input
    Hotkey,         // Keyboard shortcut
    Menu,           // Menu navigation
    Scroll,         // Scroll to view
    Wait,           // Wait for condition
    Custom          // Custom action
};

/**
 * @brief Help topic categories
 */
enum class HelpCategory {
    GettingStarted,
    Dashboard,
    ECS,
    Rendering,
    Physics,
    Audio,
    Networking,
    Assets,
    Debugging,
    Plugins,
    Scripting,
    Performance,
    Troubleshooting
};

// =============================================================================
// HELP ARTICLE CLASS
// =============================================================================

/**
 * @brief Represents a help article with rich content
 */
class HelpArticle {
public:
    struct Section {
        std::string title;
        std::string content;
        std::vector<std::string> codeExamples;
        std::vector<std::string> images;
        std::vector<std::string> relatedLinks;
    };

    HelpArticle(const std::string& id, const std::string& title);
    
    void AddSection(const Section& section);
    void AddKeyword(const std::string& keyword);
    void SetCategory(HelpCategory category);
    void SetLevel(UserLevel level);
    
    void Render();
    bool MatchesSearch(const std::string& query) const;
    
    const std::string& GetId() const { return m_id; }
    const std::string& GetTitle() const { return m_title; }
    HelpCategory GetCategory() const { return m_category; }
    
private:
    std::string m_id;
    std::string m_title;
    std::string m_summary;
    HelpCategory m_category;
    UserLevel m_minLevel;
    std::vector<Section> m_sections;
    std::vector<std::string> m_keywords;
    std::chrono::system_clock::time_point m_lastUpdated;
    int m_viewCount = 0;
};

// =============================================================================
// TUTORIAL STEP CLASS
// =============================================================================

/**
 * @brief Single step in an interactive tutorial
 */
class TutorialStep {
public:
    using ValidationFunc = std::function<bool()>;
    using ActionFunc = std::function<void()>;
    
    TutorialStep(const std::string& instruction);
    
    void SetHighlight(const ImVec2& pos, const ImVec2& size);
    void SetArrow(const ImVec2& from, const ImVec2& to);
    void SetAction(TutorialAction action, const ActionFunc& func);
    void SetValidation(const ValidationFunc& func);
    void SetHint(const std::string& hint);
    void SetSkippable(bool skippable);
    
    bool Execute();
    bool Validate();
    void Render();
    void ShowHint();
    
private:
    std::string m_instruction;
    std::string m_hint;
    TutorialAction m_action;
    ActionFunc m_actionFunc;
    ValidationFunc m_validationFunc;
    
    // Visual guides
    std::optional<ImVec2> m_highlightPos;
    std::optional<ImVec2> m_highlightSize;
    std::optional<ImVec2> m_arrowFrom;
    std::optional<ImVec2> m_arrowTo;
    
    bool m_skippable = false;
    bool m_completed = false;
    float m_progress = 0.0f;
};

// =============================================================================
// TUTORIAL CLASS
// =============================================================================

/**
 * @brief Interactive tutorial with multiple steps
 */
class Tutorial {
public:
    Tutorial(const std::string& id, const std::string& name);
    
    void AddStep(std::unique_ptr<TutorialStep> step);
    void SetCategory(HelpCategory category);
    void SetEstimatedTime(int minutes);
    void SetPrerequisites(const std::vector<std::string>& prereqs);
    
    void Start();
    void Stop();
    void Pause();
    void Resume();
    void NextStep();
    void PreviousStep();
    void SkipStep();
    void Restart();
    
    void Update(float deltaTime);
    void Render();
    
    bool IsActive() const { return m_active; }
    bool IsCompleted() const { return m_completed; }
    float GetProgress() const;
    
private:
    std::string m_id;
    std::string m_name;
    std::string m_description;
    HelpCategory m_category;
    UserLevel m_targetLevel;
    
    std::vector<std::unique_ptr<TutorialStep>> m_steps;
    size_t m_currentStep = 0;
    
    bool m_active = false;
    bool m_paused = false;
    bool m_completed = false;
    
    int m_estimatedMinutes = 5;
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::duration<float> m_elapsedTime;
    
    std::vector<std::string> m_prerequisites;
    std::unordered_map<std::string, bool> m_achievements;
};

// =============================================================================
// GUIDED TOUR CLASS
// =============================================================================

/**
 * @brief Video-like guided tour with automatic progression
 */
class GuidedTour {
public:
    struct Waypoint {
        std::string narration;
        ImVec2 cameraTarget;
        float duration;
        std::function<void()> action;
        bool pauseForUser = false;
    };
    
    GuidedTour(const std::string& id, const std::string& title);
    
    void AddWaypoint(const Waypoint& waypoint);
    void SetAutoPlay(bool autoPlay);
    void SetSpeed(float speed);
    
    void Play();
    void Pause();
    void Stop();
    void NextWaypoint();
    void PreviousWaypoint();
    
    void Update(float deltaTime);
    void Render();
    
private:
    std::string m_id;
    std::string m_title;
    std::vector<Waypoint> m_waypoints;
    size_t m_currentWaypoint = 0;
    
    bool m_playing = false;
    bool m_autoPlay = true;
    float m_playbackSpeed = 1.0f;
    float m_waypointTimer = 0.0f;
    
    // Smooth camera movement
    ImVec2 m_currentCamera;
    ImVec2 m_targetCamera;
    float m_cameraLerp = 0.0f;
};

// =============================================================================
// CONTEXT HELP PROVIDER
// =============================================================================

/**
 * @brief Provides context-sensitive help based on current UI state
 */
class ContextHelpProvider {
public:
    struct HelpContext {
        std::string widgetId;
        std::string helpText;
        std::vector<std::string> relatedTopics;
        std::optional<std::string> tutorialId;
        UserLevel minLevel = UserLevel::Beginner;
    };
    
    void RegisterContext(const std::string& widgetId, const HelpContext& context);
    void ShowContextHelp(const std::string& widgetId);
    void ShowTooltip(const std::string& widgetId);
    void UpdateHoveredWidget();
    
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    
private:
    std::unordered_map<std::string, HelpContext> m_contexts;
    std::string m_currentWidget;
    bool m_enabled = true;
    float m_hoverTime = 0.0f;
    const float m_tooltipDelay = 0.5f;
};

// =============================================================================
// SEARCH ENGINE
// =============================================================================

/**
 * @brief Full-text search engine for help content
 */
class SearchEngine {
public:
    struct SearchResult {
        std::string id;
        std::string title;
        std::string snippet;
        HelpContentType type;
        float relevance;
        std::vector<std::string> highlights;
    };
    
    void IndexContent(const HelpArticle& article);
    void IndexTutorial(const Tutorial& tutorial);
    void BuildIndex();
    
    std::vector<SearchResult> Search(const std::string& query, 
                                     int maxResults = 10);
    std::vector<SearchResult> SearchByCategory(HelpCategory category);
    std::vector<std::string> GetSuggestions(const std::string& partial);
    
    void AddSynonym(const std::string& word, const std::string& synonym);
    void SetStopWords(const std::vector<std::string>& stopWords);
    
private:
    struct IndexEntry {
        std::string contentId;
        HelpContentType type;
        std::string title;
        std::string content;
        std::vector<std::string> keywords;
        HelpCategory category;
    };
    
    std::vector<IndexEntry> m_index;
    std::unordered_map<std::string, std::vector<std::string>> m_synonyms;
    std::unordered_set<std::string> m_stopWords;
    
    // Search optimization
    std::unordered_map<std::string, std::vector<size_t>> m_invertedIndex;
    std::vector<std::string> m_searchHistory;
    
    float CalculateRelevance(const IndexEntry& entry, 
                            const std::vector<std::string>& terms);
    std::vector<std::string> TokenizeQuery(const std::string& query);
};

// =============================================================================
// TROUBLESHOOTING SYSTEM
// =============================================================================

/**
 * @brief Interactive troubleshooting guide
 */
class TroubleshootingSystem {
public:
    struct Problem {
        std::string id;
        std::string description;
        std::vector<std::string> symptoms;
        std::vector<std::string> causes;
        std::vector<std::string> solutions;
        std::function<bool()> autoDetect;
    };
    
    struct DiagnosticStep {
        std::string question;
        std::vector<std::string> options;
        std::unordered_map<std::string, std::string> nextSteps;
    };
    
    void RegisterProblem(const Problem& problem);
    void AddDiagnosticFlow(const std::string& startId, 
                           const std::vector<DiagnosticStep>& steps);
    
    std::vector<Problem> DetectProblems();
    void StartDiagnostic(const std::string& problemId);
    void AnswerDiagnostic(const std::string& answer);
    
    void ShowTroubleshooter();
    
private:
    std::unordered_map<std::string, Problem> m_problems;
    std::unordered_map<std::string, std::vector<DiagnosticStep>> m_diagnosticFlows;
    
    std::optional<std::string> m_currentDiagnostic;
    size_t m_currentStep = 0;
    std::vector<std::pair<std::string, std::string>> m_diagnosticHistory;
};

// =============================================================================
// ONBOARDING FLOW
// =============================================================================

/**
 * @brief First-time user onboarding experience
 */
class OnboardingFlow {
public:
    enum class OnboardingStage {
        Welcome,
        ProfileSetup,
        InterfaceOverview,
        FirstProject,
        BasicTutorial,
        Customization,
        Complete
    };
    
    void Start();
    void Skip();
    void NextStage();
    void PreviousStage();
    
    void SetUserProfile(UserLevel level, 
                       const std::vector<std::string>& interests);
    void CustomizeLearningPath();
    
    void Update(float deltaTime);
    void Render();
    
    bool IsActive() const { return m_active; }
    OnboardingStage GetCurrentStage() const { return m_currentStage; }
    
private:
    bool m_active = false;
    OnboardingStage m_currentStage = OnboardingStage::Welcome;
    
    UserLevel m_userLevel = UserLevel::Beginner;
    std::vector<std::string> m_userInterests;
    std::vector<std::string> m_recommendedTutorials;
    
    float m_stageProgress = 0.0f;
    bool m_skipAvailable = true;
};

// =============================================================================
// HELP SYSTEM MANAGER
// =============================================================================

/**
 * @brief Central manager for the entire help system
 */
class HelpSystem {
public:
    static HelpSystem& Get();
    
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Render();
    
    // Content management
    void LoadHelpContent(const std::string& path);
    void RegisterArticle(std::unique_ptr<HelpArticle> article);
    void RegisterTutorial(std::unique_ptr<Tutorial> tutorial);
    void RegisterTour(std::unique_ptr<GuidedTour> tour);
    
    // User interaction
    void ShowHelp(const std::string& topicId = "");
    void ShowTutorialBrowser();
    void StartTutorial(const std::string& tutorialId);
    void ShowContextHelp();
    void ShowSearchDialog();
    void ShowTroubleshooter();
    
    // Quick access
    void ShowQuickHelp(const std::string& text);
    void ShowTooltip(const std::string& text);
    void ShowHotkeys();
    
    // Settings
    void SetUserLevel(UserLevel level);
    void SetLanguage(const std::string& langCode);
    void SetTooltipsEnabled(bool enabled);
    void SetTutorialSpeed(float speed);
    
    // Analytics
    void TrackHelpUsage(const std::string& contentId);
    std::vector<std::string> GetMostViewedTopics(int count = 10);
    std::vector<std::string> GetRecommendedContent();
    
    // Context help
    ContextHelpProvider& GetContextProvider() { return m_contextProvider; }
    SearchEngine& GetSearchEngine() { return m_searchEngine; }
    TroubleshootingSystem& GetTroubleshooter() { return m_troubleshooter; }
    OnboardingFlow& GetOnboarding() { return m_onboarding; }
    
private:
    HelpSystem() = default;
    
    // Content storage
    std::unordered_map<std::string, std::unique_ptr<HelpArticle>> m_articles;
    std::unordered_map<std::string, std::unique_ptr<Tutorial>> m_tutorials;
    std::unordered_map<std::string, std::unique_ptr<GuidedTour>> m_tours;
    
    // Active elements
    std::unique_ptr<Tutorial> m_activeTutorial;
    std::unique_ptr<GuidedTour> m_activeTour;
    
    // Subsystems
    ContextHelpProvider m_contextProvider;
    SearchEngine m_searchEngine;
    TroubleshootingSystem m_troubleshooter;
    OnboardingFlow m_onboarding;
    
    // UI State
    bool m_showHelpWindow = false;
    bool m_showSearchDialog = false;
    bool m_showTutorialBrowser = false;
    bool m_showTroubleshooter = false;
    std::string m_currentArticleId;
    
    // User preferences
    UserLevel m_userLevel = UserLevel::Beginner;
    std::string m_language = "en";
    bool m_tooltipsEnabled = true;
    float m_tutorialSpeed = 1.0f;
    
    // Analytics
    std::unordered_map<std::string, int> m_viewCounts;
    std::vector<std::string> m_viewHistory;
    
    // Rendering
    void RenderHelpWindow();
    void RenderSearchDialog();
    void RenderTutorialBrowser();
    void RenderQuickHelp();
    void RenderTooltips();
    void RenderHotkeys();
};

// =============================================================================
// HELP CONTENT FACTORY
// =============================================================================

/**
 * @brief Factory for creating help content
 */
class HelpContentFactory {
public:
    static std::unique_ptr<HelpArticle> CreateArticle(
        const std::string& id,
        const std::string& title,
        const std::string& markdownContent
    );
    
    static std::unique_ptr<Tutorial> CreateBasicTutorial(
        const std::string& id,
        const std::string& name,
        HelpCategory category
    );
    
    static std::unique_ptr<GuidedTour> CreateGuidedTour(
        const std::string& id,
        const std::string& title,
        const std::vector<GuidedTour::Waypoint>& waypoints
    );
    
    // Pre-built tutorials for common tasks
    static std::unique_ptr<Tutorial> CreateGettingStartedTutorial();
    static std::unique_ptr<Tutorial> CreateECSTutorial();
    static std::unique_ptr<Tutorial> CreateRenderingTutorial();
    static std::unique_ptr<Tutorial> CreatePhysicsTutorial();
    static std::unique_ptr<Tutorial> CreateAudioTutorial();
    static std::unique_ptr<Tutorial> CreateNetworkingTutorial();
    static std::unique_ptr<Tutorial> CreateAssetPipelineTutorial();
    static std::unique_ptr<Tutorial> CreateDebuggingTutorial();
    static std::unique_ptr<Tutorial> CreatePluginTutorial();
    static std::unique_ptr<Tutorial> CreateScriptingTutorial();
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Highlight a UI region for tutorials
 */
void HighlightRegion(const ImVec2& pos, const ImVec2& size, 
                     const ImVec4& color = ImVec4(1, 1, 0, 0.5f));

/**
 * @brief Draw an arrow pointing to UI element
 */
void DrawArrow(const ImVec2& from, const ImVec2& to, 
               const ImVec4& color = ImVec4(1, 1, 0, 1));

/**
 * @brief Show a non-blocking notification
 */
void ShowNotification(const std::string& text, float duration = 3.0f);

/**
 * @brief Check if user needs help (stuck detection)
 */
bool DetectUserNeedsHelp();

} // namespace ecscope::gui::help