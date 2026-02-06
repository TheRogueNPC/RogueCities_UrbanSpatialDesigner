#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Core/Util/StableIndexVector.hpp"

namespace RogueCity::Core::Validation {

    void ValidateRoads(const fva::Container<Road>& roads);
    void ValidateDistricts(const fva::Container<District>& districts);
    void ValidateLots(const fva::Container<LotToken>& lots);
    void ValidateBuildings(const siv::Vector<BuildingSite>& buildings);

    void ValidateAll(const Editor::GlobalState& gs);
    void SpatialCheckAll(const Editor::GlobalState& gs);

} // namespace RogueCity::Core::Validation

