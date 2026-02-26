
/**
 * @class PanelRegistry
 * @brief Manages the lifecycle and registration of UI panels within the application.
 *
 * The PanelRegistry class is responsible for registering, unregistering, and rendering panels.
 * It ensures that panels retain their content when hidden, preserving context and state.
 * Panels can be optimized to skip rendering when not visible, improving performance.
 *
 * @note Panels are identified by their unique string names.
 *
 * @typedef RenderCallback
 * @brief Callback type for rendering a panel.
 *
 * @fn PanelRegistry()
 * @brief Constructs a new PanelRegistry instance.
 *
 * @fn void register_panel(const std::string& name, RenderCallback render_func)
 * @brief Registers a panel with a given name and render callback.
 * @param name Unique name of the panel.
 * @param render_func Callback function to render the panel.
 *
 * @fn void unregister_panel(const std::string& name)
 * @brief Unregisters a panel by its name.
 * @param name Name of the panel to unregister.
 *
 * @fn void render_panel(const std::string& name, bool force_render = false)
 * @brief Renders a panel by its name, optionally forcing a render regardless of optimization.
 * @param name Name of the panel to render.
 * @param force_render If true, forces rendering even if optimized.
 *
 * @fn void set_optimized(const std::string& name, bool optimized)
 * @brief Sets the optimization flag for a panel, controlling whether it is rendered when not visible.
 * @param name Name of the panel.
 * @param optimized If true, panel rendering is skipped when not visible.
 *
 * @fn bool is_optimized(const std::string& name) const
 * @brief Checks if a panel is optimized.
 * @param name Name of the panel.
 * @return True if the panel is optimized, false otherwise.
 *
 * @fn bool is_registered(const std::string& name) const
 * @brief Checks if a panel is registered.
 * @param name Name of the panel.
 * @return True if the panel is registered, false otherwise.
 *
 * @struct PanelInfo
 * @brief Internal structure holding panel information, including render callback, optimization flag, and content state.
 *
 * @var PanelInfo::render_func
 * Callback function for rendering the panel.
 * @var PanelInfo::is_optimized
 * Indicates whether the panel is optimized.
 * @var PanelInfo::content_dirty
 * Indicates whether the panel needs re-rendering after optimization.
 *
 * @var panels_
 * @brief Map storing registered panels and their associated information.
 */
 
#pragma once
#include <string>
#include <functional>
#include <map>

namespace RogueCity::App {

/// Panel lifecycle management and registration
/// Ensures panels retain content when hidden (context preservation)
class PanelRegistry {
public:
    using RenderCallback = std::function<void()>;

    PanelRegistry();

    /// Register panel with render callback
    void register_panel(const std::string& name, RenderCallback render_func);

    /// Unregister panel
    void unregister_panel(const std::string& name);

    /// Render panel (respects optimization flags)
    void render_panel(const std::string& name, bool force_render = false);

    /// Set panel optimization (skip rendering when not visible)
    void set_optimized(const std::string& name, bool optimized);

    [[nodiscard]] bool is_optimized(const std::string& name) const;
    [[nodiscard]] bool is_registered(const std::string& name) const;

private:
    struct PanelInfo {
        RenderCallback render_func;
        bool is_optimized{ false };
        bool content_dirty{ true };  // Needs re-render after optimization
    };

    std::map<std::string, PanelInfo> panels_;
};

} // namespace RogueCity::App
