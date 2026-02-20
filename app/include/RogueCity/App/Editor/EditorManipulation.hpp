#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <span>
#include <vector>

namespace RogueCity::App::EditorManipulation {

struct SplineOptions { // Options for building a Catmull-Rom spline.
    bool closed{ false };
    int samples_per_segment{ 8 };
    float tension{ 0.5f };
};

std::vector<Core::Vec2> BuildCatmullRomSpline( // Builds a Catmull-Rom spline from the given control points and options.
    const std::vector<Core::Vec2>& control_points,
    const SplineOptions& options);

bool ApplyTranslate( // Applies a translation to the selected items in the editor.
    Core::Editor::GlobalState& gs, 
    std::span<const Core::Editor::SelectionItem> selection,
    const Core::Vec2& delta);

bool ApplyRotate( // Applies a rotation to the selected items in the editor around a pivot point.
    Core::Editor::GlobalState& gs,
    std::span<const Core::Editor::SelectionItem> selection,
    const Core::Vec2& pivot,
    double radians);

bool ApplyScale( // Applies a scaling transformation to the selected items in the editor around a pivot point.
    Core::Editor::GlobalState& gs,
    std::span<const Core::Editor::SelectionItem> selection,
    const Core::Vec2& pivot,
    double factor);

// Returns true if the manipulation was successful, false if it failed (e.g. due to invalid selection or parameters).
bool MoveRoadVertex(Core::Road& road, size_t vertex_index, const Core::Vec2& new_position); 

// Returns true if the manipulation was successful, false if it failed (e.g. due to invalid selection or parameters).per vertex point in road
bool InsertDistrictVertex(Core::District& district, size_t edge_index, const Core::Vec2& new_position);

// Returns true if the manipulation was successful, false if it failed (e.g. due to invalid selection or parameters).per District polygon
bool RemoveDistrictVertex(Core::District& district, size_t vertex_index);

} // namespace RogueCity::App::EditorManipulation

