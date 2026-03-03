/**
 * @file src/Junction.cpp
 * @brief ASAM OpenDRIVE 1.8 Junction type constructors.
 *
 * All struct constructors use member-initialiser lists only — no heap
 * allocation, no virtual dispatch.  Ordering mirrors Junction.h declaration
 * order to simplify future diffs.
 */

#include "Junction.h"

namespace odr {

// ---------------------------------------------------------------------------
// § 6.3.1.3  Lane Link
// ---------------------------------------------------------------------------

JunctionLaneLink::JunctionLaneLink(int from, int to) : from(from), to(to) {}

// ---------------------------------------------------------------------------
// § 6.3.1.3  Connection
// ---------------------------------------------------------------------------

JunctionConnection::JunctionConnection(std::string id,
                                       std::string incoming_road,
                                       std::string connecting_road,
                                       ContactPoint contact_point)
    : id(std::move(id)), incoming_road(std::move(incoming_road)),
      connecting_road(std::move(connecting_road)),
      contact_point(contact_point) {}

// ---------------------------------------------------------------------------
// § 6.3.2  Priority
// ---------------------------------------------------------------------------

JunctionPriority::JunctionPriority(std::string high, std::string low)
    : high(std::move(high)), low(std::move(low)) {}

// ---------------------------------------------------------------------------
// § 6.3.4 / Code 18  Controller
// ---------------------------------------------------------------------------

JunctionController::JunctionController(std::string id, std::string type,
                                       std::uint32_t sequence)
    : id(std::move(id)), type(std::move(type)), sequence(sequence) {}

// ---------------------------------------------------------------------------
// § 6.3.3 / Code 24  Cross-Path Lane Link
// ---------------------------------------------------------------------------

JunctionCrossPathLink::JunctionCrossPathLink(double s, int from, int to)
    : s(s), from(from), to(to) {}

// ---------------------------------------------------------------------------
// § 6.3.3 / Code 24  Cross-Path
// ---------------------------------------------------------------------------

JunctionCrossPath::JunctionCrossPath(std::string id, std::string crossing_road,
                                     std::string road_at_start,
                                     std::string road_at_end,
                                     JunctionCrossPathLink start_lane_link,
                                     JunctionCrossPathLink end_lane_link)
    : id(std::move(id)), crossing_road(std::move(crossing_road)),
      road_at_start(std::move(road_at_start)),
      road_at_end(std::move(road_at_end)), start_lane_link(start_lane_link),
      end_lane_link(end_lane_link) {}

// ---------------------------------------------------------------------------
// § 6.5 / Code 32  Corner Local
// ---------------------------------------------------------------------------

JunctionCornerLocal::JunctionCornerLocal(double u, double v, double z,
                                         double height)
    : u(u), v(v), z(z), height(height) {}

// ---------------------------------------------------------------------------
// § 6.5 / Code 32  Traffic Island
// ---------------------------------------------------------------------------

JunctionTrafficIsland::JunctionTrafficIsland(std::string id, std::string name,
                                             double height, double z_offset,
                                             std::string fill_type)
    : id(std::move(id)), name(std::move(name)), height(height),
      z_offset(z_offset), fill_type(std::move(fill_type)) {}

// ---------------------------------------------------------------------------
// § 6.2  Junction
// ---------------------------------------------------------------------------

Junction::Junction(std::string name, std::string id, std::string type)
    : name(std::move(name)), id(std::move(id)), type(std::move(type)) {}

// ---------------------------------------------------------------------------
// R-OADG Builder APIs
// ---------------------------------------------------------------------------

JunctionConnection &
Junction::add_connection(std::string id, std::string incoming_road,
                         std::string connecting_road,
                         JunctionConnection::ContactPoint contact_point) {
  JunctionConnection conn(id, std::move(incoming_road),
                          std::move(connecting_road), contact_point);
  auto [it, inserted] =
      id_to_connection.insert_or_assign(std::move(id), std::move(conn));
  return it->second;
}

void Junction::add_slip_lane(std::string connection_id,
                             std::string incoming_road,
                             std::string connecting_road, int from_lane,
                             int to_lane) {
  JunctionConnection &conn = add_connection(
      std::move(connection_id), std::move(incoming_road),
      std::move(connecting_road), JunctionConnection::ContactPoint::Start);
  conn.lane_links.insert(JunctionLaneLink(from_lane, to_lane));
}

void Junction::add_furniture(
    std::string id, std::string name, double height, double z_offset,
    std::string fill_type,
    const std::vector<JunctionCornerLocal> &local_outline) {
  JunctionTrafficIsland island(std::move(id), std::move(name), height, z_offset,
                               std::move(fill_type));
  island.outline = local_outline;
  traffic_islands.push_back(std::move(island));
}

void Junction::add_multi_lane_setup(const std::string &connection_id,
                                    int start_lane, int end_lane) {
  auto it = id_to_connection.find(connection_id);
  if (it != id_to_connection.end()) {
    int step = (start_lane <= end_lane) ? 1 : -1;
    int current = start_lane;
    while (true) {
      it->second.lane_links.insert(JunctionLaneLink(current, current));
      if (current == end_lane)
        break;
      current += step;
    }
  }
}

} // namespace odr