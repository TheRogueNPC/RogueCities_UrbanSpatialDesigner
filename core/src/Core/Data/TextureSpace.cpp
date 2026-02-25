/**
 * @class TextureSpace
 * @brief Manages spatial texture data for urban design, including height, material, zone, tensor, and distance layers.
 *
 * The TextureSpace class provides a grid-based representation of spatial data, supporting multiple layers
 * (height, material, zone, tensor, distance) with configurable resolution and bounds. It tracks dirty regions
 * for efficient updates and allows marking, clearing, and querying dirty states per layer.
 *
 * @param world_bounds The spatial bounds of the texture space.
 * @param resolution The resolution (number of grid cells per dimension).
 *
 * @note
 * - Dirty regions are used to optimize updates by tracking modified areas.
 * - Layers are allocated and resized according to the specified resolution.
 *
 * @functions
 * - initialize: Sets up the texture space with bounds and resolution.
 * - markDirty: Marks a layer or region as dirty (modified).
 * - isDirty: Checks if a layer is dirty.
 * - dirtyRegion: Returns the dirty region for a layer.
 * - clearDirty: Clears the dirty state for a layer.
 * - markAllDirty: Marks all layers as dirty.
 * - clearAllDirty: Clears dirty states for all layers.
 * - allocateLayers: Allocates/resizes all texture layers.
 */

 #include "RogueCity/Core/Data/TextureSpace.hpp"

#include <algorithm>

namespace RogueCity::Core::Data {

    TextureSpace::TextureSpace(const Bounds& world_bounds, int resolution) {
        initialize(world_bounds, resolution);
    }

    void TextureSpace::initialize(const Bounds& world_bounds, int resolution) {
        const int safe_resolution = std::max(1, resolution);
        coords_.reconfigure(world_bounds, safe_resolution);
        allocateLayers(safe_resolution);
        markAllDirty();
    }

    void TextureSpace::markDirty(TextureLayer layer) {
        DirtyRegion full{};
        full.min_x = 0;
        full.min_y = 0;
        full.max_x = std::max(0, resolution() - 1);
        full.max_y = std::max(0, resolution() - 1);
        markDirty(layer, full);
    }

    void TextureSpace::markDirty(TextureLayer layer, const DirtyRegion& region) {
        dirty_layers_.set(LayerIndex(layer), true);
        DirtyRegion clamped = region;
        if (clamped.isValid()) {
            const int max_coord = std::max(0, resolution() - 1);
            clamped.min_x = std::clamp(clamped.min_x, 0, max_coord);
            clamped.min_y = std::clamp(clamped.min_y, 0, max_coord);
            clamped.max_x = std::clamp(clamped.max_x, 0, max_coord);
            clamped.max_y = std::clamp(clamped.max_y, 0, max_coord);
            if (clamped.max_x < clamped.min_x || clamped.max_y < clamped.min_y) {
                clamped.clear();
            }
        }
        auto& dirty = dirty_regions_[LayerIndex(layer)];
        dirty.merge(clamped);
    }

    bool TextureSpace::isDirty(TextureLayer layer) const {
        return dirty_layers_.test(LayerIndex(layer));
    }

    DirtyRegion TextureSpace::dirtyRegion(TextureLayer layer) const {
        return dirty_regions_[LayerIndex(layer)];
    }

    void TextureSpace::clearDirty(TextureLayer layer) {
        dirty_layers_.set(LayerIndex(layer), false);
        dirty_regions_[LayerIndex(layer)].clear();
    }

    void TextureSpace::markAllDirty() {
        dirty_layers_.set();
        DirtyRegion full{};
        full.min_x = 0;
        full.min_y = 0;
        full.max_x = std::max(0, resolution() - 1);
        full.max_y = std::max(0, resolution() - 1);
        for (auto& region : dirty_regions_) {
            region = full;
        }
    }

    void TextureSpace::clearAllDirty() {
        dirty_layers_.reset();
        for (auto& region : dirty_regions_) {
            region.clear();
        }
    }

    void TextureSpace::allocateLayers(int resolution) {
        height_.resize(resolution, resolution, 0.0f);
        material_.resize(resolution, resolution, 0u);
        zone_.resize(resolution, resolution, 0u);
        tensor_.resize(resolution, resolution, Vec2{});
        distance_.resize(resolution, resolution, 0.0f);
    }

} // namespace RogueCity::Core::Data
