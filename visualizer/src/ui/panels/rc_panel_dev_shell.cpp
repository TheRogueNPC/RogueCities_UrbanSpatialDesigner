// FILE: rc_panel_dev_shell.cpp
// PURPOSE: Development-only cockpit panel for exporting UI introspection snapshots.

#include "ui/panels/rc_panel_dev_shell.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/introspection/UiIntrospection.h"
#include "client/UiDesignAssistant.h"

#include <imgui.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

namespace RC_UI::Panels::DevShell {

namespace {
    static bool s_open = false;
    static char s_outputDir[260] = "AI/docs/ui";
    static char s_lastOutput[512] = "";
    static char s_lastError[512] = "";
    static bool s_includeDockTree = true;

    static char s_designGoal[512] = "Unify inspector-like panels and propose reusable patterns";
    static std::atomic<bool> s_designBusy{false};
    static std::mutex s_designMutex;
    static std::string s_designStatus;

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
}

bool IsOpen() {
    return s_open;
}

void Toggle() {
    s_open = !s_open;
}

void Draw(float /*dt*/) {
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

    ImGui::TextUnformatted("Development Features (disabled by default)");
    ImGui::Separator();

    ImGui::TextUnformatted("UI Introspection Export");
    ImGui::Checkbox("Include dock_tree", &s_includeDockTree);
    ImGui::InputText("Output dir", s_outputDir, sizeof(s_outputDir));
    uiint.RegisterWidget({"checkbox", "Include dock_tree", "uiint.include_dock_tree", {"dev"}});
    uiint.RegisterWidget({"text", "Output dir", "uiint.output_dir", {"dev"}});

    if (ImGui::Button("Export UI Snapshot JSON")) {
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

    if (s_lastOutput[0] != '\0') {
        ImGui::Text("Last export: %s", s_lastOutput);
    }
    if (s_lastError[0] != '\0') {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed), "Error: %s", s_lastError);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Snapshot Preview (current frame)");
    const auto preview = RogueCity::UIInt::UiIntrospector::Instance().SnapshotJson().dump(2);
    ImGui::BeginChild("##uiint_preview", ImVec2(0.0f, 220.0f), true);
    ImGui::TextUnformatted(preview.c_str());
    ImGui::EndChild();
    uiint.RegisterWidget({"tree", "Snapshot Preview", "uiint.preview", {"dev"}});

    ImGui::Separator();
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

    ImGui::Separator();
    ImGui::TextUnformatted("UI Design Assistant");

    ImGui::InputTextMultiline("Goal", s_designGoal, sizeof(s_designGoal), ImVec2(-1, 60));
    uiint.RegisterWidget({"text", "Goal", "ui_design.goal", {"dev", "input"}});

    ImGui::BeginDisabled(s_designBusy.load());
    if (ImGui::Button("Generate Design Plan (from introspection)")) {
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
    ImGui::EndDisabled();
    uiint.RegisterWidget({"button", "Generate Design Plan (from introspection)", "action:ai.ui_design.generate_plan", {"action", "ai", "dev"}});

    {
        std::scoped_lock lock(s_designMutex);
        if (!s_designStatus.empty()) {
            ImGui::TextWrapped("%s", s_designStatus.c_str());
        }
    }

    uiint.EndPanel();
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::DevShell
