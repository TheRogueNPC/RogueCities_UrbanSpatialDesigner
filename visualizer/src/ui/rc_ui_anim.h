// FILE: visualizer/src/ui/rc_ui_anim.h (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Lightweight reactive animation helpers.
#pragma once
// Header file for simple animation utilities for UI elements, including exponential smoothing and oscillators for pulsing effects.
namespace RC_UI {

float Damp(float current, float target, float lambda, float dt);
float Pulse(float t, float speed);
float PingPong(float t);

struct ReactiveF {
    float v = 0.0f;
    float target = 0.0f;
    float lambda = 8.0f;
    void Update(float dt);
};

} // namespace RC_UI
