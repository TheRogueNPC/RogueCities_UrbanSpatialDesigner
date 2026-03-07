// namespace main.cpp: Entry point for the visualizer, demonstrating integration
// of the EditorHFSM with a simple ImGui interface.
#include "RogueCity/App/Integration/DatabaseComputeWorker.hpp"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/panels/rc_panel_imgui_error.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_theme.h"
#include <RogueCity/Visualizer/SvgTextureCache.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <imgui.h>
#ifdef ROGUECITY_HAS_IMPLOT
#include <implot.h>
#endif
#ifdef ROGUECITY_HAS_IMPLOT3D
#include <implot3d.h>
#endif
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#endif

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

struct RuntimeOptions {
  int frames_to_run = 3;
  bool interactive_mode = false;
  std::string export_ui_snapshot_path;
  std::string export_ui_screenshot_path;
};

[[nodiscard]] RuntimeOptions ParseOptions(int argc, char **argv) {
  RuntimeOptions opts{};
  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (std::strcmp(arg, "--interactive") == 0 || std::strcmp(arg, "-i") == 0) {
      opts.interactive_mode = true;
      continue;
    }
    if (std::strcmp(arg, "--frames") == 0 && i + 1 < argc) {
      const int parsed_frames = std::atoi(argv[++i]);
      opts.frames_to_run = (parsed_frames > 1) ? parsed_frames : 1;
      continue;
    }
    if (std::strcmp(arg, "--export-ui-snapshot") == 0 && i + 1 < argc) {
      opts.export_ui_snapshot_path = argv[++i];
      continue;
    }
    if (std::strcmp(arg, "--export-ui-screenshot") == 0 && i + 1 < argc) {
      opts.export_ui_screenshot_path = argv[++i];
      continue;
    }
  }
  return opts;
}

// --- TUI Cyberpunk Styling ---
namespace Style {
constexpr const char *Reset = "\033[0m";
constexpr const char *Red = "\033[38;5;196m";
constexpr const char *Gold = "\033[38;5;220m";
constexpr const char *Orange = "\033[38;5;208m";
constexpr const char *Cyan = "\033[38;5;51m";
constexpr const char *DarkGray = "\033[38;5;236m";
} // namespace Style

void PrintBanner() {
  std::cout << Style::Red << R"(
    ____  ____  ____  __  __  ____    ___  ____  ____  _  _    ____  ____  ___  ____  ____  __ _  ____  ____ 
   (  _ \(  _ \(  __)(  )(  )(  __)  / __)(_  _)(_  _)( \/ )  (  _ \(  __)/ __)(_  _)(  _ \(  ( \(  __)(  _ \
    )   / )(_) ))_)   )(__)(  ) _)  ( (__  _)(_   )(   \  /    )(_) ))_) \__ \  _)(_  ) _ (/    / ) _)  )   /
   (_)\_)(____/(____)(______)(____)  \___)(____) (__)  (__)   (____/(____)(___/(____)(____/\_)__)(____)(_)\_)
  )" << Style::Reset
            << "\n";
  std::cout << Style::Gold << "   [:: HYPER-REACTIVE DEV CONSOLE ONLINE ::]"
            << Style::Reset << "\n";
  std::cout << Style::Orange << "   [:: Type 'help' for available commands ::]"
            << Style::Reset << "\n\n";
}

// --- Thread-Safe Command Queue ---
namespace Subprocess {
inline void ExecuteRaw(const std::string &cmd) {
  std::cout << Style::DarkGray;
#ifdef _WIN32
  std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"),
                                                 _pclose);
#else
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);
#endif
  if (!pipe) {
    std::cout << Style::Red << "[error] Failed to launch subprocess!\n"
              << Style::Reset << std::flush;
    return;
  }
  std::array<char, 256> buffer;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) !=
         nullptr) {
    std::cout << buffer.data();
  }
  std::cout << Style::Reset << std::flush;
}

inline void ExecuteDevShell(const std::string &cmd_suffix) {
  std::string full_cmd =
      "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"& { . "
      ".\\tools\\dev-shell.ps1 > $null; " +
      cmd_suffix + " }\" 2>&1";
  ExecuteRaw(full_cmd);
}
} // namespace Subprocess

class CommandQueue {
public:
  void Push(const std::string &cmd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(cmd);
  }
  bool TryPop(std::string &out_cmd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty())
      return false;
    out_cmd = m_queue.front();
    m_queue.pop();
    return true;
  }

private:
  std::mutex m_mutex;
  std::queue<std::string> m_queue;
};

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
struct ScreenshotRuntimeContext {
  GLFWwindow *window = nullptr;
  bool active = false;
  bool imgui_backends_active = false;
};

void glfw_error_callback(int error, const char *description) {
  std::fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool InitScreenshotRuntime(ScreenshotRuntimeContext &ctx, int width,
                           int height) {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) {
    std::fprintf(stderr, "Failed to initialize GLFW for screenshot export.\n");
    return false;
  }

#if __APPLE__
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  ctx.window =
      glfwCreateWindow(width, height, "RogueCityHeadless", nullptr, nullptr);
  if (ctx.window == nullptr) {
    std::fprintf(
        stderr,
        "Failed to create hidden OpenGL window for screenshot export.\n");
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(ctx.window);
  if (gl3wInit() != 0) {
    std::fprintf(stderr, "Failed to initialize OpenGL loader (gl3w).\n");
    glfwDestroyWindow(ctx.window);
    ctx.window = nullptr;
    glfwTerminate();
    return false;
  }
  ctx.active = true;
  return true;
}

bool InitScreenshotImGuiBackends(ScreenshotRuntimeContext &ctx) {
  if (!ctx.active || ctx.window == nullptr) {
    return false;
  }
  if (!ImGui_ImplGlfw_InitForOpenGL(ctx.window, false)) {
    std::fprintf(
        stderr,
        "Failed to initialize ImGui GLFW backend for screenshot export.\n");
    return false;
  }
  if (!ImGui_ImplOpenGL3_Init("#version 130")) {
    std::fprintf(
        stderr,
        "Failed to initialize ImGui OpenGL backend for screenshot export.\n");
    ImGui_ImplGlfw_Shutdown();
    return false;
  }
  ctx.imgui_backends_active = true;
  return true;
}

void ShutdownScreenshotRuntime(ScreenshotRuntimeContext &ctx) {
  if (ctx.imgui_backends_active) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
  }
  if (ctx.window != nullptr) {
    glfwDestroyWindow(ctx.window);
  }
  glfwTerminate();
  ctx.window = nullptr;
  ctx.active = false;
  ctx.imgui_backends_active = false;
}

bool ExportFramebufferPng(const std::string &path, GLFWwindow *window) {
  if (window == nullptr || path.empty()) {
    return false;
  }

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  if (width <= 0 || height <= 0) {
    std::fprintf(stderr,
                 "Invalid framebuffer size for screenshot export (%d x %d).\n",
                 width, height);
    return false;
  }

  std::vector<unsigned char> pixels(
      static_cast<size_t>(width) * static_cast<size_t>(height) * 4u, 0u);
  std::vector<unsigned char> flipped(pixels.size(), 0u);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadBuffer(GL_BACK);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  const size_t row_bytes = static_cast<size_t>(width) * 4u;
  for (int y = 0; y < height; ++y) {
    const size_t src_offset = static_cast<size_t>(y) * row_bytes;
    const size_t dst_offset = static_cast<size_t>(height - 1 - y) * row_bytes;
    std::memcpy(flipped.data() + dst_offset, pixels.data() + src_offset,
                row_bytes);
  }

  std::filesystem::path screenshot_path(path);
  if (screenshot_path.has_parent_path()) {
    std::error_code ec;
    std::filesystem::create_directories(screenshot_path.parent_path(), ec);
  }

  const int ok =
      stbi_write_png(path.c_str(), width, height, 4, flipped.data(), width * 4);
  if (ok == 0) {
    std::fprintf(stderr, "stbi_write_png failed for '%s'.\n", path.c_str());
    return false;
  }
  return true;
}
#endif

void draw_main_menu(EditorHFSM &hfsm, GlobalState &gs) {
  (void)gs;

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Mode")) {
      if (ImGui::MenuItem("Idle", nullptr, hfsm.state() == EditorState::Idle)) {
        hfsm.handle_event(EditorEvent::GotoIdle, gs);
      }
      if (ImGui::MenuItem("Edit Roads", nullptr,
                          hfsm.state() == EditorState::Editing_Roads)) {
        hfsm.handle_event(EditorEvent::Tool_Roads, gs);
      }
      if (ImGui::MenuItem("Edit Districts", nullptr,
                          hfsm.state() == EditorState::Editing_Districts)) {
        hfsm.handle_event(EditorEvent::Tool_Districts, gs);
      }
      if (ImGui::MenuItem("Simulate", nullptr,
                          hfsm.state() == EditorState::Simulating)) {
        hfsm.handle_event(EditorEvent::BeginSim, gs);
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void draw_editor_windows(EditorHFSM &hfsm, GlobalState &gs) {
  switch (hfsm.state()) {
  case EditorState::Editing_Roads:
    ImGui::Begin("Roads");
    ImGui::Text("Road count: %llu",
                static_cast<unsigned long long>(gs.roads.size()));
    ImGui::End();
    break;
  case EditorState::Editing_Districts:
    ImGui::Begin("Districts");
    ImGui::Text("District count: %llu",
                static_cast<unsigned long long>(gs.districts.size()));
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

void apply_theme_once() {
  static bool theme_applied = false;
  if (!theme_applied) {
    RC_UI::ApplyTheme();
    theme_applied = true;
  }
}

} // namespace

int main(int argc, char **argv) {
  const RuntimeOptions opts = ParseOptions(argc, argv);

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--help") == 0) {
      std::printf("RogueCityVisualizerHeadless options:\n");
      std::printf("  -i, --interactive              Start the cyberpunk "
                  "interactive REPL console\n");
      std::printf("  --frames <N>                   Number of headless frames "
                  "(default: 3)\n");
      std::printf("  --export-ui-snapshot <path>    Write UI introspection "
                  "JSON snapshot\n");
      std::printf(
          "  --export-ui-screenshot <path>  Write runtime screenshot PNG\n");
      return 0;
    }
  }

  const bool wants_screenshot = !opts.export_ui_screenshot_path.empty();

#if !defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  if (wants_screenshot) {
    std::fprintf(stderr, "Screenshot export requested but this build has no "
                         "OpenGL screenshot runtime.\n");
    return 3;
  }
#endif

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  ScreenshotRuntimeContext screenshot_ctx{};
  if (wants_screenshot && !InitScreenshotRuntime(screenshot_ctx, 1024, 576)) {
    return 3;
  }
#endif

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
#ifdef ROGUECITY_HAS_IMPLOT
  ImPlot::CreateContext(); // must follow ImGui::CreateContext
#endif
#ifdef ROGUECITY_HAS_IMPLOT3D
  ImPlot3D::CreateContext(); // must follow ImGui::CreateContext
#endif

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  if (wants_screenshot && !InitScreenshotImGuiBackends(screenshot_ctx)) {
#ifdef ROGUECITY_HAS_IMPLOT
    ImPlot::DestroyContext();
#endif
#ifdef ROGUECITY_HAS_IMPLOT3D
    ImPlot3D::DestroyContext();
#endif
    ImGui::DestroyContext();
    ShutdownScreenshotRuntime(screenshot_ctx);
    return 3;
  }
#endif

  auto &hfsm = RogueCity::Core::Editor::GetEditorHFSM();
  auto &gs = RogueCity::Core::Editor::GetGlobalState();

  RogueCity::App::Integration::DatabaseComputeWorker db_worker(
      gs, "rogue_cities_db", "http://localhost:3000");
  db_worker.Start();

  hfsm.handle_event(EditorEvent::BootComplete, gs);
  hfsm.handle_event(EditorEvent::NewProject, gs);
  hfsm.handle_event(EditorEvent::ProjectLoaded, gs);
  hfsm.handle_event(EditorEvent::Tool_Roads, gs);

  RC_UI::Panels::ImGuiError::Init();

  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(1280.0f, 720.0f);
  io.ConfigFlags |= DockingConfigFlag();
  io.Fonts->Build();
  unsigned char *font_pixels = nullptr;
  int font_width = 0;
  int font_height = 0;
  io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  if (wants_screenshot) {
    int fb_w = 0;
    int fb_h = 0;
    glfwGetFramebufferSize(screenshot_ctx.window, &fb_w, &fb_h);
    if (fb_w > 0 && fb_h > 0) {
      io.DisplaySize =
          ImVec2(static_cast<float>(fb_w), static_cast<float>(fb_h));
    }
  }
#endif

  auto run_frame = [&]() {
#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
    if (wants_screenshot) {
      glfwPollEvents();
      ImGui_ImplGlfw_NewFrame();
      ImGui_ImplOpenGL3_NewFrame();
      int fb_w = 0;
      int fb_h = 0;
      glfwGetFramebufferSize(screenshot_ctx.window, &fb_w, &fb_h);
      if (fb_w > 0 && fb_h > 0) {
        io.DisplaySize =
            ImVec2(static_cast<float>(fb_w), static_cast<float>(fb_h));
      }
    }
#endif
    io.DeltaTime = 1.0f / 60.0f;
    ImGui::NewFrame();

    apply_theme_once();
    // HFSM is the application state driver. Update state FIRST so that
    // DrawRoot, draw_main_menu, and draw_editor_windows all see the same
    // current-frame HFSM state in one render call.
    // (Matches the canonical game loop: Poll → UpdateState → Render → Present)
    hfsm.update(gs, io.DeltaTime);
    RC_UI::DrawRoot(io.DeltaTime);
    draw_main_menu(hfsm, gs);
    draw_editor_windows(hfsm, gs);

    ImGui::Render();

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
    if (wants_screenshot) {
      int display_w = 0;
      int display_h = 0;
      glfwGetFramebufferSize(screenshot_ctx.window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(0.10f, 0.10f, 0.12f, 1.00f);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      glFinish();
    }
#endif
    ++gs.frame_counter;
  };

  if (opts.interactive_mode) {
    PrintBanner();
    CommandQueue cmd_queue;
    std::atomic<bool> keep_running{true};
    int auto_steps = -1; // -1 means run infinitely, 0 means paused, N means run
                         // N frames then pause

    std::thread repl_thread([&]() {
      std::string line;
      while (keep_running) {
        std::cout << Style::Red << "rc> " << Style::Reset << std::flush;
        if (!std::getline(std::cin, line)) {
          // EOF
          cmd_queue.Push("quit");
          break;
        }
        if (line.empty())
          continue;

        // Subprocess interceptors (Blocking on REPL thread keeps UI 60fps)
        if (line.rfind("shell ", 0) == 0 || line.rfind("! ", 0) == 0) {
          std::string sub = line.substr(line.find(' ') + 1);
          std::cout << Style::Orange << "[shell] executing: " << sub << "\n"
                    << Style::Reset;
          Subprocess::ExecuteRaw("powershell.exe -NoProfile -Command \"" + sub +
                                 "\" 2>&1");
        } else if (line == "ai_start") {
          std::cout << Style::Orange << "[ai_start] booting bridge...\n"
                    << Style::Reset;
          Subprocess::ExecuteDevShell("rc-ai-start-wsl");
        } else if (line.rfind("ai_query ", 0) == 0) {
          std::string prompt = line.substr(9);
          std::cout << Style::Orange << "[ai_query] processing...\n"
                    << Style::Reset;
          Subprocess::ExecuteDevShell("rc-ai-query -Prompt '" + prompt + "'");
        } else if (line == "doctor") {
          std::cout << Style::Orange << "[doctor] running diagnostics...\n"
                    << Style::Reset;
          Subprocess::ExecuteDevShell("rc-doctor");
        } else if (line == "problems") {
          Subprocess::ExecuteDevShell("rc-problems");
        } else if (line == "build") {
          std::cout << Style::Orange
                    << "[build] recompiling headless target...\n"
                    << Style::Reset;
          Subprocess::ExecuteDevShell("rc-bld-headless");
        } else if (line == "agent") {
          const char *active = std::getenv("RC_ACTIVE_AGENT");
          const char *bridge = std::getenv("RC_AI_BRIDGE_BASE_URL");
          std::cout << Style::Cyan
                    << "[agent] RC_ACTIVE_AGENT    = " << Style::Gold
                    << (active ? active : "(unset)") << Style::Reset << "\n"
                    << Style::Cyan
                    << "[agent] RC_AI_BRIDGE_BASE_URL = " << Style::Gold
                    << (bridge ? bridge : "(unset)") << Style::Reset << "\n"
                    << std::flush;
        } else if (line == "claude_status") {
          std::cout << Style::Cyan << "[claude_status] fetching...\n"
                    << Style::Reset;
          Subprocess::ExecuteDevShell("rc-claude-status");
        } else if (line == "claude_handoff") {
          std::cout << Style::Cyan
                    << "[claude_handoff] writing handoff brief...\n"
                    << Style::Reset;
          Subprocess::ExecuteDevShell("rc-claude-handoff");
        } else if (line == "help" || line == "?") {
          std::cout
              << Style::Gold << "Available REPL / Shell commands:\n"
              << Style::Red << "  -- DEV SHELL SUBPROCESS --\n"
              << Style::Gold
              << "  ! <cmd> / shell <cmd> - Run arbitrary shell command\n"
              << "  build                 - Rebuild headless executable\n"
              << "  doctor                - Run environment diagnostics\n"
              << "  problems              - Fetch latest compilation issues\n"
              << "  ai_start              - Start local AI proxy server\n"
              << "  ai_query <prompt>     - Query local AI model\n"
              << "  agent                 - Show active agent + bridge URL\n"
              << "  claude_status         - Claude Code memory + config\n"
              << "  claude_handoff        - Write handoff brief to "
                 "AI/collaboration/\n\n"
              << Style::Red << "  -- UI ENGINE CONTEXT --\n"
              << Style::Gold
              << "  dump_ui               - Dump ImGui introspection JSON\n"
              << "  state                 - Print Global State summary\n"
              << "  hfsm <event>          - Dispatch EditorEvent (GotoIdle, "
                 "Tool_Roads, etc)\n"
              << "  step [N]              - Run N frames and pause\n"
              << "  pause                 - Pause simulation loop\n"
              << "  resume / play         - Unpause simulation loop\n"
              << "  quit / exit           - Terminate console\n"
              << Style::Reset << std::flush;
        } else {
          cmd_queue.Push(line);
          std::this_thread::sleep_for(
              std::chrono::milliseconds(20)); // wait for main thread print
        }

        if (line == "quit" || line == "exit")
          break;
      }
    });

    while (keep_running) {
      std::string cmd;
      while (cmd_queue.TryPop(cmd)) {
        if (cmd == "quit" || cmd == "exit") {
          std::cout << Style::Gold << "Shutting down the console..."
                    << Style::Reset << "\n"
                    << std::flush;
          keep_running = false;
        } else if (cmd == "dump_ui") {
          std::cout << Style::Orange << "[dump_ui] " << Style::Reset
                    << "Capturing Inspector State...\n";
          std::cout << Style::DarkGray
                    << RogueCity::UIInt::UiIntrospector::Instance()
                           .SnapshotJson()
                           .dump(2)
                    << Style::Reset << "\n"
                    << std::flush;
        } else if (cmd == "state") {
          std::cout << Style::Orange << "[state] " << Style::Reset
                    << "Global State Summary:\n";
          std::cout << Style::Gold << "  Frame: " << gs.frame_counter << "\n"
                    << "  Roads: " << gs.roads.size() << "\n"
                    << "  Districts: " << gs.districts.size() << Style::Reset
                    << "\n"
                    << std::flush;
        } else if (cmd.rfind("hfsm ", 0) == 0) {
          std::string event_name = cmd.substr(5);
          std::cout << Style::Orange << "[hfsm] " << Style::Reset
                    << "Dispatching event: " << event_name << "\n";
          if (event_name == "GotoIdle")
            hfsm.handle_event(EditorEvent::GotoIdle, gs);
          else if (event_name == "Tool_Roads")
            hfsm.handle_event(EditorEvent::Tool_Roads, gs);
          else if (event_name == "Tool_Districts")
            hfsm.handle_event(EditorEvent::Tool_Districts, gs);
          else if (event_name == "BeginSim")
            hfsm.handle_event(EditorEvent::BeginSim, gs);
          else {
            std::cout << Style::Red
                      << "Unknown event. Try: GotoIdle, Tool_Roads, "
                         "Tool_Districts, BeginSim"
                      << Style::Reset << "\n"
                      << std::flush;
          }
        } else if (cmd.rfind("step", 0) == 0) {
          std::istringstream iss(cmd.substr(4));
          int count = 1;
          iss >> count;
          if (count < 1)
            count = 1;
          auto_steps = count;
          std::cout << Style::Orange << "[step] " << Style::Reset << "Stepping "
                    << count << " frames\n"
                    << std::flush;
        } else if (cmd == "pause") {
          auto_steps = 0;
          std::cout << Style::Orange << "[pause] " << Style::Reset
                    << "Frame loop paused\n"
                    << std::flush;
        } else if (cmd == "resume" || cmd == "play") {
          auto_steps = -1;
          std::cout << Style::Orange << "[resume] " << Style::Reset
                    << "Frame loop unpaused\n"
                    << std::flush;
        } else {
          std::cout << Style::Red << "Command not recognized: '" << cmd << "'"
                    << Style::Reset << "\n"
                    << std::flush;
        }
      }

      if (keep_running) {
        if (auto_steps == -1) {
          run_frame();
        } else if (auto_steps > 0) {
          run_frame();
          --auto_steps;
        } else {
          // Paused, just sleep slightly to not max CPU
          std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
      }
    }

    if (repl_thread.joinable()) {
      repl_thread.join();
    }
  } else {
    for (int frame = 0; frame < opts.frames_to_run; ++frame) {
      run_frame();
    }
  }

  if (!opts.export_ui_snapshot_path.empty()) {
    std::string error;
    const bool saved =
        RogueCity::UIInt::UiIntrospector::Instance().SaveSnapshotJson(
            opts.export_ui_snapshot_path, &error);
    if (!saved) {
      std::fprintf(stderr, "Failed to save UI snapshot to '%s': %s\n",
                   opts.export_ui_snapshot_path.c_str(), error.c_str());
#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
      if (wants_screenshot) {
        ShutdownScreenshotRuntime(screenshot_ctx);
      }
#endif
#ifdef ROGUECITY_HAS_IMPLOT
      ImPlot::DestroyContext();
#endif
#ifdef ROGUECITY_HAS_IMPLOT3D
      ImPlot3D::DestroyContext();
#endif
      ImGui::DestroyContext();
      return 2;
    }
    std::printf("UI snapshot exported: %s\n",
                opts.export_ui_snapshot_path.c_str());
  }

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  if (wants_screenshot) {
    if (!ExportFramebufferPng(opts.export_ui_screenshot_path,
                              screenshot_ctx.window)) {
      std::fprintf(stderr, "Failed to export runtime screenshot to '%s'.\n",
                   opts.export_ui_screenshot_path.c_str());
      ShutdownScreenshotRuntime(screenshot_ctx);
#ifdef ROGUECITY_HAS_IMPLOT
      ImPlot::DestroyContext();
#endif
#ifdef ROGUECITY_HAS_IMPLOT3D
      ImPlot3D::DestroyContext();
#endif
      ImGui::DestroyContext();
      return 4;
    }
    std::printf("UI screenshot exported: %s\n",
                opts.export_ui_screenshot_path.c_str());
  }
#endif

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  if (wants_screenshot) {
    RC::SvgTextureCache::Get().Clear();
    ShutdownScreenshotRuntime(screenshot_ctx);
  }
#endif
  RC_UI::Panels::ImGuiError::Shutdown();
#ifdef ROGUECITY_HAS_IMPLOT
  ImPlot::DestroyContext(); // must precede ImGui::DestroyContext
#endif
#ifdef ROGUECITY_HAS_IMPLOT3D
  ImPlot3D::DestroyContext(); // must precede ImGui::DestroyContext
#endif
  ImGui::DestroyContext();
  return 0;
}
