#pragma once
#include "RogueCity/Core/Types.hpp"
#include <imgui.h>
#include <functional>
#include <string>

namespace RogueCity::App {

/// Context window popup for numeric entry (double-click knob)
/// Y2K design: hard edges, warning stripe border, fixed-width font
class ContextWindowPopup {
public:
    using OnApplyCallback = std::function<void(float)>;

    ContextWindowPopup();
    ~ContextWindowPopup();

    /// Open popup at screen position
    void open(const ImVec2& screen_pos, float current_value, 
              float min_value, float max_value, const std::string& label);

    /// Close popup (ESC or apply)
    void close();

    /// Update and render (call per frame when open)
    void update();

    /// Set callback for value changes
    void set_on_apply(OnApplyCallback callback);

    [[nodiscard]] bool is_open() const;

private:
    bool is_open_{ false };
    ImVec2 popup_pos_{ 0, 0 };
    float current_value_{ 0.0f };
    float min_value_{ 0.0f };
    float max_value_{ 1000.0f };
    std::string label_;
    char input_buffer_[64]{};

    OnApplyCallback on_apply_;

    /// Render Y2K styled popup window
    void render_popup();

    /// Smooth interpolation to new value (0.3s ease-in-out)
    void apply_with_interpolation(float new_value);
};

} // namespace RogueCity::App
