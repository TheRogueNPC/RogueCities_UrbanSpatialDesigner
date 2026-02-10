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
#include "ui/panels/rc_panel_inspector.h"
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
#include <array>
#include <vector>
#include <unordered_map>
#include <span>
#include <cmath>

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
}

static void DrawToolLibraryIcon(ImDrawList* draw_list, ToolLibrary tool, const ImVec2& center, float size) {
    const ImU32 color = IM_COL32(220, 220, 220, 255);
    const float half = size * 0.5f;
    switch (tool) {
        case ToolLibrary::Water:
            draw_list->AddTriangleFilled(
                ImVec2(center.x, center.y - half),
                ImVec2(center.x - half, center.y + half),
                ImVec2(center.x + half, center.y + half),
                color);
            break;
        case ToolLibrary::Road:
            draw_list->AddLine(
                ImVec2(center.x - half, center.y + half),
                ImVec2(center.x + half, center.y - half),
                color, 2.5f);
            break;
        case ToolLibrary::District:
            draw_list->AddRect(
                ImVec2(center.x - half, center.y - half),
                ImVec2(center.x + half, center.y + half),
                color, 3.0f, 0, 2.0f);
            break;
        case ToolLibrary::Lot:
            draw_list->AddRect(
                ImVec2(center.x - half, center.y - half),
                ImVec2(center.x + half, center.y + half),
                color, 0.0f, 0, 2.0f);
            draw_list->AddLine(
                ImVec2(center.x, center.y - half),
                ImVec2(center.x, center.y + half),
                color, 1.5f);
            draw_list->AddLine(
                ImVec2(center.x - half, center.y),
                ImVec2(center.x + half, center.y),
                color, 1.5f);
            break;
        case ToolLibrary::Building:
            draw_list->AddRectFilled(
                ImVec2(center.x - half, center.y - half),
                ImVec2(center.x + half, center.y + half),
                color, 2.0f);
            draw_list->AddRect(
                ImVec2(center.x - half, center.y - half),
                ImVec2(center.x + half, center.y + half),
                color, 2.0f, 0, 2.0f);
            break;
        case ToolLibrary::Axiom:
            draw_list->AddCircle(center, half, color, 12, 2.0f);
            draw_list->AddCircleFilled(center, half * 0.35f, color, 12);
            break;
    }
}

static void RenderToolLibraryWindow(ToolLibrary tool,
                                    const char* window_name,
                                    const char* owner_module,
                                    const char* dock_area,
                                    std::span<const char* const> entries,
                                    std::span<const char* const> spline_entries) {
    if (!IsToolLibraryOpen(tool)) {
        return;
    }

    const bool open = ImGui::Begin(window_name, nullptr, ImGuiWindowFlags_NoCollapse);
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            window_name,
            window_name,
            "toolbox",
            dock_area,
            owner_module,
            {"tool", "library"}
        },
        open
    );

    if (open) {
        BeginWindowContainer("##tool_library_container");
        const ImVec2 library_avail = ImGui::GetContentRegionAvail();
        if (library_avail.x < 80.0f || library_avail.y < 80.0f) {
            EndWindowContainer();
            uiint.EndPanel();
            ImGui::End();
            return;
        }

        const float icon_size = ImClamp(library_avail.x / 4.0f, 28.0f, 46.0f);
        const int columns = static_cast<int>(std::max(1.0f, std::floor(library_avail.x / (icon_size + 12.0f))));
        for (size_t i = 0; i < entries.size(); ++i) {
            if (i > 0 && (i % columns) != 0) {
                ImGui::SameLine();
            }
            ImGui::PushID(static_cast<int>(i));
            ImGui::InvisibleButton("ToolEntry", ImVec2(icon_size, icon_size));
            const ImVec2 bmin = ImGui::GetItemRectMin();
            const ImVec2 bmax = ImGui::GetItemRectMax();
            const ImVec2 center((bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(bmin, bmax, IM_COL32(20, 20, 20, 180), 8.0f);
            draw_list->AddRect(bmin, bmax, IM_COL32(120, 140, 160, 200), 8.0f, 0, 1.5f);
            DrawToolLibraryIcon(draw_list, tool, center, icon_size * 0.5f);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", entries[i]);
            }
            ImGui::PopID();
        }
        uiint.RegisterWidget({"table", window_name, std::string(window_name) + ".tools[]", {"tool", "library"}});

        if (!spline_entries.empty()) {
            ImGui::SeparatorText("Spline Tools");
            for (size_t i = 0; i < spline_entries.size(); ++i) {
                ImGui::BulletText("%s", spline_entries[i]);
            }
            uiint.RegisterWidget({"table", "Spline Tools", std::string(window_name) + ".splines[]", {"tool", "spline"}});
        }
        EndWindowContainer();
    }

    uiint.EndPanel();
    ImGui::End();
}

// Static minimap instance (Phase 5: Polish)
static std::unique_ptr<RogueCity::App::MinimapViewport> s_minimap;
static bool s_dock_built = false;
static bool s_dock_layout_dirty = true;
static std::array<bool, kToolLibraryOrder.size()> s_tool_library_open = {};

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

    DockTreeNode center;
    center.id = "center";
    center.panel_id = "RogueVisualizer";

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

    root.children = {main_row, bottom};
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
    return IsToolLibraryOpen(ToolLibrary::Axiom);
}

void ToggleAxiomLibrary() {
    ToggleToolLibrary(ToolLibrary::Axiom);
}

static size_t ToolLibraryIndex(ToolLibrary tool) {
    switch (tool) {
        case ToolLibrary::Axiom: return 0;
        case ToolLibrary::Water: return 1;
        case ToolLibrary::Road: return 2;
        case ToolLibrary::District: return 3;
        case ToolLibrary::Lot: return 4;
        case ToolLibrary::Building: return 5;
    }
    return 0;
}

static const char* ToolLibraryWindowName(ToolLibrary tool) {
    switch (tool) {
        case ToolLibrary::Axiom: return "Axiom Library";
        case ToolLibrary::Water: return "Water Library";
        case ToolLibrary::Road: return "Road Library";
        case ToolLibrary::District: return "District Library";
        case ToolLibrary::Lot: return "Lot Library";
        case ToolLibrary::Building: return "Building Library";
    }
    return "Tool Library";
}

bool IsToolLibraryOpen(ToolLibrary tool) {
    return s_tool_library_open[ToolLibraryIndex(tool)];
}

void ToggleToolLibrary(ToolLibrary tool) {
    const size_t index = ToolLibraryIndex(tool);
    s_tool_library_open[index] = !s_tool_library_open[index];
    if (s_tool_library_open[index]) {
        QueueDockWindow(ToolLibraryWindowName(tool), "Library", true);
    }
}

void ApplyUnifiedWindowSchema(const ImVec2& baseSize, float padding) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 viewportSize = viewport ? viewport->Size : ImVec2(1920.0f, 1080.0f);
    const ImVec2 maxSize(
        std::max(320.0f, viewportSize.x - padding * 2.0f),
        std::max(240.0f, viewportSize.y - padding * 2.0f));
    const ImVec2 clampedBase(
        std::min(baseSize.x, maxSize.x),
        std::min(baseSize.y, maxSize.y));

    ImGui::SetNextWindowSize(clampedBase, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 240.0f), maxSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
}

void PopUnifiedWindowSchema() {
    ImGui::PopStyleVar();
}

void BeginUnifiedTextWrap(float padding) {
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x - padding);
}

void EndUnifiedTextWrap() {
    ImGui::PopTextWrapPos();
}

bool BeginWindowContainer(const char* id, ImGuiWindowFlags flags) {
    const ImGuiWindowFlags child_flags = flags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    return ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), true, child_flags);
}

void EndWindowContainer() {
    ImGui::EndChild();
}

static void BuildDockLayout(ImGuiID dockspace_id) {
    ImGui::DockBuilderRemoveNodeDockedWindows(dockspace_id, true);
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    ImGuiID dock_main = dockspace_id;
    ImGuiID dock_left = 0;
    ImGuiID dock_right = 0;
    ImGuiID dock_bottom = 0;
    ImGuiID dock_tools = 0;
    ImGuiID dock_bottom_tabs = 0;
    ImGuiID dock_tool_deck = 0;
    ImGuiID dock_library = 0;

    dock_left = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.18f, nullptr, &dock_main);
    dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.27f, nullptr, &dock_main);
    dock_bottom = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.22f, nullptr, &dock_main);
    dock_tools = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Down, 0.32f, nullptr, &dock_bottom_tabs);

    ImGui::DockBuilderDockWindow("RogueVisualizer", dock_main);
    // Minimap is now embedded as overlay in RogueVisualizer (removed from docking)
    {
        ImGuiID dock_right_down = 0;
        ImGuiID dock_right_up = 0;
        ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.55f, &dock_right_down, &dock_right_up);
        dock_library = dock_right_down;
        dock_tool_deck = dock_right_up;
    }

    ImGui::DockBuilderDockWindow("Tool Deck", dock_tool_deck);
    ImGui::DockBuilderDockWindow("Inspector", dock_tool_deck);
    ImGui::DockBuilderDockWindow("Analytics", dock_tool_deck);

    ImGui::DockBuilderDockWindow("Tools", dock_tools);
    ImGui::DockBuilderDockWindow("Log", dock_bottom_tabs);
    ImGui::DockBuilderDockWindow("District Index", dock_bottom_tabs);
    ImGui::DockBuilderDockWindow("Road Index", dock_bottom_tabs);
    ImGui::DockBuilderDockWindow("Lot Index", dock_bottom_tabs);
    ImGui::DockBuilderDockWindow("River Index", dock_bottom_tabs);

    ImGui::DockBuilderDockWindow("Axiom Library", dock_library);
    ImGui::DockBuilderDockWindow("Water Library", dock_library);
    ImGui::DockBuilderDockWindow("Road Library", dock_library);
    ImGui::DockBuilderDockWindow("District Library", dock_library);
    ImGui::DockBuilderDockWindow("Lot Library", dock_library);
    ImGui::DockBuilderDockWindow("Building Library", dock_library);

    s_dock_nodes.root = dockspace_id;
    s_dock_nodes.left = dock_left;
    s_dock_nodes.right = dock_right;
    s_dock_nodes.tool_deck = dock_tool_deck;
    s_dock_nodes.library = dock_library;
    s_dock_nodes.center = dock_main;
    s_dock_nodes.bottom = dock_bottom;
    s_dock_nodes.bottom_tabs = dock_bottom_tabs;
}

static ImGuiID NodeForDockArea(const std::string& dock_area) {
    if (dock_area == "Top") {
        return s_dock_nodes.center;
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
    if (dock_area == "ToolDeck") {
        return s_dock_nodes.tool_deck;
    }
    if (dock_area == "Library") {
        return s_dock_nodes.library;
    }
    return s_dock_nodes.center;
}

static bool ProcessPendingDockRequests(ImGuiID dockspace_id) {
    if (s_pending_dock_requests.empty()) {
        return false;
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
    return any_applied;
}

static void UpdateDockLayout(ImGuiID dockspace_id) {
    bool layout_changed = false;
    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        s_dock_built = false;
        s_dock_layout_dirty = true;
    }

    if (!s_dock_built || s_dock_layout_dirty) {
        BuildDockLayout(dockspace_id);
        s_dock_built = true;
        s_dock_layout_dirty = false;
        layout_changed = true;
    }

    if (ProcessPendingDockRequests(dockspace_id)) {
        layout_changed = true;
    }

    if (layout_changed && ImGui::DockBuilderGetNode(dockspace_id) != nullptr) {
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
        // ImGuiWindowFlags_NoInputs removed - it blocks input instead of passing through!
        ImGuiWindowFlags_NoFocusOnAppearing;  // Don't steal focus

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));  // No padding for dockspace
    ImGui::Begin("RogueDockHost", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("RogueDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    UpdateDockLayout(dockspace_id);

    ImGui::End();

    static const std::array<const char*, 6> water_tools = {
        "Flow", "Contour", "Erode", "Select", "Mask", "Inspect"
    };
    static const std::array<const char*, 9> road_tools = {
        "Spline", "Grid", "Bridge", "Select", "Disconnect", "Stub", "Curve", "Strengthen", "Inspect"
    };
    static const std::array<const char*, 6> district_tools = {
        "Zone", "Paint", "Split", "Select", "Merge", "Inspect"
    };
    static const std::array<const char*, 6> lot_tools = {
        "Plot", "Slice", "Align", "Select", "Merge", "Inspect"
    };
    static const std::array<const char*, 6> building_tools = {
        "Place", "Scale", "Rotate", "Select", "Assign", "Inspect"
    };
    static const std::array<const char*, 9> spline_tools = {
        "Selection", "Direct Select", "Pen", "Convert Anchor", "Add/Remove Anchor",
        "Handle Tangents", "Snap/Align", "Join/Split", "Simplify"
    };

    RenderToolLibraryWindow(ToolLibrary::Water,
        "Water Library",
        "visualizer/src/ui/rc_ui_root.cpp",
        "Library",
        water_tools,
        spline_tools);
    RenderToolLibraryWindow(ToolLibrary::Road,
        "Road Library",
        "visualizer/src/ui/rc_ui_root.cpp",
        "Library",
        road_tools,
        spline_tools);
    RenderToolLibraryWindow(ToolLibrary::District,
        "District Library",
        "visualizer/src/ui/rc_ui_root.cpp",
        "Library",
        district_tools,
        std::span<const char* const>{});
    RenderToolLibraryWindow(ToolLibrary::Lot,
        "Lot Library",
        "visualizer/src/ui_root.cpp",
        "Library",
        lot_tools,
        std::span<const char* const>{});
    RenderToolLibraryWindow(ToolLibrary::Building,
        "Building Library",
        "visualizer/src/ui/rc_ui_root.cpp",
        "Library",
        building_tools,
        std::span<const char* const>{});

    // Panels/windows (docked by name)
    Panels::AxiomBar::Draw(dt);
    Panels::AxiomEditor::Draw(dt);
    Panels::Telemetry::Draw(dt);
    Panels::Inspector::Draw(dt);
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
    s_dock_layout_dirty = true;
    s_pending_dock_requests.clear();
}

void NotifyDockedWindow(const char* windowName, const char* dockArea) {
    if (!windowName || !dockArea || dockArea[0] == '\0') {
        return;
    }

    s_last_dock_area[windowName] = dockArea;
}

void ReturnWindowToLastDock(const char* windowName, const char* fallbackArea) {
    if (!windowName) {
        return;
    }

    const auto it = s_last_dock_area.find(windowName);
    const char* target = fallbackArea;
    if (it != s_last_dock_area.end()) {
        target = it->second.c_str();
    }

    QueueDockWindow(windowName, target);
}

bool BeginDockableWindow(const char* windowName,
                         DockableWindowState& state,
                         const char* fallbackDockArea,
                         ImGuiWindowFlags flags) {
    const bool show_close = !state.was_docked;
    bool* p_open = show_close ? &state.open : nullptr;

    const bool open = ImGui::Begin(windowName, p_open, flags);
    const bool is_docked = ImGui::IsWindowDocked();
    state.was_docked = is_docked;

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

} // namespace RC_UI
