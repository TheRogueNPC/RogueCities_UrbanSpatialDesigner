// FILE: rc_panel_city_spec.cpp
// PURPOSE: CitySpec Generator for AI-driven city design

#include "rc_panel_city_spec.h"
#include "client/CitySpecClient.h"
#include "runtime/AiBridgeRuntime.h"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "RogueCity/App/UI/DesignSystem.h"
#include "RogueCity/Generators/Pipeline/CitySpecAdapter.hpp"
#include "ui/introspection/UiIntrospection.h"
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

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, alpha));
    ImGui::Text("AI processing...");
    ImGui::PopStyleColor();
    ImGui::Separator();
}

void CitySpecPanel::Render() {
    const bool open = ImGui::Begin("City Spec Generator", nullptr, ImGuiWindowFlags_NoCollapse);

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
        uiint.EndPanel();
        ImGui::End();
        return;
    }
    
    auto& runtime = AI::AiBridgeRuntime::Instance();
    
    // Check if bridge is online
    if (!runtime.IsOnline()) {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(DesignTokens::YellowWarning));
        ImGui::Text("AI Bridge offline - start it in AI Console");
        ImGui::PopStyleColor();
        uiint.RegisterWidget({"text", "AI Bridge offline", "ai.bridge.status", {"ai", "status"}});
        uiint.EndPanel();
        ImGui::End();
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
            RogueCity::Generators::CitySpecGenerationRequest request;
            std::string adapterError;
            if (!RogueCity::Generators::CitySpecAdapter::TryBuildRequest(specCopy, request, &adapterError)) {
                m_applyResult = adapterError.empty()
                    ? "Failed to convert CitySpec to generator request."
                    : adapterError;
                m_applyError = true;
            } else {
                std::string applyError;
                const bool applied = RC_UI::Panels::AxiomEditor::ApplyGeneratorRequest(
                    request.axioms,
                    request.config,
                    &applyError);

                if (applied) {
                    m_applyResult = "Applied CitySpec to generator preview.";
                    m_applyError = false;
                } else {
                    m_applyResult = applyError.empty()
                        ? "Failed to apply generator request."
                        : applyError;
                    m_applyError = true;
                }
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
    ImGui::End();
}

} // namespace RogueCity::UI
