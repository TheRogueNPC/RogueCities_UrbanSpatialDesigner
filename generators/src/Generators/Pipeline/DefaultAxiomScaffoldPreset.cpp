#include "RogueCity/Generators/Pipeline/DefaultAxiomScaffoldPreset.hpp"

#include <algorithm>

namespace RogueCity::Generators {

std::vector<CityGenerator::AxiomInput>
DefaultAxiomScaffoldPreset::Build(const CityGenerator::Config &config) {
  const double world_w = static_cast<double>(std::max(1, config.width));
  const double world_h = static_cast<double>(std::max(1, config.height));
  const double extent = std::min(world_w, world_h);
  const double base_radius = std::clamp(extent * 0.18, 120.0, 480.0);
  const Core::Vec2 center(world_w * 0.5, world_h * 0.5);

  std::vector<CityGenerator::AxiomInput> axioms;
  axioms.reserve(6);

  CityGenerator::AxiomInput core{};
  core.id = 1;
  core.type = CityGenerator::AxiomInput::Type::Grid;
  core.position = center;
  core.radius = base_radius;
  core.decay = 2.0;
  core.theta = 0.0;
  axioms.push_back(core);

  const double axial_distance = base_radius * 1.45;

  CityGenerator::AxiomInput north{};
  north.id = 2;
  north.type = CityGenerator::AxiomInput::Type::Linear;
  north.position = Core::Vec2(center.x, std::max(0.0, center.y - axial_distance));
  north.radius = base_radius * 0.65;
  north.theta = 0.5 * 3.14159265358979323846;
  north.decay = 2.2;
  axioms.push_back(north);

  CityGenerator::AxiomInput south = north;
  south.id = 3;
  south.position = Core::Vec2(center.x, std::min(world_h, center.y + axial_distance));
  south.theta = -0.5 * 3.14159265358979323846;
  axioms.push_back(south);

  CityGenerator::AxiomInput east{};
  east.id = 4;
  east.type = CityGenerator::AxiomInput::Type::Stem;
  east.position = Core::Vec2(std::min(world_w, center.x + axial_distance), center.y);
  east.radius = base_radius * 0.7;
  east.theta = 0.0;
  east.decay = 2.2;
  east.stem_branch_angle = 0.72f;
  axioms.push_back(east);

  CityGenerator::AxiomInput west = east;
  west.id = 5;
  west.position = Core::Vec2(std::max(0.0, center.x - axial_distance), center.y);
  west.theta = 3.14159265358979323846;
  axioms.push_back(west);

  CityGenerator::AxiomInput ring{};
  ring.id = 6;
  ring.type = CityGenerator::AxiomInput::Type::Radial;
  ring.position = center;
  ring.radius = base_radius * 1.85;
  ring.decay = 1.8;
  ring.radial_spokes = 8;
  ring.radial_ring_rotation = 0.25;
  axioms.push_back(ring);

  return axioms;
}

} // namespace RogueCity::Generators
