#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

// UI system includes
#include "RogueCity/App/UI/ThemeManager.h"   // Multi-theme system
#include "ui/panels/rc_panel_axiom_editor.h" // AxiomEditor::Undo/Redo
#include "ui/rc_ui_root.h"

// AI system includes
#include "config/AiConfig.h"

// clang-format off
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
// clang-format on

// Header-only input adapter (Win32 polling) //todo ensure we are not hardcoding
// platform-specific input handling and that this can be easily extended or
// replaced with a more robust solution in the future (e.g. event-driven,
// multi-platform support).
#if __has_include(<LInput.h>)
#include <LInput.h>
#define ROGUECITY_HAS_LINPUT 1
#endif

// OpenGL loader - must be included before GLFW
#include <GL/gl3w.h>

#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdio.h>
#include <string>
#include <thread>
#include <utility>

#include <conio.h>
#include <deque>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>

#include "ui/introspection/UiIntrospection.h"

using RogueCity::Core::Editor::EditorEvent;
using RogueCity::Core::Editor::EditorHFSM;
using RogueCity::Core::Editor::EditorState;
using RogueCity::Core::Editor::GlobalState;
// Note: This example code is using the GL3W OpenGL loader. You may use any
// other loader (e.g. glad, glew, etc.) by changing the include and
// initialization code. If you use a different loader, be sure to initialize it
// before calling any OpenGL functions. For example, with glad you would do:
// #include <glad/glad.h>
// if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
//     fprintf(stderr, "Failed to initialize OpenGL loader!\n");
//     return 1;
// }
// The rest of the code should work regardless of the OpenGL loader you choose,
// as long as it is properly initialized before use.

// todo: refactor this main function to delegate responsibilities to a dedicated
// Application class that manages the lifecycle, state, and main loop of the
// application. This will improve modularity, testability, and maintainability
// by encapsulating application logic and allowing for cleaner separation of
// concerns. The Application class can handle initialization, event processing,
// state updates, and rendering, while the main function simply creates an
// instance of the Application and starts it. todo consider unifying the main
// loop with a more robust game loop structure that includes fixed timestep
// updates for simulation and variable timestep rendering, to ensure consistent
// behavior regardless of frame rate fluctuations. This would involve
// implementing a time accumulator and separating the update and render phases
// more cleanly, allowing for smoother simulations and better handling of edge
// cases like slow frames or pauses. todo  look into unifying  Glew vs Glad vs
// GLew with a single abstraction layer for OpenGL loading, to allow for easier
// switching between loaders and better modularity. This could involve creating
// a simple wrapper that abstracts the initialization and function loading
// process, allowing the underlying loader to be swapped out with minimal
// changes to the rest of the codebase. This would also help with testing and
// platform compatibility in the future.
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
  char *value = nullptr;
  size_t value_len = 0;
  if (_dupenv_s(&value, &value_len, "ROGUE_ENABLE_IMGUI_VIEWPORTS") != 0 ||
      value == nullptr) {
    return false;
  }
  const bool enabled = std::strcmp(value, "1") == 0 ||
                       std::strcmp(value, "true") == 0 ||
                       std::strcmp(value, "TRUE") == 0;
  std::free(value);
  return enabled;
#else
  const char *value = std::getenv("ROGUE_ENABLE_IMGUI_VIEWPORTS");
  if (value == nullptr) {
    return false;
  }
  return std::strcmp(value, "1") == 0 || std::strcmp(value, "true") == 0 ||
         std::strcmp(value, "TRUE") == 0;
#endif
#else
  return false;
#endif
}

[[nodiscard]] std::pair<int, int> ComputeMinimumWindowSize(GLFWwindow *window) {
  GLFWmonitor *monitor = glfwGetWindowMonitor(window);
  if (monitor == nullptr) {
    monitor = glfwGetPrimaryMonitor();
  }

  int display_width = 1280;
  int display_height = 720;
  if (monitor != nullptr) {
    int work_x = 0;
    int work_y = 0;
    int work_width = 0;
    int work_height = 0;
    glfwGetMonitorWorkarea(monitor, &work_x, &work_y, &work_width,
                           &work_height);
    (void)work_x;
    (void)work_y;
    if (work_width > 0 && work_height > 0) {
      display_width = work_width;
      display_height = work_height;
    } else if (const GLFWvidmode *mode = glfwGetVideoMode(monitor);
               mode != nullptr) {
      display_width = mode->width;
      display_height = mode->height;
    }
  }

  // UI contract: keep windows above the legible docking baseline, no panel
  // hiding/squishing.
  constexpr int kUiContractMinWidth = 1100;
  constexpr int kUiContractMinHeight = 700;
  const int contract_width =
      std::min(kUiContractMinWidth, std::max(320, display_width));
  const int contract_height =
      std::min(kUiContractMinHeight, std::max(240, display_height));

  const int min_width = std::max(
      contract_width,
      static_cast<int>(std::lround(static_cast<double>(display_width) * 0.25)));
  const int min_height =
      std::max(contract_height,
               static_cast<int>(
                   std::lround(static_cast<double>(display_height) * 0.25)));
  return {min_width, min_height};
}

[[nodiscard]] int ReadPositiveIntEnv(const char *key, int fallback) {
  std::string value;
#if defined(_WIN32)
  char *raw_value = nullptr;
  size_t raw_len = 0;
  if (_dupenv_s(&raw_value, &raw_len, key) != 0 || raw_value == nullptr) {
    return fallback;
  }
  value.assign(raw_value);
  std::free(raw_value);
#else
  const char *raw_value = std::getenv(key);
  if (raw_value != nullptr) {
    value.assign(raw_value);
  }
#endif
  if (value.empty()) {
    return fallback;
  }
  char *end = nullptr;
  const long parsed = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str() || parsed <= 0L) {
    return fallback;
  }
  return static_cast<int>(std::clamp<long>(parsed, 320L, 16384L));
}

[[nodiscard]] std::pair<int, int> ComputeStartupWindowSize() {
  const int default_width = 1280;
  const int default_height = 1024;
  const int width = ReadPositiveIntEnv("ROGUE_WINDOW_WIDTH", default_width);
  const int height = ReadPositiveIntEnv("ROGUE_WINDOW_HEIGHT", default_height);
  return {width, height};
}

} // namespace

enum class LogCategory { Build, TUI, GUI, AI };

struct LogEntry {
  std::string timestamp;
  LogCategory category;
  std::string level;
  std::string message;
};

class LogRingBuffer {
public:
  static LogRingBuffer &Instance() {
    static LogRingBuffer instance;
    return instance;
  }

  void Push(LogCategory category, const std::string &level,
            const std::string &msg) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now_c));

    LogEntry entry{buf, category, level, msg};
    m_logs.push_back(entry);
    if (m_logs.size() > 1000) {
      m_logs.pop_front();
    }

    nlohmann::json j;
    j["timestamp"] = entry.timestamp;

    const char *cat_str = "GUI";
    if (category == LogCategory::Build)
      cat_str = "BUILD";
    else if (category == LogCategory::TUI)
      cat_str = "TUI";
    else if (category == LogCategory::AI)
      cat_str = "AI";

    j["layer"] = cat_str;
    j["level"] = level;
    j["msg"] = msg;

    if (m_file.is_open()) {
      m_file << j.dump() << "\n";
      m_file.flush();
    }
  }

  std::vector<LogEntry> GetLogs(LogCategory category) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LogEntry> result;
    for (const auto &log : m_logs) {
      if (log.category == category) {
        result.push_back(log);
      }
    }
    return result;
  }

private:
  LogRingBuffer() { m_file.open("visualizer.jsonl", std::ios::app); }
  ~LogRingBuffer() {
    if (m_file.is_open())
      m_file.close();
  }
  std::mutex m_mutex;
  std::deque<LogEntry> m_logs;
  std::ofstream m_file;
};

// --- TUI Cyberpunk Styling & Utilities ---
namespace Style {
constexpr const char *Reset = "\033[0m";
constexpr const char *Red = "\033[38;5;196m";
constexpr const char *Gold = "\033[38;5;220m";
constexpr const char *Orange = "\033[38;5;208m";
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
  std::string line_buf;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) !=
         nullptr) {
    std::string chunk(buffer.data());
    for (char c : chunk) {
      if (c == '\n') {
        LogRingBuffer::Instance().Push(LogCategory::Build, "OUT", line_buf);
        std::cout << line_buf << "\n";
        line_buf.clear();
      } else if (c != '\r') {
        line_buf += c;
      }
    }
  }
  if (!line_buf.empty()) {
    LogRingBuffer::Instance().Push(LogCategory::Build, "OUT", line_buf);
    std::cout << line_buf << "\n";
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

void RunInteractiveLogViewer() {
  LogCategory current_tab = LogCategory::Build;
  bool in_viewer = true;

  auto draw_viewer = [&]() {
    // Clear console
    std::cout << "\x1b[2J\x1b[H";
    std::cout << Style::Gold << " === RC-LOG VIEWER === " << Style::Reset
              << "\n";
    std::cout << Style::DarkGray
              << " Use [Tab] to switch, [Backspace] to exit.\n\n"
              << Style::Reset;

    auto format_tab = [&](LogCategory cat, const std::string &name) {
      if (cat == current_tab)
        return std::string(Style::Orange) + "[ " + name + " ]" + Style::Reset;
      return std::string(Style::DarkGray) + "  " + name + "  " + Style::Reset;
    };

    std::cout << format_tab(LogCategory::Build, "BUILD") << " | "
              << format_tab(LogCategory::TUI, "TUI") << " | "
              << format_tab(LogCategory::GUI, "GUI") << " | "
              << format_tab(LogCategory::AI, "AI") << "\n";
    std::cout << Style::DarkGray << std::string(50, '-') << Style::Reset
              << "\n";

    auto logs = LogRingBuffer::Instance().GetLogs(current_tab);
    // Show last 20 logs
    size_t start_idx = logs.size() > 20 ? logs.size() - 20 : 0;
    for (size_t i = start_idx; i < logs.size(); ++i) {
      const auto &log = logs[i];
      std::cout << Style::DarkGray << "[" << log.timestamp << "] "
                << Style::Orange << "[" << log.level << "] " << Style::Reset
                << log.message << "\n";
    }
    std::cout << std::flush;
  };

  draw_viewer();

  while (in_viewer) {
    if (_kbhit()) {
      int ch = _getch();
      if (ch == '\t') { // Tab
        int next = (static_cast<int>(current_tab) + 1) % 4;
        current_tab = static_cast<LogCategory>(next);
        draw_viewer();
      } else if (ch == '\b' || ch == 8 || ch == 127) { // Backspace
        in_viewer = false;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  std::cout << "\x1b[2J\x1b[H";
  PrintBanner();
}

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

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char **) {
  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

  // Decide GL+GLSL versions
#if __APPLE__
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

  // Keep OS decorations for now (custom chrome is complex)
  // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);  // Disabled for easier window
  // management

  const auto [startup_width, startup_height] = ComputeStartupWindowSize();
  GLFWwindow *window =
      glfwCreateWindow(startup_width, startup_height, "RC-USD", NULL, NULL);
  if (window == NULL)
    return 1;
  glfwMakeContextCurrent(window);
  const auto [min_window_width, min_window_height] =
      ComputeMinimumWindowSize(window);
  glfwSetWindowSizeLimits(window, min_window_width, min_window_height,
                          GLFW_DONT_CARE, GLFW_DONT_CARE);
  glfwSwapInterval(1);

  // Initialize OpenGL loader (gl3w)
  if (gl3wInit()) {
    fprintf(stderr, "Failed to initialize OpenGL loader (gl3w)\n");
    return 1;
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  auto &gs_early = RogueCity::Core::Editor::GetGlobalState();

  // Enable docking in all GUI builds.
  io.ConfigFlags |= DockingConfigFlag();
  io.ConfigDpiScaleFonts = gs_early.config.ui_dpi_scale_fonts_enabled;
  io.ConfigDpiScaleViewports = gs_early.config.ui_dpi_scale_viewports_enabled;

  // Multi-viewport remains opt-in (config or explicit env override).
  const bool enable_platform_viewports =
      gs_early.config.ui_multi_viewport_enabled ||
      ShouldEnablePlatformViewports();
  if (enable_platform_viewports) {
    io.ConfigFlags |= ViewportsConfigFlag();
  } else {
    io.ConfigFlags &= ~ViewportsConfigFlag();
  }

  // When viewports are enabled, tweak WindowRounding/WindowBg for platform
  // windows
  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ViewportsConfigFlag()) {
    style.WindowRounding = 0.0f; // Y2K hard edges
    style.Colors[ImGuiCol_WindowBg].w =
        1.0f; // Opaque backgrounds for OS windows
  }

  // Initialize Theme Manager and apply saved theme
  auto &theme_mgr = RogueCity::UI::ThemeManager::Instance();
  if (!gs_early.config.active_theme.empty()) {
    theme_mgr.LoadTheme(gs_early.config.active_theme);
  } else {
    // Rogue theme on first launch
    theme_mgr.LoadTheme("Rogue");
    gs_early.config.active_theme = "Rogue";
  }

  // Load AI configuration
  RogueCity::AI::AiConfigManager::Instance().LoadFromFile("AI/ai_config.json");

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Our state
  ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

  auto &hfsm = RogueCity::Core::Editor::GetEditorHFSM();
  auto &gs = RogueCity::Core::Editor::GetGlobalState();

  hfsm.handle_event(EditorEvent::BootComplete, gs);
  hfsm.handle_event(EditorEvent::NewProject, gs);
  hfsm.handle_event(EditorEvent::ProjectLoaded, gs);
  hfsm.handle_event(EditorEvent::Tool_Roads, gs);

#if defined(ROGUECITY_HAS_LINPUT)
  // LInput init
  LInput::Input input;
  input.Init();
  // Disable legacy input injection by default while diagnosing capture
  // conflicts
  static bool g_disable_linput_injection = true;
#endif

  // Legacy key array feed removed: this imgui build does not expose
  // `io.KeysDown`. Modifier state is forwarded through
  // io.AddKeyEvent(ImGuiMod_*).

  PrintBanner();
  CommandQueue cmd_queue;
  std::atomic<bool> keep_running{true};

  std::thread repl_thread([&]() {
    std::string line;
    while (keep_running) {
      std::cout << "\n" << Style::Red << "rc> " << Style::Reset << std::flush;
      if (!std::getline(std::cin, line)) {
        // EOF or no console attached
        break;
      }
      if (line.empty())
        continue;

      if (line == "log" || line == "rc-log") {
        LogRingBuffer::Instance().Push(LogCategory::TUI, "CMD",
                                       "Entered log viewer");
        RunInteractiveLogViewer();
        continue;
      }

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
        LogRingBuffer::Instance().Push(LogCategory::AI, "QUERY", prompt);
        Subprocess::ExecuteDevShell("rc-ai-query -Prompt '" + prompt + "'");
      } else if (line == "doctor") {
        std::cout << Style::Orange << "[doctor] running diagnostics...\n"
                  << Style::Reset;
        Subprocess::ExecuteDevShell("rc-doctor");
      } else if (line == "problems") {
        Subprocess::ExecuteDevShell("rc-problems");
      } else if (line == "build") {
        std::cout << Style::Orange << "[build] recompiling gui target...\n"
                  << Style::Reset;
        Subprocess::ExecuteDevShell("rc-bld");
      } else if (line == "help" || line == "?") {
        std::cout
            << Style::Gold << "Available REPL / Shell commands:\n"
            << Style::Red << "  -- DEV SHELL SUBPROCESS --\n"
            << Style::Gold
            << "  ! <cmd> / shell <cmd> - Run arbitrary shell command\n"
            << "  build                 - Rebuild gui executable\n"
            << "  doctor                - Run environment diagnostics\n"
            << "  problems              - Fetch latest compilation issues\n"
            << "  ai_start              - Start local AI proxy server\n"
            << "  ai_query <prompt>     - Query local AI model\n\n"
            << Style::Red << "  -- UI ENGINE CONTEXT --\n"
            << Style::Gold
            << "  rc-log / log          - Open interactive TUI Dashboard "
               "Viewer (Tabs)\n"
            << "  dump_ui               - Dump ImGui introspection JSON\n"
            << "  state                 - Print Global State summary\n"
            << "  hfsm <event>          - Dispatch EditorEvent (GotoIdle, "
               "Tool_Roads, etc)\n"
            << "  quit / exit           - Terminate application\n"
            << Style::Reset << std::flush;
      } else {
        cmd_queue.Push(line);
        std::this_thread::sleep_for(
            std::chrono::milliseconds(20)); // wait for main thread print
      }

      if (line == "quit" || line == "exit") {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
      }
    }
  });

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      continue;
    }

    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    if (framebuffer_width <= 1 || framebuffer_height <= 1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      continue;
    }

#if defined(ROGUECITY_HAS_LINPUT)
    // Poll platform input and (optionally) feed ImGui IO
    input.Update();
    if (!g_disable_linput_injection) {
      io.AddMousePosEvent(static_cast<float>(input.Mouse.X()),
                          static_cast<float>(input.Mouse.Y()));
      io.AddMouseButtonEvent(ImGuiMouseButton_Left,
                             input.Mouse.LeftButtonDown());
      io.AddMouseButtonEvent(ImGuiMouseButton_Right,
                             input.Mouse.RightButtonDown());
      io.AddMouseButtonEvent(ImGuiMouseButton_Middle,
                             input.Mouse.MiddleButtonDown());
      io.AddKeyEvent(ImGuiMod_Ctrl, input.Keyboard.CtrlDown());
      io.AddKeyEvent(ImGuiMod_Shift, input.Keyboard.ShiftDown());
      io.AddKeyEvent(ImGuiMod_Alt, input.Keyboard.AltDown());
    }

// Legacy key array feed removed: this imgui build does not expose
// `io.KeysDown`. Modifier state is forwarded through
// io.AddKeyEvent(ImGuiMod_*).
#endif

    std::string cmd;
    while (cmd_queue.TryPop(cmd)) {
      if (cmd == "quit" || cmd == "exit") {
        std::cout << Style::Gold << "Shutting down the GUI from console..."
                  << Style::Reset << "\n"
                  << std::flush;
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      } else if (cmd == "dump_ui") {
        std::cout << Style::Orange << "[dump_ui] " << Style::Reset
                  << "Capturing Inspector State...\n";
        std::cout
            << Style::DarkGray
            << RogueCity::UIInt::UiIntrospector::Instance().SnapshotJson().dump(
                   2)
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
      } else {
        std::cout << Style::Red << "Command not recognized: '" << cmd << "'"
                  << Style::Reset << "\n"
                  << std::flush;
      }
    }

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
    }
    // Soft reset (Ctrl+Shift+L): rebuild dock tree but keep persisted window
    // settings.
    else if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_L)) {
      RC_UI::ResetDockLayout();
    }
    // Redo (Ctrl+R): note viewport handles Ctrl+Z for undo, Ctrl+Y for redo as
    // well
    else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R)) {
      RC_UI::Panels::AxiomEditor::Redo();
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
    if (io.ConfigFlags & ViewportsConfigFlag()) {
      GLFWwindow *backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }
#endif

    glfwSwapBuffers(window);
  }

  // Cleanup
  keep_running = false;
  repl_thread.detach();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  // On Windows, if the detached REPL thread is still blocked on
  // std::getline(std::cin), normal exit will cause the CRT to teardown static
  // I/O buffers and crash the thread, returning exit code 1. We use _Exit to
  // immediately terminate and avoid this race.
  std::_Exit(0);
}
