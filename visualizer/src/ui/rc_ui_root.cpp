// FILE: rc_ui_root.cpp - Editor window routing (non-docking version)

#include "ui/rc_ui_root.h"
#include "ui/panels/rc_panel_axiom_bar.h"
#include "ui/panels/rc_panel_system_map.h"
#include "ui/panels/rc_panel_telemetry.h"
#include "ui/panels/rc_panel_log.h"
#include "ui/panels/rc_panel_tools.h"
#include "ui/panels/rc_panel_district_index.h"
#include "ui/panels/rc_panel_road_index.h"
#include "ui/panels/rc_panel_lot_index.h"
#include "ui/panels/rc_panel_river_index.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>

namespace RC_UI {

void DrawRoot(float dt)
{
    // Get display dimensions for window positioning
    const ImGuiIO& io = ImGui::GetIO();
    const float vp_width = io.DisplaySize.x;
    const float vp_height = io.DisplaySize.y;
    
    // Window sizing
    constexpr float kTopBarHeight = 60.0f;
    constexpr float kBottomHeight = 250.0f;
    constexpr float kLeftWidth = 220.0f;
    constexpr float kRightWidth = 280.0f;
    constexpr float kIndexWidth = 0.25f; // fraction of bottom area width
    
    // Top bar (Axiom Bar)
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(vp_width, kTopBarHeight));
    Panels::AxiomBar::Draw(dt);
    
    // Left panel (Tools)
    ImGui::SetNextWindowPos(ImVec2(0.0f, kTopBarHeight));
    ImGui::SetNextWindowSize(ImVec2(kLeftWidth, vp_height - kTopBarHeight - kBottomHeight));
    Panels::Tools::Draw(dt);
    
    // Right panel (Analytics/Telemetry)
    ImGui::SetNextWindowPos(ImVec2(vp_width - kRightWidth, kTopBarHeight));
    ImGui::SetNextWindowSize(ImVec2(kRightWidth, vp_height - kTopBarHeight - kBottomHeight));
    Panels::Telemetry::Draw(dt);
    
    // Center panel (System Map)
    const float center_x = kLeftWidth;
    const float center_width = vp_width - kLeftWidth - kRightWidth;
    const float center_height = vp_height - kTopBarHeight - kBottomHeight;
    ImGui::SetNextWindowPos(ImVec2(center_x, kTopBarHeight));
    ImGui::SetNextWindowSize(ImVec2(center_width, center_height));
    Panels::SystemMap::Draw(dt);
    
    // Bottom area - Index panels (4 equal-width panels)
    const float bottom_y = vp_height - kBottomHeight;
    const float index_height = kBottomHeight * 0.7f; // Leave room for log below
    const float index_panel_width = vp_width * kIndexWidth;
    
    ImGui::SetNextWindowPos(ImVec2(0.0f, bottom_y));
    ImGui::SetNextWindowSize(ImVec2(index_panel_width, index_height));
    Panels::DistrictIndex::Draw(dt);
    
    ImGui::SetNextWindowPos(ImVec2(index_panel_width, bottom_y));
    ImGui::SetNextWindowSize(ImVec2(index_panel_width, index_height));
    Panels::RoadIndex::Draw(dt);
    
    ImGui::SetNextWindowPos(ImVec2(2 * index_panel_width, bottom_y));
    ImGui::SetNextWindowSize(ImVec2(index_panel_width, index_height));
    Panels::LotIndex::Draw(dt);
    
    ImGui::SetNextWindowPos(ImVec2(3 * index_panel_width, bottom_y));
    ImGui::SetNextWindowSize(ImVec2(index_panel_width, index_height));
    Panels::RiverIndex::Draw(dt);
    
    // Bottom log panel (full width)
    const float log_height = kBottomHeight * 0.3f;
    ImGui::SetNextWindowPos(ImVec2(0.0f, bottom_y + index_height));
    ImGui::SetNextWindowSize(ImVec2(vp_width, log_height));
    Panels::Log::Draw(dt);
}

} // namespace RC_UI