#pragma once

#include "RogueCity/App/Tools/GeometryPolicy.hpp"
#include "RogueCity/Core/Editor/EditorUtils.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/tools/rc_tool_interaction_metrics.h"

#include <imgui.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace RC_UI::Viewport::Handlers {

using RogueCity::Core::Editor::FindBuildingMutable;
using RogueCity::Core::Editor::FindDistrictMutable;
using RogueCity::Core::Editor::FindLotMutable;
using RogueCity::Core::Editor::FindRoadMutable;
using RogueCity::Core::Editor::FindWaterMutable;

struct ViewportInteractionContext {
  RogueCity::Core::Editor::GlobalState &gs;
  RogueCity::Core::Editor::EditorState editor_state;
  const RogueCity::Core::Vec2 &world_pos;
  const RC_UI::Tools::ToolInteractionMetrics &interaction_metrics;
  const RogueCity::App::Tools::GeometryPolicy &geometry_policy;
  ImGuiIO &io;
};

[[nodiscard]] uint64_t
SelectionKey(const RogueCity::Core::Editor::SelectionItem &item);

[[nodiscard]] double DistanceToSegment(const RogueCity::Core::Vec2 &p,
                                       const RogueCity::Core::Vec2 &a,
                                       const RogueCity::Core::Vec2 &b);

[[nodiscard]] bool
PointInPolygon(const RogueCity::Core::Vec2 &point,
               const std::vector<RogueCity::Core::Vec2> &polygon);

[[nodiscard]] RogueCity::Core::Vec2
PolygonCentroid(const std::vector<RogueCity::Core::Vec2> &points);

[[nodiscard]] bool IsSelectableKind(RogueCity::Core::Editor::VpEntityKind kind);

[[nodiscard]] bool
ResolveSelectionAnchor(const RogueCity::Core::Editor::GlobalState &gs,
                       RogueCity::Core::Editor::VpEntityKind kind, uint32_t id,
                       RogueCity::Core::Vec2 &out_anchor);

[[nodiscard]] std::optional<RogueCity::Core::Editor::SelectionItem>
PickFromViewportIndex(
    const RogueCity::Core::Editor::GlobalState &gs,
    const RogueCity::Core::Vec2 &world_pos,
    const RC_UI::Tools::ToolInteractionMetrics &interaction_metrics,
    double radius_scale = 1.0, bool prefer_manhattan = true);

[[nodiscard]] std::vector<RogueCity::Core::Editor::SelectionItem>
QueryRegionFromViewportIndex(
    const RogueCity::Core::Editor::GlobalState &gs,
    const std::function<bool(const RogueCity::Core::Vec2 &)> &include_point,
    bool include_hidden = false);

[[nodiscard]] RogueCity::Core::Vec2
ComputeSelectionPivot(const RogueCity::Core::Editor::GlobalState &gs);

void MarkDirtyForSelectionKind(RogueCity::Core::Editor::GlobalState &gs,
                               RogueCity::Core::Editor::VpEntityKind kind);

void PromoteEntityToUserLocked(RogueCity::Core::Editor::GlobalState &gs,
                               RogueCity::Core::Editor::VpEntityKind kind,
                               uint32_t id);

[[nodiscard]] uint32_t
NextRoadId(const RogueCity::Core::Editor::GlobalState &gs);
[[nodiscard]] uint32_t
NextDistrictId(const RogueCity::Core::Editor::GlobalState &gs);
[[nodiscard]] uint32_t
NextLotId(const RogueCity::Core::Editor::GlobalState &gs);
[[nodiscard]] uint32_t
NextBuildingId(const RogueCity::Core::Editor::GlobalState &gs);
[[nodiscard]] uint32_t
NextWaterId(const RogueCity::Core::Editor::GlobalState &gs);

[[nodiscard]] std::vector<RogueCity::Core::Vec2>
MakeSquareBoundary(const RogueCity::Core::Vec2 &center, double half_extent);

void SnapToGrid(RogueCity::Core::Vec2 &point, bool enabled, float snap_size);
void ApplyAxisAlign(RogueCity::Core::Vec2 &point,
                    const RogueCity::Core::Vec2 &reference);
[[nodiscard]] bool SimplifyPolyline(std::vector<RogueCity::Core::Vec2> &points,
                                    bool closed);

void CycleBuildingType(RogueCity::Core::BuildingType &type);

void SetPrimarySelection(RogueCity::Core::Editor::GlobalState &gs,
                         RogueCity::Core::Editor::VpEntityKind kind,
                         uint32_t id);

void ApplyToolPreferredGizmoOperation(
    RogueCity::Core::Editor::GlobalState &gs,
    RogueCity::Core::Editor::EditorState editor_state);

} // namespace RC_UI::Viewport::Handlers
