#pragma once

#include <string>

#include "CityModel.h"

// Writes the city data to a JSON file using a minimal manual serializer.
bool export_city_to_json(const CityModel::City &city, const std::string &path);
