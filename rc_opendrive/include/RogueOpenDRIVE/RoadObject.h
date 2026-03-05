#pragma once
#include "LaneValidityRecord.h"
#include "Math.hpp"
#include "Mesh.h"

#include <string>
#include <vector>

namespace odr {

struct RoadObjectRepeat {
  RoadObjectRepeat() = default;
  RoadObjectRepeat(double s0, double length, double distance, double t_start,
                   double t_end, double width_start, double width_end,
                   double height_start, double height_end,
                   double z_offset_start, double z_offset_end);

  double s0 = 0;
  double length = 0;
  double distance = 0;
  double t_start = 0;
  double t_end = 0;
  double width_start = 0;
  double width_end = 0;
  double height_start = 0;
  double height_end = 0;
  double z_offset_start = 0;
  double z_offset_end = 0;
};

struct RoadObjectCorner {
  enum class Type {
    Local_RelZ, // z relative to road’s reference line
    Local_AbsZ, // absolute z value
    Road
  };

  RoadObjectCorner() = default;
  RoadObjectCorner(int id, Vec3D pt, double height, Type type);

  int id = 0;
  Vec3D pt;
  double height = 0;
  Type type = Type::Road;
};

struct RoadObjectOutline {
  RoadObjectOutline() = default;
  RoadObjectOutline(int id, std::string fill_type, std::string lane_type,
                    bool outer, bool closed);

  int id = 0;
  std::string fill_type = "";
  std::string lane_type = "";
  bool outer = true;
  bool closed = true;

  std::vector<RoadObjectCorner> outline;
};

struct RoadObjectMaterial {
  RoadObjectMaterial() = default;
  RoadObjectMaterial(double friction, double roughness, std::string surface,
                     std::string road_mark_color);

  double friction = 0;
  double roughness = 0;
  std::string surface = "";
  std::string road_mark_color = "";
};

struct RoadObjectParkingSpace {
  enum class Access {
    All,
    Bus,
    Car,
    Electric,
    Handicapped,
    Residents,
    Truck,
    Women
  };

  RoadObjectParkingSpace() = default;
  RoadObjectParkingSpace(Access access, std::string restrictions);

  Access access = Access::All;
  std::string restrictions = "";
};

struct RoadObjectMarkingCornerRef {
  RoadObjectMarkingCornerRef() = default;
  RoadObjectMarkingCornerRef(int id);

  int id = 0;
};

struct RoadObjectMarking {
  enum class Side { Front, Left, Rear, Right };

  enum class Weight { Standard, Bold };

  RoadObjectMarking() = default;
  RoadObjectMarking(Side side, Weight weight, double width, std::string color,
                    double z_offset, double space_length, double line_length,
                    double start_offset, double stop_offset);

  Side side = Side::Front;
  Weight weight = Weight::Standard;
  double width = 0;
  std::string color = "";
  double z_offset = 0;
  double space_length = 0;
  double line_length = 0;
  double start_offset = 0;
  double stop_offset = 0;

  std::vector<RoadObjectMarkingCornerRef> corner_references;
};

struct RoadObjectBorder {
  enum class Type { Concrete, Curb, Paint };

  RoadObjectBorder() = default;
  RoadObjectBorder(double width, Type type, int outline_id,
                   bool use_for_mapping);

  double width = 0;
  Type type = Type::Concrete;
  int outline_id = 0;
  bool use_for_mapping = false;
};

struct RoadObjectSkeletonVertex {
  enum class Type { Local, Road };

  RoadObjectSkeletonVertex() = default;
  RoadObjectSkeletonVertex(Vec3D pt, Type type);

  Vec3D pt;
  Type type = Type::Local;
};

struct RoadObjectSkeleton {
  RoadObjectSkeleton() = default;
  RoadObjectSkeleton(int id);

  int id = 0;
  std::vector<RoadObjectSkeletonVertex> vertices;
};

struct ObjectReference {
  ObjectReference() = default;
  ObjectReference(std::string id, double s, double t, double z_offset,
                  double valid_length, std::string orientation);

  std::string id = "";
  double s = 0;
  double t = 0;
  double z_offset = 0;
  double valid_length = 0;
  std::string orientation = "";
};

struct RoadTunnel {
  enum class Type { Standard, Underpass };

  RoadTunnel() = default;
  RoadTunnel(std::string id, double s, double length, Type type,
             double lighting, double daylight);

  std::string id = "";
  double s = 0;
  double length = 0;
  Type type = Type::Standard;
  double lighting = 0;
  double daylight = 0;

  std::vector<LaneValidityRecord> lane_validities;
};

struct RoadBridge {
  enum class Type { Concrete, Steel, Wood, Brick, Extra, Poly };

  RoadBridge() = default;
  RoadBridge(std::string id, std::string name, double s, double length,
             Type type);

  std::string id = "";
  std::string name = "";
  double s = 0;
  double length = 0;
  Type type = Type::Concrete;

  std::vector<LaneValidityRecord> lane_validities;
};

struct RoadObject {
  RoadObject() = default;
  RoadObject(std::string road_id, std::string id, double s0, double t0,
             double z0, double length, double valid_length, double width,
             double radius, double height, double hdg, double pitch,
             double roll, std::string type, std::string name,
             std::string orientation, std::string subtype, bool is_dynamic);

  static Mesh3D get_cylinder(const double eps, const double radius,
                             const double height);
  static Mesh3D get_box(const double width, const double length,
                        const double height);

  std::string road_id = "";

  std::string id = "";
  std::string type = "";
  std::string name = "";
  std::string orientation = "";
  std::string subtype = "";

  double s0 = 0;
  double t0 = 0;
  double z0 = 0;
  double length = 0;
  double valid_length = 0;
  double width = 0;
  double radius = 0;
  double height = 0;
  double hdg = 0;
  double pitch = 0;
  double roll = 0;
  bool is_dynamic = false;

  std::vector<RoadObjectRepeat> repeats;
  std::vector<RoadObjectOutline> outlines;
  std::vector<LaneValidityRecord> lane_validities;
  std::vector<RoadObjectMaterial> materials;
  std::vector<RoadObjectParkingSpace> parking_spaces;
  std::vector<RoadObjectMarking> markings;
  std::vector<RoadObjectBorder> borders;
  std::vector<RoadObjectSkeleton> skeletons;
};

} // namespace odr