#include "ui/tools/rc_tool_dispatcher.h"

#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"

#include <utility>
#include <vector>

namespace RC_UI::Tools {
namespace {

using RogueCity::Core::Editor::BuildingSubtool;
using RogueCity::Core::Editor::DistrictSubtool;
using RogueCity::Core::Editor::EditorEvent;
using RogueCity::Core::Editor::LotSubtool;
using RogueCity::Core::Editor::RoadSplineSubtool;
using RogueCity::Core::Editor::RoadSubtool;
using RogueCity::Core::Editor::ToolDomain;
using RogueCity::Core::Editor::WaterSplineSubtool;
using RogueCity::Core::Editor::WaterSubtool;

void RegisterTriggeredAction(const ToolActionSpec& action,
                             RogueCity::UIInt::UiIntrospector* uiint,
                             const char* panel_id,
                             const char* handler_symbol,
                             const char* status) {
    if (uiint == nullptr) {
        return;
    }

    std::vector<std::string> tags = {"tool", "dispatch", "triggered"};
    if (status != nullptr && status[0] != '\0') {
        tags.emplace_back(std::string("status:") + status);
    }

    uiint->RegisterAction({
        ToolActionName(action.id),
        action.label,
        panel_id != nullptr ? panel_id : "Tool Library",
        std::move(tags),
        handler_symbol != nullptr ? handler_symbol : "RC_UI::Tools::DispatchToolAction"
    });
}

void RecordRuntimeAction(RogueCity::Core::Editor::GlobalState& gs,
                         const ToolActionSpec& action,
                         const char* status) {
    gs.tool_runtime.last_action_id = ToolActionName(action.id);
    gs.tool_runtime.last_action_label = action.label != nullptr ? action.label : "";
    gs.tool_runtime.last_action_status = status != nullptr ? status : "";
    gs.tool_runtime.action_serial += 1;
    gs.tool_runtime.last_action_frame = gs.frame_counter;
}

void ActivateModeForAction(const ToolActionSpec& action,
                           RogueCity::Core::Editor::EditorHFSM& hfsm,
                           RogueCity::Core::Editor::GlobalState& gs) {
    switch (action.domain) {
        case ToolDomain::Axiom:
            hfsm.handle_event(EditorEvent::Tool_Axioms, gs);
            gs.tool_runtime.active_domain = ToolDomain::Axiom;
            break;
        case ToolDomain::Water:
        case ToolDomain::Flow:
            hfsm.handle_event(EditorEvent::Tool_Water, gs);
            gs.tool_runtime.active_domain = action.domain;
            break;
        case ToolDomain::Road:
        case ToolDomain::Paths:
            hfsm.handle_event(EditorEvent::Tool_Roads, gs);
            gs.tool_runtime.active_domain = action.domain;
            break;
        case ToolDomain::District:
            hfsm.handle_event(EditorEvent::Tool_Districts, gs);
            gs.tool_runtime.active_domain = ToolDomain::District;
            break;
        case ToolDomain::Zone:
            hfsm.handle_event(EditorEvent::Tool_Districts, gs);
            gs.tool_runtime.active_domain = ToolDomain::Zone;
            break;
        case ToolDomain::Lot:
            hfsm.handle_event(EditorEvent::Tool_Lots, gs);
            gs.tool_runtime.active_domain = ToolDomain::Lot;
            break;
        case ToolDomain::Building:
        case ToolDomain::FloorPlan:
        case ToolDomain::Furnature:
            hfsm.handle_event(EditorEvent::Tool_Buildings, gs);
            gs.tool_runtime.active_domain = action.domain;
            break;
    }
}

void ApplySubtoolSelection(const ToolActionSpec& action,
                           RogueCity::Core::Editor::GlobalState& gs) {
    switch (action.id) {
        case ToolActionId::Axiom_Organic:
        case ToolActionId::Axiom_Grid:
        case ToolActionId::Axiom_Radial:
        case ToolActionId::Axiom_Hexagonal:
        case ToolActionId::Axiom_Stem:
        case ToolActionId::Axiom_LooseGrid:
        case ToolActionId::Axiom_Suburban:
        case ToolActionId::Axiom_Superblock:
        case ToolActionId::Axiom_Linear:
        case ToolActionId::Axiom_GridCorrective:
            gs.tool_runtime.active_domain = ToolDomain::Axiom;
            break;

        case ToolActionId::Water_Flow:
            gs.tool_runtime.water_subtool = WaterSubtool::Flow;
            gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::Pen;
            break;
        case ToolActionId::Water_Contour:
            gs.tool_runtime.water_subtool = WaterSubtool::Contour;
            gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::Pen;
            break;
        case ToolActionId::Water_Erode:
            gs.tool_runtime.water_subtool = WaterSubtool::Erode;
            gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::Pen;
            break;
        case ToolActionId::Water_Select:
            gs.tool_runtime.water_subtool = WaterSubtool::Select;
            gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::Selection;
            break;
        case ToolActionId::Water_Mask:
            gs.tool_runtime.water_subtool = WaterSubtool::Mask;
            gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::Pen;
            break;
        case ToolActionId::Water_Inspect:
            gs.tool_runtime.water_subtool = WaterSubtool::Inspect;
            gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::Selection;
            break;

        case ToolActionId::WaterSpline_Selection: gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::Selection; break;
        case ToolActionId::WaterSpline_DirectSelect: gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::DirectSelect; break;
        case ToolActionId::WaterSpline_Pen: gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::Pen; break;
        case ToolActionId::WaterSpline_ConvertAnchor: gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::ConvertAnchor; break;
        case ToolActionId::WaterSpline_AddRemoveAnchor: gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::AddRemoveAnchor; break;
        case ToolActionId::WaterSpline_HandleTangents: gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::HandleTangents; break;
        case ToolActionId::WaterSpline_SnapAlign: gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::SnapAlign; break;
        case ToolActionId::WaterSpline_JoinSplit: gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::JoinSplit; break;
        case ToolActionId::WaterSpline_Simplify: gs.tool_runtime.water_spline_subtool = WaterSplineSubtool::Simplify; break;

        case ToolActionId::Road_Spline:
            gs.tool_runtime.road_subtool = RoadSubtool::Spline;
            gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Pen;
            break;
        case ToolActionId::Road_Grid:
            gs.tool_runtime.road_subtool = RoadSubtool::Grid;
            gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Pen;
            break;
        case ToolActionId::Road_Bridge:
            gs.tool_runtime.road_subtool = RoadSubtool::Bridge;
            gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Pen;
            break;
        case ToolActionId::Road_Select:
            gs.tool_runtime.road_subtool = RoadSubtool::Select;
            gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Selection;
            break;
        case ToolActionId::Road_Disconnect:
            gs.tool_runtime.road_subtool = RoadSubtool::Disconnect;
            gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Selection;
            break;
        case ToolActionId::Road_Stub:
            gs.tool_runtime.road_subtool = RoadSubtool::Stub;
            gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Pen;
            break;
        case ToolActionId::Road_Curve:
            gs.tool_runtime.road_subtool = RoadSubtool::Curve;
            gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Pen;
            break;
        case ToolActionId::Road_Strengthen:
            gs.tool_runtime.road_subtool = RoadSubtool::Strengthen;
            gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Pen;
            break;
        case ToolActionId::Road_Inspect:
            gs.tool_runtime.road_subtool = RoadSubtool::Inspect;
            gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Selection;
            break;

        case ToolActionId::RoadSpline_Selection: gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Selection; break;
        case ToolActionId::RoadSpline_DirectSelect: gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::DirectSelect; break;
        case ToolActionId::RoadSpline_Pen: gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Pen; break;
        case ToolActionId::RoadSpline_ConvertAnchor: gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::ConvertAnchor; break;
        case ToolActionId::RoadSpline_AddRemoveAnchor: gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::AddRemoveAnchor; break;
        case ToolActionId::RoadSpline_HandleTangents: gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::HandleTangents; break;
        case ToolActionId::RoadSpline_SnapAlign: gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::SnapAlign; break;
        case ToolActionId::RoadSpline_JoinSplit: gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::JoinSplit; break;
        case ToolActionId::RoadSpline_Simplify: gs.tool_runtime.road_spline_subtool = RoadSplineSubtool::Simplify; break;

        case ToolActionId::District_Zone: gs.tool_runtime.district_subtool = DistrictSubtool::Zone; break;
        case ToolActionId::District_Paint: gs.tool_runtime.district_subtool = DistrictSubtool::Paint; break;
        case ToolActionId::District_Split: gs.tool_runtime.district_subtool = DistrictSubtool::Split; break;
        case ToolActionId::District_Select: gs.tool_runtime.district_subtool = DistrictSubtool::Select; break;
        case ToolActionId::District_Merge: gs.tool_runtime.district_subtool = DistrictSubtool::Merge; break;
        case ToolActionId::District_Inspect: gs.tool_runtime.district_subtool = DistrictSubtool::Inspect; break;

        case ToolActionId::Lot_Plot: gs.tool_runtime.lot_subtool = LotSubtool::Plot; break;
        case ToolActionId::Lot_Slice: gs.tool_runtime.lot_subtool = LotSubtool::Slice; break;
        case ToolActionId::Lot_Align: gs.tool_runtime.lot_subtool = LotSubtool::Align; break;
        case ToolActionId::Lot_Select: gs.tool_runtime.lot_subtool = LotSubtool::Select; break;
        case ToolActionId::Lot_Merge: gs.tool_runtime.lot_subtool = LotSubtool::Merge; break;
        case ToolActionId::Lot_Inspect: gs.tool_runtime.lot_subtool = LotSubtool::Inspect; break;

        case ToolActionId::Building_Place: gs.tool_runtime.building_subtool = BuildingSubtool::Place; break;
        case ToolActionId::Building_Scale: gs.tool_runtime.building_subtool = BuildingSubtool::Scale; break;
        case ToolActionId::Building_Rotate: gs.tool_runtime.building_subtool = BuildingSubtool::Rotate; break;
        case ToolActionId::Building_Select: gs.tool_runtime.building_subtool = BuildingSubtool::Select; break;
        case ToolActionId::Building_Assign: gs.tool_runtime.building_subtool = BuildingSubtool::Assign; break;
        case ToolActionId::Building_Inspect: gs.tool_runtime.building_subtool = BuildingSubtool::Inspect; break;

        case ToolActionId::Future_FloorPlan:
        case ToolActionId::Future_Paths:
        case ToolActionId::Future_Flow:
        case ToolActionId::Future_Furnature:
        case ToolActionId::Count:
            break;
    }
}

} // namespace

DispatchResult DispatchToolAction(ToolActionId action_id,
                                  const DispatchContext& context,
                                  std::string* out_status) {
    if (context.hfsm == nullptr || context.gs == nullptr) {
        if (out_status != nullptr) {
            *out_status = "invalid-dispatch-context";
        }
        return DispatchResult::InvalidContext;
    }

    const ToolActionSpec* action = FindToolAction(action_id);
    if (action == nullptr) {
        if (out_status != nullptr) {
            *out_status = "unknown-tool-action";
        }
        return DispatchResult::UnknownAction;
    }

    if (!IsToolActionEnabled(*action)) {
        const char* status = "disabled";
        RecordRuntimeAction(*context.gs, *action, status);
        RegisterTriggeredAction(*action, context.introspector, context.panel_id, "RC_UI::Tools::DispatchToolAction", status);

        if (out_status != nullptr) {
            *out_status = action->disabled_reason != nullptr && action->disabled_reason[0] != '\0'
                ? action->disabled_reason
                : "disabled";
        }
        return DispatchResult::Disabled;
    }

    ActivateModeForAction(*action, *context.hfsm, *context.gs);
    ApplySubtoolSelection(*action, *context.gs);

    const char* status = "handled";
    RecordRuntimeAction(*context.gs, *action, status);
    RegisterTriggeredAction(*action, context.introspector, context.panel_id, "RC_UI::Tools::DispatchToolAction", status);

    if (out_status != nullptr) {
        *out_status = status;
    }

    return DispatchResult::Handled;
}

} // namespace RC_UI::Tools
