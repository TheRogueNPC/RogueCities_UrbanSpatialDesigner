#pragma once

#include "RogueCity/Core/Data/TextureSpace.hpp"

#include <cstdint>

namespace RogueCity::Core::Editor {

    class TerrainBrush {
    public:
        enum class Mode : uint8_t {
            Raise = 0,
            Lower,
            Smooth,
            Flatten
        };

        struct Stroke {
            Vec2 world_center{};
            double radius_meters{ 25.0 };
            float strength{ 1.0f };
            Mode mode{ Mode::Raise };
            float flatten_height{ 0.0f };
            bool use_flatten_height{ false };
        };

        [[nodiscard]] static bool applyStroke(Data::TextureSpace& texture_space, const Stroke& stroke);
    };

} // namespace RogueCity::Core::Editor

