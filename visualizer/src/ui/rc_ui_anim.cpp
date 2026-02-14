// FILE: visualizer/src/ui/rc_ui_anim.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Exponential smoothing and simple oscillators.
#include "ui/rc_ui_anim.h"

#include <cmath>
// Implementation of simple animation utilities for UI elements, including exponential smoothing and oscillators for pulsing effects.
namespace RC_UI {

float Damp(float current, float target, float lambda, float dt)
{
    if (dt <= 0.0f) {
        return current;
    }
    const float t = 1.0f - std::exp(-lambda * dt);
    return current + (target - current) * t;
}

float Pulse(float t, float speed)
{
    return 0.5f + 0.5f * std::sin(t * speed);
}

float PingPong(float t)
{
    const float phase = std::fmod(t, 2.0f);
    return 1.0f - std::fabs(phase - 1.0f);
}

void ReactiveF::Update(float dt)
{
    v = Damp(v, target, lambda, dt);
}

} // namespace RC_UI
