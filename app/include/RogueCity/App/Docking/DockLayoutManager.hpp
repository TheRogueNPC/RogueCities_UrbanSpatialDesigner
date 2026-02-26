#pragma once

#include "RogueCity/Core/Editor/EditorState.hpp"
#include <imgui.h>
#include <map>
#include <string>
#include <vector>

namespace RogueCity::App {

struct DockLayoutState {
    Core::Editor::EditorState state{ Core::Editor::EditorState::Startup };
    std::vector<std::string> visible_panels{};
    std::map<std::string, ImGuiID> panel_dock_ids{};
    bool is_optimized{ false };

    [[nodiscard]] std::string to_ini() const;
    static DockLayoutState from_ini(const std::string& ini_data);
};

class DockLayoutManager {
public:
    DockLayoutManager();
    ~DockLayoutManager();

    void initialize(ImGuiID main_dock_id);
    void load_layout(Core::Editor::EditorState state);
    void save_current_layout();
    void transition_to_state(Core::Editor::EditorState new_state, float duration = 0.5f);
    void update(float delta_time);
    void register_panel(const std::string& panel_name);
    [[nodiscard]] bool is_panel_visible(const std::string& panel_name) const;
    void optimize_panel(const std::string& panel_name, bool optimize);
    [[nodiscard]] Core::Editor::EditorState current_state() const;

private:
    void load_layouts_from_disk();
    void save_layouts_to_disk();
    void apply_layout(const DockLayoutState& layout);

    ImGuiID main_dock_id_{ 0 };
    std::map<Core::Editor::EditorState, DockLayoutState> layouts_{};
    std::vector<std::string> registered_panels_{};
    Core::Editor::EditorState current_state_{ Core::Editor::EditorState::Startup };
    Core::Editor::EditorState target_state_{ Core::Editor::EditorState::Startup };
    bool is_transitioning_{ false };
    float transition_time_{ 0.0f };
    float transition_duration_{ 0.5f };
};

} // namespace RogueCity::App
