#pragma once

#include "RogueCity/Core/Math/Vec2.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace RogueCity::App::Tools {

struct SplineInteractionState {
    bool active{ false };
    uint32_t entity_id{ 0 };
    size_t vertex_index{ 0 };
    bool is_tangent_handle{ false };
    bool is_outgoing_tangent{ false };
    Core::Vec2 drag_start_world{};
};

struct SplineManipulatorParams {
    bool closed{ false };
    float snap_size{ 0.0f };
    double pick_radius{ 8.0 };
    float tension{ 0.5f };
    int samples_per_segment{ 8 };
    float falloff_radius{ 0.0f };
    float falloff_strength{ 1.0f };
};

class SplineManipulator final {
public:
    static bool TryBeginDrag(const std::vector<Core::Vec2>& points,
                             const Core::Vec2& mouse_world,
                             const SplineManipulatorParams& params,
                             SplineInteractionState& out_state);

    static bool UpdateDrag(std::vector<Core::Vec2>& points,
                           const Core::Vec2& mouse_world,
                           const SplineManipulatorParams& params,
                           SplineInteractionState& state);

    static bool InsertVertex(std::vector<Core::Vec2>& points,
                             const Core::Vec2& mouse_world,
                             const SplineManipulatorParams& params);

    static bool RemoveVertex(std::vector<Core::Vec2>& points,
                             const Core::Vec2& mouse_world,
                             const SplineManipulatorParams& params);

    static void ApplySmoothing(std::vector<Core::Vec2>& points,
                               const SplineManipulatorParams& params);

private:
    static std::optional<size_t> PickVertex(const std::vector<Core::Vec2>& points,
                                            const Core::Vec2& pos,
                                            double radius);

    static std::optional<size_t> PickSegment(const std::vector<Core::Vec2>& points,
                                             const Core::Vec2& pos,
                                             double radius,
                                             bool closed);
};

} // namespace RogueCity::App::Tools
