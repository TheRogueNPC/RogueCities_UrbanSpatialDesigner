// FILE: rc_panel_tools.cpp
// PURPOSE: Bottom tools strip (viewport-centric cockpit controls).

#include "ui/panels/rc_panel_tools.h"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/panels/rc_panel_dev_shell.h"
#include "ui/introspection/UiIntrospection.h"
#include "integration/AiAssist.h"

#include <imgui.h>

namespace RC_UI::Panels::Tools {

void Draw(float dt)
{
    const bool open = ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoCollapse);

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Tools",
            "Tools",
            "toolbox",
            "Bottom",
            "visualizer/src/ui/panels/rc_panel_tools.cpp",
            {"generation", "controls", "cockpit"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        ImGui::End();
        return;
    }

    // Generation controls
    const bool can_generate = AxiomEditor::CanGenerate();
    if (!can_generate) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Generate / Regenerate")) {
        AxiomEditor::ForceGenerate();
    }
    uiint.RegisterWidget({"button", "Generate / Regenerate", "action:generator.regenerate", {"action", "generator"}});
    uiint.RegisterAction({"generator.regenerate", "Generate / Regenerate", "Tools", {}, "AxiomEditor::ForceGenerate"});
    if (!can_generate) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    bool live_preview = AxiomEditor::IsLivePreviewEnabled();
    if (ImGui::Checkbox("Live Preview", &live_preview)) {
        AxiomEditor::SetLivePreviewEnabled(live_preview);
    }
    uiint.RegisterWidget({"checkbox", "Live Preview", "preview.live", {"preview"}});

    ImGui::SameLine();
    float debounce = AxiomEditor::GetDebounceSeconds();
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::DragFloat("Debounce", &debounce, 0.01f, 0.0f, 1.5f, "%.2fs")) {
        AxiomEditor::SetDebounceSeconds(debounce);
    }
    uiint.RegisterWidget({"slider", "Debounce", "preview.debounce_sec", {"preview", "timing"}});

    ImGui::SameLine();
    if (ImGui::Button("Random Seed")) {
        AxiomEditor::RandomizeSeed();
        if (AxiomEditor::IsLivePreviewEnabled()) {
            AxiomEditor::ForceGenerate();
        }
    }
    uiint.RegisterWidget({"button", "Random Seed", "action:seed.randomize", {"action", "seed"}});
    uiint.RegisterAction({"seed.randomize", "Random Seed", "Tools", {}, "AxiomEditor::RandomizeSeed"});

    ImGui::SameLine();
    ImGui::Text("Seed: %u", AxiomEditor::GetSeed());
    
    // === AI ASSIST CONTROLS ===
    ImGui::Separator();
    RogueCity::UI::AiAssist::DrawControls(dt);

    ImGui::Separator();
    if (ImGui::Button("Dev Shell")) {
        RC_UI::Panels::DevShell::Toggle();
    }
    uiint.RegisterWidget({"button", "Dev Shell", "action:devshell.toggle", {"dev", "toggle"}});

    // Status / errors
    const char* err = AxiomEditor::GetValidationError();
    if (err && err[0] != '\0') {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", err);
    }

    // Input state indicators (from ImGui IO)
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Separator();
    ImGui::Text("Mouse: (%.0f, %.0f)  L:%d R:%d M:%d",
        io.MousePos.x, io.MousePos.y,
        io.MouseDown[0] ? 1 : 0,
        io.MouseDown[1] ? 1 : 0,
        io.MouseDown[2] ? 1 : 0);
    ImGui::Text("Mods: Ctrl:%d Shift:%d Alt:%d  Capture: Mouse:%d Key:%d",
        io.KeyCtrl ? 1 : 0, io.KeyShift ? 1 : 0, io.KeyAlt ? 1 : 0,
        io.WantCaptureMouse ? 1 : 0, io.WantCaptureKeyboard ? 1 : 0);

    uiint.EndPanel();
    ImGui::End();
}

} // namespace RC_UI::Panels::Tools
