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
        dirty_layers_.set(LayerIndex(layer), true);
    }

    bool TextureSpace::isDirty(TextureLayer layer) const {
        return dirty_layers_.test(LayerIndex(layer));
    }

    void TextureSpace::clearDirty(TextureLayer layer) {
        dirty_layers_.set(LayerIndex(layer), false);
    }

    void TextureSpace::markAllDirty() {
        dirty_layers_.set();
    }

    void TextureSpace::clearAllDirty() {
        dirty_layers_.reset();
    }

    void TextureSpace::allocateLayers(int resolution) {
        height_.resize(resolution, resolution, 0.0f);
        material_.resize(resolution, resolution, 0u);
        zone_.resize(resolution, resolution, 0u);
        tensor_.resize(resolution, resolution, Vec2{});
        distance_.resize(resolution, resolution, 0.0f);
    }

} // namespace RogueCity::Core::Data

