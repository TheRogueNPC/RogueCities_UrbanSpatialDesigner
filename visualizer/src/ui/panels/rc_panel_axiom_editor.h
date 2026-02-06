// FILE: rc_panel_axiom_editor.h
#pragma once

#include <cstdint>

namespace RogueCity::App {
class AxiomPlacementTool;
class RealTimePreview;
class AxiomVisual;
}

namespace RC_UI::Panels::AxiomEditor {

/// Initialize axiom editor resources (viewports, tools)
void Initialize();

/// Cleanup axiom editor resources
void Shutdown();

/// Render axiom editor panel with integrated viewport and tool
void Draw(float dt);

// Cross-panel access (Analytics + Tools strip)
[[nodiscard]] RogueCity::App::AxiomPlacementTool* GetAxiomTool();
[[nodiscard]] RogueCity::App::RealTimePreview* GetPreview();
[[nodiscard]] RogueCity::App::AxiomVisual* GetSelectedAxiom();

[[nodiscard]] bool IsLivePreviewEnabled();
void SetLivePreviewEnabled(bool enabled);

[[nodiscard]] float GetDebounceSeconds();
void SetDebounceSeconds(float seconds);

[[nodiscard]] uint32_t GetSeed();
void SetSeed(uint32_t seed);
void RandomizeSeed();

[[nodiscard]] bool CanGenerate();
void ForceGenerate();
void MarkAxiomChanged();
[[nodiscard]] const char* GetValidationError();

} // namespace RC_UI::Panels::AxiomEditor
