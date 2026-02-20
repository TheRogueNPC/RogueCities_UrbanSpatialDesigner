#include "RogueCity/App/Tools/RoadTool.hpp"

namespace RogueCity::App {

const char* RoadTool::tool_name() const {
    return "RoadTool";
}

void RoadTool::update(float delta_time, PrimaryViewport& viewport) {
    (void)delta_time;
    (void)viewport;
}

void RoadTool::render(ImDrawList* draw_list, const PrimaryViewport& viewport) {
    (void)draw_list;
    (void)viewport;
}

void RoadTool::on_mouse_down(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void RoadTool::on_mouse_up(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void RoadTool::on_mouse_move(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void RoadTool::on_right_click(const Core::Vec2& world_pos) {
    (void)world_pos;
}

} // namespace RogueCity::App
