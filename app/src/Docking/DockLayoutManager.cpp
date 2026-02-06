#include "RogueCity/App/Docking/DockLayoutManager.hpp"
#include <fstream>
#include <sstream>

namespace RogueCity::App {

// DockLayoutState Implementation
std::string DockLayoutState::to_ini() const {
    std::ostringstream oss;
    oss << "[State_" << static_cast<int>(state) << "]\n";
    oss << "Optimized=" << (is_optimized ? "1" : "0") << "\n";
    
    oss << "VisiblePanels=";
    for (size_t i = 0; i < visible_panels.size(); ++i) {
        oss << visible_panels[i];
        if (i < visible_panels.size() - 1) oss << ",";
    }
    oss << "\n";
    
    return oss.str();
}

DockLayoutState DockLayoutState::from_ini(const std::string& ini_data) {
    DockLayoutState layout;
    // TODO: Parse INI format
    // For now, return empty layout
    return layout;
}

// DockLayoutManager Implementation
DockLayoutManager::DockLayoutManager() {
    load_layouts_from_disk();
}

DockLayoutManager::~DockLayoutManager() {
    save_layouts_to_disk();
}

void DockLayoutManager::initialize(ImGuiID main_dock_id) {
    main_dock_id_ = main_dock_id;
}

void DockLayoutManager::load_layout(Core::Editor::EditorState state) {
    if (is_transitioning_) return;

    auto it = layouts_.find(state);
    if (it != layouts_.end()) {
        apply_layout(it->second);
        current_state_ = state;
    }
}

void DockLayoutManager::save_current_layout() {
    DockLayoutState& layout = layouts_[current_state_];
    layout.state = current_state_;
    
    // Gather visible panels
    layout.visible_panels.clear();
    for (const auto& panel_name : registered_panels_) {
        // Check if panel window is open
        // TODO: Query ImGui window states
        layout.visible_panels.push_back(panel_name);
    }
}

void DockLayoutManager::transition_to_state(Core::Editor::EditorState new_state, float duration) {
    if (new_state == current_state_) return;

    save_current_layout();
    
    target_state_ = new_state;
    is_transitioning_ = true;
    transition_time_ = 0.0f;
    transition_duration_ = duration;
}

void DockLayoutManager::update(float delta_time) {
    if (!is_transitioning_) return;

    transition_time_ += delta_time;
    
    if (transition_time_ >= transition_duration_) {
        // Complete transition
        load_layout(target_state_);
        is_transitioning_ = false;
        transition_time_ = 0.0f;
    } else {
        // Animate transition (fade out/in panels)
        const float t = transition_time_ / transition_duration_;
        // TODO: Apply interpolation to panel alpha/positions
    }
}

void DockLayoutManager::register_panel(const std::string& panel_name) {
    registered_panels_.push_back(panel_name);
}

bool DockLayoutManager::is_panel_visible(const std::string& panel_name) const {
    auto it = layouts_.find(current_state_);
    if (it == layouts_.end()) return true;  // Default: show all

    const auto& visible = it->second.visible_panels;
    return std::find(visible.begin(), visible.end(), panel_name) != visible.end();
}

void DockLayoutManager::optimize_panel(const std::string& panel_name, bool optimize) {
    // Mark panel for skipped rendering when not visible
    auto& layout = layouts_[current_state_];
    layout.is_optimized = optimize;
}

Core::Editor::EditorState DockLayoutManager::current_state() const {
    return current_state_;
}

void DockLayoutManager::load_layouts_from_disk() {
    // TODO: Load from imgui_dock_layouts.ini
    // For now, create default layouts for each state
    
    using State = Core::Editor::EditorState;
    
    // Startup: minimal UI
    layouts_[State::Startup] = DockLayoutState{
        State::Startup,
        {"Viewport", "StatusBar"},
        {},
        false
    };
    
    // Editing Axioms: show axiom tools
    layouts_[State::Editing_Axioms] = DockLayoutState{
        State::Editing_Axioms,
        {"Viewport", "Minimap", "AxiomPanel", "Properties", "StatusBar"},
        {},
        false
    };
    
    // Simulating: show progress
    layouts_[State::Simulating] = DockLayoutState{
        State::Simulating,
        {"Viewport", "ProgressPanel", "Console", "StatusBar"},
        {},
        true  // Optimize hidden panels during generation
    };
}

void DockLayoutManager::save_layouts_to_disk() {
    // TODO: Save to imgui_dock_layouts.ini
    std::ofstream file("imgui_dock_layouts.ini");
    if (!file) return;

    for (const auto& [state, layout] : layouts_) {
        file << layout.to_ini() << "\n";
    }
}

void DockLayoutManager::apply_layout(const DockLayoutState& layout) {
    // Apply layout immediately (no transition)
    current_state_ = layout.state;
    
    // TODO: Apply docking configuration via ImGui Docking API
    // This would involve setting up dock splits and docking windows
}

} // namespace RogueCity::App
