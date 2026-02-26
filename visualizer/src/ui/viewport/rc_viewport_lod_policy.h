// FILE: rc_viewport_lod_policy.h
// PURPOSE: Shared viewport LOD + domain override policy helpers.

#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <cstdint>

namespace RC_UI::Viewport {

enum class ViewportLOD : uint8_t {
    Coarse = 0,
    Medium,
    Fine
};

enum class RoadDetailClass : uint8_t {
    Arterial = 0,
    Collector,
    Local
};

[[nodiscard]] ViewportLOD ResolveViewportLOD(float zoom);
[[nodiscard]] RoadDetailClass ClassifyRoadDetail(RogueCity::Core::RoadType type);
[[nodiscard]] bool ShouldRenderRoadForLOD(RogueCity::Core::RoadType type, ViewportLOD lod);

[[nodiscard]] bool IsLotDomainActive(RogueCity::Core::Editor::ToolDomain domain);
[[nodiscard]] bool IsBuildingDomainActive(RogueCity::Core::Editor::ToolDomain domain);
[[nodiscard]] bool IsRoadDomainActive(RogueCity::Core::Editor::ToolDomain domain);

[[nodiscard]] bool ShouldRenderLotsForLOD(
    const RogueCity::Core::Editor::GlobalState& gs,
    ViewportLOD lod);

[[nodiscard]] bool ShouldRenderBuildingsForLOD(
    const RogueCity::Core::Editor::GlobalState& gs,
    ViewportLOD lod);

} // namespace RC_UI::Viewport

