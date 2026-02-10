#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Data/CitySpec.hpp"
#include "RogueCity/Core/Editor/SelectionManager.hpp"
#include "RogueCity/Core/Editor/ViewportIndex.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Core/Util/IndexVector.hpp"
#include "RogueCity/Core/Util/StableIndexVector.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

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

    enum class DirtyLayer : uint8_t {
        Axioms = 0,
        Tensor,
        Roads,
        Districts,
        Lots,
        Buildings,
        ViewportIndex,
        Count
    };

    struct DirtyLayerState {
        std::array<bool, static_cast<size_t>(DirtyLayer::Count)> flags{};

        void MarkAllClean() { flags.fill(false); }

        void MarkDirty(DirtyLayer layer) {
            flags[static_cast<size_t>(layer)] = true;
        }

        [[nodiscard]] bool IsDirty(DirtyLayer layer) const {
            return flags[static_cast<size_t>(layer)];
        }

        [[nodiscard]] bool AnyDirty() const {
            for (bool v : flags) {
                if (v) {
                    return true;
                }
            }
            return false;
        }

        // Editing planner intent requires rebuilding downstream pipeline layers.
        void MarkFromAxiomEdit() {
            MarkDirty(DirtyLayer::Axioms);
            MarkDirty(DirtyLayer::Tensor);
            MarkDirty(DirtyLayer::Roads);
            MarkDirty(DirtyLayer::Districts);
            MarkDirty(DirtyLayer::Lots);
            MarkDirty(DirtyLayer::Buildings);
            MarkDirty(DirtyLayer::ViewportIndex);
        }
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
        SelectionManager selection_manager{};
        std::optional<SelectionItem> hovered_entity{};
        std::vector<VpProbeData> viewport_index{};
        DirtyLayerState dirty_layers{};
        std::optional<CitySpec> active_city_spec{};
        WorldConstraintField world_constraints{};
        SiteProfile site_profile{};
        std::vector<PlanViolation> plan_violations{};
        bool plan_approved{ true };

        uint64_t frame_counter{ 0 };
    };

    GlobalState& GetGlobalState();

} // namespace RogueCity::Core::Editor
