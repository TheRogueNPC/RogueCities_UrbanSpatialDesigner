#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Data/SpatialReference.hpp"
#include <string>
#include <vector>


namespace RogueCity::Generators::Import {

class OpenDriveBridge {
public:
  static bool
  parseAndMerge(const std::string &xodr_path,
                std::vector<Core::Road> &out_roads,
                std::vector<Core::IntersectionTemplate> &out_intersections,
                const Core::WorldConstraintField &world_constraints,
                Core::Data::SpatialReference *out_spatial_reference = nullptr);
};

} // namespace RogueCity::Generators::Import
