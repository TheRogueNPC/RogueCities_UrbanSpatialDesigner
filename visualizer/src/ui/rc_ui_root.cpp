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
#include "ui/panels/rc_panel_building_index.h"  // NEW: Building index (RC-0.10)
#include "ui/panels/rc_panel_zoning_control.h"   // NEW: Zoning control (RC-0.10)
#include "ui/panels/rc_panel_lot_control.h"      // AI_INTEGRATION_TAG: V1_PASS1_TASK4_PANEL_WIRING
#include "ui/panels/rc_panel_building_control.h" // AI_INTEGRATION_TAG: V1_PASS1_TASK4_PANEL_WIRING
#include "ui/panels/rc_panel_water_control.h"    // AI_INTEGRATION_TAG: V1_PASS1_TASK4_PANEL_WIRING
#include "ui/panels/rc_panel_ai_console.h"  // NEW: AI bridge control
#include "ui/panels/rc_panel_ui_agent.h"    // NEW: UI Agent assistant (Phase 2)
#include "ui/panels/rc_panel_city_spec.h"   // NEW: CitySpec generator (Phase 3)
#include "ui/panels/rc_panel_dev_shell.h"
#include "ui/introspection/UiIntrospection.h"
#include "RogueCity/App/Viewports/MinimapViewport.hpp"  // NEW: Minimap integration
#include "ui/rc_ui_theme.h"
#include "RogueCity/Core/Editor/EditorState.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <memory>
#include <string>
#include <vector>

namespace RC_UI {

// AI Console panel instance (static wrapper)
namespace {
    static RogueCity::UI::AiConsolePanel s_ai_console_instance;
    static RogueCity::UI::UiAgentPanel s_ui_agent_instance;     // Phase 2
    static RogueCity::UI::CitySpecPanel s_city_spec_instance;   // Phase 3

    struct DockRequest {
        std::string window_name;
        std::string dock_area;
        bool own_dock_node = false;
    };

    struct DockLayoutNodes {
        ImGuiID root = 0;
        ImGuiID top = 0;
        ImGuiID left = 0;
        ImGuiID right = 0;
        ImGuiID center = 0;
        ImGuiID bottom = 0;
        ImGuiID bottom_tabs = 0;
    };

    static DockLayoutNodes s_dock_nodes{};
    static std::vector<DockRequest> s_pending_dock_requests;
}

// Static minimap instance (Phase 5: Polish)
static std::unique_ptr<RogueCity::App::MinimapViewport> s_minimap;
static bool s_axiom_library_open = false;
static bool s_dock_built = false;

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

static RogueCity::UIInt::DockTreeNode DefaultDockTree() {
    using RogueCity::UIInt::DockTreeNode;
    DockTreeNode root;
    root.id = "root";
    root.orientation = "vertical";

    DockTreeNode top;
    top.id = "top";
    top.panel_id = "Axiom Bar";

    DockTreeNode center;
    center.id = "center";
    center.panel_id = "RogueVisualizer";

    DockTreeNode analytics;
    analytics.id = "analytics";
    analytics.panel_id = "Analytics";

    DockTreeNode axiom_library;
    axiom_library.id = "axiom_library";
    axiom_library.panel_id = "Axiom Library";

    DockTreeNode right_column;
    right_column.id = "right_column";
    right_column.orientation = "vertical";
    right_column.children = {analytics, axiom_library};

    DockTreeNode main_row;
    main_row.id = "main_row";
    main_row.orientation = "horizontal";
    main_row.children = {center, right_column};

    DockTreeNode tools;
    tools.id = "tools";
    tools.panel_id = "Tools";

    DockTreeNode log;
    log.id = "log";
    log.panel_id = "Log";

    DockTreeNode district_index;
    district_index.id = "district_index";
    district_index.panel_id = "District Index";

    DockTreeNode road_index;
    road_index.id = "road_index";
    road_index.panel_id = "Road Index";

    DockTreeNode lot_index;
    lot_index.id = "lot_index";
    lot_index.panel_id = "Lot Index";

    DockTreeNode river_index;
    river_index.id = "river_index";
    river_index.panel_id = "River Index";

    DockTreeNode bottom_tabs;
    bottom_tabs.id = "bottom_tabs";
    bottom_tabs.orientation = "horizontal";
    bottom_tabs.children = {log, district_index, road_index, lot_index, river_index};

    DockTreeNode bottom;
    bottom.id = "bottom";
    bottom.orientation = "vertical";
    bottom.children = {tools, bottom_tabs};

    root.children = {top, main_row, bottom};
    return root;
}

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
    ImGuiID dock_left = 0;
    ImGuiID dock_right = 0;
    ImGuiID dock_bottom = 0;
    ImGuiID dock_top = 0;
    ImGuiID dock_tools = 0;
    ImGuiID dock_bottom_tabs = 0;

    dock_top = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Up, 0.10f, nullptr, &dock_main);
    dock_left = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.18f, nullptr, &dock_main);
    dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.27f, nullptr, &dock_main);
    dock_bottom = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.22f, nullptr, &dock_main);
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

    s_dock_nodes.root = dockspace_id;
    s_dock_nodes.top = dock_top;
    s_dock_nodes.left = dock_left;
    s_dock_nodes.right = dock_right;
    s_dock_nodes.center = dock_main;
    s_dock_nodes.bottom = dock_bottom;
    s_dock_nodes.bottom_tabs = dock_bottom_tabs;
}

static ImGuiID NodeForDockArea(const std::string& dock_area) {
    if (dock_area == "Top") {
        return s_dock_nodes.top;
    }
    if (dock_area == "Bottom") {
        return s_dock_nodes.bottom_tabs ? s_dock_nodes.bottom_tabs : s_dock_nodes.bottom;
    }
    if (dock_area == "Left") {
        return s_dock_nodes.left;
    }
    if (dock_area == "Right") {
        return s_dock_nodes.right;
    }
    return s_dock_nodes.center;
}

static void ProcessPendingDockRequests(ImGuiID dockspace_id) {
    if (s_pending_dock_requests.empty()) {
        return;
    }

    bool any_applied = false;
    for (const DockRequest& request : s_pending_dock_requests) {
        ImGuiID target = NodeForDockArea(request.dock_area);
        if (target == 0) {
            continue;
        }

        if (request.own_dock_node) {
            ImGuiID new_target = 0;
            ImGui::DockBuilderSplitNode(target, ImGuiDir_Down, 0.55f, &new_target, &target);
            if (new_target != 0) {
                target = new_target;
            }
        }

        ImGui::DockBuilderDockWindow(request.window_name.c_str(), target);
        any_applied = true;
    }

    s_pending_dock_requests.clear();
    if (any_applied) {
        ImGui::DockBuilderFinish(dockspace_id);
    }
}

void DrawRoot(float dt)
{
    // Initialize minimap on first call
    InitializeMinim();

    // Begin UI introspection for this frame (dev-only tooling).
    auto& introspector = RogueCity::UIInt::UiIntrospector::Instance();
    introspector.BeginFrame(ActiveModeFromHFSM(), RC_UI::Panels::DevShell::IsOpen());
    if (!s_dock_built) {
        introspector.SetDockTree(DefaultDockTree());
    }

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
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoInputs |  // CRITICAL: Pass input through to docked windows!
        ImGuiWindowFlags_NoFocusOnAppearing;  // Don't steal focus

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));  // No padding for dockspace
    ImGui::Begin("RogueDockHost", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("RogueDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    if (!s_dock_built) {
        BuildDockLayout(dockspace_id);
        s_dock_built = true;
    }

    ProcessPendingDockRequests(dockspace_id);

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
    Panels::BuildingIndex::Draw(dt);      // NEW: RC-0.10
    Panels::ZoningControl::Draw(dt);      // NEW: RC-0.10
    
    // AI_INTEGRATION_TAG: V1_PASS1_TASK4_PANEL_WIRING
    // State-reactive control panels (show based on HFSM mode)
    Panels::LotControl::Draw(dt);
    Panels::BuildingControl::Draw(dt);
    Panels::WaterControl::Draw(dt);
    
    Panels::Log::Draw(dt);
    
    // AI panels (use static instances)
    s_ai_console_instance.Render();
    s_ui_agent_instance.Render();    // Phase 2
    s_city_spec_instance.Render();   // Phase 3
    Panels::DevShell::Draw(dt);

    // Minimap is now embedded as overlay in RogueVisualizer (no separate panel)
    // Still update the minimap viewport for shared camera sync
    if (s_minimap) {
        s_minimap->update(dt);
    }
}

RogueCity::App::MinimapViewport* GetMinimapViewport() {
    return s_minimap.get();
}

bool QueueDockWindow(const char* windowName, const char* dockArea, bool ownDockNode) {
    if (!windowName || !dockArea) {
        return false;
    }

    s_pending_dock_requests.push_back(
        DockRequest{
            std::string(windowName),
            std::string(dockArea),
            ownDockNode
        });
    return true;
}

void ResetDockLayout() {
    s_dock_built = false;
    s_pending_dock_requests.clear();
}

} // namespace RC_UI
