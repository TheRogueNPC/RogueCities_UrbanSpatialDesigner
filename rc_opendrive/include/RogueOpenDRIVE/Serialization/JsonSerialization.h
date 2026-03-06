#pragma once

#include "RogueOpenDRIVE/Geometries/Arc.h"
#include "RogueOpenDRIVE/Geometries/CubicSpline.h"
#include "RogueOpenDRIVE/Geometries/Line.h"
#include "RogueOpenDRIVE/Geometries/ParamPoly3.h"
#include "RogueOpenDRIVE/Geometries/Spiral.h"
#include "RogueOpenDRIVE/Junction.h"
#include "RogueOpenDRIVE/Lane.h"
#include "RogueOpenDRIVE/LaneSection.h"
#include "RogueOpenDRIVE/LaneValidityRecord.h"
#include "RogueOpenDRIVE/OpenDriveMap.h"
#include "RogueOpenDRIVE/Railroad.h" // RailroadTrack, RailroadPartner, RailroadSwitch
#include "RogueOpenDRIVE/RefLine.h"
#include "RogueOpenDRIVE/Road.h"
#include "RogueOpenDRIVE/RoadMark.h"
#include "RogueOpenDRIVE/RoadObject.h"
#include "RogueOpenDRIVE/RoadSignal.h"

#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <vector>

// Support for std::optional
namespace nlohmann {
template <typename T> struct adl_serializer<std::optional<T>> {
  static void to_json(json &j, const std::optional<T> &opt) {
    if (opt.has_value()) {
      j = *opt;
    } else {
      j = nullptr;
    }
  }

  static void from_json(const json &j, std::optional<T> &opt) {
    if (j.is_null()) {
      opt = std::nullopt;
    } else {
      opt = j.get<T>();
    }
  }
};
} // namespace nlohmann

namespace odr {

namespace detail {
template <typename T>
inline bool try_get(const nlohmann::json &j, const char *key, T &out) {
  const auto it = j.find(key);
  if (it == j.end() || it->is_null()) {
    return false;
  }
  try {
    out = it->get<T>();
    return true;
  } catch (...) {
    return false;
  }
}

template <typename T>
inline T value_or(const nlohmann::json &j, const char *key, T fallback) {
  T value = fallback;
  if (try_get(j, key, value)) {
    return value;
  }
  return fallback;
}
} // namespace detail

// 1. Basic Geometries
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CubicPoly, a, b, c, d)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CubicProfile, segments)

// 2. RoadGeometry Polymorphism
inline void to_json(nlohmann::json &j, const RoadGeometry &rg) {
  j = nlohmann::json{{"s0", rg.s0},
                     {"x0", rg.x0},
                     {"y0", rg.y0},
                     {"hdg0", rg.hdg0},
                     {"length", rg.length}};
  if (auto line = dynamic_cast<const Line *>(&rg)) {
    j["type"] = "line";
  } else if (auto arc = dynamic_cast<const Arc *>(&rg)) {
    j["type"] = "arc";
    j["curvature"] = arc->curvature;
  } else if (auto spiral = dynamic_cast<const Spiral *>(&rg)) {
    j["type"] = "spiral";
    j["curv_start"] = spiral->curv_start;
    j["curv_end"] = spiral->curv_end;
    j["s_start"] = spiral->s_start;
    j["s_end"] = spiral->s_end;
    j["c_dot"] = spiral->c_dot;
  } else if (auto poly = dynamic_cast<const ParamPoly3 *>(&rg)) {
    j["type"] = "poly3";
    j["aU"] = poly->aU;
    j["bU"] = poly->bU;
    j["cU"] = poly->cU;
    j["dU"] = poly->dU;
    j["aV"] = poly->aV;
    j["bV"] = poly->bV;
    j["cV"] = poly->cV;
    j["dV"] = poly->dV;
    j["pRange_normalized"] = poly->pRange_normalized;
  }
}

inline void from_json(const nlohmann::json &j,
                      std::unique_ptr<RoadGeometry> &rg) {
  rg.reset();
  if (!j.is_object()) {
    return;
  }

  std::string type;
  double s0 = 0.0;
  double x0 = 0.0;
  double y0 = 0.0;
  double hdg0 = 0.0;
  double length = 0.0;
  if (!detail::try_get(j, "type", type) || !detail::try_get(j, "s0", s0) ||
      !detail::try_get(j, "x0", x0) || !detail::try_get(j, "y0", y0) ||
      !detail::try_get(j, "hdg0", hdg0) ||
      !detail::try_get(j, "length", length)) {
    return;
  }
  if (length < 0.0) {
    length = 0.0;
  }

  if (type == "line") {
    rg = std::make_unique<Line>(s0, x0, y0, hdg0, length);
  } else if (type == "arc") {
    double curvature = 0.0;
    if (!detail::try_get(j, "curvature", curvature)) {
      return;
    }
    rg = std::make_unique<Arc>(s0, x0, y0, hdg0, length, curvature);
  } else if (type == "spiral") {
    double curv_start = 0.0;
    double curv_end = 0.0;
    if (!detail::try_get(j, "curv_start", curv_start) ||
        !detail::try_get(j, "curv_end", curv_end)) {
      return;
    }
    rg = std::make_unique<Spiral>(s0, x0, y0, hdg0, length, curv_start,
                                  curv_end);
  } else if (type == "poly3") {
    double aU = 0.0;
    double bU = 0.0;
    double cU = 0.0;
    double dU = 0.0;
    double aV = 0.0;
    double bV = 0.0;
    double cV = 0.0;
    double dV = 0.0;
    if (!detail::try_get(j, "aU", aU) || !detail::try_get(j, "bU", bU) ||
        !detail::try_get(j, "cU", cU) || !detail::try_get(j, "dU", dU) ||
        !detail::try_get(j, "aV", aV) || !detail::try_get(j, "bV", bV) ||
        !detail::try_get(j, "cV", cV) || !detail::try_get(j, "dV", dV)) {
      return;
    }
    const bool pRange_normalized =
        detail::value_or<bool>(j, "pRange_normalized", false);
    rg = std::make_unique<ParamPoly3>(s0, x0, y0, hdg0, length, aU, bU, cU, dU,
                                      aV, bV, cV, dV, pRange_normalized);
  }
}

// 3. RefLine
inline void to_json(nlohmann::json &j, const RefLine &rl) {
  j = nlohmann::json{{"length", rl.length},
                     {"elevation_profile", rl.elevation_profile}};
  nlohmann::json geometries = nlohmann::json::array();
  for (const auto &[s0, geom] : rl.s0_to_geometry) {
    if (geom) {
      geometries.push_back(*geom);
    }
  }
  j["geometries"] = geometries;
}

inline void from_json(const nlohmann::json &j, RefLine &rl) {
  if (!j.is_object()) {
    rl.length = 0.0;
    rl.elevation_profile = CubicProfile{};
    rl.s0_to_geometry.clear();
    return;
  }

  rl.length = detail::value_or<double>(j, "length", 0.0);
  if (const auto it = j.find("elevation_profile");
      it != j.end() && !it->is_null()) {
    try {
      rl.elevation_profile = it->get<CubicProfile>();
    } catch (...) {
      rl.elevation_profile = CubicProfile{};
    }
  } else {
    rl.elevation_profile = CubicProfile{};
  }
  rl.s0_to_geometry.clear();
  const auto geoms_it = j.find("geometries");
  if (geoms_it == j.end() || !geoms_it->is_array()) {
    return;
  }
  for (const auto &j_geom : *geoms_it) {
    std::unique_ptr<RoadGeometry> geom;
    from_json(j_geom, geom);
    if (geom) {
      rl.s0_to_geometry[geom->s0] = std::move(geom);
    }
  }
}

// 4. Lane Records and Road Marks
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LaneValidityRecord, from_lane, to_lane)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadMarksLine, road_id, lanesection_s0,
                                   lane_id, group_s0, width, length, space,
                                   t_offset, s_offset, name, rule)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadMarkGroup, road_id, lanesection_s0,
                                   lane_id, width, height, s_offset, type,
                                   weight, color, material, lane_change,
                                   roadmark_lines)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadMark, road_id, lanesection_s0, lane_id,
                                   group_s0, s_start, s_end, t_offset, width,
                                   type)

// 5. Lanes
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HeightOffset, inner, outer)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LaneKey, road_id, lanesection_s0, lane_id)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Lane, key, id, level, type, predecessor,
                                   successor, lane_width, outer_border,
                                   s_to_height_offset, roadmark_groups)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LaneSection, road_id, s0, id_to_lane)

// 6. Road Objects and Signals
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObjectMaterial, friction, roughness,
                                   surface, road_mark_color)

// RoadObjectRepeat - §11.2.2 repeat element
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObjectRepeat, s0, length, distance,
                                   t_start, t_end, width_start, width_end,
                                   height_start, height_end, z_offset_start,
                                   z_offset_end)

// RoadObjectCorner / RoadObjectOutline - §11.2.3 outline element
NLOHMANN_JSON_SERIALIZE_ENUM(
    RoadObjectCorner::Type,
    {{RoadObjectCorner::Type::Local_RelZ, "local_relz"},
     {RoadObjectCorner::Type::Local_AbsZ, "local_absz"},
     {RoadObjectCorner::Type::Road, "road"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObjectCorner, id, pt, height, type)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObjectOutline, id, fill_type, lane_type,
                                   outer, closed, outline)

NLOHMANN_JSON_SERIALIZE_ENUM(
    RoadObjectParkingSpace::Access,
    {{RoadObjectParkingSpace::Access::All, "all"},
     {RoadObjectParkingSpace::Access::Bus, "bus"},
     {RoadObjectParkingSpace::Access::Car, "car"},
     {RoadObjectParkingSpace::Access::Electric, "electric"},
     {RoadObjectParkingSpace::Access::Handicapped, "handicapped"},
     {RoadObjectParkingSpace::Access::Residents, "residents"},
     {RoadObjectParkingSpace::Access::Truck, "truck"},
     {RoadObjectParkingSpace::Access::Women, "women"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObjectParkingSpace, access, restrictions)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObjectMarkingCornerRef, id)

NLOHMANN_JSON_SERIALIZE_ENUM(RoadObjectMarking::Side,
                             {{RoadObjectMarking::Side::Front, "front"},
                              {RoadObjectMarking::Side::Left, "left"},
                              {RoadObjectMarking::Side::Rear, "rear"},
                              {RoadObjectMarking::Side::Right, "right"}})
NLOHMANN_JSON_SERIALIZE_ENUM(RoadObjectMarking::Weight,
                             {{RoadObjectMarking::Weight::Standard, "standard"},
                              {RoadObjectMarking::Weight::Bold, "bold"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObjectMarking, side, weight, width,
                                   color, z_offset, space_length, line_length,
                                   start_offset, stop_offset, corner_references)

NLOHMANN_JSON_SERIALIZE_ENUM(RoadObjectBorder::Type,
                             {{RoadObjectBorder::Type::Concrete, "concrete"},
                              {RoadObjectBorder::Type::Curb, "curb"},
                              {RoadObjectBorder::Type::Paint, "paint"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObjectBorder, width, type, outline_id,
                                   use_for_mapping)

NLOHMANN_JSON_SERIALIZE_ENUM(RoadObjectSkeletonVertex::Type,
                             {{RoadObjectSkeletonVertex::Type::Local, "Local"},
                              {RoadObjectSkeletonVertex::Type::Road, "Road"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObjectSkeletonVertex, pt, type)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObjectSkeleton, id, vertices)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ObjectReference, id, s, t, z_offset,
                                   valid_length, orientation)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadSignalDependency, id, type)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadSignalReference, element_id,
                                   element_type, type)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SignalReference, id, s, t, orientation)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadSignalPositionRoad, road_id, s, t,
                                   z_offset, h_offset, pitch, roll)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadSignalPositionInertial, x, y, z, hdg,
                                   pitch, roll)

NLOHMANN_JSON_SERIALIZE_ENUM(RoadTunnel::Type,
                             {{RoadTunnel::Type::Standard, "standard"},
                              {RoadTunnel::Type::Underpass, "underpass"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadTunnel, id, s, length, type, lighting,
                                   daylight, lane_validities)

NLOHMANN_JSON_SERIALIZE_ENUM(RoadBridge::Type,
                             {{RoadBridge::Type::Concrete, "concrete"},
                              {RoadBridge::Type::Steel, "steel"},
                              {RoadBridge::Type::Wood, "wood"},
                              {RoadBridge::Type::Brick, "brick"},
                              {RoadBridge::Type::Extra, "extra"},
                              {RoadBridge::Type::Poly, "poly"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadBridge, id, name, s, length, type,
                                   lane_validities)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadObject, road_id, id, type, name,
                                   orientation, subtype, s0, t0, z0, length,
                                   valid_length, width, radius, height, hdg,
                                   pitch, roll, is_dynamic, repeats, outlines,
                                   lane_validities, materials, parking_spaces,
                                   markings, borders, skeletons)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadSignal, road_id, id, name, s0, t0,
                                   is_dynamic, zOffset, value, height, width,
                                   hOffset, pitch, roll, orientation, country,
                                   country_revision, type, subtype, unit, text,
                                   lane_validities, dependencies, references,
                                   position_road, position_inertial)

// 7. Road Links and Metadata
// Railroad types - §15.3
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RailroadTrack, id, s, dir)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RailroadPartner, id, name)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RailroadSwitch, id, name, position,
                                   main_track, side_track, partners)

NLOHMANN_JSON_SERIALIZE_ENUM(Crossfall::Side,
                             {{Crossfall::Side::Both, "Both"},
                              {Crossfall::Side::Left, "Left"},
                              {Crossfall::Side::Right, "Right"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Crossfall, segments, sides)

NLOHMANN_JSON_SERIALIZE_ENUM(RoadLink::ContactPoint,
                             {{RoadLink::ContactPoint::None, "None"},
                              {RoadLink::ContactPoint::Start, "Start"},
                              {RoadLink::ContactPoint::End, "End"}})
NLOHMANN_JSON_SERIALIZE_ENUM(RoadLink::Type,
                             {{RoadLink::Type::None, "None"},
                              {RoadLink::Type::Road, "Road"},
                              {RoadLink::Type::Junction, "Junction"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadLink, id, type, contact_point)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoadNeighbor, id, side, direction)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SpeedRecord, max, unit)

// 8. Road
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    Road, length, id, junction, name, left_hand_traffic, predecessor, successor,
    neighbors, lane_offset, superelevation, crossfall, ref_line,
    s_to_lanesection, s_to_type, s_to_speed, id_to_object, id_to_signal,
    object_references, signal_references, tunnels, bridges, railroad_switches)

// 9. Junctions
NLOHMANN_JSON_SERIALIZE_ENUM(JunctionConnection::ContactPoint,
                             {{JunctionConnection::ContactPoint::None, "None"},
                              {JunctionConnection::ContactPoint::Start,
                               "Start"},
                              {JunctionConnection::ContactPoint::End, "End"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JunctionLaneLink, from, to)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JunctionConnection, id, incoming_road,
                                   connecting_road, contact_point, lane_links)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JunctionPriority, high, low)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JunctionController, id, type, sequence)

// 9a. Advanced Junction Features (ASAM OpenDRIVE 1.8 Common Junctions)
//
// JunctionCrossPath         — § 6.3.3 / Code 24 (Pedestrian Crossings)
// JunctionTrafficIsland     — § 6.5   / Code 32 (Traffic Islands)
// JunctionCornerLocal       — <cornerLocal> polygon vertex
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JunctionCrossPathLink, s, from, to)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JunctionCrossPath, id, crossing_road,
                                   road_at_start, road_at_end, start_lane_link,
                                   end_lane_link)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JunctionCornerLocal, u, v, z, height)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JunctionTrafficIsland, id, name, height,
                                   z_offset, fill_type, outline)

// Junction root — includes type, cross_paths, and traffic_islands
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Junction, name, id, type, id_to_connection,
                                   id_to_controller, priorities, cross_paths,
                                   traffic_islands)

// 10. Map
NLOHMANN_JSON_SERIALIZE_ENUM(CoordinateSystem,
                             {{CoordinateSystem::LocalMetric, "LocalMetric"},
                              {CoordinateSystem::GlobalProjection,
                               "GlobalProjection"}})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OpenDriveMap, proj4, x_offs, y_offs,
                                   coordinate_system, id_to_road,
                                   id_to_junction)

} // namespace odr
