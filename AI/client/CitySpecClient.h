#pragma once
#include "RogueCity/Core/Data/CitySpec.hpp"
#include <string>
#include <nlohmann/json.hpp>

namespace RogueCity::AI {

/// Client for generating city specifications using AI
class CitySpecClient {
public:
    /// Generate a city spec from natural language description
    static Core::CitySpec GenerateSpec(
        const std::string& description,
        const std::string& scale = "city"
    );
    
    /// JSON serialization helpers
    static nlohmann::json ToJson(const Core::CitySpec& spec);
    static Core::CitySpec FromJson(const nlohmann::json& j);
};

} // namespace RogueCity::AI
