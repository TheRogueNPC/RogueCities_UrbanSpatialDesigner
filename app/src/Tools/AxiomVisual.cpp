#include "RogueCity/App/Tools/AxiomVisual.hpp"
#include "RogueCity/App/Tools/AxiomAnimationController.hpp"
#include "RogueCity/App/Tools/AxiomIcon.hpp"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace RogueCity::App {

// AxiomVisual Implementation
AxiomVisual::AxiomVisual(int id, AxiomType type)
    : id_(id)
    , type_(type)
    , animator_(std::make_unique<AxiomAnimationController>())
{
    // Initialize rings with default radii
    rings_[0] = { 100.0f, 100.0f, 0.0f, false };  // Inner
    rings_[1] = { 200.0f, 200.0f, 0.0f, false };  // Middle
    rings_[2] = { 300.0f, 300.0f, 0.0f, false };  // Outer

    // Create control knobs at 90° intervals for each ring
    for (int ring_idx = 0; ring_idx < 3; ++ring_idx) {
        for (int i = 0; i < 4; ++i) {
            auto knob = std::make_unique<RingControlKnob>();
            knob->ring_index = ring_idx;
            knob->angle = (std::numbers::pi_v<float> / 2.0f) * static_cast<float>(i);  // 0°, 90°, 180°, 270°
            knob->value = 1.0f;
            knob->is_hovered = false;
            knob->is_dragging = false;
            knob->last_click_time = 0.0f;
            knobs_.push_back(std::move(knob));
        }
    }
}

AxiomVisual::~AxiomVisual() = default;

void AxiomVisual::update(float delta_time) {
    if (!animation_enabled_) return;

    // Update ring animations
    for (auto& ring : rings_) {
        if (ring.is_animating) {
            // Lerp radius towards target
            const float blend_speed = 3.0f;
            ring.radius += (ring.target_radius - ring.radius) * (1.0f - expf(-blend_speed * delta_time));
            
            // Fade in opacity during expansion
            ring.opacity += 2.0f * delta_time;
            if (ring.opacity >= 1.0f) {
                ring.opacity = 1.0f;
                ring.is_animating = false;
            }
        }
    }

    // Update knob positions based on ring radii
    for (auto& knob : knobs_) {
        const float ring_radius = rings_[knob->ring_index].radius;
        knob->world_position.x = position_.x + ring_radius * cosf(knob->angle);
        knob->world_position.y = position_.y + ring_radius * sinf(knob->angle);
    }
}

void AxiomVisual::render(ImDrawList* draw_list, const PrimaryViewport& viewport) {
    // Render rings (from outer to inner for proper layering)
    const ImU32 ring_colors[] = {
        IM_COL32(255, 200, 0, 255),   // Inner: Amber
        IM_COL32(0, 255, 200, 255),   // Middle: Cyan
        IM_COL32(255, 0, 200, 255)    // Outer: Magenta
    };

    for (int i = 2; i >= 0; --i) {
        const auto& ring = rings_[i];
        if (ring.opacity <= 0.0f) continue;

        const ImVec2 screen_center = viewport.world_to_screen(position_);
        const float screen_radius = viewport.world_to_screen_scale(ring.radius);
        
        const ImU32 color_with_alpha = IM_COL32(
            (ring_colors[i] >> IM_COL32_R_SHIFT) & 0xFF,
            (ring_colors[i] >> IM_COL32_G_SHIFT) & 0xFF,
            (ring_colors[i] >> IM_COL32_B_SHIFT) & 0xFF,
            static_cast<int>(ring.opacity * 200)  // Semi-transparent
        );

        // Y2K Style: Hard geometric circles, not soft gradients
        draw_list->AddCircle(screen_center, screen_radius, color_with_alpha, 64, 2.0f);
        
        // Add subtle pulsing inner fill when animating
        if (ring.is_animating) {
            const ImU32 fill_color = IM_COL32(
                (ring_colors[i] >> IM_COL32_R_SHIFT) & 0xFF,
                (ring_colors[i] >> IM_COL32_G_SHIFT) & 0xFF,
                (ring_colors[i] >> IM_COL32_B_SHIFT) & 0xFF,
                static_cast<int>(ring.opacity * 40)  // Very subtle fill
            );
            draw_list->AddCircleFilled(screen_center, screen_radius, fill_color, 64);
        }
    }

    // Render center marker (Y2K capsule design)
    {
        const ImVec2 screen_center = viewport.world_to_screen(position_);
        const float marker_size = hovered_ ? 10.0f : 8.0f;
        const ImU32 marker_color = selected_ ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255);
        
        // Capsule: circle with cross
        draw_list->AddCircleFilled(screen_center, marker_size, IM_COL32(0, 0, 0, 180), 16);
        draw_list->AddCircle(screen_center, marker_size, marker_color, 16, 2.0f);
        
        // Cross hairs
        draw_list->AddLine(
            ImVec2(screen_center.x - marker_size * 0.6f, screen_center.y),
            ImVec2(screen_center.x + marker_size * 0.6f, screen_center.y),
            marker_color, 1.5f
        );
        draw_list->AddLine(
            ImVec2(screen_center.x, screen_center.y - marker_size * 0.6f),
            ImVec2(screen_center.x, screen_center.y + marker_size * 0.6f),
            marker_color, 1.5f
        );

        // Type glyph (SVG-inspired, ImDrawList-native)
        const ImU32 icon_color = GetAxiomTypeInfo(type_).primary_color;
        DrawAxiomIcon(draw_list, screen_center, marker_size * 0.70f, type_, icon_color);
    }

    // Render control knobs
    for (auto& knob : knobs_) {
        knob->render(draw_list, viewport);
    }
}

bool AxiomVisual::is_hovered(const Core::Vec2& world_pos, float world_radius) const {
    const float dx = world_pos.x - position_.x;
    const float dy = world_pos.y - position_.y;
    const float dist_sq = dx * dx + dy * dy;
    return dist_sq <= (world_radius * world_radius);
}

RingControlKnob* AxiomVisual::get_hovered_knob(const Core::Vec2& world_pos) {
    const float knob_world_radius = 5.0f;  // 5 meters interaction radius
    
    for (auto& knob : knobs_) {
        if (knob->check_hover(world_pos, knob_world_radius)) {
            return knob.get();
        }
    }
    return nullptr;
}

void AxiomVisual::set_hovered(bool hovered) {
    hovered_ = hovered;
}

void AxiomVisual::set_selected(bool selected) {
    selected_ = selected;
}

void AxiomVisual::set_position(const Core::Vec2& pos) {
    position_ = pos;
}

void AxiomVisual::set_radius(float radius) {
    // Set all rings proportionally
    rings_[0].radius = radius * 0.33f;
    rings_[1].radius = radius * 0.67f;
    rings_[2].radius = radius;
}

void AxiomVisual::set_type(AxiomType type) {
    type_ = type;
}

void AxiomVisual::set_rotation(float theta) {
    rotation_ = theta;
}

int AxiomVisual::id() const { return id_; }
const Core::Vec2& AxiomVisual::position() const { return position_; }
float AxiomVisual::radius() const { return rings_[2].radius; }
AxiomVisual::AxiomType AxiomVisual::type() const { return type_; }
float AxiomVisual::rotation() const { return rotation_; }

void AxiomVisual::set_organic_curviness(float value) { organic_curviness_ = std::clamp(value, 0.0f, 1.0f); }
void AxiomVisual::set_radial_spokes(int spokes) { radial_spokes_ = std::max(3, std::min(spokes, 24)); }
void AxiomVisual::set_loose_grid_jitter(float value) { loose_grid_jitter_ = std::clamp(value, 0.0f, 1.0f); }
void AxiomVisual::set_suburban_loop_strength(float value) { suburban_loop_strength_ = std::clamp(value, 0.0f, 1.0f); }
void AxiomVisual::set_stem_branch_angle(float radians) { stem_branch_angle_ = std::clamp(radians, 0.0f, std::numbers::pi_v<float>); }
void AxiomVisual::set_superblock_block_size(float meters) { superblock_block_size_ = std::max(50.0f, meters); }

float AxiomVisual::organic_curviness() const { return organic_curviness_; }
int AxiomVisual::radial_spokes() const { return radial_spokes_; }
float AxiomVisual::loose_grid_jitter() const { return loose_grid_jitter_; }
float AxiomVisual::suburban_loop_strength() const { return suburban_loop_strength_; }
float AxiomVisual::stem_branch_angle() const { return stem_branch_angle_; }
float AxiomVisual::superblock_block_size() const { return superblock_block_size_; }

void AxiomVisual::trigger_placement_animation() {
    if (!animation_enabled_) return;

    // Start from zero and expand to target radii
    for (auto& ring : rings_) {
        ring.radius = 0.0f;
        ring.opacity = 0.0f;
        ring.is_animating = true;
    }
}

void AxiomVisual::set_animation_enabled(bool enabled) {
    animation_enabled_ = enabled;
}

Generators::CityGenerator::AxiomInput AxiomVisual::to_axiom_input() const {
    Generators::CityGenerator::AxiomInput input;
    input.type = type_;
    input.position = position_;
    input.radius = rings_[2].radius;  // Use outer ring as primary radius
    input.theta = rotation_;
    input.decay = decay_;
    input.organic_curviness = organic_curviness_;
    input.radial_spokes = radial_spokes_;
    input.loose_grid_jitter = loose_grid_jitter_;
    input.suburban_loop_strength = suburban_loop_strength_;
    input.stem_branch_angle = stem_branch_angle_;
    input.superblock_block_size = superblock_block_size_;
    return input;
}

// RingControlKnob Implementation
void RingControlKnob::render(ImDrawList* draw_list, const PrimaryViewport& viewport) {
    const ImVec2 screen_pos = viewport.world_to_screen(world_position);
    const float knob_size = is_hovered ? 8.0f : 6.0f;

    // Y2K Capsule design: Rounded rect with hard edges
    const ImU32 knob_color = is_dragging ? IM_COL32(255, 255, 0, 255) : 
                             is_hovered  ? IM_COL32(255, 255, 255, 255) :
                                           IM_COL32(180, 180, 180, 255);
    
    // Background capsule
    draw_list->AddCircleFilled(screen_pos, knob_size, IM_COL32(0, 0, 0, 200), 12);
    
    // Outer ring
    draw_list->AddCircle(screen_pos, knob_size, knob_color, 12, 2.0f);
    
    // Inner dot (affordance indicator)
    draw_list->AddCircleFilled(screen_pos, knob_size * 0.4f, knob_color, 8);
}

bool RingControlKnob::check_hover(const Core::Vec2& world_pos, float world_radius) {
    const float dx = world_pos.x - world_position.x;
    const float dy = world_pos.y - world_position.y;
    const float dist_sq = dx * dx + dy * dy;
    is_hovered = dist_sq <= (world_radius * world_radius);
    return is_hovered;
}

void RingControlKnob::start_drag() {
    is_dragging = true;
}

void RingControlKnob::end_drag() {
    is_dragging = false;
}

void RingControlKnob::update_value(float new_value) {
    value = std::clamp(new_value, 0.0f, 2.0f);  // 0-200% range
}

} // namespace RogueCity::App
