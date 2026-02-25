#pragma once
#include "RogueCity/Core/Data/CityTypes.hpp"
#include <cstdint>

namespace RogueCity::Core::Editor {
    struct GlobalState;

    [[nodiscard]] Road* FindRoadMutable(GlobalState& gs, uint32_t id);
    [[nodiscard]] District* FindDistrictMutable(GlobalState& gs, uint32_t id);
    [[nodiscard]] WaterBody* FindWaterMutable(GlobalState& gs, uint32_t id);
    [[nodiscard]] LotToken* FindLotMutable(GlobalState& gs, uint32_t id);
    [[nodiscard]] BuildingSite* FindBuildingMutable(GlobalState& gs, uint32_t id);
}
