#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <algorithm>

namespace RogueCity::Core::Editor {

    namespace {
        [[nodiscard]] bool BoundsMismatch(const Bounds& a, const Bounds& b) {
            return !a.min.equals(b.min, 1e-9) || !a.max.equals(b.max, 1e-9);
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

        texture_space->markDirty(layer);

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

        if (!TerrainBrush::applyStroke(*texture_space, stroke)) {
            return false;
        }

        MarkTextureLayerDirty(Data::TextureLayer::Height);
        return true;
    }

    bool GlobalState::ApplyTexturePaint(const TexturePainting::Stroke& stroke) {
        if (texture_space == nullptr) {
            return false;
        }

        if (!TexturePainting::applyStroke(*texture_space, stroke)) {
            return false;
        }

        const Data::TextureLayer dirty_layer =
            (stroke.layer == TexturePainting::Layer::Zone)
            ? Data::TextureLayer::Zone
            : Data::TextureLayer::Material;
        MarkTextureLayerDirty(dirty_layer);
        return true;
    }

    GlobalState& GetGlobalState()
    {
        static GlobalState gs{};
        return gs;
    }

} // namespace RogueCity::Core::Editor
