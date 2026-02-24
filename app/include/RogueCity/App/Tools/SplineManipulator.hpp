#pragma once

#include "RogueCity/Core/Math/Vec2.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include <vector>
#include <optional>

namespace RogueCity::App::Tools {

    /// Interaction state for dragging a vertex or tangent handle.
    struct SplineInteractionState {
        bool active{ false };
        uint32_t entity_id{ 0 };
        size_t vertex_index{ 0 };
        bool is_tangent_handle{ false };
        bool is_outgoing_tangent{ false };
        Core::Vec2 drag_start_world{};
    };

    /// Parameters for spline manipulation.
    struct SplineManipulatorParams {
        bool closed{ false };
        float snap_size{ 0.0f };
        double pick_radius{ 5.0 };
        float tension{ 0.5f };
        int samples_per_segment{ 4 };
        
        // Falloff effects (used for river "Erode/Flow" modes)
        float falloff_radius{ 0.0f };
        float falloff_strength{ 1.0f };
    };

    /// Specialized manipulator for interactive spline editing (Roads and Rivers).
    /// Centralizes logic for vertex dragging, tangent manipulation, and smoothing.
    class SplineManipulator {
    public:
        /// Tries to start a drag interaction by picking a vertex or handle.
        static bool TryBeginDrag(
            const std::vector<Core::Vec2>& points,
            const Core::Vec2& mouse_world,
            const SplineManipulatorParams& params,
            SplineInteractionState& out_state);

        /// Updates the spline points based on active drag state.
        /// Returns true if points were modified.
        static bool UpdateDrag(
            std::vector<Core::Vec2>& points,
            const Core::Vec2& mouse_world,
            const SplineManipulatorParams& params,
            SplineInteractionState& state);

        /// Inserts a new vertex into the closest segment.
        static bool InsertVertex(
            std::vector<Core::Vec2>& points,
            const Core::Vec2& mouse_world,
            const SplineManipulatorParams& params);

        /// Removes the vertex closest to the mouse.
        static bool RemoveVertex(
            std::vector<Core::Vec2>& points,
            const Core::Vec2& mouse_world,
            const SplineManipulatorParams& params);

        /// Applies a smoothing pass (Catmull-Rom) to the points.
        static void ApplySmoothing(
            std::vector<Core::Vec2>& points,
            const SplineManipulatorParams& params);

    private:
        /// Helper to find the closest vertex index within a radius.
        static std::optional<size_t> PickVertex(
            const std::vector<Core::Vec2>& points,
            const Core::Vec2& pos,
            double radius);

        /// Helper to find the closest segment index within a radius.
        static std::optional<size_t> PickSegment(
            const std::vector<Core::Vec2>& points,
            const Core::Vec2& pos,
            double radius,
            bool closed);
    };

} // namespace RogueCity::App::Tools
