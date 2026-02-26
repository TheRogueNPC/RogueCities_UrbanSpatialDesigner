/**
 * @file EditorManipulation.hpp
 * @brief Provides functions for manipulating city elements in the editor, including translation, rotation, scaling, and spline construction.
 *
 * Contains utilities for manipulating selected items in the city editor, such as roads and districts.
 * Includes Catmull-Rom spline generation and vertex manipulation for roads and districts.
 *
 * @namespace RogueCity::App::EditorManipulation
 * 
 * @struct SplineOptions
 *   @brief Options for building a Catmull-Rom spline.
 *   @var closed Indicates whether the spline is closed (looped).
 *   @var samples_per_segment Number of samples per segment for spline interpolation.
 *   @var tension Tension parameter for Catmull-Rom spline.
 *
 * @function BuildCatmullRomSpline
 *   @brief Builds a Catmull-Rom spline from the given control points and options.
 *   @param control_points Control points for the spline.
 *   @param options Spline construction options.
 *   @return Interpolated points along the spline.
 *
 * @function ApplyTranslate
 *   @brief Applies a translation to the selected items in the editor.
 *   @param gs Reference to the editor global state.
 *   @param selection Items to be translated.
 *   @param delta Translation vector.
 *   @return True if translation was successful, false otherwise.
 *
 * @function ApplyRotate
 *   @brief Applies a rotation to the selected items in the editor around a pivot point.
 *   @param gs Reference to the editor global state.
 *   @param selection Items to be rotated.
 *   @param pivot Pivot point for rotation.
 *   @param radians Angle of rotation in radians.
 *   @return True if rotation was successful, false otherwise.
 *
 * @function ApplyScale
 *   @brief Applies a scaling transformation to the selected items in the editor around a pivot point.
 *   @param gs Reference to the editor global state.
 *   @param selection Items to be scaled.
 *   @param pivot Pivot point for scaling.
 *   @param factor Scaling factor.
 *   @return True if scaling was successful, false otherwise.
 *
 * @function MoveRoadVertex
 *   @brief Moves a vertex in a road to a new position.
 *   @param road Road object to modify.
 *   @param vertex_index Index of the vertex to move.
 *   @param new_position New position for the vertex.
 *   @return True if the vertex was moved successfully, false otherwise.
 *
 * @function InsertDistrictVertex
 *   @brief Inserts a new vertex into a district polygon at the specified edge.
 *   @param district District object to modify.
 *   @param edge_index Index of the edge where the vertex will be inserted.
 *   @param new_position Position of the new vertex.
 *   @return True if the vertex was inserted successfully, false otherwise.
 *
 * @function RemoveDistrictVertex
 *   @brief Removes a vertex from a district polygon.
 *   @param district District object to modify.
 *   @param vertex_index Index of the vertex to remove.
 *   @return True if the vertex was removed successfully, false otherwise.
 */
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

