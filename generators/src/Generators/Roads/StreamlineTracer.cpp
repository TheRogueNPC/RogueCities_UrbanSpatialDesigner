#define _USE_MATH_DEFINES
#include "RogueCity/Generators/Roads/StreamlineTracer.hpp"
#include "RogueCity/Core/Data/MaterialEncoding.hpp"
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <unordered_map>

namespace RogueCity::Generators {

    namespace {
        constexpr double kPi = 3.14159265358979323846;

        [[nodiscard]] uint8_t SampleTextureMaterial(const Core::Data::TextureSpace& texture_space, const Vec2& world) {
            const Vec2 uv = texture_space.coordinateSystem().worldToUV(world);
            const float sample = texture_space.materialLayer().sampleBilinearU8(uv);
            return static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(sample)), 0, 255));
        }

        class SpatialPointGrid {
        public:
            explicit SpatialPointGrid(double cell_size)
                : cell_size_(std::max(1e-3, cell_size)) {}

            [[nodiscard]] bool TooClose(const Vec2& point, double min_distance) const {
                const GridCell cell = CellFor(point);
                const double min_distance_sq = min_distance * min_distance;

                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const GridCell neighbor{ cell.x + dx, cell.y + dy };
                        const auto it = bins_.find(Key(neighbor));
                        if (it == bins_.end()) {
                            continue;
                        }

                        const auto& candidates = it->second;
                        for (const auto& existing : candidates) {
                            if (point.distanceToSquared(existing) < min_distance_sq) {
                                return true;
                            }
                        }
                    }
                }

                return false;
            }

            void Insert(const Vec2& point) {
                bins_[Key(CellFor(point))].push_back(point);
            }

            void InsertPolyline(const std::vector<Vec2>& polyline) {
                for (const auto& point : polyline) {
                    Insert(point);
                }
            }

        private:
            struct GridCell {
                int x{ 0 };
                int y{ 0 };
            };

            [[nodiscard]] GridCell CellFor(const Vec2& point) const {
                return GridCell{
                    static_cast<int>(std::floor(point.x / cell_size_)),
                    static_cast<int>(std::floor(point.y / cell_size_))
                };
            }

            [[nodiscard]] static int64_t Key(const GridCell& cell) {
                const uint64_t ux = static_cast<uint64_t>(static_cast<uint32_t>(cell.x));
                const uint64_t uy = static_cast<uint64_t>(static_cast<uint32_t>(cell.y));
                return static_cast<int64_t>((ux << 32) | uy);
            }

            double cell_size_{ 1.0 };
            std::unordered_map<int64_t, std::vector<Vec2>> bins_{};
        };
    } // namespace

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
        const double separation_cell =
            params.separation_cell_size > 0.0 ? params.separation_cell_size : params.min_separation;
        SpatialPointGrid network_index(separation_cell);

        auto polyline_conflicts = [&](const std::vector<Vec2>& points) {
            if (!params.enforce_network_separation || params.min_separation <= 0.0) {
                return false;
            }
            for (const auto& point : points) {
                if (network_index.TooClose(point, params.min_separation)) {
                    return true;
                }
            }
            return false;
        };

        auto register_polyline = [&](const std::vector<Vec2>& points) {
            if (params.enforce_network_separation && params.min_separation > 0.0) {
                network_index.InsertPolyline(points);
            }
        };

        for (const auto& seed : seeds) {
            // Trace major road
            std::vector<Vec2> major_points = traceMajor(seed, field, params);
            if (major_points.size() >= 2 && !polyline_conflicts(major_points)) {
                Road major_road;
                major_road.points = std::move(major_points);
                major_road.type = RoadType::M_Major;
                major_road.id = road_id++;
                major_road.is_user_created = false;
                register_polyline(major_road.points);
                roads.add(std::move(major_road));
            }

            // Trace minor road (perpendicular)
            std::vector<Vec2> minor_points = traceMinor(seed, field, params);
            if (minor_points.size() >= 2 && !polyline_conflicts(minor_points)) {
                Road minor_road;
                minor_road.points = std::move(minor_points);
                minor_road.type = RoadType::M_Minor;
                minor_road.id = road_id++;
                minor_road.is_user_created = false;
                register_polyline(minor_road.points);
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
        auto failsConstraintCheck = [&](const Vec2& world) {
            if (params.texture_space != nullptr) {
                const float slope = params.texture_space->distanceLayer().sampleBilinear(
                    params.texture_space->coordinateSystem().worldToUV(world));
                if (slope > params.max_slope_degrees) {
                    return true;
                }

                const uint8_t material = SampleTextureMaterial(*params.texture_space, world);
                if (params.stop_at_no_build && Core::Data::DecodeMaterialNoBuild(material)) {
                    return true;
                }
                if (Core::Data::DecodeMaterialFloodMask(material) > params.max_flood_level) {
                    return true;
                }
            }

            if (params.constraints != nullptr) {
                if (params.stop_at_no_build && params.constraints->sampleNoBuild(world)) {
                    return true;
                }
                if (params.constraints->sampleSlopeDegrees(world) > params.max_slope_degrees) {
                    return true;
                }
                if (params.constraints->sampleFloodMask(world) > params.max_flood_level) {
                    return true;
                }
                if (params.constraints->sampleSoilStrength(world) < params.min_soil_strength) {
                    return true;
                }
            }

            return false;
        };

        if (failsConstraintCheck(seed)) {
            return {};
        }

        std::vector<Vec2> polyline;
        polyline.push_back(seed);

        Vec2 current = seed;
        double total_length = 0.0;
        const double base_step = std::max(1e-3, std::abs(params.step_size));
        const double min_step = std::max(1e-3, params.min_step_size);
        const double max_step = std::max(min_step, params.max_step_size);
        double step_size = std::clamp(base_step, min_step, max_step);
        Vec2 previous_direction{};
        bool has_previous_direction = false;

        for (int iter = 0; iter < params.max_iterations; ++iter) {
            // RK4 integration step
            const double signed_step = forward ? step_size : -step_size;
            Vec2 next = integrateRK4(current, field, use_major, signed_step);

            // Check boundary conditions
            if (params.texture_space != nullptr) {
                if (!params.texture_space->coordinateSystem().isInBounds(next)) {
                    break;  // Escaped domain
                }
            } else {
                const double world_width = field.getWidth() * field.getCellSize();
                const double world_height = field.getHeight() * field.getCellSize();
                if (next.x < 0.0 || next.x > world_width ||
                    next.y < 0.0 || next.y > world_height) {
                    break;  // Escaped domain
                }
            }

            if (failsConstraintCheck(next)) {
                break;
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
            if (params.adaptive_step_size) {
                Vec2 direction = next - current;
                direction.normalize();
                if (has_previous_direction) {
                    const double cos_theta = std::clamp(previous_direction.dot(direction), -1.0, 1.0);
                    const double curvature = std::acos(cos_theta) / kPi; // [0, 1]
                    const double scale = 1.0 - std::clamp(curvature * params.curvature_gain, 0.0, 0.85);
                    step_size = std::clamp(base_step * scale, min_step, max_step);
                } else {
                    step_size = std::clamp(base_step, min_step, max_step);
                    has_previous_direction = true;
                }
                previous_direction = direction;
            }
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
