#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/App/Tools/IViewportTool.hpp"
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

    inline void ResolveViewportRect(ImVec2& outPos, ImVec2& outSize) {
        if (ImGui::GetCurrentContext() != nullptr) {
            outPos = ImGui::GetWindowPos();
            outSize = ImGui::GetWindowSize();
            if (outSize.x > 1.0f && outSize.y > 1.0f) {
                return;
            }
        }

        // Test/headless fallback when no active ImGui window is available.
        outPos = ImVec2(0.0f, 0.0f);
        outSize = ImVec2(1280.0f, 720.0f);
    }

    struct RoadRenderStyle {
        ImU32 fill{ IM_COL32(0, 220, 255, 255) };
        ImU32 casing{ IM_COL32(8, 24, 34, 255) };
        float width{ 2.0f };
    };

    [[nodiscard]] RoadRenderStyle ResolveRoadRenderStyle(Core::RoadType type) {
        switch (type) {
            case Core::RoadType::Highway:
                return {IM_COL32(255, 196, 76, 255), IM_COL32(52, 30, 5, 255), 6.0f};
            case Core::RoadType::Arterial:
            case Core::RoadType::Avenue:
            case Core::RoadType::Boulevard:
            case Core::RoadType::M_Major:
                return {IM_COL32(74, 236, 255, 255), IM_COL32(10, 34, 44, 255), 4.6f};
            case Core::RoadType::Street:
            case Core::RoadType::M_Minor:
                return {IM_COL32(94, 178, 255, 240), IM_COL32(8, 28, 42, 255), 3.2f};
            case Core::RoadType::Lane:
            case Core::RoadType::Alleyway:
            case Core::RoadType::CulDeSac:
            case Core::RoadType::Drive:
            case Core::RoadType::Driveway:
            default:
                return {IM_COL32(170, 186, 198, 210), IM_COL32(14, 22, 32, 245), 2.1f};
        }
    }
}

PrimaryViewport::PrimaryViewport() = default;
PrimaryViewport::~PrimaryViewport() = default;

void PrimaryViewport::initialize(void* glfw_window) {
    (void)glfw_window;  // Reserved for future OpenGL context setup
}

void PrimaryViewport::update(float delta_time) {
    if (active_tool_ == nullptr) {
        (void)delta_time;
        return;
    }

    active_tool_->update(delta_time, *this);

    if (!is_hovered()) {
        return;
    }

    const Core::Vec2 world_pos = screen_to_world(ImGui::GetMousePos());
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        active_tool_->on_mouse_down(world_pos);
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        active_tool_->on_mouse_up(world_pos);
    }
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        active_tool_->on_mouse_move(world_pos);
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        active_tool_->on_right_click(world_pos);
    }
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
        // Render roads (two-pass casing + fill with hierarchy style).
        for (const auto& road : city_output_->roads) {
            if (road.points.size() < 2) {
                continue;
            }

            const RoadRenderStyle style = ResolveRoadRenderStyle(road.type);
            ImVec2 prev_screen = world_to_screen(road.points.front());
            for (size_t i = 1; i < road.points.size(); ++i) {
                const ImVec2 curr_screen = world_to_screen(road.points[i]);
                draw_list->AddLine(prev_screen, curr_screen, style.casing, style.width + 2.2f);
                draw_list->AddLine(prev_screen, curr_screen, style.fill, style.width);
                prev_screen = curr_screen;
            }
        }
        
        // Render districts (wireframe polygons)
        const ImU32 district_color = IM_COL32(255, 0, 255, 150);  // Magenta
        for (const auto& district : city_output_->districts) {
            if (district.border.empty()) continue;
            
            std::vector<ImVec2> screen_points;
            screen_points.reserve(district.border.size());
            for (const auto& point : district.border) {
                screen_points.push_back(world_to_screen(point));
            }
            
            // Draw district boundary
            for (size_t i = 0; i < screen_points.size(); ++i) {
                const ImVec2& p1 = screen_points[i];
                const ImVec2& p2 = screen_points[(i + 1) % screen_points.size()];
                draw_list->AddLine(p1, p2, district_color, 2.0f);
            }
            
            // Draw district label at centroid
            if (!screen_points.empty()) {
                ImVec2 centroid = ImVec2(0, 0);
                for (const auto& p : screen_points) {
                    centroid.x += p.x;
                    centroid.y += p.y;
                }
                centroid.x /= screen_points.size();
                centroid.y /= screen_points.size();
                
                const char* district_type = "???";
                switch (district.type) {
                    case Core::DistrictType::Residential: district_type = "RES"; break;
                    case Core::DistrictType::Commercial: district_type = "COM"; break;
                    case Core::DistrictType::Industrial: district_type = "IND"; break;
                    case Core::DistrictType::Mixed: district_type = "MIX"; break;
                    default: break;
                }
                
                draw_list->AddText(centroid, IM_COL32(255, 255, 255, 255), district_type);
            }
        }
        
        // Render lots (smaller subdivisions)
        const ImU32 lot_color = IM_COL32(255, 200, 0, 100);  // Yellow-amber
        for (const auto& lot : city_output_->lots) {
            if (lot.boundary.empty()) continue;
            
            std::vector<ImVec2> screen_points;
            screen_points.reserve(lot.boundary.size());
            for (const auto& point : lot.boundary) {
                screen_points.push_back(world_to_screen(point));
            }
            
            for (size_t i = 0; i < screen_points.size(); ++i) {
                const ImVec2& p1 = screen_points[i];
                const ImVec2& p2 = screen_points[(i + 1) % screen_points.size()];
                draw_list->AddLine(p1, p2, lot_color, 1.0f);
            }
        }
    } else {
        // No city output - show placeholder
        ImGui::SetCursorScreenPos(ImVec2(
            viewport_pos.x + viewport_size.x * 0.5f - 100,
            viewport_pos.y + viewport_size.y * 0.5f - 10
        ));
        ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "[No City Generated]");
    }

    ImGui::End();
}

void PrimaryViewport::set_city_output(const Generators::CityGenerator::CityOutput* output) {
    city_output_ = output;
}

void PrimaryViewport::set_active_tool(IViewportTool* tool) {
    active_tool_ = tool;
}

IViewportTool* PrimaryViewport::active_tool() const {
    return active_tool_;
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
    ImVec2 viewport_pos;
    ImVec2 viewport_size;
    ResolveViewportRect(viewport_pos, viewport_size);
    
    // Convert screen space to normalized viewport space [-0.5, 0.5]
    const float norm_x = (screen_pos.x - viewport_pos.x) / viewport_size.x - 0.5f;
    const float norm_y = (screen_pos.y - viewport_pos.y) / viewport_size.y - 0.5f;
    
    // Apply camera transform (orthographic projection for 2D)
    // World units per screen pixel depends on zoom level
    Core::Vec2 view(
        norm_x * viewport_size.x / zoom_,
        norm_y * viewport_size.y / zoom_
    );

    const Core::Vec2 world_offset = rotate(view, yaw_);
    return Core::Vec2(camera_xy_.x + world_offset.x, camera_xy_.y + world_offset.y);
}

ImVec2 PrimaryViewport::world_to_screen(const Core::Vec2& world_pos) const {
    ImVec2 viewport_pos;
    ImVec2 viewport_size;
    ResolveViewportRect(viewport_pos, viewport_size);
    
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
