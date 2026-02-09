#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Data/CitySpec.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Core/Util/IndexVector.hpp"
#include "RogueCity/Core/Util/StableIndexVector.hpp"

#include <cstdint>
#include <optional>

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
        enum class Type : uint8_t {
            Organic = 0,
            Grid = 1,
            Radial = 2,
            Hexagonal = 3,
            Stem = 4,
            LooseGrid = 5,
            Suburban = 6,
            Superblock = 7,
            Linear = 8,
            GridCorrective = 9,
            COUNT = 10
        };

        uint32_t id{ 0 };
        Type type{ Type::Grid };
        Vec2 position{};
        double radius{ 250.0 };
        double theta{ 0.0 }; // Grid/Linear/Stem/GridCorrective
        double decay{ 2.0 };
        bool is_user_placed{ true };
    };

    struct GlobalState {
        // Editor-facing data (stable handles)
        fva::Container<Road> roads;
        fva::Container<District> districts;
        fva::Container<BlockPolygon> blocks;
        fva::Container<LotToken> lots;
        fva::Container<EditorAxiom> axioms;
        fva::Container<WaterBody> waterbodies;  // AI_INTEGRATION_TAG: V1_PASS1_TASK3_WATER_COLLECTION

        // High-churn entities (validity checking)
        siv::Vector<BuildingSite> buildings;

        // Internal scratch buffers (do NOT expose directly to UI)
        civ::IndexVector<Vec2> scratch_points;

        EditorParameters params{};
        CityGenerationParams generation{};
        GenerationStats generation_stats{};
        Selection selection{};
        std::optional<CitySpec> active_city_spec{};

        uint64_t frame_counter{ 0 };
    };

    GlobalState& GetGlobalState();

} // namespace RogueCity::Core::Editor
