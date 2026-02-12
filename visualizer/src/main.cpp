#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

// ADDED (visualizer/src/main.cpp): Hyper-reactive UI root includes.
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>

using RogueCity::Core::Editor::EditorEvent;
using RogueCity::Core::Editor::EditorHFSM;
using RogueCity::Core::Editor::EditorState;
using RogueCity::Core::Editor::GlobalState;

namespace {

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

int main()
{
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

    for (int frame = 0; frame < 3; ++frame) {
        run_frame();
    }

    ImGui::DestroyContext();
    return 0;
}
