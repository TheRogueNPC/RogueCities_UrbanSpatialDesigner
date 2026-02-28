// FILE: rc_ui_root.cpp - Editor window routing with hardened docking

#include "ui/rc_ui_root.h"
#include "RogueCity/App/Editor/CommandHistory.hpp"
#include "RogueCity/App/Viewports/MinimapViewport.hpp" // NEW: Minimap integration
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/panels/rc_panel_ai_console.h" // NEW: AI bridge control
#include "ui/panels/rc_panel_axiom_bar.h"
#include "ui/panels/rc_panel_axiom_editor.h" // NEW: Integrated axiom editor
#include "ui/panels/rc_panel_building_control.h" // AI_INTEGRATION_TAG: V1_PASS1_TASK4_PANEL_WIRING
#include "ui/panels/rc_panel_building_index.h" // NEW: Building index (RC-0.10)
#include "ui/panels/rc_panel_city_spec.h" // NEW: CitySpec generator (Phase 3)
#include "ui/panels/rc_panel_dev_shell.h"
#include "ui/panels/rc_panel_district_index.h"
#include "ui/panels/rc_panel_inspector.h"
#include "ui/panels/rc_panel_log.h"
#include "ui/panels/rc_panel_lot_control.h" // AI_INTEGRATION_TAG: V1_PASS1_TASK4_PANEL_WIRING
#include "ui/panels/rc_panel_lot_index.h"
#include "ui/panels/rc_panel_river_index.h"
#include "ui/panels/rc_panel_road_index.h"
#include "ui/panels/rc_panel_system_map.h"
#include "ui/panels/rc_panel_telemetry.h"
#include "ui/panels/rc_panel_tools.h"
#include "ui/panels/rc_panel_ui_agent.h" // NEW: UI Agent assistant (Phase 2)
#include "ui/panels/rc_panel_validation.h"
#include "ui/panels/rc_panel_water_control.h" // AI_INTEGRATION_TAG: V1_PASS1_TASK4_PANEL_WIRING
#include "ui/panels/rc_panel_zoning_control.h" // NEW: Zoning control (RC-0.10)
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_input_gate.h"
#include "ui/rc_ui_responsive.h"
#include "ui/rc_ui_theme.h"
#include "ui/rc_ui_tokens.h"
#include "ui/rc_ui_viewport_config.h"
#include "ui/tools/rc_tool_contract.h"
#include "ui/tools/rc_tool_dispatcher.h"

// MASTER PANEL ARCHITECTURE (RC-0.10)
#include "ui/panels/PanelRegistry.h"
#include "ui/panels/RcMasterPanel.h"

#include "RogueCity/App/UI/ThemeManager.h"
#include "ui/panels/rc_panel_workspace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <imgui.h>
#include <imgui_internal.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RC_UI {

// MASTER PANEL SYSTEM (RC-0.10 architecture refactor)
// Replaces individual panel windows with unified container + drawer registry
namespace {
static std::unique_ptr<RC_UI::Panels::RcMasterPanel> s_master_panel;
static bool s_registry_initialized = false;

// DEPRECATED: Old AI panel instances (will be removed after full drawer
// conversion) static RogueCity::UI::AiConsolePanel s_ai_console_instance;
// static RogueCity::UI::UiAgentPanel s_ui_agent_instance;
// static RogueCity::UI::CitySpecPanel s_city_spec_instance;

struct DockRequest {
  std::string window_name;
  std::string dock_area;
  bool own_dock_node = false;
};

struct DockLayoutNodes {
  ImGuiID root = 0;
  ImGuiID left = 0;
  ImGuiID right = 0;
  ImGuiID tool_deck = 0;
  ImGuiID library = 0;
  ImGuiID center = 0;
  ImGuiID bottom = 0;
  ImGuiID bottom_tabs = 0;
};

static DockLayoutNodes s_dock_nodes{};
static std::vector<DockRequest> s_pending_dock_requests;
static std::unordered_map<std::string, std::string> s_last_dock_area;
static constexpr size_t kMaxPendingDockRequests = 128u;
static constexpr const char *kWorkspacePresetFile =
    "AI/docs/ui/workspace_presets.json";

struct SharedLibraryFrameState {
  bool initialized = false;
  ImVec2 pos = ImVec2(0.0f, 0.0f);
  ImVec2 size = ImVec2(0.0f, 0.0f);
};

static SharedLibraryFrameState s_library_frame_state{};

static IndicesTabs s_indices_tabs{
    .district = true,
    .road = true,
    .lot = true,
    .river = true,
    .building = true,
};

// Schema-2 preset entry: docking ini blob + theme snapshot.
// Schema-1 files (plain string values) are still accepted on load.
struct PresetEntry {
  std::string ini;
  RogueCity::UI::ThemeProfile theme;
  bool has_theme = false; // false for entries loaded from schema-1 files
};

struct WorkspacePresetStore {
  std::unordered_map<std::string, PresetEntry> presets;
};

[[nodiscard]] static std::filesystem::path WorkspacePresetPath() {
  return std::filesystem::path(kWorkspacePresetFile);
}

// Serialize a ThemeProfile to a JSON object (matches ThemeManager field names).
static nlohmann::json ThemeProfileToPresetJson(const RogueCity::UI::ThemeProfile &t) {
  nlohmann::json j;
  j["name"]            = t.name;
  j["primary_accent"]  = t.primary_accent;
  j["secondary_accent"]= t.secondary_accent;
  j["success_color"]   = t.success_color;
  j["warning_color"]   = t.warning_color;
  j["error_color"]     = t.error_color;
  j["background_dark"] = t.background_dark;
  j["panel_background"]= t.panel_background;
  j["grid_overlay"]    = t.grid_overlay;
  j["text_primary"]    = t.text_primary;
  j["text_secondary"]  = t.text_secondary;
  j["text_disabled"]   = t.text_disabled;
  j["border_accent"]   = t.border_accent;
  return j;
}

static RogueCity::UI::ThemeProfile ThemeProfileFromPresetJson(const nlohmann::json &j) {
  RogueCity::UI::ThemeProfile t;
  t.name             = j.value("name", "Custom");
  t.primary_accent   = j.value("primary_accent",   ImU32{0});
  t.secondary_accent = j.value("secondary_accent", ImU32{0});
  t.success_color    = j.value("success_color",    ImU32{0});
  t.warning_color    = j.value("warning_color",    ImU32{0});
  t.error_color      = j.value("error_color",      ImU32{0});
  t.background_dark  = j.value("background_dark",  ImU32{0});
  t.panel_background = j.value("panel_background", ImU32{0});
  t.grid_overlay     = j.value("grid_overlay",     ImU32{0});
  t.text_primary     = j.value("text_primary",     ImU32{0xFFFFFFFF});
  t.text_secondary   = j.value("text_secondary",   ImU32{0xFFAAAAAA});
  t.text_disabled    = j.value("text_disabled",    ImU32{0xFF666666});
  t.border_accent    = j.value("border_accent",    ImU32{0});
  return t;
}

static void LoadWorkspacePresetStore(WorkspacePresetStore &store) {
  store.presets.clear();
  const std::filesystem::path path = WorkspacePresetPath();
  if (!std::filesystem::exists(path)) {
    return;
  }

  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    return;
  }

  try {
    nlohmann::json j;
    in >> j;
    if (!j.contains("presets") || !j["presets"].is_object()) {
      return;
    }
    const int schema = j.value("schema", 1);
    for (const auto &item : j["presets"].items()) {
      PresetEntry entry;
      if (schema >= 2 && item.value().is_object()) {
        // Schema 2: { "ini": "...", "theme": { ... } }
        entry.ini = item.value().value("ini", std::string{});
        if (item.value().contains("theme") && item.value()["theme"].is_object()) {
          entry.theme     = ThemeProfileFromPresetJson(item.value()["theme"]);
          entry.has_theme = true;
        }
      } else if (item.value().is_string()) {
        // Schema 1 fallback: plain ini blob
        entry.ini = item.value().get<std::string>();
      }
      if (!entry.ini.empty()) {
        store.presets[item.key()] = std::move(entry);
      }
    }
  } catch (...) {
    // Keep editor running even if preset file is malformed.
  }
}

static bool SaveWorkspacePresetStore(const WorkspacePresetStore &store,
                                     std::string *error) {
  try {
    const std::filesystem::path path = WorkspacePresetPath();
    std::filesystem::create_directories(path.parent_path());

    nlohmann::json j;
    j["schema"] = 2;
    j["presets"] = nlohmann::json::object();
    for (const auto &[name, entry] : store.presets) {
      nlohmann::json e;
      e["ini"] = entry.ini;
      if (entry.has_theme) {
        e["theme"] = ThemeProfileToPresetJson(entry.theme);
      }
      j["presets"][name] = e;
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      if (error != nullptr) {
        *error = "Failed to open preset store for writing: " + path.string();
      }
      return false;
    }
    out << j.dump(2);
    return true;
  } catch (const std::exception &e) {
    if (error != nullptr) {
      *error = e.what();
    }
    return false;
  }
}
} // namespace

static std::array<bool, kToolLibraryOrder.size()> s_tool_library_open = {};
static std::array<bool, kToolLibraryOrder.size()> s_tool_library_popout = {};
static ToolLibrary s_active_library_tool = ToolLibrary::Water;
static bool s_library_frame_open = false;
static DockLayoutPreferences s_dock_layout_preferences =
    GetDefaultDockLayoutPreferences();
static UiInputGateState s_last_input_gate{};

// Utility for drawing icons for the tool library entries.
// TODO: Phase 2 - Refactor to use a more flexible icon system, potentially with
// support for custom icons per tool and theming.
static void DrawToolLibraryIcon(ImDrawList *draw_list, ToolLibrary tool,
                                const ImVec2 &center, float size) {
  const ImU32 color = UITokens::TextPrimary;
  const float half = size * 0.5f;
  switch (tool) {
  case ToolLibrary::Water:
    draw_list->AddTriangleFilled(ImVec2(center.x, center.y - half),
                                 ImVec2(center.x - half, center.y + half),
                                 ImVec2(center.x + half, center.y + half),
                                 color);
    break;
  case ToolLibrary::Road:
    draw_list->AddLine(ImVec2(center.x - half, center.y + half),
                       ImVec2(center.x + half, center.y - half), color, 2.5f);
    break;
  case ToolLibrary::District:
    draw_list->AddRect(ImVec2(center.x - half, center.y - half),
                       ImVec2(center.x + half, center.y + half), color, 3.0f, 0,
                       2.0f);
    break;
  case ToolLibrary::Lot:
    draw_list->AddRect(ImVec2(center.x - half, center.y - half),
                       ImVec2(center.x + half, center.y + half), color, 0.0f, 0,
                       2.0f);
    draw_list->AddLine(ImVec2(center.x, center.y - half),
                       ImVec2(center.x, center.y + half), color, 1.5f);
    draw_list->AddLine(ImVec2(center.x - half, center.y),
                       ImVec2(center.x + half, center.y), color, 1.5f);
    break;
  case ToolLibrary::Building:
    draw_list->AddRectFilled(ImVec2(center.x - half, center.y - half),
                             ImVec2(center.x + half, center.y + half), color,
                             2.0f);
    draw_list->AddRect(ImVec2(center.x - half, center.y - half),
                       ImVec2(center.x + half, center.y + half), color, 2.0f, 0,
                       2.0f);
    break;
  case ToolLibrary::Axiom:
    draw_list->AddCircle(center, half, color, 12, 2.0f);
    draw_list->AddCircleFilled(center, half * 0.35f, color, 12);
    break;
  }
}

[[nodiscard]] static bool
IsToolActionActive(const Tools::ToolActionSpec &action,
                   const RogueCity::Core::Editor::GlobalState &gs) {
  using RogueCity::Core::Editor::BuildingSubtool;
  using RogueCity::Core::Editor::DistrictSubtool;
  using RogueCity::Core::Editor::LotSubtool;
  using RogueCity::Core::Editor::RoadSplineSubtool;
  using RogueCity::Core::Editor::RoadSubtool;
  using RogueCity::Core::Editor::WaterSplineSubtool;
  using RogueCity::Core::Editor::WaterSubtool;
  using Tools::ToolActionId;

  switch (action.id) {
  case ToolActionId::Axiom_Organic:
  case ToolActionId::Axiom_Grid:
  case ToolActionId::Axiom_Radial:
  case ToolActionId::Axiom_Hexagonal:
  case ToolActionId::Axiom_Stem:
  case ToolActionId::Axiom_LooseGrid:
  case ToolActionId::Axiom_Suburban:
  case ToolActionId::Axiom_Superblock:
  case ToolActionId::Axiom_Linear:
  case ToolActionId::Axiom_GridCorrective:
    return gs.tool_runtime.active_domain ==
               RogueCity::Core::Editor::ToolDomain::Axiom &&
           gs.tool_runtime.last_action_id == Tools::ToolActionName(action.id);

  case ToolActionId::Water_Flow:
    return gs.tool_runtime.water_subtool == WaterSubtool::Flow;
  case ToolActionId::Water_Contour:
    return gs.tool_runtime.water_subtool == WaterSubtool::Contour;
  case ToolActionId::Water_Erode:
    return gs.tool_runtime.water_subtool == WaterSubtool::Erode;
  case ToolActionId::Water_Select:
    return gs.tool_runtime.water_subtool == WaterSubtool::Select;
  case ToolActionId::Water_Mask:
    return gs.tool_runtime.water_subtool == WaterSubtool::Mask;
  case ToolActionId::Water_Inspect:
    return gs.tool_runtime.water_subtool == WaterSubtool::Inspect;
  case ToolActionId::WaterSpline_Selection:
    return gs.tool_runtime.water_spline_subtool ==
           WaterSplineSubtool::Selection;
  case ToolActionId::WaterSpline_DirectSelect:
    return gs.tool_runtime.water_spline_subtool ==
           WaterSplineSubtool::DirectSelect;
  case ToolActionId::WaterSpline_Pen:
    return gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Pen;
  case ToolActionId::WaterSpline_ConvertAnchor:
    return gs.tool_runtime.water_spline_subtool ==
           WaterSplineSubtool::ConvertAnchor;
  case ToolActionId::WaterSpline_AddRemoveAnchor:
    return gs.tool_runtime.water_spline_subtool ==
           WaterSplineSubtool::AddRemoveAnchor;
  case ToolActionId::WaterSpline_HandleTangents:
    return gs.tool_runtime.water_spline_subtool ==
           WaterSplineSubtool::HandleTangents;
  case ToolActionId::WaterSpline_SnapAlign:
    return gs.tool_runtime.water_spline_subtool ==
           WaterSplineSubtool::SnapAlign;
  case ToolActionId::WaterSpline_JoinSplit:
    return gs.tool_runtime.water_spline_subtool ==
           WaterSplineSubtool::JoinSplit;
  case ToolActionId::WaterSpline_Simplify:
    return gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Simplify;

  case ToolActionId::Road_Spline:
    return gs.tool_runtime.road_subtool == RoadSubtool::Spline;
  case ToolActionId::Road_Grid:
    return gs.tool_runtime.road_subtool == RoadSubtool::Grid;
  case ToolActionId::Road_Bridge:
    return gs.tool_runtime.road_subtool == RoadSubtool::Bridge;
  case ToolActionId::Road_Select:
    return gs.tool_runtime.road_subtool == RoadSubtool::Select;
  case ToolActionId::Road_Disconnect:
    return gs.tool_runtime.road_subtool == RoadSubtool::Disconnect;
  case ToolActionId::Road_Stub:
    return gs.tool_runtime.road_subtool == RoadSubtool::Stub;
  case ToolActionId::Road_Curve:
    return gs.tool_runtime.road_subtool == RoadSubtool::Curve;
  case ToolActionId::Road_Strengthen:
    return gs.tool_runtime.road_subtool == RoadSubtool::Strengthen;
  case ToolActionId::Road_Inspect:
    return gs.tool_runtime.road_subtool == RoadSubtool::Inspect;
  case ToolActionId::RoadSpline_Selection:
    return gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::Selection;
  case ToolActionId::RoadSpline_DirectSelect:
    return gs.tool_runtime.road_spline_subtool ==
           RoadSplineSubtool::DirectSelect;
  case ToolActionId::RoadSpline_Pen:
    return gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::Pen;
  case ToolActionId::RoadSpline_ConvertAnchor:
    return gs.tool_runtime.road_spline_subtool ==
           RoadSplineSubtool::ConvertAnchor;
  case ToolActionId::RoadSpline_AddRemoveAnchor:
    return gs.tool_runtime.road_spline_subtool ==
           RoadSplineSubtool::AddRemoveAnchor;
  case ToolActionId::RoadSpline_HandleTangents:
    return gs.tool_runtime.road_spline_subtool ==
           RoadSplineSubtool::HandleTangents;
  case ToolActionId::RoadSpline_SnapAlign:
    return gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::SnapAlign;
  case ToolActionId::RoadSpline_JoinSplit:
    return gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::JoinSplit;
  case ToolActionId::RoadSpline_Simplify:
    return gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::Simplify;

  case ToolActionId::District_Zone:
    return gs.tool_runtime.district_subtool == DistrictSubtool::Zone;
  case ToolActionId::District_Paint:
    return gs.tool_runtime.district_subtool == DistrictSubtool::Paint;
  case ToolActionId::District_Split:
    return gs.tool_runtime.district_subtool == DistrictSubtool::Split;
  case ToolActionId::District_Select:
    return gs.tool_runtime.district_subtool == DistrictSubtool::Select;
  case ToolActionId::District_Merge:
    return gs.tool_runtime.district_subtool == DistrictSubtool::Merge;
  case ToolActionId::District_Inspect:
    return gs.tool_runtime.district_subtool == DistrictSubtool::Inspect;

  case ToolActionId::Lot_Plot:
    return gs.tool_runtime.lot_subtool == LotSubtool::Plot;
  case ToolActionId::Lot_Slice:
    return gs.tool_runtime.lot_subtool == LotSubtool::Slice;
  case ToolActionId::Lot_Align:
    return gs.tool_runtime.lot_subtool == LotSubtool::Align;
  case ToolActionId::Lot_Select:
    return gs.tool_runtime.lot_subtool == LotSubtool::Select;
  case ToolActionId::Lot_Merge:
    return gs.tool_runtime.lot_subtool == LotSubtool::Merge;
  case ToolActionId::Lot_Inspect:
    return gs.tool_runtime.lot_subtool == LotSubtool::Inspect;

  case ToolActionId::Building_Place:
    return gs.tool_runtime.building_subtool == BuildingSubtool::Place;
  case ToolActionId::Building_Scale:
    return gs.tool_runtime.building_subtool == BuildingSubtool::Scale;
  case ToolActionId::Building_Rotate:
    return gs.tool_runtime.building_subtool == BuildingSubtool::Rotate;
  case ToolActionId::Building_Select:
    return gs.tool_runtime.building_subtool == BuildingSubtool::Select;
  case ToolActionId::Building_Assign:
    return gs.tool_runtime.building_subtool == BuildingSubtool::Assign;
  case ToolActionId::Building_Inspect:
    return gs.tool_runtime.building_subtool == BuildingSubtool::Inspect;

  case ToolActionId::Future_FloorPlan:
  case ToolActionId::Future_Paths:
  case ToolActionId::Future_Flow:
  case ToolActionId::Future_Furnature:
  case ToolActionId::Count:
    return false;
  }

  return false;
}

using ToolLibraryContentRenderer = std::function<void()>;
static const char *ToolLibraryWindowName(ToolLibrary tool);

static void RenderToolLibraryWindow(
    ToolLibrary tool, const char *window_name, const char *owner_module,
    const char *dock_area, std::span<const Tools::ToolActionSpec> actions,
    bool popout_instance = false, bool *popout_open = nullptr,
    const ToolLibraryContentRenderer &content_renderer = {}) {
  if (popout_instance) {
    if (popout_open == nullptr || !*popout_open) {
      return;
    }
  } else if (!IsToolLibraryOpen(tool)) {
    return;
  }

#if !defined(IMGUI_HAS_DOCK)
  if (!popout_instance) {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    if (viewport != nullptr) {
      if (!s_library_frame_state.initialized) {
        const ImVec2 default_size(std::max(280.0f, viewport->Size.x * 0.22f),
                                  std::max(360.0f, viewport->Size.y * 0.64f));
        const ImVec2 default_pos(viewport->Pos.x + viewport->Size.x -
                                     default_size.x - 16.0f,
                                 viewport->Pos.y + 64.0f);
        ImGui::SetNextWindowPos(default_pos, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(default_size, ImGuiCond_FirstUseEver);
      } else {
        ImGui::SetNextWindowPos(s_library_frame_state.pos, ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(s_library_frame_state.size,
                                 ImGuiCond_Appearing);
      }
    }
  } else if (s_library_frame_state.initialized) {
    ImGui::SetNextWindowPos(ImVec2(s_library_frame_state.pos.x + 28.0f,
                                   s_library_frame_state.pos.y + 28.0f),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(s_library_frame_state.size,
                             ImGuiCond_FirstUseEver);
  }
#endif

  bool *window_open_state =
      popout_instance ? popout_open : &s_library_frame_open;
  const bool open = Components::BeginTokenPanel(
      window_name, UITokens::CyanAccent, window_open_state,
      ImGuiWindowFlags_NoCollapse);
  const bool window_requested_open =
      (window_open_state == nullptr) ? true : *window_open_state;

  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  uiint.BeginPanel(RogueCity::UIInt::PanelMeta{window_name,
                                               window_name,
                                               "toolbox",
                                               dock_area,
                                               owner_module,
                                               {"tool", "library"}},
                   open);

  bool redock_popout = false;
  if (popout_instance) {
    if (!window_requested_open) {
      redock_popout = true;
    } else if (open && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
      const ImVec2 mouse_pos = ImGui::GetMousePos();
      const ImVec2 window_pos = ImGui::GetWindowPos();
      const ImVec2 window_size = ImGui::GetWindowSize();
      const float title_bar_height = ImGui::GetFrameHeight();
      const bool over_title_bar =
          mouse_pos.x >= window_pos.x &&
          mouse_pos.x <= (window_pos.x + window_size.x) &&
          mouse_pos.y >= window_pos.y &&
          mouse_pos.y <= (window_pos.y + title_bar_height);
      if (over_title_bar) {
        if (popout_open != nullptr) {
          *popout_open = false;
        }
        redock_popout = true;
      }
    }
  } else if (!window_requested_open) {
    s_tool_library_open.fill(false);
    s_library_frame_open = false;
  }

  if (redock_popout) {
    if (popout_open != nullptr) {
      *popout_open = false;
    }
    ActivateToolLibrary(tool);
    QueueDockWindow(ToolLibraryWindowName(tool), "Library", false);
  }

  if (open && window_requested_open) {
#if !defined(IMGUI_HAS_DOCK)
    if (!popout_instance) {
      s_library_frame_state.initialized = true;
      s_library_frame_state.pos = ImGui::GetWindowPos();
      s_library_frame_state.size = ImGui::GetWindowSize();
    }
#endif
    const bool container_open =
        BeginWindowContainer("##tool_library_container");
    const ImVec2 library_avail = ImGui::GetContentRegionAvail();
    const LayoutMode layout_mode =
        ResponsiveLayout::GetLayoutMode(library_avail.x);
    if (!container_open) {
      EndWindowContainer();
      uiint.EndPanel();
      Components::EndTokenPanel();
      return;
    }
    const bool compact_layout =
        layout_mode == LayoutMode::Collapsed ||
        library_avail.x < ResponsiveConstants::MIN_PANEL_WIDTH ||
        library_avail.y < ResponsiveConstants::MIN_PANEL_HEIGHT;

    auto render_action_grid = [&]() {
      auto &hfsm = RogueCity::Core::Editor::GetEditorHFSM();
      auto &gs = RogueCity::Core::Editor::GetGlobalState();
      if (compact_layout) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled(
            "Compact layout active. Use scroll to access all tool actions.");
        ImGui::PopTextWrapPos();
        ImGui::Separator();
      }

      const float max_icon_size =
          std::max(26.0f, std::min(46.0f, library_avail.x - 12.0f));
      const float icon_size =
          std::clamp(ImGui::GetFrameHeight() * 1.6f, 26.0f, max_icon_size);
      const float spacing = std::max(4.0f, ImGui::GetStyle().ItemSpacing.x *
                                               (compact_layout ? 0.75f : 1.0f));
      const float label_band_height =
          std::max(24.0f, ImGui::GetTextLineHeight() * 1.9f);
      const ImVec2 entry_size(icon_size, icon_size + label_band_height + 5.0f);
      const int columns = static_cast<int>(std::max(
          1.0f,
          std::floor((library_avail.x + spacing) / (entry_size.x + spacing))));

      auto draw_action_section = [&](Tools::ToolActionGroup group,
                                     const char *header,
                                     const std::string &widget_binding) {
        std::vector<size_t> group_indices;
        group_indices.reserve(actions.size());
        for (size_t i = 0; i < actions.size(); ++i) {
          if (actions[i].group == group) {
            group_indices.push_back(i);
          }
        }

        if (group_indices.empty()) {
          return;
        }

        if (header != nullptr && header[0] != '\0') {
          ImGui::SeparatorText(header);
        }

        for (size_t local_idx = 0; local_idx < group_indices.size();
             ++local_idx) {
          if (local_idx > 0 && (static_cast<int>(local_idx) % columns) != 0) {
            ImGui::SameLine(0.0f, spacing);
          }

          const size_t action_index = group_indices[local_idx];
          const auto &action = actions[action_index];
          const bool action_enabled = Tools::IsToolActionEnabled(action);
          const bool same_domain =
              gs.tool_runtime.active_domain == action.domain ||
              (action.domain == RogueCity::Core::Editor::ToolDomain::District &&
               gs.tool_runtime.active_domain ==
                   RogueCity::Core::Editor::ToolDomain::Zone) ||
              (action.domain == RogueCity::Core::Editor::ToolDomain::Zone &&
               gs.tool_runtime.active_domain ==
                   RogueCity::Core::Editor::ToolDomain::District);
          const bool action_active =
              same_domain && IsToolActionActive(action, gs);

          ImGui::PushID(static_cast<int>(action_index));
          if (!action_enabled) {
            ImGui::BeginDisabled();
          }
          const bool clicked = ImGui::InvisibleButton("ToolEntry", entry_size);
          if (!action_enabled) {
            ImGui::EndDisabled();
          }

          const ImVec2 bmin = ImGui::GetItemRectMin();
          const ImVec2 bmax = ImGui::GetItemRectMax();
          const ImVec2 center((bmin.x + bmax.x) * 0.5f,
                              bmin.y + icon_size * 0.48f);
          ImDrawList *draw_list = ImGui::GetWindowDrawList();
          const float pulse =
              0.5f + 0.5f * static_cast<float>(std::sin(ImGui::GetTime() * 4.0 +
                                                        action_index * 0.37));
          const ImU32 active_border =
              WithAlpha(LerpColor(UITokens::ToolActiveBorder,
                                  UITokens::CyanAccent, pulse),
                        240u);
          const ImU32 active_fill = WithAlpha(
              LerpColor(UITokens::ToolActiveFill, UITokens::PanelBackground,
                        1.0f - pulse * 0.35f),
              245u);

          const ImU32 fill =
              !action_enabled
                  ? UITokens::ToolDisabledFill
                  : (action_active
                         ? active_fill
                         : WithAlpha(UITokens::PanelBackground, 220u));
          const ImU32 border =
              !action_enabled
                  ? WithAlpha(UITokens::TextSecondary, 100u)
                  : (action_active ? active_border
                                   : WithAlpha(UITokens::TextSecondary, 180u));
          draw_list->AddRectFilled(bmin, bmax, fill, 8.0f);
          draw_list->AddRect(bmin, bmax, border, 8.0f, 0,
                             action_active ? 2.0f : 1.5f);
          if (action_active) {
            draw_list->AddRectFilled(ImVec2(bmin.x + 3.0f, bmin.y + 3.0f),
                                     ImVec2(bmax.x - 3.0f, bmin.y + 6.0f),
                                     WithAlpha(active_border, 220u), 2.0f);
          }
          DrawToolLibraryIcon(draw_list, tool, center, icon_size * 0.34f);

          std::string label_text = action.label;
          const float max_label_width = std::max(10.0f, entry_size.x - 8.0f);
          if (ImGui::CalcTextSize(label_text.c_str()).x > max_label_width) {
            const size_t split = label_text.find(' ');
            if (split != std::string::npos && split + 1u < label_text.size()) {
              std::string wrapped = label_text;
              wrapped[split] = '\n';
              if (ImGui::CalcTextSize(wrapped.c_str()).x <=
                  max_label_width + 6.0f) {
                label_text = std::move(wrapped);
              }
            }
          }
          if (ImGui::CalcTextSize(label_text.c_str()).x > max_label_width) {
            while (label_text.size() > 4u &&
                   ImGui::CalcTextSize((label_text + "...").c_str()).x >
                       max_label_width) {
              label_text.pop_back();
            }
            label_text += "...";
          }
          const ImVec2 label_size = ImGui::CalcTextSize(label_text.c_str());
          const ImVec2 label_pos(
              bmin.x + std::max(2.0f, (entry_size.x - label_size.x) * 0.5f),
              bmin.y + icon_size +
                  std::max(1.0f, (label_band_height - label_size.y) * 0.5f));
          draw_list->AddText(label_pos,
                             action_enabled
                                 ? WithAlpha(UITokens::TextPrimary,
                                             action_active ? 255u : 215u)
                                 : UITokens::TextDisabled,
                             label_text.c_str());

          if (clicked) {
            std::string dispatch_status;
            Tools::DispatchContext context{&hfsm, &gs, &uiint, window_name};
            const auto result =
                Tools::DispatchToolAction(action.id, context, &dispatch_status);
            (void)result;
          }

          if (ImGui::IsItemHovered()) {
            if (!action_enabled) {
              ImGui::SetTooltip("%s\n%s", action.label, action.disabled_reason);
            } else if (action.tooltip != nullptr && action.tooltip[0] != '\0') {
              ImGui::SetTooltip("%s\n%s", action.label, action.tooltip);
            } else {
              ImGui::SetTooltip("%s", action.label);
            }
          }

          uiint.RegisterWidget(
              {"button",
               action.label,
               std::string("action:") + Tools::ToolActionName(action.id),
               {"tool", "library"}});
          uiint.RegisterAction({Tools::ToolActionName(action.id),
                                action.label,
                                window_name,
                                {"tool", "library"},
                                "RC_UI::Tools::DispatchToolAction"});
          ImGui::PopID();
        }

        uiint.RegisterWidget({"table",
                              header != nullptr ? header : window_name,
                              widget_binding,
                              {"tool", "library"}});
      };

      draw_action_section(Tools::ToolActionGroup::Primary, nullptr,
                          std::string(window_name) + ".primary[]");
      draw_action_section(Tools::ToolActionGroup::Spline, "Spline Tools",
                          std::string(window_name) + ".spline[]");
      draw_action_section(Tools::ToolActionGroup::FutureStub, "Future Stubs",
                          std::string(window_name) + ".future[]");

      const Tools::ToolActionSpec *active_action = nullptr;
      for (const auto &action : actions) {
        const bool same_domain =
            gs.tool_runtime.active_domain == action.domain ||
            (action.domain == RogueCity::Core::Editor::ToolDomain::District &&
             gs.tool_runtime.active_domain ==
                 RogueCity::Core::Editor::ToolDomain::Zone) ||
            (action.domain == RogueCity::Core::Editor::ToolDomain::Zone &&
             gs.tool_runtime.active_domain ==
                 RogueCity::Core::Editor::ToolDomain::District);
        if (same_domain && IsToolActionActive(action, gs)) {
          active_action = &action;
          break;
        }
      }
      if (active_action != nullptr && active_action->tooltip != nullptr &&
          active_action->tooltip[0] != '\0') {
        ImGui::Spacing();
        ImGui::SeparatorText("Context Cue");
        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(UITokens::TextPrimary), "Active: %s",
            active_action->label);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary), "%s",
            active_action->tooltip);
        ImGui::PopTextWrapPos();
      }
    };

    if (!actions.empty()) {
      render_action_grid();
    }
    if (content_renderer) {
      if (!actions.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
      }
      content_renderer();
    }
    EndWindowContainer();
  }

  uiint.EndPanel();
  Components::EndTokenPanel();
}

// Static minimap instance (Phase 5: Polish)
// TODO: Phase 5 - Refactor to support multiple viewport types and dynamic
// viewport management, with the minimap as a special case or plugin.
static std::unique_ptr<RogueCity::App::MinimapViewport> s_minimap;
static bool s_dock_built = false;
static bool s_dock_layout_dirty = true;
static bool s_legacy_window_settings_cleared = false;
static ImVec2 s_last_layout_viewport_size{0.0f, 0.0f};

namespace {
constexpr float kMinCenterRatio = 0.40f;
constexpr int kDockBuildStableFrameCount = 3;
constexpr float kDockSizeStabilityTolerancePx = 2.0f;
constexpr float kDockBuildHardMinWidth = 64.0f;
constexpr float kDockBuildHardMinHeight = 64.0f;

struct DockBuildStabilityState {
  ImVec2 last_viewport_size{0.0f, 0.0f};
  int stable_frame_count{0};
};

static DockBuildStabilityState s_dock_build_stability{};

void ResetDockBuildStability() {
  s_dock_build_stability.last_viewport_size = ImVec2(0.0f, 0.0f);
  s_dock_build_stability.stable_frame_count = 0;
}

[[nodiscard]] bool IsViewportStableForDockBuild(const ImGuiViewport *viewport) {
  if (viewport == nullptr || viewport->Size.x <= 0.0f ||
      viewport->Size.y <= 0.0f) {
    ResetDockBuildStability();
    return false;
  }

  if (viewport->Size.x < kDockBuildHardMinWidth ||
      viewport->Size.y < kDockBuildHardMinHeight) {
    ResetDockBuildStability();
    return false;
  }

  const ImVec2 current_size = viewport->Size;
  bool size_changed = true;
  if (s_dock_build_stability.last_viewport_size.x > 0.0f &&
      s_dock_build_stability.last_viewport_size.y > 0.0f) {
    const float delta_x =
        std::fabs(current_size.x - s_dock_build_stability.last_viewport_size.x);
    const float delta_y =
        std::fabs(current_size.y - s_dock_build_stability.last_viewport_size.y);
    size_changed = delta_x > kDockSizeStabilityTolerancePx ||
                   delta_y > kDockSizeStabilityTolerancePx;
  }

  s_dock_build_stability.last_viewport_size = current_size;
  if (size_changed) {
    s_dock_build_stability.stable_frame_count = 1;
  } else {
    s_dock_build_stability.stable_frame_count =
        std::min(s_dock_build_stability.stable_frame_count + 1, 1024);
  }

  if (s_dock_build_stability.stable_frame_count < kDockBuildStableFrameCount) {
    return false;
  }

  return true;
}

[[nodiscard]] bool HasSignificantViewportResize(const ImGuiViewport *viewport) {
  if (viewport == nullptr || viewport->Size.x <= 0.0f ||
      viewport->Size.y <= 0.0f) {
    return false;
  }

  if (s_last_layout_viewport_size.x <= 0.0f ||
      s_last_layout_viewport_size.y <= 0.0f) {
    s_last_layout_viewport_size = viewport->Size;
    return false;
  }

  const float dx = std::fabs(viewport->Size.x - s_last_layout_viewport_size.x);
  const float dy = std::fabs(viewport->Size.y - s_last_layout_viewport_size.y);
  const float x_threshold =
      std::max(120.0f, s_last_layout_viewport_size.x * 0.18f);
  const float y_threshold =
      std::max(120.0f, s_last_layout_viewport_size.y * 0.18f);
  const bool changed = dx >= x_threshold || dy >= y_threshold;
  if (changed) {
    s_last_layout_viewport_size = viewport->Size;
  }
  return changed;
}

void ClearLegacyPanelWindowSettingsOnce() {
  if (s_legacy_window_settings_cleared) {
    return;
  }

  const std::array<const char *, 8> legacy_windows = {
      "District Index", "Road Index", "Lot Index", "River Index",
      "Building Index", "Tools",      "Log",       "System Map"};

  for (const char *window_name : legacy_windows) {
    ImGui::ClearWindowSettings(window_name);
  }

  s_legacy_window_settings_cleared = true;
}
} // namespace

static std::string ActiveModeFromHFSM() {
  using RogueCity::Core::Editor::EditorState;
  const auto state = RogueCity::Core::Editor::GetEditorHFSM().state();

  switch (state) {
  case EditorState::Editing_Axioms:
  case EditorState::Viewport_PlaceAxiom:
    return "AXIOM";
  case EditorState::Editing_Roads:
  case EditorState::Viewport_DrawRoad:
    return "ROAD";
  case EditorState::Editing_Districts:
    return "DISTRICT";
  case EditorState::Editing_Lots:
    return "LOT";
  case EditorState::Editing_Buildings:
    return "BUILDING";
  default:
    break;
  }

  return "IDLE";
}

// Utility function to generate a default dock tree layout for new workspaces or
// when no saved layout is available.
// TODO: Phase 4 - Refactor to support multiple dock tree configurations and
// user-customizable default layouts, potentially with a visual layout editor.
static RogueCity::UIInt::DockTreeNode DefaultDockTree() {
  using RogueCity::UIInt::DockTreeNode;
  DockTreeNode root;
  root.id = "root";
  root.orientation = "horizontal";

  DockTreeNode left;
  left.id = "left";
  left.panel_id = "Master Panel";

  DockTreeNode center;
  center.id = "center";
  center.panel_id = "[/ / / RC_VISUALIZER / / /]";

  DockTreeNode tool_deck;
  tool_deck.id = "tool_deck";
  tool_deck.panel_id = "Tool Deck";

  DockTreeNode library;
  library.id = "library";
  library.panel_id = "Axiom Library";

  DockTreeNode right_column;
  right_column.id = "right_column";
  right_column.orientation = "vertical";
  right_column.children = {tool_deck, library};

  root.children = {left, center, right_column};
  return root;
}
// Initialization of the minimap viewport instance, with default settings.
void InitializeMinim() {
  if (!s_minimap) {
    s_minimap = std::make_unique<RogueCity::App::MinimapViewport>();
    s_minimap->initialize();
    s_minimap->set_size(RogueCity::App::MinimapViewport::Size::Medium);
  }
}

DockLayoutPreferences GetDefaultDockLayoutPreferences() {
  DockLayoutPreferences preferences;
  preferences.left_panel_ratio = 0.32f;
  preferences.right_panel_ratio = 0.22f;
  preferences.tool_deck_ratio = 0.24f;
  return preferences;
}

DockLayoutPreferences GetDockLayoutPreferences() {
  return s_dock_layout_preferences;
}

void SetDockLayoutPreferences(const DockLayoutPreferences &preferences) {
  DockLayoutPreferences clamped;
  clamped.left_panel_ratio =
      std::clamp(preferences.left_panel_ratio, 0.20f, 0.45f);
  clamped.right_panel_ratio =
      std::clamp(preferences.right_panel_ratio, 0.15f, 0.35f);
  clamped.tool_deck_ratio =
      std::clamp(preferences.tool_deck_ratio, 0.18f, 0.45f);

  const float side_total = clamped.left_panel_ratio + clamped.right_panel_ratio;
  const float max_side_total = 1.0f - kMinCenterRatio;
  if (side_total > max_side_total) {
    const float scale = max_side_total / std::max(0.001f, side_total);
    clamped.left_panel_ratio *= scale;
    clamped.right_panel_ratio *= scale;
  }

  s_dock_layout_preferences = clamped;
  s_dock_layout_dirty = true;
}

bool IsAxiomLibraryOpen() { return IsToolLibraryOpen(ToolLibrary::Axiom); }

void ToggleAxiomLibrary() { ToggleToolLibrary(ToolLibrary::Axiom); }

static size_t ToolLibraryIndex(ToolLibrary tool) {
  switch (tool) {
  case ToolLibrary::Axiom:
    return 0;
  case ToolLibrary::Water:
    return 1;
  case ToolLibrary::Road:
    return 2;
  case ToolLibrary::District:
    return 3;
  case ToolLibrary::Lot:
    return 4;
  case ToolLibrary::Building:
    return 5;
  }
  return 0;
}

static const char *ToolLibraryWindowName(ToolLibrary tool) {
  switch (tool) {
  case ToolLibrary::Axiom:
    return "Axiom Library";
  case ToolLibrary::Water:
    return "Water Library";
  case ToolLibrary::Road:
    return "Road Library";
  case ToolLibrary::District:
    return "District Library";
  case ToolLibrary::Lot:
    return "Lot Library";
  case ToolLibrary::Building:
    return "Building Library";
  }
  return "Tool Library";
}

bool IsToolLibraryOpen(ToolLibrary tool) {
  const size_t index = ToolLibraryIndex(tool);
  return s_library_frame_open && s_tool_library_open[index];
}

bool IsToolLibraryPopoutOpen(ToolLibrary tool) {
  return s_tool_library_popout[ToolLibraryIndex(tool)];
}

void ActivateToolLibrary(ToolLibrary tool) {
  const size_t index = ToolLibraryIndex(tool);
  s_tool_library_open.fill(false);
  s_tool_library_open[index] = true;
  s_active_library_tool = tool;
  s_library_frame_open = true;
  QueueDockWindow(ToolLibraryWindowName(tool), "Library", false);
}

void PopoutToolLibrary(ToolLibrary tool) {
  const size_t index = ToolLibraryIndex(tool);
  ActivateToolLibrary(tool);
  s_tool_library_popout[index] = true;
}

void ToggleToolLibrary(ToolLibrary tool) {
  const size_t index = ToolLibraryIndex(tool);
  if (s_library_frame_open && s_active_library_tool == tool &&
      s_tool_library_open[index]) {
    s_tool_library_open[index] = false;
    s_tool_library_popout[index] = false;
    s_library_frame_open = false;
    return;
  }
  ActivateToolLibrary(tool);
}

void ApplyUnifiedWindowSchema(const ImVec2 &baseSize, float padding) {
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  const ImVec2 viewportSize =
      viewport ? viewport->Size : ImVec2(1920.0f, 1080.0f);
  const ImVec2 maxSize(std::max(320.0f, viewportSize.x - padding * 2.0f),
                       std::max(240.0f, viewportSize.y - padding * 2.0f));
  const ImVec2 clampedBase(std::min(baseSize.x, maxSize.x),
                           std::min(baseSize.y, maxSize.y));

  ImGui::SetNextWindowSize(clampedBase, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 240.0f), maxSize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
}

void PopUnifiedWindowSchema() { ImGui::PopStyleVar(); }

void BeginUnifiedTextWrap(float padding) {
  ImGui::PushTextWrapPos(ImGui::GetCursorPos().x +
                         ImGui::GetContentRegionAvail().x - padding);
}

void EndUnifiedTextWrap() { ImGui::PopTextWrapPos(); }

bool BeginWindowContainer(const char *id, ImGuiWindowFlags flags) {
  const ImGuiWindowFlags child_flags = flags;
  return ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), true, child_flags);
}

void EndWindowContainer() { ImGui::EndChild(); }

// ============================================================================
// DYNAMIC RESPONSIVE PANEL SYSTEM
// ============================================================================
#if defined(IMGUI_HAS_DOCK)
static void UpdateDynamicPanelSizes(const ImVec2 &viewport_size) {
  (void)viewport_size;
  // Keep this path side-effect free: dynamic redraw should not create helper
  // windows while resizing because that can steal focus and destabilize
  // docking/input.
}

static void BuildDockLayout(ImGuiID dockspace_id) {
#if defined(IMGUI_HAS_DOCK)
  if (dockspace_id == 0) {
    return;
  }
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  if (viewport == nullptr || viewport->Size.x <= 0.0f ||
      viewport->Size.y <= 0.0f) {
    return;
  }

  ImGui::DockBuilderRemoveNodeDockedWindows(dockspace_id, true);
  ImGui::DockBuilderRemoveNode(dockspace_id);
  ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

  // Layout: viewport-centric center with persistent master panel on the left.
  // Use a right-hand column for tool libraries and auxiliary stacks.
  ImGuiID dock_main = dockspace_id;
  ImGuiID dock_left = 0;
  ImGuiID dock_right = 0;
  ImGuiID dock_tool_deck = 0;
  ImGuiID dock_library = 0;

  float left_ratio =
      std::clamp(s_dock_layout_preferences.left_panel_ratio, 0.20f, 0.45f);
  float right_ratio =
      std::clamp(s_dock_layout_preferences.right_panel_ratio, 0.15f, 0.35f);
  float tool_deck_ratio =
      std::clamp(s_dock_layout_preferences.tool_deck_ratio, 0.18f, 0.45f);

  const float max_side_total = 1.0f - kMinCenterRatio;
  if (left_ratio + right_ratio > max_side_total) {
    const float scale = max_side_total / (left_ratio + right_ratio);
    left_ratio *= scale;
    right_ratio *= scale;
  }

  dock_left = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, left_ratio,
                                          nullptr, &dock_main);
  dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right,
                                           right_ratio, nullptr, &dock_main);
  dock_library = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down,
                                             1.0f - tool_deck_ratio, nullptr,
                                             &dock_tool_deck);

  ImGui::DockBuilderDockWindow("Master Panel", dock_left);
  ImGui::DockBuilderDockWindow("[/ / / RC_VISUALIZER / / /]", dock_main);
  ImGui::DockBuilderDockWindow("Inspector", dock_right);
  ImGui::DockBuilderDockWindow("System Map", dock_right);
  ImGui::DockBuilderDockWindow("Tool Deck", dock_tool_deck);
  ImGui::DockBuilderDockWindow("Axiom Library", dock_library);

  s_dock_nodes.root = dockspace_id;
  s_dock_nodes.left = dock_left;
  s_dock_nodes.right = dock_right;
  s_dock_nodes.tool_deck = dock_tool_deck;
  s_dock_nodes.library = dock_library;
  s_dock_nodes.center = dock_main;
  s_dock_nodes.bottom = 0;
  s_dock_nodes.bottom_tabs = 0;
#else
  (void)dockspace_id;
  s_dock_nodes = {};
#endif
}

static ImGuiID NodeForDockArea(const std::string &dock_area) {
  if (dock_area == "Top") {
    return s_dock_nodes.center;
  }
  if (dock_area == "Bottom") {
    return s_dock_nodes.bottom_tabs ? s_dock_nodes.bottom_tabs
                                    : s_dock_nodes.bottom;
  }
  if (dock_area == "Left") {
    return s_dock_nodes.left;
  }
  if (dock_area == "Right") {
    return s_dock_nodes.right;
  }
  if (dock_area == "ToolDeck") {
    return s_dock_nodes.tool_deck;
  }
  if (dock_area == "Library") {
    return s_dock_nodes.library;
  }
  return s_dock_nodes.center;
}

static bool ProcessPendingDockRequests(ImGuiID dockspace_id) {
#if defined(IMGUI_HAS_DOCK)
  if (s_pending_dock_requests.empty()) {
    return false;
  }

  if (!IsDockspaceValid(dockspace_id)) {
    s_pending_dock_requests.clear();
    return false;
  }

  bool any_applied = false;
  std::unordered_set<std::string> seen_windows;
  seen_windows.reserve(s_pending_dock_requests.size());
  for (auto it = s_pending_dock_requests.rbegin();
       it != s_pending_dock_requests.rend(); ++it) {
    const DockRequest &request = *it;
    if (request.window_name.empty()) {
      continue;
    }
    if (!seen_windows.insert(request.window_name).second) {
      continue;
    }

    ImGuiID target = NodeForDockArea(request.dock_area);
    if (target == 0) {
      target = s_dock_nodes.center;
    }

    ImGuiDockNode *target_node = GetDockNodeSafe(target);
    if (!DockNodeValidator::IsNodeSizeValid(target_node)) {
      target = s_dock_nodes.center;
      target_node = GetDockNodeSafe(target);
      if (!DockNodeValidator::IsNodeSizeValid(target_node)) {
        continue;
      }
    }

    if (request.own_dock_node) {
      ImGuiID new_target = 0;
      ImGui::DockBuilderSplitNode(target, ImGuiDir_Down, 0.55f, &new_target,
                                  &target);
      if (new_target != 0) {
        target = new_target;
      }
    }

    DockWindowSafe(request.window_name.c_str(), target);
    any_applied = true;
  }

  s_pending_dock_requests.clear();
  return any_applied;
#else
  (void)dockspace_id;
  s_pending_dock_requests.clear();
  return false;
#endif
}

static void ClearInvalidDockAssignments(ImGuiID dockspace_id) {
  ImGuiContext *ctx = ImGui::GetCurrentContext();
  if (ctx == nullptr) {
    return;
  }

  for (ImGuiWindow *window : ctx->Windows) {
    if (window == nullptr || window->DockId == 0) {
      continue;
    }

    if (!DockNodeValidator::IsWindowDockValid(window, dockspace_id)) {
      window->DockId = 0;
      window->DockNode = nullptr;
    }
  }
}
#endif

static void UpdateDockLayout(ImGuiID dockspace_id) {
#if defined(IMGUI_HAS_DOCK)
  bool layout_changed = false;
  const ImGuiViewport *viewport = ImGui::GetMainViewport();

  if (HasSignificantViewportResize(viewport)) {
    s_dock_layout_dirty = true;
  }

  if (!IsDockspaceValid(dockspace_id)) {
    s_dock_built = false;
    s_dock_layout_dirty = true;
  }

  // Rebuild only on explicit reset/dirty transitions. Auto-rebuilding every
  // frame based on heuristic "health" thresholds causes visible de-render
  // states on small window sizes.
  const bool needs_rebuild = !s_dock_built || s_dock_layout_dirty;
  if (needs_rebuild) {
    if (!IsViewportStableForDockBuild(viewport)) {
      return;
    }

    // Hook for viewport-responsive panel adjustments (currently no-op per
    // anti-churn contract).
    if (viewport != nullptr) {
      UpdateDynamicPanelSizes(viewport->Size);
    }

    BuildDockLayout(dockspace_id);
    s_dock_built = true;
    s_dock_layout_dirty = false;
    layout_changed = true;
  }

  if (ProcessPendingDockRequests(dockspace_id)) {
    layout_changed = true;
  }

  if (layout_changed && IsDockspaceValid(dockspace_id)) {
    ClearInvalidDockAssignments(dockspace_id);
    ImGui::DockBuilderFinish(dockspace_id);
  }
#else
  (void)dockspace_id;
  s_pending_dock_requests.clear();
  s_dock_built = true;
  s_dock_layout_dirty = false;
#endif
}

static void DrawRuntimeTitlebar() {
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  if (viewport == nullptr) {
    return;
  }

  constexpr float kTitlebarHeight = 34.0f;
  ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, kTitlebarHeight),
                           ImGuiCond_Always);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::SetNextWindowBgAlpha(0.94f);

  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNav |
      ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoInputs;

  const bool open =
      ImGui::Begin("Rogue Titlebar###RC_RuntimeTitlebar", nullptr, flags);
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{
          "Rogue Titlebar",
          "Titlebar",
          "titlebar",
          "Top",
          "visualizer/src/ui/rc_ui_root.cpp",
          {"titlebar", "chrome", "runtime"}},
      open);

  if (open) {
    const std::string mode = ActiveModeFromHFSM();
    ImGui::TextUnformatted("ROGUECITY URBAN SPATIAL DESIGNER");
    ImGui::SameLine();
    ImGui::TextDisabled("| Mode: %s", mode.c_str());
    uiint.RegisterWidget({"text", "Titlebar Brand", "titlebar.brand", {"titlebar"}});
    uiint.RegisterWidget({"text", "Mode Indicator", "titlebar.mode", {"titlebar"}});
  }

  uiint.EndPanel();
  ImGui::End();
}

static void DrawRuntimeStatusBar() {
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  if (viewport == nullptr) {
    return;
  }

  constexpr float kStatusHeight = 28.0f;
  ImGui::SetNextWindowPos(
      ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - kStatusHeight),
      ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, kStatusHeight),
                           ImGuiCond_Always);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::SetNextWindowBgAlpha(0.90f);

  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNav |
      ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoInputs;

  const bool open =
      ImGui::Begin("Rogue Status Bar###RC_RuntimeStatusBar", nullptr, flags);
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{
          "Rogue Status Bar",
          "Status Bar",
          "status",
          "Bottom",
          "visualizer/src/ui/rc_ui_root.cpp",
          {"status", "runtime"}},
      open);

  if (open) {
    auto &gs = RogueCity::Core::Editor::GetGlobalState();
    const int log_events = Panels::Log::GetEventCount();
    const int validation_events = Panels::Validation::GetValidationEventCount();
    const bool validation_failed = Panels::Validation::HasValidationFailure();
    const bool dirty = gs.dirty_layers.AnyDirty();

    const char *validation_state = validation_failed ? "REJECTED"
                                  : (gs.plan_approved ? "APPROVED" : "PENDING");

    ImGui::Text("Status: %s", validation_state);
    ImGui::SameLine();
    ImGui::TextDisabled("| Validation: %d", validation_events);
    ImGui::SameLine();
    ImGui::TextDisabled("| Log: %d", log_events);
    ImGui::SameLine();
    ImGui::TextDisabled("| Dirty: %s", dirty ? "yes" : "no");
    uiint.RegisterWidget({"text", "Status Summary", "status.summary", {"status"}});
  }

  uiint.EndPanel();
  ImGui::End();
}

void DrawRoot(float dt) {
  // Initialize minimap on first call
  InitializeMinim();
  ClearLegacyPanelWindowSettingsOnce();

  // Begin UI introspection for this frame (dev-only tooling).
  auto &introspector = RogueCity::UIInt::UiIntrospector::Instance();
  introspector.BeginFrame(ActiveModeFromHFSM(),
                          RC_UI::Panels::DevShell::IsOpen());
  if (!s_dock_built) {
    introspector.SetDockTree(DefaultDockTree());
  }

  // Dockspace host window
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  if (viewport == nullptr) {
    return;
  }

  const ViewportBounds viewport_bounds{viewport->Pos.x, viewport->Pos.y,
                                       viewport->Size.x, viewport->Size.y};
  if (!viewport_bounds.IsValid()) {
    return;
  }

#if defined(IMGUI_HAS_DOCK)
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  const ImGuiWindowFlags host_flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground |
      ImGuiWindowFlags_NoFocusOnAppearing;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("RogueDockHost", nullptr, host_flags);
  ImGui::PopStyleVar(3);

  ImGuiID dockspace_id = ImGui::GetID("RogueDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0, 0),
                   ImGuiDockNodeFlags_PassthruCentralNode);
  UpdateDockLayout(dockspace_id);
  ImGui::End();
#else
  // Non-docking ImGui build: avoid creating a full-screen host window that
  // steals focus/input.
  UpdateDockLayout(0);
#endif

  DrawRuntimeTitlebar();
  DrawRuntimeStatusBar();

  // MASTER PANEL SYSTEM (RC-0.10)
  // Single canonical UI path: dock host -> master panel (drawer registry) ->
  // viewport. This avoids duplicate tool surfaces from mixed
  // direct/window/schema render flows.

  // Initialize registry once
  if (!s_registry_initialized) {
    RC_UI::Panels::InitializePanelRegistry();
    s_master_panel = std::make_unique<RC_UI::Panels::RcMasterPanel>();
    s_registry_initialized = true;
  }

  // Primary viewport should always be present as the center workspace.
#if !defined(IMGUI_HAS_DOCK)
  const float left_width = std::max(430.0f, viewport->Size.x * 0.34f);
  ImGui::SetNextWindowPos(
      ImVec2(viewport->Pos.x + left_width + 8.0f, viewport->Pos.y + 8.0f),
      ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(
      ImVec2(std::max(480.0f, viewport->Size.x - left_width - 16.0f),
             std::max(360.0f, viewport->Size.y - 16.0f)),
      ImGuiCond_FirstUseEver);
#endif

  // Single master panel hosts non-viewport drawers.
#if !defined(IMGUI_HAS_DOCK)
  ImGui::SetNextWindowPos(
      ImVec2(viewport->Pos.x + 8.0f, viewport->Pos.y + 8.0f),
      ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(std::max(380.0f, left_width - 12.0f),
                                  std::max(360.0f, viewport->Size.y - 16.0f)),
                           ImGuiCond_FirstUseEver);
#endif
  if (s_master_panel) {
    s_master_panel->Draw(dt);
  }

  // Render tool library windows (Axiom, Water, Road, etc.)
  // These are standalone windows that dock into the "Library" node.
  for (const auto tool : kToolLibraryOrder) {
    const auto actions = Tools::GetToolActionsForLibrary(tool);
    RenderToolLibraryWindow(tool, ToolLibraryWindowName(tool), "Shared",
                            "Library", actions);
  }

  // DEPRECATED: Old individual panel calls (replaced by Master Panel)
  // Panels::AxiomBar::Draw(dt);
  // Primary viewport (AxiomEditor) owns the center workspace rendering
  Panels::AxiomEditor::Draw(dt);
  Panels::Inspector::Draw(dt);
  Panels::SystemMap::Draw(dt);
  Panels::Workspace::Draw(dt);
  // Panels::Telemetry::Draw(dt);
  // ... (18 more panels)

  // Minimap is now embedded as overlay in RogueVisualizer (no separate panel)
  // TODO investigate potentially decoupleing the minimap from root to allow it
  // to be used as a standalone veiwport in other contexte s (e.g. floating,
  // secondary monitor) in the future. also to ensure that the mimimap redocks
  // gracefully to the main veiwport Still update the minimap viewport for
  // shared camera sync
  if (s_minimap) {
    s_minimap->update(dt);
  }
}

RogueCity::App::MinimapViewport *GetMinimapViewport() {
  return s_minimap.get();
}

bool QueueDockWindow(const char *windowName, const char *dockArea,
                     bool ownDockNode) {
  if (!windowName || !dockArea) {
    return false;
  }

  for (auto it = s_pending_dock_requests.rbegin();
       it != s_pending_dock_requests.rend(); ++it) {
    if (it->window_name == windowName) {
      it->dock_area = dockArea;
      it->own_dock_node = ownDockNode;
      return true;
    }
  }

  if (s_pending_dock_requests.size() >= kMaxPendingDockRequests) {
    s_pending_dock_requests.erase(s_pending_dock_requests.begin());
  }

  s_pending_dock_requests.push_back(
      DockRequest{std::string(windowName), std::string(dockArea), ownDockNode});
  return true;
}

void ResetDockLayout() {
  s_dock_built = false;
  s_dock_layout_dirty = true;
  s_pending_dock_requests.clear();
  ResetDockBuildStability();
  s_last_layout_viewport_size = ImVec2(0.0f, 0.0f);
}

bool SaveWorkspacePreset(const char *presetName, std::string *error) {
  if (presetName == nullptr || presetName[0] == '\0') {
    if (error != nullptr) {
      *error = "Preset name is empty.";
    }
    return false;
  }

  if (ImGui::GetCurrentContext() == nullptr) {
    if (error != nullptr) {
      *error = "ImGui context is not initialized.";
    }
    return false;
  }

  size_t ini_size = 0;
  const char *ini_data = ImGui::SaveIniSettingsToMemory(&ini_size);
  if (ini_data == nullptr || ini_size == 0) {
    if (error != nullptr) {
      *error = "ImGui returned empty layout data.";
    }
    return false;
  }

  WorkspacePresetStore store;
  LoadWorkspacePresetStore(store);

  PresetEntry entry;
  entry.ini       = std::string(ini_data, ini_size);
  entry.theme     = RogueCity::UI::ThemeManager::Instance().GetActiveTheme();
  entry.has_theme = true;
  store.presets[presetName] = std::move(entry);

  // TODO(ai-layout): expose preset metadata to the UI agent for monitor-aware
  // workspace suggestions.
  return SaveWorkspacePresetStore(store, error);
}

bool LoadWorkspacePreset(const char *presetName, std::string *error) {
  if (presetName == nullptr || presetName[0] == '\0') {
    if (error != nullptr) {
      *error = "Preset name is empty.";
    }
    return false;
  }

  if (ImGui::GetCurrentContext() == nullptr) {
    if (error != nullptr) {
      *error = "ImGui context is not initialized.";
    }
    return false;
  }

  WorkspacePresetStore store;
  LoadWorkspacePresetStore(store);
  const auto it = store.presets.find(presetName);
  if (it == store.presets.end()) {
    if (error != nullptr) {
      *error = "Preset not found: " + std::string(presetName);
    }
    return false;
  }

  const PresetEntry &entry = it->second;
  ImGui::LoadIniSettingsFromMemory(entry.ini.c_str(),
                                   static_cast<size_t>(entry.ini.size()));
  s_pending_dock_requests.clear();
  s_dock_layout_dirty = false;
  s_dock_built = true;

  // Restore theme snapshot if present
  if (entry.has_theme) {
    auto &tm = RogueCity::UI::ThemeManager::Instance();
    // Register and switch to the saved theme so it's reflected in the UI
    const std::string &tname = entry.theme.name;
    if (!tm.GetTheme(tname)) {
      tm.RegisterTheme(entry.theme);
    }
    if (!tm.LoadTheme(tname)) {
      // Theme name may differ; apply directly
      tm.GetActiveThemeMutable() = entry.theme;
      tm.ApplyToImGui();
    }
  }
  return true;
}

std::vector<std::string> ListWorkspacePresets() {
  WorkspacePresetStore store;
  LoadWorkspacePresetStore(store);
  std::vector<std::string> names;
  names.reserve(store.presets.size());
  for (const auto &[name, entry] : store.presets) {
    (void)entry;
    names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  return names;
}

void NotifyDockedWindow(const char *windowName, const char *dockArea) {
  if (!windowName || !dockArea || dockArea[0] == '\0') {
    return;
  }

  s_last_dock_area[windowName] = dockArea;
}

void ReturnWindowToLastDock(const char *windowName, const char *fallbackArea) {
  if (!windowName) {
    return;
  }

  const auto it = s_last_dock_area.find(windowName);
  const char *target = fallbackArea;
  if (it != s_last_dock_area.end()) {
    target = it->second.c_str();
  }

  QueueDockWindow(windowName, target);
}

[[nodiscard]] static bool IsRectVisibleOnAnyViewport(const ImVec2 &pos,
                                                     const ImVec2 &size) {
  if (size.x <= 1.0f || size.y <= 1.0f) {
    return true;
  }

  const ImRect rect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
#if defined(IMGUI_HAS_DOCK)
  ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
  for (ImGuiViewport *viewport : platform_io.Viewports) {
    if (viewport == nullptr) {
      continue;
    }
    const ImRect viewport_rect(viewport->Pos,
                               ImVec2(viewport->Pos.x + viewport->Size.x,
                                      viewport->Pos.y + viewport->Size.y));
    if (viewport_rect.Overlaps(rect)) {
      return true;
    }
  }
#endif

  if (ImGuiViewport *main_viewport = ImGui::GetMainViewport();
      main_viewport != nullptr) {
    const ImRect viewport_rect(
        main_viewport->Pos,
        ImVec2(main_viewport->Pos.x + main_viewport->Size.x,
               main_viewport->Pos.y + main_viewport->Size.y));
    return viewport_rect.Overlaps(rect);
  }
  return true;
}

bool BeginDockableWindow(const char *windowName, DockableWindowState &state,
                         const char *fallbackDockArea, ImGuiWindowFlags flags) {
  const bool show_close = !state.was_docked;
  bool *p_open = show_close ? &state.open : nullptr;
  const ImGuiWindowFlags resolved_flags = flags | ImGuiWindowFlags_NoCollapse;
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
  const bool open = ImGui::Begin(windowName, p_open, resolved_flags);
  bool is_docked = false;
#if defined(IMGUI_HAS_DOCK)
  is_docked = ImGui::IsWindowDocked();
#endif
  state.was_docked = is_docked;

  if (!is_docked) {
    const ImVec2 window_pos = ImGui::GetWindowPos();
    const ImVec2 window_size = ImGui::GetWindowSize();
    if (!IsRectVisibleOnAnyViewport(window_pos, window_size)) {
      // Rescue floating windows that were dragged off-screen.
      if (ImGuiViewport *main_viewport = ImGui::GetMainViewport();
          main_viewport != nullptr) {
        const ImVec2 rescue_pos(main_viewport->Pos.x + 48.0f,
                                main_viewport->Pos.y + 48.0f);
        ImGui::SetWindowPos(windowName, rescue_pos, ImGuiCond_Always);
      }
      ReturnWindowToLastDock(windowName, fallbackDockArea);
    }
  }

  if (is_docked) {
    NotifyDockedWindow(windowName, fallbackDockArea);
  }

  if (!open || !state.open) {
    ReturnWindowToLastDock(windowName, fallbackDockArea);
    state.open = true;
    ImGui::End();
    return false;
  }

  BeginWindowContainer();
  return true;
}

void EndDockableWindow() {
  EndWindowContainer();
  ImGui::End();
}

// ---[BEGIN: DRAW PANEL BY
// TYPE]------------------------------------------------ WHY: All panel layout
// is now driven by PanelLayout entries, not scattered Draw() calls.
//      This lets you:
//        - change where a panel is docked without touching its Draw() code,
//        - group panels into tab groups (e.g., Indices) more easily.
// WHERE: rc_ui_root.cpp

void DrawIndicesPanel(IndicesTabs &tabs, float dt) {
  auto &gs = RogueCity::Core::Editor::GetGlobalState();
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();

  if (ImGui::BeginTabBar("IndicesTabs")) {
    if (tabs.district && ImGui::BeginTabItem("District Index")) {
      Panels::DistrictIndex::GetPanel().DrawContent(gs, uiint);
      ImGui::EndTabItem();
    }
    if (tabs.road && ImGui::BeginTabItem("Road Index")) {
      Panels::RoadIndex::GetPanel().DrawContent(gs, uiint);
      ImGui::EndTabItem();
    }
    if (tabs.lot && ImGui::BeginTabItem("Lot Index")) {
      Panels::LotIndex::GetPanel().DrawContent(gs, uiint);
      ImGui::EndTabItem();
    }
    if (tabs.river && ImGui::BeginTabItem("River Index")) {
      Panels::RiverIndex::DrawContent(dt);
      ImGui::EndTabItem();
    }
    if (tabs.building && ImGui::BeginTabItem("Building Index")) {
      Panels::BuildingIndex::GetPanel().DrawContent(gs, uiint);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

void DrawPanelByType(RC_UI::Panels::PanelType type, float dt,
                     std::string_view window_name) {
  (void)window_name;

  // Content-only rendering: caller owns Begin/End and docking policy.
  if (type == RC_UI::Panels::PanelType::Indices) {
    DrawIndicesPanel(s_indices_tabs, dt);
    return;
  }

  auto &gs = RogueCity::Core::Editor::GetGlobalState();
  auto &hfsm = RogueCity::Core::Editor::GetEditorHFSM();
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  auto &registry = RC_UI::Panels::PanelRegistry::Instance();
  Panels::DrawContext ctx{
      gs, hfsm, uiint, &RogueCity::App::GetEditorCommandHistory(), dt, false};
  registry.DrawByType(type, ctx);
}

// ---[END: DRAW PANEL BY
// TYPE]--------------------------------------------------

// ---[BEGIN: BUTTON DOCKED
// PANEL]-----------------------------------------------
void DrawButtonDockedPanel(ButtonDockedPanel &panel, float dt) {
  ImGui::PushID(panel.window_name.c_str());

  // Button opens/closes the panel.
  if (ImGui::Button(("▶ " + panel.window_name).c_str())) {
    panel.m_open = !panel.m_open;
  }
  ImGui::SameLine();
  ImGui::Text("%s", "Panel");

  // Optional: hold Shift to toggle between docked/floating behavior.
  if (ImGui::IsItemHovered() && ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      panel.m_docked = !panel.m_docked;
      if (panel.m_docked) {
        QueueDockWindow(panel.window_name.c_str(), panel.dock_area.c_str());
      }
      ImGui::SetTooltip("Shift-click to toggle docked/floating.\n"
                        "Now %s.",
                        panel.m_docked ? "Docked" : "Floating");
    }
  } else {
    ImGui::SetTooltip("Click to show %s.\n"
                      "Shift-click to toggle docked/floating.",
                      panel.window_name.c_str());
  }

  if (!panel.m_open) {
    ImGui::PopID();
    return;
  }

  bool is_window_open = true;
  if (panel.m_docked) {
    // Docked in its default area.
    ImGui::Begin(panel.window_name.c_str(), nullptr,
                 ImGuiWindowFlags_NoCollapse);
    BeginWindowContainer();
  } else {
    // Floating popout window.
    ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 500));
    ImGui::SetNextWindowFocus();
    ImGui::Begin(panel.window_name.c_str(), &is_window_open,
                 ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse);
    BeginWindowContainer();
  }

  DrawPanelByType(panel.type, dt, panel.window_name);

  EndWindowContainer();
  ImGui::End();

  if (!is_window_open || !panel.m_open) {
    panel.m_open = false;
  }

  ImGui::PopID();
}
// ---[END: BUTTON DOCKED
// PANEL]-------------------------------------------------

void PublishUiInputGateState(const UiInputGateState &state) {
  s_last_input_gate = state;
}

const UiInputGateState &GetUiInputGateState() { return s_last_input_gate; }

static bool s_system_map_open = false;

bool IsSystemMapOpen() { return s_system_map_open; }
void ToggleSystemMap() { s_system_map_open = !s_system_map_open; }
void SetSystemMapOpen(bool open) { s_system_map_open = open; }

} // namespace RC_UI
