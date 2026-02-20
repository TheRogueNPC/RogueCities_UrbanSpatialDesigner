#include "RogueCity/App/Tools/WaterTool.hpp"

namespace RogueCity::App {

const char* WaterTool::tool_name() const {
    return "WaterTool";
}

void WaterTool::update(float delta_time, PrimaryViewport& viewport) {
    (void)delta_time;
    (void)viewport;
}

void WaterTool::render(ImDrawList* draw_list, const PrimaryViewport& viewport) {
    (void)draw_list;
    (void)viewport;
}

void WaterTool::on_mouse_down(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void WaterTool::on_mouse_up(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void WaterTool::on_mouse_move(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void WaterTool::on_right_click(const Core::Vec2& world_pos) {
    (void)world_pos;
}

} // namespace RogueCity::App
