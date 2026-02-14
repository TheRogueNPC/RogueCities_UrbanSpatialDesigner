// FILE: rc_ui_root.cpp - Editor window routing with hardened docking

#include "ui/rc_ui_tokens.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_responsive.h"
#include "ui/rc_ui_viewport_config.h"
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

// MASTER PANEL ARCHITECTURE (RC-0.10)
#include "ui/panels/RcMasterPanel.h"
#include "ui/panels/PanelRegistry.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <memory>
#include <string>
#include <array>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <span>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace RC_UI {

// MASTER PANEL SYSTEM (RC-0.10 architecture refactor)
// Replaces individual panel windows with unified container + drawer registry
namespace {
    static std::unique_ptr<RC_UI::Panels::RcMasterPanel> s_master_panel;
    static bool s_registry_initialized = false;
    
    // DEPRECATED: Old AI panel instances (will be removed after full drawer conversion)
    // static RogueCity::UI::AiConsolePanel s_ai_console_instance;
    // static RogueCity::UI::UiAgentPanel s_ui_agent_instance;
    // static RogueCity::UI::CitySpecPanel s_city_spec_instance;

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
    static constexpr size_t kMaxPendingDockRequests = 128u;
    static constexpr const char* kWorkspacePresetFile = "AI/docs/ui/workspace_presets.json";

    struct WorkspacePresetStore {
        std::unordered_map<std::string, std::string> presets;
    };

    [[nodiscard]] static std::filesystem::path WorkspacePresetPath() {
        return std::filesystem::path(kWorkspacePresetFile);
    }

    static void LoadWorkspacePresetStore(WorkspacePresetStore& store) {
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
            for (const auto& item : j["presets"].items()) {
                if (item.value().is_string()) {
                    store.presets[item.key()] = item.value().get<std::string>();
                }
            }
        } catch (...) {
            // Keep editor running even if preset file is malformed.
        }
    }

    static bool SaveWorkspacePresetStore(const WorkspacePresetStore& store, std::string* error) {
        try {
            const std::filesystem::path path = WorkspacePresetPath();
            std::filesystem::create_directories(path.parent_path());

            nlohmann::json j;
            j["schema"] = 1;
            j["presets"] = nlohmann::json::object();
            for (const auto& [name, ini] : store.presets) {
                j["presets"][name] = ini;
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
        } catch (const std::exception& e) {
            if (error != nullptr) {
                *error = e.what();
            }
            return false;
        }
    }
}
// Utility for drawing icons for the tool library entries.
//TODO: Phase 2 - Refactor to use a more flexible icon system, potentially with support for custom icons per tool and theming.
static void DrawToolLibraryIcon(ImDrawList* draw_list, ToolLibrary tool, const ImVec2& center, float size) {
    const ImU32 color = UITokens::TextPrimary;
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

    const bool open = Components::BeginTokenPanel(window_name, UITokens::CyanAccent);
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
        const LayoutMode layout_mode = ResponsiveLayout::GetLayoutMode(library_avail.x);
        if (layout_mode == LayoutMode::Collapsed ||
            library_avail.x < ResponsiveConstants::MIN_PANEL_WIDTH ||
            library_avail.y < ResponsiveConstants::MIN_PANEL_HEIGHT) {
            EndWindowContainer();
            uiint.EndPanel();
            Components::EndTokenPanel();
            return;
        }

        const float base_icon_size = 46.0f;
        const float base_spacing = 12.0f;
        const int total_entries = static_cast<int>(entries.size());
        const auto layout = ResponsiveButtonLayout::Calculate(
            total_entries,
            library_avail.x,
            library_avail.y,
            base_icon_size,
            base_icon_size,
            base_spacing);

        if (layout.visible_count <= 0) {
            EndWindowContainer();
            uiint.EndPanel();
            Components::EndTokenPanel();
            return;
        }

        const float icon_size = layout.button_width;
        const float spacing = layout.spacing;
        const int columns = static_cast<int>(std::max(1.0f,
            std::floor((library_avail.x + spacing) / (icon_size + spacing))));

        const int visible_entries = std::min(total_entries, layout.visible_count);
        for (int i = 0; i < visible_entries; ++i) {
            if (i > 0 && (i % columns) != 0) {
                ImGui::SameLine(0.0f, spacing);
            }
            ImGui::PushID(i);
            ImGui::InvisibleButton("ToolEntry", ImVec2(icon_size, icon_size));
            const ImVec2 bmin = ImGui::GetItemRectMin();
            const ImVec2 bmax = ImGui::GetItemRectMax();
            const ImVec2 center((bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(bmin, bmax, WithAlpha(UITokens::PanelBackground, 220u), 8.0f);
            draw_list->AddRect(bmin, bmax, WithAlpha(UITokens::TextSecondary, 180u), 8.0f, 0, 1.5f);
            DrawToolLibraryIcon(draw_list, tool, center, icon_size * 0.5f);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", entries[static_cast<size_t>(i)]);
            }
            ImGui::PopID();
        }
        uiint.RegisterWidget({"table", window_name, std::string(window_name) + ".tools[]", {"tool", "library"}});

        if (visible_entries < total_entries) {
            ImGui::SeparatorText("More tools available...");
        }

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
    Components::EndTokenPanel();
}

// Static minimap instance (Phase 5: Polish)
//TODO: Phase 5 - Refactor to support multiple viewport types and dynamic viewport management, with the minimap as a special case or plugin.
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

// Utility function to generate a default dock tree layout for new workspaces or when no saved layout is available.
// TODO: Phase 4 - Refactor to support multiple dock tree configurations and user-customizable default layouts, potentially with a visual layout editor.
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
// Initialization of the minimap viewport instance, with default settings.
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
#if defined(IMGUI_HAS_DOCK)
    if (dockspace_id == 0) {
        return;
    }
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport == nullptr || viewport->Size.x <= 0.0f || viewport->Size.y <= 0.0f) {
        return;
    }

    ImGui::DockBuilderRemoveNodeDockedWindows(dockspace_id, true);
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

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
#else
    (void)dockspace_id;
    s_dock_nodes = {};
#endif
}

static bool AreDockNodesHealthy(ImGuiID dockspace_id) {
#if defined(IMGUI_HAS_DOCK)
    if (!IsDockspaceValid(dockspace_id)) {
        return false;
    }

    ImGuiDockNode* center = GetDockNodeSafe(s_dock_nodes.center);
    ImGuiDockNode* tool_deck = GetDockNodeSafe(s_dock_nodes.tool_deck);
    ImGuiDockNode* library = GetDockNodeSafe(s_dock_nodes.library);
    ImGuiDockNode* bottom = GetDockNodeSafe(s_dock_nodes.bottom_tabs ? s_dock_nodes.bottom_tabs : s_dock_nodes.bottom);

    return DockNodeValidator::IsNodeSizeValid(center) &&
        DockNodeValidator::IsNodeSizeValid(tool_deck) &&
        DockNodeValidator::IsNodeSizeValid(library) &&
        DockNodeValidator::IsNodeSizeValid(bottom);
#else
    (void)dockspace_id;
    return true;
#endif
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
    for (auto it = s_pending_dock_requests.rbegin(); it != s_pending_dock_requests.rend(); ++it) {
        const DockRequest& request = *it;
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

        ImGuiDockNode* target_node = GetDockNodeSafe(target);
        if (!DockNodeValidator::IsNodeSizeValid(target_node)) {
            target = s_dock_nodes.center;
            target_node = GetDockNodeSafe(target);
            if (!DockNodeValidator::IsNodeSizeValid(target_node)) {
                continue;
            }
        }

        if (request.own_dock_node) {
            ImGuiID new_target = 0;
            ImGui::DockBuilderSplitNode(target, ImGuiDir_Down, 0.55f, &new_target, &target);
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
#if defined(IMGUI_HAS_DOCK)
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (ctx == nullptr) {
        return;
    }

    for (ImGuiWindow* window : ctx->Windows) {
        if (window == nullptr || window->DockId == 0) {
            continue;
        }

        if (!DockNodeValidator::IsWindowDockValid(window, dockspace_id)) {
            window->DockId = 0;
            window->DockNode = nullptr;
        }
    }
#else
    (void)dockspace_id;
#endif
}

static void UpdateDockLayout(ImGuiID dockspace_id) {
#if defined(IMGUI_HAS_DOCK)
    bool layout_changed = false;
    if (!IsDockspaceValid(dockspace_id)) {
        s_dock_built = false;
        s_dock_layout_dirty = true;
    }

    if (!s_dock_built || s_dock_layout_dirty || !AreDockNodesHealthy(dockspace_id)) {
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
    if (viewport == nullptr) {
        return;
    }

    const ViewportBounds viewport_bounds{viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y};
    if (!viewport_bounds.IsValid()) {
        return;
    }

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
#if defined(IMGUI_HAS_DOCK)
    ImGui::SetNextWindowViewport(viewport->ID);
#endif

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
#if defined(IMGUI_HAS_DOCK)
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
#endif
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
// Tool libraries (docked in library area, show based on toggle state)
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
        "visualizer/src/ui/rc_ui_root.cpp",
        "Library",
        lot_tools,
        std::span<const char* const>{});
    RenderToolLibraryWindow(ToolLibrary::Building,
        "Building Library",
        "visualizer/src/ui/rc_ui_root.cpp",
        "Library",
        building_tools,
        std::span<const char* const>{});

    // MASTER PANEL SYSTEM (RC-0.10)
    // All panels now routed through unified drawer registry
    // Provides: tabs, search overlay (Ctrl+P), popout support, state-reactive visibility
    
    // Initialize registry once
    if (!s_registry_initialized) {
        RC_UI::Panels::InitializePanelRegistry();
        s_master_panel = std::make_unique<RC_UI::Panels::RcMasterPanel>();
        s_registry_initialized = true;
    }
    
    // Single master panel hosts all 19 panel drawers
    if (s_master_panel) {
        s_master_panel->Draw(dt);
    }
    
    // DEPRECATED: Old individual panel calls (replaced by Master Panel)
    // Panels::AxiomBar::Draw(dt);
    // Panels::AxiomEditor::Draw(dt);
    // Panels::Telemetry::Draw(dt);
    // ... (18 more panels)

    // Minimap is now embedded as overlay in RogueVisualizer (no separate panel)
    // TODO investigate potentially decoupleing the minimap from root to allow it to be used as a standalone veiwport in other contexte s (e.g. floating, secondary monitor) in the future. also to ensure that the mimimap redocks gracefully to the main veiwport
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

    for (auto it = s_pending_dock_requests.rbegin(); it != s_pending_dock_requests.rend(); ++it) {
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

bool SaveWorkspacePreset(const char* presetName, std::string* error) {
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
    const char* ini_data = ImGui::SaveIniSettingsToMemory(&ini_size);
    if (ini_data == nullptr || ini_size == 0) {
        if (error != nullptr) {
            *error = "ImGui returned empty layout data.";
        }
        return false;
    }

    WorkspacePresetStore store;
    LoadWorkspacePresetStore(store);
    store.presets[presetName] = std::string(ini_data, ini_size);

    // TODO(ai-layout): expose preset metadata to the UI agent for monitor-aware workspace suggestions.
    return SaveWorkspacePresetStore(store, error);
}

bool LoadWorkspacePreset(const char* presetName, std::string* error) {
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

    ImGui::LoadIniSettingsFromMemory(it->second.c_str(), static_cast<size_t>(it->second.size()));
    s_pending_dock_requests.clear();
    s_dock_layout_dirty = false;
    s_dock_built = true;
    return true;
}

std::vector<std::string> ListWorkspacePresets() {
    WorkspacePresetStore store;
    LoadWorkspacePresetStore(store);
    std::vector<std::string> names;
    names.reserve(store.presets.size());
    for (const auto& [name, _] : store.presets) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
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

[[nodiscard]] static bool IsRectVisibleOnAnyViewport(const ImVec2& pos, const ImVec2& size) {
    if (size.x <= 1.0f || size.y <= 1.0f) {
        return true;
    }

    const ImRect rect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
#if defined(IMGUI_HAS_DOCK)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (ImGuiViewport* viewport : platform_io.Viewports) {
        if (viewport == nullptr) {
            continue;
        }
        const ImRect viewport_rect(
            viewport->Pos,
            ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y));
        if (viewport_rect.Overlaps(rect)) {
            return true;
        }
    }
#endif

    if (ImGuiViewport* main_viewport = ImGui::GetMainViewport(); main_viewport != nullptr) {
        const ImRect viewport_rect(
            main_viewport->Pos,
            ImVec2(main_viewport->Pos.x + main_viewport->Size.x, main_viewport->Pos.y + main_viewport->Size.y));
        return viewport_rect.Overlaps(rect);
    }
    return true;
}

bool BeginDockableWindow(const char* windowName,
                         DockableWindowState& state,
                         const char* fallbackDockArea,
                         ImGuiWindowFlags flags) {
    const bool show_close = !state.was_docked;
    bool* p_open = show_close ? &state.open : nullptr;

    const bool open = ImGui::Begin(windowName, p_open, flags);
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
            if (ImGuiViewport* main_viewport = ImGui::GetMainViewport(); main_viewport != nullptr) {
                const ImVec2 rescue_pos(main_viewport->Pos.x + 48.0f, main_viewport->Pos.y + 48.0f);
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

} // namespace RC_UI
