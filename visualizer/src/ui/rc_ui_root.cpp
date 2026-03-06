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
#include "ui/panels/rc_panel_activity_bar_left.h"
#include "ui/panels/rc_panel_activity_bar_right.h"
#include "ui/panels/rc_panel_inspector_sidebar.h"

#include "RogueCity/App/UI/ThemeManager.h"
#include "ui/panels/rc_panel_workspace.h"

#include <algorithm>
#include <array>
#include <ctime>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <imgui.h>
#include <imgui_internal.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
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
static std::unique_ptr<RC_UI::Panels::RcInspectorSidebar> s_inspector_sidebar;
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
  ImGuiID root       = 0;
  // Legacy aliases (kept for QueueDockWindow / NodeForDockArea compat)
  ImGuiID left       = 0; // → p3_tools
  ImGuiID right      = 0; // → p4_inspector
  ImGuiID tool_deck  = 0;
  ImGuiID library    = 0;
  ImGuiID center     = 0;
  ImGuiID bottom     = 0; // → p5_system
  ImGuiID bottom_tabs = 0;
  // New 6-zone layout nodes (BuildDefaultWorkspace)
  ImGuiID b1_icons     = 0; // Far-left activity bar  (fixed width, no-resize)
  ImGuiID p3_tools     = 0; // Tool panel / explorer
  ImGuiID p4_inspector = 0; // Inspector & diagnostics
  ImGuiID b2_icons     = 0; // Far-right activity bar (fixed width, no-resize)
  ImGuiID p5_system    = 0; // System / terminals (bottom, collapsible)
  ImGuiID viz_bar      = 0; // Visualizer context bar (below title bar)
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

// Preset store entry: docking ini blob + optional theme snapshot + optional
// runtime metadata used by UI agent layout suggestions.
struct PresetEntry {
  std::string ini;
  RogueCity::UI::ThemeProfile theme;
  bool has_theme = false; // false for entries loaded from schema-1 files
  bool has_metadata = false;
  struct Metadata {
    std::string saved_at_utc;
    bool has_viewport_size = false;
    ImVec2 viewport_size = ImVec2(0.0f, 0.0f);
    int monitor_count = 1;
    bool docking_enabled = true;
    bool multi_viewport_enabled = false;
    bool has_tool_runtime_state = false;
    int viewport_selection_mode = 0;
    int viewport_edit_tool = 0;
    int viewport_selection_target = 0;
  } metadata{};
};

struct WorkspacePresetStore {
  std::unordered_map<std::string, PresetEntry> presets;
};

[[nodiscard]] static std::filesystem::path WorkspacePresetPath() {
  return std::filesystem::path(kWorkspacePresetFile);
}

// Serialize a ThemeProfile to a JSON object (matches ThemeManager field names).
static nlohmann::json
ThemeProfileToPresetJson(const RogueCity::UI::ThemeProfile &t) {
  nlohmann::json j;
  j["name"] = t.name;
  j["primary_accent"] = t.primary_accent;
  j["secondary_accent"] = t.secondary_accent;
  j["success_color"] = t.success_color;
  j["warning_color"] = t.warning_color;
  j["error_color"] = t.error_color;
  j["background_dark"] = t.background_dark;
  j["panel_background"] = t.panel_background;
  j["grid_overlay"] = t.grid_overlay;
  j["text_primary"] = t.text_primary;
  j["text_secondary"] = t.text_secondary;
  j["text_disabled"] = t.text_disabled;
  j["border_accent"] = t.border_accent;
  return j;
}

static RogueCity::UI::ThemeProfile
ThemeProfileFromPresetJson(const nlohmann::json &j) {
  RogueCity::UI::ThemeProfile t;
  t.name = j.value("name", "Custom");
  t.primary_accent = j.value("primary_accent", ImU32{0});
  t.secondary_accent = j.value("secondary_accent", ImU32{0});
  t.success_color = j.value("success_color", ImU32{0});
  t.warning_color = j.value("warning_color", ImU32{0});
  t.error_color = j.value("error_color", ImU32{0});
  t.background_dark = j.value("background_dark", ImU32{0});
  t.panel_background = j.value("panel_background", ImU32{0});
  t.grid_overlay = j.value("grid_overlay", ImU32{0});
  t.text_primary = j.value("text_primary", ImU32{0xFFFFFFFF});
  t.text_secondary = j.value("text_secondary", ImU32{0xFFAAAAAA});
  t.text_disabled = j.value("text_disabled", ImU32{0xFF666666});
  t.border_accent = j.value("border_accent", ImU32{0});
  return t;
}

[[nodiscard]] static std::string CurrentUtcIso8601() {
  const std::time_t now = std::time(nullptr);
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &now);
#else
  gmtime_r(&now, &tm);
#endif
  std::ostringstream out;
  out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}

[[nodiscard]] static nlohmann::json
PresetMetadataToJson(const PresetEntry::Metadata &metadata) {
  nlohmann::json j;
  if (!metadata.saved_at_utc.empty()) {
    j["saved_at_utc"] = metadata.saved_at_utc;
  }
  if (metadata.has_viewport_size) {
    j["viewport_size"] = {{"x", metadata.viewport_size.x},
                          {"y", metadata.viewport_size.y}};
  }
  j["monitor_count"] = metadata.monitor_count;
  j["docking_enabled"] = metadata.docking_enabled;
  j["multi_viewport_enabled"] = metadata.multi_viewport_enabled;
  if (metadata.has_tool_runtime_state) {
    j["tool_runtime"] = {
        {"viewport_selection_mode", metadata.viewport_selection_mode},
        {"viewport_edit_tool", metadata.viewport_edit_tool},
        {"viewport_selection_target", metadata.viewport_selection_target},
    };
  }
  return j;
}

[[nodiscard]] static PresetEntry::Metadata
PresetMetadataFromJson(const nlohmann::json &j) {
  PresetEntry::Metadata metadata;
  metadata.saved_at_utc = j.value("saved_at_utc", std::string{});
  metadata.monitor_count = std::max(1, j.value("monitor_count", 1));
  metadata.docking_enabled = j.value("docking_enabled", true);
  metadata.multi_viewport_enabled = j.value("multi_viewport_enabled", false);

  if (j.contains("viewport_size") && j["viewport_size"].is_object()) {
    const auto &viewport = j["viewport_size"];
    metadata.viewport_size.x = viewport.value("x", 0.0f);
    metadata.viewport_size.y = viewport.value("y", 0.0f);
    metadata.has_viewport_size =
        metadata.viewport_size.x > 0.0f && metadata.viewport_size.y > 0.0f;
  }

  if (j.contains("tool_runtime") && j["tool_runtime"].is_object()) {
    const auto &runtime = j["tool_runtime"];
    metadata.has_tool_runtime_state = true;
    metadata.viewport_selection_mode =
        runtime.value("viewport_selection_mode", 0);
    metadata.viewport_edit_tool = runtime.value("viewport_edit_tool", 0);
    metadata.viewport_selection_target =
        runtime.value("viewport_selection_target", 0);
  }
  return metadata;
}

[[nodiscard]] static PresetEntry::Metadata CapturePresetMetadata() {
  PresetEntry::Metadata metadata;
  metadata.saved_at_utc = CurrentUtcIso8601();

  const auto &gs = RogueCity::Core::Editor::GetGlobalState();
  metadata.has_tool_runtime_state = true;
  metadata.viewport_selection_mode =
    static_cast<int>(gs.tool_runtime.viewport_selection_mode);
  metadata.viewport_edit_tool =
    static_cast<int>(gs.tool_runtime.viewport_edit_tool);
  metadata.viewport_selection_target =
    static_cast<int>(gs.tool_runtime.viewport_selection_target);

  if (const ImGuiViewport *viewport = ImGui::GetMainViewport();
      viewport != nullptr && viewport->Size.x > 0.0f &&
      viewport->Size.y > 0.0f) {
    metadata.has_viewport_size = true;
    metadata.viewport_size = viewport->Size;
  }

#if defined(IMGUI_HAS_DOCK)
  ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
  metadata.monitor_count = std::max(1, platform_io.Monitors.Size);
#else
  metadata.monitor_count = 1;
#endif

  const ImGuiIO &io = ImGui::GetIO();
  metadata.docking_enabled =
      (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0;
  metadata.multi_viewport_enabled =
      (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
  return metadata;
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
        // Schema 2+: { "ini": "...", "theme": { ... }, "meta": { ... } }
        entry.ini = item.value().value("ini", std::string{});
        if (item.value().contains("theme") &&
            item.value()["theme"].is_object()) {
          entry.theme = ThemeProfileFromPresetJson(item.value()["theme"]);
          entry.has_theme = true;
        }
        if (item.value().contains("meta") && item.value()["meta"].is_object()) {
          entry.metadata = PresetMetadataFromJson(item.value()["meta"]);
          entry.has_metadata = true;
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
    j["schema"] = 3;
    j["presets"] = nlohmann::json::object();
    for (const auto &[name, entry] : store.presets) {
      nlohmann::json e;
      e["ini"] = entry.ini;
      if (entry.has_theme) {
        e["theme"] = ThemeProfileToPresetJson(entry.theme);
      }
      if (entry.has_metadata) {
        e["meta"] = PresetMetadataToJson(entry.metadata);
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
static std::array<ToolLibraryIconRenderer, kToolLibraryOrder.size()>
    s_tool_library_icon_renderers = {};
static ToolLibrary s_active_library_tool = ToolLibrary::Water;
static bool s_library_frame_open = false;
static DockLayoutPreferences s_dock_layout_preferences =
    GetDefaultDockLayoutPreferences();
static DockTreeProfile s_dock_tree_profile = DockTreeProfile::Adaptive;
static UiInputGateState s_last_input_gate{};

static size_t ToolLibraryIndex(ToolLibrary tool);

// Default icon implementation for tool libraries. Individual tools may
// override this via SetToolLibraryIconRenderer.
static void DrawDefaultToolLibraryIcon(ImDrawList *draw_list, ToolLibrary tool,
                                       const ImVec2 &center, float size,
                                       ImU32 color) {
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

static void DrawToolLibraryIcon(ImDrawList *draw_list, ToolLibrary tool,
                                const ImVec2 &center, float size) {
  const ImU32 color = UITokens::TextPrimary;
  const ToolLibraryIconRenderer custom_renderer =
      s_tool_library_icon_renderers[ToolLibraryIndex(tool)];
  if (custom_renderer != nullptr) {
    custom_renderer(draw_list, tool, center, size, color);
    return;
  }
  DrawDefaultToolLibraryIcon(draw_list, tool, center, size, color);
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

enum class MinimapHostMode : uint8_t {
  OverlayOnly = 0,
  OverlayAndStandaloneWindow
};

// Viewport services (currently minimap) are centrally managed here so host
// behavior can evolve without rewiring panel code.
static std::unique_ptr<RogueCity::App::MinimapViewport> s_minimap;
static MinimapHostMode s_minimap_host_mode = MinimapHostMode::OverlayOnly;
static bool s_dock_built = false;
static bool s_dock_layout_dirty = true;
static bool s_legacy_window_settings_cleared = false;
static ImVec2 s_last_layout_viewport_size{0.0f, 0.0f};

// ── Global chrome state ───────────────────────────────────────────────────────
static bool s_scanlines_enabled = true;   // View > Toggle Scanlines
static bool s_show_about_modal  = false;  // Help > About
static bool s_show_new_confirm  = false;  // File > New City confirmation

// Platform-agnostic file/directory opener (delegates to shell).
static void OpenOSPath(const char *path) {
#if defined(_WIN32)
  // Use cmd /c start so we don't need to link Shell32 directly.
  std::string cmd = std::string("start \"\" \"") + path + "\"";
  system(cmd.c_str());
#elif defined(__APPLE__)
  std::string cmd = std::string("open \"") + path + "\"";
  system(cmd.c_str());
#else
  std::string cmd = std::string("xdg-open \"") + path + "\"";
  system(cmd.c_str());
#endif
}

namespace {
constexpr float kMinCenterRatio = 0.40f;
constexpr int kDockBuildStableFrameCount = 3;
constexpr float kDockSizeStabilityTolerancePx = 2.0f;
constexpr float kDockBuildHardMinWidth = 64.0f;
constexpr float kDockBuildHardMinHeight = 64.0f;

[[nodiscard]] DockTreeProfile
ResolveDockTreeProfile(const ImGuiViewport *viewport) {
  if (s_dock_tree_profile != DockTreeProfile::Adaptive) {
    return s_dock_tree_profile;
  }
  if (viewport == nullptr || viewport->Size.x <= 0.0f ||
      viewport->Size.y <= 0.0f) {
    return DockTreeProfile::StandardThreeColumn;
  }

  const float aspect = viewport->Size.x / std::max(1.0f, viewport->Size.y);
  if (aspect < 1.35f) {
    return DockTreeProfile::FocusViewport;
  }

#if defined(IMGUI_HAS_DOCK)
  ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
  const int monitor_count = std::max(1, platform_io.Monitors.Size);
  if (monitor_count > 1 && aspect > 1.65f) {
    return DockTreeProfile::WideCenter;
  }
#endif

  if (aspect > 2.10f) {
    return DockTreeProfile::WideCenter;
  }
  return DockTreeProfile::StandardThreeColumn;
}

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

// DefaultDockTree — describes the 6-zone layout for the UI introspector.
// This is the canonical representation of BuildDefaultWorkspace() used by
// snapshot / headless tooling.  It is profile-independent (the new layout
// no longer changes shape with viewport size).
static RogueCity::UIInt::DockTreeNode
DefaultDockTree(DockTreeProfile /*profile*/) {
  using RogueCity::UIInt::DockTreeNode;

  // ── Chrome (outside dockspace) ────────────────────────────────────────────
  DockTreeNode titlebar;
  titlebar.id       = "titlebar";
  titlebar.panel_id = "Rogue Titlebar";

  DockTreeNode statusbar;
  statusbar.id       = "statusbar";
  statusbar.panel_id = "Rogue Status Bar";

  // ── B1: far-left activity bar ─────────────────────────────────────────────
  DockTreeNode b1;
  b1.id       = "b1_icons";
  b1.panel_id = "ActivityBarLeft";

  // ── P3: tool panel / explorer ─────────────────────────────────────────────
  DockTreeNode p3;
  p3.id       = "p3_tools";
  p3.panel_id = "Master Panel";

  // ── Center: sacred viewport ───────────────────────────────────────────────
  DockTreeNode viz_bar;
  viz_bar.id       = "viz_bar";
  viz_bar.panel_id = "VisualizerBar";

  DockTreeNode center;
  center.id       = "viewport";
  center.panel_id = "[/ / / RC_VISUALIZER / / /]";

  DockTreeNode center_col;
  center_col.id          = "center_column";
  center_col.orientation = "vertical";
  center_col.children    = {viz_bar, center};

  // ── P4: inspector & diagnostics ───────────────────────────────────────────
  DockTreeNode p4;
  p4.id       = "p4_inspector";
  p4.panel_id = "Inspector";

  // ── B2: far-right activity bar ────────────────────────────────────────────
  DockTreeNode b2;
  b2.id       = "b2_icons";
  b2.panel_id = "ActivityBarRight";

  // ── P5: system / terminals (bottom) ──────────────────────────────────────
  DockTreeNode p5;
  p5.id          = "p5_system";
  p5.panel_id    = "Dev Shell";
  p5.orientation = "horizontal"; // tabs share the bottom band

  // ── Main horizontal band (excluding P5) ───────────────────────────────────
  DockTreeNode h_band;
  h_band.id          = "h_band";
  h_band.orientation = "horizontal";
  h_band.children    = {b1, p3, center_col, p4, b2};

  // ── Root: vertical stack (h_band on top, P5 on bottom) ───────────────────
  DockTreeNode root;
  root.id          = "root";
  root.orientation = "vertical";
  root.children    = {titlebar, h_band, p5, statusbar};

  return root;
}

// Initialization of the minimap viewport instance, with default settings.
void EnsureMinimapService() {
  if (!s_minimap) {
    s_minimap = std::make_unique<RogueCity::App::MinimapViewport>();
    s_minimap->initialize();
    s_minimap->set_size(RogueCity::App::MinimapViewport::Size::Medium);
  }
}

void UpdateViewportServices(float dt) {
  if (s_minimap == nullptr) {
    return;
  }
  s_minimap->update(dt);
  if (s_minimap_host_mode == MinimapHostMode::OverlayAndStandaloneWindow) {
    s_minimap->render();
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

DockTreeProfile GetDockTreeProfile() { return s_dock_tree_profile; }

void SetDockTreeProfile(DockTreeProfile profile) {
  if (s_dock_tree_profile == profile) {
    return;
  }
  s_dock_tree_profile = profile;
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

void SetToolLibraryIconRenderer(ToolLibrary tool,
                                ToolLibraryIconRenderer renderer) {
  s_tool_library_icon_renderers[ToolLibraryIndex(tool)] = renderer;
}

void ClearToolLibraryIconRenderer(ToolLibrary tool) {
  s_tool_library_icon_renderers[ToolLibraryIndex(tool)] = nullptr;
}

void ClearAllToolLibraryIconRenderers() {
  s_tool_library_icon_renderers.fill(nullptr);
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

// BuildDefaultWorkspace — mathematically enforces the ASCII layout:
//
//  [B1] [P3 Tool Panel]  [Viewport]  [P4 Inspector]  [B2]
//       [P5 System / Terminals — bottom collapsible       ]
//
// Slicing order is strict (each split operates on the remainder):
//  1. DOWN  0.25 → P5 System
//  2. LEFT  0.04 → B1 Activity Bar (rigid, no-resize)
//  3. LEFT  0.20 → P3 Tool Panel
//  4. RIGHT 0.04 → B2 Activity Bar (rigid, no-resize)
//  5. RIGHT 0.25 → P4 Inspector
//  6. UP    0.05 → Visualizer Context Bar
//  remainder     → Central Viewport
static void BuildDefaultWorkspace(ImGuiID dockspace_id) {
#if defined(IMGUI_HAS_DOCK)
  if (dockspace_id == 0) {
    return;
  }
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  if (viewport == nullptr || viewport->WorkSize.x <= 0.0f ||
      viewport->WorkSize.y <= 0.0f) {
    return;
  }

  // Clear any previous layout including docked windows so a fresh split is
  // deterministic on every call (e.g. after a ResetDockLayout()).
  ImGui::DockBuilderRemoveNodeDockedWindows(dockspace_id, true);
  ImGui::DockBuilderRemoveNode(dockspace_id);
  ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode);
  ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

  ImGuiID id_main = dockspace_id;

  // ── Step 1: Bottom — P5 System / Terminals ──────────────────────────────
  ImGuiID id_p5_system = 0;
  ImGui::DockBuilderSplitNode(id_main, ImGuiDir_Down, 0.25f,
                               &id_p5_system, &id_main);

  // ── Step 2: Left — B1 Activity Bar (far left, rigid icon strip) ─────────
  ImGuiID id_b1_icons = 0;
  ImGui::DockBuilderSplitNode(id_main, ImGuiDir_Left, 0.04f,
                               &id_b1_icons, &id_main);

  // ── Step 3: Left — P3 Tool Panel / Explorer ─────────────────────────────
  ImGuiID id_p3_tools = 0;
  ImGui::DockBuilderSplitNode(id_main, ImGuiDir_Left, 0.20f,
                               &id_p3_tools, &id_main);

  // ── Step 4: Right — B2 Activity Bar (far right, rigid icon strip) ────────
  ImGuiID id_b2_icons = 0;
  ImGui::DockBuilderSplitNode(id_main, ImGuiDir_Right, 0.04f,
                               &id_b2_icons, &id_main);

  // ── Step 5: Right — P4 Inspector & Diagnostics ──────────────────────────
  ImGuiID id_p4_inspector = 0;
  ImGui::DockBuilderSplitNode(id_main, ImGuiDir_Right, 0.25f,
                               &id_p4_inspector, &id_main);

  // ── Step 6: Top — Visualizer Context / Info Bar ─────────────────────────
  ImGuiID id_viz_bar = 0;
  ImGui::DockBuilderSplitNode(id_main, ImGuiDir_Up, 0.05f,
                               &id_viz_bar, &id_main);
  // id_main is now the sacred central viewport.

  // ── Node constraints: activity bars are rigid (VS Code feel) ─────────────
  if (ImGuiDockNode *n = ImGui::DockBuilderGetNode(id_b1_icons)) {
    n->LocalFlags |= static_cast<ImGuiDockNodeFlags>(static_cast<int>(ImGuiDockNodeFlags_NoResize) | static_cast<int>(ImGuiDockNodeFlags_NoTabBar));
  }
  if (ImGuiDockNode *n = ImGui::DockBuilderGetNode(id_b2_icons)) {
    n->LocalFlags |= static_cast<ImGuiDockNodeFlags>(static_cast<int>(ImGuiDockNodeFlags_NoResize) | static_cast<int>(ImGuiDockNodeFlags_NoTabBar));
  }

  // ── Window routing ────────────────────────────────────────────────────────
  // B1 / B2 — future activity bar windows (docked when first created)
  ImGui::DockBuilderDockWindow("ActivityBarLeft",  id_b1_icons);
  ImGui::DockBuilderDockWindow("ActivityBarRight", id_b2_icons);

  // P3 — Master Panel hosts all tools (HFSM-driven tab switcher)
  ImGui::DockBuilderDockWindow("Master Panel",   id_p3_tools);
  // Individual tool windows tab-share P3 when popped out or opened standalone
  ImGui::DockBuilderDockWindow("Axiom Editor",   id_p3_tools);
  ImGui::DockBuilderDockWindow("Road Editor",    id_p3_tools);
  ImGui::DockBuilderDockWindow("Zoning Control", id_p3_tools);

  // P4 — Inspector Sidebar tabs (Inspector / System Map / Validation)
  ImGui::DockBuilderDockWindow("Inspector",    id_p4_inspector);
  ImGui::DockBuilderDockWindow("System Map",   id_p4_inspector);
  ImGui::DockBuilderDockWindow("UI Settings",  id_p4_inspector);
  ImGui::DockBuilderDockWindow("Validation",   id_p4_inspector);

  // P5 — System / Terminals
  ImGui::DockBuilderDockWindow("Dev Shell",      id_p5_system);
  ImGui::DockBuilderDockWindow("Log",            id_p5_system);
  ImGui::DockBuilderDockWindow("AI Console",     id_p5_system);
  ImGui::DockBuilderDockWindow("City Spec",      id_p5_system);
  ImGui::DockBuilderDockWindow("UI Agent",       id_p5_system);

  // Center — Sacred Viewport
  ImGui::DockBuilderDockWindow("[/ / / RC_VISUALIZER / / /]", id_main);

  // ── Store node IDs for runtime dock requests (QueueDockWindow) ───────────
  s_dock_nodes.root        = dockspace_id;
  s_dock_nodes.b1_icons    = id_b1_icons;
  s_dock_nodes.p3_tools    = id_p3_tools;
  s_dock_nodes.p4_inspector= id_p4_inspector;
  s_dock_nodes.b2_icons    = id_b2_icons;
  s_dock_nodes.p5_system   = id_p5_system;
  s_dock_nodes.viz_bar     = id_viz_bar;
  s_dock_nodes.center      = id_main;
  // Legacy aliases — keep QueueDockWindow("...", "Left"/"Right"/"Bottom") valid
  s_dock_nodes.left        = id_p3_tools;
  s_dock_nodes.right       = id_p4_inspector;
  s_dock_nodes.bottom      = id_p5_system;
  s_dock_nodes.bottom_tabs = id_p5_system;
  s_dock_nodes.tool_deck   = 0;
  s_dock_nodes.library     = 0;
#else
  (void)dockspace_id;
  s_dock_nodes = {};
#endif
}

// Thin wrapper so UpdateDockLayout still calls a single named function.
static void BuildDockLayout(ImGuiID dockspace_id) {
  BuildDefaultWorkspace(dockspace_id);
}

static ImGuiID NodeForDockArea(const std::string &dock_area) {
  // New 6-zone names (preferred)
  if (dock_area == "P3" || dock_area == "Tools" || dock_area == "Left") {
    return s_dock_nodes.p3_tools ? s_dock_nodes.p3_tools : s_dock_nodes.left;
  }
  if (dock_area == "P4" || dock_area == "Inspector" || dock_area == "Right") {
    return s_dock_nodes.p4_inspector ? s_dock_nodes.p4_inspector : s_dock_nodes.right;
  }
  if (dock_area == "P5" || dock_area == "System" || dock_area == "Bottom") {
    return s_dock_nodes.p5_system ? s_dock_nodes.p5_system
           : (s_dock_nodes.bottom_tabs ? s_dock_nodes.bottom_tabs
                                       : s_dock_nodes.bottom);
  }
  if (dock_area == "B1" || dock_area == "ActivityBarLeft") {
    return s_dock_nodes.b1_icons;
  }
  if (dock_area == "B2" || dock_area == "ActivityBarRight") {
    return s_dock_nodes.b2_icons;
  }
  if (dock_area == "VizBar") {
    return s_dock_nodes.viz_bar;
  }
  if (dock_area == "Top") {
    return s_dock_nodes.center;
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

// ── About modal ───────────────────────────────────────────────────────────────
static void DrawAboutModal() {
  if (!s_show_about_modal) return;

  ImGui::OpenPopup("About RogueCities##modal");
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(480.0f, 0.0f), ImGuiCond_Always);

  if (ImGui::BeginPopupModal("About RogueCities##modal", &s_show_about_modal,
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::CyanAccent),
                       "RogueCities Urban Spatial Designer");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("A procedural urban planning & simulation platform.");
    ImGui::Spacing();
    ImGui::TextDisabled("Build:       %s %s", __DATE__, __TIME__);
    ImGui::TextDisabled("ImGui:       %s", IMGUI_VERSION);
    ImGui::TextDisabled("Docking:     %s",
#if defined(IMGUI_HAS_DOCK)
                        "enabled"
#else
                        "disabled"
#endif
    );
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "Layer 0: Pure data (core/)\n"
        "Layer 1: Generation algorithms (generators/)\n"
        "Layer 2: UI + visualizer (app/, visualizer/)");
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImGui::ColorConvertU32ToFloat4(UITokens::InfoBlue));
    ImGui::TextWrapped("Documentation: docs/00_index/README.md");
    ImGui::TextWrapped("Changelog:     CHANGELOG.md");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float btn_w = 100.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - btn_w) * 0.5f);
    if (ImGui::Button("Close", ImVec2(btn_w, 0))) {
      s_show_about_modal = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// ── New City confirmation modal ───────────────────────────────────────────────
static void DrawNewCityConfirmModal() {
  if (!s_show_new_confirm) return;

  ImGui::OpenPopup("New City##confirm");
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(320.0f, 0.0f), ImGuiCond_Always);

  if (ImGui::BeginPopupModal("New City##confirm", &s_show_new_confirm,
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::TextWrapped("This will clear all city data. Continue?");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("New City", ImVec2(120, 0))) {
      auto &hfsm = RogueCity::Core::Editor::GetEditorHFSM();
      auto &gs   = RogueCity::Core::Editor::GetGlobalState();
      hfsm.handle_event(RogueCity::Core::Editor::EditorEvent::NewProject, gs);
      s_show_new_confirm = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      s_show_new_confirm = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// DrawRuntimeTitlebar — renders as the native main-menu bar so ImGui
// automatically adjusts viewport->WorkPos/WorkSize for the dockspace.
// ALL menu items are fully functional — no visual-only stubs.
static void DrawRuntimeTitlebar() {
  using RogueCity::Core::Editor::GetEditorHFSM;
  using RogueCity::Core::Editor::GetGlobalState;
  using RogueCity::Core::Editor::EditorEvent;

  // Draw any open modal dialogs (must be called every frame before
  // EndMainMenuBar so the popup stack is consistent).
  DrawAboutModal();
  DrawNewCityConfirmModal();

  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  const bool open = ImGui::BeginMainMenuBar();
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{"Rogue Titlebar",
                                  "Titlebar",
                                  "titlebar",
                                  "Top",
                                  "visualizer/src/ui/rc_ui_root.cpp",
                                  {"titlebar", "chrome", "runtime", "menubar"}},
      open);

  if (open) {
    auto &hist = RogueCity::App::GetEditorCommandHistory();
    auto &gs   = GetGlobalState();
    auto &hfsm = GetEditorHFSM();

    // ── Keyboard shortcuts handled here (outside menus so they work globally)
    const ImGuiIO &io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) { if (hist.CanUndo()) hist.Undo(); }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) { if (hist.CanRedo()) hist.Redo(); }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) { SaveWorkspacePreset("default"); }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N, false)) { s_show_new_confirm = true; }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_GraveAccent, false)) {
      Panels::DevShell::Toggle();
    }

    // ── App icon / brand ──────────────────────────────────────────────────
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::CyanAccent), "RC");
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // ── File ─────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New City", "Ctrl+N")) {
        s_show_new_confirm = true;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Save Layout", "Ctrl+S")) {
        SaveWorkspacePreset("default");
      }
      if (ImGui::MenuItem("Load Layout")) {
        std::string err;
        LoadWorkspacePreset("default", &err);
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Open Workspace Folder")) {
        OpenOSPath("AI/docs/ui");
      }
      if (ImGui::MenuItem("Open Changelog")) {
        OpenOSPath("CHANGELOG.md");
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Quit", "Alt+F4")) {
        hfsm.handle_event(EditorEvent::Quit, gs);
      }
      ImGui::EndMenu();
    }

    // ── Edit ─────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Edit")) {
      const bool can_undo = hist.CanUndo();
      const bool can_redo = hist.CanRedo();

      // Show undo label if available
      const char *undo_label = "Undo";
      std::string undo_str;
      if (can_undo && hist.PeekUndo()) {
        undo_str = std::string("Undo: ") + hist.PeekUndo()->GetDescription();
        undo_label = undo_str.c_str();
      }
      if (ImGui::MenuItem(undo_label, "Ctrl+Z", false, can_undo)) {
        hist.Undo();
      }

      const char *redo_label = "Redo";
      std::string redo_str;
      if (can_redo && hist.PeekRedo()) {
        redo_str = std::string("Redo: ") + hist.PeekRedo()->GetDescription();
        redo_label = redo_str.c_str();
      }
      if (ImGui::MenuItem(redo_label, "Ctrl+Y", false, can_redo)) {
        hist.Redo();
      }

      ImGui::Separator();
      ImGui::MenuItem("Snap to Grid", nullptr,
                      &gs.params.snap_to_grid);
      ImGui::MenuItem("Show Grid Overlay", nullptr,
                      &gs.config.show_grid_overlay);
      ImGui::EndMenu();
    }

    // ── View ─────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Reset Layout")) {
        ResetDockLayout();
      }
      ImGui::MenuItem("Scanlines", nullptr, &s_scanlines_enabled);
      ImGui::Separator();

      // Layer visibility (directly bound to GlobalState fields)
      if (ImGui::BeginMenu("Layers")) {
        ImGui::MenuItem("Axioms",    nullptr, &gs.show_layer_axioms);
        ImGui::MenuItem("Water",     nullptr, &gs.show_layer_water);
        ImGui::MenuItem("Roads",     nullptr, &gs.show_layer_roads);
        ImGui::MenuItem("Districts", nullptr, &gs.show_layer_districts);
        ImGui::MenuItem("Lots",      nullptr, &gs.show_layer_lots);
        ImGui::MenuItem("Buildings", nullptr, &gs.show_layer_buildings);
        ImGui::EndMenu();
      }

      // Debug overlays
      if (ImGui::BeginMenu("Debug Overlays")) {
        ImGui::MenuItem("Tensor Field",  nullptr, &gs.debug_show_tensor_overlay);
        ImGui::MenuItem("Height Map",    nullptr, &gs.debug_show_height_overlay);
        ImGui::MenuItem("Zone Map",      nullptr, &gs.debug_show_zone_overlay);
        ImGui::EndMenu();
      }

      ImGui::Separator();

      // Panel focus shortcuts
      if (ImGui::MenuItem("Focus Tool Panel")) {
        Panels::RcMasterPanel::RequestCategory(Panels::PanelCategory::Tools);
      }
      if (ImGui::MenuItem("Focus Inspector")) {
        Panels::RcInspectorSidebar::RequestTab(0);
      }
      if (ImGui::MenuItem("Focus System Map")) {
        Panels::RcInspectorSidebar::RequestTab(1);
      }
      ImGui::EndMenu();
    }

    // ── Terminal ──────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Terminal")) {
      if (ImGui::MenuItem("Dev Shell", "Ctrl+`",
                          Panels::DevShell::IsOpen())) {
        Panels::DevShell::Toggle();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Explore Workspace Presets")) {
        OpenOSPath("AI/docs/ui");
      }
      if (ImGui::MenuItem("Explore AI Config")) {
        OpenOSPath("AI/config");
      }
      ImGui::EndMenu();
    }

    // ── Help ──────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("Documentation")) {
        OpenOSPath("docs/00_index/README.md");
      }
      if (ImGui::MenuItem("Changelog")) {
        OpenOSPath("CHANGELOG.md");
      }
      ImGui::Separator();
      if (ImGui::MenuItem("About RogueCities...")) {
        s_show_about_modal = true;
      }
      ImGui::EndMenu();
    }

    // ── Right-aligned: mode badge ─────────────────────────────────────────
    const std::string mode = ActiveModeFromHFSM();
    const float mode_width = ImGui::CalcTextSize(mode.c_str()).x + 20.0f;
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - mode_width);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::CyanAccent),
                       "%s", mode.c_str());

    uiint.RegisterWidget({"text", "Titlebar Brand",   "titlebar.brand",    {"titlebar"}});
    uiint.RegisterWidget({"text", "Mode Indicator",   "titlebar.mode",     {"titlebar"}});
    uiint.RegisterWidget({"menu", "File Menu",         "titlebar.file",     {"titlebar"}});
    uiint.RegisterWidget({"menu", "Edit Menu",         "titlebar.edit",     {"titlebar"}});
    uiint.RegisterWidget({"menu", "View Menu",         "titlebar.view",     {"titlebar"}});
    uiint.RegisterWidget({"menu", "Terminal Menu",     "titlebar.terminal", {"titlebar"}});
    uiint.RegisterWidget({"menu", "Help Menu",         "titlebar.help",     {"titlebar"}});
  }

  uiint.EndPanel();
  ImGui::EndMainMenuBar();
}

// DrawRuntimeStatusBar — renders as a viewport side bar pinned to the bottom.
// Using BeginViewportSideBar ensures ImGui adjusts WorkSize so the dockspace
// never overlaps this chrome. Must be called before the dockspace host Begin().
static void DrawRuntimeStatusBar() {
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  if (viewport == nullptr) {
    return;
  }

  constexpr float kStatusHeight = 28.0f;
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_NoNav;

  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  const bool open = ImGui::BeginViewportSideBar(
      "##RC_StatusBar", viewport, ImGuiDir_Down, kStatusHeight, flags);
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{"Rogue Status Bar",
                                  "Status Bar",
                                  "status",
                                  "Bottom",
                                  "visualizer/src/ui/rc_ui_root.cpp",
                                  {"status", "runtime", "sidebar"}},
      open);

  if (open) {
    auto &gs = RogueCity::Core::Editor::GetGlobalState();
    const int log_events        = Panels::Log::GetEventCount();
    const int validation_events = Panels::Validation::GetValidationEventCount();
    const bool validation_failed= Panels::Validation::HasValidationFailure();
    const bool dirty            = gs.dirty_layers.AnyDirty();

    const char *validation_state =
        validation_failed ? "REJECTED"
                          : (gs.plan_approved ? "APPROVED" : "PENDING");

    // Validation badge — colour-coded
    if (validation_failed) {
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed),
                         "  %s", validation_state);
    } else if (gs.plan_approved) {
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen),
                         "  %s", validation_state);
    } else {
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::AmberGlow),
                         "  %s", validation_state);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("| Validation: %d", validation_events);
    ImGui::SameLine();
    ImGui::TextDisabled("| Log: %d", log_events);
    ImGui::SameLine();
    ImGui::TextDisabled("| Dirty: %s", dirty ? "yes" : "no");

    // Right-aligned: active tool domain badge
    const std::string mode = ActiveModeFromHFSM();
    const float badge_w = ImGui::CalcTextSize(mode.c_str()).x + 24.0f;
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - badge_w);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::CyanAccent),
                       "[%s]", mode.c_str());

    uiint.RegisterWidget({"text", "Status Summary",    "status.summary",  {"status"}});
    uiint.RegisterWidget({"text", "Tool Domain Badge", "status.tool_domain", {"status"}});
  }

  uiint.EndPanel();
  ImGui::End();
}

static void DrawGlobalScanlines() {
  if (!s_scanlines_enabled) return;
  ImDrawList *bg_draw = ImGui::GetBackgroundDrawList();
  ImVec2 display_size = ImGui::GetIO().DisplaySize;
  const float time_sec = static_cast<float>(ImGui::GetTime());

  // Repeating scanlines mimicking the CSS linear-gradient
  const float scroll = time_sec * 10.0f;
  const float line_height = 6.0f;

  // We offset by negative line_height to hide the pop-in wrap
  for (float y = std::fmod(-scroll, line_height) - line_height;
       y < display_size.y; y += line_height) {
    bg_draw->AddRectFilled(ImVec2(0, y), ImVec2(display_size.x, y + 2.0f),
                           IM_COL32(0, 0, 0, 45));
  }
}

static void DrawToolLibraryWindows() {
  constexpr bool kEmbedLibrariesInMasterPanel = true;
  constexpr const char *kOwnerModule = "visualizer/src/ui/rc_ui_root.cpp";
  for (const ToolLibrary tool : kToolLibraryOrder) {
    const bool use_axiom_custom_content = (tool == ToolLibrary::Axiom);
    const auto action_catalog = Tools::GetToolActionsForLibrary(tool);
    const std::span<const Tools::ToolActionSpec> action_view =
        use_axiom_custom_content ? std::span<const Tools::ToolActionSpec>{}
                                 : action_catalog;
    const ToolLibraryContentRenderer content_renderer =
        use_axiom_custom_content
            ? ToolLibraryContentRenderer{[]() {
                Panels::AxiomEditor::DrawAxiomLibraryContent();
              }}
            : ToolLibraryContentRenderer{};

    const char *window_name = ToolLibraryWindowName(tool);
    if (!kEmbedLibrariesInMasterPanel) {
      RenderToolLibraryWindow(tool, window_name, kOwnerModule, "Library",
                              action_view, false, nullptr, content_renderer);
    }

    const size_t index = ToolLibraryIndex(tool);
    bool *popout_state = &s_tool_library_popout[index];
    if (*popout_state) {
      const std::string popout_name =
          std::string(window_name) + " (Popout)###ToolLibraryPopout_" +
          std::to_string(static_cast<int>(tool));
      RenderToolLibraryWindow(tool, popout_name.c_str(), kOwnerModule,
                              "Floating", action_view, true, popout_state,
                              content_renderer);
    }
  }
}

void DrawRoot(float dt) {
  // Global screen-space CRT FX
  DrawGlobalScanlines();

  // Initialize viewport services on first call.
  EnsureMinimapService();
  ClearLegacyPanelWindowSettingsOnce();

  // Dockspace host window
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  if (viewport == nullptr) {
    return;
  }

  // Begin UI introspection for this frame (dev-only tooling).
  auto &introspector = RogueCity::UIInt::UiIntrospector::Instance();
  introspector.BeginFrame(ActiveModeFromHFSM(),
                          RC_UI::Panels::DevShell::IsOpen());
  if (!s_dock_built) {
    introspector.SetDockTree(DefaultDockTree(ResolveDockTreeProfile(viewport)));
  }

  const ViewportBounds viewport_bounds{viewport->Pos.x, viewport->Pos.y,
                                       viewport->Size.x, viewport->Size.y};
  if (!viewport_bounds.IsValid()) {
    return;
  }

  // ── Viewport-level chrome ─────────────────────────────────────────────────
  // MUST be drawn before the dockspace host so that BeginMainMenuBar and
  // BeginViewportSideBar update viewport->WorkPos / WorkSize before we
  // position the host window.  These are raw ImGui chrome, never docked.
  DrawRuntimeTitlebar();   // BeginMainMenuBar  → adjusts WorkPos.y upward
  DrawRuntimeStatusBar();  // BeginViewportSideBar(Down) → adjusts WorkSize.y

#if defined(IMGUI_HAS_DOCK)
  // Dockspace host fills the work area left after the two chrome bars.
  ImGui::SetNextWindowPos(viewport->WorkPos,  ImGuiCond_Always);
  ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
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

  // MASTER PANEL SYSTEM (RC-0.10)
  // Single canonical UI path: dock host -> master panel (drawer registry) ->
  // viewport. This avoids duplicate tool surfaces from mixed
  // direct/window/schema render flows.

  // Initialize registry once
  if (!s_registry_initialized) {
    RC_UI::Panels::InitializePanelRegistry();
    s_master_panel = std::make_unique<RC_UI::Panels::RcMasterPanel>();
    s_inspector_sidebar = std::make_unique<RC_UI::Panels::RcInspectorSidebar>();
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

  // ── Activity bars (B1 / B2) ──────────────────────────────────────────────
  // Drawn as standalone docked windows (routed to b1_icons / b2_icons nodes).
  // B1 fires HFSM events + drives P3 category.  B2 routes P4 tabs.
  Panels::ActivityBarLeft::Draw(dt);
  Panels::ActivityBarRight::Draw(dt);

  // Primary viewport (AxiomEditor) owns the center workspace rendering.
  Panels::AxiomEditor::Draw(dt);

  // P4 Inspector sidebar (Inspector / System Map / Health tabs).
  if (s_inspector_sidebar) {
    s_inspector_sidebar->Draw(dt);
  }
  DrawToolLibraryWindows();
  // Panels::Telemetry::Draw(dt);
  // ... (18 more panels)

  // Update viewport services after UI composition. Minimap can run purely as an
  // overlay service or as an additional standalone viewport window.
  UpdateViewportServices(dt);
}

RogueCity::App::MinimapViewport *GetMinimapViewport() {
  return s_minimap.get();
}

void SetMinimapStandaloneWindowEnabled(bool enabled) {
  s_minimap_host_mode = enabled ? MinimapHostMode::OverlayAndStandaloneWindow
                                : MinimapHostMode::OverlayOnly;
  if (enabled) {
    QueueDockWindow("Minimap", "Right", true);
  }
}

bool IsMinimapStandaloneWindowEnabled() {
  return s_minimap_host_mode == MinimapHostMode::OverlayAndStandaloneWindow;
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
  entry.ini = std::string(ini_data, ini_size);
  entry.theme = RogueCity::UI::ThemeManager::Instance().GetActiveTheme();
  entry.has_theme = true;
  entry.metadata = CapturePresetMetadata();
  entry.has_metadata = true;
  store.presets[presetName] = std::move(entry);

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

  if (entry.has_metadata && entry.metadata.has_tool_runtime_state) {
    auto &gs = RogueCity::Core::Editor::GetGlobalState();
    const int selection_mode =
      std::clamp(entry.metadata.viewport_selection_mode, 0, 2);
    const int edit_tool = std::clamp(entry.metadata.viewport_edit_tool, 0, 2);
    const int selection_target =
      std::clamp(entry.metadata.viewport_selection_target, 0, 4);
    gs.tool_runtime.viewport_selection_mode =
        static_cast<RogueCity::Core::Editor::ViewportSelectionMode>(
        selection_mode);
    gs.tool_runtime.viewport_edit_tool =
        static_cast<RogueCity::Core::Editor::ViewportEditTool>(
        edit_tool);
    gs.tool_runtime.viewport_selection_target =
        static_cast<RogueCity::Core::Editor::ViewportSelectionTarget>(
        selection_target);
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

std::vector<WorkspacePresetMetadata> ListWorkspacePresetMetadata() {
  WorkspacePresetStore store;
  LoadWorkspacePresetStore(store);

  std::vector<WorkspacePresetMetadata> metadata;
  metadata.reserve(store.presets.size());
  for (const auto &[name, entry] : store.presets) {
    WorkspacePresetMetadata item;
    item.name = name;
    item.has_theme = entry.has_theme;
    if (entry.has_theme) {
      item.theme_name = entry.theme.name;
    }
    if (entry.has_metadata) {
      item.saved_at_utc = entry.metadata.saved_at_utc;
      item.has_viewport_size = entry.metadata.has_viewport_size;
      item.viewport_size = entry.metadata.viewport_size;
      item.monitor_count = std::max(1, entry.metadata.monitor_count);
      item.docking_enabled = entry.metadata.docking_enabled;
      item.multi_viewport_enabled = entry.metadata.multi_viewport_enabled;
    }
    metadata.push_back(std::move(item));
  }

  std::sort(
      metadata.begin(), metadata.end(),
      [](const WorkspacePresetMetadata &a, const WorkspacePresetMetadata &b) {
        return a.name < b.name;
      });
  return metadata;
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
