#include "RogueCity/App/Tools/SplineManipulator.hpp"
#include "RogueCity/App/Editor/EditorManipulation.hpp"
#include <algorithm>
#include <cmath>

namespace RogueCity::App::Tools {

namespace {
    void SnapPoint(Core::Vec2& p, float snap_size) {
        if (snap_size <= 0.0f) return;
        p.x = std::round(p.x / snap_size) * snap_size;
        p.y = std::round(p.y / snap_size) * snap_size;
    }

    double DistanceToSegment(const Core::Vec2& p, const Core::Vec2& a, const Core::Vec2& b) {
        const Core::Vec2 ab = b - a;
        const Core::Vec2 ap = p - a;
        const double length_sq = ab.lengthSquared();
        if (length_sq < 1e-8) return p.distanceTo(a);
        
        const double t = std::clamp(ap.dot(ab) / length_sq, 0.0, 1.0);
        const Core::Vec2 projection = a + ab * t;
        return p.distanceTo(projection);
    }

    float ComputeFalloff(double distance, float radius) {
        if (radius <= 1e-4f) return 1.0f;
        const float d = std::clamp(static_cast<float>(distance) / radius, 0.0f, 1.0f);
        return 1.0f - (d * d * (3.0f - 2.0f * d)); // Smoothstep
    }
}

std::optional<size_t> SplineManipulator::PickVertex(
    const std::vector<Core::Vec2>& points,
    const Core::Vec2& pos,
    double radius) {
    double best_dist = radius;
    std::optional<size_t> best_idx;
    
    for (size_t i = 0; i < points.size(); ++i) {
        const double d = points[i].distanceTo(pos);
        if (d < best_dist) {
            best_dist = d;
            best_idx = i;
        }
    }
    return best_idx;
}

std::optional<size_t> SplineManipulator::PickSegment(
    const std::vector<Core::Vec2>& points,
    const Core::Vec2& pos,
    double radius,
    bool closed) {
    if (points.size() < 2) return std::nullopt;
    
    double best_dist = radius;
    std::optional<size_t> best_idx;
    
    const size_t count = closed ? points.size() : points.size() - 1;
    for (size_t i = 0; i < count; ++i) {
        const size_t next = (i + 1) % points.size();
        const double d = DistanceToSegment(pos, points[i], points[next]);
        if (d < best_dist) {
            best_dist = d;
            best_idx = i;
        }
    }
    return best_idx;
}

bool SplineManipulator::TryBeginDrag(
    const std::vector<Core::Vec2>& points,
    const Core::Vec2& mouse_world,
    const SplineManipulatorParams& params,
    SplineInteractionState& out_state) {
    
    // 1. Try picking tangent handles first (if implemented in future)
    // 2. Pick vertex
    auto idx = PickVertex(points, mouse_world, params.pick_radius);
    if (idx) {
        out_state.active = true;
        out_state.vertex_index = *idx;
        out_state.drag_start_world = mouse_world;
        out_state.is_tangent_handle = false;
        return true;
    }
    
    return false;
}

bool SplineManipulator::UpdateDrag(
    std::vector<Core::Vec2>& points,
    const Core::Vec2& mouse_world,
    const SplineManipulatorParams& params,
    SplineInteractionState& state) {
    
    if (!state.active || state.vertex_index >= points.size()) return false;
    
    Core::Vec2 new_pos = mouse_world;
    SnapPoint(new_pos, params.snap_size);
    
    const Core::Vec2 old_pos = points[state.vertex_index];
    if (new_pos.equals(old_pos)) return false;
    
    const Core::Vec2 delta = new_pos - old_pos;
    
    // Multi-point falloff (proportional editing)
    if (params.falloff_radius > 0.0f) {
        for (size_t i = 0; i < points.size(); ++i) {
            const double dist = points[i].distanceTo(old_pos);
            if (dist <= params.falloff_radius) {
                float weight = ComputeFalloff(dist, params.falloff_radius) * params.falloff_strength;
                points[i] += delta * weight;
            }
        }
    } else {
        points[state.vertex_index] = new_pos;
    }
    
    return true;
}

bool SplineManipulator::InsertVertex(
    std::vector<Core::Vec2>& points,
    const Core::Vec2& mouse_world,
    const SplineManipulatorParams& params) {
    
    auto segment_idx = PickSegment(points, mouse_world, params.pick_radius * 2.0, params.closed);
    if (segment_idx) {
        Core::Vec2 insert_pos = mouse_world;
        SnapPoint(insert_pos, params.snap_size);
        points.insert(points.begin() + static_cast<std::ptrdiff_t>(*segment_idx + 1), insert_pos);
        return true;
    }
    return false;
}

bool SplineManipulator::RemoveVertex(
    std::vector<Core::Vec2>& points,
    const Core::Vec2& mouse_world,
    const SplineManipulatorParams& params) {
    
    if (points.size() <= (params.closed ? 3 : 2)) return false;
    
    auto idx = PickVertex(points, mouse_world, params.pick_radius);
    if (idx) {
        points.erase(points.begin() + static_cast<std::ptrdiff_t>(*idx));
        return true;
    }
    return false;
}

void SplineManipulator::ApplySmoothing(
    std::vector<Core::Vec2>& points,
    const SplineManipulatorParams& params) {
    
    if (points.size() < 3) return;
    
    EditorManipulation::SplineOptions options{};
    options.closed = params.closed;
    options.samples_per_segment = params.samples_per_segment;
    options.tension = params.tension;
    
    auto smoothed = EditorManipulation::BuildCatmullRomSpline(points, options);
    if (smoothed.size() >= (params.closed ? 3 : 2)) {
        points = std::move(smoothed);
    }
}

} // namespace RogueCity::App::Tools
