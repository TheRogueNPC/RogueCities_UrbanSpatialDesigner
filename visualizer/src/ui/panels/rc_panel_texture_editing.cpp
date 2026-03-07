// FILE: rc_panel_texture_editing.cpp
// PURPOSE: Dedicated texture editing controls moved out of the Tools panel.

#include "ui/panels/rc_panel_texture_editing.h"

#include "ui/api/rc_imgui_api.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>

namespace RC_UI::Panels::TextureEditing {

void DrawContent(float /*dt*/) {
  using RogueCity::Core::Editor::TerrainBrush;
  using RogueCity::Core::Editor::TexturePainting;

  auto &gs = RogueCity::Core::Editor::GetGlobalState();

  if (API::SectionHeader("Texture Editing", UITokens::AmberGlow, true)) {
    API::Indent();

    if (gs.HasTextureSpace()) {
      static float brush_radius_m = 30.0f;
      static float brush_strength = 1.0f;
      static int zone_value = 1;
      static int material_value = 1;

      API::SetNextItemWidth(140.0f);
      API::DragFloat("Brush Radius (m)", &brush_radius_m, 1.0f, 1.0f, 250.0f,
                     "%.1f");
      API::SetNextItemWidth(140.0f);
      API::DragFloat("Brush Strength", &brush_strength, 0.05f, 0.05f, 10.0f,
                     "%.2f");

      API::SetNextItemWidth(200.0f);
      {
        double tol_d = gs.generation.streamline_major_tensor_tolerance_degrees;
        double minv = 5.0;
        double maxv = 60.0;
        if (API::SliderScalar("Major Tensor Tolerance", ImGuiDataType_Double,
                              &tol_d, &minv, &maxv, "%.1f deg")) {
          gs.generation.streamline_major_tensor_tolerance_degrees = static_cast<
              decltype(gs.generation
                           .streamline_major_tensor_tolerance_degrees)>(tol_d);
        }
      }

      const RogueCity::Core::Vec2 brush_center =
          gs.TextureSpaceRef().bounds().center();

      if (API::Button("Raise Height @ Center", ImVec2(180.0f, 0.0f))) {
        TerrainBrush::Stroke stroke{};
        stroke.world_center = brush_center;
        stroke.radius_meters = brush_radius_m;
        stroke.strength = brush_strength;
        stroke.mode = TerrainBrush::Mode::Raise;
        (void)gs.ApplyTerrainBrush(stroke);
      }
      if (API::Button("Lower Height @ Center", ImVec2(180.0f, 0.0f))) {
        TerrainBrush::Stroke stroke{};
        stroke.world_center = brush_center;
        stroke.radius_meters = brush_radius_m;
        stroke.strength = brush_strength;
        stroke.mode = TerrainBrush::Mode::Lower;
        (void)gs.ApplyTerrainBrush(stroke);
      }
      if (API::Button("Smooth Height @ Center", ImVec2(180.0f, 0.0f))) {
        TerrainBrush::Stroke stroke{};
        stroke.world_center = brush_center;
        stroke.radius_meters = brush_radius_m;
        stroke.strength = 0.6f;
        stroke.mode = TerrainBrush::Mode::Smooth;
        (void)gs.ApplyTerrainBrush(stroke);
      }

      API::SetNextItemWidth(120.0f);
      API::DragInt("Zone Value", &zone_value, 1.0f, 0, 255);
      API::SetNextItemWidth(120.0f);
      API::DragInt("Material Value", &material_value, 1.0f, 0, 255);

      if (API::Button("Paint Zone @ Center", ImVec2(180.0f, 0.0f))) {
        TexturePainting::Stroke stroke{};
        stroke.layer = TexturePainting::Layer::Zone;
        stroke.world_center = brush_center;
        stroke.radius_meters = brush_radius_m;
        stroke.opacity = 1.0f;
        stroke.value = static_cast<uint8_t>(zone_value);
        (void)gs.ApplyTexturePaint(stroke);
      }
      if (API::Button("Paint Material @ Center", ImVec2(180.0f, 0.0f))) {
        TexturePainting::Stroke stroke{};
        stroke.layer = TexturePainting::Layer::Material;
        stroke.world_center = brush_center;
        stroke.radius_meters = brush_radius_m;
        stroke.opacity = 1.0f;
        stroke.value = static_cast<uint8_t>(material_value);
        (void)gs.ApplyTexturePaint(stroke);
      }
    } else {
      API::TextDisabled("TextureSpace not initialized yet.");
    }

    API::Unindent();
  }
}

void Draw(float dt) {
  static RC_UI::DockableWindowState s_texture_window;
  if (!RC_UI::BeginDockableWindow("Texture Editing", s_texture_window,
                                  "Left", ImGuiWindowFlags_NoCollapse)) {
    return;
  }

  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{
          "Texture Editing",
          "Texture Editing",
          "texture_editing",
          "Left",
          "visualizer/src/ui/panels/rc_panel_texture_editing.cpp",
          {"texture", "editing", "terrain", "paint"}},
      true);

  DrawContent(dt);

  uiint.EndPanel();
  RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::TextureEditing
