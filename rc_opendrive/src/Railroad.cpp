#include "Railroad.h"

namespace odr {

RailroadTrack::RailroadTrack(std::string id, double s, std::string dir)
    : id(id), s(s), dir(dir) {}

RailroadPartner::RailroadPartner(std::string id, std::string name)
    : id(id), name(name) {}

RailroadSwitch::RailroadSwitch(std::string id, std::string name,
                               std::string position)
    : id(id), name(name), position(position) {}

PlatformSegment::PlatformSegment(std::string road_id, double s_start,
                                 double s_end, std::string side)
    : road_id(road_id), s_start(s_start), s_end(s_end), side(side) {}

StationPlatform::StationPlatform(std::string id, std::string name)
    : id(id), name(name) {}

Station::Station(std::string id, std::string name, std::string type)
    : id(id), name(name), type(type) {}

} // namespace odr
