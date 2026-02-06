#include "RogueCity/App/Viewports/MinimapViewport.hpp"
#include <cmath>

namespace RogueCity::App {

MinimapViewport::MinimapViewport() = default;
MinimapViewport::~MinimapViewport() = default;

void MinimapViewport::initialize() {
    // Reserved for texture allocation
}

void MinimapViewport::update(float delta_time) {
    (void)delta_time;  // Reserved for smooth pan animations
}

void MinimapViewport::render() {
    // Y2K Cockpit Doctrine: Minimap as co-pilot instrument panel
    ImGui::Begin("Minimap", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize);
    
    const ImVec2 minimap_size = get_size() == Size::Small  ? ImVec2(256, 256) :
                                get_size() == Size::Medium ? ImVec2(512, 512) :
                                                             ImVec2(768, 768);
    
    ImGui::SetWindowSize(minimap_size);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 minimap_pos = ImGui::GetCursorScreenPos();
    
    // Background (dark with Y2K border)
    draw_list->AddRectFilled(
        minimap_pos,
        ImVec2(minimap_pos.x + minimap_size.x, minimap_pos.y + minimap_size.y),
        IM_COL32(10, 15, 25, 255)
    );
    
    // Y2K Warning stripe border (3px, high contrast)
    const ImU32 border_color = IM_COL32(255, 200, 0, 255);
    draw_list->AddRect(
        minimap_pos,
        ImVec2(minimap_pos.x + minimap_size.x, minimap_pos.y + minimap_size.y),
        border_color,
        0.0f,
        0,
        3.0f
    );
    
    // Render city output (simplified 2D top-down)
    if (city_output_) {
        // TODO: Render simplified roads as lines
        // TODO: Render axioms as colored circles
        // For now, show center crosshair
        const float center_x = minimap_pos.x + minimap_size.x * 0.5f;
        const float center_y = minimap_pos.y + minimap_size.y * 0.5f;
        
        draw_list->AddLine(
            ImVec2(center_x - 10, center_y),
            ImVec2(center_x + 10, center_y),
            IM_COL32(0, 255, 0, 200),
            2.0f
        );
        draw_list->AddLine(
            ImVec2(center_x, center_y - 10),
            ImVec2(center_x, center_y + 10),
            IM_COL32(0, 255, 0, 200),
            2.0f
        );
    }
    
    // Camera position indicator (pulsing reticle)
    const float center_x = minimap_pos.x + minimap_size.x * 0.5f;
    const float center_y = minimap_pos.y + minimap_size.y * 0.5f;
    const float pulse = 1.0f + 0.2f * std::sin(ImGui::GetTime() * 2.0f);
    const float reticle_size = 5.0f * pulse;
    
    draw_list->AddCircle(
        ImVec2(center_x, center_y),
        reticle_size,
        IM_COL32(255, 255, 255, 180),
        16,
        2.0f
    );
    
    // Label (Y2K typeface aesthetic)
    ImGui::SetCursorScreenPos(ImVec2(minimap_pos.x + 10, minimap_pos.y + 10));
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "NAV");
    
    // Camera coordinates display (cockpit readout style)
    ImGui::SetCursorScreenPos(ImVec2(
        minimap_pos.x + minimap_size.x - 120,
        minimap_pos.y + minimap_size.y - 30
    ));
    ImGui::TextColored(
        ImVec4(1.0f, 1.0f, 1.0f, 0.7f),
        "X: %.0f Y: %.0f",
        camera_xy_.x,
        camera_xy_.y
    );
    
    ImGui::End();
}

void MinimapViewport::set_city_output(const Generators::CityGenerator::CityOutput* output) {
    city_output_ = output;
}

void MinimapViewport::set_camera_position(const Core::Vec2& xy) {
    camera_xy_ = xy;
}

Core::Vec2 MinimapViewport::get_camera_xy() const {
    return camera_xy_;
}

void MinimapViewport::set_size(Size size) {
    size_ = size;
    
    switch (size) {
        case Size::Small:  texture_size_ = ImVec2(256, 256); break;
        case Size::Medium: texture_size_ = ImVec2(512, 512); break;
        case Size::Large:  texture_size_ = ImVec2(768, 768); break;
    }
}

MinimapViewport::Size MinimapViewport::get_size() const {
    return size_;
}

} // namespace RogueCity::App
