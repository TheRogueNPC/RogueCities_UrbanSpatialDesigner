#pragma once

#include "RogueCity/Core/Data/SpatialReference.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <string>
#include <vector>

namespace RogueCity::Generators::IO {

struct GeoPackageImportResult {
  std::vector<Core::Road> roads{};
  std::vector<Core::District> districts{};
  std::vector<Core::LotToken> lots{};
  Core::Data::SpatialReference spatial_reference{
      Core::Data::SpatialReference::LocalPlanarMeters()};
};

class GeoPackageIO {
public:
  [[nodiscard]] static bool IsSupported();

  [[nodiscard]] static bool ExportCity(const std::string &gpkg_path,
                                       const CityGenerator::CityOutput &city,
                                       std::string *error = nullptr);

  [[nodiscard]] static bool ImportCity(const std::string &gpkg_path,
                                       GeoPackageImportResult &out,
                                       std::string *error = nullptr);
};

} // namespace RogueCity::Generators::IO
