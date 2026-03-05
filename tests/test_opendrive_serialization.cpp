#include "RogueOpenDRIVE/Serialization/JsonSerialization.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Testing OpenDRIVE serialization..." << std::endl;

    // Test CubicPoly
    {
        odr::CubicPoly poly(1.0, 2.0, 3.0, 4.0, 0.0);
        nlohmann::json j = poly;
        std::cout << "CubicPoly JSON: " << j.dump() << std::endl;

        auto poly2 = j.get<odr::CubicPoly>();
        assert(poly2.a == poly.a);
        assert(poly2.b == poly.b);
        assert(poly2.c == poly.c);
        assert(poly2.d == poly.d);
    }

    // Test RoadLink
    {
        odr::RoadLink link("road1", odr::RoadLink::Type::Road, odr::RoadLink::ContactPoint::Start);
        nlohmann::json j_link = link;
        std::cout << "RoadLink JSON: " << j_link.dump() << std::endl;

        auto link2 = j_link.get<odr::RoadLink>();
        assert(link2.id == link.id);
        assert(link2.type == link.type);
        assert(link2.contact_point == link.contact_point);
    }

    // Test RoadGeometry polymorphism (Line)
    {
        std::unique_ptr<odr::RoadGeometry> line = std::make_unique<odr::Line>(10.0, 100.0, 200.0, 1.5, 50.0);
        nlohmann::json j_line = *line;
        std::cout << "Line JSON: " << j_line.dump() << std::endl;

        std::unique_ptr<odr::RoadGeometry> line2;
        odr::from_json(j_line, line2);
        assert(line2->s0 == line->s0);
        assert(line2->x0 == line->x0);
        assert(line2->y0 == line->y0);
        assert(line2->hdg0 == line->hdg0);
        assert(line2->length == line->length);
        assert(dynamic_cast<odr::Line*>(line2.get()) != nullptr);
    }

    // Test RefLine
    {
        odr::RefLine ref_line(100.0);
        ref_line.s0_to_geometry[0.0] = std::make_unique<odr::Line>(0.0, 0.0, 0.0, 0.0, 100.0);
        
        nlohmann::json j_ref = ref_line;
        std::cout << "RefLine JSON: " << j_ref.dump() << std::endl;

        auto ref_line2 = j_ref.get<odr::RefLine>();
        assert(ref_line2.length == ref_line.length);
        assert(ref_line2.s0_to_geometry.size() == 1);
        assert(ref_line2.s0_to_geometry.count(0.0) == 1);
    }

    // Test Road
    {
        odr::Road road("road1", 100.0, "junc1", "Main St");
        road.predecessor = odr::RoadLink("prev", odr::RoadLink::Type::Road, odr::RoadLink::ContactPoint::End);
        
        nlohmann::json j_road = road;
        std::cout << "Road JSON: " << j_road.dump() << std::endl;

        auto road2 = j_road.get<odr::Road>();
        assert(road2.id == road.id);
        assert(road2.length == road.length);
        assert(road2.junction == road.junction);
        assert(road2.name == road.name);
        assert(road2.predecessor.id == "prev");
    }

    // Test OpenDriveMap
    {
        odr::OpenDriveMap map;
        map.proj4 = "+proj=utm +zone=32 +ellps=WGS84";
        map.id_to_road["road1"] = odr::Road("road1", 100.0, "", "Road 1");
        
        nlohmann::json j_map = map;
        std::cout << "OpenDriveMap JSON (summary): " << j_map.size() << " keys" << std::endl;

        auto map2 = j_map.get<odr::OpenDriveMap>();
        assert(map2.proj4 == map.proj4);
        assert(map2.id_to_road.size() == 1);
        assert(map2.id_to_road.at("road1").id == "road1");
    }

    std::cout << "All serialization tests passed!" << std::endl;
    return 0;
}
