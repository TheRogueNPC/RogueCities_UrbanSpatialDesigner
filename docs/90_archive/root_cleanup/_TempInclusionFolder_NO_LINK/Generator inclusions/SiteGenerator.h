#pragma once

#include "CityModel.h"
#include "CityParams.h"

namespace SiteGenerator
{
    void generate(const CityParams &params,
                  CityModel::City &city,
                  const CityModel::UserPlacedInputs &user_inputs = {});
}
