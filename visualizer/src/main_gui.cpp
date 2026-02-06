#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// OpenGL loader
#include "GL/gl3w.h"

#include <GLFW/glfw3.h>
#include <stdio.h>

using RogueCity::Core::Editor::EditorEvent;
using RogueCity::Core::Editor::EditorHFSM;
using RogueCity::Core::Editor::EditorState;
using RogueCity::Core::Editor::GlobalState;

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

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if __APPLE__
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "RogueCity Visualizer", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize OpenGL loader (gl3w)
    if (gl3wInit()) {
        fprintf(stderr, "Failed to initialize OpenGL loader (gl3w)\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();

    hfsm.handle_event(EditorEvent::BootComplete, gs);
    hfsm.handle_event(EditorEvent::NewProject, gs);
    hfsm.handle_event(EditorEvent::ProjectLoaded, gs);
    hfsm.handle_event(EditorEvent::Tool_Roads, gs);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        hfsm.update(gs, ImGui::GetIO().DeltaTime);
        draw_main_menu(hfsm, gs);
        draw_editor_windows(hfsm, gs);

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
