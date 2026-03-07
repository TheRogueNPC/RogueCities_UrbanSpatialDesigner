// FILE: rc_property_editor.cpp
// PURPOSE: Context-sensitive property editor for selected entities.

#include "ui/panels/rc_property_editor.h"
#include "ui/api/rc_imgui_api.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/tools/rc_tool_contract.h"

#include "RogueCity/App/Editor/CommandHistory.hpp"
#include "RogueCity/App/Editor/EditorManipulation.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Editor/SelectionSync.hpp"

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace RC_UI::Panels {

namespace {

using RogueCity::App::CommandHistory;
using RogueCity::App::ICommand;
using RogueCity::Core::Editor::DirtyLayer;
using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Editor::SelectionItem;
using RogueCity::Core::Editor::VpEntityKind;

template <typename EnumType>
bool DrawEnumCombo(const char* label, EnumType& value) {
    const auto names = magic_enum::enum_names<EnumType>();
    const auto values = magic_enum::enum_values<EnumType>();
    const auto current_name = std::string(magic_enum::enum_name(value));
    bool changed = false;

    if (API::BeginCombo(label, current_name.c_str())) {
        for (size_t i = 0; i < values.size(); ++i) {
            const bool selected = (values[i] == value);
            const std::string name = std::string(names[i]);
            if (API::Selectable(name.c_str(), selected)) {
                value = values[i];
                changed = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        API::EndCombo();
    }
    return changed;
}

struct LambdaCommand final : ICommand {
    std::function<void()> apply{};
    std::function<void()> undo{};
    std::string description{};

    void Execute() override {
        if (apply) {
            apply();
        }
    }

    void Undo() override {
        if (undo) {
            undo();
        }
    }

    const char* GetDescription() const override {
        return description.c_str();
    }
};

CommandHistory& PropertyHistory() {
    return RogueCity::App::GetEditorCommandHistory();
}

void CommitLambda(
    CommandHistory& history,
    const char* description,
    std::function<void()> undo_fn,
    std::function<void()> redo_fn) {
    auto cmd = std::make_unique<LambdaCommand>();
    cmd->description = description ? description : "Property Edit";
    cmd->undo = std::move(undo_fn);
    cmd->apply = std::move(redo_fn);
    history.Commit(std::move(cmd));
}

template <typename TValue, typename TSetter>
void CommitValueChange(
    CommandHistory& history,
    const char* description,
    const TValue& before,
    const TValue& after,
    TSetter setter) {
    if (before == after) {
        return;
    }

    CommitLambda(
        history,
        description,
        [setter, before]() mutable { setter(before); },
        [setter, after]() mutable { setter(after); });
}

template <typename TValue, typename TSetter>
bool CommitValueOnDeactivate(
    CommandHistory& history,
    const char* description,
    const TValue& current_value,
    TSetter setter) {
    static std::unordered_map<ImGuiID, TValue> edit_start_values{};

    const ImGuiID item_id = ImGui::GetItemID();
    if (item_id == 0u) {
        return false;
    }
    if (ImGui::IsItemActivated()) {
        edit_start_values[item_id] = current_value;
    }
    if (!ImGui::IsItemDeactivatedAfterEdit()) {
        return false;
    }

    TValue before_value = current_value;
    if (const auto it = edit_start_values.find(item_id); it != edit_start_values.end()) {
        before_value = it->second;
        edit_start_values.erase(it);
    }

    if (before_value == current_value) {
        return false;
    }

    CommitValueChange(history, description, before_value, current_value, setter);
    return true;
}

void MarkDirtyForKind(GlobalState& gs, VpEntityKind kind) {
    switch (kind) {
    case VpEntityKind::Road:
        gs.dirty_layers.MarkDirty(DirtyLayer::Roads);
        gs.dirty_layers.MarkDirty(DirtyLayer::Districts);
        gs.dirty_layers.MarkDirty(DirtyLayer::Lots);
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
        break;
    case VpEntityKind::District:
        gs.dirty_layers.MarkDirty(DirtyLayer::Districts);
        gs.dirty_layers.MarkDirty(DirtyLayer::Lots);
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
        break;
    case VpEntityKind::Lot:
        gs.dirty_layers.MarkDirty(DirtyLayer::Lots);
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
        break;
    case VpEntityKind::Building:
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
        break;
    default:
        break;
    }
}

RogueCity::Core::Road* FindRoad(GlobalState& gs, uint32_t id) {
    for (auto& road : gs.roads) {
        if (road.id == id) {
            return &road;
        }
    }
    return nullptr;
}

RogueCity::Core::District* FindDistrict(GlobalState& gs, uint32_t id) {
    for (auto& district : gs.districts) {
        if (district.id == id) {
            return &district;
        }
    }
    return nullptr;
}

RogueCity::Core::LotToken* FindLot(GlobalState& gs, uint32_t id) {
    for (auto& lot : gs.lots) {
        if (lot.id == id) {
            return &lot;
        }
    }
    return nullptr;
}

RogueCity::Core::BuildingSite* FindBuilding(GlobalState& gs, uint32_t id) {
    for (auto& building : gs.buildings) {
        if (building.id == id) {
            return &building;
        }
    }
    return nullptr;
}

const char* WaterSubtoolName(RogueCity::Core::Editor::WaterSubtool value) {
    switch (value) {
    case RogueCity::Core::Editor::WaterSubtool::Flow: return "Flow";
    case RogueCity::Core::Editor::WaterSubtool::Contour: return "Contour";
    case RogueCity::Core::Editor::WaterSubtool::Erode: return "Erode";
    case RogueCity::Core::Editor::WaterSubtool::Select: return "Select";
    case RogueCity::Core::Editor::WaterSubtool::Mask: return "Mask";
    case RogueCity::Core::Editor::WaterSubtool::Inspect: return "Inspect";
    }
    return "Unknown";
}

const char* WaterSplineSubtoolName(RogueCity::Core::Editor::WaterSplineSubtool value) {
    switch (value) {
    case RogueCity::Core::Editor::WaterSplineSubtool::Selection: return "Selection";
    case RogueCity::Core::Editor::WaterSplineSubtool::DirectSelect: return "Direct Select";
    case RogueCity::Core::Editor::WaterSplineSubtool::Pen: return "Pen";
    case RogueCity::Core::Editor::WaterSplineSubtool::ConvertAnchor: return "Convert Anchor";
    case RogueCity::Core::Editor::WaterSplineSubtool::AddRemoveAnchor: return "Add/Remove Anchor";
    case RogueCity::Core::Editor::WaterSplineSubtool::HandleTangents: return "Handle Tangents";
    case RogueCity::Core::Editor::WaterSplineSubtool::SnapAlign: return "Snap/Align";
    case RogueCity::Core::Editor::WaterSplineSubtool::JoinSplit: return "Join/Split";
    case RogueCity::Core::Editor::WaterSplineSubtool::Simplify: return "Simplify";
    }
    return "Unknown";
}

const char* RoadSubtoolName(RogueCity::Core::Editor::RoadSubtool value) {
    switch (value) {
    case RogueCity::Core::Editor::RoadSubtool::Spline: return "Spline";
    case RogueCity::Core::Editor::RoadSubtool::Grid: return "Grid";
    case RogueCity::Core::Editor::RoadSubtool::Bridge: return "Bridge";
    case RogueCity::Core::Editor::RoadSubtool::Select: return "Select";
    case RogueCity::Core::Editor::RoadSubtool::Disconnect: return "Disconnect";
    case RogueCity::Core::Editor::RoadSubtool::Stub: return "Stub";
    case RogueCity::Core::Editor::RoadSubtool::Curve: return "Curve";
    case RogueCity::Core::Editor::RoadSubtool::Strengthen: return "Strengthen";
    case RogueCity::Core::Editor::RoadSubtool::Inspect: return "Inspect";
    }
    return "Unknown";
}

const char* RoadSplineSubtoolName(RogueCity::Core::Editor::RoadSplineSubtool value) {
    switch (value) {
    case RogueCity::Core::Editor::RoadSplineSubtool::Selection: return "Selection";
    case RogueCity::Core::Editor::RoadSplineSubtool::DirectSelect: return "Direct Select";
    case RogueCity::Core::Editor::RoadSplineSubtool::Pen: return "Pen";
    case RogueCity::Core::Editor::RoadSplineSubtool::ConvertAnchor: return "Convert Anchor";
    case RogueCity::Core::Editor::RoadSplineSubtool::AddRemoveAnchor: return "Add/Remove Anchor";
    case RogueCity::Core::Editor::RoadSplineSubtool::HandleTangents: return "Handle Tangents";
    case RogueCity::Core::Editor::RoadSplineSubtool::SnapAlign: return "Snap/Align";
    case RogueCity::Core::Editor::RoadSplineSubtool::JoinSplit: return "Join/Split";
    case RogueCity::Core::Editor::RoadSplineSubtool::Simplify: return "Simplify";
    }
    return "Unknown";
}

const char* DistrictSubtoolName(RogueCity::Core::Editor::DistrictSubtool value) {
    switch (value) {
    case RogueCity::Core::Editor::DistrictSubtool::Zone: return "Zone";
    case RogueCity::Core::Editor::DistrictSubtool::Paint: return "Paint";
    case RogueCity::Core::Editor::DistrictSubtool::Split: return "Split";
    case RogueCity::Core::Editor::DistrictSubtool::Select: return "Select";
    case RogueCity::Core::Editor::DistrictSubtool::Merge: return "Merge";
    case RogueCity::Core::Editor::DistrictSubtool::Inspect: return "Inspect";
    }
    return "Unknown";
}

const char* LotSubtoolName(RogueCity::Core::Editor::LotSubtool value) {
    switch (value) {
    case RogueCity::Core::Editor::LotSubtool::Plot: return "Plot";
    case RogueCity::Core::Editor::LotSubtool::Slice: return "Slice";
    case RogueCity::Core::Editor::LotSubtool::Align: return "Align";
    case RogueCity::Core::Editor::LotSubtool::Select: return "Select";
    case RogueCity::Core::Editor::LotSubtool::Merge: return "Merge";
    case RogueCity::Core::Editor::LotSubtool::Inspect: return "Inspect";
    }
    return "Unknown";
}

const char* BuildingSubtoolName(RogueCity::Core::Editor::BuildingSubtool value) {
    switch (value) {
    case RogueCity::Core::Editor::BuildingSubtool::Place: return "Place";
    case RogueCity::Core::Editor::BuildingSubtool::Scale: return "Scale";
    case RogueCity::Core::Editor::BuildingSubtool::Rotate: return "Rotate";
    case RogueCity::Core::Editor::BuildingSubtool::Select: return "Select";
    case RogueCity::Core::Editor::BuildingSubtool::Assign: return "Assign";
    case RogueCity::Core::Editor::BuildingSubtool::Inspect: return "Inspect";
    }
    return "Unknown";
}

const char* GenerationTagName(RogueCity::Core::GenerationTag value) {
    switch (value) {
    case RogueCity::Core::GenerationTag::Generated: return "Generated";
    case RogueCity::Core::GenerationTag::M_user: return "M_user";
    }
    return "Unknown";
}

void DrawToolRuntime(GlobalState& gs) {
    ImGui::SeparatorText("Active Tool Runtime");
    ImGui::Text("Domain: %s", RC_UI::Tools::ToolDomainName(gs.tool_runtime.active_domain));
    ImGui::Text("Last Action: %s (%s)",
        gs.tool_runtime.last_action_label.empty() ? "none" : gs.tool_runtime.last_action_label.c_str(),
        gs.tool_runtime.last_action_status.empty() ? "idle" : gs.tool_runtime.last_action_status.c_str());
    ImGui::Text("Viewport Status: %s",
        gs.tool_runtime.last_viewport_status.empty() ? "idle" : gs.tool_runtime.last_viewport_status.c_str());
    const auto policy_for_active_domain = gs.generation_policy.ForDomain(gs.tool_runtime.active_domain);
    ImGui::Text("Generation Policy: %s",
        policy_for_active_domain == RogueCity::Core::Editor::GenerationMutationPolicy::LiveDebounced
            ? "LiveDebounced"
            : "ExplicitOnly");
    ImGui::Text("Explicit Generate Pending: %s", gs.tool_runtime.explicit_generation_pending ? "yes" : "no");
    ImGui::Text("Dispatch Serial: %llu", static_cast<unsigned long long>(gs.tool_runtime.action_serial));

    switch (gs.tool_runtime.active_domain) {
    case RogueCity::Core::Editor::ToolDomain::Water:
    case RogueCity::Core::Editor::ToolDomain::Flow:
        ImGui::BulletText("Water Tool: %s", WaterSubtoolName(gs.tool_runtime.water_subtool));
        ImGui::BulletText("Water Spline: %s", WaterSplineSubtoolName(gs.tool_runtime.water_spline_subtool));
        API::Checkbox("Spline Editing Enabled", &gs.spline_editor.enabled);
        API::SetNextItemWidth(140.0f);
        API::SliderInt("Spline Samples", &gs.spline_editor.samples_per_segment, 2, 24);
        break;
    case RogueCity::Core::Editor::ToolDomain::Road:
    case RogueCity::Core::Editor::ToolDomain::Paths:
        ImGui::BulletText("Road Tool: %s", RoadSubtoolName(gs.tool_runtime.road_subtool));
        ImGui::BulletText("Road Spline: %s", RoadSplineSubtoolName(gs.tool_runtime.road_spline_subtool));
        API::Checkbox("Spline Editing Enabled", &gs.spline_editor.enabled);
        API::SetNextItemWidth(140.0f);
        API::SliderInt("Spline Samples", &gs.spline_editor.samples_per_segment, 2, 24);
        break;
    case RogueCity::Core::Editor::ToolDomain::District:
    case RogueCity::Core::Editor::ToolDomain::Zone:
        ImGui::BulletText("District Tool: %s", DistrictSubtoolName(gs.tool_runtime.district_subtool));
        API::Checkbox("Boundary Editor", &gs.district_boundary_editor.enabled);
        API::Checkbox("Boundary Snap", &gs.district_boundary_editor.snap_to_grid);
        API::SetNextItemWidth(140.0f);
        API::DragFloat("Boundary Snap Size", &gs.district_boundary_editor.snap_size, 0.5f, 0.5f, 200.0f, "%.1f");
        break;
    case RogueCity::Core::Editor::ToolDomain::Lot:
        ImGui::BulletText("Lot Tool: %s", LotSubtoolName(gs.tool_runtime.lot_subtool));
        break;
    case RogueCity::Core::Editor::ToolDomain::Building:
    case RogueCity::Core::Editor::ToolDomain::FloorPlan:
    case RogueCity::Core::Editor::ToolDomain::Furnature:
        ImGui::BulletText("Building Tool: %s", BuildingSubtoolName(gs.tool_runtime.building_subtool));
        API::TextDisabled("FloorPlan/Furnature are feature stubs until backend modules are wired.");
        break;
    case RogueCity::Core::Editor::ToolDomain::Axiom:
        API::TextDisabled("Axiom subtype is driven by the Axiom Library icons.");
        break;
    }
}

void DrawLayerManager(GlobalState& gs) {
    ImGui::SeparatorText("Layer Manager");

    auto& layer_state = gs.layer_manager;
    for (auto& layer : layer_state.layers) {
        ImGui::PushID(static_cast<int>(layer.id));

        bool visible = layer.visible;
        if (API::Checkbox("##visible", &visible)) {
            layer.visible = visible;
            gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
        }
        API::SameLine();

        const bool active = (layer_state.active_layer == layer.id);
        if (API::Selectable(layer.name.c_str(), active, ImGuiSelectableFlags_None, ImVec2(140.0f, 0.0f))) {
            layer_state.active_layer = layer.id;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Layer %u | Opacity %.2f", static_cast<unsigned>(layer.id), layer.opacity);
        }
        API::SameLine();
        API::SetNextItemWidth(100.0f);
        if (API::SliderFloat("Opacity", &layer.opacity, 0.15f, 1.0f, "%.2f")) {
            layer.opacity = std::clamp(layer.opacity, 0.15f, 1.0f);
            gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
        }
        ImGui::PopID();
    }

    API::Checkbox("Dim Inactive Layers", &layer_state.dim_inactive);
    API::Checkbox("Box Through Hidden Layers", &layer_state.allow_through_hidden);

    if (API::Button("Assign Selection -> Active Layer")) {
        const uint8_t layer = layer_state.active_layer;
        for (const auto& item : gs.selection_manager.Items()) {
            gs.SetEntityLayer(item.kind, item.id, layer);
        }
        gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
    }
}

void DrawValidationOverlayControls(GlobalState& gs) {
    ImGui::SeparatorText("Validation Overlay");
    API::Checkbox("Show Validation Overlay", &gs.validation_overlay.enabled);
    API::SameLine();
    API::Checkbox("Show Warnings", &gs.validation_overlay.show_warnings);
    API::Checkbox("Show Labels", &gs.validation_overlay.show_labels);
    ImGui::Text("Errors: %zu", gs.validation_overlay.errors.size());
}

void DrawGizmoControls(GlobalState& gs) {
    ImGui::SeparatorText("Gizmo");
    API::Checkbox("Enabled", &gs.gizmo.enabled);
    API::SameLine();
    API::Checkbox("Visible", &gs.gizmo.visible);
    API::SameLine();
    API::Checkbox("Snapping", &gs.gizmo.snapping);

    const char* ops[] = {"Translate (G)", "Rotate (R)", "Scale (S)"};
    int op = static_cast<int>(gs.gizmo.operation);
    if (API::Combo("Operation", &op, ops, IM_ARRAYSIZE(ops))) {
        op = std::clamp(op, 0, 2);
        gs.gizmo.operation = static_cast<RogueCity::Core::Editor::GizmoOperation>(op);
    }

    API::SetNextItemWidth(140.0f);
    API::SliderFloat("Translate Snap", &gs.gizmo.translate_snap, 0.5f, 100.0f, "%.1f");
    API::SetNextItemWidth(140.0f);
    API::SliderFloat("Rotate Snap (deg)", &gs.gizmo.rotate_snap_degrees, 1.0f, 90.0f, "%.1f");
    API::SetNextItemWidth(140.0f);
    API::SliderFloat("Scale Snap", &gs.gizmo.scale_snap, 0.01f, 0.5f, "%.2f");
}

// -------------------------------------------------------------------------
// Shared entity flag helpers — extracted from 4 DrawSingle* functions.
// Each replaced ~17+10+5 = 32 lines of copy-paste per entity type.
// -------------------------------------------------------------------------

// User Placed/Created flag + GenerationTag sync, called before GenLocked.
template<typename T>
static void DrawUserFlagBlock(
    CommandHistory& history, GlobalState& gs, T& e,
    VpEntityKind kind, bool T::*flag_field,
    const char* label, const char* commit_desc) {
    const bool before = e.*flag_field;
    if (API::Checkbox(label, &(e.*flag_field))) {
        const bool after = e.*flag_field;
        if (after) {
            e.generation_tag = RogueCity::Core::GenerationTag::M_user;
            e.generation_locked = true;
        } else if (e.generation_tag == RogueCity::Core::GenerationTag::M_user) {
            e.generation_tag = RogueCity::Core::GenerationTag::Generated;
            e.generation_locked = false;
        }
        CommitValueChange(history, commit_desc, before, after,
            [&e, flag_field](bool v) { e.*flag_field = v; });
        MarkDirtyForKind(gs, kind);
    }
    ImGui::Text("Generation Tag: %s", GenerationTagName(e.generation_tag));
}

// Generation Locked checkbox.
template<typename T>
static void DrawGenerationLocked(
    CommandHistory& history, GlobalState& gs, T& e,
    VpEntityKind kind, const char* commit_desc) {
    const bool before = e.generation_locked;
    if (API::Checkbox("Generation Locked", &e.generation_locked)) {
        const bool after = e.generation_locked;
        CommitValueChange(history, commit_desc, before, after,
            [&e](bool v) { e.generation_locked = v; });
        MarkDirtyForKind(gs, kind);
    }
}

// Layer assignment input widget.
static void DrawLayerAssignment(GlobalState& gs, VpEntityKind kind, uint32_t id) {
    const uint8_t current_layer = gs.GetEntityLayer(kind, id);
    int layer_idx = static_cast<int>(current_layer);
    if (API::InputInt("Layer", &layer_idx)) {
        layer_idx = std::clamp(layer_idx, 0, 255);
        gs.SetEntityLayer(kind, id, static_cast<uint8_t>(layer_idx));
        gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
    }
}

void DrawSingleRoad(GlobalState& gs, RogueCity::Core::Road& road) {
    auto& history = PropertyHistory();
    ImGui::Text("Selection: Road");
    API::Separator();
    API::InputScalar("Road ID", ImGuiDataType_U32, &road.id);

    {
        const auto before = road.type;
        if (DrawEnumCombo("Type", road.type)) {
            const auto after = road.type;
            CommitValueChange(
                history,
                "Road Type",
                before,
                after,
                [&road](RogueCity::Core::RoadType v) { road.type = v; });
            MarkDirtyForKind(gs, VpEntityKind::Road);
        }
    }

    DrawUserFlagBlock(history, gs, road, VpEntityKind::Road,
        &RogueCity::Core::Road::is_user_created, "User Created", "Road User Flag");
    DrawGenerationLocked(history, gs, road, VpEntityKind::Road, "Road Generation Lock");

    ImGui::Text("Point Count: %zu", road.points.size());

    DrawLayerAssignment(gs, VpEntityKind::Road, road.id);

    ImGui::SeparatorText("Curve / Spline");
    API::Checkbox("Enable Spline Tool", &gs.spline_editor.enabled);
    API::SameLine();
    API::Checkbox("Closed", &gs.spline_editor.closed);
    API::SetNextItemWidth(120.0f);
    API::SliderInt("Samples", &gs.spline_editor.samples_per_segment, 2, 24);
    API::SetNextItemWidth(120.0f);
    API::SliderFloat("Tension", &gs.spline_editor.tension, 0.0f, 1.0f, "%.2f");

    if (API::Button("Apply Catmull-Rom")) {
        RogueCity::App::EditorManipulation::SplineOptions options{};
        options.closed = gs.spline_editor.closed;
        options.samples_per_segment = gs.spline_editor.samples_per_segment;
        options.tension = gs.spline_editor.tension;

        const auto before = road.points;
        auto after = RogueCity::App::EditorManipulation::BuildCatmullRomSpline(before, options);
        if (after.size() >= 2 && before != after) {
            CommitLambda(
                history,
                "Road Spline Smooth",
                [&road, before]() { road.points = before; },
                [&road, after]() mutable { road.points = std::move(after); });
            MarkDirtyForKind(gs, VpEntityKind::Road);
        }
    }
}

void DrawSingleDistrict(GlobalState& gs, RogueCity::Core::District& district) {
    auto& history = PropertyHistory();
    ImGui::Text("Selection: District");
    API::Separator();
    API::InputScalar("District ID", ImGuiDataType_U32, &district.id);
    API::InputInt("Primary Axiom", &district.primary_axiom_id);
    API::InputInt("Secondary Axiom", &district.secondary_axiom_id);

    {
        const auto before = district.type;
        if (DrawEnumCombo("Type", district.type)) {
            const auto after = district.type;
            CommitValueChange(
                history,
                "District Type",
                before,
                after,
                [&district](RogueCity::Core::DistrictType v) { district.type = v; });
            MarkDirtyForKind(gs, VpEntityKind::District);
        }
    }
    DrawUserFlagBlock(history, gs, district, VpEntityKind::District,
        &RogueCity::Core::District::is_user_placed, "User Placed", "District User Flag");
    DrawGenerationLocked(history, gs, district, VpEntityKind::District, "District Generation Lock");

    {
        API::InputFloat("Budget Allocated", &district.budget_allocated);
        if (CommitValueOnDeactivate(
                history,
                "District Budget",
                district.budget_allocated,
                [&district](float v) { district.budget_allocated = v; })) {
            MarkDirtyForKind(gs, VpEntityKind::District);
        }
    }

    {
        API::InputScalar("Population", ImGuiDataType_U32, &district.projected_population);
        if (CommitValueOnDeactivate(
                history,
                "District Population",
                district.projected_population,
                [&district](uint32_t v) { district.projected_population = v; })) {
            MarkDirtyForKind(gs, VpEntityKind::District);
        }
    }

    {
        API::InputFloat("Desirability", &district.desirability);
        if (CommitValueOnDeactivate(
                history,
                "District Desirability",
                district.desirability,
                [&district](float v) { district.desirability = v; })) {
            MarkDirtyForKind(gs, VpEntityKind::District);
        }
    }

    ImGui::Text("Border Points: %zu", district.border.size());

    DrawLayerAssignment(gs, VpEntityKind::District, district.id);

    ImGui::SeparatorText("Boundary Editor");
    API::Checkbox("Enable Boundary Edit", &gs.district_boundary_editor.enabled);
    API::SameLine();
    API::Checkbox("Insert Mode", &gs.district_boundary_editor.insert_mode);
    API::SameLine();
    API::Checkbox("Delete Mode", &gs.district_boundary_editor.delete_mode);
    API::Checkbox("Snap To Grid", &gs.district_boundary_editor.snap_to_grid);
    API::SetNextItemWidth(120.0f);
    API::DragFloat("Snap Size", &gs.district_boundary_editor.snap_size, 0.5f, 0.5f, 200.0f, "%.1f");

    for (size_t i = 0; i < district.border.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        float vertex[2] = {
            static_cast<float>(district.border[i].x),
            static_cast<float>(district.border[i].y)
        };
        if (API::InputFloat2("V", vertex, "%.2f")) {
            const auto before = district.border;
            district.border[i].x = vertex[0];
            district.border[i].y = vertex[1];
            const auto after = district.border;
            CommitLambda(
                history,
                "District Boundary Vertex",
                [&district, before]() { district.border = before; },
                [&district, after]() { district.border = after; });
            MarkDirtyForKind(gs, VpEntityKind::District);
        }
        API::SameLine();
        if (API::SmallButton("+")) {
            const auto before = district.border;
            const auto& a = district.border[i];
            const auto& b = district.border[(i + 1) % district.border.size()];
            RogueCity::App::EditorManipulation::InsertDistrictVertex(
                district,
                i,
                RogueCity::Core::Vec2((a.x + b.x) * 0.5, (a.y + b.y) * 0.5));
            const auto after = district.border;
            CommitLambda(
                history,
                "District Boundary Insert",
                [&district, before]() { district.border = before; },
                [&district, after]() { district.border = after; });
            MarkDirtyForKind(gs, VpEntityKind::District);
        }
        API::SameLine();
        if (API::SmallButton("-") && district.border.size() > 3) {
            const auto before = district.border;
            RogueCity::App::EditorManipulation::RemoveDistrictVertex(district, i);
            const auto after = district.border;
            CommitLambda(
                history,
                "District Boundary Remove",
                [&district, before]() { district.border = before; },
                [&district, after]() { district.border = after; });
            MarkDirtyForKind(gs, VpEntityKind::District);
        }
        ImGui::PopID();
    }
}

void DrawSingleLot(GlobalState& gs, RogueCity::Core::LotToken& lot) {
    auto& history = PropertyHistory();
    ImGui::Text("Selection: Lot");
    API::Separator();
    API::InputScalar("Lot ID", ImGuiDataType_U32, &lot.id);
    API::InputScalar("District ID", ImGuiDataType_U32, &lot.district_id);

    {
        float centroid_xy[2] = { static_cast<float>(lot.centroid.x), static_cast<float>(lot.centroid.y) };
        if (API::InputFloat2("Centroid", centroid_xy)) {
            lot.centroid.x = centroid_xy[0];
            lot.centroid.y = centroid_xy[1];
        }
        if (CommitValueOnDeactivate(
                history,
                "Lot Centroid",
                lot.centroid,
                [&lot](const RogueCity::Core::Vec2& v) { lot.centroid = v; })) {
            MarkDirtyForKind(gs, VpEntityKind::Lot);
        }
    }

    {
        const auto before = lot.lot_type;
        if (DrawEnumCombo("Lot Type", lot.lot_type)) {
            const auto after = lot.lot_type;
            CommitValueChange(
                history,
                "Lot Type",
                before,
                after,
                [&lot](RogueCity::Core::LotType v) { lot.lot_type = v; });
            MarkDirtyForKind(gs, VpEntityKind::Lot);
        }
    }

    DrawUserFlagBlock(history, gs, lot, VpEntityKind::Lot,
        &RogueCity::Core::LotToken::is_user_placed, "User Placed", "Lot User Flag");
    DrawGenerationLocked(history, gs, lot, VpEntityKind::Lot, "Lot Generation Lock");

    {
        const bool before = lot.locked_type;
        if (API::Checkbox("Locked Type", &lot.locked_type)) {
            const bool after = lot.locked_type;
            CommitValueChange(
                history,
                "Lot Locked Type",
                before,
                after,
                [&lot](bool v) { lot.locked_type = v; });
            MarkDirtyForKind(gs, VpEntityKind::Lot);
        }
    }

    DrawLayerAssignment(gs, VpEntityKind::Lot, lot.id);

    ImGui::SeparatorText("AESP Scores");
    ImGui::Text("Access: %.2f", lot.access);
    ImGui::Text("Exposure: %.2f", lot.exposure);
    ImGui::Text("Service: %.2f", lot.serviceability);
    ImGui::Text("Privacy: %.2f", lot.privacy);
    ImGui::Text("Area: %.2f", lot.area);
}

void DrawSingleBuilding(GlobalState& gs, RogueCity::Core::BuildingSite& building) {
    auto& history = PropertyHistory();
    ImGui::Text("Selection: Building");
    API::Separator();
    API::InputScalar("Building ID", ImGuiDataType_U32, &building.id);
    API::InputScalar("Lot ID", ImGuiDataType_U32, &building.lot_id);
    API::InputScalar("District ID", ImGuiDataType_U32, &building.district_id);

    {
        float pos_xy[2] = { static_cast<float>(building.position.x), static_cast<float>(building.position.y) };
        if (API::InputFloat2("Position", pos_xy)) {
            building.position.x = pos_xy[0];
            building.position.y = pos_xy[1];
        }
        if (CommitValueOnDeactivate(
                history,
                "Building Position",
                building.position,
                [&building](const RogueCity::Core::Vec2& v) { building.position = v; })) {
            MarkDirtyForKind(gs, VpEntityKind::Building);
        }
    }

    {
        const auto before = building.type;
        if (DrawEnumCombo("Type", building.type)) {
            const auto after = building.type;
            CommitValueChange(
                history,
                "Building Type",
                before,
                after,
                [&building](RogueCity::Core::BuildingType v) { building.type = v; });
            MarkDirtyForKind(gs, VpEntityKind::Building);
        }
    }

    DrawUserFlagBlock(history, gs, building, VpEntityKind::Building,
        &RogueCity::Core::BuildingSite::is_user_placed, "User Placed", "Building User Flag");
    DrawGenerationLocked(history, gs, building, VpEntityKind::Building, "Building Generation Lock");

    {
        const bool before = building.locked_type;
        if (API::Checkbox("Locked Type", &building.locked_type)) {
            const bool after = building.locked_type;
            CommitValueChange(
                history,
                "Building Locked Type",
                before,
                after,
                [&building](bool v) { building.locked_type = v; });
            MarkDirtyForKind(gs, VpEntityKind::Building);
        }
    }

    {
        API::InputFloat("Estimated Cost", &building.estimated_cost);
        if (CommitValueOnDeactivate(
                history,
                "Building Cost",
                building.estimated_cost,
                [&building](float v) { building.estimated_cost = v; })) {
            MarkDirtyForKind(gs, VpEntityKind::Building);
        }
    }

    {
        API::InputFloat("Rotation (rad)", &building.rotation_radians);
        if (CommitValueOnDeactivate(
                history,
                "Building Rotation",
                building.rotation_radians,
                [&building](float v) { building.rotation_radians = v; })) {
            MarkDirtyForKind(gs, VpEntityKind::Building);
        }
    }

    {
        if (API::DragFloat("Uniform Scale", &building.uniform_scale, 0.01f, 0.05f, 20.0f, "%.2f")) {
            building.uniform_scale = std::clamp(building.uniform_scale, 0.05f, 20.0f);
        }
        if (CommitValueOnDeactivate(
                history,
                "Building Scale",
                building.uniform_scale,
                [&building](float v) { building.uniform_scale = v; })) {
            MarkDirtyForKind(gs, VpEntityKind::Building);
        }
    }

    DrawLayerAssignment(gs, VpEntityKind::Building, building.id);
}

template <typename TEntity, typename TFind, typename TApply, typename TValue>
void ApplyBatchValue(
    GlobalState& gs,
    VpEntityKind kind,
    const std::vector<uint32_t>& ids,
    const char* description,
    TValue before,
    TValue after,
    TFind find_entity,
    TApply apply_value) {
    if (before == after) {
        return;
    }

    auto apply_snapshot = [&](TValue value) {
        for (uint32_t id : ids) {
            if (TEntity* entity = find_entity(gs, id); entity != nullptr) {
                apply_value(*entity, value);
            }
        }
    };

    CommitLambda(
        PropertyHistory(),
        description,
        [apply_snapshot, before]() mutable { apply_snapshot(before); },
        [apply_snapshot, after]() mutable { apply_snapshot(after); });

    MarkDirtyForKind(gs, kind);
}

void DrawBatchEditor(GlobalState& gs, VpEntityKind kind, const std::vector<uint32_t>& ids) {
    ImGui::Text("Multiple selection (%zu)", ids.size());
    API::Separator();

    if (API::Button("Assign Batch -> Active Layer")) {
        const uint8_t layer = gs.layer_manager.active_layer;
        for (uint32_t id : ids) {
            gs.SetEntityLayer(kind, id, layer);
        }
        gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
    }
    API::Separator();

    if (kind == VpEntityKind::Road) {
        ImGui::Text("Batch Edit: Roads");
        RogueCity::Core::RoadType type = RogueCity::Core::RoadType::Street;
        if (auto* first = FindRoad(gs, ids.front())) {
            type = first->type;
        }
        const auto before_type = type;
        if (DrawEnumCombo("Type", type)) {
            ApplyBatchValue<RogueCity::Core::Road>(
                gs,
                kind,
                ids,
                "Batch Road Type",
                before_type,
                type,
                FindRoad,
                [](RogueCity::Core::Road& road, RogueCity::Core::RoadType v) { road.type = v; });
        }

        bool user_created = false;
        if (auto* first = FindRoad(gs, ids.front())) {
            user_created = first->is_user_created;
        }
        const bool before_user = user_created;
        if (API::Checkbox("User Created", &user_created)) {
            ApplyBatchValue<RogueCity::Core::Road>(
                gs,
                kind,
                ids,
                "Batch Road User Flag",
                before_user,
                user_created,
                FindRoad,
                [](RogueCity::Core::Road& road, bool v) { road.is_user_created = v; });
        }
        return;
    }

    if (kind == VpEntityKind::District) {
        ImGui::Text("Batch Edit: Districts");
        RogueCity::Core::DistrictType type = RogueCity::Core::DistrictType::Mixed;
        if (auto* first = FindDistrict(gs, ids.front())) {
            type = first->type;
        }
        const auto before = type;
        if (DrawEnumCombo("Type", type)) {
            ApplyBatchValue<RogueCity::Core::District>(
                gs,
                kind,
                ids,
                "Batch District Type",
                before,
                type,
                FindDistrict,
                [](RogueCity::Core::District& district, RogueCity::Core::DistrictType v) { district.type = v; });
        }
        return;
    }

    if (kind == VpEntityKind::Lot) {
        ImGui::Text("Batch Edit: Lots");
        RogueCity::Core::LotType lot_type = RogueCity::Core::LotType::None;
        if (auto* first = FindLot(gs, ids.front())) {
            lot_type = first->lot_type;
        }
        const auto before_type = lot_type;
        if (DrawEnumCombo("Lot Type", lot_type)) {
            ApplyBatchValue<RogueCity::Core::LotToken>(
                gs,
                kind,
                ids,
                "Batch Lot Type",
                before_type,
                lot_type,
                FindLot,
                [](RogueCity::Core::LotToken& lot, RogueCity::Core::LotType v) { lot.lot_type = v; });
        }

        bool locked = false;
        if (auto* first = FindLot(gs, ids.front())) {
            locked = first->locked_type;
        }
        const bool before_locked = locked;
        if (API::Checkbox("Locked Type", &locked)) {
            ApplyBatchValue<RogueCity::Core::LotToken>(
                gs,
                kind,
                ids,
                "Batch Lot Locked",
                before_locked,
                locked,
                FindLot,
                [](RogueCity::Core::LotToken& lot, bool v) { lot.locked_type = v; });
        }
        return;
    }

    if (kind == VpEntityKind::Building) {
        ImGui::Text("Batch Edit: Buildings");
        RogueCity::Core::BuildingType type = RogueCity::Core::BuildingType::None;
        if (auto* first = FindBuilding(gs, ids.front())) {
            type = first->type;
        }
        const auto before_type = type;
        if (DrawEnumCombo("Type", type)) {
            ApplyBatchValue<RogueCity::Core::BuildingSite>(
                gs,
                kind,
                ids,
                "Batch Building Type",
                before_type,
                type,
                FindBuilding,
                [](RogueCity::Core::BuildingSite& building, RogueCity::Core::BuildingType v) { building.type = v; });
        }

        bool locked = false;
        if (auto* first = FindBuilding(gs, ids.front())) {
            locked = first->locked_type;
        }
        const bool before_locked = locked;
        if (API::Checkbox("Locked Type", &locked)) {
            ApplyBatchValue<RogueCity::Core::BuildingSite>(
                gs,
                kind,
                ids,
                "Batch Building Locked",
                before_locked,
                locked,
                FindBuilding,
                [](RogueCity::Core::BuildingSite& building, bool v) { building.locked_type = v; });
        }
        return;
    }

    ImGui::Text("Batch editing is unavailable for this mixed selection.");
}

void DrawQuerySelection(GlobalState& gs) {
    ImGui::SeparatorText("Query Selection");

    static int kind_filter = 0;       // 0 Any, 1 Road, 2 District, 3 Lot, 4 Building
    static int id_min = -1;
    static int id_max = -1;
    static int district_filter = -1;
    static bool user_only = false;

    const char* kinds[] = {"Any", "Road", "District", "Lot", "Building"};
    API::SetNextItemWidth(140.0f);
    API::Combo("Kind", &kind_filter, kinds, IM_ARRAYSIZE(kinds));
    API::SetNextItemWidth(100.0f);
    API::InputInt("ID Min", &id_min);
    API::SetNextItemWidth(100.0f);
    API::InputInt("ID Max", &id_max);
    API::SetNextItemWidth(100.0f);
    API::InputInt("District ID", &district_filter);
    API::Checkbox("User-Created Only", &user_only);

    if (API::Button("Apply Query")) {
        std::vector<SelectionItem> items;

        auto id_in_range = [&](uint32_t id) {
            if (id_min >= 0 && static_cast<int>(id) < id_min) {
                return false;
            }
            if (id_max >= 0 && static_cast<int>(id) > id_max) {
                return false;
            }
            return true;
        };

        if (kind_filter == 0 || kind_filter == 1) {
            for (const auto& road : gs.roads) {
                if (!id_in_range(road.id)) {
                    continue;
                }
                if (user_only && !road.is_user_created) {
                    continue;
                }
                items.push_back({VpEntityKind::Road, road.id});
            }
        }
        if (kind_filter == 0 || kind_filter == 2) {
            for (const auto& district : gs.districts) {
                if (!id_in_range(district.id)) {
                    continue;
                }
                items.push_back({VpEntityKind::District, district.id});
            }
        }
        if (kind_filter == 0 || kind_filter == 3) {
            for (const auto& lot : gs.lots) {
                if (!id_in_range(lot.id)) {
                    continue;
                }
                if (district_filter >= 0 && static_cast<int>(lot.district_id) != district_filter) {
                    continue;
                }
                if (user_only && !lot.is_user_placed) {
                    continue;
                }
                items.push_back({VpEntityKind::Lot, lot.id});
            }
        }
        if (kind_filter == 0 || kind_filter == 4) {
            for (const auto& building : gs.buildings) {
                if (!id_in_range(building.id)) {
                    continue;
                }
                if (district_filter >= 0 && static_cast<int>(building.district_id) != district_filter) {
                    continue;
                }
                if (user_only && !building.is_user_placed) {
                    continue;
                }
                items.push_back({VpEntityKind::Building, building.id});
            }
        }

        gs.selection_manager.SetItems(std::move(items));
        RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
    }

    API::SameLine();
    if (API::Button("Clear Selection")) {
        gs.selection_manager.Clear();
        RogueCity::Core::Editor::ClearPrimarySelection(gs.selection);
    }
}

} // namespace

void PropertyEditor::Draw(GlobalState& gs) {
    auto& history = PropertyHistory();
    ImGuiIO& io = ImGui::GetIO();

    const bool can_undo = history.CanUndo();
    const bool can_redo = history.CanRedo();
    if (!can_undo) {
        API::BeginDisabled();
    }
    if (API::Button("Property Undo")) {
        history.Undo();
        gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
    }
    if (!can_undo) {
        API::EndDisabled();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", can_undo ? history.PeekUndo()->GetDescription() : "No property edits");
    }

    API::SameLine();
    if (!can_redo) {
        API::BeginDisabled();
    }
    if (API::Button("Property Redo")) {
        history.Redo();
        gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
    }
    if (!can_redo) {
        API::EndDisabled();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", can_redo ? history.PeekRedo()->GetDescription() : "No property edits");
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && !io.WantTextInput) {
        const bool redo_pressed =
            RC_UI::Keymap::IsPressed(RC_UI::Keymap::Action::kRedoShiftZ) ||
            RC_UI::Keymap::IsPressed(RC_UI::Keymap::Action::kRedo);
        if (redo_pressed && history.CanRedo()) {
            history.Redo();
            gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
        } else if (RC_UI::Keymap::IsPressed(RC_UI::Keymap::Action::kUndo) && history.CanUndo()) {
            history.Undo();
            gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
        }
    }

    const auto selection_count = gs.selection_manager.Count();
    if (selection_count > 1) {
        const auto items = gs.selection_manager.Items();
        const VpEntityKind first_kind = items.front().kind;
        bool same_kind = true;
        std::vector<uint32_t> ids;
        ids.reserve(items.size());
        for (const auto& item : items) {
            ids.push_back(item.id);
            if (item.kind != first_kind) {
                same_kind = false;
            }
        }

        if (same_kind) {
            DrawBatchEditor(gs, first_kind, ids);
        } else {
            ImGui::Text("Multiple selection (%zu)", selection_count);
            ImGui::Text("Mixed entity kinds. Use Query Selection to refine.");
            for (const auto& item : items) {
                ImGui::BulletText("Kind %u: %u", static_cast<uint32_t>(item.kind), item.id);
            }
        }
    } else if (gs.selection.selected_building) {
        DrawSingleBuilding(gs, *gs.selection.selected_building);
    } else if (gs.selection.selected_lot) {
        DrawSingleLot(gs, *gs.selection.selected_lot);
    } else if (gs.selection.selected_district) {
        DrawSingleDistrict(gs, *gs.selection.selected_district);
    } else if (gs.selection.selected_road) {
        DrawSingleRoad(gs, *gs.selection.selected_road);
    } else {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::TextSecondary, 217u)), "No selection");
    }

    DrawQuerySelection(gs);
}

} // namespace RC_UI::Panels
