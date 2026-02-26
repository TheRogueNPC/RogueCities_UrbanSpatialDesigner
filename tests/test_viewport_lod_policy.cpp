#include "ui/viewport/rc_viewport_lod_policy.h"

#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <cassert>

int main() {
    using RogueCity::Core::RoadType;
    using RogueCity::Core::Editor::GlobalState;
    using RogueCity::Core::Editor::ToolDomain;
    using RC_UI::Viewport::ResolveViewportLOD;
    using RC_UI::Viewport::ShouldRenderBuildingsForLOD;
    using RC_UI::Viewport::ShouldRenderLotsForLOD;
    using RC_UI::Viewport::ShouldRenderRoadForLOD;
    using RC_UI::Viewport::ViewportLOD;

    // Threshold behavior is strict `<` at 0.20 and 0.80.
    assert(ResolveViewportLOD(0.19f) == ViewportLOD::Coarse);
    assert(ResolveViewportLOD(0.20f) == ViewportLOD::Medium);
    assert(ResolveViewportLOD(0.79f) == ViewportLOD::Medium);
    assert(ResolveViewportLOD(0.80f) == ViewportLOD::Fine);

    // Road detail policy.
    assert(ShouldRenderRoadForLOD(RoadType::Arterial, ViewportLOD::Coarse));
    assert(!ShouldRenderRoadForLOD(RoadType::Street, ViewportLOD::Coarse));
    assert(!ShouldRenderRoadForLOD(RoadType::Lane, ViewportLOD::Medium));
    assert(ShouldRenderRoadForLOD(RoadType::Street, ViewportLOD::Medium));
    assert(ShouldRenderRoadForLOD(RoadType::Lane, ViewportLOD::Fine));

    GlobalState gs{};

    // Lots hidden at coarse unless lot domain is active.
    gs.tool_runtime.active_domain = ToolDomain::Road;
    assert(!ShouldRenderLotsForLOD(gs, ViewportLOD::Coarse));
    gs.tool_runtime.active_domain = ToolDomain::Lot;
    assert(ShouldRenderLotsForLOD(gs, ViewportLOD::Coarse));

    // Buildings hidden below fine unless a building-related domain is active.
    gs.tool_runtime.active_domain = ToolDomain::Road;
    assert(!ShouldRenderBuildingsForLOD(gs, ViewportLOD::Medium));
    gs.tool_runtime.active_domain = ToolDomain::Building;
    assert(ShouldRenderBuildingsForLOD(gs, ViewportLOD::Medium));
    gs.tool_runtime.active_domain = ToolDomain::FloorPlan;
    assert(ShouldRenderBuildingsForLOD(gs, ViewportLOD::Medium));
    gs.tool_runtime.active_domain = ToolDomain::Road;
    assert(ShouldRenderBuildingsForLOD(gs, ViewportLOD::Fine));

    return 0;
}

