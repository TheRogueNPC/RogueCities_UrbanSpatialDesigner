#pragma once

#include "ui/api/rc_ui_dataviz.h"
#include "ui/rc_ui_tokens.h"

#include <imgui.h>

namespace RC_UI::API {

struct PanelSpec {
  const char *title = "";
  bool *open = nullptr;
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
  ImU32 border_color = UITokens::YellowWarning;
  ImU32 background_color = UITokens::PanelBackground;
};

bool BeginPanel(const PanelSpec &spec);
void EndPanel();

bool SectionHeader(const char *label, ImU32 bar_color = UITokens::CyanAccent,
                   bool default_open = true, ImTextureID icon = 0,
                   float icon_size = 14.0f);

bool ActionButton(const char *id, const char *label, ButtonFeedback &feedback,
                  float dt, bool active = false,
                  ImVec2 size = ImVec2(0.0f, 0.0f));

void StatusChip(const char *label, ImU32 color, bool emphasized = false);
void Text(ImU32 color, const char *fmt, ...);
void BoundedText(const char *text, float padding = UITokens::SpaceS);
void Meter(const char *label, float value, ImU32 fill_color,
           const char *value_text = nullptr);
void DiagRow(const char *label, const char *value,
             ImU32 value_color = UITokens::TextPrimary);
void DrawScanlineBackdrop(const ImVec2 &min, const ImVec2 &max, float time_sec,
                          ImU32 tint = UITokens::GreenHUD);

// Data Visualization — D3-style, display-only (v1)
// TODO(interactivity): hover/click overloads planned for a future pass.
void BarChart(const RC_UI::DataViz::BarChartSpec &spec);
void Sparkline(const RC_UI::DataViz::SparklineSpec &spec);
void Donut(const RC_UI::DataViz::DonutSpec &spec);

// Force-directed graph — animated, drag-interactive.
// Call StepForceGraph each frame before DrawForceGraph.
void StepForceGraph(RC_UI::DataViz::ForceGraphSpec &spec, int iterations = 3);
void DrawForceGraph(RC_UI::DataViz::ForceGraphSpec &spec);

// Drag-collision simulation — D3 forceCollide, push circles around.
// Call StepCollision each frame before DrawCollision.
void StepCollision(RC_UI::DataViz::CollisionSpec &spec, int ticks = 1);
void DrawCollision(RC_UI::DataViz::CollisionSpec &spec);

// Smooth zoom — phyllotaxis spiral with auto-transitioning camera.
// Call InitSmoothZoom once, then StepSmoothZoom(spec, dt) + DrawSmoothZoom each frame.
void InitSmoothZoom(RC_UI::DataViz::SmoothZoomSpec &spec, float cx, float cy,
                    float step = 12.0f);
void StepSmoothZoom(RC_UI::DataViz::SmoothZoomSpec &spec, float dt);
void DrawSmoothZoom(const RC_UI::DataViz::SmoothZoomSpec &spec);

void TextDisabled(const char *fmt, ...);
void Spacing();
void Separator();
void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);
void BeginDisabled(bool disabled = true);
void EndDisabled();
void SetNextItemWidth(float width);
void Indent(float width = 0.0f);
void Unindent(float width = 0.0f);
bool Button(const char *label, const ImVec2 &size = ImVec2(0.0f, 0.0f));
bool SmallButton(const char *label);
bool InvisibleButton(const char *str_id, const ImVec2 &size,
                     ImGuiButtonFlags flags = 0);
bool Checkbox(const char *label, bool *v);
bool RadioButton(const char *label, bool active);
bool Selectable(const char *label, bool selected = false,
                ImGuiSelectableFlags flags = 0,
                const ImVec2 &size = ImVec2(0.0f, 0.0f));
bool Selectable(const char *label, bool *p_selected,
                ImGuiSelectableFlags flags = 0,
                const ImVec2 &size = ImVec2(0.0f, 0.0f));
bool InputText(const char *label, char *buf, size_t buf_size,
               ImGuiInputTextFlags flags = 0,
               ImGuiInputTextCallback callback = nullptr,
               void *user_data = nullptr);
bool InputTextMultiline(const char *label, char *buf, size_t buf_size,
                        const ImVec2 &size = ImVec2(0.0f, 0.0f),
                        ImGuiInputTextFlags flags = 0,
                        ImGuiInputTextCallback callback = nullptr,
                        void *user_data = nullptr);
bool InputInt(const char *label, int *v, int step = 1, int step_fast = 100,
              ImGuiInputTextFlags flags = 0);
bool InputFloat(const char *label, float *v, float step = 0.0f,
                float step_fast = 0.0f, const char *format = "%.3f",
                ImGuiInputTextFlags flags = 0);
bool InputFloat2(const char *label, float v[2], const char *format = "%.3f",
                 ImGuiInputTextFlags flags = 0);
bool InputScalar(const char *label, ImGuiDataType data_type, void *p_data,
                 const void *p_step = nullptr, const void *p_step_fast = nullptr,
                 const char *format = nullptr,
                 ImGuiInputTextFlags flags = 0);
bool DragFloat(const char *label, float *v, float v_speed = 1.0f,
               float v_min = 0.0f, float v_max = 0.0f,
               const char *format = "%.3f",
               ImGuiSliderFlags flags = ImGuiSliderFlags_None);
bool DragInt(const char *label, int *v, float v_speed = 1.0f, int v_min = 0,
             int v_max = 0, const char *format = "%d",
             ImGuiSliderFlags flags = ImGuiSliderFlags_None);
bool SliderFloat(const char *label, float *v, float v_min, float v_max,
                 const char *format = "%.3f",
                 ImGuiSliderFlags flags = ImGuiSliderFlags_None);
bool SliderInt(const char *label, int *v, int v_min, int v_max,
               const char *format = "%d",
               ImGuiSliderFlags flags = ImGuiSliderFlags_None);
bool SliderScalar(const char *label, ImGuiDataType data_type, void *p_data,
                  const void *p_min, const void *p_max,
                  const char *format = nullptr,
                  ImGuiSliderFlags flags = ImGuiSliderFlags_None);
bool SliderAngle(const char *label, float *v_rad, float v_degrees_min = -360.0f,
                 float v_degrees_max = 360.0f, const char *format = "%.0f deg",
                 ImGuiSliderFlags flags = 0);
bool Combo(const char *label, int *current_item, const char *const items[],
           int items_count, int popup_max_height_in_items = -1);
bool BeginCombo(const char *label, const char *preview_value,
                ImGuiComboFlags flags = 0);
void EndCombo();
bool CollapsingHeader(const char *label,
                      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None);
bool CollapsingHeader(const char *label, bool *p_visible,
                      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None);
bool MenuItem(const char *label, const char *shortcut = nullptr,
              bool selected = false, bool enabled = true);
bool MenuItem(const char *label, const char *shortcut, bool *p_selected,
              bool enabled = true);
void OpenPopup(const char *str_id, ImGuiPopupFlags popup_flags = 0);
bool BeginPopup(const char *str_id, ImGuiWindowFlags flags = 0);
bool BeginPopupModal(const char *name, bool *p_open = nullptr,
                     ImGuiWindowFlags flags = 0);
void EndPopup();
void CloseCurrentPopup();

class ScopedID {
public:
  explicit ScopedID(int id);
  explicit ScopedID(const void *id);
  explicit ScopedID(const char *id);

  ScopedID(const ScopedID &) = delete;
  ScopedID &operator=(const ScopedID &) = delete;

  ScopedID(ScopedID &&other) noexcept;
  ScopedID &operator=(ScopedID &&other) noexcept;

  ~ScopedID();

private:
  bool active_ = true;
};

namespace Mutate {

ImDrawList *WindowDrawList();
ImDrawList *ForegroundDrawList();
ImDrawList *BackgroundDrawList();

void ClaimLayoutSpace(const ImVec2 &size);
bool BeginChild(const char *id, const ImVec2 &size = ImVec2(0.0f, 0.0f),
                ImGuiChildFlags child_flags = 0,
                ImGuiWindowFlags window_flags = 0);
void EndChild();

} // namespace Mutate

} // namespace RC_UI::API

// Global alias for migration ergonomics in legacy namespaces.
namespace API = RC_UI::API;
