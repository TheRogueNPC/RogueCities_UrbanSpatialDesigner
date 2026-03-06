// FILE: rc_panel_imgui_error.cpp
// PURPOSE: ImGui error recovery agent panel implementation.
//
// Error recovery scenarios supported:
//   (1) Programmer seats (default): all flags ON  — asserts fire, tooltips visible, log active
//   (2) Programmer seats (nicer):   assert OFF    — tooltips only, must be manually toggled
//   (3) Non-programmer seats:       assert OFF    — ensure log is surfaced externally
//   (4) Scripting language host:    assert OFF    — use BeginProtectedSection per script call
//   (5) Exception handling:         BeginProtectedSection before try{}, EndProtectedSection in catch{}

#include "ui/panels/rc_panel_imgui_error.h"
#include "ui/rc_ui_tokens.h"
#include "ui/introspection/UiIntrospection.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <deque>
#include <string>

namespace RC_UI::Panels::ImGuiError {

namespace {

    constexpr int k_max_log_entries = 64;
    constexpr int k_msg_size        = 256;

    struct ErrorEntry {
        float       timestamp = 0.0f;  // seconds since Init()
        char        message[k_msg_size]{};
        bool        recovered = true;
    };

    static bool                   s_open            = false;
    static std::deque<ErrorEntry> s_error_log;
    static int                    s_log_consumed    = 0;   // bytes already scanned in DebugLogBuf
    static float                  s_elapsed         = 0.0f;
    static ImGuiErrorRecoveryState s_protection_state{};
    static bool                   s_section_active  = false;
    static char                   s_section_label[128]{};
    static bool                   s_scroll_to_bottom = false;

    // Scan the ImGui internal debug log for lines tagged [imgui-error].
    // ImGui writes these when ConfigErrorRecoveryEnableDebugLog is true.
    static void ScanDebugLog() {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        if (!ctx) return;

        const ImGuiTextBuffer& buf = ctx->DebugLogBuf;
        const int              total_size = buf.size();
        if (total_size <= s_log_consumed) return;

        // Walk new bytes line by line
        const char* begin = buf.begin() + s_log_consumed;
        const char* end   = buf.begin() + total_size;
        const char* line_start = begin;

        for (const char* p = begin; p < end; ++p) {
            if (*p == '\n' || p == end - 1) {
                const char* line_end = (*p == '\n') ? p : p + 1;
                // Check for the ImGui error recovery tag
                const char* tag = "[imgui-error]";
                bool is_error = false;
                for (const char* q = line_start; q + 13 <= line_end; ++q) {
                    if (std::strncmp(q, tag, 13) == 0) {
                        is_error = true;
                        break;
                    }
                }
                if (is_error) {
                    ErrorEntry entry;
                    entry.timestamp = s_elapsed;
                    entry.recovered = true;
                    int len = static_cast<int>(line_end - line_start);
                    if (len >= k_msg_size) len = k_msg_size - 1;
                    std::memcpy(entry.message, line_start, static_cast<size_t>(len));
                    entry.message[len] = '\0';

                    s_error_log.push_back(entry);
                    if (static_cast<int>(s_error_log.size()) > k_max_log_entries)
                        s_error_log.pop_front();
                    s_scroll_to_bottom = true;
                }
                line_start = p + 1;
            }
        }

        s_log_consumed = total_size;
    }

    static void DrawConfigSection() {
        ImGuiIO& io = ImGui::GetIO();

        ImGui::PushStyleColor(ImGuiCol_Text, UITokens::InfoBlue);
        ImGui::TextUnformatted("Error Recovery Configuration");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Checkbox("ConfigErrorRecovery (master)", &io.ConfigErrorRecovery)) {}
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Master switch. Disable only if you want hard crashes on API misuse.\n"
                              "Recommended: ON for all seats.");

        ImGui::BeginDisabled(!io.ConfigErrorRecovery);

        if (ImGui::Checkbox("EnableAssert  (Scenario 1 — programmer seats)", &io.ConfigErrorRecoveryEnableAssert)) {}
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Scenario 1: IM_ASSERT fires on every recoverable error.\n"
                              "Default for programmer seats. Disable to use Scenario 2 (tooltip-only).");

        if (ImGui::Checkbox("EnableDebugLog (log to DebugLogBuf)", &io.ConfigErrorRecoveryEnableDebugLog)) {}
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Appends [imgui-error] lines to the internal DebugLogBuf.\n"
                              "This panel scans that buffer to populate the Error Log below.");

        if (ImGui::Checkbox("EnableTooltip  (Scenario 2 — tooltip-only)", &io.ConfigErrorRecoveryEnableTooltip)) {}
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Scenario 2 / Non-programmer seats: show an error tooltip in the UI.\n"
                              "The tooltip also includes a button to re-enable asserts.");

        ImGui::EndDisabled();
        ImGui::Spacing();
    }

    static void DrawErrorLogSection() {
        ImGui::PushStyleColor(ImGuiCol_Text, s_error_log.empty() ? UITokens::GreenHUD : UITokens::ErrorRed);
        ImGui::Text("Error Log  (%d entries)", static_cast<int>(s_error_log.size()));
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) {
            s_error_log.clear();
        }
        ImGui::Separator();

        const float table_height = 160.0f;
        if (ImGui::BeginTable("##errlog", 3,
                ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingStretchProp,
                ImVec2(0.0f, table_height)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Time (s)",  ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Message",   ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Recovered", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableHeadersRow();

            for (const auto& e : s_error_log) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%.2f", e.timestamp);
                ImGui::TableSetColumnIndex(1);
                ImGui::PushStyleColor(ImGuiCol_Text, UITokens::ErrorRed);
                ImGui::TextUnformatted(e.message);
                ImGui::PopStyleColor();
                ImGui::TableSetColumnIndex(2);
                ImGui::PushStyleColor(ImGuiCol_Text, e.recovered ? UITokens::GreenHUD : UITokens::ErrorRed);
                ImGui::TextUnformatted(e.recovered ? "yes" : "no");
                ImGui::PopStyleColor();
            }

            if (s_scroll_to_bottom) {
                ImGui::SetScrollHereY(1.0f);
                s_scroll_to_bottom = false;
            }
            ImGui::EndTable();
        }
        ImGui::Spacing();
    }

    static void DrawProtectedSectionApiSection() {
        ImGui::PushStyleColor(ImGuiCol_Text, UITokens::CyanAccent);
        ImGui::TextUnformatted("BeginProtectedSection / EndProtectedSection API");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        const char* status_label = s_section_active
            ? (s_section_label[0] ? s_section_label : "(active)")
            : "(none)";
        ImGui::Text("Active section: %s", status_label);
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, UITokens::GreenHUD);
        ImGui::TextUnformatted("Scenario 4 — scripting language host:");
        ImGui::PopStyleColor();
        ImGui::TextWrapped(
            "  ImGuiError::BeginProtectedSection(\"script\");\n"
            "  // disable assert, run interpreter, re-enable\n"
            "  ImGuiError::EndProtectedSection();");
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, UITokens::GreenHUD);
        ImGui::TextUnformatted("Scenario 5 — exception boundary:");
        ImGui::PopStyleColor();
        ImGui::TextWrapped(
            "  ImGuiError::BeginProtectedSection(\"try-block\");\n"
            "  try { RunCode(); }\n"
            "  catch (...) { ImGuiError::EndProtectedSection(); throw; }\n"
            "  ImGuiError::EndProtectedSection();");
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Init() {
    ImGuiIO& io = ImGui::GetIO();
    // Programmer-seat defaults — Scenario 1 (all ON).
    io.ConfigErrorRecovery                = true;
    io.ConfigErrorRecoveryEnableAssert    = true;
    io.ConfigErrorRecoveryEnableDebugLog  = true;
    io.ConfigErrorRecoveryEnableTooltip   = true;

    s_error_log.clear();
    s_log_consumed   = 0;
    s_elapsed        = 0.0f;
    s_section_active = false;
    s_section_label[0] = '\0';
}

void Shutdown() {
    s_error_log.clear();
    s_log_consumed = 0;
}

bool IsOpen() { return s_open; }
void Toggle() { s_open = !s_open; }

void DrawContent(float dt) {
    s_elapsed += dt;
    ScanDebugLog();

    if (ImGui::CollapsingHeader("Config", ImGuiTreeNodeFlags_DefaultOpen))
        DrawConfigSection();

    if (ImGui::CollapsingHeader("Error Log", ImGuiTreeNodeFlags_DefaultOpen))
        DrawErrorLogSection();

    if (ImGui::CollapsingHeader("Protected Section API"))
        DrawProtectedSectionApiSection();
}

void Draw(float dt) {
    if (!s_open) return;

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel("ImGuiErrorAgent", "ImGui Error Agent");
    ImGui::SetNextWindowSize(ImVec2(540.0f, 440.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("ImGui Error Agent##ImGuiErrorAgent", &s_open))
        DrawContent(dt);
    ImGui::End();
    uiint.EndPanel();
}

void BeginProtectedSection(const char* label) {
    ImGui::ErrorRecoveryStoreState(&s_protection_state);
    s_section_active = true;
    if (label && label[0]) {
        std::strncpy(s_section_label, label, sizeof(s_section_label) - 1);
        s_section_label[sizeof(s_section_label) - 1] = '\0';
    } else {
        s_section_label[0] = '\0';
    }
}

void EndProtectedSection() {
    ImGui::ErrorRecoveryTryToRecoverState(&s_protection_state);
    s_section_active   = false;
    s_section_label[0] = '\0';
}

int GetErrorCount() {
    return static_cast<int>(s_error_log.size());
}

} // namespace RC_UI::Panels::ImGuiError
