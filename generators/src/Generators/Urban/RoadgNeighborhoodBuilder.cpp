#include "RogueCity/Generators/Urban/RoadgNeighborhoodBuilder.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace RogueCity::Generators::Urban {

namespace {

struct Bounds2D {
  double min_x{0.0};
  double max_x{0.0};
  double min_y{0.0};
  double max_y{0.0};
};

[[nodiscard]] bool IsFinite(const Core::Vec2 &p) {
  return std::isfinite(p.x) && std::isfinite(p.y);
}

[[nodiscard]] bool ComputeBounds(const std::vector<Core::Vec2> &points,
                                 Bounds2D &out) {
  if (points.empty()) {
    return false;
  }

  out.min_x = points.front().x;
  out.max_x = points.front().x;
  out.min_y = points.front().y;
  out.max_y = points.front().y;
  for (const auto &p : points) {
    if (!IsFinite(p)) {
      continue;
    }
    out.min_x = std::min(out.min_x, p.x);
    out.max_x = std::max(out.max_x, p.x);
    out.min_y = std::min(out.min_y, p.y);
    out.max_y = std::max(out.max_y, p.y);
  }
  return out.max_x > out.min_x && out.max_y > out.min_y;
}

[[nodiscard]] std::vector<double> BuildAxisSamples(double min_v, double max_v,
                                                   double spacing) {
  std::vector<double> values;
  if (!(max_v > min_v)) {
    return values;
  }

  const double span = max_v - min_v;
  const double step = std::clamp(spacing, 20.0, 400.0);
  if (span <= (step * 1.5)) {
    values.push_back((min_v + max_v) * 0.5);
    return values;
  }

  for (double v = min_v + step; v < max_v - step + 1e-6; v += step) {
    values.push_back(v);
  }
  if (values.empty()) {
    values.push_back((min_v + max_v) * 0.5);
  }
  return values;
}

[[nodiscard]] Core::Road MakeRoad(uint32_t id, Core::RoadType type,
                                  const Core::Vec2 &a, const Core::Vec2 &b) {
  Core::Road road{};
  road.id = id;
  road.type = type;
  road.generation_tag = Core::GenerationTag::Generated;
  road.generation_locked = false;
  road.points.push_back(a);
  road.points.push_back(b);
  return road;
}

[[nodiscard]] Core::Vec2 NearestPoint(const Core::Vec2 &query,
                                      const std::vector<Core::Vec2> &points) {
  Core::Vec2 best = query;
  double best_dist_sq = std::numeric_limits<double>::infinity();
  for (const auto &p : points) {
    const double dx = query.x - p.x;
    const double dy = query.y - p.y;
    const double dist_sq = dx * dx + dy * dy;
    if (dist_sq < best_dist_sq) {
      best_dist_sq = dist_sq;
      best = p;
    }
  }
  return best;
}

} // namespace

std::vector<RoadgRegion> RoadgNeighborhoodBuilder::InferRegionsFromAxiomSkeleton(
    const fva::Container<Core::Road> &outer_roads) {
  std::vector<RoadgRegion> regions{};

  std::vector<Core::Vec2> points;
  points.reserve(outer_roads.size() * 2u);
  for (const auto &road : outer_roads) {
    for (const auto &point : road.points) {
      points.push_back(point);
    }
  }

  Bounds2D bounds{};
  if (!ComputeBounds(points, bounds)) {
    return regions;
  }

  RoadgRegion inferred{};
  inferred.id = 1u;
  inferred.boundary.push_back(Core::Vec2(bounds.min_x, bounds.min_y));
  inferred.boundary.push_back(Core::Vec2(bounds.max_x, bounds.min_y));
  inferred.boundary.push_back(Core::Vec2(bounds.max_x, bounds.max_y));
  inferred.boundary.push_back(Core::Vec2(bounds.min_x, bounds.max_y));
  regions.push_back(std::move(inferred));

  return regions;
}

RoadgNeighborhoodBuilder::Output
RoadgNeighborhoodBuilder::Build(const Input &input, const Config &config) {
  Output output{};

  const std::vector<RoadgRegion> regions =
      input.regions.empty() ? InferRegionsFromAxiomSkeleton(input.outer_roads)
                            : input.regions;

  uint32_t next_road_id = 1u;

  for (const auto &region : regions) {
    Bounds2D bounds{};
    if (!ComputeBounds(region.boundary, bounds)) {
      continue;
    }

    RegionMetadata metadata{};
    metadata.region_id = region.id;

    const double inset = std::max(0.0, config.boundary_inset);
    const double min_x = std::min(bounds.max_x, bounds.min_x + inset);
    const double max_x = std::max(bounds.min_x, bounds.max_x - inset);
    const double min_y = std::min(bounds.max_y, bounds.min_y + inset);
    const double max_y = std::max(bounds.min_y, bounds.max_y - inset);

    const auto xs = BuildAxisSamples(min_x, max_x, config.grid_spacing);
    const auto ys = BuildAxisSamples(min_y, max_y, config.grid_spacing);

    std::vector<Core::Vec2> inner_nodes;
    inner_nodes.reserve(xs.size() * ys.size() + xs.size() * 2 + ys.size() * 2);

    for (const double x : xs) {
      const Core::Vec2 a(x, min_y);
      const Core::Vec2 b(x, max_y);
      if (a.distanceTo(b) < 1.0) {
        continue;
      }
      output.inner_roads.add(
          MakeRoad(next_road_id++, Core::RoadType::Street, a, b));
      metadata.inner_road_count += 1u;
      inner_nodes.push_back(a);
      inner_nodes.push_back(b);
    }

    for (const double y : ys) {
      const Core::Vec2 a(min_x, y);
      const Core::Vec2 b(max_x, y);
      if (a.distanceTo(b) < 1.0) {
        continue;
      }
      output.inner_roads.add(
          MakeRoad(next_road_id++, Core::RoadType::Street, a, b));
      metadata.inner_road_count += 1u;
      inner_nodes.push_back(a);
      inner_nodes.push_back(b);
    }

    for (const double x : xs) {
      for (const double y : ys) {
        inner_nodes.emplace_back(x, y);
      }
    }

    if (inner_nodes.empty()) {
      inner_nodes.push_back(
          Core::Vec2((bounds.min_x + bounds.max_x) * 0.5,
                     (bounds.min_y + bounds.max_y) * 0.5));
    }

    const Core::Vec2 center((bounds.min_x + bounds.max_x) * 0.5,
                            (bounds.min_y + bounds.max_y) * 0.5);
    const std::array<Core::Vec2, 4> side_points{{
        Core::Vec2(center.x, bounds.max_y),
        Core::Vec2(bounds.max_x, center.y),
        Core::Vec2(center.x, bounds.min_y),
        Core::Vec2(bounds.min_x, center.y),
    }};

    for (size_t side_index = 0; side_index < side_points.size(); ++side_index) {
      const Core::Vec2 boundary_point = side_points[side_index];
      const Core::Vec2 target = NearestPoint(boundary_point, inner_nodes);
      if (boundary_point.distanceTo(target) < 1.0) {
        continue;
      }

      output.stitch_edges.add(MakeRoad(next_road_id++, Core::RoadType::Avenue,
                                       boundary_point, target));
      metadata.stitch_count += 1u;
      metadata.stitches_per_side[side_index] += 1u;
    }

    if (config.enforce_stitch_per_side) {
      for (const uint32_t count : metadata.stitches_per_side) {
        if (count == 0u) {
          metadata.connected = false;
          output.metadata.push_back(metadata);
          goto append_next_region;
        }
      }
    }

    metadata.connected =
        metadata.inner_road_count > 0u && metadata.stitch_count > 0u;
    output.metadata.push_back(metadata);

  append_next_region:
    continue;
  }

  return output;
}

} // namespace RogueCity::Generators::Urban
