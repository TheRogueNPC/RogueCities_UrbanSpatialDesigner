// FILE: visualizer/src/ui/panels/rc_panel_city_health.cpp
// PURPOSE: City Health tab — on-demand CityInvariantsChecker display with
//          coloured category rows, pulsing run button, scrollable violation list.

#include "ui/panels/rc_panel_city_health.h"

#include "ui/rc_ui_tokens.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>
#include <RogueCity/Core/Validation/CityInvariants.hpp>

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

namespace RC_UI::Panels::CityHealth {

namespace {

// ---- cached state ----------------------------------------------------------

struct CachedResult {
    Core::Validation::InvariantCheckResult result{};
    bool has_run  = false;
    float age_sec = 0.0f; // seconds since last run
};

static CachedResult s_cache{};
static bool         s_dirty = true;

// ---- helpers ---------------------------------------------------------------

[[nodiscard]] size_t CountByCategory(
    const Core::Validation::InvariantCheckResult& r,
    const char* category,
    Core::Validation::ViolationSeverity sev)
{
    size_t n = 0;
    for (const auto& v : r.violations) {
        if (v.category == category && v.severity == sev) {
            ++n;
        }
    }
    return n;
}

void DrawCategoryRow(
    const char* label,
    const Core::Validation::InvariantCheckResult& result,
    const char* category)
{
    using namespace Core::Validation;
    const size_t errors   = CountByCategory(result, category, ViolationSeverity::Error);
    const size_t warnings = CountByCategory(result, category, ViolationSeverity::Warning);

    // Dot indicator
    ImU32 dot_color;
    if (errors > 0) {
        dot_color = UITokens::ErrorRed;
    } else if (warnings > 0) {
        dot_color = UITokens::AmberGlow;
    } else {
        dot_color = UITokens::SuccessGreen;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p  = ImGui::GetCursorScreenPos();
    constexpr float kR = 5.0f;
    dl->AddCircleFilled(ImVec2(p.x + kR + 2.0f, p.y + ImGui::GetTextLineHeight() * 0.5f),
                        kR, dot_color);
    ImGui::Dummy(ImVec2(kR * 2.0f + 6.0f, ImGui::GetTextLineHeight()));
    ImGui::SameLine();

    // Label
    ImGui::TextUnformatted(label);
    ImGui::SameLine();

    // Count chips — errors first, then warnings
    if (errors > 0) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed),
                           "%zu error%s", errors, errors == 1 ? "" : "s");
        if (warnings > 0) { ImGui::SameLine(); }
    }
    if (warnings > 0) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::AmberGlow),
                           "%zu warning%s", warnings, warnings == 1 ? "" : "s");
    }
    if (errors == 0 && warnings == 0) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen), "OK");
    }
}

void DrawViolationList(const Core::Validation::InvariantCheckResult& result) {
    using namespace Core::Validation;

    constexpr float kListHeight = 180.0f;
    ImGui::BeginChild("##health_violations", ImVec2(0.0f, kListHeight), true);

    if (result.violations.empty()) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen),
                           "No violations found.");
    } else {
        for (const auto& v : result.violations) {
            const bool is_error = (v.severity == ViolationSeverity::Error);
            const ImU32 col = is_error ? UITokens::ErrorRed : UITokens::AmberGlow;

            // Icon glyph
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col),
                               is_error ? "[!]" : "[~]");
            ImGui::SameLine();

            // rule_id (dimmed) + description
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary),
                               "%s", v.rule_id.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextPrimary),
                               "%s", v.description.c_str());

            // ID badge — click copies to clipboard
            ImGui::SameLine();
            const std::string badge = "id=" + std::to_string(v.entity_id);
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImGui::ColorConvertU32ToFloat4(UITokens::InfoBlue));
            if (ImGui::SmallButton(badge.c_str())) {
                ImGui::SetClipboardText(std::to_string(v.entity_id).c_str());
            }
            ImGui::PopStyleColor();
        }
    }

    ImGui::EndChild();
}

} // namespace

// ============================================================================

void MarkDirty() {
    s_dirty = true;
}

void DrawContent(float dt) {
    using namespace Core::Validation;
    namespace Core = RogueCity::Core;

    s_cache.age_sec += dt;

    // ---- Header row: title + age label ------------------------------------
    {
        const ImVec4 title_col = ImGui::ColorConvertU32ToFloat4(UITokens::CyanAccent);
        ImGui::TextColored(title_col, "CITY HEALTH");
        ImGui::SameLine();

        if (s_cache.has_run) {
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextDisabled),
                               " (%.1fs ago)", s_cache.age_sec);
        }
    }

    ImGui::Separator();

    // ---- Run Check button -------------------------------------------------
    {
        // Pulse the border cyan when dirty
        const float pulse = 0.5f + 0.5f * std::sin(
            static_cast<float>(ImGui::GetTime()) * UITokens::AnimPulse * 3.14159f * 2.0f);
        const ImU32 btn_col = s_dirty
            ? WithAlpha(UITokens::CyanAccent, static_cast<uint8_t>(120 + 135 * pulse))
            : WithAlpha(UITokens::CyanAccent, 120);

        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImGui::ColorConvertU32ToFloat4(btn_col));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImGui::ColorConvertU32ToFloat4(
                                  WithAlpha(UITokens::CyanAccent, 180)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImGui::ColorConvertU32ToFloat4(UITokens::CyanAccent));

        if (ImGui::Button("  Run Check  ")) {
            auto& gs = RogueCity::Core::Editor::GetGlobalState();

            // Convert fva::Container → std::vector for the checker
            std::vector<Core::Road>     roads(gs.roads.begin(),     gs.roads.end());
            std::vector<Core::District> districts(gs.districts.begin(), gs.districts.end());
            std::vector<Core::LotToken> lots(gs.lots.begin(),       gs.lots.end());

            CityInvariantsChecker checker{};
            s_cache.result  = checker.Check(roads, districts, lots, gs.buildings);
            s_cache.has_run = true;
            s_cache.age_sec = 0.0f;
            s_dirty         = false;
        }

        ImGui::PopStyleColor(3);
    }

    if (!s_cache.has_run) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextDisabled),
                           "Press Run Check to evaluate the current city.");
        return;
    }

    ImGui::Spacing();

    // ---- Overall status badge --------------------------------------------
    {
        const size_t total_errors   = s_cache.result.ErrorCount();
        const size_t total_warnings = s_cache.result.WarningCount();

        ImU32 badge_col;
        const char* badge_label;
        if (total_errors > 0) {
            badge_col   = UITokens::ErrorRed;
            badge_label = "FAIL";
        } else if (total_warnings > 0) {
            badge_col   = UITokens::AmberGlow;
            badge_label = "WARN";
        } else {
            badge_col   = UITokens::SuccessGreen;
            badge_label = "PASS";
        }

        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(badge_col), "[%s]", badge_label);
        ImGui::SameLine();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary),
                           "%zu error%s, %zu warning%s",
                           total_errors,   total_errors   == 1 ? "" : "s",
                           total_warnings, total_warnings == 1 ? "" : "s");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ---- Per-category rows -----------------------------------------------
    DrawCategoryRow("Roads     ", s_cache.result, "Roads");
    DrawCategoryRow("Districts ", s_cache.result, "Districts");
    DrawCategoryRow("Lots      ", s_cache.result, "Lots");
    DrawCategoryRow("Buildings ", s_cache.result, "Buildings");

    ImGui::Spacing();
    ImGui::Separator();

    // ---- Violation list header -------------------------------------------
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary),
                       "VIOLATIONS");
    ImGui::Spacing();

    DrawViolationList(s_cache.result);
}

} // namespace RC_UI::Panels::CityHealth
