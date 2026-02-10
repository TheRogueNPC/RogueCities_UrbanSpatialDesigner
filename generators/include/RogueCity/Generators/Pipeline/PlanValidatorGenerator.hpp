#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"

#include <vector>

namespace RogueCity::Generators {

/// Post-generation validator that checks roads/lots against world constraints.
class PlanValidatorGenerator {
public:
    struct Config {
        float max_road_slope_deg{ 28.0f };
        float max_lot_slope_deg{ 22.0f };
        uint8_t max_road_flood_level{ 1u };
        uint8_t max_lot_flood_level{ 0u };
        float min_soil_strength{ 0.18f };
        float max_policy_friction{ 0.80f };
    };

    struct Input {
        const Core::WorldConstraintField* constraints{ nullptr };
        const Core::SiteProfile* site_profile{ nullptr };
        const fva::Container<Core::Road>* roads{ nullptr };
        const std::vector<Core::LotToken>* lots{ nullptr };
    };

    struct Output {
        std::vector<Core::PlanViolation> violations;
        bool approved{ true };
        float max_severity{ 0.0f };
    };

    [[nodiscard]] Output validate(const Input& input, const Config& config = Config{}) const;
};

} // namespace RogueCity::Generators
