// FILE: rc_ui_root.cpp - Editor window routing (non-docking version)

#include "ui/rc_ui_root.h"
#include "ui/panels/rc_panel_axiom_bar.h"
#include "ui/panels/rc_panel_axiom_editor.h"  // NEW: Integrated axiom editor
#include "ui/panels/rc_panel_system_map.h"
#include "ui/panels/rc_panel_telemetry.h"
#include "ui/panels/rc_panel_log.h"
#include "ui/panels/rc_panel_tools.h"
#include "ui/panels/rc_panel_district_index.h"
#include "ui/panels/rc_panel_road_index.h"
#include "ui/panels/rc_panel_lot_index.h"
#include "ui/panels/rc_panel_river_index.h"
#include "ui/panels/rc_panel_ai_console.h"  // NEW: AI bridge control
#include "RogueCity/App/Viewports/MinimapViewport.hpp"  // NEW: Minimap integration
#include "ui/rc_ui_theme.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <memory>

namespace RC_UI {

// AI Console panel instance (static wrapper)
namespace {
    static RogueCity::UI::AiConsolePanel s_ai_console_instance;
}

// Static minimap instance (Phase 5: Polish)
static std::unique_ptr<RogueCity::App::MinimapViewport> s_minimap;
static bool s_axiom_library_open = false;
static bool s_dock_built = false;

void InitializeMinim() {
    if (!s_minimap) {
        s_minimap = std::make_unique<RogueCity::App::MinimapViewport>();
        s_minimap->initialize();
        s_minimap->set_size(RogueCity::App::MinimapViewport::Size::Medium);
    }
}

bool IsAxiomLibraryOpen() {
    return s_axiom_library_open;
}

void ToggleAxiomLibrary() {
    s_axiom_library_open = !s_axiom_library_open;
}

static void BuildDockLayout(ImGuiID dockspace_id) {
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    ImGuiID dock_main = dockspace_id;
    ImGuiID dock_right = 0;
    ImGuiID dock_bottom = 0;
    ImGuiID dock_top = 0;
    ImGuiID dock_right_bottom = 0;
    ImGuiID dock_tools = 0;
    ImGuiID dock_bottom_tabs = 0;

    dock_top = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Up, 0.10f, nullptr, &dock_main);
    dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.27f, nullptr, &dock_main);
    dock_bottom = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.22f, nullptr, &dock_main);

    dock_right_bottom = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.45f, nullptr, &dock_right);
    dock_tools = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Down, 0.32f, nullptr, &dock_bottom_tabs);

    ImGui::DockBuilderDockWindow("Axiom Bar", dock_top);
    ImGui::DockBuilderDockWindow("RogueVisualizer", dock_main);
    // Minimap is now embedded as overlay in RogueVisualizer (removed from docking)
    ImGui::DockBuilderDockWindow("Analytics", dock_right);

    ImGui::DockBuilderDockWindow("Tools", dock_tools);
    ImGui::DockBuilderDockWindow("Log", dock_bottom_tabs);
    ImGui::DockBuilderDockWindow("District Index", dock_bottom_tabs);
    ImGui::DockBuilderDockWindow("Road Index", dock_bottom_tabs);
    ImGui::DockBuilderDockWindow("Lot Index", dock_bottom_tabs);
    ImGui::DockBuilderDockWindow("River Index", dock_bottom_tabs);

    ImGui::DockBuilderDockWindow("Axiom Library", dock_right);

    ImGui::DockBuilderFinish(dockspace_id);
}

void DrawRoot(float dt)
{
    // Initialize minimap on first call
    InitializeMinim();

    // Dockspace host window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    const ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("RogueDockHost", nullptr, host_flags);
    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("RogueDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    if (!s_dock_built) {
        BuildDockLayout(dockspace_id);
        s_dock_built = true;
    }

    ImGui::End();

    // Panels/windows (docked by name)
    Panels::AxiomBar::Draw(dt);
    Panels::AxiomEditor::Draw(dt);
    Panels::Telemetry::Draw(dt);
    Panels::Tools::Draw(dt);
    Panels::DistrictIndex::Draw(dt);
    Panels::RoadIndex::Draw(dt);
    Panels::LotIndex::Draw(dt);
    Panels::RiverIndex::Draw(dt);
    Panels::Log::Draw(dt);
    
    // AI Console (uses static instance)
    s_ai_console_instance.Render();

    // Minimap is now embedded as overlay in RogueVisualizer (no separate panel)
    // Still update the minimap viewport for shared camera sync
    if (s_minimap) {
        s_minimap->update(dt);
    }
}

RogueCity::App::MinimapViewport* GetMinimapViewport() {
    return s_minimap.get();
}

} // namespace RC_UI
