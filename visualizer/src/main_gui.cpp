#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

// UI system includes
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_theme.h"
#include "RogueCity/App/UI/DesignSystem.h"  // Cockpit Doctrine theme

// AI system includes
#include "config/AiConfig.h"

#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Header-only input adapter (Win32 polling)
#if __has_include(<LInput.h>)
    #include <LInput.h>
    #define ROGUECITY_HAS_LINPUT 1
#endif

// OpenGL loader - must be included before GLFW
#include <GL/gl3w.h>

#include <GLFW/glfw3.h>
#include <stdio.h>

using RogueCity::Core::Editor::EditorEvent;
using RogueCity::Core::Editor::EditorHFSM;
using RogueCity::Core::Editor::EditorState;
using RogueCity::Core::Editor::GlobalState;

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

    // Keep OS decorations for now (custom chrome is complex)
    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);  // Disabled for easier window management

    GLFWwindow* window = glfwCreateWindow(1280, 720, "RC-USD", NULL, NULL);
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
    
    // CRITICAL: Enable docking + multi-viewport for floating windows
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    // When viewports are enabled, tweak WindowRounding/WindowBg for platform windows
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;  // Y2K hard edges
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;  // Opaque backgrounds for OS windows
    }

    // Apply Cockpit Doctrine theme (MUST be before any UI rendering)
    RogueCity::UI::DesignSystem::ApplyCockpitTheme();
    
    // Load AI configuration
    RogueCity::AI::AiConfigManager::Instance().LoadFromFile("AI/ai_config.json");

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

    auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();

    hfsm.handle_event(EditorEvent::BootComplete, gs);
    hfsm.handle_event(EditorEvent::NewProject, gs);
    hfsm.handle_event(EditorEvent::ProjectLoaded, gs);
    hfsm.handle_event(EditorEvent::Tool_Roads, gs);

    #if defined(ROGUECITY_HAS_LINPUT)
    // LInput init
    LInput::Input input;
    input.Init();
    // Disable legacy input injection by default while diagnosing capture conflicts
    static bool g_disable_linput_injection = true;
    #endif

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        #if defined(ROGUECITY_HAS_LINPUT)
        // Poll platform input and (optionally) feed ImGui IO
        input.Update();
        if (!g_disable_linput_injection) {
            io.MousePos = ImVec2(static_cast<float>(input.Mouse.X()), static_cast<float>(input.Mouse.Y()));
            io.MouseDown[0] = input.Mouse.LeftButtonDown();
            io.MouseDown[1] = input.Mouse.RightButtonDown();
            io.MouseDown[2] = input.Mouse.MiddleButtonDown();

            io.KeyCtrl = input.Keyboard.CtrlDown();
            io.KeyShift = input.Keyboard.ShiftDown();
            io.KeyAlt = input.Keyboard.AltDown();
        }

        // Legacy key array feed removed: this imgui build does not expose `io.KeysDown`.
        // We still set modifier keys above (KeyCtrl/KeyShift/KeyAlt) when injection is enabled.
        #endif

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Update HFSM and draw all panels with docking enabled
        float dt = io.DeltaTime;
        hfsm.update(gs, dt);
        
        // Add main menu bar for window management
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Window")) {
                if (ImGui::MenuItem("Reset Layout", "Ctrl+R")) {
                    RC_UI::ResetDockLayout();
                }
                ImGui::Separator();
                ImGui::Text("Tip: Drag panel titles to dock/undock");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        
        // Hotkey for reset layout
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R)) {
            RC_UI::ResetDockLayout();
        }
        
        RC_UI::DrawRoot(dt);

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and render platform windows (multi-viewport support)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

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
