// FILE: rc_ui_root.cpp - Editor dockspace and window routing

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
// #include "ui/panels/rc_panel_inspector.h" // inspector not used in this layout
#include "ui/rc_ui_theme.h"

#include <imgui.h>

namespace RC_UI {

// Fullscreen dock host window flags
constexpr ImGuiWindowFlags kHostFlags = ImGuiWindowFlags_NoDocking |
                                        ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus;

void DrawRoot(float dt)
{
    // Acquire main viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Create a transparent host window for the dockspace
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("RcUiDockHost", nullptr, kHostFlags);
    ImGui::PopStyleVar(2);

    // Create dockspace and default layout if not already built
    const ImGuiID dockspace_id = ImGui::GetID("RcUiDockSpace");
    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        // Top bar for axioms/modes
        ImGuiID dock_top = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.08f, nullptr, &dockspace_id);

        // Bottom area for indexes and log
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.32f, nullptr, &dockspace_id);
        // Further split bottom for log (bottom quarter) and indexes (rest)
        ImGuiID dock_log = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Down, 0.25f, nullptr, &dock_bottom);

        // Split middle area horizontally: tools (left), analytics (right), viewport (center)
        ImGuiID dock_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, nullptr, &dockspace_id);
        ImGuiID dock_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.22f, nullptr, &dockspace_id);

        // Split bottom index area into four equal parts
        ImGuiID dock_bottom_0 = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.25f, nullptr, &dock_bottom);
        ImGuiID dock_bottom_1 = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.333f, nullptr, &dock_bottom);
        ImGuiID dock_bottom_2 = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.50f, nullptr, &dock_bottom);
        // Remaining dock_bottom is last panel
        ImGuiID dock_bottom_3 = dock_bottom;

        // Dock windows into layout
        ImGui::DockBuilderDockWindow("Axiom Bar", dock_top);
        ImGui::DockBuilderDockWindow("Tools", dock_left);
        ImGui::DockBuilderDockWindow("Analytics", dock_right);
        ImGui::DockBuilderDockWindow("System Map", dockspace_id);

        ImGui::DockBuilderDockWindow("District Index", dock_bottom_0);
        ImGui::DockBuilderDockWindow("Road Index", dock_bottom_1);
        ImGui::DockBuilderDockWindow("Lot Index", dock_bottom_2);
        ImGui::DockBuilderDockWindow("River Index", dock_bottom_3);

        ImGui::DockBuilderDockWindow("Log", dock_log);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    // Render the dockspace (central node is transparent)
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End(); // RcUiDockHost

    // Draw each panel. Order here doesn't matter for docking.
    Panels::AxiomBar::Draw(dt);
    Panels::SystemMap::Draw(dt);
    Panels::Telemetry::Draw(dt);     // renamed window title shows "Analytics"
    Panels::Log::Draw(dt);

    Panels::Tools::Draw(dt);
    Panels::DistrictIndex::Draw(dt);
    Panels::RoadIndex::Draw(dt);
    Panels::LotIndex::Draw(dt);
    Panels::RiverIndex::Draw(dt);
}

} // namespace RC_UI