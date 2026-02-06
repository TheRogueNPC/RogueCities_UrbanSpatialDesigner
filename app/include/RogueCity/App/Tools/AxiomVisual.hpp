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

private:
    int id_;
    AxiomType type_;
    Core::Vec2 position_{ 0.0, 0.0 };
    float rotation_{ 0.0 };
    float decay_{ 2.0 };

    float organic_curviness_{ 0.5f };
    int radial_spokes_{ 8 };
    float loose_grid_jitter_{ 0.15f };
    float suburban_loop_strength_{ 0.7f };
    float stem_branch_angle_{ 0.7f };
    float superblock_block_size_{ 250.0f };

    Ring rings_[3];  // Immediate, Medium, Far influence
    std::vector<std::unique_ptr<RingControlKnob>> knobs_;
    std::unique_ptr<AxiomAnimationController> animator_;

    bool hovered_{ false };
    bool selected_{ false };
    bool animation_enabled_{ true };
};

/// Control knob for adjusting ring radius
/// Y2K capsule design with double-click popup support
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
};

} // namespace RogueCity::App
