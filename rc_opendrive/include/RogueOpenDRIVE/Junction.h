/**
 * @file include/RogueOpenDRIVE/Junction.h
 * @brief ASAM OpenDRIVE 1.8 Junction data model for rc_opendrive.
 *
 * Implements the full Common Junction topology (ASAM OpenDRIVE § 6):
 *   - JunctionLaneLink        : <laneLink>         (§ 6.3.1.3)
 *   - JunctionConnection      : <connection>        (§ 6.3.1.3)
 *   - JunctionPriority        : <priority>          (§ 6.3.2)
 *   - JunctionController      : <controller>        (§ 6.3.4 / Code 18)
 *   - JunctionCrossPathLink   : <startLaneLink> /
 *                               <endLaneLink>       (§ 6.3.3 / Code 24)
 *   - JunctionCrossPath       : <crossPath>         (§ 6.3.3 / Code 24)
 *   - JunctionCornerLocal     : <cornerLocal>       (§ 6.5   / Code 32)
 *   - JunctionTrafficIsland   : <object type=
 *                               "trafficIsland">    (§ 6.5   / Code 32)
 *   - Junction                : <junction>          (§ 6.2)
 *
 * Naming follows snake_case to match existing odr:: conventions and allow
 * zero-boilerplate NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE bindings.
 */

#pragma once
#include "Utils.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace odr {

// =============================================================================
// § 6.3.1.3  Lane Links
// =============================================================================

/// Represents a single <laneLink from="..." to="..."/> element.
/// Maps one lane on the incoming road to one lane on the connecting road.
struct JunctionLaneLink {
  JunctionLaneLink() = default;
  JunctionLaneLink(int from, int to);

  int from = 0; ///< Lane ID on the incoming road.
  int to = 0;   ///< Lane ID on the connecting road.
};

} // namespace odr

namespace std {
template <> struct less<odr::JunctionLaneLink> {
  bool operator()(const odr::JunctionLaneLink &lhs,
                  const odr::JunctionLaneLink &rhs) const {
    return odr::compare_class_members(lhs, rhs, less<void>(),
                                      &odr::JunctionLaneLink::from,
                                      &odr::JunctionLaneLink::to);
  }
};
} // namespace std

namespace odr {

// =============================================================================
// § 6.3.1.3  Connection (vehicle path through the junction)
// =============================================================================

/// Represents a <connection> element inside a <junction>.
/// Each connection describes one navigable path: it links an incoming road to
/// a connecting (virtual) road that physically exists inside the junction area.
struct JunctionConnection {
  enum class ContactPoint {
    None,  ///< Unspecified / legacy fallback.
    Start, ///< The start end of the connecting road.
    End    ///< The end of the connecting road.
  };

  JunctionConnection() = default;
  JunctionConnection(std::string id, std::string incoming_road,
                     std::string connecting_road, ContactPoint contact_point);

  std::string id = "";
  std::string incoming_road = "";   ///< ID of road arriving at junction.
  std::string connecting_road = ""; ///< ID of the virtual road inside junction.
  ContactPoint contact_point = ContactPoint::None;

  std::set<JunctionLaneLink> lane_links; ///< Lane-to-lane mappings.
};

// =============================================================================
// § 6.3.2  Priority
// =============================================================================

/// Represents a <priority high="..." low="..."/> element.
/// Encodes right-of-way between two roads at an uncontrolled junction.
struct JunctionPriority {
  JunctionPriority() = default;
  JunctionPriority(std::string high, std::string low);

  std::string high = ""; ///< Road ID with priority.
  std::string low = "";  ///< Road ID that must yield.
};

} // namespace odr

namespace std {
template <> struct less<odr::JunctionPriority> {
  bool operator()(const odr::JunctionPriority &lhs,
                  const odr::JunctionPriority &rhs) const {
    return odr::compare_class_members(lhs, rhs, less<void>(),
                                      &odr::JunctionPriority::high,
                                      &odr::JunctionPriority::low);
  }
};
} // namespace std

namespace odr {

// =============================================================================
// § 6.3.4 / Code 18  Controller Reference
// =============================================================================

/// Represents a <controller id="..." type="..." sequence="..."/> element.
/// Links the junction to a traffic-light controller for signal synchronisation
/// (required for OpenSCENARIO interop).
struct JunctionController {
  JunctionController() = default;
  JunctionController(std::string id, std::string type, std::uint32_t sequence);

  std::string id = "";
  std::string type = "";      ///< Usually "0" for standard controllers.
  std::uint32_t sequence = 0; ///< Signal-plan sequence index.
};

// =============================================================================
// § 6.3.3 / Code 24  Pedestrian Cross-Path Lane Links
// =============================================================================

/// Sub-element of JunctionCrossPath.
/// Represents <startLaneLink s="..." from="..." to="..."/> or
/// <endLaneLink s="..." from="..." to="..."/>.
/// Maps a sidewalk lane on a connecting road to a lane on the crossing road.
struct JunctionCrossPathLink {
  JunctionCrossPathLink() = default;
  JunctionCrossPathLink(double s, int from, int to);

  double s = 0.0; ///< s-coordinate along the crossing road.
  int from = 0;   ///< Lane ID on the connecting road's sidewalk.
  int to = 0;     ///< Lane ID on the pedestrian crossing road.
};

// =============================================================================
// § 6.3.3 / Code 24  Pedestrian Crossing (Cross-Path)
// =============================================================================

/// Represents a <crossPath> element inside a <junction>.
///
/// Pedestrian crossings inside a junction are modelled as their own roads
/// (e.g. footway/crosswalk roads).  The <crossPath> element acts as the lookup
/// that connects the sidewalk lanes of adjacent connecting roads to those
/// pedestrian crossing roads so that routing and simulation tools can navigate
/// them correctly.
///
/// JSON key mapping (snake_case ↔ camelCase):
///   crossing_road  → crossingRoad
///   road_at_start  → roadAtStart
///   road_at_end    → roadAtEnd
struct JunctionCrossPath {
  JunctionCrossPath() = default;
  JunctionCrossPath(std::string id, std::string crossing_road,
                    std::string road_at_start, std::string road_at_end,
                    JunctionCrossPathLink start_lane_link,
                    JunctionCrossPathLink end_lane_link);

  std::string id = "";            ///< Unique cross-path ID.
  std::string crossing_road = ""; ///< Road ID of the pedestrian crossing.
  std::string road_at_start = ""; ///< Road ID at the start of the crossing.
  std::string road_at_end = "";   ///< Road ID at the end of the crossing.
  JunctionCrossPathLink start_lane_link; ///< Connects start to crossing lane.
  JunctionCrossPathLink end_lane_link;   ///< Connects end to crossing lane.
};

// =============================================================================
// § 6.5 / Code 32  Traffic Island Corner (Local Coordinates)
// =============================================================================

/// Represents a <cornerLocal u="..." v="..." z="..." height="..."/> element.
/// Coordinates are in the junction's local reference frame (metres):
///   u = longitudinal offset from junction origin
///   v = lateral offset from junction origin
///   z = vertical offset (road surface)
///   height = curb/island height above road surface
struct JunctionCornerLocal {
  JunctionCornerLocal() = default;
  JunctionCornerLocal(double u, double v, double z, double height);

  double u = 0.0;      ///< Longitudinal offset [m], junction-local frame.
  double v = 0.0;      ///< Lateral offset [m], junction-local frame.
  double z = 0.0;      ///< Vertical offset from road surface [m].
  double height = 0.0; ///< Curb/island height above road surface [m].
};

// =============================================================================
// § 6.5 / Code 32  Traffic Island Object
// =============================================================================

/// Represents an <object type="trafficIsland"> element inside a junction's
/// <objects> block.
///
/// Traffic islands with complex polygonal geometry are explicitly NOT modelled
/// as lane sections — doing so would break the ASAM invariant that only
/// connecting roads may overlap.  Instead, they are described as objects with
/// a <cornerLocal> outline polygon in the junction's local coordinate system.
///
/// fill_type corresponds to the <outline fillType="..."> attribute.
/// Common values: "concrete", "grass", "asphalt", "cobblestone".
struct JunctionTrafficIsland {
  JunctionTrafficIsland() = default;
  JunctionTrafficIsland(std::string id, std::string name, double height,
                        double z_offset, std::string fill_type);

  std::string id = "";        ///< Unique object ID (global within the map).
  std::string name = "";      ///< Human-readable label, e.g. "Island_South".
  double height = 0.0;        ///< Base object height attribute [m].
  double z_offset = 0.0;      ///< Vertical placement offset [m].
  std::string fill_type = ""; ///< Surface material for simulation / rendering.

  std::vector<JunctionCornerLocal> outline; ///< Ordered polygon vertices.
};

// =============================================================================
// § 6.2  Junction
// =============================================================================

/// Top-level representation of a <junction> element.
///
/// type:
///   "default" — Standard common junction (physical area, ASAM § 6).
///   "virtual" — Virtual junction used to split long roads into segments;
///               no physical area, no connecting roads required.
///
/// Storage conventions (mirror existing odr:: patterns):
///   connections  → id_to_connection  (map for O(log n) lookup by ID)
///   controllers  → id_to_controller  (map)
///   priorities   → priorities        (ordered set, existing API)
///   cross_paths  → cross_paths       (vector; ordering encodes specification)
///   traffic_islands → traffic_islands (vector; ordering encodes specification)
class Junction {
public:
  Junction() = default;
  Junction(std::string name, std::string id, std::string type = "default");

  std::string name = "";
  std::string id = "";
  std::string type = "default"; ///< "default" | "virtual"  (ASAM § 6.2)

  // Standard vehicle routing
  std::map<std::string, JunctionConnection> id_to_connection;
  std::map<std::string, JunctionController> id_to_controller;
  std::set<JunctionPriority> priorities;

  // ASAM Common Junction advanced features
  std::vector<JunctionCrossPath> cross_paths;         ///< § 6.3.3 / Code 24
  std::vector<JunctionTrafficIsland> traffic_islands; ///< § 6.5   / Code 32

  // R-OADG Builder APIs
  /**
   * @brief Adds a standard vehicle connection through the junction.
   * @param id Unique ID for this connection.
   * @param incoming_road ID of the road entering the junction.
   * @param connecting_road ID of the virtual path inside the junction.
   * @param contact_point "Start" or "End" of the connecting road.
   * @return A reference to the newly created connection for chaining or adding
   * lane links.
   */
  JunctionConnection &
  add_connection(std::string id, std::string incoming_road,
                 std::string connecting_road,
                 JunctionConnection::ContactPoint contact_point);

  /**
   * @brief Helper to configure a dedicated slip lane (free-flow right turn,
   * etc). Automatically creates a connection with a 1:1 lane mapping.
   * @param connection_id Unique ID for the slip lane connection.
   * @param incoming_road ID of the approaching road.
   * @param connecting_road ID of the virtual geometry road for the slip lane.
   * @param from_lane The specific lane ID yielding into the slip.
   * @param to_lane The specific lane ID on the connecting slip road.
   */
  void add_slip_lane(std::string connection_id, std::string incoming_road,
                     std::string connecting_road, int from_lane, int to_lane);

  /**
   * @brief Adds structural "furniture" to the junction, such as traffic islands
   * or raised curbs.
   * @param id Unique identifier across the map.
   * @param name Human-readable label (e.g., "North_Island").
   * @param height Height of the furniture above the road surface [m].
   * @param z_offset Vertical placement offset [m].
   * @param fill_type Material mapping (e.g., "concrete", "grass").
   * @param local_outline Set of polygon contour vertices in junction-local
   * coords.
   */
  void add_furniture(std::string id, std::string name, double height,
                     double z_offset, std::string fill_type,
                     const std::vector<JunctionCornerLocal> &local_outline);

  /**
   * @brief Batch helper for routing multiple lanes from an incoming road
   * directly into a connecting road.
   * @param connection_id The connection to populate.
   * @param start_lane Start of the lane ID range (inclusive).
   * @param end_lane End of the lane ID range (inclusive).
   */
  void add_multi_lane_setup(const std::string &connection_id, int start_lane,
                            int end_lane);
};

} // namespace odr