#include "RogueCity/App/Tools/BuildingTool.hpp"

namespace RogueCity::App {

const char* BuildingTool::tool_name() const {
    return "BuildingTool";
}

void BuildingTool::update(float delta_time, PrimaryViewport& viewport) {
    (void)delta_time;
    (void)viewport;
}

void BuildingTool::render(ImDrawList* draw_list, const PrimaryViewport& viewport) {
    (void)draw_list;
    (void)viewport;
}

void BuildingTool::on_mouse_down(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void BuildingTool::on_mouse_up(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void BuildingTool::on_mouse_move(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void BuildingTool::on_right_click(const Core::Vec2& world_pos) {
    (void)world_pos;
}

} // namespace RogueCity::App
