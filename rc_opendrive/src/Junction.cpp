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

} // namespace odr