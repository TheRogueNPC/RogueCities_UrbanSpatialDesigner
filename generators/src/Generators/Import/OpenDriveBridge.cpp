#include "RogueCity/Generators/Import/OpenDriveBridge.hpp"
#include "RogueOpenDRIVE/Junction.h"
#include "RogueOpenDRIVE/OpenDriveMap.h"
#include "RogueOpenDRIVE/Road.h"


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

    // Sampling resolution along the reference line
    const double resolution = 2.0;

    for (double s = 0.0; s <= odr_road.length; s += resolution) {
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
