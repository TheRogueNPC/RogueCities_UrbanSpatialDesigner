#pragma once

#include "RogueOpenDRIVE/OpenDriveMap.h"
#include "RogueOpenDRIVE/Railroad.h"
#include <pugixml.hpp>
#include <string>

namespace odr {

/**
 * @brief Helper to write full ASAM OpenDRIVE 1.8 Railroad spec to XML.
 */
class RailroadXmlWriter {
public:
  static void write_railroad(const std::vector<RailroadSwitch> &switches,
                             pugi::xml_node &road_node) {
    if (switches.empty())
      return;

    pugi::xml_node rr_node = road_node.append_child("railroad");
    for (const auto &sw : switches) {
      pugi::xml_node switch_node = rr_node.append_child("switch");
      switch_node.append_attribute("id").set_value(sw.id.c_str());
      switch_node.append_attribute("name").set_value(sw.name.c_str());
      switch_node.append_attribute("position").set_value(sw.position.c_str());

      if (!sw.main_track.id.empty()) {
        pugi::xml_node main_node = switch_node.append_child("mainTrack");
        main_node.append_attribute("id").set_value(sw.main_track.id.c_str());
        main_node.append_attribute("s").set_value(sw.main_track.s);
        main_node.append_attribute("dir").set_value(sw.main_track.dir.c_str());
      }

      if (!sw.side_track.id.empty()) {
        pugi::xml_node side_node = switch_node.append_child("sideTrack");
        side_node.append_attribute("id").set_value(sw.side_track.id.c_str());
        side_node.append_attribute("s").set_value(sw.side_track.s);
        side_node.append_attribute("dir").set_value(sw.side_track.dir.c_str());
      }

      for (const auto &partner : sw.partners) {
        pugi::xml_node p_node = switch_node.append_child("partner");
        p_node.append_attribute("id").set_value(partner.id.c_str());
        p_node.append_attribute("name").set_value(partner.name.c_str());
      }
    }
  }

  static void write_stations(const std::vector<Station> &stations,
                             pugi::xml_node &root_node) {
    for (const auto &station : stations) {
      pugi::xml_node st_node = root_node.append_child("station");
      st_node.append_attribute("id").set_value(station.id.c_str());
      st_node.append_attribute("name").set_value(station.name.c_str());
      st_node.append_attribute("type").set_value(station.type.c_str());

      for (const auto &platform : station.platforms) {
        pugi::xml_node pl_node = st_node.append_child("platform");
        pl_node.append_attribute("id").set_value(platform.id.c_str());
        pl_node.append_attribute("name").set_value(platform.name.c_str());

        for (const auto &segment : platform.segments) {
          pugi::xml_node seg_node = pl_node.append_child("segment");
          seg_node.append_attribute("roadId").set_value(
              segment.road_id.c_str());
          seg_node.append_attribute("sStart").set_value(segment.s_start);
          seg_node.append_attribute("sEnd").set_value(segment.s_end);
          seg_node.append_attribute("side").set_value(segment.side.c_str());
        }
      }
    }
  }
};

} // namespace odr
