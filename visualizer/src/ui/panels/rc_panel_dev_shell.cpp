// FILE: rc_panel_dev_shell.cpp
// PURPOSE: Development-only cockpit panel for exporting UI introspection snapshots.

#include "ui/panels/rc_panel_dev_shell.h"
#include "ui/api/rc_imgui_api.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/rc_ui_components.h"
#include "ui/tools/rc_tool_contract.h"
#include <RogueCity/Visualizer/LucideIcons.hpp>
#include "ui/introspection/UiIntrospection.h"
#include "client/UiDesignAssistant.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Visualizer/MockupTokenParser.hpp"

#include <imgui.h>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

namespace RC_UI::Panels::DevShell {

namespace {
    static bool s_open = false;
    static char s_outputDir[260] = "AI/docs/ui";
    static char s_lastOutput[512] = "";
    static char s_lastError[512] = "";
    static bool s_includeDockTree = true;
    static char s_workspacePresetName[96] = "default";
    static int s_workspacePresetIndex = 0;

    static char s_designGoal[512] = "Unify inspector-like panels and propose reusable patterns";
    static std::atomic<bool> s_designBusy{false};
    static std::mutex s_designMutex;
    static std::string s_designStatus;
    static bool s_show_metrics_window = false;
    static bool s_show_debug_log_window = false;
    static bool s_show_id_stack_tool_window = false;
    static bool s_debug_begin_return_value_loop = false;

    // Test Suite
    static std::string       s_test_output;
    static std::atomic<bool> s_test_busy{false};
    static std::mutex        s_test_mutex;

    // PVS-Studio Static Analysis
    static char              s_pvs_dir[260]       = "C:\\Program Files (x86)\\PVS-Studio";
    static char              s_pvs_output_dir[260] = "tools/.run/pvs_studio";
    static char              s_pvs_project[128]    = "RogueCityVisualizer";
    static bool              s_pvs_open_report     = false;
    static std::string       s_pvs_status;
    static std::atomic<bool> s_pvs_busy{false};
    static std::mutex        s_pvs_mutex;

    // Launch a dev-shell command asynchronously via PowerShell.
    // Writes stdout+stderr to *out_status under *out_mutex; clears *out_busy when done.
    static void RunDevShellCmdAsync(
        const std::string& cmd_suffix,
        std::string*       out_status,
        std::mutex*        out_mutex,
        std::atomic<bool>* out_busy)
    {
        const std::string full_cmd =
            "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "
            "\"& { . .\\tools\\dev-shell.ps1 > $null; " + cmd_suffix + " }\" 2>&1";
        std::thread([full_cmd, out_status, out_mutex, out_busy]() {
            std::string result;
#ifdef _WIN32
            FILE* pipe = _popen(full_cmd.c_str(), "r");
#else
            FILE* pipe = popen(full_cmd.c_str(), "r");
#endif
            if (!pipe) {
                result = "[error] Failed to launch subprocess";
            } else {
                char buf[256];
                while (fgets(buf, sizeof(buf), pipe))
                    result += buf;
#ifdef _WIN32
                _pclose(pipe);
#else
                pclose(pipe);
#endif
            }
            {
                std::scoped_lock lock(*out_mutex);
                *out_status = result.empty() ? "(no output)" : result;
            }
            *out_busy = false;
        }).detach();
    }

    static std::string TimestampForFilename() {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto t = system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        tm = *std::localtime(&t);
#endif
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_" << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }

    /// Write a sidecar HTML comment file containing the current UI introspection
    /// snapshot JSON. Intended for round-tripping state back into the mockup.
    static bool ExportMockupStateComment(std::string* outPath, std::string* outError) {
        const std::string path = "visualizer/RC_UI_Mockup_state.html";

        auto& introspector = RogueCity::UIInt::UiIntrospector::Instance();
        const auto snapshot = introspector.SnapshotJson();

        try {
            std::filesystem::create_directories("visualizer");
            std::ofstream f(path, std::ios::binary);
            if (!f.is_open()) {
                if (outError) *outError = "Failed to open: " + path;
                return false;
            }

            using namespace std::chrono;
            auto now = system_clock::now();
            auto t = system_clock::to_time_t(now);
            std::tm tm{};
#ifdef _WIN32
            localtime_s(&tm, &t);
#else
            tm = *std::localtime(&t);
#endif
            std::ostringstream ts;
            ts << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");

            f << "<!-- RC-USD UI State Snapshot\n";
            f << "     Generated: " << ts.str() << "\n";
            f << "     Source: UiIntrospector live frame\n";
            f << "\n";
            f << snapshot.dump(2);
            f << "\n-->\n";

            if (outPath) *outPath = path;
            return true;
        } catch (const std::exception& e) {
            if (outError) *outError = e.what();
            return false;
        }
    }
}

bool IsOpen() {
    return s_open;
}

void Toggle() {
    s_open = !s_open;
}

void DrawContent(float /*dt*/)
{
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    
    // ======================================================================
    // DEV MODE FEATURE TOGGLE (unlocks AI panels and experimental features)
    // ======================================================================
    if (API::SectionHeader("Developer Mode", UITokens::InfoBlue, true)) {
        
        bool dev_mode_changed = API::Checkbox("Enable Dev Mode", &gs.config.dev_mode_enabled);
        uiint.RegisterWidget({"checkbox", "Enable Dev Mode", "dev.mode_enabled", {"dev", "features"}});
        
        if (dev_mode_changed) {
            if (gs.config.dev_mode_enabled) {
                std::snprintf(s_lastOutput, sizeof(s_lastOutput), "Dev Mode ENABLED - AI features unlocked");
            } else {
                std::snprintf(s_lastOutput, sizeof(s_lastOutput), "Dev Mode DISABLED - AI features locked");
            }
            s_lastError[0] = '\0';
        }
        
        API::SameLine();
        if (gs.config.dev_mode_enabled) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen));
            ImGui::Text(" [ACTIVE]");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary));
            ImGui::Text(" [INACTIVE]");
            ImGui::PopStyleColor();
        }
        
        API::Spacing();
        ImGui::TextWrapped("Dev Mode unlocks feature-gated panels (AI Console, UI Agent, City Spec) and experimental tools. Requires AI bridge dependencies at runtime.");
        API::Spacing();
    }
    
    API::Separator();
    
    // ======================================================================
    // UI INTROSPECTION EXPORT
    // ======================================================================
    ImGui::TextUnformatted("UI Introspection Export");
    API::Checkbox("Include dock_tree", &s_includeDockTree);
    API::InputText("Output dir", s_outputDir, sizeof(s_outputDir));
    uiint.RegisterWidget({"checkbox", "Include dock_tree", "uiint.include_dock_tree", {"dev"}});
    uiint.RegisterWidget({"text", "Output dir", "uiint.output_dir", {"dev"}});

    if (auto ico = RC::SvgTextureCache::Get().Load(LC::Terminal, 14.f)) {
        ImGui::Image(ico, ImVec2(14, 14)); API::SameLine(0, 4);
    }
    if (API::Button("Export UI Snapshot JSON")) {
        auto& introspector = RogueCity::UIInt::UiIntrospector::Instance();
        std::string file = std::string(s_outputDir) + "/ui_introspection_" + TimestampForFilename() + ".json";

        if (!s_includeDockTree) {
            // If dock_tree is disabled, overwrite with empty node for this export.
            auto j = introspector.SnapshotJson();
            j.erase("dock_tree");
            try {
                // best-effort write
                std::filesystem::create_directories(std::string(s_outputDir));
                std::ofstream f(file, std::ios::binary);
                if (!f.is_open()) {
                    std::snprintf(s_lastError, sizeof(s_lastError), "Failed to open %s", file.c_str());
                } else {
                    f << j.dump(2);
                    std::snprintf(s_lastOutput, sizeof(s_lastOutput), "%s", file.c_str());
                    s_lastError[0] = '\0';
                }
            } catch (const std::exception& e) {
                std::snprintf(s_lastError, sizeof(s_lastError), "%s", e.what());
            }
        } else {
            std::string err;
            if (introspector.SaveSnapshotJson(file, &err)) {
                std::snprintf(s_lastOutput, sizeof(s_lastOutput), "%s", file.c_str());
                s_lastError[0] = '\0';
            } else {
                std::snprintf(s_lastError, sizeof(s_lastError), "%s", err.c_str());
            }
        }
    }
    uiint.RegisterWidget({"button", "Export UI Snapshot JSON", "action:uiint.export", {"action", "dev"}});

    API::SameLine();
    if (auto ico = RC::SvgTextureCache::Get().Load(LC::Terminal, 14.f)) {
        ImGui::Image(ico, ImVec2(14, 14)); API::SameLine(0, 4);
    }
    if (API::Button("Export Snapshot \xe2\x86\x92 Mockup Comment")) {
        std::string path, err;
        if (ExportMockupStateComment(&path, &err)) {
            std::snprintf(s_lastOutput, sizeof(s_lastOutput), "%s", path.c_str());
            s_lastError[0] = '\0';
        } else {
            std::snprintf(s_lastError, sizeof(s_lastError), "%s", err.c_str());
        }
    }
    uiint.RegisterWidget({"button", "Export Snapshot \xe2\x86\x92 Mockup Comment",
        "action:uiint.export_mockup_comment", {"action", "dev", "mockup"}});

    if (s_lastOutput[0] != '\0') {
        ImGui::Text("Last export: %s", s_lastOutput);
    }
    if (s_lastError[0] != '\0') {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed), "Error: %s", s_lastError);
    }

    API::Separator();
    ImGui::TextUnformatted("Snapshot Preview (current frame)");
    const auto preview = RogueCity::UIInt::UiIntrospector::Instance().SnapshotJson().dump(2);
    ImGui::BeginChild("##uiint_preview", ImVec2(0.0f, 220.0f), true);
    ImGui::TextUnformatted(preview.c_str());
    ImGui::EndChild();
    uiint.RegisterWidget({"tree", "Snapshot Preview", "uiint.preview", {"dev"}});

    API::Separator();
    ImGui::TextUnformatted("Workspace Presets");
    API::InputText("Preset Name", s_workspacePresetName, sizeof(s_workspacePresetName));
    uiint.RegisterWidget({"text", "Preset Name", "workspace.preset_name", {"dev", "layout"}});

    if (auto ico = RC::SvgTextureCache::Get().Load(LC::Plus, 14.f)) {
        ImGui::Image(ico, ImVec2(14, 14)); API::SameLine(0, 4);
    }
    if (API::Button("Save Workspace Preset")) {
        std::string err;
        if (RC_UI::SaveWorkspacePreset(s_workspacePresetName, &err)) {
            std::snprintf(s_lastOutput, sizeof(s_lastOutput), "Saved workspace preset: %s", s_workspacePresetName);
            s_lastError[0] = '\0';
        } else {
            std::snprintf(s_lastError, sizeof(s_lastError), "%s", err.c_str());
        }
    }
    API::SameLine();
    if (auto ico = RC::SvgTextureCache::Get().Load(LC::RefreshCw, 14.f)) {
        ImGui::Image(ico, ImVec2(14, 14)); API::SameLine(0, 4);
    }
    if (API::Button("Reset Dock Layout")) {
        RC_UI::ResetDockLayout();
        std::snprintf(s_lastOutput, sizeof(s_lastOutput), "Dock layout reset to defaults");
        s_lastError[0] = '\0';
    }
    uiint.RegisterWidget({"button", "Save Workspace Preset", "action:workspace.save_preset", {"action", "layout"}});
    uiint.RegisterWidget({"button", "Reset Dock Layout", "action:workspace.reset_layout", {"action", "layout"}});

    const std::vector<std::string> presets = RC_UI::ListWorkspacePresets();
    if (presets.empty()) {
        API::TextDisabled("No saved presets yet.");
    } else {
        std::vector<const char*> preset_names;
        preset_names.reserve(presets.size());
        for (const std::string& preset : presets) {
            preset_names.push_back(preset.c_str());
        }
        s_workspacePresetIndex = std::clamp(s_workspacePresetIndex, 0, static_cast<int>(preset_names.size()) - 1);
        API::Combo("Saved Presets", &s_workspacePresetIndex, preset_names.data(), static_cast<int>(preset_names.size()));
        if (API::Button("Load Selected Preset")) {
            std::string err;
            const std::string& selected = presets[static_cast<size_t>(s_workspacePresetIndex)];
            if (RC_UI::LoadWorkspacePreset(selected.c_str(), &err)) {
                std::snprintf(s_lastOutput, sizeof(s_lastOutput), "Loaded workspace preset: %s", selected.c_str());
                s_lastError[0] = '\0';
            } else {
                std::snprintf(s_lastError, sizeof(s_lastError), "%s", err.c_str());
            }
        }
        uiint.RegisterWidget({"combo", "Saved Presets", "workspace.saved_presets", {"layout"}});
        uiint.RegisterWidget({"button", "Load Selected Preset", "action:workspace.load_preset", {"action", "layout"}});
    }

    API::Separator();
    ImGui::TextUnformatted("Input Debug");
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Mouse: (%.0f, %.0f)  L:%d R:%d M:%d",
        io.MousePos.x, io.MousePos.y,
        io.MouseDown[0] ? 1 : 0,
        io.MouseDown[1] ? 1 : 0,
        io.MouseDown[2] ? 1 : 0);
    ImGui::Text("Mods: Ctrl:%d Shift:%d Alt:%d  Capture: Mouse:%d Key:%d",
        io.KeyCtrl ? 1 : 0, io.KeyShift ? 1 : 0, io.KeyAlt ? 1 : 0,
        io.WantCaptureMouse ? 1 : 0, io.WantCaptureKeyboard ? 1 : 0);
    ImGui::Text("WantTextInput: %d", io.WantTextInput ? 1 : 0);
    const auto& input_gate = RC_UI::GetUiInputGateState();
    ImGui::Text("Input Gate: mouse=%d key=%d",
        input_gate.allow_viewport_mouse_actions ? 1 : 0,
        input_gate.allow_viewport_key_actions ? 1 : 0);

    API::Separator();
    ImGui::TextUnformatted("Tool Runtime");
    ImGui::Text("Domain: %s", RC_UI::Tools::ToolDomainName(gs.tool_runtime.active_domain));
    ImGui::Text("Last Action ID: %s",
        gs.tool_runtime.last_action_id.empty() ? "<none>" : gs.tool_runtime.last_action_id.c_str());
    ImGui::Text("Last Action Label: %s",
        gs.tool_runtime.last_action_label.empty() ? "<none>" : gs.tool_runtime.last_action_label.c_str());
    ImGui::Text("Last Action Status: %s",
        gs.tool_runtime.last_action_status.empty() ? "<none>" : gs.tool_runtime.last_action_status.c_str());
    ImGui::Text("Viewport Status: %s",
        gs.tool_runtime.last_viewport_status.empty() ? "<none>" : gs.tool_runtime.last_viewport_status.c_str());
    const auto policy_for_active_domain = gs.generation_policy.ForDomain(gs.tool_runtime.active_domain);
    ImGui::Text("Generation Policy: %s",
        policy_for_active_domain == RogueCity::Core::Editor::GenerationMutationPolicy::LiveDebounced
            ? "LiveDebounced"
            : "ExplicitOnly");
    ImGui::Text("Explicit Generate Pending: %s", gs.tool_runtime.explicit_generation_pending ? "yes" : "no");
    ImGui::Text("Dispatch Serial: %llu  Frame: %llu",
        static_cast<unsigned long long>(gs.tool_runtime.action_serial),
        static_cast<unsigned long long>(gs.tool_runtime.last_action_frame));

    API::Separator();
    ImGui::TextUnformatted("ImGui Diagnostics");
    API::Checkbox("Show Metrics Window", &s_show_metrics_window);
    API::Checkbox("Show Debug Log Window", &s_show_debug_log_window);
    API::Checkbox("Show ID Stack Tool", &s_show_id_stack_tool_window);
    if (API::Checkbox("Debug Begin/BeginChild Return Loop", &s_debug_begin_return_value_loop)) {
        io.ConfigDebugBeginReturnValueLoop = s_debug_begin_return_value_loop;
    }
    uiint.RegisterWidget({"checkbox", "Show Metrics Window", "dev.imgui.metrics", {"dev", "imgui"}});
    uiint.RegisterWidget({"checkbox", "Show Debug Log Window", "dev.imgui.debug_log", {"dev", "imgui"}});
    uiint.RegisterWidget({"checkbox", "Show ID Stack Tool", "dev.imgui.id_stack", {"dev", "imgui"}});
    uiint.RegisterWidget({"checkbox", "Debug Begin/BeginChild Return Loop", "dev.imgui.begin_return_loop", {"dev", "imgui"}});

    API::Separator();

    // ======================================================================
    // IMGUI DEBUG PRODUCTIVITY TIPS
    // ======================================================================
    if (API::SectionHeader("ImGui Debug Productivity", UITokens::AmberGlow, false)) {

        // Alt-key breakpoint filter tip
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::AmberGlow));
        ImGui::TextUnformatted("Alt-Key Breakpoint Filter");
        ImGui::PopStyleColor();
        ImGui::TextWrapped(
            "Gate debugger breakpoints on io.KeyAlt to trigger them on demand:\n"
            "  if (ImGui::GetIO().KeyAlt) printf(\"\"); // set breakpoint here\n"
            "Set up your UI state, then press ALT to trigger.");
        API::Spacing();
        if (io.KeyAlt) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen));
            ImGui::TextUnformatted("  ALT: HELD  [breakpoint condition is TRUE]");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary));
            ImGui::TextUnformatted("  ALT: released");
            ImGui::PopStyleColor();
        }

        API::Spacing();
        API::Separator();

        // Natvis file
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::InfoBlue));
        ImGui::TextUnformatted("Natvis Debugger Support");
        ImGui::PopStyleColor();
        const char* k_natvis = "3rdparty/imgui/misc/debuggers/imgui.natvis";
        const bool natvis_ok = std::filesystem::exists(k_natvis);
        if (natvis_ok) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen));
            ImGui::TextUnformatted("imgui.natvis: found");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed));
            ImGui::TextUnformatted("imgui.natvis: NOT FOUND");
            ImGui::PopStyleColor();
        }
        API::SameLine();
        if (API::SmallButton("Copy Path##natvis")) {
            ImGui::SetClipboardText(k_natvis);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "Add misc/debuggers/imgui.natvis to your VS project to get\n"
                "Dear ImGui type expansion in Watch/Locals (e.g. ImVector<>).");
        API::Spacing();
    }
    uiint.RegisterWidget({"group", "ImGui Debug Productivity", "dev.imgui.tips", {"dev", "imgui", "debug"}});

    API::Separator();

    // ======================================================================
    // TEST SUITE (CTest)
    // ======================================================================
    if (API::SectionHeader("Test Suite (CTest)", UITokens::CyanAccent, false)) {
        ImGui::TextWrapped(
            "Run the regression suite. Results are captured below.\n"
            "Adding test cases when fixing bugs catches regressions early.");
        API::Spacing();

        API::BeginDisabled(s_test_busy.load());
        auto run_test = [](const char* cmd_suffix) {
            if (!s_test_busy.exchange(true)) {
                { std::scoped_lock lk(s_test_mutex); s_test_output = "Running..."; }
                RunDevShellCmdAsync(cmd_suffix, &s_test_output, &s_test_mutex, &s_test_busy);
            }
        };
        if (API::Button("Core##tst"))  run_test("rc-tst-core");
        API::SameLine();
        if (API::Button("Gen##tst"))   run_test("rc-tst-gen");
        API::SameLine();
        if (API::Button("App##tst"))   run_test("rc-tst-app");
        API::SameLine();
        if (API::Button("All##tst"))   run_test("rc-tst");
        API::EndDisabled();

        if (s_test_busy.load()) {
            API::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::AmberGlow));
            ImGui::TextUnformatted("  running...");
            ImGui::PopStyleColor();
        }
        {
            std::scoped_lock lk(s_test_mutex);
            if (!s_test_output.empty()) {
                ImGui::BeginChild("##test_out", ImVec2(0.0f, 120.0f), true);
                ImGui::TextUnformatted(s_test_output.c_str());
                ImGui::EndChild();
            }
        }
    }
    uiint.RegisterWidget({"group", "Test Suite", "dev.test_suite", {"dev", "test", "ctest"}});

    API::Separator();

    // ======================================================================
    // PVS-STUDIO STATIC ANALYSIS
    // ======================================================================
    if (API::SectionHeader("PVS-Studio Static Analysis", UITokens::GreenHUD, false)) {
        ImGui::TextWrapped(
            "Run PVS-Studio on the MSVC project. Reports are written to the output dir.\n"
            "Invokes rc-pvs-studio from dev-shell.ps1 (GA:1,2 + OP:1 severity gates).");
        API::Spacing();

        API::InputText("PVS-Studio Dir##pvs",  s_pvs_dir,        sizeof(s_pvs_dir));
        API::InputText("Output Dir##pvs",       s_pvs_output_dir, sizeof(s_pvs_output_dir));
        API::InputText("Project Name##pvs",     s_pvs_project,    sizeof(s_pvs_project));
        API::Checkbox("Open HTML report after analysis##pvs", &s_pvs_open_report);
        API::Spacing();

        const bool pvs_installed =
            std::filesystem::exists(std::string(s_pvs_dir) + "\\PVS-Studio_Cmd.exe");
        if (!pvs_installed) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::AmberGlow));
            ImGui::TextWrapped(
                "PVS-Studio_Cmd.exe not found at the configured dir.\n"
                "Install from pvs-studio.com (free for OSS) or adjust path above.");
            ImGui::PopStyleColor();
        }

        API::BeginDisabled(s_pvs_busy.load() || !pvs_installed);
        if (API::Button("Run PVS-Studio Analysis##pvs")) {
            if (!s_pvs_busy.exchange(true)) {
                const std::string open_flag = s_pvs_open_report ? " -OpenReport" : "";
                const std::string cmd =
                    std::string("rc-pvs-studio -PvsDir '") + s_pvs_dir +
                    "' -OutputDir '" + s_pvs_output_dir +
                    "' -ProjectName '" + s_pvs_project + "'" + open_flag;
                {
                    std::scoped_lock lk(s_pvs_mutex);
                    s_pvs_status = "Launching analysis... (may take several minutes)";
                }
                RunDevShellCmdAsync(cmd, &s_pvs_status, &s_pvs_mutex, &s_pvs_busy);
            }
        }
        API::EndDisabled();

        if (s_pvs_busy.load()) {
            API::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::AmberGlow));
            ImGui::TextUnformatted("  analyzing...");
            ImGui::PopStyleColor();
        }
        {
            std::scoped_lock lk(s_pvs_mutex);
            if (!s_pvs_status.empty()) {
                ImGui::BeginChild("##pvs_out", ImVec2(0.0f, 100.0f), true);
                ImGui::TextWrapped("%s", s_pvs_status.c_str());
                ImGui::EndChild();
            }
        }
    }
    uiint.RegisterWidget({"group", "PVS-Studio", "dev.pvs_studio", {"dev", "analysis", "static"}});

    API::Separator();
    ImGui::TextUnformatted("UI Design Assistant");

    API::InputTextMultiline("Goal", s_designGoal, sizeof(s_designGoal), ImVec2(-1, 60));
    uiint.RegisterWidget({"text", "Goal", "ui_design.goal", {"dev", "input"}});

    API::BeginDisabled(s_designBusy.load());
    if (auto ico = RC::SvgTextureCache::Get().Load(LC::Bot, 14.f)) {
        ImGui::Image(ico, ImVec2(14, 14)); API::SameLine(0, 4);
    }
    if (API::Button("Generate Design Plan (from introspection)")) {
        if (!s_designBusy.exchange(true)) {
            {
                std::scoped_lock lock(s_designMutex);
                s_designStatus = "Processing...";
            }

            auto snapshotJson = RogueCity::UIInt::UiIntrospector::Instance().SnapshotJson();
            std::string goal = std::string(s_designGoal);
            std::string outDir = std::string(s_outputDir);

            std::thread([snapshotJson, goal, outDir]() {
                auto plan = RogueCity::AI::UiDesignAssistant::GenerateDesignPlan(snapshotJson, goal);
                std::string filename = outDir + "/ui_design_" + TimestampForFilename() + ".json";
                bool saved = RogueCity::AI::UiDesignAssistant::SaveDesignPlan(plan, filename);

                {
                    std::scoped_lock lock(s_designMutex);
                    if (saved) {
                        s_designStatus = "Saved plan: " + filename + "\n\n" + plan.summary;
                    } else {
                        s_designStatus = "Failed to save plan (check console output)";
                    }
                }

                s_designBusy = false;
            }).detach();
        }
    }
    API::EndDisabled();
    uiint.RegisterWidget({"button", "Generate Design Plan (from introspection)", "action:ai.ui_design.generate_plan", {"action", "ai", "dev"}});

    {
        std::scoped_lock lock(s_designMutex);
        if (!s_designStatus.empty()) {
            ImGui::TextWrapped("%s", s_designStatus.c_str());
        }
    }

    if (s_show_metrics_window) {
        ImGui::ShowMetricsWindow(&s_show_metrics_window);
    }
    if (s_show_debug_log_window) {
        ImGui::ShowDebugLogWindow(&s_show_debug_log_window);
    }
    if (s_show_id_stack_tool_window) {
        ImGui::ShowIDStackToolWindow(&s_show_id_stack_tool_window);
    }

}

void Draw(float dt) {
    if (!s_open) return;

    static RC_UI::DockableWindowState s_dev_shell_window;
    if (!RC_UI::BeginDockableWindow("Dev Shell", s_dev_shell_window, "Right", ImGuiWindowFlags_NoCollapse)) {
        return;
    }

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Dev Shell",
            "Dev Shell",
            "settings",
            "Floating",
            "visualizer/src/ui/panels/rc_panel_dev_shell.cpp",
            {"dev", "introspection"}
        },
        true
    );
    
    DrawContent(dt);
    
    uiint.EndPanel();
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::DevShell
