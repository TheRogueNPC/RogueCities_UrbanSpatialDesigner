#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <span>
#include <vector>

namespace RogueCity::App::EditorManipulation {

struct SplineOptions {
    bool closed{ false };
    int samples_per_segment{ 8 };
    float tension{ 0.5f };
};

std::vector<Core::Vec2> BuildCatmullRomSpline(
    const std::vector<Core::Vec2>& control_points,
    const SplineOptions& options);

bool ApplyTranslate(
    Core::Editor::GlobalState& gs,
    std::span<const Core::Editor::SelectionItem> selection,
    const Core::Vec2& delta);

bool ApplyRotate(
    Core::Editor::GlobalState& gs,
    std::span<const Core::Editor::SelectionItem> selection,
    const Core::Vec2& pivot,
    double radians);

bool ApplyScale(
    Core::Editor::GlobalState& gs,
    std::span<const Core::Editor::SelectionItem> selection,
    const Core::Vec2& pivot,
    double factor);

bool MoveRoadVertex(Core::Road& road, size_t vertex_index, const Core::Vec2& new_position);
bool InsertDistrictVertex(Core::District& district, size_t edge_index, const Core::Vec2& new_position);
bool RemoveDistrictVertex(Core::District& district, size_t vertex_index);

} // namespace RogueCity::App::EditorManipulation

