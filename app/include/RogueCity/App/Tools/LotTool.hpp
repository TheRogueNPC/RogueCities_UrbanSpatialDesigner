/**
 * @class LotTool
 * @brief Tool for manipulating lots within the viewport in RogueCity.
 *
 * Inherits from IViewportTool and provides functionality for interacting with lots
 * via mouse events and rendering within the application's primary viewport.
 *
 * @note This tool handles mouse interactions such as mouse down, mouse up, mouse move,
 * and right click, allowing for custom lot manipulation behaviors.
 */
#pragma once

#include "RogueCity/App/Tools/IViewportTool.hpp"

namespace RogueCity::App {

class LotTool final : public IViewportTool {
public:
    [[nodiscard]] const char* tool_name() const override;

    void update(float delta_time, PrimaryViewport& viewport) override;
    void render(ImDrawList* draw_list, const PrimaryViewport& viewport) override;

    void on_mouse_down(const Core::Vec2& world_pos) override;
    void on_mouse_up(const Core::Vec2& world_pos) override;
    void on_mouse_move(const Core::Vec2& world_pos) override;
    void on_right_click(const Core::Vec2& world_pos) override;
};

} // namespace RogueCity::App
