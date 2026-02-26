#include "RogueCity/Core/Geometry/PolygonOps.hpp"
#include <clipper2/clipper.h>

namespace RogueCity::Core::Geometry {

// Contract explicit rule: Use 1000.0 (1mm resolution) for Clipper scaling
constexpr double kClipperScale = 1000.0;
constexpr double kClipperInvScale = 1.0 / kClipperScale;

// Utility to convert Vec2 to Clipper2's Point64
static inline Clipper2Lib::Point64 ToClipperPoint(const Vec2 &v) {
  return Clipper2Lib::Point64(
      static_cast<int64_t>(std::round(v.x * kClipperScale)),
      static_cast<int64_t>(std::round(v.y * kClipperScale)));
}

// Utility to convert Clipper2's Point64 back to Vec2
static inline Vec2 FromClipperPoint(const Clipper2Lib::Point64 &pt) {
  return Vec2(static_cast<double>(pt.x) * kClipperInvScale,
              static_cast<double>(pt.y) * kClipperInvScale);
}

// Utility to convert Core::Geometry::Polygon to Clipper2 Path64
static Clipper2Lib::Path64 ToClipperPath(const Polygon &poly) {
  Clipper2Lib::Path64 path;
  path.reserve(poly.vertices.size());
  for (const auto &v : poly.vertices) {
    path.push_back(ToClipperPoint(v));
  }
  return path;
}

// Utility to extract a vector of Polygons from a Clipper2 Paths64
static std::vector<Polygon> ExtractPolygons(const Clipper2Lib::Paths64 &paths) {
  std::vector<Polygon> result;
  result.reserve(paths.size());
  for (const auto &path : paths) {
    Polygon outPoly;
    outPoly.vertices.reserve(path.size());
    for (const auto &pt : path) {
      outPoly.vertices.push_back(FromClipperPoint(pt));
    }
    result.push_back(std::move(outPoly));
  }
  return result;
}

std::vector<Polygon> PolygonOps::InsetPolygon(const Polygon &poly,
                                              double insetAmount) {
  if (poly.vertices.empty())
    return {};

  Clipper2Lib::Path64 inputPath = ToClipperPath(poly);

  // Insetting translates to a negative delta in Clipper
  double scaledDelta = -insetAmount * kClipperScale;

  Clipper2Lib::Paths64 solution;
  solution = Clipper2Lib::InflatePaths({inputPath}, scaledDelta,
                                       Clipper2Lib::JoinType::Miter,
                                       Clipper2Lib::EndType::Polygon);

  return ExtractPolygons(solution);
}

std::vector<Polygon> PolygonOps::ClipPolygons(const Polygon &subject,
                                              const Polygon &clip) {
  if (subject.vertices.empty())
    return {};
  if (clip.vertices.empty())
    return {subject};

  Clipper2Lib::Clipper64 clipper;
  clipper.AddSubject({ToClipperPath(subject)});
  clipper.AddClip({ToClipperPath(clip)});

  Clipper2Lib::Paths64 solution;
  clipper.Execute(Clipper2Lib::ClipType::Intersection,
                  Clipper2Lib::FillRule::NonZero, solution);

  return ExtractPolygons(solution);
}

} // namespace RogueCity::Core::Geometry
