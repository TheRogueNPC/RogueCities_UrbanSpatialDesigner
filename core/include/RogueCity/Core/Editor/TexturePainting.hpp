#pragma once

#include "RogueCity/Core/Data/TextureSpace.hpp"

#include <cstdint>

namespace RogueCity::Core::Editor {

    class TexturePainting {
    public:
        enum class Layer : uint8_t {
            Material = 0,
            Zone
        };

        struct Stroke {
            Vec2 world_center{};
            double radius_meters{ 25.0 };
            uint8_t value{ 0u };
            float opacity{ 1.0f };
            Layer layer{ Layer::Zone };
        };

        [[nodiscard]] static bool applyStroke(Data::TextureSpace& texture_space, const Stroke& stroke);
    };

} // namespace RogueCity::Core::Editor

