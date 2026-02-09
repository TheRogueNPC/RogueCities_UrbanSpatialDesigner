#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "AxiomVisual.hpp"
#include "ContextWindowPopup.hpp"
#include <vector>
#include <memory>

namespace RogueCity::App {

class PrimaryViewport;

/// Tool for placing and editing axioms in viewport
/// Active during EditorState::Editing_Axioms
class AxiomPlacementTool {
public:
    enum class Mode {
        Idle,              // No interaction
        Placing,           // Click to place new axiom
        DraggingSize,      // Dragging to set radius
        DraggingAxiom,     // Dragging axiom position
        DraggingKnob,      // Adjusting ring radius via knob
        Hovering           // Mouse over existing axiom
    };

    AxiomPlacementTool();
    ~AxiomPlacementTool();

    /// Update tool state (call per frame when active)
    void update(float delta_time, PrimaryViewport& viewport);

    /// Render tool overlays (ghost preview, knobs, etc.)
    void render(ImDrawList* draw_list, const PrimaryViewport& viewport);

    /// Handle mouse events
    void on_mouse_down(const Core::Vec2& world_pos);
    void on_mouse_up(const Core::Vec2& world_pos);
    void on_mouse_move(const Core::Vec2& world_pos);
    void on_right_click(const Core::Vec2& world_pos);  // Delete axiom

    /// Axiom management
    void add_axiom(std::unique_ptr<AxiomVisual> axiom);
    void remove_axiom(int axiom_id);
    void clear_axioms();

    [[nodiscard]] const std::vector<std::unique_ptr<AxiomVisual>>& axioms() const;
    [[nodiscard]] AxiomVisual* get_selected_axiom();

    /// Configuration
    void set_default_axiom_type(AxiomVisual::AxiomType type);
    [[nodiscard]] AxiomVisual::AxiomType default_axiom_type() const;
    void set_animation_enabled(bool enabled);

    /// Convert all axioms to generator inputs
    [[nodiscard]] std::vector<Generators::CityGenerator::AxiomInput> get_axiom_inputs() const;

    /// Returns true once when axioms changed (place/modify/delete)
    [[nodiscard]] bool consume_dirty();

    /// True while the user is manipulating the tool (placing/dragging)
    [[nodiscard]] bool is_interacting() const;

private:
    std::vector<std::unique_ptr<AxiomVisual>> axioms_;
    int next_axiom_id_{ 1 };
    int selected_axiom_id_{ -1 };
    int hovered_axiom_id_{ -1 };

    Mode mode_{ Mode::Idle };
    AxiomVisual::AxiomType default_type_{ AxiomVisual::AxiomType::Radial };
    Core::Vec2 placement_start_pos_{ 0.0, 0.0 };
    float ghost_radius_{ 100.0f };

    RingControlKnob* dragging_knob_{ nullptr };
    Core::Vec2 knob_drag_start_{ 0.0, 0.0 };
    Core::Vec2 axiom_drag_offset_{ 0.0, 0.0 };

    bool animation_enabled_{ true };
    bool dirty_{ false };
    ContextWindowPopup knob_popup_;
    int popup_axiom_id_{ -1 };
    int popup_ring_index_{ -1 };
};

} // namespace RogueCity::App
