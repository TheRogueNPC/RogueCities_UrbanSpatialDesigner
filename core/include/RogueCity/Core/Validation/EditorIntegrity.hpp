#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Core/Util/StableIndexVector.hpp"

namespace RogueCity::Core::Validation {

    struct IntegrityReport {
        std::vector<PlanViolation> violations{};
        uint32_t warning_count{ 0u };
        uint32_t error_count{ 0u };
    };

    void ValidateRoads(const fva::Container<Road>& roads);
    void ValidateDistricts(const fva::Container<District>& districts);
    void ValidateLots(const fva::Container<LotToken>& lots);
    void ValidateBuildings(const siv::Vector<BuildingSite>& buildings);

    [[nodiscard]] IntegrityReport CollectEntityIntegrityReport(const Editor::GlobalState& gs);
    [[nodiscard]] IntegrityReport CollectSpatialIntegrityReport(const Editor::GlobalState& gs);

    void ValidateAll(Editor::GlobalState& gs);
    void SpatialCheckAll(Editor::GlobalState& gs);

    // Const overloads preserve compatibility for call sites that only need to execute checks.
    void ValidateAll(const Editor::GlobalState& gs);
    void SpatialCheckAll(const Editor::GlobalState& gs);

} // namespace RogueCity::Core::Validation
