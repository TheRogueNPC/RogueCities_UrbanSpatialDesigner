#include "RogueCity/Core/Data/SpatialReference.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationInput.hpp"

#include <cassert>

int main() {
  using RogueCity::Core::Data::SpatialReference;
  using RogueCity::Generators::CityGenerator;
  using RogueCity::Generators::GenerationInput;

  const auto local = SpatialReference::LocalPlanarMeters();
  assert(local.isLocalPlanarMeters());
  assert(local.unitsMeters());
  assert(local.canonicalId() == "LOCAL:RCSD_PLANAR_METERS");

  const auto epsg_3857 = SpatialReference::FromEPSG(3857, "WebMercator");
  assert(epsg_3857.kind() == RogueCity::Core::Data::SpatialReferenceKind::EPSG);
  assert(epsg_3857.canonicalId() == "EPSG:3857");

  CityGenerator::Config cfg{};
  cfg.width = 1200;
  cfg.height = 1200;
  cfg.seed = 123u;

  GenerationInput input{};
  input.config = cfg;

  CityGenerator generator{};
  const auto output = generator.generate(input);
  assert(output.spatial_reference.isLocalPlanarMeters());

  return 0;
}
