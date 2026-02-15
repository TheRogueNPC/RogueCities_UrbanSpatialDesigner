#include "RogueCity/App/Viewports/MinimapViewport.hpp"
#include <algorithm>
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
        Core::Bounds world_bounds{};
        bool has_bounds = false;
        if (city_output_->world_constraints.isValid()) {
            world_bounds.min = Core::Vec2(0.0, 0.0);
            world_bounds.max = Core::Vec2(
                static_cast<double>(city_output_->world_constraints.width) * city_output_->world_constraints.cell_size,
                static_cast<double>(city_output_->world_constraints.height) * city_output_->world_constraints.cell_size);
            has_bounds = true;
        }

        if (!has_bounds) {
            for (const auto& road : city_output_->roads) {
                for (const auto& p : road.points) {
                    if (!has_bounds) {
                        world_bounds.min = p;
                        world_bounds.max = p;
                        has_bounds = true;
                    } else {
                        world_bounds.min.x = std::min(world_bounds.min.x, p.x);
                        world_bounds.min.y = std::min(world_bounds.min.y, p.y);
                        world_bounds.max.x = std::max(world_bounds.max.x, p.x);
                        world_bounds.max.y = std::max(world_bounds.max.y, p.y);
                    }
                }
            }
        }

        if (has_bounds && world_bounds.width() > 1e-6 && world_bounds.height() > 1e-6) {
            const auto world_to_screen = [&](const Core::Vec2& p) {
                const float u = static_cast<float>((p.x - world_bounds.min.x) / world_bounds.width());
                const float v = static_cast<float>((p.y - world_bounds.min.y) / world_bounds.height());
                return ImVec2(
                    minimap_pos.x + u * minimap_size.x,
                    minimap_pos.y + v * minimap_size.y);
            };

            for (const auto& road : city_output_->roads) {
                if (road.points.size() < 2) {
                    continue;
                }
                for (size_t i = 1; i < road.points.size(); ++i) {
                    draw_list->AddLine(
                        world_to_screen(road.points[i - 1]),
                        world_to_screen(road.points[i]),
                        IM_COL32(80, 220, 255, 180),
                        1.25f);
                }
            }
        } else {
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
    }
    
    // Camera position indicator (pulsing reticle)
    const float center_x = minimap_pos.x + minimap_size.x * 0.5f;
    const float center_y = minimap_pos.y + minimap_size.y * 0.5f;
    const float t = static_cast<float>(ImGui::GetTime());
    const float pulse = 1.0f + 0.2f * std::sin(t * 2.0f);
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
