#include "RogueCity/Core/Geometry/PolygonOps.hpp"
#include <clipper2/clipper.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>

namespace RogueCity::Core::Geometry {

namespace {

constexpr double kClipperScale = 1000.0; // 1mm resolution contract
constexpr double kClipperInvScale = 1.0 / kClipperScale;
constexpr double kDefaultEpsilon = 1e-6;

[[nodiscard]] bool IsFinite(const Vec2& v) {
  return std::isfinite(v.x) && std::isfinite(v.y);
}

[[nodiscard]] double SquaredDistance(const Vec2& a, const Vec2& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  return (dx * dx) + (dy * dy);
}

[[nodiscard]] double SignedArea(const Polygon& poly) {
  if (poly.vertices.size() < 3u) {
    return 0.0;
  }
  double area = 0.0;
  for (size_t i = 0; i < poly.vertices.size(); ++i) {
    const Vec2& a = poly.vertices[i];
    const Vec2& b = poly.vertices[(i + 1u) % poly.vertices.size()];
    area += (a.x * b.y) - (b.x * a.y);
  }
  return area * 0.5;
}

[[nodiscard]] double AbsArea(const Polygon& poly) {
  return std::abs(SignedArea(poly));
}

void RemoveCollinearVertices(std::vector<Vec2>& vertices, double epsilon) {
  if (vertices.size() < 3u) {
    return;
  }
  const double cross_epsilon = std::max(epsilon, 1e-12);
  bool removed = true;
  while (removed && vertices.size() >= 3u) {
    removed = false;
    for (size_t i = 0; i < vertices.size(); ++i) {
      const size_t prev_i = (i + vertices.size() - 1u) % vertices.size();
      const size_t next_i = (i + 1u) % vertices.size();
      const Vec2& a = vertices[prev_i];
      const Vec2& b = vertices[i];
      const Vec2& c = vertices[next_i];
      const double abx = b.x - a.x;
      const double aby = b.y - a.y;
      const double bcx = c.x - b.x;
      const double bcy = c.y - b.y;
      const double cross = (abx * bcy) - (aby * bcx);
      if (std::abs(cross) > cross_epsilon) {
        continue;
      }
      vertices.erase(vertices.begin() + static_cast<std::ptrdiff_t>(i));
      removed = true;
      break;
    }
  }
}

[[nodiscard]] Polygon SanitizePolygon(const Polygon& poly, double epsilon) {
  Polygon sanitized;
  const double epsilon_sq = epsilon * epsilon;
  sanitized.vertices.reserve(poly.vertices.size());

  for (const Vec2& v : poly.vertices) {
    if (!IsFinite(v)) {
      continue;
    }
    if (!sanitized.vertices.empty() &&
        SquaredDistance(sanitized.vertices.back(), v) <= epsilon_sq) {
      continue;
    }
    sanitized.vertices.push_back(v);
  }

  if (sanitized.vertices.size() >= 2u &&
      SquaredDistance(sanitized.vertices.front(), sanitized.vertices.back()) <= epsilon_sq) {
    sanitized.vertices.pop_back();
  }

  RemoveCollinearVertices(sanitized.vertices, epsilon);
  return sanitized;
}

[[nodiscard]] bool IsUsable(const Polygon& poly, double epsilon) {
  if (poly.vertices.size() < 3u) {
    return false;
  }
  return std::abs(SignedArea(poly)) > epsilon;
}

[[nodiscard]] Vec2 PolygonCentroid(const Polygon& poly, double epsilon) {
  if (poly.vertices.empty()) {
    return Vec2(0.0, 0.0);
  }

  const double a = SignedArea(poly);
  if (std::abs(a) <= epsilon) {
    Vec2 mean(0.0, 0.0);
    for (const Vec2& v : poly.vertices) {
      mean.x += v.x;
      mean.y += v.y;
    }
    const double denom = static_cast<double>(poly.vertices.size());
    return Vec2(mean.x / denom, mean.y / denom);
  }

  double cx = 0.0;
  double cy = 0.0;
  for (size_t i = 0; i < poly.vertices.size(); ++i) {
    const Vec2& p0 = poly.vertices[i];
    const Vec2& p1 = poly.vertices[(i + 1u) % poly.vertices.size()];
    const double cross = (p0.x * p1.y) - (p1.x * p0.y);
    cx += (p0.x + p1.x) * cross;
    cy += (p0.y + p1.y) * cross;
  }
  const double factor = 1.0 / (6.0 * a);
  return Vec2(cx * factor, cy * factor);
}

[[nodiscard]] Clipper2Lib::Point64 ToClipperPoint(const Vec2& v) {
  return Clipper2Lib::Point64(
      static_cast<int64_t>(std::llround(v.x * kClipperScale)),
      static_cast<int64_t>(std::llround(v.y * kClipperScale)));
}

[[nodiscard]] Vec2 FromClipperPoint(const Clipper2Lib::Point64& pt) {
  return Vec2(
      static_cast<double>(pt.x) * kClipperInvScale,
      static_cast<double>(pt.y) * kClipperInvScale);
}

[[nodiscard]] Clipper2Lib::Path64 ToClipperPath(const Polygon& poly) {
  Clipper2Lib::Path64 path;
  path.reserve(poly.vertices.size());
  for (const Vec2& v : poly.vertices) {
    path.push_back(ToClipperPoint(v));
  }
  return path;
}

[[nodiscard]] double TotalArea(const std::vector<Polygon>& polygons) {
  double area = 0.0;
  for (const Polygon& poly : polygons) {
    area += AbsArea(poly);
  }
  return area;
}

[[nodiscard]] std::vector<Polygon> ExtractPolygons(
    const Clipper2Lib::Paths64& paths,
    double epsilon) {
  std::vector<Polygon> result;
  result.reserve(paths.size());
  for (const auto& path : paths) {
    Polygon out_poly;
    out_poly.vertices.reserve(path.size());
    for (const auto& pt : path) {
      out_poly.vertices.push_back(FromClipperPoint(pt));
    }
    out_poly = SanitizePolygon(out_poly, epsilon);
    if (IsUsable(out_poly, epsilon)) {
      result.push_back(std::move(out_poly));
    }
  }
  return result;
}

[[nodiscard]] std::vector<Polygon> ClipPolygonsImpl(
    const Polygon& subject,
    const Polygon& clip,
    double epsilon) {
  const Polygon subject_input = SanitizePolygon(subject, epsilon);
  if (!IsUsable(subject_input, epsilon)) {
    return {};
  }

  const Polygon clip_input = SanitizePolygon(clip, epsilon);
  if (!IsUsable(clip_input, epsilon)) {
    return {subject_input};
  }

  Clipper2Lib::Clipper64 clipper;
  clipper.AddSubject({ToClipperPath(subject_input)});
  clipper.AddClip({ToClipperPath(clip_input)});
  Clipper2Lib::Paths64 solution;
  clipper.Execute(
      Clipper2Lib::ClipType::Intersection,
      Clipper2Lib::FillRule::NonZero,
      solution);
  return ExtractPolygons(solution, epsilon);
}

[[nodiscard]] std::vector<Polygon> DifferencePolygonsImpl(
    const Polygon& subject,
    const Polygon& clip,
    double epsilon) {
  const Polygon subject_input = SanitizePolygon(subject, epsilon);
  if (!IsUsable(subject_input, epsilon)) {
    return {};
  }

  const Polygon clip_input = SanitizePolygon(clip, epsilon);
  if (!IsUsable(clip_input, epsilon)) {
    return {subject_input};
  }

  Clipper2Lib::Clipper64 clipper;
  clipper.AddSubject({ToClipperPath(subject_input)});
  clipper.AddClip({ToClipperPath(clip_input)});
  Clipper2Lib::Paths64 solution;
  clipper.Execute(
      Clipper2Lib::ClipType::Difference,
      Clipper2Lib::FillRule::NonZero,
      solution);
  return ExtractPolygons(solution, epsilon);
}

[[nodiscard]] std::vector<Polygon> UnionPolygonsImpl(
    const std::vector<Polygon>& polygons,
    double epsilon) {
  Clipper2Lib::Paths64 subject_paths;
  subject_paths.reserve(polygons.size());
  for (const Polygon& poly : polygons) {
    const Polygon sanitized = SanitizePolygon(poly, epsilon);
    if (IsUsable(sanitized, epsilon)) {
      subject_paths.push_back(ToClipperPath(sanitized));
    }
  }
  if (subject_paths.empty()) {
    return {};
  }

  Clipper2Lib::Clipper64 clipper;
  clipper.AddSubject(subject_paths);
  Clipper2Lib::Paths64 solution;
  clipper.Execute(
      Clipper2Lib::ClipType::Union,
      Clipper2Lib::FillRule::NonZero,
      solution);
  return ExtractPolygons(solution, epsilon);
}

void EnsureOrientation(Clipper2Lib::Path64& path, bool want_positive) {
  if (path.size() < 3u) {
    return;
  }
  if (Clipper2Lib::IsPositive(path) != want_positive) {
    std::reverse(path.begin(), path.end());
  }
}

[[nodiscard]] Clipper2Lib::Paths64 BuildRegionClipPaths(
    const PolygonRegion& region,
    double epsilon) {
  const Polygon outer = SanitizePolygon(region.outer, epsilon);
  if (!IsUsable(outer, epsilon)) {
    return {};
  }

  Clipper2Lib::Paths64 clip_paths;
  Clipper2Lib::Path64 outer_path = ToClipperPath(outer);
  const bool outer_positive = Clipper2Lib::IsPositive(outer_path);
  clip_paths.push_back(std::move(outer_path));

  for (const Polygon& hole_raw : region.holes) {
    const Polygon hole = SanitizePolygon(hole_raw, epsilon);
    if (!IsUsable(hole, epsilon)) {
      continue;
    }
    Clipper2Lib::Path64 hole_path = ToClipperPath(hole);
    EnsureOrientation(hole_path, !outer_positive);
    clip_paths.push_back(std::move(hole_path));
  }
  return clip_paths;
}

[[nodiscard]] Polygon PathToPolygon(const Clipper2Lib::Path64& path, double epsilon) {
  Polygon polygon{};
  polygon.vertices.reserve(path.size());
  for (const Clipper2Lib::Point64& pt : path) {
    polygon.vertices.push_back(FromClipperPoint(pt));
  }
  return SanitizePolygon(polygon, epsilon);
}

[[nodiscard]] Clipper2Lib::Paths64 BuildRegionSubjectPaths(
    const PolygonRegion& region,
    double epsilon) {
  return BuildRegionClipPaths(region, epsilon);
}

[[nodiscard]] std::vector<PolygonRegion> ExtractRegions(
    const Clipper2Lib::PolyTree64& tree,
    double epsilon) {
  std::vector<PolygonRegion> regions;

  std::function<void(const Clipper2Lib::PolyPath64&)> visit =
      [&](const Clipper2Lib::PolyPath64& node) {
        if (node.Level() > 0u && !node.IsHole()) {
          PolygonRegion raw{};
          raw.outer = PathToPolygon(node.Polygon(), epsilon);
          for (size_t i = 0; i < node.Count(); ++i) {
            const Clipper2Lib::PolyPath64* child = node.Child(i);
            if (child == nullptr || !child->IsHole()) {
              continue;
            }
            Polygon hole = PathToPolygon(child->Polygon(), epsilon);
            if (IsUsable(hole, epsilon)) {
              raw.holes.push_back(std::move(hole));
            }
          }

          const PolygonRegion simplified = PolygonOps::SimplifyRegion(raw, epsilon);
          if (IsUsable(simplified.outer, epsilon)) {
            regions.push_back(simplified);
          }
        }

        for (size_t i = 0; i < node.Count(); ++i) {
          const Clipper2Lib::PolyPath64* child = node.Child(i);
          if (child != nullptr) {
            visit(*child);
          }
        }
      };

  for (size_t i = 0; i < tree.Count(); ++i) {
    const Clipper2Lib::PolyPath64* child = tree.Child(i);
    if (child != nullptr) {
      visit(*child);
    }
  }
  return regions;
}

[[nodiscard]] std::vector<Polygon> FlattenRegions(const std::vector<PolygonRegion>& regions) {
  std::vector<Polygon> polygons;
  size_t reserve_count = 0;
  for (const PolygonRegion& region : regions) {
    reserve_count += 1u + region.holes.size();
  }
  polygons.reserve(reserve_count);
  for (const PolygonRegion& region : regions) {
    polygons.push_back(region.outer);
    for (const Polygon& hole : region.holes) {
      polygons.push_back(hole);
    }
  }
  return polygons;
}

[[nodiscard]] std::vector<PolygonRegion> ExecuteRegionOp(
    const Clipper2Lib::Paths64& subject_paths,
    const Clipper2Lib::Paths64& clip_paths,
    Clipper2Lib::ClipType clip_type,
    double epsilon) {
  if (subject_paths.empty()) {
    return {};
  }
  Clipper2Lib::Clipper64 clipper;
  clipper.AddSubject(subject_paths);
  if (!clip_paths.empty()) {
    clipper.AddClip(clip_paths);
  }
  Clipper2Lib::PolyTree64 tree;
  clipper.Execute(clip_type, Clipper2Lib::FillRule::NonZero, tree);
  return ExtractRegions(tree, epsilon);
}

} // namespace

std::vector<Polygon> PolygonOps::InsetPolygon(const Polygon& poly, double insetAmount) {
  const Polygon input = SanitizePolygon(poly, kDefaultEpsilon);
  if (!IsUsable(input, kDefaultEpsilon)) {
    return {};
  }
  if (std::abs(insetAmount) <= kDefaultEpsilon) {
    return {input};
  }

  const double scaled_delta = -insetAmount * kClipperScale;
  const Clipper2Lib::Paths64 solution = Clipper2Lib::InflatePaths(
      {ToClipperPath(input)},
      scaled_delta,
      Clipper2Lib::JoinType::Miter,
      Clipper2Lib::EndType::Polygon);
  return ExtractPolygons(solution, kDefaultEpsilon);
}

std::vector<Polygon> PolygonOps::ClipPolygons(const Polygon& subject, const Polygon& clip) {
  return ClipPolygonsImpl(subject, clip, kDefaultEpsilon);
}

std::vector<Polygon> PolygonOps::ClipPolygons(
    const Polygon& subject,
    const PolygonRegion& clipRegion) {
  const Polygon subject_input = SanitizePolygon(subject, kDefaultEpsilon);
  if (!IsUsable(subject_input, kDefaultEpsilon)) {
    return {};
  }

  const PolygonRegion region_input = SimplifyRegion(clipRegion, kDefaultEpsilon);
  if (!IsUsable(region_input.outer, kDefaultEpsilon)) {
    return {};
  }

  PolygonRegion subject_region{};
  subject_region.outer = subject_input;
  return FlattenRegions(ClipRegions(subject_region, region_input));
}

std::vector<Polygon> PolygonOps::DifferencePolygons(const Polygon& subject, const Polygon& clip) {
  return DifferencePolygonsImpl(subject, clip, kDefaultEpsilon);
}

std::vector<Polygon> PolygonOps::DifferencePolygons(
    const Polygon& subject,
    const PolygonRegion& clipRegion) {
  const Polygon subject_input = SanitizePolygon(subject, kDefaultEpsilon);
  if (!IsUsable(subject_input, kDefaultEpsilon)) {
    return {};
  }

  const PolygonRegion region_input = SimplifyRegion(clipRegion, kDefaultEpsilon);
  if (!IsUsable(region_input.outer, kDefaultEpsilon)) {
    return {subject_input};
  }

  PolygonRegion subject_region{};
  subject_region.outer = subject_input;
  return FlattenRegions(DifferenceRegions(subject_region, region_input));
}

std::vector<Polygon> PolygonOps::UnionPolygons(const std::vector<Polygon>& polygons) {
  return UnionPolygonsImpl(polygons, kDefaultEpsilon);
}

std::vector<PolygonRegion> PolygonOps::ClipRegions(
    const PolygonRegion& subjectRegion,
    const PolygonRegion& clipRegion) {
  const PolygonRegion subject_input = SimplifyRegion(subjectRegion, kDefaultEpsilon);
  if (!IsUsable(subject_input.outer, kDefaultEpsilon)) {
    return {};
  }
  const PolygonRegion clip_input = SimplifyRegion(clipRegion, kDefaultEpsilon);
  if (!IsUsable(clip_input.outer, kDefaultEpsilon)) {
    return {};
  }

  const Clipper2Lib::Paths64 subject_paths =
      BuildRegionSubjectPaths(subject_input, kDefaultEpsilon);
  const Clipper2Lib::Paths64 clip_paths =
      BuildRegionClipPaths(clip_input, kDefaultEpsilon);
  return ExecuteRegionOp(
      subject_paths,
      clip_paths,
      Clipper2Lib::ClipType::Intersection,
      kDefaultEpsilon);
}

std::vector<PolygonRegion> PolygonOps::DifferenceRegions(
    const PolygonRegion& subjectRegion,
    const PolygonRegion& clipRegion) {
  const PolygonRegion subject_input = SimplifyRegion(subjectRegion, kDefaultEpsilon);
  if (!IsUsable(subject_input.outer, kDefaultEpsilon)) {
    return {};
  }
  const PolygonRegion clip_input = SimplifyRegion(clipRegion, kDefaultEpsilon);
  if (!IsUsable(clip_input.outer, kDefaultEpsilon)) {
    return {subject_input};
  }

  const Clipper2Lib::Paths64 subject_paths =
      BuildRegionSubjectPaths(subject_input, kDefaultEpsilon);
  const Clipper2Lib::Paths64 clip_paths =
      BuildRegionClipPaths(clip_input, kDefaultEpsilon);
  return ExecuteRegionOp(
      subject_paths,
      clip_paths,
      Clipper2Lib::ClipType::Difference,
      kDefaultEpsilon);
}

std::vector<PolygonRegion> PolygonOps::UnionRegions(const std::vector<PolygonRegion>& regions) {
  Clipper2Lib::Paths64 subject_paths;
  for (const PolygonRegion& raw_region : regions) {
    const PolygonRegion region = SimplifyRegion(raw_region, kDefaultEpsilon);
    if (!IsUsable(region.outer, kDefaultEpsilon)) {
      continue;
    }
    const Clipper2Lib::Paths64 region_paths =
        BuildRegionSubjectPaths(region, kDefaultEpsilon);
    subject_paths.insert(
        subject_paths.end(),
        region_paths.begin(),
        region_paths.end());
  }
  return ExecuteRegionOp(
      subject_paths,
      {},
      Clipper2Lib::ClipType::Union,
      kDefaultEpsilon);
}

Polygon PolygonOps::SimplifyPolygon(const Polygon& poly, double epsilon) {
  const double safe_epsilon = std::max(epsilon, kDefaultEpsilon);
  return SanitizePolygon(poly, safe_epsilon);
}

PolygonRegion PolygonOps::SimplifyRegion(const PolygonRegion& region, double epsilon) {
  const double safe_epsilon = std::max(epsilon, kDefaultEpsilon);
  PolygonRegion simplified{};
  simplified.outer = SanitizePolygon(region.outer, safe_epsilon);
  if (!IsUsable(simplified.outer, safe_epsilon)) {
    return simplified;
  }

  for (const Polygon& hole_raw : region.holes) {
    const Polygon hole = SanitizePolygon(hole_raw, safe_epsilon);
    if (!IsUsable(hole, safe_epsilon)) {
      continue;
    }

    const double overlap_with_outer = TotalArea(
        ClipPolygonsImpl(hole, simplified.outer, safe_epsilon));
    if (std::abs(overlap_with_outer - AbsArea(hole)) > (2.0 * safe_epsilon)) {
      continue;
    }

    const Vec2 center = PolygonCentroid(hole, safe_epsilon);
    if (!IsFinite(center)) {
      continue;
    }
    const Clipper2Lib::PointInPolygonResult pip =
        Clipper2Lib::PointInPolygon(ToClipperPoint(center), ToClipperPath(simplified.outer));
    if (pip == Clipper2Lib::PointInPolygonResult::IsOutside) {
      continue;
    }

    bool overlaps_existing = false;
    for (const Polygon& existing_hole : simplified.holes) {
      const double overlap_area = TotalArea(
          ClipPolygonsImpl(hole, existing_hole, safe_epsilon));
      if (overlap_area > safe_epsilon) {
        overlaps_existing = true;
        break;
      }
    }
    if (overlaps_existing) {
      continue;
    }

    simplified.holes.push_back(hole);
  }

  return simplified;
}

bool PolygonOps::IsValidPolygon(const Polygon& poly, double epsilon) {
  const double safe_epsilon = std::max(epsilon, kDefaultEpsilon);
  const Polygon sanitized = SanitizePolygon(poly, safe_epsilon);
  return IsUsable(sanitized, safe_epsilon);
}

bool PolygonOps::IsValidRegion(const PolygonRegion& region, double epsilon) {
  const double safe_epsilon = std::max(epsilon, kDefaultEpsilon);
  const Polygon outer = SanitizePolygon(region.outer, safe_epsilon);
  if (!IsUsable(outer, safe_epsilon)) {
    return false;
  }

  std::vector<Polygon> accepted_holes;
  for (const Polygon& hole_raw : region.holes) {
    const Polygon hole = SanitizePolygon(hole_raw, safe_epsilon);
    if (!IsUsable(hole, safe_epsilon)) {
      return false;
    }

    const double overlap_with_outer = TotalArea(
        ClipPolygonsImpl(hole, outer, safe_epsilon));
    if (std::abs(overlap_with_outer - AbsArea(hole)) > (2.0 * safe_epsilon)) {
      return false;
    }

    for (const Polygon& existing_hole : accepted_holes) {
      const double overlap_area = TotalArea(
          ClipPolygonsImpl(hole, existing_hole, safe_epsilon));
      if (overlap_area > safe_epsilon) {
        return false;
      }
    }
    accepted_holes.push_back(hole);
  }
  return true;
}

} // namespace RogueCity::Core::Geometry
