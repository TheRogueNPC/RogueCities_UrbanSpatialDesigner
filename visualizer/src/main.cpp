// namespace main.cpp: Entry point for the visualizer, demonstrating integration of the EditorHFSM with a simple ImGui interface.
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
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
  std::string export_ui_snapshot_path;
  std::string export_ui_screenshot_path;
};

[[nodiscard]] RuntimeOptions ParseOptions(int argc, char** argv) {
  RuntimeOptions opts{};
  for (int i = 1; i < argc; ++i) {
    const char* arg = argv[i];
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

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
struct ScreenshotRuntimeContext {
  GLFWwindow* window = nullptr;
  bool active = false;
  bool imgui_backends_active = false;
};

void glfw_error_callback(int error, const char* description) {
  std::fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool InitScreenshotRuntime(ScreenshotRuntimeContext& ctx, int width, int height) {
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

  ctx.window = glfwCreateWindow(width, height, "RogueCityHeadless", nullptr, nullptr);
  if (ctx.window == nullptr) {
    std::fprintf(stderr, "Failed to create hidden OpenGL window for screenshot export.\n");
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

bool InitScreenshotImGuiBackends(ScreenshotRuntimeContext& ctx) {
  if (!ctx.active || ctx.window == nullptr) {
    return false;
  }
  if (!ImGui_ImplGlfw_InitForOpenGL(ctx.window, false)) {
    std::fprintf(stderr, "Failed to initialize ImGui GLFW backend for screenshot export.\n");
    return false;
  }
  if (!ImGui_ImplOpenGL3_Init("#version 130")) {
    std::fprintf(stderr, "Failed to initialize ImGui OpenGL backend for screenshot export.\n");
    ImGui_ImplGlfw_Shutdown();
    return false;
  }
  ctx.imgui_backends_active = true;
  return true;
}

void ShutdownScreenshotRuntime(ScreenshotRuntimeContext& ctx) {
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

bool ExportFramebufferPng(const std::string& path, GLFWwindow* window) {
  if (window == nullptr || path.empty()) {
    return false;
  }

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  if (width <= 0 || height <= 0) {
    std::fprintf(stderr, "Invalid framebuffer size for screenshot export (%d x %d).\n", width, height);
    return false;
  }

  std::vector<unsigned char> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u, 0u);
  std::vector<unsigned char> flipped(pixels.size(), 0u);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadBuffer(GL_BACK);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  const size_t row_bytes = static_cast<size_t>(width) * 4u;
  for (int y = 0; y < height; ++y) {
    const size_t src_offset = static_cast<size_t>(y) * row_bytes;
    const size_t dst_offset = static_cast<size_t>(height - 1 - y) * row_bytes;
    std::memcpy(flipped.data() + dst_offset, pixels.data() + src_offset, row_bytes);
  }

  std::filesystem::path screenshot_path(path);
  if (screenshot_path.has_parent_path()) {
    std::error_code ec;
    std::filesystem::create_directories(screenshot_path.parent_path(), ec);
  }

  const int ok = stbi_write_png(path.c_str(), width, height, 4, flipped.data(), width * 4);
  if (ok == 0) {
    std::fprintf(stderr, "stbi_write_png failed for '%s'.\n", path.c_str());
    return false;
  }
  return true;
}
#endif

void draw_main_menu(EditorHFSM& hfsm, GlobalState& gs) {
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

void draw_editor_windows(EditorHFSM& hfsm, GlobalState& gs) {
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

void apply_theme_once() {
  static bool theme_applied = false;
  if (!theme_applied) {
    RC_UI::ApplyTheme();
    theme_applied = true;
  }
}

} // namespace

int main(int argc, char** argv) {
  const RuntimeOptions opts = ParseOptions(argc, argv);

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--help") == 0) {
      std::printf("RogueCityVisualizerHeadless options:\n");
      std::printf("  --frames <N>                   Number of headless frames (default: 3)\n");
      std::printf("  --export-ui-snapshot <path>    Write UI introspection JSON snapshot\n");
      std::printf("  --export-ui-screenshot <path>  Write runtime screenshot PNG\n");
      return 0;
    }
  }

  const bool wants_screenshot = !opts.export_ui_screenshot_path.empty();

#if !defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  if (wants_screenshot) {
    std::fprintf(stderr,
                 "Screenshot export requested but this build has no OpenGL screenshot runtime.\n");
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

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  if (wants_screenshot && !InitScreenshotImGuiBackends(screenshot_ctx)) {
    ImGui::DestroyContext();
    ShutdownScreenshotRuntime(screenshot_ctx);
    return 3;
  }
#endif

  auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
  auto& gs = RogueCity::Core::Editor::GetGlobalState();

  hfsm.handle_event(EditorEvent::BootComplete, gs);
  hfsm.handle_event(EditorEvent::NewProject, gs);
  hfsm.handle_event(EditorEvent::ProjectLoaded, gs);
  hfsm.handle_event(EditorEvent::Tool_Roads, gs);

  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(1280.0f, 720.0f);
  io.ConfigFlags |= DockingConfigFlag();
  io.Fonts->Build();
  unsigned char* font_pixels = nullptr;
  int font_width = 0;
  int font_height = 0;
  io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  if (wants_screenshot) {
    int fb_w = 0;
    int fb_h = 0;
    glfwGetFramebufferSize(screenshot_ctx.window, &fb_w, &fb_h);
    if (fb_w > 0 && fb_h > 0) {
      io.DisplaySize = ImVec2(static_cast<float>(fb_w), static_cast<float>(fb_h));
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
        io.DisplaySize = ImVec2(static_cast<float>(fb_w), static_cast<float>(fb_h));
      }
    }
#endif
    io.DeltaTime = 1.0f / 60.0f;
    ImGui::NewFrame();

    apply_theme_once();
    RC_UI::DrawRoot(io.DeltaTime);

    hfsm.update(gs, io.DeltaTime);
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

  for (int frame = 0; frame < opts.frames_to_run; ++frame) {
    run_frame();
  }

  if (!opts.export_ui_snapshot_path.empty()) {
    std::string error;
    const bool saved = RogueCity::UIInt::UiIntrospector::Instance().SaveSnapshotJson(
        opts.export_ui_snapshot_path, &error);
    if (!saved) {
      std::fprintf(stderr, "Failed to save UI snapshot to '%s': %s\n",
                   opts.export_ui_snapshot_path.c_str(), error.c_str());
#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
      if (wants_screenshot) {
        ShutdownScreenshotRuntime(screenshot_ctx);
      }
#endif
      ImGui::DestroyContext();
      return 2;
    }
    std::printf("UI snapshot exported: %s\n", opts.export_ui_snapshot_path.c_str());
  }

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  if (wants_screenshot) {
    if (!ExportFramebufferPng(opts.export_ui_screenshot_path, screenshot_ctx.window)) {
      std::fprintf(stderr, "Failed to export runtime screenshot to '%s'.\n",
                   opts.export_ui_screenshot_path.c_str());
      ShutdownScreenshotRuntime(screenshot_ctx);
      ImGui::DestroyContext();
      return 4;
    }
    std::printf("UI screenshot exported: %s\n", opts.export_ui_screenshot_path.c_str());
  }
#endif

#if defined(ROGUECITY_HEADLESS_GL_SCREENSHOT)
  if (wants_screenshot) {
    ShutdownScreenshotRuntime(screenshot_ctx);
  }
#endif
  ImGui::DestroyContext();
  return 0;
}
