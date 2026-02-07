#pragma once
#include <string>
#include <vector>

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

/// Complete city specification for generation
struct CitySpec {
    CityIntent intent;
    std::vector<DistrictHint> districts;
    
    // Generation parameters (filled by engine or AI)
    uint32_t seed = 0;
    float roadDensity = 0.5f;
};

} // namespace RogueCity::Core
