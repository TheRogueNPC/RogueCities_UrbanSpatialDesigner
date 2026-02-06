#pragma once

namespace RogueCity::App {

class AxiomVisual;

/// Controls ring expansion and pulse animations for AxiomVisual
/// Implements affordance patterns: rings expand to teach influence radius
class AxiomAnimationController {
public:
    enum class AnimationState {
        Idle,
        Expanding,    // Ring expansion on placement
        Pulsing       // Continuous subtle pulse
    };

    AxiomAnimationController();
    ~AxiomAnimationController();

    /// Start expansion animation (0.8s default)
    void start_expansion(float duration = 0.8f, float max_radius = 300.0f);

    /// Start continuous pulse animation
    void start_pulse();

    /// Stop all animations
    void stop();

    /// Update animations (call per frame)
    void update(float delta_time);

    /// Get current animation values
    [[nodiscard]] float get_current_radius() const;
    [[nodiscard]] float get_pulse_scale() const;
    [[nodiscard]] bool is_animating() const;

private:
    AnimationState state_{ AnimationState::Idle };
    float time_{ 0.0f };
    float duration_{ 0.8f };
    float current_radius_{ 0.0f };
    float max_radius_{ 300.0f };
    float pulse_scale_{ 1.0f };
};

} // namespace RogueCity::App
