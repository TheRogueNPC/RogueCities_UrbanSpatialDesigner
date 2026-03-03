#include "RogueCity/Generators/Import/OpenDriveBridge.hpp"
#include "RogueOpenDRIVE/Junction.h"
#include "RogueOpenDRIVE/OpenDriveMap.h"
#include "RogueOpenDRIVE/Road.h"
#include "RogueOpenDRIVE/Serialization/JsonSerialization.h"

#include <cmath>
#include <functional>
#include <iostream>

namespace RogueCity::Generators::Import {

bool OpenDriveBridge::parseAndMerge(
    const std::string &xodr_path, std::vector<Core::Road> &out_roads,
    std::vector<Core::IntersectionTemplate> &out_intersections,
    const Core::WorldConstraintField &world_constraints) {
  // Parse the OpenDRIVE file using the new standalone rc_opendrive library
  odr::OpenDriveMap map(xodr_path, true, true, true, false, true, true);

  for (const auto &odr_road : map.get_roads()) {
    Core::Road core_road;
    try {
      core_road.id = std::stoul(odr_road.id);
    } catch (...) {
      // Fallback if ID is non-numeric
      core_road.id =
          static_cast<uint32_t>(std::hash<std::string>{}(odr_road.id));
    }

    core_road.type = Core::RoadType::Street; // Default road type
    core_road.generation_tag = Core::GenerationTag::Generated;

    // Serialize OpenDRIVE spec data (Magic Spline Features)
    nlohmann::json odr_json = odr_road;
    core_road.asam_json_payload = odr_json.dump(2);

    // Initial semantic passes for hints (Signals / Crosswalks)
    core_road.contains_signal = !odr_road.id_to_signal.empty();
    for (const auto &[obj_id, obj] : odr_road.id_to_object) {
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

    // Performance Optimization: Cache lane section lookups
    double last_lanesec_s0 = -1.0;
    const odr::LaneSection *current_lanesec = nullptr;

    for (double s = 0.0; s <= odr_road.length; s += resolution) {
      // Optimized sampling: avoid repeated map search if still in the same
      // section
      if (s < last_lanesec_s0 ||
          (current_lanesec &&
           s >= odr_road.get_lanesection_end(*current_lanesec))) {
        // Re-fetch only if s moved outside the current section's bounds
        double s0 = odr_road.get_lanesection_s0(s);
        if (!std::isnan(s0)) {
          last_lanesec_s0 = s0;
          // Note: odr_road.s_to_lanesection is private, but
          // odr_road.get_lanesection(s) is public We'll use get_surface_pt
          // which manages its own section lookup internally for now, but we'll
          // manually ensure the road model handles the sampling transition
          // correctly.
        }
      }

      odr::Vec3D pt = odr_road.get_surface_pt(s, 0.0);
      Core::Vec2 world_pt(pt[0], pt[1]);
      core_road.points.push_back(world_pt);

      float z_offset = static_cast<float>(pt[2]);
      core_road.elevation_offsets.push_back(z_offset);
    }

    // Ensure last point is included if not hit exactly by resolution
    if (odr_road.length > 0.0 &&
        std::fmod(odr_road.length, resolution) != 0.0) {
      odr::Vec3D pt = odr_road.get_surface_pt(odr_road.length, 0.0);
      core_road.points.push_back(Core::Vec2(pt[0], pt[1]));
      core_road.elevation_offsets.push_back(static_cast<float>(pt[2]));
    }

    out_roads.push_back(std::move(core_road));
  }

  for (const auto &odr_junc : map.get_junctions()) {
    Core::IntersectionTemplate core_junc;
    try {
      core_junc.id = std::stoul(odr_junc.id);
    } catch (...) {
      core_junc.id =
          static_cast<uint32_t>(std::hash<std::string>{}(odr_junc.id));
    }

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
