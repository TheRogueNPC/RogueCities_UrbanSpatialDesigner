#include "RogueCity/Core/Editor/EditorUtils.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include <algorithm>

    /**
     * @brief Finds and returns a mutable pointer to a District object by its ID.
     *
     * Iterates through the list of districts in the provided GlobalState and returns
     * a pointer to the district whose ID matches the given id. If no matching district
     * is found, returns nullptr.
     *
     * @param gs Reference to the GlobalState containing the districts.
     * @param id The unique identifier of the district to find.
     * @return District* Pointer to the matching District object, or nullptr if not found.
     */
     // Explanation: This function allows modification of the found District object by returning a mutable pointer.

namespace RogueCity::Core::Editor {

    Road* FindRoadMutable(GlobalState& gs, uint32_t id) {
        for (auto& road : gs.roads) {
            if (road.id == id) {
                return &road;
            }
        }
        return nullptr;
    }

    District* FindDistrictMutable(GlobalState& gs, uint32_t id) {
        for (auto& district : gs.districts) {
            if (district.id == id) {
                return &district;
            }
        }
        return nullptr;
    }

    WaterBody* FindWaterMutable(GlobalState& gs, uint32_t id) {
        for (auto& water : gs.waterbodies) {
            if (water.id == id) {
                return &water;
            }
        }
        return nullptr;
    }

    LotToken* FindLotMutable(GlobalState& gs, uint32_t id) {
        for (auto& lot : gs.lots) {
            if (lot.id == id) {
                return &lot;
            }
        }
        return nullptr;
    }

    BuildingSite* FindBuildingMutable(GlobalState& gs, uint32_t id) {
        for (auto& building : gs.buildings) {
            if (building.id == id) {
                return &building;
            }
        }
        return nullptr;
    }

} // namespace RogueCity::Core::Editor
