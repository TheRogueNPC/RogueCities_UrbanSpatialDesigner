// FILE: rc_panel_ui_agent.cpp
// PURPOSE: UI Agent Assistant for AI-driven layout optimization AND design/refactor planning

#include "rc_panel_ui_agent.h"
#include "client/UiAgentClient.h"
#include "client/UiDesignAssistant.h"
#include "runtime/AiBridgeRuntime.h"
#include "RogueCity/App/UI/DesignSystem.h"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/App/Tools/AxiomVisual.hpp"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/introspection/UiIntrospection.h"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>
#include <cmath>

namespace RogueCity::UI {

static void RenderBusyIndicator(std::atomic<bool>& busyFlag, float& busyTimeSeconds) {
    if (!busyFlag.load()) return;

    busyTimeSeconds += ImGui::GetIO().DeltaTime;
    float t = (sinf(busyTimeSeconds * 3.14f) * 0.5f) + 0.5f; // 0..1
    float alpha = 0.3f + 0.7f * t;

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImGui::ColorConvertU32ToFloat4(RC_UI::WithAlpha(RC_UI::UITokens::InfoBlue, static_cast<uint8_t>(alpha * 255.0f))));
    ImGui::Text("AI processing...");
    ImGui::PopStyleColor();
    ImGui::Separator();
}

static std::string ToUpperCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

static std::string ActiveModeFromHFSM() {
    using RogueCity::Core::Editor::EditorState;
    const auto state = RogueCity::Core::Editor::GetEditorHFSM().state();
    switch (state) {
        case EditorState::Editing_Axioms:
        case EditorState::Viewport_PlaceAxiom: return "AXIOM";
        case EditorState::Editing_Roads:
        case EditorState::Viewport_DrawRoad: return "ROAD";
        case EditorState::Editing_Districts: return "DISTRICT";
        case EditorState::Editing_Lots: return "LOT";
        case EditorState::Editing_Buildings: return "BUILDING";
        case EditorState::Idle: return "IDLE";
        default: return "IDLE";
    }
}

static std::string ResolveWindowName(const std::string& panel) {
    const std::string key = ToUpperCopy(panel);
    if (key == "TOOLS" || key == "RC_PANEL_TOOLS") return "Tools";
    if (key == "ROGUEVISUALIZER" || key == "VIEWPORT" || key == "RC_UI_ROGUE_VISUALIZER") return "RogueVisualizer";
    if (key == "LOG" || key == "RC_PANEL_LOG") return "Log";
    if (key == "DISTRICT_INDEX" || key == "DISTRICT INDEX" || key == "RC_PANEL_DISTRICT_INDEX") return "District Index";
    if (key == "ROAD_INDEX" || key == "ROAD INDEX" || key == "RC_PANEL_ROAD_INDEX") return "Road Index";
    if (key == "LOT_INDEX" || key == "LOT INDEX" || key == "RC_PANEL_LOT_INDEX") return "Lot Index";
    if (key == "RIVER_INDEX" || key == "RIVER INDEX" || key == "RC_PANEL_RIVER_INDEX") return "River Index";
    if (key == "INSPECTOR" || key == "RC_PANEL_INSPECTOR") return "Inspector";
    if (key == "AI CONSOLE" || key == "RC_PANEL_AI_CONSOLE") return "AI Console";
    if (key == "UI AGENT ASSISTANT" || key == "RC_PANEL_UI_AGENT") return "UI Agent Assistant";
    if (key == "CITY SPEC GENERATOR" || key == "RC_PANEL_CITY_SPEC") return "City Spec Generator";
    if (key == "AXIOM BAR" || key == "RC_PANEL_AXIOM_BAR" || key == "TOOL DECK" || key == "RC_PANEL_TOOL_DECK") return "Tool Deck";
    if (key == "AXIOM LIBRARY" || key == "RC_PANEL_AXIOM_LIBRARY") return "Axiom Library";
    return panel;
}

static std::string NormalizeDockArea(const std::string& dock) {
    const std::string key = ToUpperCopy(dock);
    if (key == "LEFT") return "Left";
    if (key == "RIGHT") return "Right";
    if (key == "TOP") return "Top";
    if (key == "BOTTOM") return "Bottom";
    if (key == "CENTER") return "Center";
    if (key == "TOOLDECK" || key == "TOOL DECK") return "ToolDeck";
    if (key == "LIBRARY" || key == "LIBRARYDOCK") return "Library";
    return "Center";
}

// Helper to build snapshot with live code-shape metadata
static AI::UiSnapshot BuildEnhancedSnapshot() {
    AI::UiSnapshot snapshot;
    snapshot.app = "RogueCity Visualizer";
    snapshot.header.left = "ROGUENAV";
    snapshot.header.mode = RC_UI::Panels::AxiomEditor::GetRogueNavModeName();
    snapshot.header.filter = RC_UI::Panels::AxiomEditor::GetRogueNavFilterName();

    const ImGuiIO& io = ImGui::GetIO();
    snapshot.dockingEnabled = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0;
    snapshot.multiViewportEnabled = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;

    const auto& introspection = RogueCity::UIInt::UiIntrospector::Instance().Snapshot();
    for (const auto& panel : introspection.panels) {
        AI::UiPanelInfo outPanel;
        outPanel.id = panel.id;
        outPanel.dock = panel.dock_area.empty() ? "Floating" : panel.dock_area;
        outPanel.visible = panel.visible;
        outPanel.role = panel.role;
        outPanel.owner_module = panel.owner_module;

        for (const auto& widget : panel.widgets) {
            if (!widget.binding.empty()) {
                outPanel.data_bindings.push_back(widget.binding);
            }
        }
        for (const auto& tag : panel.tags) {
            outPanel.interaction_patterns.push_back(tag);
        }

        snapshot.panels.push_back(std::move(outPanel));
    }

    if (snapshot.panels.empty()) {
        snapshot.panels.push_back({"Tools", "Bottom", true, "toolbox", "rc_panel_tools", {"tool.active"}, {"toolbar"}});
        snapshot.panels.push_back({"RogueVisualizer", "Center", true, "viewport", "rc_panel_axiom_editor", {"camera.*"}, {"canvas"}});
    }

    const auto& globalState = RogueCity::Core::Editor::GetGlobalState();
    snapshot.state.flowRate = RC_UI::Panels::AxiomEditor::GetFlowRate();
    snapshot.state.livePreview = RC_UI::Panels::AxiomEditor::IsLivePreviewEnabled();
    snapshot.state.debounceSec = RC_UI::Panels::AxiomEditor::GetDebounceSeconds();
    snapshot.state.seed = RC_UI::Panels::AxiomEditor::GetSeed();
    snapshot.state.activeTool = ActiveModeFromHFSM();

    if (auto* selected = RC_UI::Panels::AxiomEditor::GetSelectedAxiom(); selected != nullptr) {
        snapshot.state.selectedAxioms.push_back("A" + std::to_string(selected->id()));
    }

    snapshot.state.state_model["frame_counter"] = std::to_string(globalState.frame_counter);
    snapshot.state.state_model["roads.count"] = std::to_string(globalState.roads.size());
    snapshot.state.state_model["districts.count"] = std::to_string(globalState.districts.size());
    snapshot.state.state_model["lots.count"] = std::to_string(globalState.lots.size());
    snapshot.state.state_model["mode.active"] = ActiveModeFromHFSM();

    snapshot.logTail.push_back("Mode: " + ActiveModeFromHFSM());
    snapshot.logTail.push_back("LivePreview: " + std::string(snapshot.state.livePreview ? "on" : "off"));
    snapshot.logTail.push_back("Seed: " + std::to_string(snapshot.state.seed));

    return snapshot;
}

static bool ApplyUiCommand(const AI::UiCommand& cmd, std::string& lineOut) {
    using RogueCity::Core::Editor::EditorEvent;
    auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();

    if (cmd.cmd == "SetHeader") {
        bool changed = false;
        if (cmd.mode) {
            const std::string mode = ToUpperCopy(*cmd.mode);
            if (mode == "AXIOM") {
                hfsm.handle_event(EditorEvent::Tool_Axioms, gs);
                changed = true;
            } else if (mode == "ROAD") {
                hfsm.handle_event(EditorEvent::Tool_Roads, gs);
                changed = true;
            } else if (mode == "DISTRICT") {
                hfsm.handle_event(EditorEvent::Tool_Districts, gs);
                changed = true;
            } else if (mode == "LOT") {
                hfsm.handle_event(EditorEvent::Tool_Lots, gs);
                changed = true;
            } else if (mode == "BUILDING") {
                hfsm.handle_event(EditorEvent::Tool_Buildings, gs);
                changed = true;
            } else if (mode == "IDLE") {
                hfsm.handle_event(EditorEvent::GotoIdle, gs);
                changed = true;
            }
        }
        if (cmd.filter) {
            changed = RC_UI::Panels::AxiomEditor::SetRogueNavFilterByName(*cmd.filter) || changed;
        }
        lineOut = changed ? "applied SetHeader" : "ignored SetHeader";
        return changed;
    }

    if (cmd.cmd == "DockPanel") {
        if (!cmd.panel || !cmd.targetDock) {
            lineOut = "ignored DockPanel (missing panel/target)";
            return false;
        }
        const std::string windowName = ResolveWindowName(*cmd.panel);
        const std::string target = NormalizeDockArea(*cmd.targetDock);
        const bool queued = RC_UI::QueueDockWindow(windowName.c_str(), target.c_str(), cmd.ownDockNode.value_or(false));
        lineOut = queued ? "queued DockPanel " + windowName + " -> " + target : "failed DockPanel";
        return queued;
    }

    if (cmd.cmd == "SetState") {
        if (!cmd.key) {
            lineOut = "ignored SetState (missing key)";
            return false;
        }

        const std::string& key = *cmd.key;
        if (key == "flowRate" && cmd.valueNumber) {
            RC_UI::Panels::AxiomEditor::SetFlowRate(static_cast<float>(*cmd.valueNumber));
            lineOut = "applied flowRate";
            return true;
        }
        if (key == "livePreview" && cmd.valueBool) {
            RC_UI::Panels::AxiomEditor::SetLivePreviewEnabled(*cmd.valueBool);
            lineOut = "applied livePreview";
            return true;
        }
        if (key == "debounceSec" && cmd.valueNumber) {
            RC_UI::Panels::AxiomEditor::SetDebounceSeconds(static_cast<float>(*cmd.valueNumber));
            lineOut = "applied debounceSec";
            return true;
        }
        if (key == "seed" && cmd.valueNumber) {
            RC_UI::Panels::AxiomEditor::SetSeed(static_cast<uint32_t>(*cmd.valueNumber));
            if (RC_UI::Panels::AxiomEditor::IsLivePreviewEnabled() && RC_UI::Panels::AxiomEditor::CanGenerate()) {
                RC_UI::Panels::AxiomEditor::ForceGenerate();
            }
            lineOut = "applied seed";
            return true;
        }

        lineOut = "ignored SetState key=" + key;
        return false;
    }

    if (cmd.cmd == "Request") {
        lineOut = "AI requested more fields";
        return true;
    }

    lineOut = "ignored cmd=" + cmd.cmd;
    return false;
}

void UiAgentPanel::Render() {
    RC_UI::ApplyUnifiedWindowSchema();
    const bool open = ImGui::Begin("UI Agent Assistant", nullptr, ImGuiWindowFlags_NoCollapse);
    RC_UI::PopUnifiedWindowSchema();
    RC_UI::BeginUnifiedTextWrap();

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "UI Agent Assistant",
            "UI Agent Assistant",
            "toolbox",
            "Floating",
            "visualizer/src/ui/panels/rc_panel_ui_agent.cpp",
            {"ai", "layout", "refactor", "v0.0.9"}
        },
        open
    );

    if (!open) {
        RC_UI::EndUnifiedTextWrap();
        uiint.EndPanel();
        ImGui::End();
        return;
    }
    
    auto& runtime = AI::AiBridgeRuntime::Instance();
    
    // Check if bridge is online
    if (!runtime.IsOnline()) {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(RC_UI::UITokens::YellowWarning));
        ImGui::Text("AI Bridge offline - start it in AI Console");
        ImGui::PopStyleColor();
        uiint.RegisterWidget({"text", "AI Bridge offline", "ai.bridge.status", {"ai", "status"}});
        uiint.EndPanel();
        RC_UI::EndUnifiedTextWrap();
        ImGui::End();
        return;
    }
    
    // === MODE 1: LAYOUT COMMANDS ===
    DesignSystem::SectionHeader("Layout Optimization");
    RenderBusyIndicator(m_processing, m_busyTime);
    
    ImGui::Text("Ask AI to adjust the UI layout:");
    ImGui::InputTextMultiline("##goal", m_goalBuffer, sizeof(m_goalBuffer), ImVec2(-1, 60));
    uiint.RegisterWidget({"text", "Goal", "ui_agent.goal", {"input"}});
    
    ImGui::BeginDisabled(m_processing.load());
    if (DesignSystem::ButtonPrimary("Apply AI Layout", ImVec2(180, 30))) {
        if (!m_processing.exchange(true)) {
            m_busyTime = 0.0f;
            {
                std::scoped_lock lock(m_resultMutex);
                m_lastResult = "Processing...";
            }

            std::string goal = std::string(m_goalBuffer);
            std::thread([this, goal]() {
                auto snapshot = BuildEnhancedSnapshot();
                auto commands = AI::UiAgentClient::QueryAgent(snapshot, goal);

                std::string result;
                if (commands.empty()) {
                    result = "No commands returned (check console for errors)";
                } else {
                    int appliedCount = 0;
                    result = "Received " + std::to_string(commands.size()) + " commands:\n";
                    for (const auto& cmd : commands) {
                        std::string line;
                        if (ApplyUiCommand(cmd, line)) {
                            ++appliedCount;
                        }
                        result += "- " + cmd.cmd + " => " + line + "\n";
                    }
                    result += "\nApplied " + std::to_string(appliedCount) + " command(s).";
                }

                {
                    std::scoped_lock lock(m_resultMutex);
                    m_lastResult = result;
                }

                m_processing = false;
            }).detach();
        }
    }
    ImGui::EndDisabled();
    uiint.RegisterWidget({"button", "Apply AI Layout", "action:ai.ui_agent.apply_layout", {"action", "ai"}});
    
    {
        std::scoped_lock lock(m_resultMutex);
        if (!m_lastResult.empty()) {
            ImGui::TextWrapped("%s", m_lastResult.c_str());
        }
    }
    
    DesignSystem::Separator();
    
    // === MODE 2: DESIGN/REFACTOR PLANNING ===
    DesignSystem::SectionHeader("Design & Refactoring");
    RenderBusyIndicator(m_designProcessing, m_designBusyTime);
    
    ImGui::Text("Ask AI to analyze UI structure and suggest refactorings:");
    ImGui::InputTextMultiline("##design_goal", m_designGoalBuffer, sizeof(m_designGoalBuffer), ImVec2(-1, 60));
    uiint.RegisterWidget({"text", "Design Goal", "ui_design.goal", {"input"}});
    
    ImGui::BeginDisabled(m_designProcessing.load());
    if (DesignSystem::ButtonSecondary("Generate Refactor Plan", ImVec2(180, 30))) {
        if (!m_designProcessing.exchange(true)) {
            m_designBusyTime = 0.0f;
            {
                std::scoped_lock lock(m_designResultMutex);
                m_lastDesignResult = "Generating plan...";
            }

            std::string goal = std::string(m_designGoalBuffer);
            std::thread([this, goal]() {
                auto snapshot = BuildEnhancedSnapshot();
                auto plan = AI::UiDesignAssistant::GenerateDesignPlan(snapshot, goal);

                // Generate timestamped filename
                auto now = std::chrono::system_clock::now();
                auto t = std::chrono::system_clock::to_time_t(now);
                std::tm tm{};
#ifdef _WIN32
                localtime_s(&tm, &t);
#else
                tm = *std::localtime(&t);
#endif
                std::stringstream ss;
                ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
                std::string filename = "AI/docs/ui/ui_refactor_" + ss.str() + ".json";

                std::string result;
                if (AI::UiDesignAssistant::SaveDesignPlan(plan, filename)) {
                    result = "Saved plan to: " + filename + "\n\n";
                    result += "Patterns: " + std::to_string(plan.component_patterns.size()) + "\n";
                    result += "Refactoring opportunities: " + std::to_string(plan.refactoring_opportunities.size()) + "\n\n";

                    if (!plan.summary.empty()) {
                        result += "Summary:\n" + plan.summary;
                    }
                } else {
                    result = "Failed to save plan (check console)";
                }

                {
                    std::scoped_lock lock(m_designResultMutex);
                    m_lastDesignResult = result;
                }

                m_designProcessing = false;
            }).detach();
        }
    }
    ImGui::EndDisabled();
    uiint.RegisterWidget({"button", "Generate Refactor Plan", "action:ai.ui_design.generate_plan", {"action", "ai"}});
    
    {
        std::scoped_lock lock(m_designResultMutex);
        if (!m_lastDesignResult.empty()) {
            ImGui::TextWrapped("%s", m_lastDesignResult.c_str());
        }
    }

    uiint.EndPanel();
    RC_UI::EndUnifiedTextWrap();
    ImGui::End();
}

} // namespace RogueCity::UI
