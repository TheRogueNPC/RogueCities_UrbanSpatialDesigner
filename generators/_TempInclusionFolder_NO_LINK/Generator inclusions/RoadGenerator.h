#pragma once

#include <vector>

#include "CityModel.h"
#include "CityParams.h"
#include "TensorField.h"
#include "Streamlines.h"
#include "Integrator.h"

namespace RoadGenerator
{

    void generate_roads(const CityParams &params,
                        const TensorField::TensorField &field,
                        const std::vector<CityModel::Polyline> &water,
                        CityModel::City &outCity);

} // namespace RoadGenerator
