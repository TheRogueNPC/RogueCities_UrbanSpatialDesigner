// FILE: rc_panel_tools.cpp
// PURPOSE: Bottom tools strip (viewport-centric cockpit controls).

#include "ui/panels/rc_panel_tools.h"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/tools/rc_tool_dispatcher.h"
#include "RogueCity/App/Editor/CommandHistory.hpp"

#include <RogueCity/Core/Editor/EditorState.hpp>
#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <imgui.h>

namespace RC_UI::Panels::Tools {

namespace {

[[nodiscard]] std::optional<RC_UI::Tools::ToolActionId> DefaultLibraryAction(RC_UI::ToolLibrary tool) {
    using RC_UI::Tools::ToolActionId;
    switch (tool) {
        case RC_UI::ToolLibrary::Axiom: return ToolActionId::Axiom_Organic;
        case RC_UI::ToolLibrary::Water: return ToolActionId::Water_Flow;
        case RC_UI::ToolLibrary::Road: return ToolActionId::Road_Spline;
        case RC_UI::ToolLibrary::District: return ToolActionId::District_Zone;
        case RC_UI::ToolLibrary::Lot: return ToolActionId::Lot_Plot;
        case RC_UI::ToolLibrary::Building: return ToolActionId::Building_Place;
    }
    return std::nullopt;
}

} // namespace

// AI_INTEGRATION_TAG: V1_PASS1_TASK2_TOOL_BUTTONS
// Helper to render a tool button with state-reactive highlighting
static void RenderToolButton(
    const char* label, 
    RogueCity::Core::Editor::EditorEvent event,
    RogueCity::Core::Editor::EditorState active_state,
    RC_UI::ToolLibrary library_tool,
    RogueCity::Core::Editor::EditorHFSM& hfsm,
    RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::UIInt::UiIntrospector& uiint,
    const ImVec2& size)
{
    using namespace RogueCity::Core::Editor;
    
    bool is_active = (hfsm.state() == active_state);
    
    // Y2K affordance: glow when active
    if (is_active) {
        const float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(ImGui::GetTime() * 4.0));
        const ImU32 pulse_color = LerpColor(UITokens::GreenHUD, UITokens::CyanAccent, 1.0f - pulse);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(pulse_color));
    }
    
    if (ImGui::Button(label, size)) {
        bool dispatched = false;
        if (const auto default_action = DefaultLibraryAction(library_tool); default_action.has_value()) {
            std::string dispatch_status;
            const auto dispatch_result = RC_UI::Tools::DispatchToolAction(
                *default_action,
                RC_UI::Tools::DispatchContext{
                    &hfsm,
                    &gs,
                    &uiint,
                    "Tools"
                },
                &dispatch_status);
            dispatched = dispatch_result == RC_UI::Tools::DispatchResult::Handled;
        }
        if (!dispatched) {
            hfsm.handle_event(event, gs);
        }
        RC_UI::ActivateToolLibrary(library_tool);
    }
    
    if (is_active) {
        ImGui::PopStyleColor();
    }
}

void DrawContent(float dt)
{
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    
    // === STATUS LINE (Mode/Filter/Panels) ===
    using namespace RogueCity::Core::Editor;
    EditorHFSM& hfsm = GetEditorHFSM();
    GlobalState& gs = GetGlobalState();
    
    // Determine active mode from HFSM state
    const char* mode_str = "IDLE";
    switch (hfsm.state()) {
        case EditorState::Editing_Axioms:
        case EditorState::Viewport_PlaceAxiom: mode_str = "AXIOM"; break;
        case EditorState::Editing_Water: mode_str = "WATER"; break;
        case EditorState::Editing_Roads:
        case EditorState::Viewport_DrawRoad: mode_str = "ROAD"; break;
        case EditorState::Editing_Districts: mode_str = "DISTRICT"; break;
        case EditorState::Editing_Lots: mode_str = "LOT"; break;
        case EditorState::Editing_Buildings: mode_str = "BUILDING"; break;
        case EditorState::Simulating: mode_str = "SIM"; break;
        default: break;
    }
    
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary),
        "Mode: %s | Seed: %u", mode_str, AxiomEditor::GetSeed());
    ImGui::Spacing();
    
    // AI_INTEGRATION_TAG: V1_PASS1_TASK2_TOOL_MODE_BUTTONS
    // === TOOL MODE BUTTONS (HFSM-driven) ===
    ImGui::SeparatorText("Editor Tools");
    
    struct ToolModeButton {
        const char* label;
        EditorEvent event;
        EditorState state;
        RC_UI::ToolLibrary library;
    };
    constexpr std::array<ToolModeButton, 6> kToolButtons = {{
        {"Axiom", EditorEvent::Tool_Axioms, EditorState::Editing_Axioms, RC_UI::ToolLibrary::Axiom},
        {"Water", EditorEvent::Tool_Water, EditorState::Editing_Water, RC_UI::ToolLibrary::Water},
        {"Road", EditorEvent::Tool_Roads, EditorState::Editing_Roads, RC_UI::ToolLibrary::Road},
        {"District", EditorEvent::Tool_Districts, EditorState::Editing_Districts, RC_UI::ToolLibrary::District},
        {"Lot", EditorEvent::Tool_Lots, EditorState::Editing_Lots, RC_UI::ToolLibrary::Lot},
        {"Building", EditorEvent::Tool_Buildings, EditorState::Editing_Buildings, RC_UI::ToolLibrary::Building},
    }};

    const float toolbar_width = ImGui::GetContentRegionAvail().x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float button_height = std::max(34.0f, ImGui::GetFrameHeight() * 1.30f);

    float min_button_width = 84.0f;
    for (const ToolModeButton& tool_button : kToolButtons) {
        const float text_width = ImGui::CalcTextSize(tool_button.label).x;
        min_button_width = std::max(
            min_button_width,
            text_width + ImGui::GetStyle().FramePadding.x * 2.0f + 14.0f);
    }

    const int tool_count = static_cast<int>(kToolButtons.size());
    const int columns = std::clamp(
        static_cast<int>((toolbar_width + spacing) / (min_button_width + spacing)),
        1,
        tool_count);
    const float button_width = std::max(
        min_button_width,
        (toolbar_width - spacing * static_cast<float>(columns - 1)) / static_cast<float>(columns));
    const ImVec2 button_size(button_width, button_height);
    const auto same_line_if_room = [](float min_remaining_width = 96.0f) {
        if (ImGui::GetContentRegionAvail().x > min_remaining_width) {
            ImGui::SameLine();
        }
    };

    for (int i = 0; i < tool_count; ++i) {
        if ((i % columns) != 0) {
            same_line_if_room();
        }
        const ToolModeButton& tool_button = kToolButtons[static_cast<size_t>(i)];
        ImGui::PushID(tool_button.label);
        RenderToolButton(
            tool_button.label,
            tool_button.event,
            tool_button.state,
            tool_button.library,
            hfsm,
            gs,
            uiint,
            button_size);
        ImGui::PopID();
    }
    
    ImGui::Spacing();
    ImGui::Separator();

    // Undo / Redo (prefer global history, fallback to legacy axiom history)
    auto& global_history = RogueCity::App::GetEditorCommandHistory();
    const bool can_undo = global_history.CanUndo() || AxiomEditor::CanUndo();
    const bool can_redo = global_history.CanRedo() || AxiomEditor::CanRedo();
    if (!can_undo) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Undo")) {
        if (global_history.CanUndo()) {
            global_history.Undo();
        } else {
            AxiomEditor::Undo();
        }
    }
    if (!can_undo) {
        ImGui::EndDisabled();
    }
    uiint.RegisterWidget({"button", "Undo", "action:editor.undo", {"action", "history"}});
    uiint.RegisterAction({"editor.undo", "Undo", "Tools", {}, "AxiomEditor::Undo"});
    if (ImGui::IsItemHovered()) {
        const char* undo_label = AxiomEditor::GetUndoLabel();
        if (global_history.CanUndo() && global_history.PeekUndo() != nullptr) {
            undo_label = global_history.PeekUndo()->GetDescription();
        }
        ImGui::SetTooltip("%s (Ctrl+Z)", undo_label);
    }

    same_line_if_room();
    if (!can_redo) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Redo")) {
        if (global_history.CanRedo()) {
            global_history.Redo();
        } else {
            AxiomEditor::Redo();
        }
    }
    if (!can_redo) {
        ImGui::EndDisabled();
    }
    uiint.RegisterWidget({"button", "Redo", "action:editor.redo", {"action", "history"}});
    uiint.RegisterAction({"editor.redo", "Redo", "Tools", {}, "AxiomEditor::Redo"});
    if (ImGui::IsItemHovered()) {
        const char* redo_label = AxiomEditor::GetRedoLabel();
        if (global_history.CanRedo() && global_history.PeekRedo() != nullptr) {
            redo_label = global_history.PeekRedo()->GetDescription();
        }
        ImGui::SetTooltip("%s (Ctrl+R)", redo_label);
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

    // === Clear Operations (Undoable) ===
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Clear All Data button (warning style)
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(0xFF3A1A1A));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4(0xFF5A2A2A));
    
    static bool show_clear_all_confirm = false;
    if (ImGui::Button("Clear All Data")) {
        show_clear_all_confirm = true;
    }
    ImGui::PopStyleColor(3);
    
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear ALL layers (Axioms, Water, Roads, Districts, Lots, Buildings)\nThis action is UNDOABLE with Ctrl+Z");
    }
    
    // Confirmation modal
    if (show_clear_all_confirm) {
        ImGui::OpenPopup("Confirm Clear All");
        show_clear_all_confirm = false;
    }
    
    if (ImGui::BeginPopupModal("Confirm Clear All", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed), "WARNING:");
        ImGui::Text("This will clear ALL city data:");
        ImGui::BulletText("Axioms");
        ImGui::BulletText("Water Bodies");
        ImGui::BulletText("Roads");
        ImGui::BulletText("Districts");
        ImGui::BulletText("Lots");
        ImGui::BulletText("Buildings");
        ImGui::Spacing();
        ImGui::Text("You can undo this operation with Ctrl+Z.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (ImGui::Button("Yes, Clear All", ImVec2(120, 0))) {
            AxiomEditor::ClearAllData();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Clear by Layer (collapsing header)
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Clear by Layer")) {
        ImGui::Indent();
        
        if (ImGui::Button("Clear Axioms")) {
            AxiomEditor::ClearAxioms();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Clear all Axioms (UNDOABLE with Ctrl+Z)");
        }
        
        if (ImGui::Button("Clear Water")) {
            AxiomEditor::ClearWater();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Clear all Water Bodies (UNDOABLE with Ctrl+Z)");
        }
        
        if (ImGui::Button("Clear Roads")) {
            AxiomEditor::ClearRoads();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Clear all Roads (UNDOABLE with Ctrl+Z)");
        }
        
        if (ImGui::Button("Clear Districts")) {
            AxiomEditor::ClearDistricts();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Clear all Districts (UNDOABLE with Ctrl+Z)");
        }
        
        if (ImGui::Button("Clear Lots")) {
            AxiomEditor::ClearLots();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Clear all Lots (UNDOABLE with Ctrl+Z)");
        }
        
        if (ImGui::Button("Clear Buildings")) {
            AxiomEditor::ClearBuildings();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Clear all Buildings (UNDOABLE with Ctrl+Z)");
        }
        
        ImGui::Unindent();
    }

    // Dirty-layer propagation status (edit -> dirty downstream layers -> regenerate).
    const bool dirty_any = gs.dirty_layers.AnyDirty();
    same_line_if_room();
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
    same_line_if_room();
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
    struct DirtyChip {
        const char* label;
        DirtyLayer layer;
    };
    constexpr std::array<DirtyChip, 7> kDirtyChips = {{
        {"Axioms", DirtyLayer::Axioms},
        {"Tensor", DirtyLayer::Tensor},
        {"Roads", DirtyLayer::Roads},
        {"Districts", DirtyLayer::Districts},
        {"Lots", DirtyLayer::Lots},
        {"Buildings", DirtyLayer::Buildings},
        {"Viewport", DirtyLayer::ViewportIndex},
    }};
    ImGui::PushID("DirtyLayerChips");
    for (size_t chip_index = 0; chip_index < kDirtyChips.size(); ++chip_index) {
        const DirtyChip& chip = kDirtyChips[chip_index];
        ImGui::PushID(static_cast<int>(chip_index));
        draw_chip(chip.label, gs.dirty_layers.IsDirty(chip.layer));
        ImGui::PopID();
        if (chip_index + 1 < kDirtyChips.size()) {
            const float next_chip_width = ImGui::CalcTextSize(kDirtyChips[chip_index + 1].label).x + 30.0f;
            same_line_if_room(next_chip_width);
        }
    }
    ImGui::PopID();

    ImGui::SeparatorText("Debug Overlays");
    ImGui::Checkbox("Tensor Field Overlay", &gs.debug_show_tensor_overlay);
    same_line_if_room();
    ImGui::Checkbox("Height Field Overlay", &gs.debug_show_height_overlay);
    same_line_if_room();
    ImGui::Checkbox("Zone Field Overlay", &gs.debug_show_zone_overlay);
    same_line_if_room();
    ImGui::Checkbox("Validation Errors", &gs.validation_overlay.enabled);
    
    ImGui::SeparatorText("Layer Visibility");
    ImGui::Checkbox("Axioms##layer_visibility", &gs.show_layer_axioms);
    uiint.RegisterWidget({"checkbox", "Axioms", "toggle:layer.axioms", {"layer", "visibility"}});
    same_line_if_room();
    ImGui::Checkbox("Water##layer_visibility", &gs.show_layer_water);
    uiint.RegisterWidget({"checkbox", "Water", "toggle:layer.water", {"layer", "visibility"}});
    same_line_if_room();
    ImGui::Checkbox("Roads##layer_visibility", &gs.show_layer_roads);
    uiint.RegisterWidget({"checkbox", "Roads", "toggle:layer.roads", {"layer", "visibility"}});
    same_line_if_room();
    ImGui::Checkbox("Districts##layer_visibility", &gs.show_layer_districts);
    uiint.RegisterWidget({"checkbox", "Districts", "toggle:layer.districts", {"layer", "visibility"}});
    same_line_if_room();
    ImGui::Checkbox("Lots##layer_visibility", &gs.show_layer_lots);
    uiint.RegisterWidget({"checkbox", "Lots", "toggle:layer.lots", {"layer", "visibility"}});
    same_line_if_room();
    ImGui::Checkbox("Buildings##layer_visibility", &gs.show_layer_buildings);
    uiint.RegisterWidget({"checkbox", "Buildings", "toggle:layer.buildings", {"layer", "visibility"}});
    
    ImGui::PushTextWrapPos();
    ImGui::TextDisabled("Minimap: wheel=zoom, drag=pan, click=jump, L=pin/cycle, Shift+L=release auto, 1/2/3=set LOD, K=adaptive.");
    ImGui::PopTextWrapPos();

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
    same_line_if_room();
    ImGui::TextDisabled("%s", AxiomEditor::GetMinimapLODStatusText());

    ImGui::SeparatorText("Texture Editing (Phase 6)");
    if (gs.HasTextureSpace()) {
        static float brush_radius_m = 30.0f;
        static float brush_strength = 1.0f;
        static int zone_value = 1;
        static int material_value = 1;

        ImGui::SetNextItemWidth(120.0f);
        ImGui::DragFloat("Brush Radius (m)", &brush_radius_m, 1.0f, 1.0f, 250.0f, "%.1f");
        same_line_if_room();
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
        same_line_if_room();
        if (ImGui::Button("Lower Height @ Center")) {
            TerrainBrush::Stroke stroke{};
            stroke.world_center = brush_center;
            stroke.radius_meters = brush_radius_m;
            stroke.strength = brush_strength;
            stroke.mode = TerrainBrush::Mode::Lower;
            const bool applied = gs.ApplyTerrainBrush(stroke);
            (void)applied;
        }
        same_line_if_room();
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
        same_line_if_room();
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
        same_line_if_room();
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
        same_line_if_room();
        ImGui::TextDisabled("Tip: Generate/Regenerate also clears successful rebuild layers.");
    }

    same_line_if_room();
    bool live_preview = AxiomEditor::IsLivePreviewEnabled();
    if (ImGui::Checkbox("Live Preview", &live_preview)) {
        AxiomEditor::SetLivePreviewEnabled(live_preview);
    }
    uiint.RegisterWidget({"checkbox", "Live Preview", "preview.live", {"preview"}});

    same_line_if_room();
    float debounce = AxiomEditor::GetDebounceSeconds();
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::DragFloat("Debounce", &debounce, 0.01f, 0.0f, 1.5f, "%.2fs")) {
        AxiomEditor::SetDebounceSeconds(debounce);
    }
    uiint.RegisterWidget({"slider", "Debounce", "preview.debounce_sec", {"preview", "timing"}});

    same_line_if_room();
    if (ImGui::Button("Random Seed")) {
        AxiomEditor::RandomizeSeed();
        if (AxiomEditor::IsLivePreviewEnabled()) {
            AxiomEditor::ForceGenerate();
        }
    }
    uiint.RegisterWidget({"button", "Random Seed", "action:seed.randomize", {"action", "seed"}});
    uiint.RegisterAction({"seed.randomize", "Random Seed", "Tools", {}, "AxiomEditor::RandomizeSeed"});

    same_line_if_room();
    ImGui::Text("Seed: %u", AxiomEditor::GetSeed());

    // Status / errors
    const char* err = AxiomEditor::GetValidationError();
    if (err && err[0] != '\0') {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed), "%s", err);
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

    // RC-0.09-Test P1: Add scrollable region for long content 
    ImGui::BeginChild("ToolsScrollRegion", ImVec2(0, 0), false, 
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);
    
    DrawContent(dt);
    
    ImGui::EndChild();  // End scrollable region
    uiint.EndPanel();
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::Tools
