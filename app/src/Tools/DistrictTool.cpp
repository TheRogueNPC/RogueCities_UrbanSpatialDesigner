#include "RogueCity/App/Tools/DistrictTool.hpp"

namespace RogueCity::App {

const char* DistrictTool::tool_name() const {
    return "DistrictTool";
}

void DistrictTool::update(float delta_time, PrimaryViewport& viewport) {
    (void)delta_time;
    (void)viewport;
}

void DistrictTool::render(ImDrawList* draw_list, const PrimaryViewport& viewport) {
    (void)draw_list;
    (void)viewport;
}

void DistrictTool::on_mouse_down(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void DistrictTool::on_mouse_up(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void DistrictTool::on_mouse_move(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void DistrictTool::on_right_click(const Core::Vec2& world_pos) {
    (void)world_pos;
}

} // namespace RogueCity::App
