// FILE: rc_ui_root.h
// PURPOSE: Root-level entry points for the procedural editor UI.
// todo phase 2 - refactor to dynamic panel management system with factory
// registration for better modularity and decoupling. This file should serve as
// the main entry point for rendering the UI each frame, managing the dockspace,
// and providing shared utilities for panel management and workspace presets,
// while individual panels and components should be defined in their own headers
// and source files to maintain separation of concerns and avoid ODR violations.

#pragma once

#include "ui/panels/IPanelDrawer.h"
#include "ui/rc_ui_input_gate.h"
#include <array>
#include <imgui.h>
#include <span>
#include <string>
#include <vector>

namespace RogueCity::App {
class MinimapViewport;
}

namespace RC_UI {

// Render the full UI frame, including the dockspace and all panels. Pass in
// delta time for animations.
void DrawRoot(float dt);

// Apply the custom UI theme (colors, rounding). Should be called once before
// drawing begins.
void ApplyTheme();

namespace Keymap {

struct ShortcutBinding {
  ImGuiKey key{ImGuiKey_None};
  bool ctrl{false};
  bool shift{false};
  bool alt{false};
  bool super{false};
};

struct ShortcutActionInfo {
  const char *id;
  const char *category;
  const char *label;
};

namespace Action {
inline constexpr char kUndo[] = "edit.undo";
inline constexpr char kRedo[] = "edit.redo";
inline constexpr char kRedoShiftZ[] = "edit.redo_shift_z";
inline constexpr char kSaveLayout[] = "file.save_layout";
inline constexpr char kNewCity[] = "file.new_city";
inline constexpr char kQuit[] = "file.quit";
inline constexpr char kCommandCancel[] = "command.cancel";
inline constexpr char kMasterSearchOpen[] = "master.search_open";
inline constexpr char kMasterSearchNext[] = "master.search_next";
inline constexpr char kMasterSearchPrev[] = "master.search_prev";
inline constexpr char kMasterSearchClose[] = "master.search_close";
inline constexpr char kViewportOpenSmartList[] = "viewport.open_smart_list";
inline constexpr char kViewportOpenPieMenu[] = "viewport.open_pie_menu";
inline constexpr char kViewportOpenPalettePrimary[] =
    "viewport.open_palette_primary";
inline constexpr char kViewportOpenPaletteAlt[] = "viewport.open_palette_alt";
inline constexpr char kViewportDomainHoldA[] = "viewport.domain_hold_axiom";
inline constexpr char kViewportDomainHoldW[] = "viewport.domain_hold_water";
inline constexpr char kViewportDomainHoldR[] = "viewport.domain_hold_road";
inline constexpr char kViewportDomainHoldD[] = "viewport.domain_hold_district";
inline constexpr char kViewportDomainHoldL[] = "viewport.domain_hold_lot";
inline constexpr char kViewportDomainHoldB[] = "viewport.domain_hold_building";
inline constexpr char kViewportSelectAuto[] = "viewport.select_auto";
inline constexpr char kViewportGizmoTranslate[] = "viewport.gizmo_translate";
inline constexpr char kViewportGizmoTranslateAlt[] =
    "viewport.gizmo_translate_alt";
inline constexpr char kViewportGizmoRotate[] = "viewport.gizmo_rotate";
inline constexpr char kViewportGizmoScale[] = "viewport.gizmo_scale";
inline constexpr char kViewportGizmoScaleAlt[] = "viewport.gizmo_scale_alt";
inline constexpr char kViewportGizmoSnapToggle[] = "viewport.gizmo_snap_toggle";
inline constexpr char kViewportLayer0[] = "viewport.layer_0";
inline constexpr char kViewportLayer1[] = "viewport.layer_1";
inline constexpr char kViewportLayer2[] = "viewport.layer_2";
inline constexpr char kViewportDeleteSelected[] = "viewport.delete_selected";
inline constexpr char kViewportDeleteSelectedAlt[] =
    "viewport.delete_selected_alt";
inline constexpr char kViewportToolPaletteToggle[] =
    "viewport.tool_palette_toggle";
inline constexpr char kViewportQuickRect[] = "viewport.quick_rect_select";
inline constexpr char kViewportQuickLasso[] = "viewport.quick_lasso_select";
inline constexpr char kViewportQuickMove[] = "viewport.quick_move_nodes";
inline constexpr char kViewportQuickHandle[] = "viewport.quick_handle_move";
inline constexpr char kMinimapLodCycle[] = "minimap.lod_cycle";
inline constexpr char kMinimapLodAuto[] = "minimap.lod_auto";
inline constexpr char kMinimapLod0[] = "minimap.lod_0";
inline constexpr char kMinimapLod1[] = "minimap.lod_1";
inline constexpr char kMinimapLod2[] = "minimap.lod_2";
inline constexpr char kMinimapAdaptiveToggle[] = "minimap.adaptive_toggle";
inline constexpr char kMinimapToggleVisible[] = "minimap.toggle_visible";
inline constexpr char kMinimapToggleSearch[] = "minimap.toggle_search";
inline constexpr char kToggleBottomPanel[] = "view.toggle_bottom_panel";
inline constexpr char kToggleLeftPanel[] = "view.toggle_left_panel";
inline constexpr char kToggleRightPanel[] = "view.toggle_right_panel";
inline constexpr char kDockModeModifier[] = "view.dock_mode_modifier";
inline constexpr char kToggleActivityBarLeft[] = "view.toggle_activity_left";
inline constexpr char kToggleActivityBarRight[] = "view.toggle_activity_right";
inline constexpr char kToggleDevShell[] = "terminal.toggle_dev_shell";
inline constexpr char kSoftResetLayout[] = "debug.reset_layout";
inline constexpr char kHardResetLayout[] = "debug.reset_hard";
inline constexpr char kRedoAlt[] = "debug.redo_alt";
// Ruler overlay: L = toggle, Shift+L = crosshair, Ctrl+L = div-snap
inline constexpr char kRulerToggle[]    = "view.ruler_toggle";
inline constexpr char kRulerCrosshair[] = "view.ruler_crosshair";
inline constexpr char kRulerDivMode[]   = "view.ruler_div_mode";
} // namespace Action

void EnsureLoaded();
[[nodiscard]] bool IsPressed(const char *action_id, bool repeat = false);
[[nodiscard]] bool IsDown(const char *action_id);
[[nodiscard]] const char *ShortcutLabel(const char *action_id);
[[nodiscard]] ShortcutBinding GetBinding(const char *action_id);
bool SetBinding(const char *action_id, const ShortcutBinding &binding,
                std::string *error = nullptr);
bool ResetBinding(const char *action_id, std::string *error = nullptr);
bool ResetAll(std::string *error = nullptr);
[[nodiscard]] std::span<const ShortcutActionInfo> GetActions();
[[nodiscard]] bool IsEditorOpen();
[[nodiscard]] const char *KeymapPath();

} // namespace Keymap

enum class ToolLibrary { Axiom, Water, Road, District, Lot, Building };
using ToolLibraryIconRenderer =
    void (*)(ImDrawList *draw_list, ToolLibrary tool, const ImVec2 &center,
             float size, ImU32 color);

// Canonical tool palette (5 color families, with per-tool variation support).
[[nodiscard]] ImU32 ToolColor(ToolLibrary tool, int variation = 0);
[[nodiscard]] ImU32 ToolColorActive(ToolLibrary tool, int variation = 0);
[[nodiscard]] ImU32 ToolColorMuted(ToolLibrary tool, int variation = 0);

// ---[BEGIN: NEW TYPE
// SCHEMA]--------------------------------------------------- WHY: Decouple
// layout and docking logic from individual panel Draw() calls. WHERE:
// rc_ui_root.h

using Panels::PanelType;

// A layout entry that maps a panel type to a window name and dock area.
struct PanelLayout {
  PanelType type;
  std::string window_name;
  std::string dock_area;
  bool is_index_like = false;
};

// A helper that lets a button control a docked or floating panel instance.
struct ButtonDockedPanel {
  PanelType type;
  std::string window_name;
  std::string dock_area;
  bool m_open = false;
  bool m_docked = true; // true => docked; false => floating
};
// ---[END: NEW TYPE SCHEMA]----------------------------------------------------

inline constexpr std::array<ToolLibrary, 6> kToolLibraryOrder = {
    ToolLibrary::Axiom,    ToolLibrary::Water, ToolLibrary::Road,
    ToolLibrary::District, ToolLibrary::Lot,   ToolLibrary::Building};

[[nodiscard]] bool IsToolLibraryOpen(ToolLibrary tool);
void ToggleToolLibrary(ToolLibrary tool);
void ActivateToolLibrary(ToolLibrary tool);
void PopoutToolLibrary(ToolLibrary tool);
[[nodiscard]] bool IsToolLibraryPopoutOpen(ToolLibrary tool);
void SetToolLibraryIconRenderer(ToolLibrary tool,
                                ToolLibraryIconRenderer renderer);
void ClearToolLibraryIconRenderer(ToolLibrary tool);
void ClearAllToolLibraryIconRenderers();

void ApplyUnifiedWindowSchema(const ImVec2 &baseSize = ImVec2(540.0f, 720.0f),
                              float padding = 16.0f);
void PopUnifiedWindowSchema();
void BeginUnifiedTextWrap(float padding = 16.0f);
void EndUnifiedTextWrap();
bool BeginWindowContainer(const char *id = "##window_container",
                          ImGuiWindowFlags flags = 0);
void EndWindowContainer();

struct DockLayoutPreferences {
  float left_panel_ratio = 0.32f;
  float right_panel_ratio = 0.22f;
  float tool_deck_ratio = 0.24f;
};

enum class DockTreeProfile {
  Adaptive = 0,
  StandardThreeColumn,
  WideCenter,
  FocusViewport
};

[[nodiscard]] DockLayoutPreferences GetDockLayoutPreferences();
[[nodiscard]] DockLayoutPreferences GetDefaultDockLayoutPreferences();
void SetDockLayoutPreferences(const DockLayoutPreferences &preferences);
[[nodiscard]] DockTreeProfile GetDockTreeProfile();
void SetDockTreeProfile(DockTreeProfile profile);

// Axiom Deck -> Axiom Library toggle.
[[nodiscard]] bool IsAxiomLibraryOpen();
void ToggleAxiomLibrary();

// Get minimap viewport for camera sync (Phase 5: Polish)
RogueCity::App::MinimapViewport *GetMinimapViewport();
void SetMinimapStandaloneWindowEnabled(bool enabled);
[[nodiscard]] bool IsMinimapStandaloneWindowEnabled();

// Queue a dock reassignment request for an existing panel/window.
bool QueueDockWindow(const char *windowName, const char *dockArea,
                     bool ownDockNode = false);

// System Map toggle.
[[nodiscard]] bool IsSystemMapOpen();
void ToggleSystemMap();
void SetSystemMapOpen(bool open);

// Reset dock layout to default (useful if user messes up docking)
void ResetDockLayout();

// Workspace preset persistence (layout + docking state across runs).
bool SaveWorkspacePreset(const char *presetName, std::string *error = nullptr);
bool LoadWorkspacePreset(const char *presetName, std::string *error = nullptr);
std::vector<std::string> ListWorkspacePresets();

struct WorkspacePresetMetadata {
  std::string name{};
  bool has_theme{false};
  std::string theme_name{};
  std::string saved_at_utc{};
  bool has_viewport_size{false};
  ImVec2 viewport_size{0.0f, 0.0f};
  int monitor_count{1};
  bool docking_enabled{true};
  bool multi_viewport_enabled{false};
};

std::vector<WorkspacePresetMetadata> ListWorkspacePresetMetadata();

// Track last docked area for windows (for re-docking on close).
void NotifyDockedWindow(const char *windowName, const char *dockArea);
void ReturnWindowToLastDock(const char *windowName, const char *fallbackArea);

struct DockableWindowState {
  bool open = true;
  bool was_docked = true;
};

// Begin a dockable window with unified behavior: X only when undocked, re-dock
// on close.
bool BeginDockableWindow(const char *windowName, DockableWindowState &state,
                         const char *fallbackDockArea,
                         ImGuiWindowFlags flags = 0);

void EndDockableWindow();

// Publish/inspect per-frame UI input arbitration for viewport actions.
void PublishUiInputGateState(const UiInputGateState &state);
[[nodiscard]] const UiInputGateState &GetUiInputGateState();

// ---[BEGIN: NEW DRAW
// API]------------------------------------------------------
struct IndicesTabs {
  bool district = true;
  bool road = true;
  bool lot = true;
  bool river = true;
  bool building = true;
};

void DrawPanelByType(PanelType type, float dt, std::string_view window_name);
void DrawButtonDockedPanel(ButtonDockedPanel &panel, float dt);
void DrawIndicesPanel(IndicesTabs &tabs, float dt);
// ---[END: NEW DRAW
// API]--------------------------------------------------------

} // namespace RC_UI
