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

    // Undo / Redo (Axiom tool command history)
    const bool can_undo = AxiomEditor::CanUndo();
    const bool can_redo = AxiomEditor::CanRedo();
    if (!can_undo) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Undo")) {
        AxiomEditor::Undo();
    }
    if (!can_undo) {
        ImGui::EndDisabled();
    }
    uiint.RegisterWidget({"button", "Undo", "action:editor.undo", {"action", "history"}});
    uiint.RegisterAction({"editor.undo", "Undo", "Tools", {}, "AxiomEditor::Undo"});
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", AxiomEditor::GetUndoLabel());
    }

    ImGui::SameLine();
    if (!can_redo) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Redo")) {
        AxiomEditor::Redo();
    }
    if (!can_redo) {
        ImGui::EndDisabled();
    }
    uiint.RegisterWidget({"button", "Redo", "action:editor.redo", {"action", "history"}});
    uiint.RegisterAction({"editor.redo", "Redo", "Tools", {}, "AxiomEditor::Redo"});
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", AxiomEditor::GetRedoLabel());
    }

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

    // Dirty-layer propagation status (edit -> dirty downstream layers -> regenerate).
    const bool dirty_any = gs.dirty_layers.AnyDirty();
    ImGui::SameLine();
    ImGui::TextColored(
        dirty_any ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(0.6f, 0.85f, 0.6f, 1.0f),
        dirty_any ? "Dirty Layers: pending" : "Dirty Layers: clean");

    int dirty_count = 0;
    for (int i = 0; i < static_cast<int>(DirtyLayer::Count); ++i) {
        if (gs.dirty_layers.IsDirty(static_cast<DirtyLayer>(i))) {
            ++dirty_count;
        }
    }
    const float dirty_ratio = static_cast<float>(dirty_count) / static_cast<float>(static_cast<int>(DirtyLayer::Count));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::ProgressBar(
        dirty_ratio,
        ImVec2(120.0f, 0.0f),
        dirty_any ? "Rebuild pending" : "Clean");

    auto draw_chip = [](const char* label, bool dirty) {
        ImVec4 color = dirty ? ImVec4(0.96f, 0.56f, 0.22f, 0.90f) : ImVec4(0.35f, 0.72f, 0.40f, 0.75f);
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
        ImGui::SmallButton(label);
        ImGui::PopStyleColor(3);
    };

    ImGui::SeparatorText("Dirty Layers");
    draw_chip("Axioms", gs.dirty_layers.IsDirty(DirtyLayer::Axioms)); ImGui::SameLine();
    draw_chip("Tensor", gs.dirty_layers.IsDirty(DirtyLayer::Tensor)); ImGui::SameLine();
    draw_chip("Roads", gs.dirty_layers.IsDirty(DirtyLayer::Roads)); ImGui::SameLine();
    draw_chip("Districts", gs.dirty_layers.IsDirty(DirtyLayer::Districts)); ImGui::SameLine();
    draw_chip("Lots", gs.dirty_layers.IsDirty(DirtyLayer::Lots)); ImGui::SameLine();
    draw_chip("Buildings", gs.dirty_layers.IsDirty(DirtyLayer::Buildings)); ImGui::SameLine();
    draw_chip("Viewport", gs.dirty_layers.IsDirty(DirtyLayer::ViewportIndex));

    if (dirty_any) {
        if (ImGui::Button("Clear Dirty Flags")) {
            gs.dirty_layers.MarkAllClean();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Tip: Generate/Regenerate also clears successful rebuild layers.");
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
