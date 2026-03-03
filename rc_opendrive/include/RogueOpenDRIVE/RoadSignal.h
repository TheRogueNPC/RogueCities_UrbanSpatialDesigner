#pragma once
#include "LaneValidityRecord.h"
#include "Mesh.h"

#include <optional>
#include <string>
#include <vector>

namespace odr {

/**
 * @brief Relationship to another signal.
 * ASAM OpenDRIVE 1.8 - 14.3 Signal dependency
 */
struct RoadSignalDependency {
  RoadSignalDependency() = default;
  RoadSignalDependency(std::string id, std::string type);

  std::string id = "";   ///< ID of the controlling signal.
  std::string type = ""; ///< Type of dependency (e.g. "supplementary").
};

/**
 * @brief Reference to another element (signal or object).
 * ASAM OpenDRIVE 1.8 - 14.4 Signal reference
 */
struct RoadSignalReference {
  RoadSignalReference() = default;
  RoadSignalReference(std::string element_id, std::string element_type,
                      std::string type);

  std::string element_id = "";   ///< ID of the linked element.
  std::string element_type = ""; ///< "signal" or "object".
  std::string type = "";         ///< Nature of the link.
};

/**
 * @brief Reference to a signal on another road.
 * ASAM OpenDRIVE 1.8 - 14.5 Signals that apply to multiple roads
 */
struct SignalReference {
  SignalReference() = default;
  SignalReference(std::string id, double s, double t, std::string orientation);

  std::string id = "";          ///< Unique ID of the referenced signal.
  double s = 0.0;               ///< s-coordinate along the road.
  double t = 0.0;               ///< t-coordinate.
  std::string orientation = ""; ///< "+", "-", "none".
};

/**
 * @brief Physical position in road coordinates (deprecated but in spec).
 * ASAM OpenDRIVE 1.8 - 14.9 Signal positioning
 */
struct RoadSignalPositionRoad {
  std::string road_id = "";
  double s = 0.0;
  double t = 0.0;
  double z_offset = 0.0;
  double h_offset = 0.0;
  double pitch = 0.0;
  double roll = 0.0;
};

/**
 * @brief Physical position in inertial coordinates (deprecated but in spec).
 * ASAM OpenDRIVE 1.8 - 14.9 Signal positioning
 */
struct RoadSignalPositionInertial {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double hdg = 0.0;
  double pitch = 0.0;
  double roll = 0.0;
};

struct RoadSignal {
  RoadSignal() = default;
  RoadSignal(std::string road_id, std::string id, std::string name, double s0,
             double t0, bool is_dynamic, double zOffset, double value,
             double height, double width, double hOffset, double pitch,
             double roll, std::string orientation, std::string country,
             std::string type, std::string subtype, std::string unit,
             std::string text);

  static Mesh3D get_box(const double width, const double length,
                        const double height);

  std::string road_id = "";
  std::string id = "";
  std::string name = "";
  double s0 = 0;
  double t0 = 0;
  bool is_dynamic = false;
  double zOffset = 0;
  double value = 0;
  double height = 0;
  double width = 0;
  double hOffset = 0;
  double pitch = 0;
  double roll = 0;
  std::string orientation = "";
  std::string country = "";
  std::string country_revision = ""; ///< § 14.1 countryRevision
  std::string type = "";
  std::string subtype = "";
  std::string unit = "";
  std::string text = "";

  std::vector<LaneValidityRecord> lane_validities;
  std::vector<RoadSignalDependency> dependencies; ///< § 14.3
  std::vector<RoadSignalReference> references;    ///< § 14.4

  std::optional<RoadSignalPositionRoad> position_road;         ///< § 14.9
  std::optional<RoadSignalPositionInertial> position_inertial; ///< § 14.9

  // R-OADG Builder APIs
  /**
   * @brief Adds a dependency on another signal (e.g., this is a supplementary
   * sign to a main signal).
   * @param dep_id ID of the controlling or main signal.
   * @param dep_type Type of dependency (e.g., "supplementary").
   */
  void add_dependency(std::string dep_id, std::string dep_type);

  /**
   * @brief Adds a reference to a related element (e.g., a pole or another
   * object).
   * @param element_id ID of the linked element.
   * @param element_type Type of the element: "signal" or "object".
   * @param ref_type Nature of the link.
   */
  void add_reference(std::string element_id, std::string element_type,
                     std::string ref_type);
};

} // namespace odr
