// FILE: rc_panel_data_index_traits.h
// PURPOSE: Trait specializations for RcDataIndexPanel template
// PROVIDES: Road, District, and Lot index trait implementations

#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "ui/patterns/rc_ui_data_index_panel.h"
#include "ui/viewport/rc_viewport_overlays.h"
#include <cstddef>
#include <iterator>
#include <sstream>

namespace RC_UI::Panels {
namespace {
using RogueCity::Core::Editor::GetGlobalState;
using RogueCity::Core::Editor::SelectionManager;
using RogueCity::Core::Editor::VpEntityKind;

void ApplySelectionModifier(SelectionManager &selection, VpEntityKind kind,
                            uint32_t id) {
  const ImGuiIO &io = ImGui::GetIO();
  if (io.KeyShift) {
    selection.Add(kind, id);
  } else if (io.KeyCtrl) {
    selection.Toggle(kind, id);
  } else {
    selection.Select(kind, id);
  }
}

template <typename T, typename Predicate>
void EraseIfFva(fva::Container<T> &container, Predicate &&predicate) {
  for (size_t data_index = container.size(); data_index > 0; --data_index) {
    const size_t idx = data_index - 1;
    auto it = container.begin();
    std::advance(it, static_cast<std::ptrdiff_t>(idx));
    if (!predicate(*it)) {
      continue;
    }

    auto handle = container.createHandleFromData(idx);
    container.remove(handle);
  }
}

} // namespace

// ============================================================================
// ROAD INDEX TRAITS
// ============================================================================

struct RoadIndexTraits {
  using EntityType = RogueCity::Core::Road;
  using ContainerType = fva::Container<EntityType>;

  static ContainerType &GetData(RogueCity::Core::Editor::GlobalState &gs) {
    return gs.roads;
  }

  static int GetColumnCount() { return 3; }
  static const char *GetColumnHeader(int column) {
    switch (column) {
    case 0:
      return "ID";
    case 1:
      return "Type";
    case 2:
      return "Points";
    default:
      return "";
    }
  }

  static void RenderCell(EntityType &road, size_t /*index*/, int column) {
    switch (column) {
    case 0:
      ImGui::Text("%u", road.id);
      break;
    case 1:
      ImGui::TextUnformatted(road.is_user_created ? "User" : "Gen");
      break;
    case 2:
      ImGui::Text("%zu", road.points.size());
      break;
    }
  }

  static const char *GetPanelTitle() { return "Road Index"; }
  static const char *GetDataName() { return "Roads"; }

  static std::vector<std::string> GetTags() { return {"roads", "index"}; }

  static bool FilterEntity(const EntityType &road, const std::string &filter) {
    std::string id_str = std::to_string(road.id);
    return id_str.find(filter) != std::string::npos ||
           (road.is_user_created && filter.find("user") != std::string::npos) ||
           (!road.is_user_created && filter.find("gen") != std::string::npos);
  }

  static void ShowContextMenu(EntityType &road, size_t index) {
    if (ImGui::MenuItem("Delete Road")) {
      auto &gs = GetGlobalState();
      auto handle = gs.roads.createHandleFromData(index);
      gs.roads.remove(handle);
      gs.selection.selected_road = {};
      gs.selection_manager.Clear();
    }
    if (ImGui::MenuItem("Focus on Map")) {
      if (!road.points.empty()) {
        const auto midpoint = road.points[road.points.size() / 2];
        RC_UI::Viewport::GetViewportOverlays().SetSelectedLot(midpoint);
      }
    }
  }

  static void OnEntitySelected(EntityType &road, size_t index) {
    auto &gs = GetGlobalState();
    gs.selection.selected_road = gs.roads.createHandleFromData(index);
    gs.selection.selected_district = {};
    gs.selection.selected_lot = {};
    gs.selection.selected_building = {};
    ApplySelectionModifier(gs.selection_manager, VpEntityKind::Road, road.id);
    if (!road.points.empty()) {
      const auto midpoint = road.points[road.points.size() / 2];
      RC_UI::Viewport::GetViewportOverlays().SetSelectedLot(midpoint);
    }
  }

  static void OnEntityHovered(EntityType &road, size_t index) {
    (void)index;
    ImGui::BeginTooltip();
    ImGui::Text("Road ID: %u", road.id);
    bool is_user = road.is_user_created;
    if (ImGui::Checkbox("User Created", &is_user)) {
      road.is_user_created = is_user;
      GetGlobalState().dirty_layers.MarkDirty(
          RogueCity::Core::Editor::DirtyLayer::Roads);
    }
    ImGui::Text("Points: %zu", road.points.size());
    ImGui::EndTooltip();
    if (!road.points.empty()) {
      const auto midpoint = road.points[road.points.size() / 2];
      RC_UI::Viewport::GetViewportOverlays().SetHoveredLot(midpoint);
    }
  }
};

// ============================================================================
// DISTRICT INDEX TRAITS
// ============================================================================

struct DistrictIndexTraits {
  using EntityType = RogueCity::Core::District;
  using ContainerType = fva::Container<EntityType>;

  static ContainerType &GetData(RogueCity::Core::Editor::GlobalState &gs) {
    return gs.districts;
  }

  static int GetColumnCount() { return 3; }
  static const char *GetColumnHeader(int column) {
    switch (column) {
    case 0:
      return "ID";
    case 1:
      return "Axiom";
    case 2:
      return "Border";
    default:
      return "";
    }
  }

  static void RenderCell(EntityType &district, size_t /*index*/, int column) {
    switch (column) {
    case 0:
      ImGui::Text("%u", district.id);
      break;
    case 1:
      ImGui::Text("%u", district.primary_axiom_id);
      break;
    case 2:
      ImGui::Text("%zu", district.border.size());
      break;
    }
  }

  static const char *GetPanelTitle() { return "District Index"; }
  static const char *GetDataName() { return "Districts"; }

  static std::vector<std::string> GetTags() { return {"districts", "index"}; }

  static bool FilterEntity(const EntityType &district,
                           const std::string &filter) {
    std::string id_str = std::to_string(district.id);
    std::string axiom_str = std::to_string(district.primary_axiom_id);
    return id_str.find(filter) != std::string::npos ||
           axiom_str.find(filter) != std::string::npos;
  }

  static void ShowContextMenu(EntityType &district, size_t index) {
    if (ImGui::MenuItem("Delete District")) {
      auto &gs = GetGlobalState();
      const uint32_t district_id = district.id;

      EraseIfFva(gs.blocks,
                 [district_id](const RogueCity::Core::BlockPolygon &block) {
                   return block.district_id == district_id;
                 });
      EraseIfFva(gs.lots, [district_id](const RogueCity::Core::LotToken &lot) {
        return lot.district_id == district_id;
      });
      gs.buildings.remove_if(
          [district_id](const RogueCity::Core::BuildingSite &building) {
            return building.district_id == district_id;
          });

      auto handle = gs.districts.createHandleFromData(index);
      gs.districts.remove(handle);
      gs.selection.selected_district = {};
      gs.selection_manager.Clear();
    }
    if (ImGui::MenuItem("Focus on Map")) {
      if (!district.border.empty()) {
        RC_UI::Viewport::GetViewportOverlays().SetSelectedLot(
            district.border[0]);
      }
    }
  }

  static void OnEntitySelected(EntityType &district, size_t index) {
    auto &gs = GetGlobalState();
    gs.selection.selected_road = {};
    gs.selection.selected_district = gs.districts.createHandleFromData(index);
    gs.selection.selected_lot = {};
    gs.selection.selected_building = {};
    ApplySelectionModifier(gs.selection_manager, VpEntityKind::District,
                           district.id);
    if (!district.border.empty()) {
      RC_UI::Viewport::GetViewportOverlays().SetSelectedLot(district.border[0]);
    }
  }

  static void OnEntityHovered(EntityType &district, size_t index) {
    (void)index;
    if (!district.border.empty()) {
      RC_UI::Viewport::GetViewportOverlays().SetHoveredLot(district.border[0]);
    }
  }
};

// ============================================================================
// LOT INDEX TRAITS
// ============================================================================

struct LotIndexTraits {
  using EntityType = RogueCity::Core::LotToken;
  using ContainerType = fva::Container<EntityType>;

  static ContainerType &GetData(RogueCity::Core::Editor::GlobalState &gs) {
    return gs.lots;
  }

  static int GetColumnCount() { return 2; }
  static const char *GetColumnHeader(int column) {
    switch (column) {
    case 0:
      return "ID";
    case 1:
      return "District";
    default:
      return "";
    }
  }

  static void RenderCell(EntityType &lot, size_t /*index*/, int column) {
    switch (column) {
    case 0:
      ImGui::Text("%u", lot.id);
      break;
    case 1:
      ImGui::Text("%u", lot.district_id);
      break;
    }
  }

  static const char *GetPanelTitle() { return "Lot Index"; }
  static const char *GetDataName() { return "Lots"; }

  static std::vector<std::string> GetTags() { return {"lots", "index"}; }

  static bool FilterEntity(const EntityType &lot, const std::string &filter) {
    std::string id_str = std::to_string(lot.id);
    std::string district_str = std::to_string(lot.district_id);
    return id_str.find(filter) != std::string::npos ||
           district_str.find(filter) != std::string::npos;
  }

  static void ShowContextMenu(EntityType &lot, size_t index) {
    if (ImGui::MenuItem("Delete Lot")) {
      auto &gs = GetGlobalState();
      const uint32_t lot_id = lot.id;
      gs.buildings.remove_if(
          [lot_id](const RogueCity::Core::BuildingSite &building) {
            return building.lot_id == lot_id;
          });
      auto handle = gs.lots.createHandleFromData(index);
      gs.lots.remove(handle);
      gs.selection.selected_lot = {};
      gs.selection_manager.Clear();
    }
    if (ImGui::MenuItem("Focus on Map")) {
      RC_UI::Viewport::GetViewportOverlays().SetSelectedLot(lot.centroid);
    }
  }

  static void OnEntitySelected(EntityType &lot, size_t index) {
    auto &gs = GetGlobalState();
    gs.selection.selected_road = {};
    gs.selection.selected_district = {};
    gs.selection.selected_lot = gs.lots.createHandleFromData(index);
    gs.selection.selected_building = {};
    ApplySelectionModifier(gs.selection_manager, VpEntityKind::Lot, lot.id);
    RC_UI::Viewport::GetViewportOverlays().SetSelectedLot(lot.centroid);
  }

  static void OnEntityHovered(EntityType &lot, size_t index) {
    (void)index;
    RC_UI::Viewport::GetViewportOverlays().SetHoveredLot(lot.centroid);
  }
};

// ============================================================================
// BUILDING INDEX TRAITS
// ============================================================================

struct BuildingIndexTraits {
  using EntityType = RogueCity::Core::BuildingSite;
  using ContainerType = siv::Vector<EntityType>;

  static ContainerType &GetData(RogueCity::Core::Editor::GlobalState &gs) {
    return gs.buildings;
  }

  static int GetColumnCount() { return 4; }
  static const char *GetColumnHeader(int column) {
    switch (column) {
    case 0:
      return "ID";
    case 1:
      return "Type";
    case 2:
      return "Lot";
    case 3:
      return "Src";
    default:
      return "";
    }
  }

  static void RenderCell(EntityType &building, size_t /*index*/, int column) {
    switch (column) {
    case 0:
      ImGui::Text("%u", building.id);
      break;
    case 1:
      ImGui::Text("%d", static_cast<int>(building.type));
      break;
    case 2:
      ImGui::Text("%u", building.lot_id);
      break;
    case 3:
      ImGui::TextUnformatted(building.is_user_placed ? "User" : "Gen");
      break;
    }
  }

  static const char *GetPanelTitle() { return "Building Index"; }
  static const char *GetDataName() { return "Buildings"; }

  static std::vector<std::string> GetTags() { return {"buildings", "index"}; }

  static bool FilterEntity(const EntityType &building,
                           const std::string &filter) {
    std::string id_str = std::to_string(building.id);
    std::string lot_str = std::to_string(building.lot_id);
    std::string district_str = std::to_string(building.district_id);
    return id_str.find(filter) != std::string::npos ||
           lot_str.find(filter) != std::string::npos ||
           district_str.find(filter) != std::string::npos ||
           (building.is_user_placed &&
            filter.find("user") != std::string::npos);
  }

  static void ShowContextMenu(EntityType &building, size_t index) {
    if (ImGui::MenuItem("Delete Building")) {
      auto &gs = GetGlobalState();
      gs.buildings.eraseViaData(static_cast<uint32_t>(index));
      gs.selection.selected_building = {};
      gs.selection_manager.Clear();
    }
    if (ImGui::MenuItem("Focus on Map")) {
      RC_UI::Viewport::GetViewportOverlays().SetSelectedBuilding(
          building.position);
    }
  }

  static void OnEntitySelected(EntityType &building, size_t index) {
    auto &gs = GetGlobalState();
    gs.selection.selected_road = {};
    gs.selection.selected_district = {};
    gs.selection.selected_lot = {};
    gs.selection.selected_building = gs.buildings.createHandleFromData(index);
    ApplySelectionModifier(gs.selection_manager, VpEntityKind::Building,
                           building.id);
    RC_UI::Viewport::GetViewportOverlays().SetSelectedBuilding(
        building.position);
  }

  static void OnEntityHovered(EntityType &building, size_t index) {
    (void)index;
    RC_UI::Viewport::GetViewportOverlays().SetHoveredBuilding(
        building.position);
  }
};

// ============================================================================
// WATER/RIVER INDEX TRAITS
// ============================================================================

struct RiverIndexTraits {
  using EntityType = RogueCity::Core::WaterBody;
  using ContainerType = fva::Container<EntityType>;

  static ContainerType &GetData(RogueCity::Core::Editor::GlobalState &gs) {
    return gs.waterbodies;
  }

  static int GetColumnCount() { return 5; }
  static const char *GetColumnHeader(int column) {
    switch (column) {
    case 0:
      return "ID";
    case 1:
      return "Type";
    case 2:
      return "Depth";
    case 3:
      return "Shore";
    case 4:
      return "Src";
    default:
      return "";
    }
  }

  static void RenderCell(EntityType &water, size_t /*index*/, int column) {
    switch (column) {
    case 0:
      ImGui::Text("%u", water.id);
      break;
    case 1: {
      using RogueCity::Core::WaterType;
      const char *label = "Unknown";
      switch (water.type) {
      case WaterType::Lake:
        label = "Lake";
        break;
      case WaterType::River:
        label = "River";
        break;
      case WaterType::Ocean:
        label = "Ocean";
        break;
      case WaterType::Pond:
        label = "Pond";
        break;
      }
      ImGui::TextUnformatted(label);
    } break;
    case 2:
      ImGui::Text("%.1fm", water.depth);
      break;
    case 3:
      ImGui::TextUnformatted(water.generate_shore ? "Yes" : "No");
      break;
    case 4:
      ImGui::TextUnformatted(water.is_user_placed ? "User" : "Gen");
      break;
    }
  }

  static const char *GetPanelTitle() { return "Water Index"; }
  static const char *GetDataName() { return "WaterBodies"; }

  static std::vector<std::string> GetTags() {
    return {"water", "rivers", "index"};
  }

  static bool FilterEntity(const EntityType &water, const std::string &filter) {
    // Filter specifically for rivers if no filter text, or fuzzy search ID/Type
    std::string id_str = std::to_string(water.id);
    return id_str.find(filter) != std::string::npos ||
           (water.is_user_placed && filter.find("user") != std::string::npos);
  }

  static void ShowContextMenu(EntityType &water, size_t index) {
    if (ImGui::MenuItem("Delete Body")) {
      auto &gs = GetGlobalState();
      auto handle =
          gs.waterbodies.createHandleFromData(static_cast<uint32_t>(index));
      gs.waterbodies.remove(handle);
      gs.dirty_layers.MarkDirty(
          RogueCity::Core::Editor::DirtyLayer::ViewportIndex);
    }
  }

  static void OnEntitySelected(EntityType &water, size_t /*index*/) {
    if (!water.boundary.empty()) {
      RC_UI::Viewport::GetViewportOverlays().SetSelectedLot(water.boundary[0]);
    }
  }

  static void OnEntityHovered(EntityType &water, size_t /*index*/) {
    if (!water.boundary.empty()) {
      RC_UI::Viewport::GetViewportOverlays().SetHoveredLot(water.boundary[0]);
    }
  }
};

} // namespace RC_UI::Panels
