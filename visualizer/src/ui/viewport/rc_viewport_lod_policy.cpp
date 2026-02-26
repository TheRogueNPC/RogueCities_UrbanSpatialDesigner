// FILE: rc_viewport_lod_policy.cpp
// PURPOSE: Shared viewport LOD + domain override policy helpers.

#include "ui/viewport/rc_viewport_lod_policy.h"

namespace RC_UI::Viewport {

ViewportLOD ResolveViewportLOD(float zoom) {
    if (zoom < 0.20f) {
        return ViewportLOD::Coarse;
    }
    if (zoom < 0.80f) {
        return ViewportLOD::Medium;
    }
    return ViewportLOD::Fine;
}

RoadDetailClass ClassifyRoadDetail(RogueCity::Core::RoadType type) {
    using RogueCity::Core::RoadType;
    switch (type) {
    case RoadType::Highway:
    case RoadType::Arterial:
    case RoadType::Avenue:
    case RoadType::Boulevard:
    case RoadType::M_Major:
        return RoadDetailClass::Arterial;
    case RoadType::Street:
    case RoadType::M_Minor:
        return RoadDetailClass::Collector;
    case RoadType::Lane:
    case RoadType::Alleyway:
    case RoadType::CulDeSac:
    case RoadType::Drive:
    case RoadType::Driveway:
    default:
        return RoadDetailClass::Local;
    }
}

bool ShouldRenderRoadForLOD(RogueCity::Core::RoadType type, ViewportLOD lod) {
    const RoadDetailClass detail = ClassifyRoadDetail(type);
    if (lod == ViewportLOD::Fine) {
        return true;
    }
    if (lod == ViewportLOD::Medium) {
        return detail != RoadDetailClass::Local;
    }
    return detail == RoadDetailClass::Arterial;
}

bool IsLotDomainActive(RogueCity::Core::Editor::ToolDomain domain) {
    return domain == RogueCity::Core::Editor::ToolDomain::Lot;
}

bool IsBuildingDomainActive(RogueCity::Core::Editor::ToolDomain domain) {
    return domain == RogueCity::Core::Editor::ToolDomain::Building ||
        domain == RogueCity::Core::Editor::ToolDomain::FloorPlan ||
        domain == RogueCity::Core::Editor::ToolDomain::Furnature;
}

bool IsRoadDomainActive(RogueCity::Core::Editor::ToolDomain domain) {
    return domain == RogueCity::Core::Editor::ToolDomain::Road ||
        domain == RogueCity::Core::Editor::ToolDomain::Paths;
}

bool ShouldRenderLotsForLOD(
    const RogueCity::Core::Editor::GlobalState& gs,
    ViewportLOD lod) {
    return lod != ViewportLOD::Coarse || IsLotDomainActive(gs.tool_runtime.active_domain);
}

bool ShouldRenderBuildingsForLOD(
    const RogueCity::Core::Editor::GlobalState& gs,
    ViewportLOD lod) {
    if (lod == ViewportLOD::Fine) {
        return true;
    }
    return IsBuildingDomainActive(gs.tool_runtime.active_domain);
}

} // namespace RC_UI::Viewport

