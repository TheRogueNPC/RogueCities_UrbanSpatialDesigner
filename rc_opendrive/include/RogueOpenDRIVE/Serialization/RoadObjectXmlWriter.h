#pragma once

#include "RogueOpenDRIVE/RoadObject.h"
#include <pugixml.hpp>
#include <string>

namespace odr {

/**
 * @brief Helper to write full ASAM OpenDRIVE 1.8 Object spec to XML.
 */
class RoadObjectXmlWriter {
public:
  static void write_road_object(const RoadObject &obj,
                                pugi::xml_node &objects_node) {
    pugi::xml_node node = objects_node.append_child("object");
    node.append_attribute("id").set_value(obj.id.c_str());
    node.append_attribute("type").set_value(obj.type.c_str());
    node.append_attribute("name").set_value(obj.name.c_str());
    node.append_attribute("s").set_value(obj.s0);
    node.append_attribute("t").set_value(obj.t0);
    node.append_attribute("zOffset").set_value(obj.z0);
    node.append_attribute("length").set_value(obj.length);
    node.append_attribute("width").set_value(obj.width);
    node.append_attribute("height").set_value(obj.height);
    node.append_attribute("hdg").set_value(obj.hdg);
    node.append_attribute("pitch").set_value(obj.pitch);
    node.append_attribute("roll").set_value(obj.roll);
    node.append_attribute("orientation").set_value(obj.orientation.c_str());
    if (!obj.subtype.empty())
      node.append_attribute("subtype").set_value(obj.subtype.c_str());
    if (obj.is_dynamic)
      node.append_attribute("dynamic").set_value("yes");

    // 13.2 repeat
    for (const auto &repeat : obj.repeats) {
      pugi::xml_node r_node = node.append_child("repeat");
      r_node.append_attribute("s").set_value(repeat.s0);
      r_node.append_attribute("length").set_value(repeat.length);
      r_node.append_attribute("distance").set_value(repeat.distance);
      r_node.append_attribute("tStart").set_value(repeat.t_start);
      r_node.append_attribute("tEnd").set_value(repeat.t_end);
      r_node.append_attribute("widthStart").set_value(repeat.width_start);
      r_node.append_attribute("widthEnd").set_value(repeat.width_end);
      r_node.append_attribute("heightStart").set_value(repeat.height_start);
      r_node.append_attribute("heightEnd").set_value(repeat.height_end);
      r_node.append_attribute("zOffsetStart").set_value(repeat.z_offset_start);
      r_node.append_attribute("zOffsetEnd").set_value(repeat.z_offset_end);
    }

    // 13.3 outline (v1.45+ multi-outline)
    if (!obj.outlines.empty()) {
      pugi::xml_node outlines_node = node.append_child("outlines");
      for (const auto &outline : obj.outlines) {
        pugi::xml_node o_node = outlines_node.append_child("outline");
        if (outline.id != -1)
          o_node.append_attribute("id").set_value(outline.id);
        o_node.append_attribute("fillType")
            .set_value(outline.fill_type.c_str());
        o_node.append_attribute("laneType")
            .set_value(outline.lane_type.c_str());
        o_node.append_attribute("outer").set_value(outline.outer);
        o_node.append_attribute("closed").set_value(outline.closed);

        for (const auto &corner : outline.outline) {
          if (corner.type == RoadObjectCorner::Type::Road) {
            pugi::xml_node c_node = o_node.append_child("cornerRoad");
            c_node.append_attribute("s").set_value(corner.pt.x);
            c_node.append_attribute("t").set_value(corner.pt.y);
            c_node.append_attribute("dz").set_value(corner.pt.z);
            if (corner.height != 0)
              c_node.append_attribute("height").set_value(corner.height);
          } else {
            pugi::xml_node c_node = o_node.append_child("cornerLocal");
            c_node.append_attribute("u").set_value(corner.pt.x);
            c_node.append_attribute("v").set_value(corner.pt.y);
            c_node.append_attribute("z").set_value(corner.pt.z);
            if (corner.height != 0)
              c_node.append_attribute("height").set_value(corner.height);
          }
        }
      }
    }

    // 13.4 skeleton
    for (const auto &skeleton : obj.skeletons) {
      pugi::xml_node skel_node = node.append_child("skeleton");
      skel_node.append_attribute("id").set_value(skeleton.id);
      pugi::xml_node poly_node = skel_node.append_child("polyline");
      for (const auto &vertex : skeleton.vertices) {
        if (vertex.type == RoadObjectSkeletonVertex::Type::Local) {
          pugi::xml_node v_node = poly_node.append_child("vertexLocal");
          v_node.append_attribute("u").set_value(vertex.pt.x);
          v_node.append_attribute("v").set_value(vertex.pt.y);
          v_node.append_attribute("z").set_value(vertex.pt.z);
        } else {
          pugi::xml_node v_node = poly_node.append_child("vertexRoad");
          v_node.append_attribute("s").set_value(vertex.pt.x);
          v_node.append_attribute("t").set_value(vertex.pt.y);
          v_node.append_attribute("dz").set_value(vertex.pt.z);
        }
      }
    }

    // 13.5 material
    for (const auto &material : obj.materials) {
      pugi::xml_node mat_node = node.append_child("material");
      mat_node.append_attribute("friction").set_value(material.friction);
      mat_node.append_attribute("roughness").set_value(material.roughness);
      mat_node.append_attribute("surface").set_value(material.surface.c_str());
      mat_node.append_attribute("roadMarkColor")
          .set_value(material.road_mark_color.c_str());
    }

    // 13.8 markings
    if (!obj.markings.empty()) {
      pugi::xml_node markings_node = node.append_child("markings");
      for (const auto &marking : obj.markings) {
        pugi::xml_node m_node = markings_node.append_child("marking");
        std::string side = "front";
        if (marking.side == RoadObjectMarking::Side::Left)
          side = "left";
        else if (marking.side == RoadObjectMarking::Side::Rear)
          side = "rear";
        else if (marking.side == RoadObjectMarking::Side::Right)
          side = "right";
        m_node.append_attribute("side").set_value(side.c_str());
        m_node.append_attribute("weight").set_value(
            marking.weight == RoadObjectMarking::Weight::Bold ? "bold"
                                                              : "standard");
        m_node.append_attribute("width").set_value(marking.width);
        m_node.append_attribute("color").set_value(marking.color.c_str());
        m_node.append_attribute("zOffset").set_value(marking.z_offset);
        m_node.append_attribute("spaceLength").set_value(marking.space_length);
        m_node.append_attribute("lineLength").set_value(marking.line_length);
        m_node.append_attribute("startOffset").set_value(marking.start_offset);
        m_node.append_attribute("stopOffset").set_value(marking.stop_offset);

        for (const auto &ref : marking.corner_references) {
          m_node.append_child("cornerReference")
              .append_attribute("id")
              .set_value(ref.id);
        }
      }
    }

    // 13.9 border
    for (const auto &border : obj.borders) {
      pugi::xml_node b_node = node.append_child("border");
      b_node.append_attribute("width").set_value(border.width);
      std::string type = "concrete";
      if (border.type == RoadObjectBorder::Type::Curb)
        type = "curb";
      else if (border.type == RoadObjectBorder::Type::Paint)
        type = "paint";
      b_node.append_attribute("type").set_value(type.c_str());
      b_node.append_attribute("outlineId").set_value(border.outline_id);
      b_node.append_attribute("useForMapping")
          .set_value(border.use_for_mapping);
    }

    // 13.7 parkingSpace
    for (const auto &parking : obj.parking_spaces) {
      pugi::xml_node p_node = node.append_child("parkingSpace");
      std::string access = "all";
      switch (parking.access) {
      case RoadObjectParkingSpace::Access::Bus:
        access = "bus";
        break;
      case RoadObjectParkingSpace::Access::Car:
        access = "car";
        break;
      case RoadObjectParkingSpace::Access::Electric:
        access = "electric";
        break;
      case RoadObjectParkingSpace::Access::Handicapped:
        access = "handicapped";
        break;
      case RoadObjectParkingSpace::Access::Residents:
        access = "residents";
        break;
      case RoadObjectParkingSpace::Access::Truck:
        access = "truck";
        break;
      case RoadObjectParkingSpace::Access::Women:
        access = "women";
        break;
      default:
        access = "all";
        break;
      }
      p_node.append_attribute("access").set_value(access.c_str());
      p_node.append_attribute("restrictions")
          .set_value(parking.restrictions.c_str());
    }

    // 13.14 validity
    for (const auto &validity : obj.lane_validities) {
      pugi::xml_node v_node = node.append_child("validity");
      v_node.append_attribute("fromLane").set_value(validity.from_lane);
      v_node.append_attribute("toLane").set_value(validity.to_lane);
    }
  }

  static void write_object_reference(const ObjectReference &ref,
                                     pugi::xml_node &objects_node) {
    pugi::xml_node node = objects_node.append_child("objectReference");
    node.append_attribute("s").set_value(ref.s);
    node.append_attribute("t").set_value(ref.t);
    node.append_attribute("id").set_value(ref.id.c_str());
    node.append_attribute("zOffset").set_value(ref.z_offset);
    node.append_attribute("validLength").set_value(ref.valid_length);
    node.append_attribute("orientation").set_value(ref.orientation.c_str());
  }

  static void write_tunnel(const RoadTunnel &tunnel,
                           pugi::xml_node &road_node) {
    pugi::xml_node node = road_node.append_child("tunnel");
    node.append_attribute("s").set_value(tunnel.s);
    node.append_attribute("length").set_value(tunnel.length);
    node.append_attribute("id").set_value(tunnel.id.c_str());
    node.append_attribute("type").set_value(
        tunnel.type == RoadTunnel::Type::Underpass ? "underpass" : "standard");
    node.append_attribute("lighting").set_value(tunnel.lighting);
    node.append_attribute("daylight").set_value(tunnel.daylight);

    for (const auto &validity : tunnel.lane_validities) {
      pugi::xml_node v_node = node.append_child("validity");
      v_node.append_attribute("fromLane").set_value(validity.from_lane);
      v_node.append_attribute("toLane").set_value(validity.to_lane);
    }
  }

  static void write_bridge(const RoadBridge &bridge,
                           pugi::xml_node &road_node) {
    pugi::xml_node node = road_node.append_child("bridge");
    node.append_attribute("s").set_value(bridge.s);
    node.append_attribute("length").set_value(bridge.length);
    node.append_attribute("id").set_value(bridge.id.c_str());
    node.append_attribute("name").set_value(bridge.name.c_str());

    std::string type = "concrete";
    switch (bridge.type) {
    case RoadBridge::Type::Steel:
      type = "steel";
      break;
    case RoadBridge::Type::Wood:
      type = "wood";
      break;
    case RoadBridge::Type::Brick:
      type = "brick";
      break;
    case RoadBridge::Type::Extra:
      type = "extra";
      break;
    case RoadBridge::Type::Poly:
      type = "poly";
      break;
    default:
      type = "concrete";
      break;
    }
    node.append_attribute("type").set_value(type.c_str());

    for (const auto &validity : bridge.lane_validities) {
      pugi::xml_node v_node = node.append_child("validity");
      v_node.append_attribute("fromLane").set_value(validity.from_lane);
      v_node.append_attribute("toLane").set_value(validity.to_lane);
    }
  }
};

} // namespace odr
