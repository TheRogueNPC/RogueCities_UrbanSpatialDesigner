#pragma once

#include "RogueOpenDRIVE/RoadSignal.h"
#include <pugixml.hpp>
#include <string>

namespace odr {

/**
 * @brief Helper to write full ASAM OpenDRIVE 1.8 Signal spec to XML.
 */
class RoadSignalXmlWriter {
public:
  static void write_road_signal(const RoadSignal &sig,
                                pugi::xml_node &signals_node) {
    pugi::xml_node node = signals_node.append_child("signal");
    node.append_attribute("id").set_value(sig.id.c_str());
    node.append_attribute("name").set_value(sig.name.c_str());
    node.append_attribute("s").set_value(sig.s0);
    node.append_attribute("t").set_value(sig.t0);
    node.append_attribute("dynamic").set_value(sig.is_dynamic ? "yes" : "no");
    node.append_attribute("zOffset").set_value(sig.zOffset);
    node.append_attribute("country").set_value(sig.country.c_str());
    if (!sig.country_revision.empty())
      node.append_attribute("countryRevision")
          .set_value(sig.country_revision.c_str());
    node.append_attribute("type").set_value(sig.type.c_str());
    node.append_attribute("subtype").set_value(sig.subtype.c_str());
    node.append_attribute("value").set_value(sig.value);
    node.append_attribute("unit").set_value(sig.unit.c_str());
    node.append_attribute("height").set_value(sig.height);
    node.append_attribute("width").set_value(sig.width);
    node.append_attribute("hOffset").set_value(sig.hOffset);
    node.append_attribute("pitch").set_value(sig.pitch);
    node.append_attribute("roll").set_value(sig.roll);
    node.append_attribute("orientation").set_value(sig.orientation.c_str());
    node.append_attribute("text").set_value(sig.text.c_str());

    // 14.2 validity
    for (const auto &validity : sig.lane_validities) {
      pugi::xml_node v_node = node.append_child("validity");
      v_node.append_attribute("fromLane").set_value(validity.from_lane);
      v_node.append_attribute("toLane").set_value(validity.to_lane);
    }

    // 14.3 dependency
    for (const auto &dep : sig.dependencies) {
      pugi::xml_node d_node = node.append_child("dependency");
      d_node.append_attribute("id").set_value(dep.id.c_str());
      d_node.append_attribute("type").set_value(dep.type.c_str());
    }

    // 14.4 reference
    for (const auto &ref : sig.references) {
      pugi::xml_node r_node = node.append_child("reference");
      r_node.append_attribute("elementId").set_value(ref.element_id.c_str());
      r_node.append_attribute("elementType")
          .set_value(ref.element_type.c_str());
      r_node.append_attribute("type").set_value(ref.type.c_str());
    }

    // 14.9 positionRoad (deprecated)
    if (sig.position_road) {
      pugi::xml_node p_node = node.append_child("positionRoad");
      p_node.append_attribute("roadId").set_value(
          sig.position_road->road_id.c_str());
      p_node.append_attribute("s").set_value(sig.position_road->s);
      p_node.append_attribute("t").set_value(sig.position_road->t);
      p_node.append_attribute("zOffset").set_value(sig.position_road->z_offset);
      p_node.append_attribute("hOffset").set_value(sig.position_road->h_offset);
      p_node.append_attribute("pitch").set_value(sig.position_road->pitch);
      p_node.append_attribute("roll").set_value(sig.position_road->roll);
    }

    // 14.9 positionInertial (deprecated)
    if (sig.position_inertial) {
      pugi::xml_node p_node = node.append_child("positionInertial");
      p_node.append_attribute("x").set_value(sig.position_inertial->x);
      p_node.append_attribute("y").set_value(sig.position_inertial->y);
      p_node.append_attribute("z").set_value(sig.position_inertial->z);
      p_node.append_attribute("hdg").set_value(sig.position_inertial->hdg);
      p_node.append_attribute("pitch").set_value(sig.position_inertial->pitch);
      p_node.append_attribute("roll").set_value(sig.position_inertial->roll);
    }
  }

  static void write_signal_reference(const SignalReference &ref,
                                     pugi::xml_node &signals_node) {
    pugi::xml_node node = signals_node.append_child("signalReference");
    node.append_attribute("id").set_value(ref.id.c_str());
    node.append_attribute("s").set_value(ref.s);
    node.append_attribute("t").set_value(ref.t);
    node.append_attribute("orientation").set_value(ref.orientation.c_str());
  }
};

} // namespace odr
