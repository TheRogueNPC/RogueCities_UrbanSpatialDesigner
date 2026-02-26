/**
 * @class AxiomAnimationController
 * @brief Controls ring expansion and pulse animations for AxiomVisual.
 *
 * Implements affordance patterns where rings expand to visually indicate influence radius.
 * Provides animation states for idle, expansion, and pulsing effects.
 *
 * @enum AnimationState
 *   - Idle: No animation is active.
 *   - Expanding: Ring expansion animation triggered on placement.
 *   - Pulsing: Continuous subtle pulse animation.
 *
 * @constructor AxiomAnimationController()
 *   Initializes the animation controller.
 *
 * @destructor ~AxiomAnimationController()
 *   Cleans up resources used by the animation controller.
 *
 * @fn void start_expansion(float duration = 0.8f, float max_radius = 300.0f)
 *   Starts the ring expansion animation with specified duration and maximum radius.
 *
 * @fn void start_pulse()
 *   Starts the continuous pulse animation.
 *
 * @fn void stop()
 *   Stops all ongoing animations and resets state.
 *
 * @fn void update(float delta_time)
 *   Updates the animation state and values. Should be called once per frame.
 *
 * @fn float get_current_radius() const
 *   Returns the current radius value of the animation.
 *
 * @fn float get_pulse_scale() const
 *   Returns the current pulse scale value.
 *
 * @fn bool is_animating() const
 *   Returns true if any animation is currently active.
 *
 * @private
 *   AnimationState state_: Current animation state.
 *   float time_: Elapsed time for the current animation.
 *   float duration_: Duration of the expansion animation.
 *   float current_radius_: Current radius value.
 *   float max_radius_: Maximum radius for expansion.
 *   float pulse_scale_: Current scale for pulsing animation.
 */
 
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
