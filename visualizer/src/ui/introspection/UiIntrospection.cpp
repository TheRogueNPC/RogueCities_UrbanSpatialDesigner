#include "UiIntrospection.h"

#include <filesystem>
#include <fstream>

namespace RogueCity::UIInt {

using json = nlohmann::json;

void to_json(json& j, const UiWidgetInfo& w) {
    j = json{
        {"type", w.type},
        {"label", w.label},
        {"binding", w.binding},
        {"tags", w.tags},
    };
}

void from_json(const json& j, UiWidgetInfo& w) {
    w.type = j.value("type", "");
    w.label = j.value("label", "");
    w.binding = j.value("binding", "");
    w.tags = j.value("tags", std::vector<std::string>{});
}

void to_json(json& j, const UiPanelInfo& p) {
    j = json{
        {"id", p.id},
        {"title", p.title},
        {"role", p.role},
        {"dock_area", p.dock_area},
        {"visible", p.visible},
        {"owner_module", p.owner_module},
        {"tags", p.tags},
        {"widgets", p.widgets},
    };
}

void from_json(const json& j, UiPanelInfo& p) {
    p.id = j.value("id", "");
    p.title = j.value("title", "");
    p.role = j.value("role", "");
    p.dock_area = j.value("dock_area", "Unknown");
    p.visible = j.value("visible", true);
    p.owner_module = j.value("owner_module", "");
    p.tags = j.value("tags", std::vector<std::string>{});
    p.widgets = j.value("widgets", std::vector<UiWidgetInfo>{});
}

void to_json(json& j, const UiActionInfo& a) {
    j = json{
        {"id", a.id},
        {"label", a.label},
        {"panel_id", a.panel_id},
        {"context_tags", a.context_tags},
        {"handler_symbol", a.handler_symbol},
    };
}

void from_json(const json& j, UiActionInfo& a) {
    a.id = j.value("id", "");
    a.label = j.value("label", "");
    a.panel_id = j.value("panel_id", "");
    a.context_tags = j.value("context_tags", std::vector<std::string>{});
    a.handler_symbol = j.value("handler_symbol", "");
}

void to_json(json& j, const DockTreeNode& n) {
    j = json::object();
    if (!n.id.empty()) j["id"] = n.id;
    if (!n.orientation.empty()) j["orientation"] = n.orientation;
    if (!n.panel_id.empty()) j["panel_id"] = n.panel_id;
    if (!n.children.empty()) j["children"] = n.children;
}

void from_json(const json& j, DockTreeNode& n) {
    n.id = j.value("id", "");
    n.orientation = j.value("orientation", "");
    n.panel_id = j.value("panel_id", "");
    n.children = j.value("children", std::vector<DockTreeNode>{});
}

void to_json(json& j, const UiIntrospectionSnapshot& s) {
    j = json{
        {"app_name", s.app_name},
        {"protocol_version", s.protocol_version},
        {"active_mode", s.active_mode},
        {"context_tags", s.context_tags},
        {"panels", s.panels},
        {"actions", s.actions},
    };
    if (!s.dock_tree.id.empty() || !s.dock_tree.panel_id.empty() || !s.dock_tree.children.empty()) {
        j["dock_tree"] = s.dock_tree;
    }
    if (!s.patterns_detected.empty()) {
        j["patterns_detected"] = s.patterns_detected;
    }
}

void from_json(const json& j, UiIntrospectionSnapshot& s) {
    s.app_name = j.value("app_name", "RogueCity Visualizer");
    s.protocol_version = j.value("protocol_version", "ui-introspect-v1");
    s.active_mode = j.value("active_mode", "");
    s.context_tags = j.value("context_tags", std::vector<std::string>{});
    s.panels = j.value("panels", std::vector<UiPanelInfo>{});
    s.actions = j.value("actions", std::vector<UiActionInfo>{});
    if (j.contains("dock_tree")) s.dock_tree = j["dock_tree"].get<DockTreeNode>();
    if (j.contains("patterns_detected") && j["patterns_detected"].is_array()) {
        s.patterns_detected.clear();
        for (const auto& p : j["patterns_detected"]) s.patterns_detected.push_back(p);
    }
}

UiIntrospector& UiIntrospector::Instance() {
    static UiIntrospector inst;
    return inst;
}

void UiIntrospector::BeginFrame(const std::string& activeMode, bool devShellActive) {
    m_snapshot = UiIntrospectionSnapshot{};
    m_snapshot.active_mode = activeMode;
    m_snapshot.context_tags.clear();
    if (!activeMode.empty()) {
        m_snapshot.context_tags.push_back("mode:" + activeMode);
    }
    m_snapshot.context_tags.push_back(std::string("devshell:") + (devShellActive ? "active" : "inactive"));
    m_panelStack.clear();
}

void UiIntrospector::SetDockTree(const DockTreeNode& dockTree) {
    m_snapshot.dock_tree = dockTree;
}

void UiIntrospector::BeginPanel(const PanelMeta& meta, bool visible) {
    UiPanelInfo p;
    p.id = meta.id;
    p.title = meta.title.empty() ? meta.id : meta.title;
    p.role = meta.role;
    p.dock_area = meta.dock_area.empty() ? "Unknown" : meta.dock_area;
    p.visible = visible;
    p.owner_module = meta.owner_module;
    p.tags = meta.tags;
    m_snapshot.panels.push_back(std::move(p));
    m_panelStack.push_back(static_cast<int>(m_snapshot.panels.size()) - 1);
}

void UiIntrospector::RegisterWidget(const UiWidgetInfo& w) {
    if (m_panelStack.empty()) return;
    int idx = m_panelStack.back();
    if (idx < 0 || idx >= static_cast<int>(m_snapshot.panels.size())) return;
    m_snapshot.panels[idx].widgets.push_back(w);
}

void UiIntrospector::RegisterAction(const UiActionInfo& a) {
    m_snapshot.actions.push_back(a);
}

void UiIntrospector::EndPanel() {
    if (!m_panelStack.empty()) m_panelStack.pop_back();
}

json UiIntrospector::SnapshotJson() const {
    json j = m_snapshot;
    return j;
}

bool UiIntrospector::SaveSnapshotJson(const std::string& filename, std::string* outError) const {
    try {
        std::filesystem::path path(filename);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        std::ofstream f(path.string(), std::ios::binary);
        if (!f.is_open()) {
            if (outError) *outError = "Failed to open output file";
            return false;
        }
        f << SnapshotJson().dump(2);
        return true;
    } catch (const std::exception& e) {
        if (outError) *outError = e.what();
        return false;
    }
}

} // namespace RogueCity::UIInt
