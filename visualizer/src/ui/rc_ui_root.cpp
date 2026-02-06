// FILE: visualizer/src/ui/rc_ui_root.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Fullscreen dockspace + panel routing.
#include "ui/rc_ui_root.h"

#include "ui/panels/rc_panel_axiom_bar.h"
#include "ui/panels/rc_panel_inspector.h"
#include "ui/panels/rc_panel_log.h"
#include "ui/panels/rc_panel_system_map.h"
#include "ui/panels/rc_panel_telemetry.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>

namespace RC_UI {

constexpr ImGuiWindowFlags kHostFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus;

void DrawRoot(float dt)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("RcUiDockHost", nullptr, kHostFlags);
    ImGui::PopStyleVar(2);

    // ADDED (visualizer/src/ui/rc_ui_root.cpp): Single fullscreen dockspace with default layout.
    const ImGuiID dockspace_id = ImGui::GetID("RcUiDockSpace");
    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        ImGuiID dock_main = dockspace_id;
        ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.24f, nullptr, &dock_main);
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.26f, nullptr, &dock_main);
        ImGuiID dock_bottom_left = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.40f, nullptr, &dock_bottom);
        ImGuiID dock_top = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Up, 0.14f, nullptr, &dock_main);

        ImGui::DockBuilderDockWindow("Axiom Bar", dock_top);
        ImGui::DockBuilderDockWindow("System Map", dock_main);
        ImGui::DockBuilderDockWindow("Telemetry", dock_right);
        ImGui::DockBuilderDockWindow("Inspector", dock_bottom_left);
        ImGui::DockBuilderDockWindow("Log", dock_bottom);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    Panels::AxiomBar::Draw(dt);
    Panels::SystemMap::Draw(dt);
    Panels::Telemetry::Draw(dt);
    Panels::Inspector::Draw(dt);
    Panels::Log::Draw(dt);
}

} // namespace RC_UI
