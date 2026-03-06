// FILE: rc_panel_tools.cpp
// PURPOSE: Bottom tools strip (viewport-centric cockpit controls).

#include "ui/panels/rc_panel_tools.h"
#include "RogueCity/App/Editor/CommandHistory.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/tools/rc_tool_dispatcher.h"

#include <RogueCity/Core/Editor/EditorState.hpp>
#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <imgui.h>
#include <optional>
#include <string>
namespace RC_UI::Panels::Tools {

void DrawContent(float dt) {
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();

  // === STATUS LINE (Mode/Filter/Panels) ===
  using namespace RogueCity::Core::Editor;
  EditorHFSM &hfsm = GetEditorHFSM();
  GlobalState &gs = GetGlobalState();

  // Determine active mode from HFSM state
  const char *mode_str = "IDLE";
  std::optional<ToolLibrary> active_lib = std::nullopt;
  switch (hfsm.state()) {
  case EditorState::Editing_Axioms:
  case EditorState::Viewport_PlaceAxiom:
    mode_str = "AXIOM";
    active_lib = ToolLibrary::Axiom;
    break;
  case EditorState::Editing_Water:
    mode_str = "WATER";
    active_lib = ToolLibrary::Water;
    break;
  case EditorState::Editing_Roads:
  case EditorState::Viewport_DrawRoad:
    mode_str = "ROAD";
    active_lib = ToolLibrary::Road;
    break;
  case EditorState::Editing_Districts:
    mode_str = "DISTRICT";
    active_lib = ToolLibrary::District;
    break;
  case EditorState::Editing_Lots:
    mode_str = "LOT";
    active_lib = ToolLibrary::Lot;
    break;
  case EditorState::Editing_Buildings:
    mode_str = "BUILDING";
    active_lib = ToolLibrary::Building;
    break;
  case EditorState::Simulating:
    mode_str = "SIM";
    break;
  default:
    break;
  }

  ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary),
                     "Mode: %s | Seed: %u", mode_str, AxiomEditor::GetSeed());
  ImGui::Spacing();

  // Restore iconified mode switching as the primary cockpit surface.
  if (Components::DrawSectionHeader("Tool Deck", UITokens::CyanAccent,
                                    /*default_open=*/true)) {
    ImGui::Indent();
    RC_UI::Panels::DrawContext ctx{gs, hfsm, uiint, nullptr, dt, false};
    Components::DrawToolLibrarySwitcher(ctx);
    ImGui::Unindent();
    ImGui::Spacing();
  }

  // Draw active sub-tool grid if in a tool-compatible mode
  if (active_lib.has_value() && active_lib != ToolLibrary::Axiom) {
    std::string section_label = std::string(mode_str) + " SUB-TOOLS";
    if (Components::DrawSectionHeader(section_label.c_str(),
                                      UITokens::CyanAccent,
                                      /*default_open=*/true)) {
      ImGui::Indent();
      // Construct temporary context for the shared component
      RC_UI::Panels::DrawContext ctx{gs, hfsm, uiint, nullptr, dt, false};
      Components::DrawToolActionGrid(*active_lib, ctx);
      ImGui::Unindent();
      ImGui::Spacing();
    }
  }

  // Axiom type library: shown in Axiom mode so users can select placement type.
  if (active_lib == ToolLibrary::Axiom) {
    if (Components::DrawSectionHeader("Axiom Library", UITokens::AmberGlow,
                                      /*default_open=*/true)) {
      ImGui::Indent();
      AxiomEditor::DrawAxiomLibraryContent();
      ImGui::Unindent();
      ImGui::Spacing();
    }
  }

  // Helper used by all sections — defined here so it stays in scope.
  const auto same_line_if_room = [](float min_remaining_width = 96.0f) {
    if (ImGui::GetContentRegionAvail().x > min_remaining_width) {
      ImGui::SameLine();
    }
  };

  if (Components::DrawSectionHeader("Actions", UITokens::CyanAccent)) {
    ImGui::Indent();

    // Undo / Redo (prefer global history, fallback to legacy axiom history)
    auto &global_history = RogueCity::App::GetEditorCommandHistory();
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
    uiint.RegisterWidget(
        {"button", "Undo", "action:editor.undo", {"action", "history"}});
    uiint.RegisterAction(
        {"editor.undo", "Undo", "Tools", {}, "AxiomEditor::Undo"});
    if (ImGui::IsItemHovered()) {
      const char *undo_label = AxiomEditor::GetUndoLabel();
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
    uiint.RegisterWidget(
        {"button", "Redo", "action:editor.redo", {"action", "history"}});
    uiint.RegisterAction(
        {"editor.redo", "Redo", "Tools", {}, "AxiomEditor::Redo"});
    if (ImGui::IsItemHovered()) {
      const char *redo_label = AxiomEditor::GetRedoLabel();
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
    uiint.RegisterWidget({"button",
                          "Generate / Regenerate",
                          "action:generator.regenerate",
                          {"action", "generator"}});
    uiint.RegisterAction({"generator.regenerate",
                          "Generate / Regenerate",
                          "Tools",
                          {},
                          "AxiomEditor::ForceGenerate"});
    if (!can_generate) {
      ImGui::EndDisabled();
    }

    // === Clear Operations (Undoable) ===
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Clear All Data button (warning style)
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImGui::ColorConvertU32ToFloat4(0xFF3A1A1A));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImGui::ColorConvertU32ToFloat4(0xFF5A2A2A));

    static bool show_clear_all_confirm = false;
    if (ImGui::Button("Clear All Data")) {
      show_clear_all_confirm = true;
    }
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(
          "Clear ALL layers (Axioms, Water, Roads, Districts, Lots, "
          "Buildings)\nThis action is UNDOABLE with Ctrl+Z");
    }

    // Confirmation modal
    if (show_clear_all_confirm) {
      ImGui::OpenPopup("Confirm Clear All");
      show_clear_all_confirm = false;
    }

    if (ImGui::BeginPopupModal("Confirm Clear All", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed),
                         "WARNING:");
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

    ImGui::Unindent();
    ImGui::Spacing();
  } // Editor Tools section

  ImGui::Spacing();
  ImGui::TextDisabled("Layer, Dirty, Debug overlays, and Minimap LOD controls moved to Inspector/System Map.");

  ImGui::Spacing();
  ImGui::TextDisabled(
      "Texture authoring moved to dedicated Texture Editing panel.");
  ImGui::TextDisabled(
      "Texture size/policy/live preview/seed controls moved to Tool Deck.");
}

void Draw(float dt) {
  static RC_UI::DockableWindowState s_tools_window;
  if (!RC_UI::BeginDockableWindow("Tools", s_tools_window, "Bottom",
                                  ImGuiWindowFlags_NoCollapse)) {
    return;
  }

  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{"Tools",
                                  "Tools",
                                  "toolbox",
                                  "Bottom",
                                  "visualizer/src/ui/panels/rc_panel_tools.cpp",
                                  {"generation", "controls", "cockpit"}},
      true);

  // RC-0.09-Test P1: Add scrollable region for long content
  ImGui::BeginChild("ToolsScrollRegion", ImVec2(0, 0), false,
                    ImGuiWindowFlags_AlwaysVerticalScrollbar);

  DrawContent(dt);

  ImGui::EndChild(); // End scrollable region
  uiint.EndPanel();
  RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::Tools
