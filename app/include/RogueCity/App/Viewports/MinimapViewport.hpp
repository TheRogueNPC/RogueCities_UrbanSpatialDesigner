#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <imgui.h>

namespace RogueCity::App {

/// Minimap viewport: 2D orthographic top-down view acts as an overveiw for the entire city making proccess. needs to have contextual interactivity and is just as useful as the editor itself, Syncs XY with primary viewport (optional), acts as navigation aid
class MinimapViewport {
public:
    enum class Size { Small, Medium, Large };

    MinimapViewport();
    ~MinimapViewport();

    void initialize();
    void update(float delta_time);
    void render();

    /// Set city output to display (simplified rendering)
    void set_city_output(const Generators::CityGenerator::CityOutput* output);

    /// Camera position (synced or independent)
    ///todo allow for a toggle to set if synced or independant 
    void set_camera_position(const Core::Vec2& xy);
    [[nodiscard]] Core::Vec2 get_camera_xy() const;

    /// Configuration needs to allow for user control
    void set_size(Size size);
    [[nodiscard]] Size get_size() const;

private:
    const Generators::CityGenerator::CityOutput* city_output_{ nullptr };
    Core::Vec2 camera_xy_{ 0.0, 0.0 };
    Size size_{ Size::Medium };
    ImVec2 texture_size_{ 512, 512 };  // Render texture dimensions
};

} // namespace RogueCity::App
