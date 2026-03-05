/**
 * @file include/RogueOpenDRIVE/JunctionXmlWriter.h
 * @brief ASAM OpenDRIVE 1.8 XML writer for Junction elements.
 *
 * Header-only utility that serialises an odr::Junction to a well-formed
 * OpenDRIVE XML string using the pugixml DOM builder already bundled with
 * rc_opendrive.  Building through the DOM (rather than raw stringstream)
 * guarantees correct XML character escaping for all string attributes.
 *
 * Implements the element ordering mandated by the ASAM OpenDRIVE 1.8 schema:
 *   1. <connection>  (§ 6.3.1.3)
 *   2. <crossPath>   (§ 6.3.3 / Code 24)
 *   3. <controller>  (§ 6.3.4 / Code 18)
 *   4. <objects>/<object type="trafficIsland"> (§ 6.5 / Code 32)
 *
 * Usage:
 *   #include "RogueOpenDRIVE/JunctionXmlWriter.h"
 *   std::string xml = odr::WriteJunctionXml(my_junction);
 *
 * The returned string contains only the <junction> subtree, not a full
 * <OpenDRIVE> document.  Callers are responsible for embedding it into their
 * document writer.
 */

#pragma once

#include "Junction.h"
#include "pugixml.hpp"

#include <sstream>
#include <string>

namespace odr {

// ---------------------------------------------------------------------------
// Internal helpers — not part of the public API
// ---------------------------------------------------------------------------

namespace detail {

/// Helper: set a string attribute only when the value is non-empty, avoiding
/// spurious empty-string attributes that would technically violate the schema.
inline void set_attr_if(pugi::xml_node &node, const char *name,
                        const std::string &value) {
  if (!value.empty())
    node.append_attribute(name) = value.c_str();
}

/// Helper: set a double attribute formatted to 8 decimal places, matching
/// the precision used by most ASAM-compliant OpenDRIVE generators.
inline void set_attr_d(pugi::xml_node &node, const char *name, double value) {
  // pugixml will format the double; we round to 8 dp via a sprintf to keep
  // output files comparable across platforms.
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.8f", value);
  node.append_attribute(name) = buf;
}

} // namespace detail

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
 * @brief Serialise a Junction to an ASAM OpenDRIVE 1.8-compliant XML string.
 *
 * @param junction  The junction to serialise.
 * @return          UTF-8 string containing the <junction>…</junction> subtree.
 *
 * Element ordering follows the informative ASAM UML diagram for <junction>:
 *   connections → crossPaths → controllers → objects(trafficIslands)
 */
inline std::string WriteJunctionXml(const Junction &junction) {
  pugi::xml_document doc;

  // -----------------------------------------------------------------------
  // Root <junction> element  (ASAM § 6.2)
  // -----------------------------------------------------------------------
  pugi::xml_node jn = doc.append_child("junction");
  detail::set_attr_if(jn, "name", junction.name);
  jn.append_attribute("id") = junction.id.c_str();
  jn.append_attribute("type") = junction.type.c_str();

  // -----------------------------------------------------------------------
  // 1. <connection> + <laneLink>  (ASAM § 6.3.1.3)
  //
  // Each connection describes one navigable vehicle path through the
  // junction: incoming road → connecting (virtual) road → outgoing road.
  // Lane links explicitly map traffic lanes between the two roads.
  // -----------------------------------------------------------------------
  for (const auto &[conn_id, conn] : junction.id_to_connection) {
    pugi::xml_node cn = jn.append_child("connection");
    cn.append_attribute("id") = conn.id.c_str();
    cn.append_attribute("incomingRoad") = conn.incoming_road.c_str();
    cn.append_attribute("connectingRoad") = conn.connecting_road.c_str();

    // contactPoint: the end of the connecting road that traffic enters.
    const char *cp_str = "start";
    if (conn.contact_point == JunctionConnection::ContactPoint::End)
      cp_str = "end";
    cn.append_attribute("contactPoint") = cp_str;

    // Lane links — one per driving-lane mapping (§ 6.3.1.3, Code 7)
    for (const auto &ll : conn.lane_links) {
      pugi::xml_node ll_node = cn.append_child("laneLink");
      ll_node.append_attribute("from") = ll.from;
      ll_node.append_attribute("to") = ll.to;
    }
  }

  // -----------------------------------------------------------------------
  // 2. <crossPath>  (ASAM § 6.3.3 / Code 24)
  //
  // Pedestrian crossings are modelled as their own roads inside the
  // junction area.  The <crossPath> element is the lookup table that maps
  // the sidewalk lanes of adjacent connecting roads to the crossing road,
  // enabling pedestrian routing and simulation.
  // -----------------------------------------------------------------------
  for (const auto &cp : junction.cross_paths) {
    pugi::xml_node cpn = jn.append_child("crossPath");
    cpn.append_attribute("id") = cp.id.c_str();
    cpn.append_attribute("crossingRoad") = cp.crossing_road.c_str();
    cpn.append_attribute("roadAtStart") = cp.road_at_start.c_str();
    cpn.append_attribute("roadAtEnd") = cp.road_at_end.c_str();

    // Start-side lane mapping
    pugi::xml_node sll = cpn.append_child("startLaneLink");
    detail::set_attr_d(sll, "s", cp.start_lane_link.s);
    sll.append_attribute("from") = cp.start_lane_link.from;
    sll.append_attribute("to") = cp.start_lane_link.to;

    // End-side lane mapping
    pugi::xml_node ell = cpn.append_child("endLaneLink");
    detail::set_attr_d(ell, "s", cp.end_lane_link.s);
    ell.append_attribute("from") = cp.end_lane_link.from;
    ell.append_attribute("to") = cp.end_lane_link.to;
  }

  // -----------------------------------------------------------------------
  // 3. <controller>  (ASAM § 6.3.4 / Code 18)
  //
  // References external traffic-light controller elements, enabling
  // OpenSCENARIO simulation synchronisation with signal-plan cycles.
  // -----------------------------------------------------------------------
  for (const auto &[ctrl_id, ctrl] : junction.id_to_controller) {
    pugi::xml_node ctn = jn.append_child("controller");
    ctn.append_attribute("id") = ctrl.id.c_str();
    detail::set_attr_if(ctn, "type", ctrl.type);
    if (ctrl.sequence > 0)
      ctn.append_attribute("sequence") = ctrl.sequence;
  }

  // -----------------------------------------------------------------------
  // 4. <objects> / <object type="trafficIsland">  (ASAM § 6.5 / Code 32)
  //
  // Complex traffic islands MUST be modelled as objects rather than lane
  // sections — only connecting roads may overlap inside a junction area.
  // Each island is defined by a polygon of <cornerLocal> coordinates in the
  // junction's local reference frame.
  // -----------------------------------------------------------------------
  if (!junction.traffic_islands.empty()) {
    pugi::xml_node objects_node = jn.append_child("objects");

    for (const auto &island : junction.traffic_islands) {
      pugi::xml_node obj = objects_node.append_child("object");
      obj.append_attribute("type") = "trafficIsland";
      obj.append_attribute("id") = island.id.c_str();
      detail::set_attr_if(obj, "name", island.name);
      detail::set_attr_d(obj, "height", island.height);
      detail::set_attr_d(obj, "zOffset", island.z_offset);

      // <outlines> → <outline> → <cornerLocal>
      // Using the v1.45+ <outlines> wrapper for forward compatibility.
      pugi::xml_node outlines = obj.append_child("outlines");
      pugi::xml_node outline = outlines.append_child("outline");
      outline.append_attribute("id") = 0; // single outline per island

      detail::set_attr_if(outline, "fillType", island.fill_type);

      for (const auto &corner : island.outline) {
        pugi::xml_node cn = outline.append_child("cornerLocal");
        detail::set_attr_d(cn, "u", corner.u);
        detail::set_attr_d(cn, "v", corner.v);
        detail::set_attr_d(cn, "z", corner.z);
        detail::set_attr_d(cn, "height", corner.height);
      }
    }
  }

  // -----------------------------------------------------------------------
  // Serialise the DOM to a string via an ostringstream writer.
  // -----------------------------------------------------------------------
  struct OssWriter : pugi::xml_writer {
    std::ostringstream oss;
    void write(const void *data, size_t size) override {
      oss.write(static_cast<const char *>(data),
                static_cast<std::streamsize>(size));
    }
  } writer;

  // Use indent=2 spaces, no XML declaration — callers embed into a larger
  // document and will emit their own header.
  doc.print(writer, "  ", pugi::format_indent | pugi::format_no_declaration);
  return writer.oss.str();
}

} // namespace odr
