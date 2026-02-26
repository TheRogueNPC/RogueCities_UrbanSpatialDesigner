#include "ui/panels/rc_panel_river_index.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_components.h"

namespace RC_UI::Panels::RiverIndex {

void Draw(float dt) {
  (void)dt;
  auto &panel = GetPanel();
  const bool open =
      Components::BeginTokenPanel("Water Index", UITokens::InfoBlue);

  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{
          "Water Index",
          "Water Index",
          "index",
          "Bottom",
          "visualizer/src/ui/panels/rc_panel_river_index.cpp",
          {"water", "rivers", "index"}},
      open);

  if (!open) {
    uiint.EndPanel();
    Components::EndTokenPanel();
    return;
  }

  auto &gs = RogueCity::Core::Editor::GetGlobalState();
  panel.DrawContent(gs, uiint);

  uiint.EndPanel();
  Components::EndTokenPanel();
}

void DrawContent(float dt) {
  (void)dt;
  auto &gs = RogueCity::Core::Editor::GetGlobalState();
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  GetPanel().DrawContent(gs, uiint);
}

} // namespace RC_UI::Panels::RiverIndex
