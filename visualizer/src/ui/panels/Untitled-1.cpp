/**
 * @file src/OpenDriveGenerator.cpp
 * @brief XML Generator implementation for OpenDRIVE Junctions
 */

#include "OpenDriveJunctions.h"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace RogueCities {
namespace OpenDrive {

class OpenDriveGenerator {
public:
  /**
   * @brief Generates the XML block for a single <junction> element.
   *        Implements ASAM OpenDRIVE 1.8.0 Junction Guidelines.
   */
  std::string GenerateJunctionXML(const Junction &junction) {
    std::stringstream ss;
    // Set precision for coordinates
    ss << std::fixed << std::setprecision(8);

    ss << "<junction name=\"" << junction.name << "\" id=\"" << junction.id
       << "\" type=\"" << junction.type << "\">\n";

    // ---------------------------------------------------------
    // 1. Connections (Standard vehicle paths)
    // ---------------------------------------------------------
    for (const auto &conn : junction.connections) {
      ss << "  <connection id=\"" << conn.id << "\" "
         << "incomingRoad=\"" << conn.incomingRoadId << "\" "
         << "connectingRoad=\"" << conn.connectingRoadId << "\" "
         << "contactPoint=\"" << conn.contactPoint << "\">\n";

      for (const auto &link : conn.laneLinks) {
        ss << "    <laneLink from=\"" << link.from << "\" to=\"" << link.to
           << "\"/>\n";
      }
      ss << "  </connection>\n";
    }

    // ---------------------------------------------------------
    // 2. CrossPaths (Pedestrian Crossings)
    // Ref: ASAM Guideline Code 24 (Page 70)
    // ---------------------------------------------------------
    for (const auto &cp : junction.crossPaths) {
      ss << "  <crossPath id=\"" << cp.id << "\" "
         << "crossingRoad=\"" << cp.crossingRoadId << "\" "
         << "roadAtStart=\"" << cp.roadAtStartId << "\" "
         << "roadAtEnd=\"" << cp.roadAtEndId << "\">\n";

      ss << "    <startLaneLink s=\"" << cp.startLaneLink.s << "\" "
         << "from=\"" << cp.startLaneLink.from << "\" "
         << "to=\"" << cp.startLaneLink.to << "\"/>\n";

      ss << "    <endLaneLink s=\"" << cp.endLaneLink.s << "\" "
         << "from=\"" << cp.endLaneLink.from << "\" "
         << "to=\"" << cp.endLaneLink.to << "\"/>\n";

      ss << "  </crossPath>\n";
    }

    // ---------------------------------------------------------
    // 3. Controllers (Traffic Lights)
    // Ref: ASAM Guideline Code 18 (Page 57)
    // ---------------------------------------------------------
    for (const auto &ctrl : junction.controllers) {
      ss << "  <controller id=\"" << ctrl.id << "\" type=\"" << ctrl.type
         << "\"/>\n";
    }

    // ---------------------------------------------------------
    // 4. Traffic Islands (Objects)
    // Ref: ASAM Guideline Code 32 (Page 116)
    // Note: In common junctions, traffic islands defined by cornerLocal
    // are placed inside <objects> within the <junction> element.
    // ---------------------------------------------------------
    if (!junction.trafficIslands.empty()) {
      ss << "  <objects>\n";
      for (const auto &island : junction.trafficIslands) {
        ss << "    <object type=\"trafficIsland\" id=\"" << island.id
           << "\" name=\"" << island.name << "\" "
           << "height=\"" << island.height << "\" zOffset=\"" << island.zOffset
           << "\">\n";

        ss << "      <outlines>\n";
        // Assuming a single outline for simplicity, but could be a vector in
        // the struct
        ss << "        <outline id=\"0\" fillType=\"" << island.fillType
           << "\">\n";

        for (const auto &corner : island.outline) {
          ss << "          <cornerLocal u=\"" << corner.u << "\" v=\""
             << corner.v << "\" "
             << "z=\"" << corner.z << "\" height=\"" << corner.height
             << "\"/>\n";
        }

        ss << "        </outline>\n";
        ss << "      </outlines>\n";
        ss << "    </object>\n";
      }
      ss << "  </objects>\n";
    }

    ss << "</junction>\n";
    return ss.str();
  }
};

} // namespace OpenDrive
} // namespace RogueCities
