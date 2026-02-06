#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <imgui.h>

namespace RogueCity::App {

/// Primary viewport: 3D/2D hybrid with orbital camera
/// Displays full city with interactive axiom placement
class PrimaryViewport {
public:
    PrimaryViewport();
    ~PrimaryViewport();

    void initialize(void* glfw_window);
    void update(float delta_time);
    void render();

    /// Set city output to display
    void set_city_output(const Generators::CityGenerator::CityOutput* output);

    /// Camera controls
    void set_camera_position(const Core::Vec2& xy, float z);
    [[nodiscard]] Core::Vec2 get_camera_xy() const;
    [[nodiscard]] float get_camera_z() const;

    /// Mouse interaction
    [[nodiscard]] Core::Vec2 screen_to_world(const ImVec2& screen_pos) const;
    [[nodiscard]] ImVec2 world_to_screen(const Core::Vec2& world_pos) const;
    [[nodiscard]] float world_to_screen_scale(float world_distance) const;
    [[nodiscard]] bool is_hovered() const;

private:
    const Generators::CityGenerator::CityOutput* city_output_{ nullptr };
    Core::Vec2 camera_xy_{ 0.0, 0.0 };
    float camera_z_{ 500.0f };
    float zoom_{ 1.0f };
};

} // namespace RogueCity::App
