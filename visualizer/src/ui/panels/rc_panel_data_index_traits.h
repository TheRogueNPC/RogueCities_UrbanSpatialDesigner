// FILE: rc_panel_data_index_traits.h
// PURPOSE: Trait specializations for RcDataIndexPanel template
// PROVIDES: Road, District, and Lot index trait implementations

#pragma once

#include "ui/patterns/rc_ui_data_index_panel.h"
#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "ui/viewport/rc_viewport_overlays.h"
#include <sstream>

namespace RC_UI::Panels {

// ============================================================================
// ROAD INDEX TRAITS
// ============================================================================

struct RoadIndexTraits {
    using EntityType = RogueCity::Core::Road;
    using ContainerType = fva::Container<EntityType>;
    
    static ContainerType& GetData(RogueCity::Core::Editor::GlobalState& gs) {
        return gs.roads;
    }
    
    static std::string GetEntityLabel(const EntityType& road, size_t /*index*/) {
        std::ostringstream oss;
        oss << "Road " << road.id << " (" << (road.is_user_created ? "User" : "Generated") << ")";
        return oss.str();
    }
    
    static const char* GetPanelTitle() { return "Road Index"; }
    static const char* GetDataName() { return "Roads"; }
    
    static std::vector<std::string> GetTags() { 
        return {"roads", "index"}; 
    }
    
    static bool FilterEntity(const EntityType& road, const std::string& filter) {
        std::string id_str = std::to_string(road.id);
        return id_str.find(filter) != std::string::npos ||
               (road.is_user_created && filter.find("user") != std::string::npos) ||
               (!road.is_user_created && filter.find("gen") != std::string::npos);
    }
    
    static void ShowContextMenu(EntityType& road, size_t index) {
        if (ImGui::MenuItem("Delete Road")) {
            // TODO: Wire to delete function
        }
        if (ImGui::MenuItem("Focus on Map")) {
            // TODO: Wire to camera focus
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Inspect Properties")) {
            // TODO: Wire to inspector panel
        }
    }
    
    static void OnEntitySelected(EntityType& road, size_t index) {
        (void)index;
        if (!road.points.empty()) {
            const auto midpoint = road.points[road.points.size() / 2];
            RC_UI::Viewport::GetViewportOverlays().SetSelectedLot(midpoint);
        }
    }

    static void OnEntityHovered(EntityType& road, size_t index) {
        (void)index;
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
    
    static ContainerType& GetData(RogueCity::Core::Editor::GlobalState& gs) {
        return gs.districts;
    }
    
    static std::string GetEntityLabel(const EntityType& district, size_t /*index*/) {
        std::ostringstream oss;
        oss << "District " << district.id << " (Primary Axiom: " << district.primary_axiom_id << ")";
        return oss.str();
    }
    
    static const char* GetPanelTitle() { return "District Index"; }
    static const char* GetDataName() { return "Districts"; }
    
    static std::vector<std::string> GetTags() { 
        return {"districts", "index"}; 
    }
    
    static bool FilterEntity(const EntityType& district, const std::string& filter) {
        std::string id_str = std::to_string(district.id);
        std::string axiom_str = std::to_string(district.primary_axiom_id);
        return id_str.find(filter) != std::string::npos ||
               axiom_str.find(filter) != std::string::npos;
    }
    
    static void ShowContextMenu(EntityType& district, size_t index) {
        if (ImGui::MenuItem("Delete District")) {
            // TODO: Wire to delete function
        }
        if (ImGui::MenuItem("Focus on Map")) {
            // TODO: Wire to camera focus
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Show AESP Values")) {
            // TODO: Wire to AESP visualization
        }
        if (ImGui::MenuItem("Inspect Properties")) {
            // TODO: Wire to inspector panel
        }
    }
    
    static void OnEntitySelected(EntityType& district, size_t index) {
        (void)index;
        if (!district.border.empty()) {
            RC_UI::Viewport::GetViewportOverlays().SetSelectedLot(district.border[0]);
        }
    }

    static void OnEntityHovered(EntityType& district, size_t index) {
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
    
    static ContainerType& GetData(RogueCity::Core::Editor::GlobalState& gs) {
        return gs.lots;
    }
    
    static std::string GetEntityLabel(const EntityType& lot, size_t /*index*/) {
        std::ostringstream oss;
        oss << "Lot " << lot.id << " (District " << lot.district_id << ")";
        return oss.str();
    }
    
    static const char* GetPanelTitle() { return "Lot Index"; }
    static const char* GetDataName() { return "Lots"; }
    
    static std::vector<std::string> GetTags() { 
        return {"lots", "index"}; 
    }
    
    static bool FilterEntity(const EntityType& lot, const std::string& filter) {
        std::string id_str = std::to_string(lot.id);
        std::string district_str = std::to_string(lot.district_id);
        return id_str.find(filter) != std::string::npos ||
               district_str.find(filter) != std::string::npos;
    }
    
    static void ShowContextMenu(EntityType& lot, size_t index) {
        if (ImGui::MenuItem("Delete Lot")) {
            // TODO: Wire to delete function
        }
        if (ImGui::MenuItem("Focus on Map")) {
            // TODO: Wire to camera focus
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Show Lot Details")) {
            // TODO: Wire to lot detail view
        }
        if (ImGui::MenuItem("Inspect Properties")) {
            // TODO: Wire to inspector panel
        }
    }
    
    static void OnEntitySelected(EntityType& lot, size_t index) {
        (void)index;
        RC_UI::Viewport::GetViewportOverlays().SetSelectedLot(lot.centroid);
    }

    static void OnEntityHovered(EntityType& lot, size_t index) {
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
    
    static ContainerType& GetData(RogueCity::Core::Editor::GlobalState& gs) {
        return gs.buildings;
    }
    
    static std::string GetEntityLabel(const EntityType& building, size_t /*index*/) {
        std::ostringstream oss;
        oss << "Building " << building.id << " (Lot " << building.lot_id << ", District " << building.district_id << ")";
        return oss.str();
    }
    
    static const char* GetPanelTitle() { return "Building Index"; }
    static const char* GetDataName() { return "Buildings"; }
    
    static std::vector<std::string> GetTags() { 
        return {"buildings", "index"}; 
    }
    
    static bool FilterEntity(const EntityType& building, const std::string& filter) {
        std::string id_str = std::to_string(building.id);
        std::string lot_str = std::to_string(building.lot_id);
        std::string district_str = std::to_string(building.district_id);
        return id_str.find(filter) != std::string::npos ||
               lot_str.find(filter) != std::string::npos ||
               district_str.find(filter) != std::string::npos ||
               (building.is_user_placed && filter.find("user") != std::string::npos);
    }
    
    static void ShowContextMenu(EntityType& building, size_t index) {
        if (ImGui::MenuItem("Delete Building")) {
            // TODO: Wire to delete function
        }
        if (ImGui::MenuItem("Focus on Map")) {
            // TODO: Wire to camera focus
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Change Type")) {
            // TODO: Wire to type selector
        }
        if (ImGui::MenuItem("Show Site Info")) {
            // TODO: Wire to building detail view
        }
        if (ImGui::MenuItem("Inspect Properties")) {
            // TODO: Wire to inspector panel
        }
    }
    
    static void OnEntitySelected(EntityType& building, size_t index) {
        (void)index;
        RC_UI::Viewport::GetViewportOverlays().SetSelectedBuilding(building.position);
    }

    static void OnEntityHovered(EntityType& building, size_t index) {
        (void)index;
        RC_UI::Viewport::GetViewportOverlays().SetHoveredBuilding(building.position);
    }
};

} // namespace RC_UI::Panels

