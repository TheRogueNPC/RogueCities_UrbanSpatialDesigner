#include "RogueCity/Generators/IO/GeoPackageIO.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationInput.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv) {
  using RogueCity::Generators::CityGenerator;
  using RogueCity::Generators::GenerationInput;
  using RogueCity::Generators::IO::GeoPackageIO;
  using RogueCity::Generators::IO::GeoPackageImportResult;

  if (argc < 3) {
    std::cerr << "Usage:\n"
              << "  rc_city_gpkg_tool export <city.gpkg>\n"
              << "  rc_city_gpkg_tool import <city.gpkg>\n";
    return 1;
  }

  const std::string mode = argv[1];
  const std::string path = argv[2];

  if (mode == "export") {
    CityGenerator::Config cfg{};
    cfg.width = 2000;
    cfg.height = 2000;
    cfg.seed = 1337u;
    cfg.num_seeds = 24;

    GenerationInput input{};
    input.config = cfg;
    input.deterministic_seed = cfg.seed;

    CityGenerator generator{};
    const auto city = generator.generate(input);

    std::string error;
    if (!GeoPackageIO::ExportCity(path, city, &error)) {
      std::cerr << "Export failed: " << error << '\n';
      return 2;
    }

    std::cout << "Exported city to " << path << "\n"
              << "roads=" << city.roads.size() << " districts="
              << city.districts.size() << " lots=" << city.lots.size() << '\n';
    return 0;
  }

  if (mode == "import") {
    GeoPackageImportResult imported{};
    std::string error;
    if (!GeoPackageIO::ImportCity(path, imported, &error)) {
      std::cerr << "Import failed: " << error << '\n';
      return 3;
    }

    std::cout << "Imported city from " << path << "\n"
              << "roads=" << imported.roads.size() << " districts="
              << imported.districts.size() << " lots=" << imported.lots.size()
              << "\ncrs=" << imported.spatial_reference.canonicalId() << '\n';
    return 0;
  }

  std::cerr << "Unknown mode: " << mode << '\n';
  return 1;
}
