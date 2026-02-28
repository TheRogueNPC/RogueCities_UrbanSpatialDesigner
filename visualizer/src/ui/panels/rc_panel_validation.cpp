// FILE: visualizer/src/ui/panels/rc_panel_validation.cpp
// PURPOSE: Live validation panel showing generator constraints and rule violations.

#include "ui/panels/rc_panel_validation.h"
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_theme.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>
#include <RogueCity/Core/Infomatrix.hpp>

#include <imgui.h>
#include <string>

namespace RC_UI::Panels::Validation {

void DrawContent(float dt) {
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();

    ImGui::BeginChild("ValidationStream", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    auto es = gs.infomatrix.events();
    using RogueCity::Core::Editor::InfomatrixEvent;

    for (const auto& ev : es.data) {
        if (ev.cat == InfomatrixEvent::Category::Validation) {
            ImU32 color = ev.msg.find("rejected") != std::string::npos ? UITokens::ErrorRed : UITokens::GreenHUD;
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
            ImGui::TextUnformatted(ev.msg.c_str());
            ImGui::PopStyleColor();
        }
    }

    ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    uiint.RegisterWidget({"tree", "ValidationStream", "validation.tail", {"validation"}});
}

void Draw(float dt) {
    const bool open = Components::BeginTokenPanel("Validation", UITokens::AmberGlow);
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Validation",
            "Validation",
            "validation",
            "Bottom",
            "visualizer/src/ui/panels/rc_panel_validation.cpp",
            {"validation", "runtime"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }

    DrawContent(dt);

    uiint.EndPanel();
    Components::EndTokenPanel();
}

int GetValidationEventCount() {
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto es = gs.infomatrix.events();
    using RogueCity::Core::Editor::InfomatrixEvent;
    int count = 0;
    for (const auto& ev : es.data) {
        if (ev.cat == InfomatrixEvent::Category::Validation) {
            ++count;
        }
    }
    return count;
}

bool HasValidationFailure() {
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto es = gs.infomatrix.events();
    using RogueCity::Core::Editor::InfomatrixEvent;
    for (const auto& ev : es.data) {
        if (ev.cat != InfomatrixEvent::Category::Validation) {
            continue;
        }
        if (ev.msg.find("rejected") != std::string::npos || ev.msg.find("ERROR") != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace RC_UI::Panels::Validation
