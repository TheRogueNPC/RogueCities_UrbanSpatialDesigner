#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Data/CitySpec.hpp"
#include "RogueCity/Core/Data/TextureSpace.hpp"
#include "RogueCity/Core/Editor/TerrainBrush.hpp"
#include "RogueCity/Core/Editor/TexturePainting.hpp"
#include "RogueCity/Core/Editor/SelectionManager.hpp"
#include "RogueCity/Core/Editor/ViewportIndex.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Core/Util/IndexVector.hpp"
#include "RogueCity/Core/Util/StableIndexVector.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
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

    enum class ValidationSeverity : uint8_t {
        Warning = 0,
        Error,
        Critical
    };

    struct ValidationError {
        ValidationSeverity severity{ ValidationSeverity::Warning };
        VpEntityKind entity_kind{ VpEntityKind::Unknown };
        uint32_t entity_id{ 0 };
        std::string message{};
        Vec2 world_position{};
    };

    struct ValidationOverlayState {
        bool enabled{ true };
        bool show_warnings{ true };
        bool show_labels{ true };
        std::vector<ValidationError> errors{};
    };

    enum class GizmoOperation : uint8_t {
        Translate = 0,
        Rotate,
        Scale
    };

    enum class GizmoMode : uint8_t {
        Local = 0,
        World
    };

    struct GizmoState {
        bool enabled{ true };
        bool visible{ true };
        bool snapping{ false };
        GizmoOperation operation{ GizmoOperation::Translate };
        GizmoMode mode{ GizmoMode::World };
        float translate_snap{ 10.0f };
        float rotate_snap_degrees{ 15.0f };
        float scale_snap{ 0.1f };
    };

    struct EditorLayer {
        uint8_t id{ 0 };
        std::string name{ "Layer" };
        bool visible{ true };
        float opacity{ 1.0f };
        std::array<float, 3> tint{ {1.0f, 1.0f, 1.0f} };
    };

    struct LayerManagerState {
        std::vector<EditorLayer> layers{
            EditorLayer{ 0u, "Ground", true, 1.0f, { {1.0f, 1.0f, 1.0f} } },
            EditorLayer{ 1u, "Elevated", true, 0.92f, { {0.90f, 0.95f, 1.00f} } },
            EditorLayer{ 2u, "Underground", true, 0.85f, { {0.78f, 0.92f, 0.78f} } }
        };
        uint8_t active_layer{ 0u };
        bool dim_inactive{ true };
        bool allow_through_hidden{ false };
    };

    struct DistrictBoundaryEditorState {
        bool enabled{ false };
        bool insert_mode{ false };
        bool delete_mode{ false };
        bool snap_to_grid{ false };
        float snap_size{ 10.0f };
        int selected_vertex{ -1 };
    };

    struct SplineEditorState {
        bool enabled{ false };
        bool preview{ true };
        bool closed{ false };
        int samples_per_segment{ 8 };
        float tension{ 0.5f };
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
        ValidationOverlayState validation_overlay{};
        bool debug_show_tensor_overlay{ false };
        bool debug_show_height_overlay{ false };
        bool debug_show_zone_overlay{ false };
        GizmoState gizmo{};
        LayerManagerState layer_manager{};
        std::unordered_map<uint64_t, uint8_t> entity_layers{};
        DistrictBoundaryEditorState district_boundary_editor{};
        SplineEditorState spline_editor{};
        std::optional<CitySpec> active_city_spec{};
        WorldConstraintField world_constraints{};
        SiteProfile site_profile{};
        std::vector<PlanViolation> plan_violations{};
        bool plan_approved{ true };
        std::unique_ptr<Data::TextureSpace> texture_space{};
        Bounds texture_space_bounds{};
        int texture_space_resolution{ 0 };

        uint64_t frame_counter{ 0 };

        [[nodiscard]] static uint64_t MakeEntityKey(VpEntityKind kind, uint32_t id) {
            return (static_cast<uint64_t>(static_cast<uint8_t>(kind)) << 32ull) | static_cast<uint64_t>(id);
        }

        void SetEntityLayer(VpEntityKind kind, uint32_t id, uint8_t layer_id) {
            entity_layers[MakeEntityKey(kind, id)] = layer_id;
        }

        [[nodiscard]] uint8_t GetEntityLayer(VpEntityKind kind, uint32_t id) const {
            const auto it = entity_layers.find(MakeEntityKey(kind, id));
            if (it == entity_layers.end()) {
                return 0u;
            }
            return it->second;
        }

        [[nodiscard]] const EditorLayer* FindLayer(uint8_t id) const {
            for (const auto& layer : layer_manager.layers) {
                if (layer.id == id) {
                    return &layer;
                }
            }
            return nullptr;
        }

        [[nodiscard]] bool IsLayerVisible(uint8_t id) const {
            const EditorLayer* layer = FindLayer(id);
            return layer == nullptr ? true : layer->visible;
        }

        [[nodiscard]] bool IsEntityVisible(VpEntityKind kind, uint32_t id) const {
            return IsLayerVisible(GetEntityLayer(kind, id));
        }

        void InitializeTextureSpace(const Bounds& bounds, int resolution);
        void initializeTextureSpace(const Bounds& bounds, int resolution) { InitializeTextureSpace(bounds, resolution); }

        [[nodiscard]] bool HasTextureSpace() const {
            return texture_space != nullptr;
        }
        [[nodiscard]] bool hasTextureSpace() const { return HasTextureSpace(); }

        [[nodiscard]] Data::TextureSpace& TextureSpaceRef() {
            return *texture_space;
        }
        [[nodiscard]] Data::TextureSpace& textureSpace() { return TextureSpaceRef(); }

        [[nodiscard]] const Data::TextureSpace& TextureSpaceRef() const {
            return *texture_space;
        }
        [[nodiscard]] const Data::TextureSpace& textureSpace() const { return TextureSpaceRef(); }

        void MarkTextureLayerDirty(Data::TextureLayer layer);
        void MarkTextureLayerDirty(Data::TextureLayer layer, const Data::DirtyRegion& region);
        void markTextureLayerDirty(Data::TextureLayer layer) { MarkTextureLayerDirty(layer); }
        void markTextureLayerDirty(Data::TextureLayer layer, const Data::DirtyRegion& region) { MarkTextureLayerDirty(layer, region); }
        void ClearTextureLayerDirty(Data::TextureLayer layer);
        void clearTextureLayerDirty(Data::TextureLayer layer) { ClearTextureLayerDirty(layer); }
        void MarkAllTextureLayersDirty();
        void markAllTextureLayersDirty() { MarkAllTextureLayersDirty(); }
        void ClearAllTextureLayersDirty();
        void clearAllTextureLayersDirty() { ClearAllTextureLayersDirty(); }
        [[nodiscard]] Data::DirtyRegion TextureLayerDirtyRegion(Data::TextureLayer layer) const {
            if (texture_space == nullptr) {
                return {};
            }
            return texture_space->dirtyRegion(layer);
        }

        [[nodiscard]] bool ApplyTerrainBrush(const TerrainBrush::Stroke& stroke);
        [[nodiscard]] bool applyTerrainBrush(const TerrainBrush::Stroke& stroke) { return ApplyTerrainBrush(stroke); }
        [[nodiscard]] bool ApplyTexturePaint(const TexturePainting::Stroke& stroke);
        [[nodiscard]] bool applyTexturePaint(const TexturePainting::Stroke& stroke) { return ApplyTexturePaint(stroke); }
    };

    GlobalState& GetGlobalState();

} // namespace RogueCity::Core::Editor
