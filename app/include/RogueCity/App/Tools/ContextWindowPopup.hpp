/**
 * @class ContextWindowPopup
 * @brief Provides a reusable, context-sensitive popup window for numeric entry in the UI.
 *
 * Designed with a Y2K aesthetic (hard edges, warning stripe border, fixed-width font), this class enables
 * flexible contextual interactions such as adjusting building properties, axiom properties, or other numeric values.
 * The popup can display different content and handle various input types based on the context, supporting
 * customization via a generic content rendering function or data structure.
 *
 * Features:
 * - Opens at a specified screen position with a given value range and label.
 * - Supports smooth value interpolation (0.3s ease-in-out) for user feedback.
 * - Allows setting a callback for value changes.
 * - Designed for intuitive user experience with labels, tooltips, and clear feedback.
 *
 * Usage:
 * - Call `open()` to display the popup at a desired position.
 * - Call `update()` per frame while the popup is open to render and handle input.
 * - Use `set_on_apply()` to handle value changes.
 * - Call `close()` to dismiss the popup.
 *
 * @note The popup is intended to be reusable across different contexts and should not be tightly coupled to a specific use case.
 */
 
#pragma once
#include "RogueCity/Core/Types.hpp"
#include <imgui.h>
#include <functional>
#include <string>
//todo ensure that we are not hardcoding contextual popups within our functions and instead are using a more flexible system that allows for different types of contextual interactions (e.g. for building properties, axiom properties, etc.) and that the ContextWindowPopup is designed to be reusable across these different contexts. The popup should be able to display different content and handle different types of input based on the context in which it is used, rather than being tightly coupled to a specific use case like adjusting ring radii. This may involve designing the ContextWindowPopup to accept a more generic content rendering function or data structure that can be customized for each use case, rather than hardcoding it for a specific type of interaction. also consider the user experience of the popup, ensuring that it is intuitive and easy to use across different contexts, and that it provides clear feedback to the user about what they are adjusting and how it will affect the city generation. This may involve adding labels, tooltips, or other UI elements to guide the user in using the popup effectively. 
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
