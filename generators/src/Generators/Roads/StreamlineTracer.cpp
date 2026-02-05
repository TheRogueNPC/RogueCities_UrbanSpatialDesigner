#define _USE_MATH_DEFINES
#include "RogueCity/Generators/Roads/StreamlineTracer.hpp"
#include <cmath>
#include <algorithm>

namespace RogueCity::Generators {

    std::vector<Vec2> StreamlineTracer::traceMajor(
        const Vec2& seed,
        const TensorFieldGenerator& field,
        const Params& params
    ) {
        std::vector<Vec2> forward_trace;
        std::vector<Vec2> backward_trace;

        // Trace forward
        forward_trace = traceDirection(seed, field, true, params, true);

        // Trace backward if bidirectional
        if (params.bidirectional) {
            backward_trace = traceDirection(seed, field, true, params, false);
        }

        // Combine: backward (reversed) + seed + forward
        std::vector<Vec2> result;
        result.reserve(backward_trace.size() + forward_trace.size());

        // Add reversed backward trace
        for (auto it = backward_trace.rbegin(); it != backward_trace.rend(); ++it) {
            if (!result.empty() && it->equals(result.back(), 1e-3)) continue;
            result.push_back(*it);
        }

        // Add forward trace
        for (const auto& pt : forward_trace) {
            if (!result.empty() && pt.equals(result.back(), 1e-3)) continue;
            result.push_back(pt);
        }

        return result;
    }

    std::vector<Vec2> StreamlineTracer::traceMinor(
        const Vec2& seed,
        const TensorFieldGenerator& field,
        const Params& params
    ) {
        std::vector<Vec2> forward_trace;
        std::vector<Vec2> backward_trace;

        // Trace forward along minor eigenvector
        forward_trace = traceDirection(seed, field, false, params, true);

        // Trace backward if bidirectional
        if (params.bidirectional) {
            backward_trace = traceDirection(seed, field, false, params, false);
        }

        // Combine traces
        std::vector<Vec2> result;
        result.reserve(backward_trace.size() + forward_trace.size());

        for (auto it = backward_trace.rbegin(); it != backward_trace.rend(); ++it) {
            if (!result.empty() && it->equals(result.back(), 1e-3)) continue;
            result.push_back(*it);
        }

        for (const auto& pt : forward_trace) {
            if (!result.empty() && pt.equals(result.back(), 1e-3)) continue;
            result.push_back(pt);
        }

        return result;
    }

    fva::Container<Road> StreamlineTracer::traceNetwork(
        const std::vector<Vec2>& seeds,
        const TensorFieldGenerator& field,
        const Params& params
    ) {
        fva::Container<Road> roads;

        uint32_t road_id = 0;

        for (const auto& seed : seeds) {
            // Trace major road
            std::vector<Vec2> major_points = traceMajor(seed, field, params);
            if (major_points.size() >= 2) {
                Road major_road;
                major_road.points = std::move(major_points);
                major_road.type = RoadType::M_Major;
                major_road.id = road_id++;
                major_road.is_user_created = false;
                roads.add(std::move(major_road));
            }

            // Trace minor road (perpendicular)
            std::vector<Vec2> minor_points = traceMinor(seed, field, params);
            if (minor_points.size() >= 2) {
                Road minor_road;
                minor_road.points = std::move(minor_points);
                minor_road.type = RoadType::M_Minor;
                minor_road.id = road_id++;
                minor_road.is_user_created = false;
                roads.add(std::move(minor_road));
            }
        }

        return roads;
    }

    Vec2 StreamlineTracer::integrateRK4(
        const Vec2& pos,
        const TensorFieldGenerator& field,
        bool use_major,
        double dt
    ) {
        // Fourth-order Runge-Kutta integration (RK4)
        // k1 = f(t, y)
        Tensor2D t1 = field.sampleTensor(pos);
        Vec2 k1 = use_major ? t1.majorEigenvector() : t1.minorEigenvector();

        // k2 = f(t + dt/2, y + k1*dt/2)
        Tensor2D t2 = field.sampleTensor(pos + k1 * (dt * 0.5));
        Vec2 k2 = use_major ? t2.majorEigenvector() : t2.minorEigenvector();

        // k3 = f(t + dt/2, y + k2*dt/2)
        Tensor2D t3 = field.sampleTensor(pos + k2 * (dt * 0.5));
        Vec2 k3 = use_major ? t3.majorEigenvector() : t3.minorEigenvector();

        // k4 = f(t + dt, y + k3*dt)
        Tensor2D t4 = field.sampleTensor(pos + k3 * dt);
        Vec2 k4 = use_major ? t4.majorEigenvector() : t4.minorEigenvector();

        // y_next = y + (k1 + 2*k2 + 2*k3 + k4) * dt / 6
        Vec2 velocity = (k1 + k2 * 2.0 + k3 * 2.0 + k4) * (1.0 / 6.0);
        velocity.normalize();

        return pos + velocity * dt;
    }

    std::vector<Vec2> StreamlineTracer::traceDirection(
        const Vec2& seed,
        const TensorFieldGenerator& field,
        bool use_major,
        const Params& params,
        bool forward
    ) {
        std::vector<Vec2> polyline;
        polyline.push_back(seed);

        Vec2 current = seed;
        double total_length = 0.0;
        double step_size = forward ? params.step_size : -params.step_size;

        for (int iter = 0; iter < params.max_iterations; ++iter) {
            // RK4 integration step
            Vec2 next = integrateRK4(current, field, use_major, step_size);

            // Check boundary conditions
            double world_width = field.getWidth() * field.getCellSize();
            double world_height = field.getHeight() * field.getCellSize();

            if (next.x < 0.0 || next.x > world_width ||
                next.y < 0.0 || next.y > world_height) {
                break;  // Escaped domain
            }

            // Check step length
            double step_len = current.distanceTo(next);
            if (step_len < 1e-3) {
                break;  // Degenerate point (tensor magnitude too small)
            }

            total_length += step_len;
            if (total_length > params.max_length) {
                break;  // Maximum length reached
            }

            // Optional: Check separation constraint
            // (Disabled for MVP - enable when tracing dense networks)
            // if (tooCloseToExisting(next, polyline, params.min_separation)) {
            //     break;
            // }

            polyline.push_back(next);
            current = next;
        }

        return polyline;
    }

    bool StreamlineTracer::tooCloseToExisting(
        const Vec2& p,
        const std::vector<Vec2>& existing,
        double min_dist
    ) {
        // Check if point is within min_dist of any existing point
        double min_dist_sq = min_dist * min_dist;
        for (const auto& pt : existing) {
            if (p.distanceToSquared(pt) < min_dist_sq) {
                return true;
            }
        }
        return false;
    }

} // namespace RogueCity::Generators
