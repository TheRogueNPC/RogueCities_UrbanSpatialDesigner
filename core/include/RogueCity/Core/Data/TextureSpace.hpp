#pragma once

#include "RogueCity/Core/Data/CoordinateSystem.hpp"
#include "RogueCity/Core/Data/Texture2D.hpp"

#include <bitset>
#include <cstdint>

namespace RogueCity::Core::Data {

    enum class TextureLayer : uint8_t {
        Height = 0,
        Material,
        Zone,
        Tensor,
        Distance,
        Count
    };

    class TextureSpace {
    public:
        TextureSpace() = default;
        TextureSpace(const Bounds& world_bounds, int resolution);

        void initialize(const Bounds& world_bounds, int resolution);

        [[nodiscard]] const CoordinateSystem& coordinateSystem() const { return coords_; }
        [[nodiscard]] CoordinateSystem& coordinateSystem() { return coords_; }
        [[nodiscard]] int resolution() const { return coords_.resolution(); }
        [[nodiscard]] const Bounds& bounds() const { return coords_.bounds(); }

        [[nodiscard]] const Texture2D<float>& heightLayer() const { return height_; }
        [[nodiscard]] Texture2D<float>& heightLayer() { return height_; }

        [[nodiscard]] const Texture2D<uint8_t>& materialLayer() const { return material_; }
        [[nodiscard]] Texture2D<uint8_t>& materialLayer() { return material_; }

        [[nodiscard]] const Texture2D<uint8_t>& zoneLayer() const { return zone_; }
        [[nodiscard]] Texture2D<uint8_t>& zoneLayer() { return zone_; }

        [[nodiscard]] const Texture2D<Vec2>& tensorLayer() const { return tensor_; }
        [[nodiscard]] Texture2D<Vec2>& tensorLayer() { return tensor_; }

        [[nodiscard]] const Texture2D<float>& distanceLayer() const { return distance_; }
        [[nodiscard]] Texture2D<float>& distanceLayer() { return distance_; }

        void markDirty(TextureLayer layer);
        [[nodiscard]] bool isDirty(TextureLayer layer) const;
        void clearDirty(TextureLayer layer);
        void markAllDirty();
        void clearAllDirty();

    private:
        [[nodiscard]] static constexpr size_t LayerIndex(TextureLayer layer) {
            return static_cast<size_t>(layer);
        }

        void allocateLayers(int resolution);

        CoordinateSystem coords_{};
        Texture2D<float> height_{};
        Texture2D<uint8_t> material_{};
        Texture2D<uint8_t> zone_{};
        Texture2D<Vec2> tensor_{};
        Texture2D<float> distance_{};
        std::bitset<static_cast<size_t>(TextureLayer::Count)> dirty_layers_{};
    };

} // namespace RogueCity::Core::Data

