// FILE: rc_panel_telemetry.cpp
// PURPOSE: Live analytics panel showing procedural generation metrics.

#include "ui/panels/rc_panel_telemetry.h"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_theme.h"
#include "RogueCity/App/Tools/AxiomVisual.hpp"
#include "RogueCity/App/Tools/AxiomIcon.hpp"

#include <imgui.h>
#include <numbers>

namespace RC_UI::Panels::Telemetry {

void Draw(float dt)
{
    // Color the panel background using the theme palette
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorPanel);
    // The window title is "Analytics" to reflect its purpose
    ImGui::Begin("Analytics", nullptr, ImGuiWindowFlags_NoCollapse);

    // TODO: Bind real metrics here. The following reactive bar animates using Pulse().
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 120.0f);

    static ReactiveF fill;
    fill.target = Pulse(static_cast<float>(ImGui::GetTime()), 1.2f);
    fill.Update(dt);

    const ImVec2 bar_end(start.x + size.x, start.y + size.y);
    draw_list->AddRectFilled(start, bar_end, ImGui::ColorConvertFloat4ToU32(Palette::Nebula), 18.0f);

    const float fill_height = size.y * (0.2f + 0.7f * fill.v);
    const ImVec2 fill_start(start.x + 8.0f, bar_end.y - fill_height - 8.0f);
    const ImVec2 fill_end(bar_end.x - 8.0f, bar_end.y - 8.0f);
    draw_list->AddRectFilled(fill_start, fill_end, ImGui::ColorConvertFloat4ToU32(ZoneTelemetry.accent), 14.0f);

    ImGui::Dummy(size);
    ImGui::Text("Flow Rate");
    ImGui::TextColored(ColorAccentB, "%.2f", fill.v);

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* selected = AxiomEditor::GetSelectedAxiom();
        if (!selected) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.9f), "No axiom selected.");
        } else {
            const auto infos = RogueCity::App::GetAxiomTypeInfos();
            int current_index = 0;
            for (int i = 0; i < static_cast<int>(infos.size()); ++i) {
                if (infos[static_cast<size_t>(i)].type == selected->type()) {
                    current_index = i;
                    break;
                }
            }

            bool modified = false;

            if (ImGui::BeginCombo("Type", infos[static_cast<size_t>(current_index)].name)) {
                for (int i = 0; i < static_cast<int>(infos.size()); ++i) {
                    const bool is_selected = (i == current_index);
                    if (ImGui::Selectable(infos[static_cast<size_t>(i)].name, is_selected)) {
                        selected->set_type(infos[static_cast<size_t>(i)].type);
                        modified = true;
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            float radius = selected->radius();
            if (ImGui::DragFloat("Radius", &radius, 1.0f, 50.0f, 1000.0f, "%.0fm")) {
                selected->set_radius(radius);
                modified = true;
            }

            float pos[2] = { static_cast<float>(selected->position().x), static_cast<float>(selected->position().y) };
            if (ImGui::DragFloat2("Position", pos, 1.0f, 0.0f, 2000.0f, "%.0f")) {
                selected->set_position(RogueCity::Core::Vec2(pos[0], pos[1]));
                modified = true;
            }

            float theta = selected->rotation();
            if (ImGui::DragFloat("Theta", &theta, 0.01f, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f")) {
                selected->set_rotation(theta);
                modified = true;
            }

            switch (selected->type()) {
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::Organic: {
                float curv = selected->organic_curviness();
                if (ImGui::SliderFloat("Curviness", &curv, 0.0f, 1.0f)) {
                    selected->set_organic_curviness(curv);
                    modified = true;
                }
                break;
            }
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::Radial: {
                int spokes = selected->radial_spokes();
                if (ImGui::SliderInt("Spokes", &spokes, 3, 24)) {
                    selected->set_radial_spokes(spokes);
                    modified = true;
                }
                break;
            }
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::LooseGrid: {
                float jitter = selected->loose_grid_jitter();
                if (ImGui::SliderFloat("Jitter", &jitter, 0.0f, 1.0f)) {
                    selected->set_loose_grid_jitter(jitter);
                    modified = true;
                }
                break;
            }
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::Suburban: {
                float loop = selected->suburban_loop_strength();
                if (ImGui::SliderFloat("Loop Strength", &loop, 0.0f, 1.0f)) {
                    selected->set_suburban_loop_strength(loop);
                    modified = true;
                }
                break;
            }
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::Stem: {
                float branch = selected->stem_branch_angle();
                if (ImGui::SliderAngle("Branch Angle", &branch, 0.0f, 90.0f)) {
                    selected->set_stem_branch_angle(branch);
                    modified = true;
                }
                break;
            }
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::Superblock: {
                float block = selected->superblock_block_size();
                if (ImGui::DragFloat("Block Size", &block, 1.0f, 50.0f, 600.0f, "%.0fm")) {
                    selected->set_superblock_block_size(block);
                    modified = true;
                }
                break;
            }
            default:
                break;
            }

            if (modified) {
                AxiomEditor::MarkAxiomChanged();
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Panels::Telemetry
