#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include <cmath>

namespace RogueCity::App {

namespace {
    [[nodiscard]] inline Core::Vec2 rotate(const Core::Vec2& v, float radians) {
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        return Core::Vec2(
            v.x * c - v.y * s,
            v.x * s + v.y * c
        );
    }
}

PrimaryViewport::PrimaryViewport() = default;
PrimaryViewport::~PrimaryViewport() = default;

void PrimaryViewport::initialize(void* glfw_window) {
    (void)glfw_window;  // Reserved for future OpenGL context setup
}

void PrimaryViewport::update(float delta_time) {
    (void)delta_time;  // Reserved for camera smoothing, input handling
}

void PrimaryViewport::render() {
    // Y2K Cockpit Doctrine: Clean, high-contrast viewport frame
    ImGui::Begin("Primary Viewport");
    
    const ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    
    // Viewport background (dark grid pattern for depth cue)
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 viewport_pos = ImGui::GetCursorScreenPos();
    
    // Background fill (deep blue-black)
    draw_list->AddRectFilled(
        viewport_pos,
        ImVec2(viewport_pos.x + viewport_size.x, viewport_pos.y + viewport_size.y),
        IM_COL32(15, 20, 30, 255)
    );
    
    // Grid overlay (subtle, Y2K aesthetic)
    const float grid_spacing = 50.0f * zoom_;
    const ImU32 grid_color = IM_COL32(40, 50, 70, 100);
    
    for (float x = 0; x < viewport_size.x; x += grid_spacing) {
        draw_list->AddLine(
            ImVec2(viewport_pos.x + x, viewport_pos.y),
            ImVec2(viewport_pos.x + x, viewport_pos.y + viewport_size.y),
            grid_color
        );
    }
    
    for (float y = 0; y < viewport_size.y; y += grid_spacing) {
        draw_list->AddLine(
            ImVec2(viewport_pos.x, viewport_pos.y + y),
            ImVec2(viewport_pos.x + viewport_size.x, viewport_pos.y + y),
            grid_color
        );
    }
    
    // Render city output (if available)
    if (city_output_) {
        // TODO: Render roads, districts, lots
        // For now, just show placeholder text
        ImGui::SetCursorScreenPos(ImVec2(
            viewport_pos.x + viewport_size.x * 0.5f - 100,
            viewport_pos.y + viewport_size.y * 0.5f - 10
        ));
        ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "[City Preview Placeholder]");
    }
    
    ImGui::End();
}

void PrimaryViewport::set_city_output(const Generators::CityGenerator::CityOutput* output) {
    city_output_ = output;
}

void PrimaryViewport::set_camera_position(const Core::Vec2& xy, float z) {
    camera_xy_ = xy;
    camera_z_ = z;
    zoom_ = 500.0f / std::max(100.0f, camera_z_);  // Zoom based on camera height
}

Core::Vec2 PrimaryViewport::get_camera_xy() const {
    return camera_xy_;
}

float PrimaryViewport::get_camera_z() const {
    return camera_z_;
}

void PrimaryViewport::set_camera_yaw(float radians) {
    yaw_ = radians;
}

float PrimaryViewport::get_camera_yaw() const {
    return yaw_;
}

Core::Vec2 PrimaryViewport::screen_to_world(const ImVec2& screen_pos) const {
    // Get viewport dimensions
    const ImVec2 viewport_pos = ImGui::GetWindowPos();
    const ImVec2 viewport_size = ImGui::GetWindowSize();
    
    // Convert screen space to normalized viewport space [-0.5, 0.5]
    const float norm_x = (screen_pos.x - viewport_pos.x) / viewport_size.x - 0.5f;
    const float norm_y = (screen_pos.y - viewport_pos.y) / viewport_size.y - 0.5f;
    
    // Apply camera transform (orthographic projection for 2D)
    // World units per screen pixel depends on zoom level
    const float world_scale = camera_z_ / zoom_;
    
    Core::Vec2 view(
        norm_x * viewport_size.x / zoom_,
        norm_y * viewport_size.y / zoom_
    );

    const Core::Vec2 world_offset = rotate(view, yaw_);
    return Core::Vec2(camera_xy_.x + world_offset.x, camera_xy_.y + world_offset.y);
}

ImVec2 PrimaryViewport::world_to_screen(const Core::Vec2& world_pos) const {
    // Get viewport dimensions
    const ImVec2 viewport_pos = ImGui::GetWindowPos();
    const ImVec2 viewport_size = ImGui::GetWindowSize();
    
    // Transform world position relative to camera
    const Core::Vec2 rel(world_pos.x - camera_xy_.x, world_pos.y - camera_xy_.y);
    const Core::Vec2 view = rotate(rel, -yaw_);
    
    // Apply zoom and convert to screen space
    const float screen_x = viewport_pos.x + viewport_size.x * 0.5f + static_cast<float>(view.x) * zoom_;
    const float screen_y = viewport_pos.y + viewport_size.y * 0.5f + static_cast<float>(view.y) * zoom_;
    
    return ImVec2(screen_x, screen_y);
}

float PrimaryViewport::world_to_screen_scale(float world_distance) const {
    return world_distance * zoom_;
}

bool PrimaryViewport::is_hovered() const {
    return ImGui::IsWindowHovered();
}

} // namespace RogueCity::App
