#pragma once

#include "ui/commands/rc_command_palette.h"
#include "ui/commands/rc_context_menu_pie.h"
#include "ui/commands/rc_context_menu_smart.h"
#include "ui/rc_ui_input_gate.h"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Math/Vec2.hpp"

#include <imgui.h>
#include <vector>

namespace RogueCity::App {
class AxiomPlacementTool;
class PrimaryViewport;
}

namespace RC_UI::Viewport {

struct CommandMenuStateBundle {
    RC_UI::Commands::SmartMenuState* smart_menu{ nullptr };
    RC_UI::Commands::PieMenuState* pie_menu{ nullptr };
    RC_UI::Commands::CommandPaletteState* command_palette{ nullptr };
};

struct CommandInteractionParams {
    UiInputGateState input_gate{};
    bool in_viewport{ false };
    bool minimap_hovered{ false };
    ImVec2 mouse_pos{ 0.0f, 0.0f };
    const RogueCity::Core::Editor::EditorConfig* editor_config{ nullptr };
};

void ProcessViewportCommandTriggers(
    const CommandInteractionParams& params,
    const CommandMenuStateBundle& state_bundle);

struct AxiomInteractionParams {
    bool axiom_mode{ false };
    bool allow_viewport_mouse_actions{ false };
    bool allow_viewport_key_actions{ false };
    ImVec2 mouse_pos{ 0.0f, 0.0f };
    RogueCity::App::PrimaryViewport* primary_viewport{ nullptr };
    RogueCity::App::AxiomPlacementTool* axiom_tool{ nullptr };
    RogueCity::Core::Editor::GlobalState* global_state{ nullptr };
};

struct AxiomInteractionResult {
    bool active{ false };
    bool nav_active{ false };
    bool has_world_pos{ false };
    RogueCity::Core::Vec2 world_pos{};
};

[[nodiscard]] AxiomInteractionResult ProcessAxiomViewportInteraction(const AxiomInteractionParams& params);

struct SelectionDragState {
    bool lasso_active{ false };
    bool box_active{ false };
    RogueCity::Core::Vec2 box_start{};
    RogueCity::Core::Vec2 box_end{};
    std::vector<RogueCity::Core::Vec2> lasso_points{};
};

struct GizmoDragState {
    bool active{ false };
    RogueCity::Core::Vec2 pivot{};
    RogueCity::Core::Vec2 previous_world{};
};

struct RoadVertexDragState {
    bool active{ false };
    uint32_t road_id{ 0 };
    size_t vertex_index{ 0 };
};

struct DistrictBoundaryDragState {
    bool active{ false };
    uint32_t district_id{ 0 };
    size_t vertex_index{ 0 };
};

struct WaterVertexDragState {
    bool active{ false };
    uint32_t water_id{ 0 };
    size_t vertex_index{ 0 };
};

struct NonAxiomInteractionState {
    SelectionDragState selection_drag{};
    GizmoDragState gizmo_drag{};
    RoadVertexDragState road_vertex_drag{};
    DistrictBoundaryDragState district_boundary_drag{};
    WaterVertexDragState water_vertex_drag{};
};

struct NonAxiomInteractionParams {
    bool axiom_mode{ false };
    bool in_viewport{ false };
    bool minimap_hovered{ false };
    bool allow_viewport_mouse_actions{ false };
    bool allow_viewport_key_actions{ false };
    ImVec2 mouse_pos{ 0.0f, 0.0f };
    RogueCity::App::PrimaryViewport* primary_viewport{ nullptr };
    RogueCity::Core::Editor::EditorState editor_state{ RogueCity::Core::Editor::EditorState::Idle };
    RogueCity::Core::Editor::GlobalState* global_state{ nullptr };
};

struct NonAxiomInteractionResult {
    bool active{ false };
    bool has_world_pos{ false };
    RogueCity::Core::Vec2 world_pos{};
};

[[nodiscard]] NonAxiomInteractionResult ProcessNonAxiomViewportInteraction(
    const NonAxiomInteractionParams& params,
    NonAxiomInteractionState* interaction_state);

} // namespace RC_UI::Viewport
