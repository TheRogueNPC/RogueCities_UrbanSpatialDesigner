#include "RogueCity/Core/Validation/EditorIntegrity.hpp"

namespace RogueCity::Core::Validation {

    void ValidateRoads(const fva::Container<Road>& roads)
    {
        for (const Road& r : roads) {
            // Editor may temporarily create roads with incomplete geometry while drawing.
            // Keep this check non-fatal; future implementations can return rich diagnostics.
            (void)r;
        }
    }

    void ValidateDistricts(const fva::Container<District>& districts)
    {
        for (const District& d : districts) {
            (void)d;
        }
    }

    void ValidateLots(const fva::Container<LotToken>& lots)
    {
        for (const LotToken& l : lots) {
            (void)l;
        }
    }

    void ValidateBuildings(const siv::Vector<BuildingSite>& buildings)
    {
        for (const BuildingSite& b : buildings) {
            (void)b;
        }
    }

    void ValidateAll(const Editor::GlobalState& gs)
    {
        ValidateRoads(gs.roads);
        ValidateDistricts(gs.districts);
        ValidateLots(gs.lots);
        ValidateBuildings(gs.buildings);
    }

    void SpatialCheckAll(const Editor::GlobalState& gs)
    {
        // Placeholder: spatial integrity checks (overlaps, containment, adjacency) go here.
        // Kept intentionally lightweight for now.
        (void)gs;
    }

} // namespace RogueCity::Core::Validation
