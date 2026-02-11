#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <cassert>

using RogueCity::Core::Bounds;
using RogueCity::Core::Vec2;
using RogueCity::Core::Data::TextureLayer;
using RogueCity::Core::Editor::DirtyLayer;
using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Editor::TerrainBrush;
using RogueCity::Core::Editor::TexturePainting;

int main() {
    GlobalState gs{};
    Bounds bounds{};
    bounds.min = Vec2(0.0, 0.0);
    bounds.max = Vec2(200.0, 200.0);
    gs.InitializeTextureSpace(bounds, 64);
    gs.ClearAllTextureLayersDirty();
    gs.dirty_layers.MarkAllClean();

    TerrainBrush::Stroke raise{};
    raise.world_center = Vec2(100.0, 100.0);
    raise.radius_meters = 30.0;
    raise.strength = 2.0f;
    raise.mode = TerrainBrush::Mode::Raise;
    const bool raised = gs.ApplyTerrainBrush(raise);
    assert(raised);
    assert(gs.TextureSpaceRef().isDirty(TextureLayer::Height));
    assert(gs.dirty_layers.IsDirty(DirtyLayer::Tensor));
    assert(gs.dirty_layers.IsDirty(DirtyLayer::Roads));
    assert(gs.dirty_layers.IsDirty(DirtyLayer::ViewportIndex));
    const auto dirty_height = gs.TextureLayerDirtyRegion(TextureLayer::Height);
    assert(dirty_height.isValid());

    const auto center = gs.TextureSpaceRef().coordinateSystem().worldToPixel(raise.world_center);
    assert(gs.TextureSpaceRef().heightLayer().at(center.x, center.y) > 0.0f);
    assert(center.x >= dirty_height.min_x && center.x <= dirty_height.max_x);
    assert(center.y >= dirty_height.min_y && center.y <= dirty_height.max_y);

    gs.ClearAllTextureLayersDirty();
    gs.dirty_layers.MarkAllClean();

    TexturePainting::Stroke zone_paint{};
    zone_paint.layer = TexturePainting::Layer::Zone;
    zone_paint.world_center = Vec2(100.0, 100.0);
    zone_paint.radius_meters = 15.0;
    zone_paint.opacity = 1.0f;
    zone_paint.value = 7u;
    const bool painted_zone = gs.ApplyTexturePaint(zone_paint);
    assert(painted_zone);
    assert(gs.TextureSpaceRef().isDirty(TextureLayer::Zone));
    assert(gs.TextureSpaceRef().zoneLayer().at(center.x, center.y) == 7u);
    const auto dirty_zone = gs.TextureLayerDirtyRegion(TextureLayer::Zone);
    assert(dirty_zone.isValid());
    assert(center.x >= dirty_zone.min_x && center.x <= dirty_zone.max_x);
    assert(center.y >= dirty_zone.min_y && center.y <= dirty_zone.max_y);

    gs.ClearAllTextureLayersDirty();
    gs.dirty_layers.MarkAllClean();

    TexturePainting::Stroke material_paint{};
    material_paint.layer = TexturePainting::Layer::Material;
    material_paint.world_center = Vec2(100.0, 100.0);
    material_paint.radius_meters = 15.0;
    material_paint.opacity = 1.0f;
    material_paint.value = 123u;
    const bool painted_material = gs.ApplyTexturePaint(material_paint);
    assert(painted_material);
    assert(gs.TextureSpaceRef().isDirty(TextureLayer::Material));
    assert(gs.TextureSpaceRef().materialLayer().at(center.x, center.y) == 123u);
    const auto dirty_material = gs.TextureLayerDirtyRegion(TextureLayer::Material);
    assert(dirty_material.isValid());
    assert(center.x >= dirty_material.min_x && center.x <= dirty_material.max_x);
    assert(center.y >= dirty_material.min_y && center.y <= dirty_material.max_y);

    return 0;
}
