#include "RogueCity/Generators/IO/GeoPackageIO.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationInput.hpp"

#include <cassert>
#include <cstdio>
#include <string>

int main() {
  using RogueCity::Generators::CityGenerator;
  using RogueCity::Generators::GenerationInput;
  using RogueCity::Generators::IO::GeoPackageIO;
  using RogueCity::Generators::IO::GeoPackageImportResult;

  if (!GeoPackageIO::IsSupported()) {
    return 0;
  }

  CityGenerator::Config cfg{};
  cfg.width = 1400;
  cfg.height = 1400;
  cfg.seed = 11u;
  cfg.num_seeds = 14;

  GenerationInput input{};
  input.config = cfg;
  input.deterministic_seed = cfg.seed;

  CityGenerator generator{};
  const auto city = generator.generate(input);
  assert(city.roads.size() > 0u);

  const std::string path = "rc_test_roundtrip.gpkg";
  std::string error;
  assert(GeoPackageIO::ExportCity(path, city, &error));

  GeoPackageImportResult imported{};
  assert(GeoPackageIO::ImportCity(path, imported, &error));
  assert(imported.roads.size() > 0u);
  assert(imported.roads.size() <= city.roads.size());
  assert(imported.spatial_reference.canonicalId() ==
         city.spatial_reference.canonicalId());

  std::remove(path.c_str());
  return 0;
}
