#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <vector>
#include <memory>

struct ImDrawList;

namespace RogueCity::App {

struct RingControlKnob;
class AxiomAnimationController;

/// Visual representation of an axiom using ImDesignManager ShapeItem
/// Implements Cockpit Doctrine: reactive, affordance-rich control surface
class AxiomVisual {
public:
    using AxiomType = Generators::CityGenerator::AxiomInput::Type;

    struct Ring {
        float radius;          // Current radius (meters)
        float target_radius;   // Target radius (for animation)
        float opacity;         // Fade in/out
        bool is_animating;     // Expansion animation active
    };

    AxiomVisual(int id, AxiomType type);
    ~AxiomVisual();

    /// Update visual state (animation, hover, etc.)
    void update(float delta_time);

    /// Render to ImGui draw list
    void render(ImDrawList* draw_list, const class PrimaryViewport& viewport);

    /// Mouse interaction
    [[nodiscard]] bool is_hovered(const Core::Vec2& world_pos, float world_radius) const;
    [[nodiscard]] RingControlKnob* get_hovered_knob(const Core::Vec2& world_pos);
    void set_hovered(bool hovered);
    void set_selected(bool selected);

    /// Axiom properties
    void set_position(const Core::Vec2& pos);
    void set_radius(float radius);
    void set_type(AxiomType type);
    void set_rotation(float theta);

    // Type-specific parameters
    void set_organic_curviness(float value);
    void set_radial_spokes(int spokes);
    void set_loose_grid_jitter(float value);
    void set_suburban_loop_strength(float value);
    void set_stem_branch_angle(float radians);
    void set_superblock_block_size(float meters);

    [[nodiscard]] int id() const;
    [[nodiscard]] const Core::Vec2& position() const;
    [[nodiscard]] float radius() const;
    [[nodiscard]] AxiomType type() const;
    [[nodiscard]] float rotation() const;

    [[nodiscard]] float organic_curviness() const;
    [[nodiscard]] int radial_spokes() const;
    [[nodiscard]] float loose_grid_jitter() const;
    [[nodiscard]] float suburban_loop_strength() const;
    [[nodiscard]] float stem_branch_angle() const;
    [[nodiscard]] float superblock_block_size() const;

    /// Animation control
    void trigger_placement_animation();  // Ring expansion on place
    void set_animation_enabled(bool enabled);

    /// Convert to generator input
    [[nodiscard]] Generators::CityGenerator::AxiomInput to_axiom_input() const;

    //this private helper function computes the current decay parameter based on the ring distribution (used for real-time preview generation) needs to be incorporated into the generator bridge if the bridge is responsible for this logic. 
//todo check the usefulness of-> [[nodiscard]] double compute_decay() const;
private:
    int id_;
    AxiomType type_; // Determines how the axiom influences generation (radial, grid, etc.)
    Core::Vec2 position_{ 0.0, 0.0 }; // World coordinates (meters)
    float rotation_{ 0.0 }; // Radians, for directional axioms (e.g. stem branch angle)
    float decay_{ 2.0 }; // Overall influence decay (computed from rings, affects generation)

    float organic_curviness_{ 0.5f }; // For grid axioms: 0 = strict grid, 1 = fully organic 

    int radial_spokes_{ 8 }; // For radial axioms: number of spokes (0 for pure circle influence)

    float loose_grid_jitter_{ 0.15f }; // For grid axioms: max random offset as fraction of cell size (0 = perfect grid, 1 = max jitter)

    float suburban_loop_strength_{ 0.7f }; // For suburban axioms: 0 = no loops (strict tree), 1 = strong loops (more like organic)

    float stem_branch_angle_{ 0.7f }; // For stem axioms: angle in radians between main stem and branches (e.g. 0.7 ~ 40 degrees)

    float superblock_block_size_{ 250.0f }; // For superblock axioms: size of each block in meters
//todo consider if the ring parameters should be exposed for user editing or if they should be fixed and only used for internal decay computation. If exposed, we would need to add getters/setters for each ring radius to possibly allow independent control of each ring's influence on the decay calculation.
    Ring rings_[3];  // Immediate, Medium, Far influence
    std::vector<std::unique_ptr<RingControlKnob>> knobs_;
    std::unique_ptr<AxiomAnimationController> animator_;

    bool hovered_{ false };
    bool selected_{ false };
    bool animation_enabled_{ true };
};

/// Control knob for adjusting ring radius with double-click popup support
struct RingControlKnob {
    int ring_index;                  // 0, 1, 2
    Core::Vec2 world_position;       // Position on ring
    float angle;                     // Angle on ring (radians)
    float value;                     // Ring radius multiplier [0.33, 0.67, 1.0]
    bool is_hovered;
    bool is_dragging;
    float last_click_time;           // For double-click detection

    /// Render knob to draw list
    void render(ImDrawList* draw_list, const class PrimaryViewport& viewport);

    /// Check if mouse is over knob
    [[nodiscard]] bool check_hover(const Core::Vec2& world_pos, float world_radius);
    
    /// Drag state management
    void start_drag();
    void end_drag();
    void update_value(float new_value);
};

} // namespace RogueCity::App
