#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace RogueCity::Core {

/// High-level intent for city generation
struct CityIntent {
    std::string description;  // Natural language description
    std::string scale;        // "hamlet", "town", "city", "metro"
    std::string climate;      // "temperate", "tropical", "arid", etc.
    std::vector<std::string> styleTags;  // ["modern", "industrial", "cyberpunk"]
};

/// District hint for generation
struct DistrictHint {
    std::string type;  // "residential", "commercial", "industrial", "downtown"
    float density;     // 0.0 to 1.0
};

/// Building budget allocation per district type
struct BuildingBudget {
    // Per-building cost factors (abstract units)
    float residentialCost = 1.0f;
    float commercialCost = 2.0f;
    float industrialCost = 3.0f;
    float civicCost = 4.0f;
    float luxuryCost = 8.0f;
    
    // Total budget available (abstract units)
    float totalBudget = 1000.0f;
    
    // Min/max buildings per district
    uint32_t minBuildingsPerDistrict = 3;
    uint32_t maxBuildingsPerDistrict = 20;
};

/// Population density configuration
struct PopulationConfig {
    // Residents per building type
    float residentialDensity = 50.0f;    // People per residential building
    float mixedUseDensity = 30.0f;       // Residential component
    float rowhomeDensity = 20.0f;        // Compact housing
    float luxuryDensity = 10.0f;         // Low-density luxury
    
    // Workers per commercial/industrial building
    float commercialWorkers = 25.0f;
    float industrialWorkers = 100.0f;
    float civicWorkers = 40.0f;
    
    // Target population range for validation
    uint32_t minPopulation = 0;          // 0 = no minimum
    uint32_t maxPopulation = 0;          // 0 = no maximum
};

/// Lot placement and subdivision constraints
struct ZoningConstraints {
    // Lot size limits (world units)
    float minLotWidth = 10.0f;
    float maxLotWidth = 50.0f;
    float minLotDepth = 15.0f;
    float maxLotDepth = 60.0f;
    
    // Lot area constraints (square units)
    float minLotArea = 150.0f;
    float maxLotArea = 3000.0f;
    
    // Building setbacks (distance from lot edge)
    float frontSetback = 3.0f;
    float sideSetback = 2.0f;
    float rearSetback = 5.0f;
    
    // Density multipliers per district type
    float residentialDensity = 0.6f;     // 60% lot coverage
    float commercialDensity = 0.8f;      // 80% lot coverage
    float industrialDensity = 0.7f;      // 70% lot coverage
    float civicDensity = 0.5f;           // 50% lot coverage
    
    // Lot subdivision strategy
    bool allowRecursiveSubdivision = true;
    uint32_t maxSubdivisionDepth = 3;
};

/// Complete city specification for generation
struct CitySpec {
    CityIntent intent;
    std::vector<DistrictHint> districts;
    
    // Generation parameters (filled by engine or AI)
    uint32_t seed = 0;
    float roadDensity = 0.5f;
    
    // Budget, population, and zoning constraints (Option A: extend CitySpec)
    BuildingBudget buildingBudget;
    PopulationConfig populationConfig;
    ZoningConstraints zoningConstraints;
};

} // namespace RogueCity::Core
