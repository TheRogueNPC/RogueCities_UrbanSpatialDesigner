#pragma once

#include <string>
#include <vector>

namespace odr {

/**
 * @brief Sub-element of a railroad switch defining the main track.
 * ASAM OpenDRIVE 1.8 - 15.3.1
 */
struct RailroadTrack {
  RailroadTrack() = default;
  RailroadTrack(std::string id, double s, std::string dir);

  std::string id = "";
  double s = 0.0;
  std::string dir = ""; ///< "+" or "-"
};

/**
 * @brief Partner switch connection.
 * ASAM OpenDRIVE 1.8 - 15.3.1
 */
struct RailroadPartner {
  RailroadPartner() = default;
  RailroadPartner(std::string id, std::string name);

  std::string id = "";
  std::string name = "";
};

/**
 * @brief Represents a railroad switch on a road.
 * ASAM OpenDRIVE 1.8 - 15.3.1
 */
struct RailroadSwitch {
  RailroadSwitch() = default;
  RailroadSwitch(std::string id, std::string name, std::string position);

  std::string id = "";
  std::string name = "";
  std::string position = ""; ///< "dynamic", "static straight", "static turn"

  RailroadTrack main_track;
  RailroadTrack side_track;
  std::vector<RailroadPartner> partners; ///< Multiplicity 0..1 in spec, using
                                         ///< vector for flexibility/simplicity
};

/**
 * @brief Segment of a station platform tying it to a road.
 * ASAM OpenDRIVE 1.8 - 15.4
 */
struct PlatformSegment {
  PlatformSegment() = default;
  PlatformSegment(std::string road_id, double s_start, double s_end,
                  std::string side);

  std::string road_id = "";
  double s_start = 0.0;
  double s_end = 0.0;
  std::string side = ""; ///< "left", "right", "center"
};

/**
 * @brief Railroad station platform.
 * ASAM OpenDRIVE 1.8 - 15.4
 */
struct StationPlatform {
  StationPlatform() = default;
  StationPlatform(std::string id, std::string name);

  std::string id = "";
  std::string name = "";
  std::vector<PlatformSegment> segments;
};

/**
 * @brief Railroad station element.
 * ASAM OpenDRIVE 1.8 - 15.4
 */
struct Station {
  Station() = default;
  Station(std::string id, std::string name, std::string type);

  std::string id = "";
  std::string name = "";
  std::string type = ""; ///< e.g., "small", "medium", "large"
  std::vector<StationPlatform> platforms;
};

} // namespace odr
