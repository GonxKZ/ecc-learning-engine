/**
 * @file help_system.cpp
 * @brief Implementation of ECScope comprehensive help system
 */

#include "ecscope/gui/help_system.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace ecscope::gui::help {

// =============================================================================
// HELP ARTICLE IMPLEMENTATION
// =============================================================================

HelpArticle::HelpArticle(const std::string& id, const std::string& title)
    : m_id(id)
    , m_title(title)
    , m_category(HelpCategory::GettingStarted)
    , m_minLevel(UserLevel::Beginner)
    , m_lastUpdated(std::chrono::system_clock::now()) {
}

void HelpArticle::AddSection(const Section& section) {
    m_sections.push_back(section);
}

void HelpArticle::AddKeyword(const std::string& keyword) {
    m_keywords.push_back(keyword);
}

void HelpArticle::SetCategory(HelpCategory category) {
    m_category = category;
}

void HelpArticle::SetLevel(UserLevel level) {
    m_minLevel = level;
}

void HelpArticle::Render() {
#ifdef ECSCOPE_HAS_IMGUI
    // Header
    ImGui::Text("%s", m_title.c_str());
    ImGui::Separator();
    
    // Summary
    if (!m_summary.empty()) {
        ImGui::TextWrapped("%s", m_summary.c_str());
        ImGui::Spacing();
    }
    
    // Sections
    for (const auto& section : m_sections) {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Assuming header font
        ImGui::Text("%s", section.title.c_str());
        ImGui::PopFont();
        
        ImGui::TextWrapped("%s", section.content.c_str());
        
        // Code examples
        if (!section.codeExamples.empty()) {
            for (const auto& code : section.codeExamples) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                ImGui::BeginChild("code", ImVec2(0, ImGui::GetTextLineHeight() * 10), true);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]); // Monospace font
                ImGui::TextUnformatted(code.c_str());
                ImGui::PopFont();
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
        }
        
        // Related links
        if (!section.relatedLinks.empty()) {
            ImGui::Text("Related Topics:");
            for (const auto& link : section.relatedLinks) {
                if (ImGui::SmallButton(link.c_str())) {
                    HelpSystem::Get().ShowHelp(link);
                }
                ImGui::SameLine();
            }
            ImGui::NewLine();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
    
    // Footer with metadata
    ImGui::TextDisabled("Last updated: %s | Views: %d", 
                       "2024-01-01", m_viewCount); // Format date properly in production
#endif
}

bool HelpArticle::MatchesSearch(const std::string& query) const {
    // Convert to lowercase for case-insensitive search
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    // Check title
    std::string lowerTitle = m_title;
    std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);
    if (lowerTitle.find(lowerQuery) != std::string::npos) {
        return true;
    }
    
    // Check keywords
    for (const auto& keyword : m_keywords) {
        std::string lowerKeyword = keyword;
        std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
        if (lowerKeyword.find(lowerQuery) != std::string::npos) {
            return true;
        }
    }
    
    // Check content
    for (const auto& section : m_sections) {
        std::string lowerContent = section.content;
        std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
        if (lowerContent.find(lowerQuery) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

// =============================================================================
// TUTORIAL STEP IMPLEMENTATION
// =============================================================================

TutorialStep::TutorialStep(const std::string& instruction)
    : m_instruction(instruction)
    , m_action(TutorialAction::Click)
    , m_skippable(false)
    , m_completed(false)
    , m_progress(0.0f) {
}

void TutorialStep::SetHighlight(const ImVec2& pos, const ImVec2& size) {
    m_highlightPos = pos;
    m_highlightSize = size;
}

void TutorialStep::SetArrow(const ImVec2& from, const ImVec2& to) {
    m_arrowFrom = from;
    m_arrowTo = to;
}

void TutorialStep::SetAction(TutorialAction action, const ActionFunc& func) {
    m_action = action;
    m_actionFunc = func;
}

void TutorialStep::SetValidation(const ValidationFunc& func) {
    m_validationFunc = func;
}

void TutorialStep::SetHint(const std::string& hint) {
    m_hint = hint;
}

void TutorialStep::SetSkippable(bool skippable) {
    m_skippable = skippable;
}

bool TutorialStep::Execute() {
    if (m_actionFunc) {
        m_actionFunc();
        return true;
    }
    return false;
}

bool TutorialStep::Validate() {
    if (m_validationFunc) {
        m_completed = m_validationFunc();
        if (m_completed) {
            m_progress = 1.0f;
        }
        return m_completed;
    }
    m_completed = true;
    m_progress = 1.0f;
    return true;
}

void TutorialStep::Render() {
#ifdef ECSCOPE_HAS_IMGUI
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    // Draw highlight if specified
    if (m_highlightPos && m_highlightSize) {
        ImVec2 min = m_highlightPos.value();
        ImVec2 max = ImVec2(min.x + m_highlightSize.value().x, 
                            min.y + m_highlightSize.value().y);
        
        // Darken everything except highlighted area
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        drawList->AddRectFilled(ImVec2(0, 0), displaySize, 
                                IM_COL32(0, 0, 0, 128));
        drawList->AddRectFilled(min, max, IM_COL32(0, 0, 0, 0));
        
        // Highlight border
        drawList->AddRect(min, max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f);
        
        // Pulsing effect
        float pulse = (sinf(ImGui::GetTime() * 3.0f) + 1.0f) * 0.5f;
        drawList->AddRect(min, max, IM_COL32(255, 255, 0, (int)(128 * pulse)), 
                         0.0f, 0, 6.0f);
    }
    
    // Draw arrow if specified
    if (m_arrowFrom && m_arrowTo) {
        DrawArrow(m_arrowFrom.value(), m_arrowTo.value());
    }
    
    // Draw instruction box
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 100), 
                           ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.95f));
    
    ImGui::Begin("Tutorial", nullptr, 
                ImGuiWindowFlags_NoTitleBar | 
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove);
    
    // Instruction text
    ImGui::TextWrapped("%s", m_instruction.c_str());
    
    // Progress bar
    ImGui::ProgressBar(m_progress, ImVec2(-1, 0));
    
    // Action buttons
    if (m_skippable) {
        if (ImGui::Button("Skip")) {
            m_completed = true;
            m_progress = 1.0f;
        }
        ImGui::SameLine();
    }
    
    if (!m_hint.empty()) {
        if (ImGui::Button("Show Hint")) {
            ShowHint();
        }
    }
    
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
#endif
}

void TutorialStep::ShowHint() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::OpenPopup("Hint");
    if (ImGui::BeginPopupModal("Hint", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", m_hint.c_str());
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
#endif
}

// =============================================================================
// TUTORIAL IMPLEMENTATION
// =============================================================================

Tutorial::Tutorial(const std::string& id, const std::string& name)
    : m_id(id)
    , m_name(name)
    , m_category(HelpCategory::GettingStarted)
    , m_targetLevel(UserLevel::Beginner)
    , m_currentStep(0)
    , m_active(false)
    , m_paused(false)
    , m_completed(false)
    , m_estimatedMinutes(5) {
}

void Tutorial::AddStep(std::unique_ptr<TutorialStep> step) {
    m_steps.push_back(std::move(step));
}

void Tutorial::SetCategory(HelpCategory category) {
    m_category = category;
}

void Tutorial::SetEstimatedTime(int minutes) {
    m_estimatedMinutes = minutes;
}

void Tutorial::SetPrerequisites(const std::vector<std::string>& prereqs) {
    m_prerequisites = prereqs;
}

void Tutorial::Start() {
    m_active = true;
    m_paused = false;
    m_completed = false;
    m_currentStep = 0;
    m_startTime = std::chrono::steady_clock::now();
    m_elapsedTime = std::chrono::duration<float>::zero();
}

void Tutorial::Stop() {
    m_active = false;
    m_paused = false;
}

void Tutorial::Pause() {
    m_paused = true;
}

void Tutorial::Resume() {
    m_paused = false;
}

void Tutorial::NextStep() {
    if (m_currentStep < m_steps.size() - 1) {
        if (m_steps[m_currentStep]->Validate()) {
            m_currentStep++;
        }
    } else {
        m_completed = true;
        m_active = false;
    }
}

void Tutorial::PreviousStep() {
    if (m_currentStep > 0) {
        m_currentStep--;
    }
}

void Tutorial::SkipStep() {
    if (m_currentStep < m_steps.size() - 1) {
        m_currentStep++;
    }
}

void Tutorial::Restart() {
    m_currentStep = 0;
    m_completed = false;
    Start();
}

void Tutorial::Update(float deltaTime) {
    if (!m_active || m_paused) return;
    
    m_elapsedTime += std::chrono::duration<float>(deltaTime);
    
    // Check if current step is completed
    if (m_currentStep < m_steps.size()) {
        if (m_steps[m_currentStep]->Validate()) {
            NextStep();
        }
    }
}

void Tutorial::Render() {
    if (!m_active) return;
    
#ifdef ECSCOPE_HAS_IMGUI
    // Render current step
    if (m_currentStep < m_steps.size()) {
        m_steps[m_currentStep]->Render();
    }
    
    // Tutorial control panel
    ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetIO().DisplaySize.y - 100), 
                           ImGuiCond_Always);
    ImGui::Begin("Tutorial Controls", nullptr, 
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove);
    
    ImGui::Text("Tutorial: %s", m_name.c_str());
    ImGui::Text("Step %zu of %zu", m_currentStep + 1, m_steps.size());
    
    // Progress bar
    float progress = GetProgress();
    ImGui::ProgressBar(progress, ImVec2(200, 0));
    
    // Control buttons
    if (ImGui::Button(m_paused ? "Resume" : "Pause")) {
        m_paused ? Resume() : Pause();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Stop")) {
        Stop();
    }
    
    ImGui::End();
#endif
}

float Tutorial::GetProgress() const {
    if (m_steps.empty()) return 0.0f;
    return static_cast<float>(m_currentStep) / static_cast<float>(m_steps.size());
}

// =============================================================================
// GUIDED TOUR IMPLEMENTATION
// =============================================================================

GuidedTour::GuidedTour(const std::string& id, const std::string& title)
    : m_id(id)
    , m_title(title)
    , m_currentWaypoint(0)
    , m_playing(false)
    , m_autoPlay(true)
    , m_playbackSpeed(1.0f)
    , m_waypointTimer(0.0f)
    , m_currentCamera(ImVec2(0, 0))
    , m_targetCamera(ImVec2(0, 0))
    , m_cameraLerp(0.0f) {
}

void GuidedTour::AddWaypoint(const Waypoint& waypoint) {
    m_waypoints.push_back(waypoint);
}

void GuidedTour::SetAutoPlay(bool autoPlay) {
    m_autoPlay = autoPlay;
}

void GuidedTour::SetSpeed(float speed) {
    m_playbackSpeed = std::max(0.1f, std::min(3.0f, speed));
}

void GuidedTour::Play() {
    m_playing = true;
    m_currentWaypoint = 0;
    m_waypointTimer = 0.0f;
    if (!m_waypoints.empty()) {
        m_targetCamera = m_waypoints[0].cameraTarget;
    }
}

void GuidedTour::Pause() {
    m_playing = false;
}

void GuidedTour::Stop() {
    m_playing = false;
    m_currentWaypoint = 0;
    m_waypointTimer = 0.0f;
}

void GuidedTour::NextWaypoint() {
    if (m_currentWaypoint < m_waypoints.size() - 1) {
        m_currentWaypoint++;
        m_waypointTimer = 0.0f;
        m_targetCamera = m_waypoints[m_currentWaypoint].cameraTarget;
        m_cameraLerp = 0.0f;
    } else {
        Stop();
    }
}

void GuidedTour::PreviousWaypoint() {
    if (m_currentWaypoint > 0) {
        m_currentWaypoint--;
        m_waypointTimer = 0.0f;
        m_targetCamera = m_waypoints[m_currentWaypoint].cameraTarget;
        m_cameraLerp = 0.0f;
    }
}

void GuidedTour::Update(float deltaTime) {
    if (!m_playing || m_waypoints.empty()) return;
    
    const auto& waypoint = m_waypoints[m_currentWaypoint];
    
    // Update camera smoothly
    m_cameraLerp = std::min(1.0f, m_cameraLerp + deltaTime * 2.0f);
    m_currentCamera.x = m_currentCamera.x + (m_targetCamera.x - m_currentCamera.x) * m_cameraLerp;
    m_currentCamera.y = m_currentCamera.y + (m_targetCamera.y - m_currentCamera.y) * m_cameraLerp;
    
    // Execute waypoint action
    if (waypoint.action && m_waypointTimer == 0.0f) {
        waypoint.action();
    }
    
    // Update timer
    if (!waypoint.pauseForUser) {
        m_waypointTimer += deltaTime * m_playbackSpeed;
        
        if (m_waypointTimer >= waypoint.duration && m_autoPlay) {
            NextWaypoint();
        }
    }
}

void GuidedTour::Render() {
    if (!m_playing || m_waypoints.empty()) return;
    
#ifdef ECSCOPE_HAS_IMGUI
    const auto& waypoint = m_waypoints[m_currentWaypoint];
    
    // Narration overlay
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 
                                   ImGui::GetIO().DisplaySize.y - 150), 
                           ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
    
    ImGui::Begin("Tour Narration", nullptr, 
                ImGuiWindowFlags_NoTitleBar | 
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove);
    
    ImGui::TextWrapped("%s", waypoint.narration.c_str());
    
    // Progress indicator
    float progress = (m_currentWaypoint + 1.0f) / m_waypoints.size();
    ImGui::ProgressBar(progress, ImVec2(300, 0));
    
    // Controls
    if (waypoint.pauseForUser) {
        if (ImGui::Button("Continue")) {
            NextWaypoint();
        }
    }
    
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    
    // Video-style controls
    ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetIO().DisplaySize.y - 50), 
                           ImGuiCond_Always);
    ImGui::Begin("Tour Controls", nullptr, 
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoTitleBar);
    
    if (ImGui::Button(m_playing ? "||" : ">")) {
        m_playing ? Pause() : Play();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("<<")) {
        PreviousWaypoint();
    }
    ImGui::SameLine();
    
    if (ImGui::Button(">>")) {
        NextWaypoint();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Stop")) {
        Stop();
    }
    ImGui::SameLine();
    
    ImGui::Text("Speed:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##Speed", &m_playbackSpeed, 0.5f, 2.0f);
    
    ImGui::End();
#endif
}

// =============================================================================
// CONTEXT HELP PROVIDER IMPLEMENTATION
// =============================================================================

void ContextHelpProvider::RegisterContext(const std::string& widgetId, 
                                         const HelpContext& context) {
    m_contexts[widgetId] = context;
}

void ContextHelpProvider::ShowContextHelp(const std::string& widgetId) {
    auto it = m_contexts.find(widgetId);
    if (it == m_contexts.end()) return;
    
#ifdef ECSCOPE_HAS_IMGUI
    const auto& context = it->second;
    
    ImGui::OpenPopup("Context Help");
    if (ImGui::BeginPopupModal("Context Help", nullptr, 
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", context.helpText.c_str());
        
        if (!context.relatedTopics.empty()) {
            ImGui::Separator();
            ImGui::Text("Related Topics:");
            for (const auto& topic : context.relatedTopics) {
                if (ImGui::SmallButton(topic.c_str())) {
                    HelpSystem::Get().ShowHelp(topic);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        if (context.tutorialId) {
            ImGui::Separator();
            if (ImGui::Button("Start Tutorial")) {
                HelpSystem::Get().StartTutorial(context.tutorialId.value());
                ImGui::CloseCurrentPopup();
            }
        }
        
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
#endif
}

void ContextHelpProvider::ShowTooltip(const std::string& widgetId) {
    if (!m_enabled) return;
    
    auto it = m_contexts.find(widgetId);
    if (it == m_contexts.end()) return;
    
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::IsItemHovered()) {
        m_hoverTime += ImGui::GetIO().DeltaTime;
        
        if (m_hoverTime >= m_tooltipDelay) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(it->second.helpText.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    } else {
        m_hoverTime = 0.0f;
    }
#endif
}

void ContextHelpProvider::UpdateHoveredWidget() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::IsAnyItemHovered()) {
        ImGuiID id = ImGui::GetHoveredID();
        m_currentWidget = std::to_string(id);
    } else {
        m_currentWidget.clear();
        m_hoverTime = 0.0f;
    }
#endif
}

// =============================================================================
// SEARCH ENGINE IMPLEMENTATION
// =============================================================================

void SearchEngine::IndexContent(const HelpArticle& article) {
    IndexEntry entry;
    entry.contentId = article.GetId();
    entry.type = HelpContentType::Article;
    entry.title = article.GetTitle();
    entry.category = article.GetCategory();
    // Note: Would need to expose article content for full indexing
    m_index.push_back(entry);
}

void SearchEngine::IndexTutorial(const Tutorial& tutorial) {
    IndexEntry entry;
    entry.contentId = tutorial.m_id;
    entry.type = HelpContentType::Tutorial;
    entry.title = tutorial.m_name;
    entry.category = tutorial.m_category;
    m_index.push_back(entry);
}

void SearchEngine::BuildIndex() {
    // Build inverted index for faster searching
    m_invertedIndex.clear();
    
    for (size_t i = 0; i < m_index.size(); ++i) {
        const auto& entry = m_index[i];
        
        // Tokenize title
        std::istringstream iss(entry.title);
        std::string token;
        while (iss >> token) {
            // Convert to lowercase
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            m_invertedIndex[token].push_back(i);
        }
        
        // Tokenize keywords
        for (const auto& keyword : entry.keywords) {
            std::string lowerKeyword = keyword;
            std::transform(lowerKeyword.begin(), lowerKeyword.end(), 
                          lowerKeyword.begin(), ::tolower);
            m_invertedIndex[lowerKeyword].push_back(i);
        }
    }
}

std::vector<SearchEngine::SearchResult> SearchEngine::Search(
    const std::string& query, int maxResults) {
    
    std::vector<SearchResult> results;
    auto terms = TokenizeQuery(query);
    
    // Score each indexed item
    std::vector<std::pair<float, size_t>> scores;
    for (size_t i = 0; i < m_index.size(); ++i) {
        float relevance = CalculateRelevance(m_index[i], terms);
        if (relevance > 0.0f) {
            scores.push_back({relevance, i});
        }
    }
    
    // Sort by relevance
    std::sort(scores.begin(), scores.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Build results
    for (size_t i = 0; i < std::min(static_cast<size_t>(maxResults), scores.size()); ++i) {
        const auto& entry = m_index[scores[i].second];
        
        SearchResult result;
        result.id = entry.contentId;
        result.title = entry.title;
        result.type = entry.type;
        result.relevance = scores[i].first;
        result.snippet = entry.content.substr(0, 200) + "...";
        
        // Highlight matching terms
        for (const auto& term : terms) {
            size_t pos = 0;
            std::string lowerContent = entry.content;
            std::transform(lowerContent.begin(), lowerContent.end(), 
                          lowerContent.begin(), ::tolower);
            
            while ((pos = lowerContent.find(term, pos)) != std::string::npos) {
                result.highlights.push_back(entry.content.substr(pos, term.length()));
                pos += term.length();
            }
        }
        
        results.push_back(result);
    }
    
    // Update search history
    m_searchHistory.push_back(query);
    if (m_searchHistory.size() > 100) {
        m_searchHistory.erase(m_searchHistory.begin());
    }
    
    return results;
}

std::vector<SearchEngine::SearchResult> SearchEngine::SearchByCategory(
    HelpCategory category) {
    
    std::vector<SearchResult> results;
    
    for (const auto& entry : m_index) {
        if (entry.category == category) {
            SearchResult result;
            result.id = entry.contentId;
            result.title = entry.title;
            result.type = entry.type;
            result.relevance = 1.0f;
            results.push_back(result);
        }
    }
    
    return results;
}

std::vector<std::string> SearchEngine::GetSuggestions(const std::string& partial) {
    std::vector<std::string> suggestions;
    
    std::string lowerPartial = partial;
    std::transform(lowerPartial.begin(), lowerPartial.end(), 
                  lowerPartial.begin(), ::tolower);
    
    // Search in previous queries
    for (const auto& query : m_searchHistory) {
        if (query.find(lowerPartial) == 0) {
            suggestions.push_back(query);
        }
    }
    
    // Search in index
    for (const auto& [term, indices] : m_invertedIndex) {
        if (term.find(lowerPartial) == 0) {
            suggestions.push_back(term);
        }
    }
    
    // Remove duplicates
    std::sort(suggestions.begin(), suggestions.end());
    suggestions.erase(std::unique(suggestions.begin(), suggestions.end()), 
                     suggestions.end());
    
    // Limit suggestions
    if (suggestions.size() > 10) {
        suggestions.resize(10);
    }
    
    return suggestions;
}

void SearchEngine::AddSynonym(const std::string& word, const std::string& synonym) {
    m_synonyms[word].push_back(synonym);
    m_synonyms[synonym].push_back(word);
}

void SearchEngine::SetStopWords(const std::vector<std::string>& stopWords) {
    m_stopWords.clear();
    for (const auto& word : stopWords) {
        m_stopWords.insert(word);
    }
}

float SearchEngine::CalculateRelevance(const IndexEntry& entry, 
                                       const std::vector<std::string>& terms) {
    float relevance = 0.0f;
    
    for (const auto& term : terms) {
        // Skip stop words
        if (m_stopWords.count(term) > 0) continue;
        
        // Check title (higher weight)
        std::string lowerTitle = entry.title;
        std::transform(lowerTitle.begin(), lowerTitle.end(), 
                      lowerTitle.begin(), ::tolower);
        
        if (lowerTitle.find(term) != std::string::npos) {
            relevance += 2.0f;
        }
        
        // Check keywords (medium weight)
        for (const auto& keyword : entry.keywords) {
            std::string lowerKeyword = keyword;
            std::transform(lowerKeyword.begin(), lowerKeyword.end(), 
                          lowerKeyword.begin(), ::tolower);
            
            if (lowerKeyword.find(term) != std::string::npos) {
                relevance += 1.5f;
            }
        }
        
        // Check content (lower weight)
        std::string lowerContent = entry.content;
        std::transform(lowerContent.begin(), lowerContent.end(), 
                      lowerContent.begin(), ::tolower);
        
        size_t pos = 0;
        while ((pos = lowerContent.find(term, pos)) != std::string::npos) {
            relevance += 0.5f;
            pos += term.length();
        }
        
        // Check synonyms
        auto it = m_synonyms.find(term);
        if (it != m_synonyms.end()) {
            for (const auto& synonym : it->second) {
                if (lowerTitle.find(synonym) != std::string::npos) {
                    relevance += 1.0f;
                }
                if (lowerContent.find(synonym) != std::string::npos) {
                    relevance += 0.25f;
                }
            }
        }
    }
    
    return relevance;
}

std::vector<std::string> SearchEngine::TokenizeQuery(const std::string& query) {
    std::vector<std::string> tokens;
    std::istringstream iss(query);
    std::string token;
    
    while (iss >> token) {
        // Convert to lowercase
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        
        // Remove punctuation
        token.erase(std::remove_if(token.begin(), token.end(), 
                                   [](char c) { return !std::isalnum(c); }), 
                   token.end());
        
        if (!token.empty() && m_stopWords.count(token) == 0) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

// =============================================================================
// HELP SYSTEM MANAGER IMPLEMENTATION
// =============================================================================

HelpSystem& HelpSystem::Get() {
    static HelpSystem instance;
    return instance;
}

void HelpSystem::Initialize() {
    // Set up default stop words
    std::vector<std::string> stopWords = {
        "the", "a", "an", "and", "or", "but", "in", "on", "at", 
        "to", "for", "of", "with", "by", "from", "as", "is", "was",
        "are", "were", "been", "be", "have", "has", "had", "do",
        "does", "did", "will", "would", "could", "should", "may",
        "might", "must", "can", "this", "that", "these", "those"
    };
    m_searchEngine.SetStopWords(stopWords);
    
    // Add common synonyms
    m_searchEngine.AddSynonym("create", "make");
    m_searchEngine.AddSynonym("create", "new");
    m_searchEngine.AddSynonym("delete", "remove");
    m_searchEngine.AddSynonym("delete", "destroy");
    m_searchEngine.AddSynonym("modify", "edit");
    m_searchEngine.AddSynonym("modify", "change");
    m_searchEngine.AddSynonym("component", "comp");
    m_searchEngine.AddSynonym("entity", "object");
    m_searchEngine.AddSynonym("entity", "gameobject");
    
    // Build initial index
    m_searchEngine.BuildIndex();
}

void HelpSystem::Shutdown() {
    m_activeTutorial.reset();
    m_activeTour.reset();
    m_articles.clear();
    m_tutorials.clear();
    m_tours.clear();
}

void HelpSystem::Update(float deltaTime) {
    // Update active tutorial
    if (m_activeTutorial && m_activeTutorial->IsActive()) {
        m_activeTutorial->Update(deltaTime);
    }
    
    // Update active tour
    if (m_activeTour) {
        m_activeTour->Update(deltaTime);
    }
    
    // Update context provider
    m_contextProvider.UpdateHoveredWidget();
    
    // Update onboarding
    if (m_onboarding.IsActive()) {
        m_onboarding.Update(deltaTime);
    }
}

void HelpSystem::Render() {
    // Render active tutorial
    if (m_activeTutorial && m_activeTutorial->IsActive()) {
        m_activeTutorial->Render();
    }
    
    // Render active tour
    if (m_activeTour) {
        m_activeTour->Render();
    }
    
    // Render onboarding
    if (m_onboarding.IsActive()) {
        m_onboarding.Render();
    }
    
    // Render help windows
    if (m_showHelpWindow) {
        RenderHelpWindow();
    }
    
    if (m_showSearchDialog) {
        RenderSearchDialog();
    }
    
    if (m_showTutorialBrowser) {
        RenderTutorialBrowser();
    }
    
    if (m_showTroubleshooter) {
        m_troubleshooter.ShowTroubleshooter();
    }
    
    // Render tooltips
    RenderTooltips();
}

void HelpSystem::LoadHelpContent(const std::string& path) {
    // Load help content from files
    // This would typically parse markdown or JSON files
    // For now, we'll create content programmatically
}

void HelpSystem::RegisterArticle(std::unique_ptr<HelpArticle> article) {
    m_searchEngine.IndexContent(*article);
    m_articles[article->GetId()] = std::move(article);
}

void HelpSystem::RegisterTutorial(std::unique_ptr<Tutorial> tutorial) {
    m_searchEngine.IndexTutorial(*tutorial);
    m_tutorials[tutorial->m_id] = std::move(tutorial);
}

void HelpSystem::RegisterTour(std::unique_ptr<GuidedTour> tour) {
    m_tours[tour->m_id] = std::move(tour);
}

void HelpSystem::ShowHelp(const std::string& topicId) {
    m_showHelpWindow = true;
    if (!topicId.empty()) {
        m_currentArticleId = topicId;
    }
}

void HelpSystem::ShowTutorialBrowser() {
    m_showTutorialBrowser = true;
}

void HelpSystem::StartTutorial(const std::string& tutorialId) {
    auto it = m_tutorials.find(tutorialId);
    if (it != m_tutorials.end()) {
        m_activeTutorial = std::make_unique<Tutorial>(*it->second);
        m_activeTutorial->Start();
    }
}

void HelpSystem::ShowContextHelp() {
    m_contextProvider.ShowContextHelp(m_contextProvider.m_currentWidget);
}

void HelpSystem::ShowSearchDialog() {
    m_showSearchDialog = true;
}

void HelpSystem::ShowTroubleshooter() {
    m_showTroubleshooter = true;
}

void HelpSystem::ShowQuickHelp(const std::string& text) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
#endif
}

void HelpSystem::ShowTooltip(const std::string& text) {
    ShowQuickHelp(text);
}

void HelpSystem::ShowHotkeys() {
    RenderHotkeys();
}

void HelpSystem::SetUserLevel(UserLevel level) {
    m_userLevel = level;
}

void HelpSystem::SetLanguage(const std::string& langCode) {
    m_language = langCode;
}

void HelpSystem::SetTooltipsEnabled(bool enabled) {
    m_tooltipsEnabled = enabled;
    m_contextProvider.SetEnabled(enabled);
}

void HelpSystem::SetTutorialSpeed(float speed) {
    m_tutorialSpeed = speed;
}

void HelpSystem::TrackHelpUsage(const std::string& contentId) {
    m_viewCounts[contentId]++;
    m_viewHistory.push_back(contentId);
    
    // Keep history size manageable
    if (m_viewHistory.size() > 1000) {
        m_viewHistory.erase(m_viewHistory.begin());
    }
}

std::vector<std::string> HelpSystem::GetMostViewedTopics(int count) {
    // Sort by view count
    std::vector<std::pair<int, std::string>> sorted;
    for (const auto& [id, views] : m_viewCounts) {
        sorted.push_back({views, id});
    }
    
    std::sort(sorted.begin(), sorted.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    std::vector<std::string> result;
    for (size_t i = 0; i < std::min(static_cast<size_t>(count), sorted.size()); ++i) {
        result.push_back(sorted[i].second);
    }
    
    return result;
}

std::vector<std::string> HelpSystem::GetRecommendedContent() {
    std::vector<std::string> recommendations;
    
    // Recommend based on user level
    for (const auto& [id, article] : m_articles) {
        if (article->m_minLevel == m_userLevel) {
            recommendations.push_back(id);
        }
    }
    
    // Recommend based on recent views
    if (!m_viewHistory.empty()) {
        const std::string& lastViewed = m_viewHistory.back();
        auto it = m_articles.find(lastViewed);
        if (it != m_articles.end()) {
            // Find articles in same category
            for (const auto& [id, article] : m_articles) {
                if (id != lastViewed && 
                    article->GetCategory() == it->second->GetCategory()) {
                    recommendations.push_back(id);
                }
            }
        }
    }
    
    // Limit recommendations
    if (recommendations.size() > 5) {
        recommendations.resize(5);
    }
    
    return recommendations;
}

void HelpSystem::RenderHelpWindow() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Help Center", &m_showHelpWindow)) {
        // Sidebar with categories
        ImGui::BeginChild("Categories", ImVec2(200, 0), true);
        
        const char* categories[] = {
            "Getting Started", "Dashboard", "ECS System", "Rendering",
            "Physics", "Audio", "Networking", "Assets", "Debugging",
            "Plugins", "Scripting", "Performance", "Troubleshooting"
        };
        
        for (int i = 0; i < IM_ARRAYSIZE(categories); ++i) {
            if (ImGui::Selectable(categories[i])) {
                auto results = m_searchEngine.SearchByCategory(
                    static_cast<HelpCategory>(i));
                
                if (!results.empty()) {
                    m_currentArticleId = results[0].id;
                }
            }
        }
        
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Content area
        ImGui::BeginChild("Content", ImVec2(0, 0), true);
        
        // Search bar
        static char searchBuffer[256] = "";
        if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer))) {
            // Perform search
            if (strlen(searchBuffer) > 0) {
                auto results = m_searchEngine.Search(searchBuffer);
                if (!results.empty()) {
                    m_currentArticleId = results[0].id;
                }
            }
        }
        
        ImGui::Separator();
        
        // Display current article
        if (!m_currentArticleId.empty()) {
            auto it = m_articles.find(m_currentArticleId);
            if (it != m_articles.end()) {
                it->second->Render();
                TrackHelpUsage(m_currentArticleId);
            }
        } else {
            ImGui::Text("Welcome to ECScope Help Center");
            ImGui::Separator();
            ImGui::TextWrapped("Select a category from the left or use the search bar to find help topics.");
            
            // Show recommended content
            auto recommendations = GetRecommendedContent();
            if (!recommendations.empty()) {
                ImGui::Separator();
                ImGui::Text("Recommended for you:");
                for (const auto& id : recommendations) {
                    auto it = m_articles.find(id);
                    if (it != m_articles.end()) {
                        if (ImGui::SmallButton(it->second->GetTitle().c_str())) {
                            m_currentArticleId = id;
                        }
                    }
                }
            }
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
#endif
}

void HelpSystem::RenderSearchDialog() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Search Help", &m_showSearchDialog)) {
        static char searchQuery[256] = "";
        bool search = false;
        
        if (ImGui::InputText("Search Query", searchQuery, sizeof(searchQuery), 
                            ImGuiInputTextFlags_EnterReturnsTrue)) {
            search = true;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Search")) {
            search = true;
        }
        
        if (search && strlen(searchQuery) > 0) {
            auto results = m_searchEngine.Search(searchQuery);
            
            ImGui::Separator();
            ImGui::Text("Found %zu results", results.size());
            ImGui::Separator();
            
            ImGui::BeginChild("Results");
            for (const auto& result : results) {
                if (ImGui::TreeNode(result.title.c_str())) {
                    ImGui::TextWrapped("%s", result.snippet.c_str());
                    
                    if (ImGui::Button("Open")) {
                        if (result.type == HelpContentType::Article) {
                            m_currentArticleId = result.id;
                            m_showHelpWindow = true;
                        } else if (result.type == HelpContentType::Tutorial) {
                            StartTutorial(result.id);
                        }
                    }
                    
                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();
        }
        
        // Show suggestions
        if (strlen(searchQuery) > 0) {
            auto suggestions = m_searchEngine.GetSuggestions(searchQuery);
            if (!suggestions.empty()) {
                ImGui::Text("Suggestions:");
                for (const auto& suggestion : suggestions) {
                    if (ImGui::SmallButton(suggestion.c_str())) {
                        strcpy(searchQuery, suggestion.c_str());
                    }
                    ImGui::SameLine();
                }
                ImGui::NewLine();
            }
        }
    }
    ImGui::End();
#endif
}

void HelpSystem::RenderTutorialBrowser() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Tutorial Browser", &m_showTutorialBrowser)) {
        // Filter by category
        static int selectedCategory = 0;
        const char* categories[] = {
            "All", "Getting Started", "Dashboard", "ECS", "Rendering",
            "Physics", "Audio", "Networking", "Assets", "Debugging",
            "Plugins", "Scripting"
        };
        
        ImGui::Combo("Category", &selectedCategory, categories, IM_ARRAYSIZE(categories));
        
        ImGui::Separator();
        
        // Tutorial list
        ImGui::BeginChild("Tutorials");
        
        for (const auto& [id, tutorial] : m_tutorials) {
            // Filter by category
            if (selectedCategory != 0 && 
                tutorial->m_category != static_cast<HelpCategory>(selectedCategory - 1)) {
                continue;
            }
            
            ImGui::PushID(id.c_str());
            
            if (ImGui::CollapsingHeader(tutorial->m_name.c_str())) {
                ImGui::Text("Description: %s", tutorial->m_description.c_str());
                ImGui::Text("Estimated Time: %d minutes", tutorial->m_estimatedMinutes);
                ImGui::Text("Level: %s", 
                           tutorial->m_targetLevel == UserLevel::Beginner ? "Beginner" :
                           tutorial->m_targetLevel == UserLevel::Intermediate ? "Intermediate" :
                           tutorial->m_targetLevel == UserLevel::Advanced ? "Advanced" : "Expert");
                
                if (!tutorial->m_prerequisites.empty()) {
                    ImGui::Text("Prerequisites:");
                    for (const auto& prereq : tutorial->m_prerequisites) {
                        ImGui::BulletText("%s", prereq.c_str());
                    }
                }
                
                if (ImGui::Button("Start Tutorial")) {
                    StartTutorial(id);
                    m_showTutorialBrowser = false;
                }
            }
            
            ImGui::PopID();
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
#endif
}

void HelpSystem::RenderQuickHelp() {
#ifdef ECSCOPE_HAS_IMGUI
    // Quick help overlay for current context
    if (m_tooltipsEnabled && !m_contextProvider.m_currentWidget.empty()) {
        m_contextProvider.ShowTooltip(m_contextProvider.m_currentWidget);
    }
#endif
}

void HelpSystem::RenderTooltips() {
    RenderQuickHelp();
}

void HelpSystem::RenderHotkeys() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Keyboard Shortcuts")) {
        struct Hotkey {
            const char* action;
            const char* keys;
            const char* category;
        };
        
        Hotkey hotkeys[] = {
            // File operations
            {"New Project", "Ctrl+N", "File"},
            {"Open Project", "Ctrl+O", "File"},
            {"Save", "Ctrl+S", "File"},
            {"Save As", "Ctrl+Shift+S", "File"},
            
            // Edit operations
            {"Undo", "Ctrl+Z", "Edit"},
            {"Redo", "Ctrl+Y", "Edit"},
            {"Cut", "Ctrl+X", "Edit"},
            {"Copy", "Ctrl+C", "Edit"},
            {"Paste", "Ctrl+V", "Edit"},
            {"Delete", "Delete", "Edit"},
            {"Select All", "Ctrl+A", "Edit"},
            {"Find", "Ctrl+F", "Edit"},
            {"Replace", "Ctrl+H", "Edit"},
            
            // View operations
            {"Toggle Fullscreen", "F11", "View"},
            {"Zoom In", "Ctrl++", "View"},
            {"Zoom Out", "Ctrl+-", "View"},
            {"Reset Zoom", "Ctrl+0", "View"},
            
            // Engine operations
            {"Play/Pause", "Space", "Engine"},
            {"Stop", "Escape", "Engine"},
            {"Step Frame", "F10", "Engine"},
            {"Toggle Debug", "F12", "Engine"},
            
            // Help
            {"Show Help", "F1", "Help"},
            {"Context Help", "Shift+F1", "Help"},
            {"Search", "Ctrl+Shift+F", "Help"},
        };
        
        // Group by category
        const char* currentCategory = nullptr;
        for (const auto& hotkey : hotkeys) {
            if (currentCategory == nullptr || strcmp(currentCategory, hotkey.category) != 0) {
                currentCategory = hotkey.category;
                ImGui::Separator();
                ImGui::Text("%s", currentCategory);
                ImGui::Separator();
            }
            
            ImGui::Text("%-30s %s", hotkey.action, hotkey.keys);
        }
    }
    ImGui::End();
#endif
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void HighlightRegion(const ImVec2& pos, const ImVec2& size, const ImVec4& color) {
#ifdef ECSCOPE_HAS_IMGUI
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    // Draw highlight rectangle
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                            ImGui::GetColorU32(color));
    
    // Draw border
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, 1.0f)),
                      0.0f, 0, 2.0f);
#endif
}

void DrawArrow(const ImVec2& from, const ImVec2& to, const ImVec4& color) {
#ifdef ECSCOPE_HAS_IMGUI
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    // Draw line
    drawList->AddLine(from, to, ImGui::GetColorU32(color), 2.0f);
    
    // Calculate arrowhead
    ImVec2 dir = ImVec2(to.x - from.x, to.y - from.y);
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (length > 0) {
        dir.x /= length;
        dir.y /= length;
        
        const float arrowSize = 15.0f;
        ImVec2 p1 = ImVec2(to.x - dir.x * arrowSize - dir.y * arrowSize * 0.5f,
                          to.y - dir.y * arrowSize + dir.x * arrowSize * 0.5f);
        ImVec2 p2 = ImVec2(to.x - dir.x * arrowSize + dir.y * arrowSize * 0.5f,
                          to.y - dir.y * arrowSize - dir.x * arrowSize * 0.5f);
        
        drawList->AddTriangleFilled(to, p1, p2, ImGui::GetColorU32(color));
    }
#endif
}

void ShowNotification(const std::string& text, float duration) {
#ifdef ECSCOPE_HAS_IMGUI
    // This would typically be handled by a notification system
    // For now, we'll just show a tooltip
    ImGui::SetTooltip("%s", text.c_str());
#endif
}

bool DetectUserNeedsHelp() {
    // Implement heuristics to detect if user needs help
    // - Check idle time
    // - Check repeated undo operations
    // - Check error count
    // - Check if user is hovering over same element repeatedly
    
    // Placeholder implementation
    return false;
}

} // namespace ecscope::gui::help