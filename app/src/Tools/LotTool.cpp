#include "RogueCity/App/Tools/LotTool.hpp"

namespace RogueCity::App {

const char* LotTool::tool_name() const {
    return "LotTool";
}

void LotTool::update(float delta_time, PrimaryViewport& viewport) {
    (void)delta_time;
    (void)viewport;
}

void LotTool::render(ImDrawList* draw_list, const PrimaryViewport& viewport) {
    (void)draw_list;
    (void)viewport;
}

void LotTool::on_mouse_down(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void LotTool::on_mouse_up(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void LotTool::on_mouse_move(const Core::Vec2& world_pos) {
    (void)world_pos;
}

void LotTool::on_right_click(const Core::Vec2& world_pos) {
    (void)world_pos;
}

} // namespace RogueCity::App
