#pragma once

#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Core/Types.hpp"

#include <cstdint>

namespace RC_UI::Viewport {

struct SceneFrame {
    const RogueCity::Generators::CityGenerator::CityOutput* output{ nullptr };
    RogueCity::Core::Vec2 camera_xy{};
    float camera_z{ 500.0f };
    float camera_yaw{ 0.0f };
    float viewport_zoom{ 1.0f };
    uint64_t generation_serial{ 0 };
};

[[nodiscard]] SceneFrame BuildSceneFrame(
    const RogueCity::Generators::CityGenerator::CityOutput* output,
    const RogueCity::Core::Vec2& camera_xy,
    float camera_z,
    float camera_yaw,
    uint64_t generation_serial);

} // namespace RC_UI::Viewport
