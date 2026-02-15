// FILE: RcMasterPanel.h
// PURPOSE: Master panel container with hybrid tabs+search navigation
// PATTERN: Single ImGui window hosting pluggable drawers via registry

#pragma once

#include "IPanelDrawer.h"
#include "PanelRegistry.h"
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace RC_UI::Panels {

// Master panel manages the root container and routes to drawers
class RcMasterPanel {
public:
    RcMasterPanel();
    
    // Main draw function (called once per frame)
    void Draw(float dt);
    
    // Programmatically select a panel (for external navigation)
    void SetActivePanel(PanelType type);
    
    // Get current active panel
    [[nodiscard]] PanelType GetActivePanel() const { return m_active_panel; }
    
    // Popout management
    void SetPopout(PanelType type, bool popout);
    [[nodiscard]] bool IsPopout(PanelType type) const;
    
    // Search overlay
    void OpenSearch();
    void CloseSearch();
    [[nodiscard]] bool IsSearchOpen() const { return m_search_open; }
    
private:
    // UI rendering
    void DrawTabBar(DrawContext& ctx);
    void DrawSearchOverlay(DrawContext& ctx);
    void DrawActiveDrawer(DrawContext& ctx);
    void HandlePopouts(DrawContext& ctx);
    void DrawCategoryTab(PanelCategory cat, const std::vector<PanelType>& panels);
    
    // Search filtering
    std::vector<PanelType> FilterPanelsBySearch(const std::string& query);
    void SelectSearchResult(int index);
    
    // State
    PanelType m_active_panel = PanelType::RoadIndex;  // Default to first index panel
    PanelCategory m_active_category = PanelCategory::Indices;
    PanelType m_context_menu_target = PanelType::RoadIndex;  // For right-click context menu
    
    // Popout tracking (true = floating window, false = embedded)
    std::unordered_map<PanelType, bool> m_popout_states;
    
    // Search state
    char m_search_filter[128] = {};
    bool m_search_open = false;
    int m_search_selected_index = 0;
    std::vector<PanelType> m_search_results;
    
    // Activation tracking for lifecycle hooks
    PanelType m_last_active_panel = PanelType::RoadIndex;
    std::unordered_set<PanelType> m_active_popouts;
    
    // Master panel window state
    bool m_master_window_open = true;
};

} // namespace RC_UI::Panels
