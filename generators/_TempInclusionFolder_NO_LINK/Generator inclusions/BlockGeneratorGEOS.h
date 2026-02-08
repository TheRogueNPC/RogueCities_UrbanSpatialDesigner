#pragma once

#include <vector>
#include "CityModel.h"
#include "CityParams.h"
#include "DistrictGenerator.h"
#include "BlockGenerator.h"

namespace BlockGeneratorGEOS
{
    // Generate block polygons using GEOS polygonize from road linework.
    // This is the GEOS-only implementation path.
    void generate(const CityParams &params,
                  const CityModel::City &city,
                  const CityModel::UserPlacedInputs &user_inputs,
                  const DistrictGenerator::DistrictField &field,
                  std::vector<CityModel::BlockPolygon> &out_polygons,
                  std::vector<CityModel::Polygon> *out_faces,
                  BlockGenerator::Stats *out_stats = nullptr,
                  const BlockGenerator::Settings &settings = {});

} // namespace BlockGeneratorGEOS
