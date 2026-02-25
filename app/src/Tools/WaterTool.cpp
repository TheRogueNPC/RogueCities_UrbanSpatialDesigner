#include "RogueCity/App/Tools/WaterTool.hpp"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/EditorUtils.hpp"
#include <imgui.h>

namespace RogueCity::App {

namespace {
    Tools::SplineManipulatorParams GetWaterParams(
        const Core::Editor::GlobalState& gs, 
        const PrimaryViewport* viewport,
        const Core::WaterBody& water) {
        
        Tools::SplineManipulatorParams params{};
        params.closed = (water.type != Core::WaterType::River);
        params.snap_size = gs.params.snap_to_grid ? gs.params.snap_size : 0.0f;
        
        const float zoom = viewport ? viewport->world_to_screen_scale(1.0f) : 1.0f;
        params.pick_radius = 8.0 / static_cast<double>(zoom);
        
        params.tension = gs.spline_editor.tension;
        params.samples_per_segment = gs.spline_editor.samples_per_segment;
        
        // Special falloff modes for Water
        using Core::Editor::WaterSubtool;
        if (gs.tool_runtime.water_subtool == WaterSubtool::Erode ||
            gs.tool_runtime.water_subtool == WaterSubtool::Contour ||
            gs.tool_runtime.water_subtool == WaterSubtool::Flow) {
            
            params.falloff_radius = 50.0f; // TODO: Pull from GeometryPolicy
            params.falloff_strength = (gs.tool_runtime.water_subtool == WaterSubtool::Erode) ? 0.72f : 0.9f;
        }
        
        return params;
    }
}

const char* WaterTool::tool_name() const {
    return "WaterTool";
}

void WaterTool::update(float delta_time, PrimaryViewport& viewport) {
    (void)delta_time;
    viewport_ = &viewport;
}

void WaterTool::render(ImDrawList* draw_list, const PrimaryViewport& viewport) {
    (void)draw_list;
    (void)viewport;
}

void WaterTool::on_mouse_down(const Core::Vec2& world_pos) {
    auto& gs = Core::Editor::GetGlobalState();
    const auto* primary = gs.selection_manager.Primary();
    uint32_t water_id = (primary && primary->kind == Core::Editor::VpEntityKind::Water) ? primary->id : 0u;
    auto* water = water_id != 0 ? Core::Editor::FindWaterMutable(gs, water_id) : nullptr;
    
    if (water) {
        const auto params = GetWaterParams(gs, viewport_, *water);
        if (Tools::SplineManipulator::TryBeginDrag(water->boundary, world_pos, params, spline_state_)) {
            spline_state_.entity_id = water->id;
        } else if (gs.tool_runtime.water_spline_subtool == Core::Editor::WaterSplineSubtool::Pen) {
            water->boundary.push_back(world_pos);
            gs.dirty_layers.MarkDirty(Core::Editor::DirtyLayer::ViewportIndex);
        }
    }
}

void WaterTool::on_mouse_up(const Core::Vec2& world_pos) {
    (void)world_pos;
    spline_state_.active = false;
}

void WaterTool::on_mouse_move(const Core::Vec2& world_pos) {
    if (!spline_state_.active) return;
    
    auto& gs = Core::Editor::GetGlobalState();
    auto* water = Core::Editor::FindWaterMutable(gs, spline_state_.entity_id);
    
    if (water) {
        const auto params = GetWaterParams(gs, viewport_, *water);
        if (Tools::SplineManipulator::UpdateDrag(water->boundary, world_pos, params, spline_state_)) {
            // Water changes don't currently mark a generator layer dirty because 
            // they are decorative/environment, but we should mark viewport index.
            gs.dirty_layers.MarkDirty(Core::Editor::DirtyLayer::ViewportIndex);
        }
    }
}

void WaterTool::on_right_click(const Core::Vec2& world_pos) {
    auto& gs = Core::Editor::GetGlobalState();
    const auto* primary = gs.selection_manager.Primary();
    uint32_t water_id = (primary && primary->kind == Core::Editor::VpEntityKind::Water) ? primary->id : 0u;
    auto* water = water_id != 0 ? Core::Editor::FindWaterMutable(gs, water_id) : nullptr;
    
    if (water) {
        const auto params = GetWaterParams(gs, viewport_, *water);
        if (Tools::SplineManipulator::RemoveVertex(water->boundary, world_pos, params)) {
            gs.dirty_layers.MarkDirty(Core::Editor::DirtyLayer::ViewportIndex);
        }
    }
}

} // namespace RogueCity::App
