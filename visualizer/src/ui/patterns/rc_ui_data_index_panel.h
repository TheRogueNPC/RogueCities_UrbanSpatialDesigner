// FILE: rc_ui_data_index_panel.h
// PURPOSE: Generic template for data index panels with sortable/filterable table
// REDUCES: 80% code duplication across road_index, district_index, lot_index panels

#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>

#include "ui/rc_ui_root.h"
#include "ui/introspection/UiIntrospection.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"

namespace RC_UI::Patterns {

// Trait interface for customizing RcDataIndexPanel behavior
template<typename T>
struct DataIndexPanelTraits {
    using ContainerType = std::vector<T>;
    // Required: Get the data collection from GlobalState
    static ContainerType& GetData(RogueCity::Core::Editor::GlobalState& gs);
    
    // Required: Get display name for an entity
    static std::string GetEntityLabel(const T& entity, size_t index);
    
    // Optional: Custom panel title
    static const char* GetPanelTitle() { return "Data Index"; }
    
    // Optional: Custom data name (for introspection)
    static const char* GetDataName() { return "entities"; }
    
    // Optional: Custom tags for introspection
    static std::vector<std::string> GetTags() { return {"index"}; }
    
    // Optional: Custom filter predicate
    static bool FilterEntity(const T& entity, const std::string& filter) { return true; }
    
    // Optional: Custom sort comparator (default: by ID)
    static bool CompareEntities(const T& a, const T& b) { return a.id < b.id; }
    
    // Optional: Custom context menu for entity
    static void ShowContextMenu(T& entity, size_t index) { /* no-op by default */ }
    
    // Optional: Custom selection callback
    static void OnEntitySelected(T& entity, size_t index) { /* no-op by default */ }

    // Optional: Custom hover callback
    static void OnEntityHovered(T& entity, size_t index) { /* no-op by default */ }
};

// Generic data index panel template
template<typename T, typename Traits = DataIndexPanelTraits<T>>
class RcDataIndexPanel {
public:
    RcDataIndexPanel(const char* panel_title = nullptr, const char* source_file = "unknown")
        : m_panel_title(panel_title ? panel_title : Traits::GetPanelTitle())
        , m_source_file(source_file)
        , m_filter_text{}
        , m_selected_index(-1)
        , m_sort_ascending(true)
    {
    }
    
    // Main draw function
    void Draw(float dt) {
        using RogueCity::Core::Editor::GetGlobalState;
        auto& gs = GetGlobalState();
        
        static RC_UI::DockableWindowState s_index_window;
        if (!RC_UI::BeginDockableWindow(m_panel_title, s_index_window, "Bottom", ImGuiWindowFlags_NoCollapse)) {
            return;
        }
        
        // Register with introspection system
        auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
        uiint.BeginPanel(
            RogueCity::UIInt::PanelMeta{
                m_panel_title,
                m_panel_title,
                "index",
                "Bottom",
                m_source_file,
                Traits::GetTags()
            },
            true
        );
        
        // Get data
        auto& data = Traits::GetData(gs);
        
        // Header with count
        ImGui::Text("%s: %llu", Traits::GetDataName(), static_cast<unsigned long long>(data.size()));
        
        // Filter input
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputTextWithHint("##filter", "Filter...", m_filter_text, sizeof(m_filter_text));
        
        // Sort button
        ImGui::SameLine();
        if (ImGui::SmallButton(m_sort_ascending ? "? Sort" : "? Sort")) {
            m_sort_ascending = !m_sort_ascending;
        }
        
        ImGui::Separator();
        
        // Table
        const ImGuiTableFlags table_flags = 
            ImGuiTableFlags_RowBg | 
            ImGuiTableFlags_Borders | 
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_Hideable;
        
        if (ImGui::BeginTable("##data_table", 1, table_flags, ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_NoHide);
            ImGui::TableHeadersRow();
            
            // Filter and sort
            std::vector<size_t> visible_indices;
            std::string filter_str(m_filter_text);
            
            for (size_t i = 0; i < data.size(); ++i) {
                if (filter_str.empty() || Traits::FilterEntity(data[i], filter_str)) {
                    visible_indices.push_back(i);
                }
            }
            
            // Sort if requested
            if (!m_sort_ascending) {
                std::reverse(visible_indices.begin(), visible_indices.end());
            }
            
            // Render rows
            for (size_t idx : visible_indices) {
                auto& entity = data[idx];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                
                // Create unique ID for selectable
                ImGui::PushID(static_cast<int>(idx));
                
                std::string label = Traits::GetEntityLabel(entity, idx);
                const bool is_selected = (m_selected_index == static_cast<int>(idx));
                
                if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    m_selected_index = static_cast<int>(idx);
                    Traits::OnEntitySelected(entity, idx);
                }

                if (ImGui::IsItemHovered()) {
                    Traits::OnEntityHovered(entity, idx);
                }
                
                // Right-click context menu
                if (ImGui::BeginPopupContextItem()) {
                    Traits::ShowContextMenu(entity, idx);
                    ImGui::EndPopup();
                }
                
                ImGui::PopID();
            }
            
            ImGui::EndTable();
        }
        
        // Footer with selection info
        if (m_selected_index >= 0 && m_selected_index < static_cast<int>(data.size())) {
            ImGui::Separator();
            ImGui::Text("Selected: %s", Traits::GetEntityLabel(data[m_selected_index], m_selected_index).c_str());
        }
        
        uiint.RegisterWidget({"table", Traits::GetDataName(), std::string(Traits::GetDataName()) + "[]", {"index"}});
        uiint.EndPanel();
        RC_UI::EndDockableWindow();
    }
    
    // Get currently selected index
    int GetSelectedIndex() const { return m_selected_index; }
    
    // Programmatically set selection
    void SetSelectedIndex(int index) { m_selected_index = index; }
    
    // Clear selection
    void ClearSelection() { m_selected_index = -1; }
    
private:
    const char* m_panel_title;
    const char* m_source_file;
    char m_filter_text[256];
    int m_selected_index;
    bool m_sort_ascending;
};

} // namespace RC_UI::Patterns
