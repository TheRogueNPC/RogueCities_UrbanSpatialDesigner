#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <vector>
#include <memory>

struct ImDrawList;

namespace RogueCity::App {

class AxiomAnimationController;

enum class LatticeTopology {
    BezierPatch, // 4x4 Grid of control points (Organic, LooseGrid)
    Polygon,     // N-sided shape (Grid, Hexagonal, Superblock)
    Radial,      // Center + spoke points (Radial, Suburban)
    Linear       // Spine points with width handles (Stem, Linear)
};

struct ControlVertex {
    int id{ 0 };
    Core::Vec2 world_pos{};
    Core::Vec2 uv{}; // Normalized coordinate within the local axiom space.
    bool is_hovered{ false };
    bool is_dragging{ false };
};

struct ControlLattice {
    LatticeTopology topology{ LatticeTopology::Polygon };
    std::vector<ControlVertex> vertices{};
    int rows{ 0 }; // Used by BezierPatch.
    int cols{ 0 }; // Used by BezierPatch.

    // Conceptual replacement for the former 3 rings.
    float zone_inner_uv{ 0.33f };
    float zone_middle_uv{ 0.67f };
    float zone_outer_uv{ 1.0f };
};

/// Visual representation of an axiom using ImDesignManager ShapeItem
/// Implements Cockpit Doctrine: reactive, affordance-rich control surface
class AxiomVisual {
public:
    using AxiomType = Generators::CityGenerator::AxiomInput::Type;

    AxiomVisual(int id, AxiomType type);
    ~AxiomVisual();

    /// Update visual state (animation, hover, etc.)
    void update(float delta_time);

    /// Render to ImGui draw list
    void render(ImDrawList* draw_list, const class PrimaryViewport& viewport);

    /// Mouse interaction
    [[nodiscard]] bool is_hovered(const Core::Vec2& world_pos, float world_radius) const;
    [[nodiscard]] ControlVertex* get_hovered_vertex(const Core::Vec2& world_pos);
    [[nodiscard]] const ControlLattice& lattice() const;
    [[nodiscard]] ControlLattice& lattice();
    [[nodiscard]] bool update_vertex_world_position(int vertex_id, const Core::Vec2& world_pos);
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
    void initialize_lattice_for_type();
    void refresh_zone_defaults_from_type();
    void clear_vertex_interaction_flags();
    void recalculate_radius_from_lattice();

    int id_;
    AxiomType type_; // Determines how the axiom influences generation (radial, grid, etc.)
    Core::Vec2 position_{ 0.0, 0.0 }; // World coordinates (meters)
    float rotation_{ 0.0 }; // Radians, for directional axioms (e.g. stem branch angle)
    float radius_{ 300.0f };
    float decay_{ 2.0 }; // Overall influence decay (affects generation)

    float organic_curviness_{ 0.5f }; // For grid axioms: 0 = strict grid, 1 = fully organic 

    int radial_spokes_{ 8 }; // For radial axioms: number of spokes (0 for pure circle influence)

    float loose_grid_jitter_{ 0.15f }; // For grid axioms: max random offset as fraction of cell size (0 = perfect grid, 1 = max jitter)

    float suburban_loop_strength_{ 0.7f }; // For suburban axioms: 0 = no loops (strict tree), 1 = strong loops (more like organic)

    float stem_branch_angle_{ 0.7f }; // For stem axioms: angle in radians between main stem and branches (e.g. 0.7 ~ 40 degrees)

    float superblock_block_size_{ 250.0f }; // For superblock axioms: size of each block in meters
    ControlLattice lattice_{};
    std::unique_ptr<AxiomAnimationController> animator_;

    float lattice_animation_alpha_{ 1.0f };
    bool hovered_{ false };
    bool selected_{ false };
    bool animation_enabled_{ true };
};

} // namespace RogueCity::App
