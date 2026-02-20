#pragma once

#include "RogueCity/Core/Types.hpp"

#include <imgui.h>

namespace RogueCity::App {

class PrimaryViewport;

class IViewportTool {
public:
    virtual ~IViewportTool() = default;

    [[nodiscard]] virtual const char* tool_name() const = 0;

    virtual void update(float delta_time, PrimaryViewport& viewport) = 0;
    virtual void render(ImDrawList* draw_list, const PrimaryViewport& viewport) = 0;

    virtual void on_mouse_down(const Core::Vec2& world_pos) = 0;
    virtual void on_mouse_up(const Core::Vec2& world_pos) = 0;
    virtual void on_mouse_move(const Core::Vec2& world_pos) = 0;
    virtual void on_right_click(const Core::Vec2& world_pos) = 0;
};

} // namespace RogueCity::App
