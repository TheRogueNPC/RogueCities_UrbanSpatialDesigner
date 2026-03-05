/**
 * @file CitySpec.cpp
 * @brief Implementation file for the CitySpec struct.
 *
 * This file includes the header for the CitySpec struct, which is a header-only
 * Plain Old Data (POD) structure used to represent city specifications within
 * the RogueCity::Core namespace. No implementation is required in this file.
 */

#include "RogueCity/Core/Data/CitySpec.hpp"

#include <algorithm>

namespace RogueCity::Core {

void CitySpec::Validate() {
    // ---- BuildingBudget ----
    buildingBudget.totalBudget = std::max(0.0f, buildingBudget.totalBudget);
    if (buildingBudget.minBuildingsPerDistrict > buildingBudget.maxBuildingsPerDistrict) {
        std::swap(buildingBudget.minBuildingsPerDistrict, buildingBudget.maxBuildingsPerDistrict);
    }

    // ---- ZoningConstraints ----
    auto& z = zoningConstraints;

    z.minLotWidth = std::max(0.0f, z.minLotWidth);
    z.maxLotWidth = std::max(z.minLotWidth, z.maxLotWidth);

    z.minLotDepth = std::max(0.0f, z.minLotDepth);
    z.maxLotDepth = std::max(z.minLotDepth, z.maxLotDepth);

    z.minLotArea = std::max(0.0f, z.minLotArea);
    z.maxLotArea = std::max(z.minLotArea, z.maxLotArea);

    z.frontSetback = std::max(0.0f, z.frontSetback);
    z.sideSetback  = std::max(0.0f, z.sideSetback);
    z.rearSetback  = std::max(0.0f, z.rearSetback);

    z.residentialDensity = std::clamp(z.residentialDensity, 0.0f, 1.0f);
    z.commercialDensity  = std::clamp(z.commercialDensity,  0.0f, 1.0f);
    z.industrialDensity  = std::clamp(z.industrialDensity,  0.0f, 1.0f);
    z.civicDensity       = std::clamp(z.civicDensity,       0.0f, 1.0f);

    // ---- General ----
    roadDensity = std::clamp(roadDensity, 0.0f, 1.0f);
}

} // namespace RogueCity::Core
