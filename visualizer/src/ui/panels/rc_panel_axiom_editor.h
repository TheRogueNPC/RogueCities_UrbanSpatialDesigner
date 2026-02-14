// FILE: rc_panel_axiom_editor.h
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

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
void DrawContent(float dt);

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
[[nodiscard]] float GetFlowRate();
void SetFlowRate(float flowRate);

[[nodiscard]] const char* GetRogueNavModeName();
[[nodiscard]] const char* GetRogueNavFilterName();
bool SetRogueNavModeByName(const std::string& mode);
bool SetRogueNavFilterByName(const std::string& filter);
[[nodiscard]] bool IsMinimapManualLODOverride();
void SetMinimapManualLODOverride(bool enabled);
[[nodiscard]] int GetMinimapManualLODLevel();
void SetMinimapManualLODLevel(int level);
[[nodiscard]] bool IsMinimapAdaptiveQualityEnabled();
void SetMinimapAdaptiveQualityEnabled(bool enabled);
[[nodiscard]] int GetMinimapEffectiveLODLevel();
[[nodiscard]] const char* GetMinimapLODStatusText();

[[nodiscard]] bool CanUndo();
[[nodiscard]] bool CanRedo();
void Undo();
void Redo();
[[nodiscard]] const char* GetUndoLabel();
[[nodiscard]] const char* GetRedoLabel();

[[nodiscard]] bool CanGenerate();
void ForceGenerate();
bool ApplyGeneratorRequest(
    const std::vector<RogueCity::Generators::CityGenerator::AxiomInput>& axioms,
    const RogueCity::Generators::CityGenerator::Config& config,
    std::string* outError = nullptr);
void MarkAxiomChanged();
[[nodiscard]] const char* GetValidationError();

} // namespace RC_UI::Panels::AxiomEditor
