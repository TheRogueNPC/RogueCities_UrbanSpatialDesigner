#pragma once

#include "RogueCity/App/Editor/CommandHistory.hpp"
#include "RogueCity/App/Tools/IViewportTool.hpp"
#include "AxiomVisual.hpp"
#include "ContextWindowPopup.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace RogueCity::App {

class PrimaryViewport;

/// Tool for placing and editing axioms in viewport.
/// Active during EditorState::Editing_Axioms.
class AxiomPlacementTool : public IViewportTool {
public:
    struct AxiomSnapshot;

    enum class Mode {
        Idle,
        Placing,
        DraggingSize,
        DraggingAxiom,
        DraggingKnob,
        Hovering
    };

    AxiomPlacementTool();
    ~AxiomPlacementTool();

    [[nodiscard]] const char* tool_name() const override;

    /// Update tool state (call per frame when active).
    void update(float delta_time, PrimaryViewport& viewport) override;

    /// Render tool overlays (ghost preview, knobs, etc.).
    void render(ImDrawList* draw_list, const PrimaryViewport& viewport) override;

    /// Handle mouse events.
    void on_mouse_down(const Core::Vec2& world_pos) override;
    void on_mouse_up(const Core::Vec2& world_pos) override;
    void on_mouse_move(const Core::Vec2& world_pos) override;
    void on_right_click(const Core::Vec2& world_pos) override;

    /// Axiom management.
    void add_axiom(std::unique_ptr<AxiomVisual> axiom);
    void remove_axiom(int axiom_id);
    void clear_axioms();
    void add_axiom_from_snapshot(const AxiomSnapshot& snapshot);
    void apply_snapshot(const AxiomSnapshot& snapshot);

    [[nodiscard]] const std::vector<std::unique_ptr<AxiomVisual>>& axioms() const;
    [[nodiscard]] AxiomVisual* get_selected_axiom();
    [[nodiscard]] int selected_axiom_id() const { return selected_axiom_id_; }

    /// Configuration.
    void set_default_axiom_type(AxiomVisual::AxiomType type);
    [[nodiscard]] AxiomVisual::AxiomType default_axiom_type() const;
    void set_default_terminal_features(AxiomVisual::AxiomType type, const Generators::TerminalFeatureSet& features);
    [[nodiscard]] Generators::TerminalFeatureSet default_terminal_features(AxiomVisual::AxiomType type) const;
    void set_default_radial_ring_rotation(AxiomVisual::AxiomType type, float radians);
    [[nodiscard]] float default_radial_ring_rotation(AxiomVisual::AxiomType type) const;
    void set_animation_enabled(bool enabled);

    /// Convert all axioms to generator inputs.
    [[nodiscard]] std::vector<Generators::CityGenerator::AxiomInput> get_axiom_inputs() const;

    /// Returns true once when axioms changed (place/modify/delete).
    [[nodiscard]] bool consume_dirty();

    /// True while the user is manipulating the tool (placing/dragging).
    [[nodiscard]] bool is_interacting() const;

    // Undo/Redo support.
    [[nodiscard]] bool can_undo() const;
    [[nodiscard]] bool can_redo() const;
    void undo();
    void redo();
    [[nodiscard]] const char* undo_label() const;
    [[nodiscard]] const char* redo_label() const;
    void push_command(std::unique_ptr<ICommand> cmd);

    struct AxiomSnapshot {
        int id{ 0 };
        AxiomVisual::AxiomType type{ AxiomVisual::AxiomType::Radial };
        Core::Vec2 position{ 0.0, 0.0 };
        float radius{ 100.0f };
        float rotation{ 0.0f };
        float organic_curviness{ 0.5f };
        int radial_spokes{ 8 };
        float loose_grid_jitter{ 0.15f };
        float suburban_loop_strength{ 0.7f };
        float stem_branch_angle{ 0.7f };
        float superblock_block_size{ 250.0f };
        float radial_ring_rotation{ 0.0f };
        std::array<std::array<float, 4>, 3> radial_ring_knob_weights{{
            {{1.15f, 0.90f, 1.05f, 0.85f}},
            {{1.10f, 0.95f, 1.00f, 0.90f}},
            {{1.05f, 1.00f, 0.95f, 0.92f}}
        }};
        Generators::TerminalFeatureSet terminal_features{};
    };

private:
    AxiomSnapshot snapshot_axiom(const AxiomVisual& axiom) const;
    AxiomVisual* find_axiom(int axiom_id);
    AxiomVisual* find_best_axiom_for_interaction(const Core::Vec2& world_pos);
    bool resolve_core_overlap_for_axiom(AxiomVisual* edited_axiom);
    void push_non_intrusive_warning(const char* message) const;

    std::vector<std::unique_ptr<AxiomVisual>> axioms_;
    int next_axiom_id_{ 1 };
    int selected_axiom_id_{ -1 };
    int hovered_axiom_id_{ -1 };

    Mode mode_{ Mode::Idle };
    AxiomVisual::AxiomType default_type_{ AxiomVisual::AxiomType::Radial };
    std::array<Generators::TerminalFeatureSet, static_cast<size_t>(AxiomVisual::AxiomType::COUNT)> default_terminal_features_{};
    std::array<float, static_cast<size_t>(AxiomVisual::AxiomType::COUNT)> default_radial_ring_rotation_{};
    Core::Vec2 placement_start_pos_{ 0.0, 0.0 };
    float ghost_radius_{ 100.0f };

    ControlVertex* dragging_vertex_{ nullptr };
    Core::Vec2 knob_drag_start_{ 0.0, 0.0 };
    Core::Vec2 axiom_drag_offset_{ 0.0, 0.0 };

    bool animation_enabled_{ true };
    bool dirty_{ false };
    ContextWindowPopup knob_popup_;
    int popup_axiom_id_{ -1 };
    int popup_vertex_id_{ -1 };
    std::optional<AxiomSnapshot> drag_start_snapshot_{};
    CommandHistory history_{};
};

} // namespace RogueCity::App
