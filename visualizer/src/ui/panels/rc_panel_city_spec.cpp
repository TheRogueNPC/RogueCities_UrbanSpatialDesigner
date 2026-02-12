// FILE: rc_panel_city_spec.cpp
// PURPOSE: CitySpec Generator for AI-driven city design

#include "rc_panel_city_spec.h"
#include "client/CitySpecClient.h"
#include "runtime/AiBridgeRuntime.h"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/panels/rc_panel_zoning_control.h"
#include "RogueCity/App/UI/DesignSystem.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Generators/Pipeline/CitySpecAdapter.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/rc_ui_components.h"
#include <imgui.h>
#include <algorithm>
#include <thread>
#include <cmath>

namespace RogueCity::UI {

static void RenderBusyIndicator(std::atomic<bool>& busyFlag, float& busyTimeSeconds) {
    if (!busyFlag.load()) return;

    busyTimeSeconds += ImGui::GetIO().DeltaTime;
    float t = (sinf(busyTimeSeconds * 3.14f) * 0.5f) + 0.5f; // 0..1
    float alpha = 0.3f + 0.7f * t;

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImGui::ColorConvertU32ToFloat4(RC_UI::WithAlpha(RC_UI::UITokens::InfoBlue, static_cast<uint8_t>(alpha * 255.0f))));
    ImGui::Text("AI processing...");
    ImGui::PopStyleColor();
    ImGui::Separator();
}

void CitySpecPanel::Render() {
    RC_UI::ApplyUnifiedWindowSchema();
    const bool open = RC_UI::Components::BeginTokenPanel(
        "City Spec Generator",
        RC_UI::UITokens::MagentaHighlight,
        nullptr,
        ImGuiWindowFlags_NoCollapse);
    RC_UI::PopUnifiedWindowSchema();
    RC_UI::BeginUnifiedTextWrap();

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "City Spec Generator",
            "City Spec Generator",
            "toolbox",
            "Floating",
            "visualizer/src/ui/panels/rc_panel_city_spec.cpp",
            {"ai", "city_spec", "generator_bridge", "v0.0.9"}
        },
        open
    );

    if (!open) {
        RC_UI::EndUnifiedTextWrap();
        uiint.EndPanel();
        RC_UI::Components::EndTokenPanel();
        return;
    }
    
    auto& runtime = AI::AiBridgeRuntime::Instance();
    
    // Check if bridge is online
    if (!runtime.IsOnline()) {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(RC_UI::UITokens::YellowWarning));
        ImGui::Text("AI Bridge offline - start it in AI Console");
        ImGui::PopStyleColor();
        uiint.RegisterWidget({"text", "AI Bridge offline", "ai.bridge.status", {"ai", "status"}});
        uiint.EndPanel();
        RC_UI::EndUnifiedTextWrap();
        RC_UI::Components::EndTokenPanel();
        return;
    }
    
    DesignSystem::SectionHeader("City Description");
    RenderBusyIndicator(m_processing, m_busyTime);
    
    ImGui::Text("Describe your city:");
    ImGui::InputTextMultiline("##desc", m_descBuffer, sizeof(m_descBuffer), ImVec2(-1, 120));
    uiint.RegisterWidget({"text", "Description", "city_spec.description", {"input", "ai", "city_spec"}});
    
    const char* scales[] = { "Hamlet", "Town", "City", "Metro" };
    ImGui::Combo("Scale", &m_scaleIndex, scales, 4);
    uiint.RegisterWidget({"combo", "Scale", "city_spec.scale", {"input", "ai", "city_spec"}});
    
    ImGui::BeginDisabled(m_processing.load());
    if (DesignSystem::ButtonPrimary("Generate CitySpec", ImVec2(180, 30))) {
        if (!m_processing.exchange(true)) {
            m_busyTime = 0.0f;

            std::string desc = std::string(m_descBuffer);
            std::string scaleStr = scales[m_scaleIndex];
            std::transform(scaleStr.begin(), scaleStr.end(), scaleStr.begin(), ::tolower);

            std::thread([this, desc, scaleStr]() {
                Core::CitySpec spec = AI::CitySpecClient::GenerateSpec(desc, scaleStr);
                bool hasSpec = !spec.intent.description.empty();
                {
                    std::scoped_lock lock(m_specMutex);
                    m_currentSpec = std::move(spec);
                    m_hasSpec = hasSpec;
                }
                m_processing = false;
            }).detach();
        }
    }
    ImGui::EndDisabled();
    uiint.RegisterWidget({"button", "Generate CitySpec", "action:ai.city_spec.generate", {"action", "ai", "city_spec"}});
    uiint.RegisterAction({"ai.city_spec.generate", "Generate CitySpec", "City Spec Generator", {"ai", "city_spec"}, "AI::CitySpecClient::GenerateSpec"});
    
    Core::CitySpec specCopy;
    bool hasSpecCopy = false;
    {
        std::scoped_lock lock(m_specMutex);
        specCopy = m_currentSpec;
        hasSpecCopy = m_hasSpec;
    }

    if (hasSpecCopy) {
        DesignSystem::Separator();
        DesignSystem::SectionHeader("Generated Specification");
        
        ImGui::Text("Scale: %s", specCopy.intent.scale.c_str());
        ImGui::Text("Climate: %s", specCopy.intent.climate.c_str());
        ImGui::Text("Description: %s", specCopy.intent.description.c_str());
        
        if (!specCopy.intent.styleTags.empty()) {
            ImGui::Text("Style Tags:");
            for (const auto& tag : specCopy.intent.styleTags) {
                ImGui::BulletText("%s", tag.c_str());
            }
        }
        
        if (!specCopy.districts.empty()) {
            DesignSystem::Separator();
            ImGui::Text("Districts (%zu):", specCopy.districts.size());
            for (size_t i = 0; i < specCopy.districts.size(); ++i) {
                const auto& d = specCopy.districts[i];
                ImGui::BulletText("%s (density: %.2f)", d.type.c_str(), d.density);
            }
        }
        
        DesignSystem::Separator();
        ImGui::Text("Seed: %u", specCopy.seed);
        ImGui::Text("Road Density: %.2f", specCopy.roadDensity);
        
        if (DesignSystem::ButtonPrimary("Apply to Generator", ImVec2(180, 30))) {
            auto& gs = RogueCity::Core::Editor::GetGlobalState();
            gs.active_city_spec = specCopy;

            auto& zoning_state = RC_UI::Panels::ZoningControl::GetPanelState();
            std::string pipeline_error;
            const bool generated = zoning_state.bridge.GenerateFromCitySpec(
                specCopy,
                zoning_state.config,
                gs,
                &pipeline_error);

            if (!generated) {
                m_applyResult = pipeline_error.empty()
                    ? "Failed to apply CitySpec through generation pipeline."
                    : pipeline_error;
                m_applyError = true;
            } else {
                RogueCity::Generators::CitySpecGenerationRequest request;
                std::string adapter_error;
                if (RogueCity::Generators::CitySpecAdapter::TryBuildRequest(specCopy, request, &adapter_error)) {
                    std::string preview_error;
                    RC_UI::Panels::AxiomEditor::ApplyGeneratorRequest(
                        request.axioms,
                        request.config,
                        &preview_error);
                }

                m_applyResult = "Applied CitySpec to roads/districts/lots/buildings pipeline.";
                m_applyError = false;
            }
        }
        uiint.RegisterWidget({"button", "Apply to Generator", "action:generator.apply_city_spec", {"action", "generator", "city_spec"}});
        uiint.RegisterAction({"generator.apply_city_spec", "Apply to Generator", "City Spec Generator", {"generator", "ai"}, "RC_UI::Panels::AxiomEditor::ApplyGeneratorRequest"});

        if (!m_applyResult.empty()) {
            DesignSystem::StatusMessage(m_applyResult.c_str(), m_applyError);
            uiint.RegisterWidget({"text", "Apply Status", "city_spec.apply.status", {"status", m_applyError ? "error" : "success"}});
        }
    }

    uiint.EndPanel();
    RC_UI::EndUnifiedTextWrap();
    RC_UI::Components::EndTokenPanel();
}

} // namespace RogueCity::UI
