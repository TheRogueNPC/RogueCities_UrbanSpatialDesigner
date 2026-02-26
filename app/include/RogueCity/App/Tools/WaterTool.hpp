/**
 * @brief Tool for manipulating water features in the viewport using spline-based interactions.
 *
 * WaterTool provides interactive editing capabilities for water elements within the urban spatial designer.
 * It allows users to add, modify, and visualize water features by responding to mouse events and rendering
 * spline manipulations in the viewport.
 *
 * Inherits from IViewportTool and manages its own interaction state and viewport reference.
 */
#pragma once

#include "RogueCity/App/Tools/IViewportTool.hpp"
#include "RogueCity/App/Tools/SplineManipulator.hpp"

namespace RogueCity::App {

class WaterTool final : public IViewportTool {
public:
    [[nodiscard]] const char* tool_name() const override;

    void update(float delta_time, PrimaryViewport& viewport) override;
    void render(ImDrawList* draw_list, const PrimaryViewport& viewport) override;

    void on_mouse_down(const Core::Vec2& world_pos) override;
    void on_mouse_up(const Core::Vec2& world_pos) override;
    void on_mouse_move(const Core::Vec2& world_pos) override;
    void on_right_click(const Core::Vec2& world_pos) override;

    void on_activate(PrimaryViewport& viewport) override { viewport_ = &viewport; }
    void on_deactivate(PrimaryViewport& viewport) override { viewport_ = nullptr; }

private:
    Tools::SplineInteractionState spline_state_{};
    PrimaryViewport* viewport_{ nullptr };
};

} // namespace RogueCity::App
