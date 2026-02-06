#include "RogueCity/App/Tools/AxiomAnimationController.hpp"
#include <cmath>
#include <numbers>

namespace RogueCity::App {

AxiomAnimationController::AxiomAnimationController() = default;
AxiomAnimationController::~AxiomAnimationController() = default;

void AxiomAnimationController::start_expansion(float duration, float max_radius) {
    state_ = AnimationState::Expanding;
    time_ = 0.0f;
    duration_ = duration;
    max_radius_ = max_radius;
    current_radius_ = 0.0f;
}

void AxiomAnimationController::start_pulse() {
    state_ = AnimationState::Pulsing;
    time_ = 0.0f;
}

void AxiomAnimationController::stop() {
    state_ = AnimationState::Idle;
    time_ = 0.0f;
}

void AxiomAnimationController::update(float delta_time) {
    if (state_ == AnimationState::Idle) return;

    time_ += delta_time;

    switch (state_) {
    case AnimationState::Expanding: {
        // Ease-out cubic expansion
        const float t = std::min(time_ / duration_, 1.0f);
        const float ease_out = 1.0f - powf(1.0f - t, 3.0f);
        current_radius_ = max_radius_ * ease_out;
        
        if (t >= 1.0f) {
            state_ = AnimationState::Idle;
        }
        break;
    }
    case AnimationState::Pulsing: {
        // Continuous sine wave pulse (0.9 - 1.1 scale)
        const float pulse_freq = 2.0f;  // Hz
        pulse_scale_ = 1.0f + 0.1f * sinf(time_ * pulse_freq * 2.0f * std::numbers::pi_v<float>);
        break;
    }
    default:
        break;
    }
}

float AxiomAnimationController::get_current_radius() const {
    return current_radius_;
}

float AxiomAnimationController::get_pulse_scale() const {
    return pulse_scale_;
}

bool AxiomAnimationController::is_animating() const {
    return state_ != AnimationState::Idle;
}

} // namespace RogueCity::App
