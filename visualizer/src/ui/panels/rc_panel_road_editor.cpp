// FILE: visualizer/src/ui/panels/rc_panel_road_editor.cpp
// PURPOSE: Implementation of the Road Network Editor Panel.

#include "ui/panels/rc_panel_road_editor.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_tokens.h"
#include "ui/tools/rc_tool_dispatcher.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Editor/SelectionSync.hpp"

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

namespace RC_UI::Panels::RoadEditor {

namespace {

static RoadEditorSubtool s_active_subtool = RoadEditorSubtool::Select;

// Helper to find a road by ID.
RogueCity::Core::Road* FindRoad(RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (auto& road : gs.roads) {
        if (road.id == id) {
            return &road;
        }
    }
    return nullptr;
}

// Render a single capsule-style toggle button for the mode selector.
void RenderSubtoolToggle(const char* label, RoadEditorSubtool tool) {
    bool is_active = (s_active_subtool == tool);

    if (is_active) {
        const float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(ImGui::GetTime() * 4.0));
        const ImU32 pulse_color = LerpColor(UITokens::GreenHUD, UITokens::CyanAccent, 1.0f - pulse);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(pulse_color));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(UITokens::Transparent));
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    if (ImGui::Button(label)) {
        s_active_subtool = tool;
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

template <typename EnumType>
bool DrawEnumCombo(const char* label, EnumType& value) {
    const auto names = magic_enum::enum_names<EnumType>();
    const auto values = magic_enum::enum_values<EnumType>();
    const auto current_name = std::string(magic_enum::enum_name(value));
    bool changed = false;

    if (ImGui::BeginCombo(label, current_name.c_str())) {
        for (size_t i = 0; i < values.size(); ++i) {
            const bool selected = (values[i] == value);
            const std::string name = std::string(names[i]);
            if (ImGui::Selectable(name.c_str(), selected)) {
                value = values[i];
                changed = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

} // anonymous namespace

void DrawContent(float dt) {
    auto& gs = RogueCity::Core::Editor::GetGlobalState();

    // --- Station A (Mode Selector) ---
    ImGui::SeparatorText("Station A: Mode");
    ImGui::Spacing();

    RenderSubtoolToggle("Select", RoadEditorSubtool::Select);
    ImGui::SameLine();
    RenderSubtoolToggle("Add Vertex", RoadEditorSubtool::AddVertex);
    ImGui::SameLine();
    RenderSubtoolToggle("Split", RoadEditorSubtool::Split);
    ImGui::SameLine();
    RenderSubtoolToggle("Merge", RoadEditorSubtool::Merge);
    ImGui::SameLine();
    RenderSubtoolToggle("Convert", RoadEditorSubtool::Convert);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // --- Station B (Telemetry) ---
    ImGui::SeparatorText("Station B: Telemetry");
    if (gs.selection.selected_road) {
        RogueCity::Core::Road* road = FindRoad(gs, gs.selection.selected_road->id);
        if (road) {
            ImGui::Text("Road ID: %u", road->id);
            // Using number of points as a proxy for length for now.
            ImGui::Text("Length: %.2f m (approx)", road->points.size() > 1 ? (road->points.size() - 1) * 10.0f : 0.0f);

            const auto type_name = magic_enum::enum_name(road->type);
            ImGui::Text("Flow Capacity (Type): %s", type_name.data());
        } else {
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextDisabled), "Selected road not found.");
        }
    } else {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextDisabled), "No road selected in ViewportIndex.");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // --- Station C (Mutation) ---
    ImGui::SeparatorText("Station C: Mutation");
    switch (s_active_subtool) {
    case RoadEditorSubtool::Select:
        ImGui::Text("Select a road segment in the viewport.");
        break;
    case RoadEditorSubtool::AddVertex:
        ImGui::Text("Click on a road to add a vertex.");
        break;
    case RoadEditorSubtool::Split:
        ImGui::Text("Click on a road segment to split it.");
        break;
    case RoadEditorSubtool::Merge:
        ImGui::Text("Select two roads to merge them.");
        break;
    case RoadEditorSubtool::Convert:
        if (gs.selection.selected_road) {
            RogueCity::Core::Road* road = FindRoad(gs, gs.selection.selected_road->id);
            if(road) {
                if (DrawEnumCombo("Road Type", road->type)) {
                    // TODO: Dispatch action to change road type
                }
            }
        } else {
            ImGui::Text("Select a road to change its type.");
        }
        break;
    }
}

void Draw(float dt) {
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    
    // The actual visibility is controlled by the drawer's is_visible() method.
    // This BeginTokenPanel is just for the look and feel.
    if (!Components::BeginTokenPanel("Road Network Editor", UITokens::AmberGlow)) {
        Components::EndTokenPanel();
        return;
    }

    DrawContent(dt);

    Components::EndTokenPanel();
}

} // namespace RC_UI::Panels::RoadEditor
