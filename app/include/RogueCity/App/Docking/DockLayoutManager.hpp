#pragma once
#include "RogueCity/Core/Editor/EditorState.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <map>

namespace RogueCity::App {

/// Per-state docking layout configuration
/// Implements state-reactive panel visibility and optimization
struct DockLayoutState {
    Core::Editor::EditorState state;
    std::vector<std::string> visible_panels;          // Panels shown in this state
    std::map<std::string, ImGuiID> panel_dock_ids;    // Panel -> DockSpace ID mapping
    bool is_optimized{ false };                        // Hidden panels skip rendering

    /// Serialize to .ini format
    [[nodiscard]] std::string to_ini() const;

    /// Deserialize from .ini format
    static DockLayoutState from_ini(const std::string& ini_data);
};

/// Manages docking layouts across HFSM editor states
/// Enforces Cockpit Doctrine: panels as control stations, state-reactive
class DockLayoutManager {
public:
    DockLayoutManager();
    ~DockLayoutManager();

    /// Initialize docking system (call after ImGui context creation)
    void initialize(ImGuiID main_dock_id);

    /// Load layout for editor state
    void load_layout(Core::Editor::EditorState state);

    /// Save current layout for active state
    void save_current_layout();

    /// Transition between states with smooth blend (0.5s)
    void transition_to_state(Core::Editor::EditorState new_state, float duration = 0.5f);

    /// Update transition animation (call per frame)
    void update(float delta_time);

    /// Register panel with manager
    void register_panel(const std::string& panel_name);

    /// Check if panel should be visible in current state
    [[nodiscard]] bool is_panel_visible(const std::string& panel_name) const;

    /// Optimize panel (hide and skip rendering)
    void optimize_panel(const std::string& panel_name, bool optimize);

    [[nodiscard]] Core::Editor::EditorState current_state() const;

private:
    ImGuiID main_dock_id_{ 0 };
    Core::Editor::EditorState current_state_{ Core::Editor::EditorState::Startup };
    Core::Editor::EditorState target_state_{ Core::Editor::EditorState::Startup };

    std::map<Core::Editor::EditorState, DockLayoutState> layouts_;
    std::vector<std::string> registered_panels_;

    bool is_transitioning_{ false };
    float transition_time_{ 0.0f };
    float transition_duration_{ 0.5f };

    /// Load layouts from disk
    void load_layouts_from_disk();

    /// Save layouts to disk
    void save_layouts_to_disk();

    /// Apply layout immediately (no transition)
    void apply_layout(const DockLayoutState& layout);
};

} // namespace RogueCity::App
