/**
 * @class BuildingTool
 * @brief Placeholder class for the building placement tool in the RogueCity application.
 *
 * This class is intended to serve as a temporary implementation for testing tool switching and viewport interaction,
 * without interfering with the axiom placement tool. The final implementation will likely follow similar patterns to
 * AxiomPlacementTool, with custom rendering and interaction logic tailored to building placement.
 *
 * @note Future plans involve merging the axiom placement tool and building placement tool into a unified
 * "Spatial Element Placement Tool". This new tool will allow users to place and edit all spatial elements (axioms,
 * buildings, and other nodal points) through a single interface. Contextual type selection will enable relevant menus
 * and editors to surface based on the selection, supporting multi-element editing in the inspector and sharing common
 * interaction and rendering logic where possible.
 *
 * BuildingTool will be refactored into this unified tool, integrating building-specific logic alongside axiom placement
 * functionality.
 *
 * @see IViewportTool
 */
 
#pragma once

#include "RogueCity/App/Tools/IViewportTool.hpp"

namespace RogueCity::App {

//this class is suppose to be a placeholder for the building placement tool that will be implemented in the future. It currently does a few things that can be used to test tool switching and viewport interaction without affecting the axiom placement tool. The actual implementation will likely involve similar patterns to AxiomPlacementTool, with custom rendering and interaction logic specific to building placement.

//todo we are looking to fuse our axiom placemtn tool with the building placement tool to create a more general "Spatial Element Placement Tool" that can handle both axioms  buildings and any other spacially charged nodal point (and potentially other elements) in a unified interface. This would allow users to place and edit all spatial elements in one tool, rather than switching between separate tools for "Axial Nodes". The new tool will make use of our current selection methods marrying them in a more unified base/class/struct  the type selection needs to be contextual allowing for the relivent menu to surface based on element held in selection. if multiples are selected then all editors relivant to the selection should be available in the inspector for editing,  sharing common interaction patterns and rendering logic where possible. The BuildingTool class will be refactored into this new unified tool, with building-specific logic integrated alongside the existing axiom placement functionality.
    class BuildingTool final : public IViewportTool {
public:
    [[nodiscard]] const char* tool_name() const override;

    void update(float delta_time, PrimaryViewport& viewport) override;
    void render(ImDrawList* draw_list, const PrimaryViewport& viewport) override;

    void on_mouse_down(const Core::Vec2& world_pos) override;
    void on_mouse_up(const Core::Vec2& world_pos) override;
    void on_mouse_move(const Core::Vec2& world_pos) override;
    void on_right_click(const Core::Vec2& world_pos) override;
};

} // namespace RogueCity::App
