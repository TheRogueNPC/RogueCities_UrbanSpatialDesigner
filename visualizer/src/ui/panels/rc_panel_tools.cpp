// FILE: rc_panel_tools.cpp
// PURPOSE: Bottom tools strip (viewport-centric cockpit controls).

#include "ui/panels/rc_panel_tools.h"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/panels/rc_panel_dev_shell.h"
#include "ui/introspection/UiIntrospection.h"
#include "integration/AiAssist.h"

#include <RogueCity/Core/Editor/EditorState.hpp>
#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <cmath>
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
        const float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(ImGui::GetTime() * 4.0));
        const ImU32 pulse_color = LerpColor(UITokens::GreenHUD, UITokens::CyanAccent, 1.0f - pulse);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(pulse_color));
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
        ImGui::ColorConvertU32ToFloat4(dirty_any ? UITokens::YellowWarning : UITokens::SuccessGreen),
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
        const ImU32 chip_color = dirty
            ? WithAlpha(UITokens::AmberGlow, 230u)
            : WithAlpha(UITokens::SuccessGreen, 190u);
        ImVec4 color = ImGui::ColorConvertU32ToFloat4(chip_color);
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

    ImGui::SeparatorText("Debug Overlays");
    ImGui::Checkbox("Tensor Field Overlay", &gs.debug_show_tensor_overlay);
    ImGui::SameLine();
    ImGui::Checkbox("Height Field Overlay", &gs.debug_show_height_overlay);
    ImGui::SameLine();
    ImGui::Checkbox("Zone Field Overlay", &gs.debug_show_zone_overlay);
    ImGui::TextDisabled("Minimap: wheel=zoom, drag=pan, click=jump, L=pin/cycle, Shift+L=release auto, 1/2/3=set LOD, K=adaptive.");

    ImGui::SeparatorText("Minimap LOD");
    bool manual_lod = AxiomEditor::IsMinimapManualLODOverride();
    if (ImGui::Checkbox("Manual LOD Pin", &manual_lod)) {
        AxiomEditor::SetMinimapManualLODOverride(manual_lod);
    }

    const char* lod_items[] = { "LOD0 (Full)", "LOD1 (Medium)", "LOD2 (Coarse)" };
    int manual_level = AxiomEditor::GetMinimapManualLODLevel();
    if (!manual_lod) {
        ImGui::BeginDisabled();
    }
    ImGui::SetNextItemWidth(170.0f);
    if (ImGui::Combo("Pinned LOD", &manual_level, lod_items, IM_ARRAYSIZE(lod_items))) {
        AxiomEditor::SetMinimapManualLODLevel(manual_level);
    }
    if (!manual_lod) {
        ImGui::EndDisabled();
    }

    bool adaptive_quality = AxiomEditor::IsMinimapAdaptiveQualityEnabled();
    if (ImGui::Checkbox("Adaptive Degradation", &adaptive_quality)) {
        AxiomEditor::SetMinimapAdaptiveQualityEnabled(adaptive_quality);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", AxiomEditor::GetMinimapLODStatusText());

    ImGui::SeparatorText("Texture Editing (Phase 6)");
    if (gs.HasTextureSpace()) {
        static float brush_radius_m = 30.0f;
        static float brush_strength = 1.0f;
        static int zone_value = 1;
        static int material_value = 1;

        ImGui::SetNextItemWidth(120.0f);
        ImGui::DragFloat("Brush Radius (m)", &brush_radius_m, 1.0f, 1.0f, 250.0f, "%.1f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::DragFloat("Brush Strength", &brush_strength, 0.05f, 0.05f, 10.0f, "%.2f");

        const RogueCity::Core::Vec2 brush_center = gs.TextureSpaceRef().bounds().center();
        if (ImGui::Button("Raise Height @ Center")) {
            TerrainBrush::Stroke stroke{};
            stroke.world_center = brush_center;
            stroke.radius_meters = brush_radius_m;
            stroke.strength = brush_strength;
            stroke.mode = TerrainBrush::Mode::Raise;
            const bool applied = gs.ApplyTerrainBrush(stroke);
            (void)applied;
        }
        ImGui::SameLine();
        if (ImGui::Button("Lower Height @ Center")) {
            TerrainBrush::Stroke stroke{};
            stroke.world_center = brush_center;
            stroke.radius_meters = brush_radius_m;
            stroke.strength = brush_strength;
            stroke.mode = TerrainBrush::Mode::Lower;
            const bool applied = gs.ApplyTerrainBrush(stroke);
            (void)applied;
        }
        ImGui::SameLine();
        if (ImGui::Button("Smooth Height @ Center")) {
            TerrainBrush::Stroke stroke{};
            stroke.world_center = brush_center;
            stroke.radius_meters = brush_radius_m;
            stroke.strength = 0.6f;
            stroke.mode = TerrainBrush::Mode::Smooth;
            const bool applied = gs.ApplyTerrainBrush(stroke);
            (void)applied;
        }

        ImGui::SetNextItemWidth(100.0f);
        ImGui::DragInt("Zone Value", &zone_value, 1.0f, 0, 255);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110.0f);
        ImGui::DragInt("Material Value", &material_value, 1.0f, 0, 255);
        if (ImGui::Button("Paint Zone @ Center")) {
            TexturePainting::Stroke stroke{};
            stroke.layer = TexturePainting::Layer::Zone;
            stroke.world_center = brush_center;
            stroke.radius_meters = brush_radius_m;
            stroke.opacity = 1.0f;
            stroke.value = static_cast<uint8_t>(zone_value);
            const bool applied = gs.ApplyTexturePaint(stroke);
            (void)applied;
        }
        ImGui::SameLine();
        if (ImGui::Button("Paint Material @ Center")) {
            TexturePainting::Stroke stroke{};
            stroke.layer = TexturePainting::Layer::Material;
            stroke.world_center = brush_center;
            stroke.radius_meters = brush_radius_m;
            stroke.opacity = 1.0f;
            stroke.value = static_cast<uint8_t>(material_value);
            const bool applied = gs.ApplyTexturePaint(stroke);
            (void)applied;
        }
    } else {
        ImGui::TextDisabled("TextureSpace not initialized yet.");
    }

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
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed), "%s", err);
    }

    uiint.EndPanel();
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::Tools
