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
