#include "RogueCity/App/Docking/DockLayoutManager.hpp"
#include <imgui_internal.h>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace RogueCity::App {

namespace {

[[nodiscard]] constexpr bool DockingAvailable() {
#if defined(IMGUI_HAS_DOCK)
    return true;
#else
    return false;
#endif
}

std::string Trim(const std::string& s) {
    const size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::vector<std::string> SplitCsv(const std::string& csv) {
    std::vector<std::string> out;
    std::stringstream ss(csv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        const std::string trimmed = Trim(token);
        if (!trimmed.empty()) {
            out.push_back(trimmed);
        }
    }
    return out;
}

bool ParseStateHeader(const std::string& line, Core::Editor::EditorState& out_state) {
    if (!line.starts_with("[State_") || !line.ends_with(']')) {
        return false;
    }
    const std::string raw = line.substr(7, line.size() - 8);
    try {
        const int value = std::stoi(raw);
        out_state = static_cast<Core::Editor::EditorState>(value);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

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
    std::stringstream ss(ini_data);
    std::string line;
    while (std::getline(ss, line)) {
        line = Trim(line);
        if (line.empty()) {
            continue;
        }

        Core::Editor::EditorState parsed_state{};
        if (ParseStateHeader(line, parsed_state)) {
            layout.state = parsed_state;
            continue;
        }

        if (line.starts_with("Optimized=")) {
            layout.is_optimized = line.substr(10) == "1";
            continue;
        }

        if (line.starts_with("VisiblePanels=")) {
            layout.visible_panels = SplitCsv(line.substr(14));
        }
    }
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
    layout.panel_dock_ids.clear();
    
    // Gather visible panels
    layout.visible_panels.clear();
    for (const auto& panel_name : registered_panels_) {
        ImGuiWindow* window = ImGui::FindWindowByName(panel_name.c_str());
        if (window == nullptr) {
            continue;
        }
        if (window->Hidden || window->Collapsed) {
            continue;
        }
        layout.visible_panels.push_back(panel_name);
#if defined(IMGUI_HAS_DOCK)
        if (window->DockId != 0) {
            layout.panel_dock_ids[panel_name] = window->DockId;
        }
#endif
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
        // Keep transition deterministic and bounded; per-window blending can be layered above.
        const float t = std::clamp(transition_time_ / std::max(0.001f, transition_duration_), 0.0f, 1.0f);
        (void)t;
    }
}

void DockLayoutManager::register_panel(const std::string& panel_name) {
    if (std::find(registered_panels_.begin(), registered_panels_.end(), panel_name) == registered_panels_.end()) {
        registered_panels_.push_back(panel_name);
    }
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
    layouts_.clear();
    std::ifstream file("imgui_dock_layouts.ini");
    if (file.good()) {
        std::string block;
        std::string line;
        while (std::getline(file, line)) {
            if (line.starts_with("[State_") && !block.empty()) {
                DockLayoutState parsed = DockLayoutState::from_ini(block);
                layouts_[parsed.state] = std::move(parsed);
                block.clear();
            }
            block += line;
            block += '\n';
        }
        if (!block.empty()) {
            DockLayoutState parsed = DockLayoutState::from_ini(block);
            layouts_[parsed.state] = std::move(parsed);
        }
    }
    
    using State = Core::Editor::EditorState;
    
    if (!layouts_.contains(State::Startup)) {
        layouts_[State::Startup] = DockLayoutState{
            State::Startup,
            {"RogueVisualizer", "Tools", "Log"},
            {},
            false
        };
    }

    if (!layouts_.contains(State::Editing_Axioms)) {
        layouts_[State::Editing_Axioms] = DockLayoutState{
            State::Editing_Axioms,
            {"RogueVisualizer", "Tool Deck", "Inspector", "Tools", "Log"},
            {},
            false
        };
    }

    if (!layouts_.contains(State::Simulating)) {
        layouts_[State::Simulating] = DockLayoutState{
            State::Simulating,
            {"RogueVisualizer", "Analytics", "Log"},
            {},
            true
        };
    }
}

void DockLayoutManager::save_layouts_to_disk() {
    std::ofstream file("imgui_dock_layouts.ini");
    if (!file) return;

    for (const auto& [state, layout] : layouts_) {
        file << layout.to_ini() << "\n";
    }
}

void DockLayoutManager::apply_layout(const DockLayoutState& layout) {
    current_state_ = layout.state;
    if (!DockingAvailable()) {
        return;
    }
    if (main_dock_id_ == 0) {
        return;
    }
#if defined(IMGUI_HAS_DOCK)
    ImGuiDockNode* root = ImGui::DockBuilderGetNode(main_dock_id_);
    if (root == nullptr) {
        return;
    }

    for (const auto& panel_name : layout.visible_panels) {
        const auto it = layout.panel_dock_ids.find(panel_name);
        const ImGuiID dock_id = (it != layout.panel_dock_ids.end()) ? it->second : main_dock_id_;
        if (ImGui::DockBuilderGetNode(dock_id) == nullptr) {
            ImGui::DockBuilderDockWindow(panel_name.c_str(), main_dock_id_);
        } else {
            ImGui::DockBuilderDockWindow(panel_name.c_str(), dock_id);
        }
    }
    ImGui::DockBuilderFinish(main_dock_id_);
#endif
}

} // namespace RogueCity::App
