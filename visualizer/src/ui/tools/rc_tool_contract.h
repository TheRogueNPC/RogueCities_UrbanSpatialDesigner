#pragma once

#include "ui/rc_ui_root.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace RC_UI::Tools {

enum class ToolActionGroup : uint8_t {
    Primary = 0,
    Spline,
    FutureStub
};

enum class ToolExecutionPolicy : uint8_t {
    ActivateOnly = 0,
    ActivateAndExecute,
    Disabled
};

enum class ToolAvailability : uint8_t {
    Available = 0,
    Disabled
};

enum class ToolActionId : uint16_t {
    Axiom_Organic = 0,
    Axiom_Grid,
    Axiom_Radial,
    Axiom_Hexagonal,
    Axiom_Stem,
    Axiom_LooseGrid,
    Axiom_Suburban,
    Axiom_Superblock,
    Axiom_Linear,
    Axiom_GridCorrective,

    Water_Flow,
    Water_Contour,
    Water_Erode,
    Water_Select,
    Water_Mask,
    Water_Inspect,

    WaterSpline_Selection,
    WaterSpline_DirectSelect,
    WaterSpline_Pen,
    WaterSpline_ConvertAnchor,
    WaterSpline_AddRemoveAnchor,
    WaterSpline_HandleTangents,
    WaterSpline_SnapAlign,
    WaterSpline_JoinSplit,
    WaterSpline_Simplify,

    Road_Spline,
    Road_Grid,
    Road_Bridge,
    Road_Select,
    Road_Disconnect,
    Road_Stub,
    Road_Curve,
    Road_Strengthen,
    Road_Inspect,

    RoadSpline_Selection,
    RoadSpline_DirectSelect,
    RoadSpline_Pen,
    RoadSpline_ConvertAnchor,
    RoadSpline_AddRemoveAnchor,
    RoadSpline_HandleTangents,
    RoadSpline_SnapAlign,
    RoadSpline_JoinSplit,
    RoadSpline_Simplify,

    District_Zone,
    District_Paint,
    District_Split,
    District_Select,
    District_Merge,
    District_Inspect,

    Lot_Plot,
    Lot_Slice,
    Lot_Align,
    Lot_Select,
    Lot_Merge,
    Lot_Inspect,

    Building_Place,
    Building_Scale,
    Building_Rotate,
    Building_Select,
    Building_Assign,
    Building_Inspect,

    Future_FloorPlan,
    Future_Paths,
    Future_Flow,
    Future_Furnature,

    Count
};

struct ToolActionSpec {
    ToolActionId id{};
    RogueCity::Core::Editor::ToolDomain domain{};
    ToolLibrary library{};
    ToolActionGroup group{};
    const char* label = "";
    const char* tooltip = "";
    ToolExecutionPolicy policy{};
    ToolAvailability availability{};
    const char* disabled_reason = "";
};

[[nodiscard]] std::span<const ToolActionSpec> GetToolActionsForLibrary(ToolLibrary library);
[[nodiscard]] std::span<const ToolActionSpec> GetToolActionCatalog();
[[nodiscard]] const ToolActionSpec* FindToolAction(ToolActionId id);
[[nodiscard]] bool IsToolActionEnabled(const ToolActionSpec& action);
[[nodiscard]] const char* ToolDomainName(RogueCity::Core::Editor::ToolDomain domain);
[[nodiscard]] const char* ToolActionName(ToolActionId id);

} // namespace RC_UI::Tools
