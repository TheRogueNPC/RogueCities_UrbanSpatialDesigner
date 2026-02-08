#pragma once

#include <vector>

#include "AxiomInput.h"
#include "CityModel.h"
#include "CityParams.h"
#include "DistrictGenerator.h"
#include "TensorField.h"

namespace LotGenerator
{
    void generate(const CityParams &params,
                  const std::vector<AxiomInput> &axioms,
                  const std::vector<CityModel::District> &districts,
                  const DistrictGenerator::DistrictField &field,
                  const TensorField::TensorField &tensor_field,
                  CityModel::City &city,
                  const CityModel::UserPlacedInputs &user_inputs = {});
} // namespace LotGenerator
