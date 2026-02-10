// FILE: rc_panel_tools.cpp
// PURPOSE: Bottom tools strip (viewport-centric cockpit controls).

#include "ui/panels/rc_panel_tools.h"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_root.h"
#include "ui/panels/rc_panel_dev_shell.h"
#include "ui/introspection/UiIntrospection.h"
#include "integration/AiAssist.h"

#include <RogueCity/Core/Editor/EditorState.hpp>
#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <imgui.h>

namespace RC_UI::Panels::Tools {

// AI_INTEGRATION_TAG: V1_PASS1_TASK2_TOOL_BUTTONS
// Helper to render a tool button with state-reactive highlighting
static void RenderToolButton(
    const char* label, 
    RogueCity::Core::Editor::EditorEvent event,
    RogueCity::Core::Editor::EditorState active_state,
    RogueCity::Core::Editor::EditorHFSM& hfsm,
    RogueCity::Core::Editor::GlobalState& gs)
{
    using namespace RogueCity::Core::Editor;
    
    bool is_active = (hfsm.state() == active_state);
    
    // Y2K affordance: glow when active
    if (is_active) {
        float pulse = 0.5f + 0.5f * sinf(ImGui::GetTime() * 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f * pulse + 0.2f, 0.2f, 1.0f));
    }
    
    if (ImGui::Button(label, ImVec2(100, 40))) {
        hfsm.handle_event(event, gs);
    }
    
    if (is_active) {
        ImGui::PopStyleColor();
    }
}

void Draw(float dt)
{
    static RC_UI::DockableWindowState s_tools_window;
    if (!RC_UI::BeginDockableWindow("Tools", s_tools_window, "Bottom", ImGuiWindowFlags_NoCollapse)) {
        return;
    }

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Tools",
            "Tools",
            "toolbox",
            "Bottom",
            "visualizer/src/ui/panels/rc_panel_tools.cpp",
            {"generation", "controls", "cockpit"}
        },
        true
    );

    // AI_INTEGRATION_TAG: V1_PASS1_TASK2_TOOL_MODE_BUTTONS
    // === TOOL MODE BUTTONS (HFSM-driven) ===
    ImGui::SeparatorText("Editor Tools");
    
    // Get HFSM and GlobalState
    using namespace RogueCity::Core::Editor;
    EditorHFSM& hfsm = GetEditorHFSM();
    GlobalState& gs = GetGlobalState();
    
    RenderToolButton("Axiom", EditorEvent::Tool_Axioms, EditorState::Editing_Axioms, hfsm, gs);
    ImGui::SameLine();

    RenderToolButton("Water", EditorEvent::Tool_Water, EditorState::Editing_Water, hfsm, gs);
    ImGui::SameLine();

    RenderToolButton("Road", EditorEvent::Tool_Roads, EditorState::Editing_Roads, hfsm, gs);
    ImGui::SameLine();
    
    RenderToolButton("District", EditorEvent::Tool_Districts, EditorState::Editing_Districts, hfsm, gs);
    ImGui::SameLine();
    
    RenderToolButton("Lot", EditorEvent::Tool_Lots, EditorState::Editing_Lots, hfsm, gs);
    ImGui::SameLine();
    
    RenderToolButton("Building", EditorEvent::Tool_Buildings, EditorState::Editing_Buildings, hfsm, gs);
    
    ImGui::Spacing();
    ImGui::Separator();

    // Generation controls
    const bool can_generate = AxiomEditor::CanGenerate();
    if (!can_generate) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Generate / Regenerate")) {
        AxiomEditor::ForceGenerate();
    }
    uiint.RegisterWidget({"button", "Generate / Regenerate", "action:generator.regenerate", {"action", "generator"}});
    uiint.RegisterAction({"generator.regenerate", "Generate / Regenerate", "Tools", {}, "AxiomEditor::ForceGenerate"});
    if (!can_generate) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    bool live_preview = AxiomEditor::IsLivePreviewEnabled();
    if (ImGui::Checkbox("Live Preview", &live_preview)) {
        AxiomEditor::SetLivePreviewEnabled(live_preview);
    }
    uiint.RegisterWidget({"checkbox", "Live Preview", "preview.live", {"preview"}});

    ImGui::SameLine();
    float debounce = AxiomEditor::GetDebounceSeconds();
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::DragFloat("Debounce", &debounce, 0.01f, 0.0f, 1.5f, "%.2fs")) {
        AxiomEditor::SetDebounceSeconds(debounce);
    }
    uiint.RegisterWidget({"slider", "Debounce", "preview.debounce_sec", {"preview", "timing"}});

    ImGui::SameLine();
    if (ImGui::Button("Random Seed")) {
        AxiomEditor::RandomizeSeed();
        if (AxiomEditor::IsLivePreviewEnabled()) {
            AxiomEditor::ForceGenerate();
        }
    }
    uiint.RegisterWidget({"button", "Random Seed", "action:seed.randomize", {"action", "seed"}});
    uiint.RegisterAction({"seed.randomize", "Random Seed", "Tools", {}, "AxiomEditor::RandomizeSeed"});

    ImGui::SameLine();
    ImGui::Text("Seed: %u", AxiomEditor::GetSeed());
    
    // === AI ASSIST CONTROLS ===
    ImGui::Separator();
    RogueCity::UI::AiAssist::DrawControls(dt);

    ImGui::Separator();
    if (ImGui::Button("Dev Shell")) {
        RC_UI::Panels::DevShell::Toggle();
    }
    uiint.RegisterWidget({"button", "Dev Shell", "action:devshell.toggle", {"dev", "toggle"}});

    // Status / errors
    const char* err = AxiomEditor::GetValidationError();
    if (err && err[0] != '\0') {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", err);
    }

    uiint.EndPanel();
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::Tools
