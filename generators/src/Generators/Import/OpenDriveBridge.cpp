#include "RogueCity/Generators/Import/OpenDriveBridge.hpp"
#include "RogueOpenDRIVE/Junction.h"
#include "RogueOpenDRIVE/OpenDriveMap.h"
#include "RogueOpenDRIVE/Road.h"
#include "RogueOpenDRIVE/Serialization/JsonSerialization.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>

namespace {

uint32_t ParseStableId(const std::string &id_value) {
  try {
    const unsigned long parsed = std::stoul(id_value);
    if (parsed <= std::numeric_limits<uint32_t>::max()) {
      return static_cast<uint32_t>(parsed);
    }
  } catch (...) {
  }

  const uint32_t hashed =
      static_cast<uint32_t>(std::hash<std::string>{}(id_value));
  return hashed == 0u ? 1u : hashed;
}

bool IsFinitePoint(const odr::Vec3D &pt) {
  return std::isfinite(pt[0]) && std::isfinite(pt[1]) && std::isfinite(pt[2]);
}

void AppendRoadSample(const odr::Vec3D &pt, RogueCity::Core::Road &core_road) {
  if (!IsFinitePoint(pt)) {
    return;
  }

  const RogueCity::Core::Vec2 world_pt(pt[0], pt[1]);
  if (!core_road.points.empty()) {
    const RogueCity::Core::Vec2 &last_pt = core_road.points.back();
    const double dx = last_pt.x - world_pt.x;
    const double dy = last_pt.y - world_pt.y;
    if ((dx * dx + dy * dy) <= 1e-12) {
      core_road.elevation_offsets.back() = static_cast<float>(pt[2]);
      return;
    }
  }

  core_road.points.push_back(world_pt);
  core_road.elevation_offsets.push_back(static_cast<float>(pt[2]));
}

} // namespace

namespace RogueCity::Generators::Import {

bool OpenDriveBridge::parseAndMerge(
    const std::string &xodr_path, std::vector<Core::Road> &out_roads,
    std::vector<Core::IntersectionTemplate> &out_intersections,
    const Core::WorldConstraintField &world_constraints,
    Core::Data::SpatialReference *out_spatial_reference) {
  (void)world_constraints;
  if (xodr_path.empty()) {
    return false;
  }

  // Parse the OpenDRIVE file using the new standalone rc_opendrive library
  std::unique_ptr<odr::OpenDriveMap> map;
  try {
    map = std::make_unique<odr::OpenDriveMap>(xodr_path, true, true, true,
                                              false, true, true);
  } catch (...) {
    return false;
  }

  if (out_spatial_reference != nullptr) {
    if (!map->proj4.empty()) {
      *out_spatial_reference =
          Core::Data::SpatialReference::FromProj4(map->proj4);
    } else {
      *out_spatial_reference = Core::Data::SpatialReference::LocalPlanarMeters();
    }
  }

  for (const auto &odr_road : map->get_roads()) {
    Core::Road core_road;
    core_road.id = ParseStableId(odr_road.id);

    core_road.type = Core::RoadType::Street; // Default road type
    core_road.generation_tag = Core::GenerationTag::Generated;

    // Serialize OpenDRIVE spec data (Magic Spline Features)
    nlohmann::json odr_json = odr_road;
    core_road.asam_json_payload = odr_json.dump(2);

    // Initial semantic passes for hints (Signals / Crosswalks)
    core_road.contains_signal = !odr_road.id_to_signal.empty();
    for (const auto &[object_id, obj] : odr_road.id_to_object) {
      (void)object_id;
      if (obj.type == "crosswalk") {
        core_road.contains_crosswalk = true;
        break;
      }
    }

    // Spatial Intelligence: Infrastructure Mapping
    if (!odr_road.tunnels.empty()) {
      core_road.layer_id = -1; // Tunnel level
      core_road.has_grade_separation = true;
    } else if (!odr_road.bridges.empty()) {
      core_road.layer_id = 1; // Bridge level
      core_road.has_grade_separation = true;
    }

    // Sampling resolution along the reference line
    const double resolution = 2.0;
    if (odr_road.length > 0.0) {
      const std::size_t sample_count = static_cast<std::size_t>(
                                           std::ceil(odr_road.length / resolution)) +
                                       1u;
      core_road.points.reserve(sample_count);
      core_road.elevation_offsets.reserve(sample_count);
    }

    for (double s = 0.0; s <= odr_road.length; s += resolution) {
      AppendRoadSample(odr_road.get_surface_pt(s, 0.0), core_road);
    }

    // Ensure last point is included if not hit exactly by resolution
    if (odr_road.length > 0.0 &&
        std::fmod(odr_road.length, resolution) != 0.0) {
      AppendRoadSample(odr_road.get_surface_pt(odr_road.length, 0.0),
                       core_road);
    }

    if (core_road.points.empty()) {
      continue;
    }

    out_roads.push_back(std::move(core_road));
  }

  for (const auto &odr_junc : map->get_junctions()) {
    Core::IntersectionTemplate core_junc;
    core_junc.id = ParseStableId(odr_junc.id);

    // OpenDRIVE does not provide an explicit center point for junctions,
    // it defines topologies. For MVP, we can leave center at 0,0 or compute
    // a bounded average if needed in the future.
    core_junc.center = Core::Vec2(0.0, 0.0);
    core_junc.radius = 10.0f;

    out_intersections.push_back(std::move(core_junc));
  }

  return true;
}

} // namespace RogueCity::Generators::Import
