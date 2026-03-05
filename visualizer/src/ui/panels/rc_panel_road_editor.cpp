// FILE: visualizer/src/ui/panels/rc_panel_road_editor.cpp
// PURPOSE: Implementation of the Road Network Editor Panel.

#include "ui/panels/rc_panel_road_editor.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_tokens.h"
#include <RogueCity/Visualizer/LucideIcons.hpp>
#include "ui/tools/rc_tool_dispatcher.h"
#include "RogueCity/App/Editor/CommandHistory.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Editor/SelectionSync.hpp"

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include <memory>

namespace RC_UI::Panels::RoadEditor {

namespace {

static RoadEditorSubtool s_active_subtool = RoadEditorSubtool::Select;

using RogueCity::Core::Editor::DirtyLayer;

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

void MarkRoadMutationDirty(RogueCity::Core::Editor::GlobalState& gs) {
    gs.dirty_layers.MarkDirty(DirtyLayer::Roads);
    gs.dirty_layers.MarkDirty(DirtyLayer::Districts);
    gs.dirty_layers.MarkDirty(DirtyLayer::Lots);
    gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
    gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
}

class SetRoadTypeCommand final : public RogueCity::App::ICommand {
public:
    SetRoadTypeCommand(
        uint32_t road_id,
        RogueCity::Core::RoadType before,
        RogueCity::Core::RoadType after)
        : road_id_(road_id),
          before_(before),
          after_(after) {}

    void Execute() override {
        Apply(after_);
    }

    void Undo() override {
        Apply(before_);
    }

    const char* GetDescription() const override {
        return "Road Type";
    }

private:
    void Apply(RogueCity::Core::RoadType type) {
        auto& gs = RogueCity::Core::Editor::GetGlobalState();
        RogueCity::Core::Road* road = FindRoad(gs, road_id_);
        if (road == nullptr) {
            return;
        }

        road->type = type;
        road->is_user_created = true;
        road->generation_tag = RogueCity::Core::GenerationTag::M_user;
        road->generation_locked = true;
        MarkRoadMutationDirty(gs);
    }

    uint32_t road_id_{0};
    RogueCity::Core::RoadType before_{RogueCity::Core::RoadType::Street};
    RogueCity::Core::RoadType after_{RogueCity::Core::RoadType::Street};
};

void DispatchRoadTypeChange(
    uint32_t road_id,
    RogueCity::Core::RoadType before,
    RogueCity::Core::RoadType after) {
    if (before == after) {
        return;
    }

    auto command = std::make_unique<SetRoadTypeCommand>(road_id, before, after);
    RogueCity::App::GetEditorCommandHistory().Execute(std::move(command));
}

} // anonymous namespace

void DrawContent(float dt) {
    auto& gs = RogueCity::Core::Editor::GetGlobalState();

    if (Components::DrawSectionHeader("Station A: Mode", UITokens::CyanAccent, true,
                                      RC::SvgTextureCache::Get().Load(LC::Move, 14.f))) {
        ImGui::Indent();
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
        ImGui::Unindent();
        ImGui::Spacing();
    }

    if (Components::DrawSectionHeader("Station B: Telemetry", UITokens::InfoBlue, true,
                                      RC::SvgTextureCache::Get().Load(LC::Activity, 14.f))) {
        ImGui::Indent();
        if (gs.selection.selected_road) {
            RogueCity::Core::Road* road = FindRoad(gs, gs.selection.selected_road->id);
            if (road) {
                ImGui::Text("Road ID: %u", road->id);
                ImGui::Text("Length: %.2f m (approx)", road->points.size() > 1 ? (road->points.size() - 1) * 10.0f : 0.0f);
                const auto type_name = magic_enum::enum_name(road->type);
                ImGui::Text("Flow Capacity (Type): %s", type_name.data());
            } else {
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextDisabled), "Selected road not found.");
            }
        } else {
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextDisabled), "No road selected in ViewportIndex.");
        }
        ImGui::Unindent();
        ImGui::Spacing();
    }

    if (Components::DrawSectionHeader("Station C: Mutation", UITokens::AmberGlow, true,
                                      RC::SvgTextureCache::Get().Load(LC::Pencil, 14.f))) {
        ImGui::Indent();
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
                if (road) {
                    const RogueCity::Core::RoadType before = road->type;
                    RogueCity::Core::RoadType requested = before;
                    if (DrawEnumCombo("Road Type", requested)) {
                        DispatchRoadTypeChange(road->id, before, requested);
                    }
                }
            } else {
                ImGui::Text("Select a road to change its type.");
            }
            break;
        }
        ImGui::Unindent();
        ImGui::Spacing();
    }
}

void Draw(float dt) {
    
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
