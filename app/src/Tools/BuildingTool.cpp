#include "RogueCity/App/Tools/BuildingTool.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/SelectionSync.hpp"
#include "RogueCity/Generators/Urban/PolygonUtil.hpp"
#include <imgui.h>

namespace RogueCity::App {

const char *BuildingTool::tool_name() const { return "BuildingTool"; }

void BuildingTool::update(float delta_time, PrimaryViewport &viewport) {
  (void)delta_time;
  (void)viewport;
}

void BuildingTool::render(ImDrawList *draw_list,
                          const PrimaryViewport &viewport) {
  (void)draw_list;
  (void)viewport;
}

void BuildingTool::on_mouse_down(const Core::Vec2 &world_pos) {
  auto &gs = Core::Editor::GetGlobalState();
  const bool additive_select = ImGui::GetIO().KeyShift;

  // Clear selection unless Shift is held
  if (!additive_select) {
    gs.selection_manager.Clear();
  }

  // Find building under cursor using footprint polygons
  for (const auto &site : gs.buildings) {
    if (site.outline.size() >= 3) {
      if (Generators::Urban::PolygonUtil::insidePolygon(world_pos,
                                                        site.outline)) {
        if (additive_select) {
          gs.selection_manager.Add(Core::Editor::VpEntityKind::Building, site.id);
        } else {
          gs.selection_manager.Select(Core::Editor::VpEntityKind::Building,
                                      site.id);
        }
        Core::Editor::SyncPrimarySelectionFromManager(gs);
        // Select the first one found that hits
        return;
      }
    }
  }

  if (!additive_select) {
    Core::Editor::SyncPrimarySelectionFromManager(gs);
  }
}

void BuildingTool::on_mouse_up(const Core::Vec2 &world_pos) { (void)world_pos; }

void BuildingTool::on_mouse_move(const Core::Vec2 &world_pos) {
  (void)world_pos;
}

void BuildingTool::on_right_click(const Core::Vec2 &world_pos) {
  (void)world_pos;
}

} // namespace RogueCity::App
