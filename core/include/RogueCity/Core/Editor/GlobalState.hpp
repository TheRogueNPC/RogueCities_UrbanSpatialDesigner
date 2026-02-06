#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Core/Util/IndexVector.hpp"
#include "RogueCity/Core/Util/StableIndexVector.hpp"

#include <cstdint>

namespace RogueCity::Core::Editor {

    struct EditorParameters {
        uint32_t seed{ 1 };
        bool snap_to_grid{ true };
        float snap_size{ 1.0f };
        float viewport_pan_speed{ 1.0f };
    };

    struct Selection {
        fva::Handle<Road> selected_road{};
        fva::Handle<District> selected_district{};
        fva::Handle<LotToken> selected_lot{};
        siv::Handle<BuildingSite> selected_building{};
    };

    struct EditorAxiom {
        enum class Type : uint8_t { Radial, Grid, Delta, GridCorrective };
        enum class DeltaTerminal : uint8_t {
            North,
            South,
            East,
            West,
            NorthEast,
            NorthWest,
            SouthEast,
            SouthWest
        };

        uint32_t id{ 0 };
        Type type{ Type::Grid };
        Vec2 position{};
        double radius{ 250.0 };
        double theta{ 0.0 }; // Grid/GridCorrective
        DeltaTerminal terminal{ DeltaTerminal::North }; // Delta
        double decay{ 2.0 };
        bool is_user_placed{ true };
    };

    struct GlobalState {
        // Editor-facing data (stable handles)
        fva::Container<Road> roads;
        fva::Container<District> districts;
        fva::Container<LotToken> lots;
        fva::Container<EditorAxiom> axioms;

        // High-churn entities (validity checking)
        siv::Vector<BuildingSite> buildings;

        // Internal scratch buffers (do NOT expose directly to UI)
        civ::IndexVector<Vec2> scratch_points;

        EditorParameters params{};
        Selection selection{};

        uint64_t frame_counter{ 0 };
    };

    GlobalState& GetGlobalState();

} // namespace RogueCity::Core::Editor

