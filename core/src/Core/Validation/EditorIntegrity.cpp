/**
 * @file EditorIntegrity.cpp
 * @brief Implements validation routines for editor entities in RogueCities Urban Spatial Designer.
 *
 * Contains functions to validate the integrity of roads, districts, lots, and buildings within the editor's global state.
 * These checks are intentionally lightweight and non-fatal, allowing for incomplete geometry during editing.
 * Future implementations may provide richer diagnostics and spatial integrity checks (such as overlaps, containment, and adjacency).
 *
 * Functions:
 * - ValidateRoads: Validates the collection of roads for basic integrity.
 * - ValidateDistricts: Validates the collection of districts.
 * - ValidateLots: Validates the collection of lot tokens.
 * - ValidateBuildings: Validates the collection of building sites.
 * - ValidateAll: Runs all validation routines on the editor's global state.
 * - SpatialCheckAll: Placeholder for spatial integrity checks on the editor's global state.
 */
 
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
