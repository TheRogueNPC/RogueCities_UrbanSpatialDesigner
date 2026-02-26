/**
 * @class DistrictTool
 * @brief Tool for manipulating city districts within the viewport.
 *
 * Inherits from IViewportTool and provides functionality for district editing,
 * including responding to mouse events and rendering district overlays.
 *
 * @note This tool is intended for use in the urban spatial designer application.
 *
 * @see IViewportTool
 */
#pragma once

#include "RogueCity/App/Tools/IViewportTool.hpp"

namespace RogueCity::App {

class DistrictTool final : public IViewportTool {
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
