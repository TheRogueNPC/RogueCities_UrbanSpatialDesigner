#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"

#include <vector>

namespace RogueCity::Generators {

/// Post-generation validator that checks roads/lots against world constraints.
class PlanValidatorGenerator {
public:
    // Configuration structure for the plan validator
    struct Config {
        float max_road_slope_deg{ 28.0f };       // Maximum allowed slope for roads in degrees
        float max_lot_slope_deg{ 22.0f };        // Maximum allowed slope for lots in degrees
        uint8_t max_road_flood_level{ 1u };      // Maximum allowed flood level for roads (0-255)
        uint8_t max_lot_flood_level{ 0u };       // Maximum allowed flood level for lots (0-255)
        float min_soil_strength{ 0.18f };        // Minimum required soil strength
        float max_policy_friction{ 0.80f };      // Maximum allowed policy friction value
    };

    // Input structure passed to the validation function
    struct Input {
        const Core::WorldConstraintField* constraints{ nullptr };   // World constraint field for validation
        const Core::SiteProfile* site_profile{ nullptr };           // Site profile for context
        const fva::Container<Core::Road>* roads{ nullptr };         // Container of roads to validate
        const std::vector<Core::LotToken>* lots{ nullptr };         // Vector of lot tokens to validate
    };

    // Output structure returned by the validation function
    struct Output {
        std::vector<Core::PlanViolation> violations;                // List of detected violations
        bool approved{ true };                                      // Whether the plan is approved or not
        float max_severity{ 0.0f };                                // Maximum severity of violations
    };

    // Validate roads and lots against configuration and input constraints
    [[nodiscard]] Output validate(const Input& input) const;

    // Overloaded version of `validate` that allows specifying a custom configuration
    [[nodiscard]] Output validate(const Input& input, const Config& config) const;
};

} // namespace RogueCity::Generators
