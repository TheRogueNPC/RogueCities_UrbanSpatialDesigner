// FILE: rc_panel_imgui_error.h
// PURPOSE: ImGui error recovery agent — configures io.ConfigErrorRecovery* flags,
//          monitors the ImGui debug log for recoverable error entries, and exposes
//          BeginProtectedSection / EndProtectedSection for wrapping risky ImGui code.
//
// Programmer-seat default (Scenario 1): all four flags ON, asserts enabled.
// Use the panel's Config section to switch to Scenario 2 (tooltip-only) at runtime.
//
// MANDATE: Any panel driving dynamic or scripted ImGui code MUST wrap that code
//          with BeginProtectedSection / EndProtectedSection.
//
// Usage example (exception boundary):
//   ImGuiError::BeginProtectedSection("MyScript");
//   try { RunScript(); }
//   catch (...) { ImGuiError::EndProtectedSection(); throw; }
//   ImGuiError::EndProtectedSection();

#pragma once

namespace RC_UI::Panels::ImGuiError {

    // Call once after ImGui::CreateContext() + ImGui::GetIO() is live.
    // Sets all four io.ConfigErrorRecovery* flags to programmer-seat defaults
    // and resets the internal error log.
    void Init();

    // Call before ImGui::DestroyContext().
    void Shutdown();

    bool IsOpen();
    void Toggle();

    // Scan the ImGui debug log for new [imgui-error] entries and render the panel.
    void DrawContent(float dt);
    void Draw(float dt);

    // Wrap risky ImGui code with these to allow recovery if the call stack is
    // corrupted (mismatched Begin/End, wrong push/pop, etc.).
    // Uses ImGui::ErrorRecoveryStoreState / ErrorRecoveryTryToRecoverState internally.
    void BeginProtectedSection(const char* label = nullptr);
    void EndProtectedSection();

    // Returns the number of [imgui-error] entries captured this session.
    // Useful for a status-bar badge or DevShell indicator.
    int GetErrorCount();

} // namespace RC_UI::Panels::ImGuiError
