#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace RogueCity::Core::Editor {

    namespace {
        constexpr float kRuntimeBuildableSlopeThresholdDeg = 24.0f;

        [[nodiscard]] bool BoundsMismatch(const Bounds& a, const Bounds& b) {
            return !a.min.equals(b.min, 1e-9) || !a.max.equals(b.max, 1e-9);
        }

        [[nodiscard]] float SampleConstraintHeightClamped(const WorldConstraintField& constraints, int x, int y) {
            const int cx = std::clamp(x, 0, constraints.width - 1);
            const int cy = std::clamp(y, 0, constraints.height - 1);
            return constraints.height_meters[static_cast<size_t>(constraints.toIndex(cx, cy))];
        }

        void SyncConstraintsFromHeightTextureRegion(
            const Data::TextureSpace& texture_space,
            const Data::DirtyRegion& dirty_region,
            WorldConstraintField& constraints) {
            if (!dirty_region.isValid() || !constraints.isValid()) {
                return;
            }

            const auto& coords = texture_space.coordinateSystem();
            const auto& height = texture_space.heightLayer();
            std::unordered_set<int> touched_cells;
            touched_cells.reserve(static_cast<size_t>((dirty_region.max_x - dirty_region.min_x + 1) * (dirty_region.max_y - dirty_region.min_y + 1)));

            for (int y = dirty_region.min_y; y <= dirty_region.max_y; ++y) {
                for (int x = dirty_region.min_x; x <= dirty_region.max_x; ++x) {
                    const Vec2 world = coords.pixelToWorld({ x, y });
                    int gx = 0;
                    int gy = 0;
                    if (!constraints.worldToGrid(world, gx, gy)) {
                        continue;
                    }

                    const int idx = constraints.toIndex(gx, gy);
                    constraints.height_meters[static_cast<size_t>(idx)] = height.at(x, y);
                    touched_cells.insert(idx);
                }
            }

            if (touched_cells.empty()) {
                return;
            }

            std::unordered_set<int> slope_cells;
            slope_cells.reserve(touched_cells.size() * 9u);
            for (const int idx : touched_cells) {
                const int gx = idx % constraints.width;
                const int gy = idx / constraints.width;
                for (int oy = -1; oy <= 1; ++oy) {
                    for (int ox = -1; ox <= 1; ++ox) {
                        const int nx = std::clamp(gx + ox, 0, constraints.width - 1);
                        const int ny = std::clamp(gy + oy, 0, constraints.height - 1);
                        slope_cells.insert(constraints.toIndex(nx, ny));
                    }
                }
            }

            const double denom = std::max(1e-6, 2.0 * constraints.cell_size);
            for (const int idx : slope_cells) {
                const int gx = idx % constraints.width;
                const int gy = idx / constraints.width;
                const float h_l = SampleConstraintHeightClamped(constraints, gx - 1, gy);
                const float h_r = SampleConstraintHeightClamped(constraints, gx + 1, gy);
                const float h_d = SampleConstraintHeightClamped(constraints, gx, gy - 1);
                const float h_u = SampleConstraintHeightClamped(constraints, gx, gy + 1);

                const double dzdx = (static_cast<double>(h_r) - static_cast<double>(h_l)) / denom;
                const double dzdy = (static_cast<double>(h_u) - static_cast<double>(h_d)) / denom;
                const double gradient = std::sqrt(dzdx * dzdx + dzdy * dzdy);
                const float slope = std::clamp(static_cast<float>(std::atan(gradient) * (180.0 / 3.14159265358979323846)), 0.0f, 89.0f);
                const size_t slope_index = static_cast<size_t>(idx);
                constraints.slope_degrees[slope_index] = slope;

                const bool sacred = HasHistoryTag(constraints.history_tags[slope_index], HistoryTag::SacredSite);
                const bool severe_flood = constraints.flood_mask[slope_index] >= 2u;
                const bool slope_blocked = slope > kRuntimeBuildableSlopeThresholdDeg;
                constraints.no_build_mask[slope_index] = (sacred || severe_flood || slope_blocked) ? 1u : 0u;
            }
        }
    } // namespace

    void GlobalState::InitializeTextureSpace(const Bounds& bounds, int resolution) {
        const int safe_resolution = std::max(1, resolution);
        const bool needs_init = (texture_space == nullptr);
        const bool mismatch = (!needs_init) &&
            (texture_space_resolution != safe_resolution || BoundsMismatch(texture_space_bounds, bounds));

        if (!needs_init && !mismatch) {
            return;
        }

        if (texture_space == nullptr) {
            texture_space = std::make_unique<Data::TextureSpace>(bounds, safe_resolution);
        } else {
            texture_space->initialize(bounds, safe_resolution);
        }

        texture_space_bounds = bounds;
        texture_space_resolution = safe_resolution;
        MarkAllTextureLayersDirty();
    }

    void GlobalState::MarkTextureLayerDirty(Data::TextureLayer layer) {
        if (texture_space == nullptr) {
            return;
        }

        Data::DirtyRegion full{};
        full.min_x = 0;
        full.min_y = 0;
        full.max_x = std::max(0, texture_space->resolution() - 1);
        full.max_y = std::max(0, texture_space->resolution() - 1);
        MarkTextureLayerDirty(layer, full);
    }

    void GlobalState::MarkTextureLayerDirty(Data::TextureLayer layer, const Data::DirtyRegion& region) {
        if (texture_space == nullptr) {
            return;
        }

        texture_space->markDirty(layer, region);

        switch (layer) {
            case Data::TextureLayer::Height:
                dirty_layers.MarkDirty(DirtyLayer::Tensor);
                dirty_layers.MarkDirty(DirtyLayer::Roads);
                dirty_layers.MarkDirty(DirtyLayer::Districts);
                dirty_layers.MarkDirty(DirtyLayer::Lots);
                dirty_layers.MarkDirty(DirtyLayer::Buildings);
                break;
            case Data::TextureLayer::Tensor:
                dirty_layers.MarkDirty(DirtyLayer::Tensor);
                dirty_layers.MarkDirty(DirtyLayer::Roads);
                dirty_layers.MarkDirty(DirtyLayer::Districts);
                dirty_layers.MarkDirty(DirtyLayer::Lots);
                dirty_layers.MarkDirty(DirtyLayer::Buildings);
                break;
            case Data::TextureLayer::Material:
            case Data::TextureLayer::Zone:
            case Data::TextureLayer::Distance:
                dirty_layers.MarkDirty(DirtyLayer::Districts);
                dirty_layers.MarkDirty(DirtyLayer::Lots);
                dirty_layers.MarkDirty(DirtyLayer::Buildings);
                break;
            case Data::TextureLayer::Count:
                break;
        }

        dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
    }

    void GlobalState::ClearTextureLayerDirty(Data::TextureLayer layer) {
        if (texture_space == nullptr) {
            return;
        }
        texture_space->clearDirty(layer);
    }

    void GlobalState::MarkAllTextureLayersDirty() {
        if (texture_space == nullptr) {
            return;
        }
        texture_space->markAllDirty();
        dirty_layers.MarkFromAxiomEdit();
    }

    void GlobalState::ClearAllTextureLayersDirty() {
        if (texture_space == nullptr) {
            return;
        }
        texture_space->clearAllDirty();
    }

    bool GlobalState::ApplyTerrainBrush(const TerrainBrush::Stroke& stroke) {
        if (texture_space == nullptr) {
            return false;
        }

        Data::DirtyRegion dirty_region{};
        if (!TerrainBrush::applyStroke(*texture_space, stroke, &dirty_region)) {
            return false;
        }

        if (world_constraints.isValid()) {
            SyncConstraintsFromHeightTextureRegion(*texture_space, dirty_region, world_constraints);
        }
        last_texture_edit_frame = frame_counter;
        MarkTextureLayerDirty(Data::TextureLayer::Height, dirty_region);
        return true;
    }

    bool GlobalState::ApplyTexturePaint(const TexturePainting::Stroke& stroke) {
        if (texture_space == nullptr) {
            return false;
        }

        Data::DirtyRegion dirty_region{};
        if (!TexturePainting::applyStroke(*texture_space, stroke, &dirty_region)) {
            return false;
        }

        const Data::TextureLayer dirty_layer =
            (stroke.layer == TexturePainting::Layer::Zone)
            ? Data::TextureLayer::Zone
            : Data::TextureLayer::Material;
        last_texture_edit_frame = frame_counter;
        MarkTextureLayerDirty(dirty_layer, dirty_region);
        return true;
    }

    void GlobalState::EnsureTextureSpaceUpToDate() {
        if (!texture_space_dirty && HasTextureSpace() && texture_space_resolution == city_texture_size)
            return;
        const Bounds b = ComputeWorldBounds(city_texture_size, city_meters_per_pixel);
        InitializeTextureSpace(b, city_texture_size);
        texture_space_bounds = b;
        texture_space_resolution = city_texture_size;
        texture_space_dirty = false;
        MarkAllTextureLayersDirty();
    }

    GlobalState& GetGlobalState()
    {
        static GlobalState gs{};
        return gs;
    }

} // namespace RogueCity::Core::Editor
