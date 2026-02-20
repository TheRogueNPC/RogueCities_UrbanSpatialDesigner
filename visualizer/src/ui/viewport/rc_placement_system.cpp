#include "ui/viewport/rc_placement_system.h"

namespace RC_UI::Viewport {

PlacementGenerationPlan BuildPlacementGenerationPlan(
    const RogueCity::Core::Editor::DirtyLayerState& dirty_layers) {
    using RogueCity::Core::Editor::DirtyLayer;
    using RogueCity::Generators::GenerationStage;
    using RogueCity::Generators::MarkStageDirty;

    PlacementGenerationPlan plan{};
    if (dirty_layers.IsDirty(DirtyLayer::Axioms)) {
        plan.use_incremental = false;
        plan.dirty_stages = RogueCity::Generators::FullStageMask();
        return plan;
    }

    if (dirty_layers.IsDirty(DirtyLayer::Tensor)) {
        MarkStageDirty(plan.dirty_stages, GenerationStage::TensorField);
    }
    if (dirty_layers.IsDirty(DirtyLayer::Roads)) {
        MarkStageDirty(plan.dirty_stages, GenerationStage::Roads);
    }
    if (dirty_layers.IsDirty(DirtyLayer::Districts)) {
        MarkStageDirty(plan.dirty_stages, GenerationStage::Districts);
    }
    if (dirty_layers.IsDirty(DirtyLayer::Lots)) {
        MarkStageDirty(plan.dirty_stages, GenerationStage::Lots);
    }
    if (dirty_layers.IsDirty(DirtyLayer::Buildings)) {
        MarkStageDirty(plan.dirty_stages, GenerationStage::Buildings);
    }

    plan.use_incremental = plan.dirty_stages.any();
    return plan;
}

} // namespace RC_UI::Viewport

