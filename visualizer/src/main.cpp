// namespace main.cpp: Entry point for the visualizer, demonstrating integration of the EditorHFSM with a simple ImGui interface. This file should remain focused on application setup and the main loop, while delegating UI rendering and state management to other modules to maintain separation of concerns and avoid ODR violations. Any shared state or utilities needed across multiple files should be defined in appropriate header/source files within the Core/Editor namespace, and this file should only include those headers without defining any additional state or non-trivial functions here.
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"

// ADDED (visualizer/src/main.cpp): Hyper-reactive UI root includes.
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

using RogueCity::Core::Editor::EditorEvent;
using RogueCity::Core::Editor::EditorHFSM;
using RogueCity::Core::Editor::EditorState;
using RogueCity::Core::Editor::GlobalState;

namespace {
// this namespace is for internal linkage of helper functions and should not contain any state or definitions that need to be shared across translation units.
// we must not define any non-trivial functions or state here that are needed in other files, to avoid ODR violations and maintain clear separation of concerns. This is strictly for implementation details local to this file.
// we must always ensure that any function defined here is either inline or static to prevent linkage issues, and we should avoid including any headers here that are not needed for the implementation of these functions to minimize coupling and compilation dependencies.
[[nodiscard]] constexpr ImGuiConfigFlags DockingConfigFlag() {
#if defined(IMGUI_HAS_DOCK)
    return ImGuiConfigFlags_DockingEnable;
#else
    return 0;
#endif
}

} // namespace

static void draw_main_menu(EditorHFSM& hfsm, GlobalState& gs)
{
    (void)gs;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Mode")) {
            if (ImGui::MenuItem("Idle", nullptr, hfsm.state() == EditorState::Idle)) {
                hfsm.handle_event(EditorEvent::GotoIdle, gs);
            }
            if (ImGui::MenuItem("Edit Roads", nullptr, hfsm.state() == EditorState::Editing_Roads)) {
                hfsm.handle_event(EditorEvent::Tool_Roads, gs);
            }
            if (ImGui::MenuItem("Edit Districts", nullptr, hfsm.state() == EditorState::Editing_Districts)) {
                hfsm.handle_event(EditorEvent::Tool_Districts, gs);
            }
            if (ImGui::MenuItem("Simulate", nullptr, hfsm.state() == EditorState::Simulating)) {
                hfsm.handle_event(EditorEvent::BeginSim, gs);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

static void draw_editor_windows(EditorHFSM& hfsm, GlobalState& gs)
{
    switch (hfsm.state()) {
    case EditorState::Editing_Roads:
        ImGui::Begin("Roads");
        ImGui::Text("Road count: %llu", static_cast<unsigned long long>(gs.roads.size()));
        ImGui::End();
        break;
    case EditorState::Editing_Districts:
        ImGui::Begin("Districts");
        ImGui::Text("District count: %llu", static_cast<unsigned long long>(gs.districts.size()));
        ImGui::End();
        break;
    case EditorState::Simulating:
        ImGui::Begin("Simulation");
        ImGui::Text("Simulation running...");
        ImGui::End();
        break;
    default:
        break;
    }
}

static void apply_theme_once()
{
    // ADDED (visualizer/src/main.cpp): Apply RC_UI theme a single time after NewFrame.
    static bool theme_applied = false;
    if (!theme_applied) {
        RC_UI::ApplyTheme();
        theme_applied = true;
    }
}

int main(int argc, char** argv)
{
    int frames_to_run = 3;
    std::string export_ui_snapshot_path;
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (std::strcmp(arg, "--help") == 0) {
            std::printf("RogueCityVisualizerHeadless options:\n");
            std::printf("  --frames <N>                 Number of headless frames (default: 3)\n");
            std::printf("  --export-ui-snapshot <path>  Write UI introspection JSON snapshot\n");
            return 0;
        }
        if (std::strcmp(arg, "--frames") == 0 && i + 1 < argc) {
            frames_to_run = std::max(1, std::atoi(argv[++i]));
            continue;
        }
        if (std::strcmp(arg, "--export-ui-snapshot") == 0 && i + 1 < argc) {
            export_ui_snapshot_path = argv[++i];
            continue;
        }
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();

    hfsm.handle_event(EditorEvent::BootComplete, gs);
    hfsm.handle_event(EditorEvent::NewProject, gs);
    hfsm.handle_event(EditorEvent::ProjectLoaded, gs);
    hfsm.handle_event(EditorEvent::Tool_Roads, gs);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    // ADDED (visualizer/src/main.cpp): Enable ImGui docking for the RC_UI dockspace.
    io.ConfigFlags |= DockingConfigFlag();
    io.Fonts->Build();
    unsigned char* font_pixels = nullptr;
    int font_width = 0;
    int font_height = 0;
    io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);

    // Headless "frame loop" demonstrating HFSM integration.
    auto run_frame = [&]() {
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();

        // ADDED (visualizer/src/main.cpp): Hyper-reactive UI root hook.
        apply_theme_once();
        RC_UI::DrawRoot(io.DeltaTime);

        hfsm.update(gs, io.DeltaTime);
        draw_main_menu(hfsm, gs);
        draw_editor_windows(hfsm, gs);

        ImGui::Render();
        ++gs.frame_counter;
    };

    for (int frame = 0; frame < frames_to_run; ++frame) {
        run_frame();
    }

    if (!export_ui_snapshot_path.empty()) {
        std::string error;
        const bool saved = RogueCity::UIInt::UiIntrospector::Instance().SaveSnapshotJson(
            export_ui_snapshot_path, &error);
        if (!saved) {
            std::fprintf(stderr, "Failed to save UI snapshot to '%s': %s\n",
                         export_ui_snapshot_path.c_str(), error.c_str());
            ImGui::DestroyContext();
            return 2;
        }
        std::printf("UI snapshot exported: %s\n", export_ui_snapshot_path.c_str());
    }

    ImGui::DestroyContext();
    return 0;
}
