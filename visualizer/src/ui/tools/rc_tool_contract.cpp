#include "ui/tools/rc_tool_contract.h"

#include <array>
#include <algorithm>

namespace RC_UI::Tools {
namespace {

using Spec = ToolActionSpec;
using ToolDomain = RogueCity::Core::Editor::ToolDomain;

constexpr std::array<Spec, 10> kAxiomActions{ {
    {ToolActionId::Axiom_Organic, ToolDomain::Axiom, ToolLibrary::Axiom, ToolActionGroup::Primary, "Organic", "Activate Organic as the active/default axiom type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Axiom_Grid, ToolDomain::Axiom, ToolLibrary::Axiom, ToolActionGroup::Primary, "Grid", "Activate Grid as the active/default axiom type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Axiom_Radial, ToolDomain::Axiom, ToolLibrary::Axiom, ToolActionGroup::Primary, "Radial", "Activate Radial as the active/default axiom type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Axiom_Hexagonal, ToolDomain::Axiom, ToolLibrary::Axiom, ToolActionGroup::Primary, "Hexagonal", "Activate Hexagonal as the active/default axiom type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Axiom_Stem, ToolDomain::Axiom, ToolLibrary::Axiom, ToolActionGroup::Primary, "Stem", "Activate Stem as the active/default axiom type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Axiom_LooseGrid, ToolDomain::Axiom, ToolLibrary::Axiom, ToolActionGroup::Primary, "Loose Grid", "Activate Loose Grid as the active/default axiom type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Axiom_Suburban, ToolDomain::Axiom, ToolLibrary::Axiom, ToolActionGroup::Primary, "Suburban", "Activate Suburban as the active/default axiom type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Axiom_Superblock, ToolDomain::Axiom, ToolLibrary::Axiom, ToolActionGroup::Primary, "Superblock", "Activate Superblock as the active/default axiom type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Axiom_Linear, ToolDomain::Axiom, ToolLibrary::Axiom, ToolActionGroup::Primary, "Linear", "Activate Linear as the active/default axiom type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Axiom_GridCorrective, ToolDomain::Axiom, ToolLibrary::Axiom, ToolActionGroup::Primary, "Grid Corrective", "Activate Grid Corrective as the active/default axiom type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
} };

constexpr std::array<Spec, 16> kWaterActions{ {
    {ToolActionId::Water_Flow, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Primary, "Flow", "Activate hydrology flow authoring mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Water_Contour, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Primary, "Contour", "Activate contour-aware water shaping tools.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Water_Erode, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Primary, "Erode", "Activate erosion-oriented water adjustments.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Water_Select, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Primary, "Select", "Activate water entity selection mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Water_Mask, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Primary, "Mask", "Activate water masking controls.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Water_Inspect, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Primary, "Inspect", "Inspect selected water entities in properties.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},

    {ToolActionId::WaterSpline_Selection, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Spline, "Selection", "Spline selection helper for water boundaries.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::WaterSpline_DirectSelect, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Spline, "Direct Select", "Direct-select spline control points.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::WaterSpline_Pen, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Spline, "Pen", "Draw water splines with pen mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::WaterSpline_ConvertAnchor, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Spline, "Convert Anchor", "Convert selected spline anchor type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::WaterSpline_AddRemoveAnchor, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Spline, "Add/Remove Anchor", "Insert or remove spline anchors.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::WaterSpline_HandleTangents, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Spline, "Handle Tangents", "Adjust tangent handles for smooth curves.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::WaterSpline_SnapAlign, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Spline, "Snap/Align", "Snap spline edits to guide constraints.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::WaterSpline_JoinSplit, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Spline, "Join/Split", "Join or split water spline segments.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::WaterSpline_Simplify, ToolDomain::Water, ToolLibrary::Water, ToolActionGroup::Spline, "Simplify", "Simplify water spline vertex density.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},

    {ToolActionId::Future_Flow, ToolDomain::Flow, ToolLibrary::Water, ToolActionGroup::FutureStub, "Flow (Future)", "Pedestrian/traffic flow tooling is planned but not wired yet.", ToolExecutionPolicy::Disabled, ToolAvailability::Disabled, "Planned module: Flow (pedestrian/traffic control)."},
} };

constexpr std::array<Spec, 19> kRoadActions{ {
    {ToolActionId::Road_Spline, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Primary, "Spline", "Activate spline road creation workflow.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Road_Grid, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Primary, "Grid", "Activate grid-oriented road shaping mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Road_Bridge, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Primary, "Bridge", "Activate bridge-placement compatible road mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Road_Select, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Primary, "Select", "Select roads for edit and inspection.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Road_Disconnect, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Primary, "Disconnect", "Prepare road disconnection edits.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Road_Stub, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Primary, "Stub", "Activate short-stub road editing mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Road_Curve, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Primary, "Curve", "Activate curve refinement mode for roads.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Road_Strengthen, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Primary, "Strengthen", "Activate road hierarchy strengthening mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Road_Inspect, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Primary, "Inspect", "Inspect selected roads in properties.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},

    {ToolActionId::RoadSpline_Selection, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Spline, "Selection", "Spline selection helper for roads.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::RoadSpline_DirectSelect, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Spline, "Direct Select", "Direct-select road spline control points.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::RoadSpline_Pen, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Spline, "Pen", "Draw road splines with pen mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::RoadSpline_ConvertAnchor, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Spline, "Convert Anchor", "Convert selected road spline anchor type.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::RoadSpline_AddRemoveAnchor, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Spline, "Add/Remove Anchor", "Insert or remove road spline anchors.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::RoadSpline_HandleTangents, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Spline, "Handle Tangents", "Adjust tangent handles for road curves.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::RoadSpline_SnapAlign, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Spline, "Snap/Align", "Snap road spline edits to guide constraints.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::RoadSpline_JoinSplit, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Spline, "Join/Split", "Join or split road spline segments.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::RoadSpline_Simplify, ToolDomain::Road, ToolLibrary::Road, ToolActionGroup::Spline, "Simplify", "Simplify road spline vertex density.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},

    {ToolActionId::Future_Paths, ToolDomain::Paths, ToolLibrary::Road, ToolActionGroup::FutureStub, "Paths (Future)", "Navigation map pathing tools are planned but not wired yet.", ToolExecutionPolicy::Disabled, ToolAvailability::Disabled, "Planned module: Paths (navmap)."},
} };

constexpr std::array<Spec, 6> kDistrictActions{ {
    {ToolActionId::District_Zone, ToolDomain::Zone, ToolLibrary::District, ToolActionGroup::Primary, "Zone", "Activate zone painting mapped to district/zoning pipeline.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::District_Paint, ToolDomain::District, ToolLibrary::District, ToolActionGroup::Primary, "Paint", "Activate district paint mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::District_Split, ToolDomain::District, ToolLibrary::District, ToolActionGroup::Primary, "Split", "Activate district split operations.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::District_Select, ToolDomain::District, ToolLibrary::District, ToolActionGroup::Primary, "Select", "Select district entities for editing.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::District_Merge, ToolDomain::District, ToolLibrary::District, ToolActionGroup::Primary, "Merge", "Activate district merge operations.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::District_Inspect, ToolDomain::District, ToolLibrary::District, ToolActionGroup::Primary, "Inspect", "Inspect district entities in properties.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
} };

constexpr std::array<Spec, 6> kLotActions{ {
    {ToolActionId::Lot_Plot, ToolDomain::Lot, ToolLibrary::Lot, ToolActionGroup::Primary, "Plot", "Activate lot plotting mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Lot_Slice, ToolDomain::Lot, ToolLibrary::Lot, ToolActionGroup::Primary, "Slice", "Activate lot slicing operations.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Lot_Align, ToolDomain::Lot, ToolLibrary::Lot, ToolActionGroup::Primary, "Align", "Activate lot alignment mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Lot_Select, ToolDomain::Lot, ToolLibrary::Lot, ToolActionGroup::Primary, "Select", "Select lot entities for edit.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Lot_Merge, ToolDomain::Lot, ToolLibrary::Lot, ToolActionGroup::Primary, "Merge", "Activate lot merge workflow.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Lot_Inspect, ToolDomain::Lot, ToolLibrary::Lot, ToolActionGroup::Primary, "Inspect", "Inspect selected lot entities.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
} };

constexpr std::array<Spec, 8> kBuildingActions{ {
    {ToolActionId::Building_Place, ToolDomain::Building, ToolLibrary::Building, ToolActionGroup::Primary, "Place", "Activate building placement mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Building_Scale, ToolDomain::Building, ToolLibrary::Building, ToolActionGroup::Primary, "Scale", "Activate building scale mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Building_Rotate, ToolDomain::Building, ToolLibrary::Building, ToolActionGroup::Primary, "Rotate", "Activate building rotate mode.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Building_Select, ToolDomain::Building, ToolLibrary::Building, ToolActionGroup::Primary, "Select", "Select building entities for editing.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Building_Assign, ToolDomain::Building, ToolLibrary::Building, ToolActionGroup::Primary, "Assign", "Assign building types and presets.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},
    {ToolActionId::Building_Inspect, ToolDomain::Building, ToolLibrary::Building, ToolActionGroup::Primary, "Inspect", "Inspect selected building entities.", ToolExecutionPolicy::ActivateOnly, ToolAvailability::Available, ""},

    {ToolActionId::Future_FloorPlan, ToolDomain::FloorPlan, ToolLibrary::Building, ToolActionGroup::FutureStub, "FloorPlan (Future)", "Interior floor plan generation is planned but not wired yet.", ToolExecutionPolicy::Disabled, ToolAvailability::Disabled, "Planned module: FloorPlan (interior generation)."},
    {ToolActionId::Future_Furnature, ToolDomain::Furnature, ToolLibrary::Building, ToolActionGroup::FutureStub, "Furnature (Future)", "Interior/exterior decoration set tooling is planned but not wired yet.", ToolExecutionPolicy::Disabled, ToolAvailability::Disabled, "Planned module: Furnature (decoration sets)."},
} };

constexpr std::array<Spec, 65> kAllActions{ {
    kAxiomActions[0], kAxiomActions[1], kAxiomActions[2], kAxiomActions[3], kAxiomActions[4],
    kAxiomActions[5], kAxiomActions[6], kAxiomActions[7], kAxiomActions[8], kAxiomActions[9],

    kWaterActions[0], kWaterActions[1], kWaterActions[2], kWaterActions[3], kWaterActions[4], kWaterActions[5],
    kWaterActions[6], kWaterActions[7], kWaterActions[8], kWaterActions[9], kWaterActions[10], kWaterActions[11],
    kWaterActions[12], kWaterActions[13], kWaterActions[14], kWaterActions[15],

    kRoadActions[0], kRoadActions[1], kRoadActions[2], kRoadActions[3], kRoadActions[4], kRoadActions[5],
    kRoadActions[6], kRoadActions[7], kRoadActions[8], kRoadActions[9], kRoadActions[10], kRoadActions[11],
    kRoadActions[12], kRoadActions[13], kRoadActions[14], kRoadActions[15], kRoadActions[16], kRoadActions[17],
    kRoadActions[18],

    kDistrictActions[0], kDistrictActions[1], kDistrictActions[2], kDistrictActions[3], kDistrictActions[4], kDistrictActions[5],

    kLotActions[0], kLotActions[1], kLotActions[2], kLotActions[3], kLotActions[4], kLotActions[5],

    kBuildingActions[0], kBuildingActions[1], kBuildingActions[2], kBuildingActions[3], kBuildingActions[4],
    kBuildingActions[5], kBuildingActions[6], kBuildingActions[7]
} };

} // namespace

std::span<const ToolActionSpec> GetToolActionsForLibrary(ToolLibrary library) {
    switch (library) {
        case ToolLibrary::Axiom: return std::span<const ToolActionSpec>(kAxiomActions.data(), kAxiomActions.size());
        case ToolLibrary::Water: return std::span<const ToolActionSpec>(kWaterActions.data(), kWaterActions.size());
        case ToolLibrary::Road: return std::span<const ToolActionSpec>(kRoadActions.data(), kRoadActions.size());
        case ToolLibrary::District: return std::span<const ToolActionSpec>(kDistrictActions.data(), kDistrictActions.size());
        case ToolLibrary::Lot: return std::span<const ToolActionSpec>(kLotActions.data(), kLotActions.size());
        case ToolLibrary::Building: return std::span<const ToolActionSpec>(kBuildingActions.data(), kBuildingActions.size());
    }
    return {};
}

std::span<const ToolActionSpec> GetToolActionCatalog() {
    return std::span<const ToolActionSpec>(kAllActions.data(), kAllActions.size());
}

const ToolActionSpec* FindToolAction(ToolActionId id) {
    const auto catalog = GetToolActionCatalog();
    const auto it = std::find_if(catalog.begin(), catalog.end(), [id](const ToolActionSpec& action) {
        return action.id == id;
    });
    return it == catalog.end() ? nullptr : &(*it);
}

bool IsToolActionEnabled(const ToolActionSpec& action) {
    return action.availability == ToolAvailability::Available && action.policy != ToolExecutionPolicy::Disabled;
}

const char* ToolDomainName(RogueCity::Core::Editor::ToolDomain domain) {
    switch (domain) {
        case ToolDomain::Axiom: return "Axiom";
        case ToolDomain::Water: return "Water";
        case ToolDomain::Road: return "Road";
        case ToolDomain::District: return "District";
        case ToolDomain::Zone: return "Zone";
        case ToolDomain::Lot: return "Lot";
        case ToolDomain::Building: return "Building";
        case ToolDomain::FloorPlan: return "FloorPlan";
        case ToolDomain::Paths: return "Paths";
        case ToolDomain::Flow: return "Flow";
        case ToolDomain::Furnature: return "Furnature";
    }
    return "Unknown";
}

const char* ToolActionName(ToolActionId id) {
    switch (id) {
        case ToolActionId::Axiom_Organic: return "Axiom_Organic";
        case ToolActionId::Axiom_Grid: return "Axiom_Grid";
        case ToolActionId::Axiom_Radial: return "Axiom_Radial";
        case ToolActionId::Axiom_Hexagonal: return "Axiom_Hexagonal";
        case ToolActionId::Axiom_Stem: return "Axiom_Stem";
        case ToolActionId::Axiom_LooseGrid: return "Axiom_LooseGrid";
        case ToolActionId::Axiom_Suburban: return "Axiom_Suburban";
        case ToolActionId::Axiom_Superblock: return "Axiom_Superblock";
        case ToolActionId::Axiom_Linear: return "Axiom_Linear";
        case ToolActionId::Axiom_GridCorrective: return "Axiom_GridCorrective";

        case ToolActionId::Water_Flow: return "Water_Flow";
        case ToolActionId::Water_Contour: return "Water_Contour";
        case ToolActionId::Water_Erode: return "Water_Erode";
        case ToolActionId::Water_Select: return "Water_Select";
        case ToolActionId::Water_Mask: return "Water_Mask";
        case ToolActionId::Water_Inspect: return "Water_Inspect";
        case ToolActionId::WaterSpline_Selection: return "WaterSpline_Selection";
        case ToolActionId::WaterSpline_DirectSelect: return "WaterSpline_DirectSelect";
        case ToolActionId::WaterSpline_Pen: return "WaterSpline_Pen";
        case ToolActionId::WaterSpline_ConvertAnchor: return "WaterSpline_ConvertAnchor";
        case ToolActionId::WaterSpline_AddRemoveAnchor: return "WaterSpline_AddRemoveAnchor";
        case ToolActionId::WaterSpline_HandleTangents: return "WaterSpline_HandleTangents";
        case ToolActionId::WaterSpline_SnapAlign: return "WaterSpline_SnapAlign";
        case ToolActionId::WaterSpline_JoinSplit: return "WaterSpline_JoinSplit";
        case ToolActionId::WaterSpline_Simplify: return "WaterSpline_Simplify";

        case ToolActionId::Road_Spline: return "Road_Spline";
        case ToolActionId::Road_Grid: return "Road_Grid";
        case ToolActionId::Road_Bridge: return "Road_Bridge";
        case ToolActionId::Road_Select: return "Road_Select";
        case ToolActionId::Road_Disconnect: return "Road_Disconnect";
        case ToolActionId::Road_Stub: return "Road_Stub";
        case ToolActionId::Road_Curve: return "Road_Curve";
        case ToolActionId::Road_Strengthen: return "Road_Strengthen";
        case ToolActionId::Road_Inspect: return "Road_Inspect";
        case ToolActionId::RoadSpline_Selection: return "RoadSpline_Selection";
        case ToolActionId::RoadSpline_DirectSelect: return "RoadSpline_DirectSelect";
        case ToolActionId::RoadSpline_Pen: return "RoadSpline_Pen";
        case ToolActionId::RoadSpline_ConvertAnchor: return "RoadSpline_ConvertAnchor";
        case ToolActionId::RoadSpline_AddRemoveAnchor: return "RoadSpline_AddRemoveAnchor";
        case ToolActionId::RoadSpline_HandleTangents: return "RoadSpline_HandleTangents";
        case ToolActionId::RoadSpline_SnapAlign: return "RoadSpline_SnapAlign";
        case ToolActionId::RoadSpline_JoinSplit: return "RoadSpline_JoinSplit";
        case ToolActionId::RoadSpline_Simplify: return "RoadSpline_Simplify";

        case ToolActionId::District_Zone: return "District_Zone";
        case ToolActionId::District_Paint: return "District_Paint";
        case ToolActionId::District_Split: return "District_Split";
        case ToolActionId::District_Select: return "District_Select";
        case ToolActionId::District_Merge: return "District_Merge";
        case ToolActionId::District_Inspect: return "District_Inspect";

        case ToolActionId::Lot_Plot: return "Lot_Plot";
        case ToolActionId::Lot_Slice: return "Lot_Slice";
        case ToolActionId::Lot_Align: return "Lot_Align";
        case ToolActionId::Lot_Select: return "Lot_Select";
        case ToolActionId::Lot_Merge: return "Lot_Merge";
        case ToolActionId::Lot_Inspect: return "Lot_Inspect";

        case ToolActionId::Building_Place: return "Building_Place";
        case ToolActionId::Building_Scale: return "Building_Scale";
        case ToolActionId::Building_Rotate: return "Building_Rotate";
        case ToolActionId::Building_Select: return "Building_Select";
        case ToolActionId::Building_Assign: return "Building_Assign";
        case ToolActionId::Building_Inspect: return "Building_Inspect";

        case ToolActionId::Future_FloorPlan: return "Future_FloorPlan";
        case ToolActionId::Future_Paths: return "Future_Paths";
        case ToolActionId::Future_Flow: return "Future_Flow";
        case ToolActionId::Future_Furnature: return "Future_Furnature";

        case ToolActionId::Count: return "Count";
    }

    return "Unknown";
}

} // namespace RC_UI::Tools
