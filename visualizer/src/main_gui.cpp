#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

// UI system includes
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_theme.h"
#include "RogueCity/App/UI/DesignSystem.h"  // Cockpit Doctrine theme
#include "RogueCity/App/UI/ThemeManager.h"  // Multi-theme system

// AI system includes
#include "config/AiConfig.h"

#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Header-only input adapter (Win32 polling) //todo ensure we are not hardcoding platform-specific input handling and that this can be easily extended or replaced with a more robust solution in the future (e.g. event-driven, multi-platform support).
#if __has_include(<LInput.h>)
    #include <LInput.h>
    #define ROGUECITY_HAS_LINPUT 1
#endif

// OpenGL loader - must be included before GLFW
#include <GL/gl3w.h>

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>

using RogueCity::Core::Editor::EditorEvent;
using RogueCity::Core::Editor::EditorHFSM;
using RogueCity::Core::Editor::EditorState;
using RogueCity::Core::Editor::GlobalState;
// Note: This example code is using the GL3W OpenGL loader. You may use any other loader (e.g. glad, glew, etc.) by changing the include and initialization code.
// If you use a different loader, be sure to initialize it before calling any OpenGL functions.
// For example, with glad you would do:
// #include <glad/glad.h>
// if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
//     fprintf(stderr, "Failed to initialize OpenGL loader!\n");
//     return 1;
// }
// The rest of the code should work regardless of the OpenGL loader you choose, as long as it is properly initialized before use.

// todo: refactor this main function to delegate responsibilities to a dedicated Application class that manages the lifecycle, state, and main loop of the application. This will improve modularity, testability, and maintainability by encapsulating application logic and allowing for cleaner separation of concerns. The Application class can handle initialization, event processing, state updates, and rendering, while the main function simply creates an instance of the Application and starts it.
// todo consider unifying the main loop with a more robust game loop structure that includes fixed timestep updates for simulation and variable timestep rendering, to ensure consistent behavior regardless of frame rate fluctuations. This would involve implementing a time accumulator and separating the update and render phases more cleanly, allowing for smoother simulations and better handling of edge cases like slow frames or pauses.
// todo  look into unifying  Glew vs Glad vs GLew with a single abstraction layer for OpenGL loading, to allow for easier switching between loaders and better modularity. This could involve creating a simple wrapper that abstracts the initialization and function loading process, allowing the underlying loader to be swapped out with minimal changes to the rest of the codebase. This would also help with testing and platform compatibility in the future.
namespace {

[[nodiscard]] constexpr ImGuiConfigFlags DockingConfigFlag() {
#if defined(IMGUI_HAS_DOCK)
    return ImGuiConfigFlags_DockingEnable;
#else
    return 0;
#endif
}

[[nodiscard]] constexpr ImGuiConfigFlags ViewportsConfigFlag() {
#if defined(IMGUI_HAS_DOCK)
    return ImGuiConfigFlags_ViewportsEnable;
#else
    return 0;
#endif
}

[[nodiscard]] bool ShouldEnablePlatformViewports() {
#if defined(IMGUI_HAS_DOCK)
#if defined(_WIN32)
    char* value = nullptr;
    size_t value_len = 0;
    if (_dupenv_s(&value, &value_len, "ROGUE_ENABLE_IMGUI_VIEWPORTS") != 0 || value == nullptr) {
        return false;
    }
    const bool enabled = std::strcmp(value, "1") == 0 ||
        std::strcmp(value, "true") == 0 ||
        std::strcmp(value, "TRUE") == 0;
    std::free(value);
    return enabled;
#else
    const char* value = std::getenv("ROGUE_ENABLE_IMGUI_VIEWPORTS");
    if (value == nullptr) {
        return false;
    }
    return std::strcmp(value, "1") == 0 ||
        std::strcmp(value, "true") == 0 ||
        std::strcmp(value, "TRUE") == 0;
#endif
#else
    return false;
#endif
}

} // namespace

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
    auto& gs_early = RogueCity::Core::Editor::GetGlobalState();

    // Enable docking in all GUI builds.
    io.ConfigFlags |= DockingConfigFlag();
    io.ConfigDpiScaleFonts = gs_early.config.ui_dpi_scale_fonts_enabled;
    io.ConfigDpiScaleViewports = gs_early.config.ui_dpi_scale_viewports_enabled;

    // Multi-viewport remains opt-in (config or explicit env override).
    const bool enable_platform_viewports =
        gs_early.config.ui_multi_viewport_enabled || ShouldEnablePlatformViewports();
    if (enable_platform_viewports) {
        io.ConfigFlags |= ViewportsConfigFlag();
    } else {
        io.ConfigFlags &= ~ViewportsConfigFlag();
    }
    
    // When viewports are enabled, tweak WindowRounding/WindowBg for platform windows
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ViewportsConfigFlag())
    {
        style.WindowRounding = 0.0f;  // Y2K hard edges
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;  // Opaque backgrounds for OS windows
    }

    // Initialize Theme Manager and apply saved theme
    auto& theme_mgr = RogueCity::UI::ThemeManager::Instance();
    if (!gs_early.config.active_theme.empty()) {
        theme_mgr.LoadTheme(gs_early.config.active_theme);
    } else {
        // Default theme on first launch
        theme_mgr.LoadTheme("Default");
        gs_early.config.active_theme = "Default";
    }
    
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

    bool was_iconified = false;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        const bool is_iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE;
        if (is_iconified) {
            was_iconified = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        if (was_iconified) {
            RC_UI::ResetDockLayout();
            was_iconified = false;
        }

        #if defined(ROGUECITY_HAS_LINPUT)
        // Poll platform input and (optionally) feed ImGui IO
        input.Update();
        if (!g_disable_linput_injection) {
            io.AddMousePosEvent(static_cast<float>(input.Mouse.X()), static_cast<float>(input.Mouse.Y()));
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, input.Mouse.LeftButtonDown());
            io.AddMouseButtonEvent(ImGuiMouseButton_Right, input.Mouse.RightButtonDown());
            io.AddMouseButtonEvent(ImGuiMouseButton_Middle, input.Mouse.MiddleButtonDown());
            io.AddKeyEvent(ImGuiMod_Ctrl, input.Keyboard.CtrlDown());
            io.AddKeyEvent(ImGuiMod_Shift, input.Keyboard.ShiftDown());
            io.AddKeyEvent(ImGuiMod_Alt, input.Keyboard.AltDown());
        }

        // Legacy key array feed removed: this imgui build does not expose `io.KeysDown`.
        // Modifier state is forwarded through io.AddKeyEvent(ImGuiMod_*).
        #endif

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Update HFSM and draw all panels with docking enabled
        float dt = io.DeltaTime;
        hfsm.update(gs, dt);
        
        // Hard reset (Ctrl+Shift+R): clear all saved ImGui window state.
        if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_R)) {
            ImGui::LoadIniSettingsFromMemory("", 0);
            RC_UI::ResetDockLayout();
        } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R)) {
            // Soft reset (Ctrl+R): rebuild dock tree but keep persisted window settings.
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
#if defined(IMGUI_HAS_DOCK)
        if (io.ConfigFlags & ViewportsConfigFlag())
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
#endif

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
