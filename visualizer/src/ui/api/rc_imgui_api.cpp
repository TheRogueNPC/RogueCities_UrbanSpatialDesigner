#include "ui/api/rc_imgui_api.h"

#include "ui/api/rc_ui_dataviz.h"
#include "ui/rc_ui_components.h"

#include <cstdarg>
#include <utility>

namespace RC_UI::API {

bool BeginPanel(const PanelSpec &spec) {
  return Components::BeginTokenPanel(spec.title, spec.border_color, spec.open,
                                     spec.flags, spec.background_color);
}

void EndPanel() { Components::EndTokenPanel(); }

bool SectionHeader(const char *label, ImU32 bar_color, bool default_open,
                   ImTextureID icon, float icon_size) {
  return Components::DrawSectionHeader(label, bar_color, default_open, icon,
                                       icon_size);
}

bool ActionButton(const char *id, const char *label, ButtonFeedback &feedback,
                  float dt, bool active, ImVec2 size) {
  return Components::AnimatedActionButton(id, label, feedback, dt, active,
                                          size);
}

void StatusChip(const char *label, ImU32 color, bool emphasized) {
  Components::StatusChip(label, color, emphasized);
}

void Text(ImU32 color, const char *fmt, ...) {
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
  va_list args;
  va_start(args, fmt);
  ImGui::TextV(fmt, args);
  va_end(args);
  ImGui::PopStyleColor();
}

void BoundedText(const char *text, float padding) {
  Components::BoundedText(text, padding);
}

void Meter(const char *label, float value, ImU32 fill_color,
           const char *value_text) {
  Components::DrawMeter(label, value, fill_color, value_text);
}

void DiagRow(const char *label, const char *value, ImU32 value_color) {
  Components::DrawDiagRow(label, value, value_color);
}

void DrawScanlineBackdrop(const ImVec2 &min, const ImVec2 &max, float time_sec,
                          ImU32 tint) {
  Components::DrawScanlineBackdrop(min, max, time_sec, tint);
}

void BarChart(const DataViz::BarChartSpec &spec) { DataViz::DrawBarChart(spec); }

void Sparkline(const DataViz::SparklineSpec &spec) { DataViz::DrawSparkline(spec); }

void Donut(const DataViz::DonutSpec &spec) { DataViz::DrawDonut(spec); }

void StepForceGraph(DataViz::ForceGraphSpec &spec, int iterations) {
  DataViz::StepForceGraph(spec, iterations);
}

void DrawForceGraph(DataViz::ForceGraphSpec &spec) {
  DataViz::DrawForceGraph(spec);
}

void StepCollision(DataViz::CollisionSpec &spec, int ticks) {
  DataViz::StepCollision(spec, ticks);
}

void DrawCollision(DataViz::CollisionSpec &spec) {
  DataViz::DrawCollision(spec);
}

void InitSmoothZoom(DataViz::SmoothZoomSpec &spec, float cx, float cy,
                    float step) {
  DataViz::InitSmoothZoom(spec, cx, cy, step);
}

void StepSmoothZoom(DataViz::SmoothZoomSpec &spec, float dt) {
  DataViz::StepSmoothZoom(spec, dt);
}

void DrawSmoothZoom(const DataViz::SmoothZoomSpec &spec) {
  DataViz::DrawSmoothZoom(spec);
}

float EaseValue(DataViz::EasingType type, float t,
                const DataViz::EasingParams &params) {
  return DataViz::EvaluateEasing(type, t, params);
}

void EasingCurve(const DataViz::EasingCurveSpec &spec) {
  DataViz::DrawEasingCurve(spec);
}

void EasingMotion(const DataViz::EasingMotionSpec &spec) {
  DataViz::DrawEasingMotion(spec);
}

void TextDisabled(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  ImGui::TextDisabledV(fmt, args);
  va_end(args);
}

void Spacing() { ImGui::Spacing(); }

void Separator() { ImGui::Separator(); }

void SameLine(float offset_from_start_x, float spacing) {
  ImGui::SameLine(offset_from_start_x, spacing);
}

void BeginDisabled(bool disabled) { ImGui::BeginDisabled(disabled); }

void EndDisabled() { ImGui::EndDisabled(); }

void SetNextItemWidth(float width) { ImGui::SetNextItemWidth(width); }

void Indent(float width) { ImGui::Indent(width); }

void Unindent(float width) { ImGui::Unindent(width); }

bool Button(const char *label, const ImVec2 &size) {
  return ImGui::Button(label, size);
}

bool SmallButton(const char *label) { return ImGui::SmallButton(label); }

bool InvisibleButton(const char *str_id, const ImVec2 &size,
                     ImGuiButtonFlags flags) {
  return ImGui::InvisibleButton(str_id, size, flags);
}

bool Checkbox(const char *label, bool *v) { return ImGui::Checkbox(label, v); }

bool RadioButton(const char *label, bool active) {
  return ImGui::RadioButton(label, active);
}

bool Selectable(const char *label, bool selected, ImGuiSelectableFlags flags,
                const ImVec2 &size) {
  return ImGui::Selectable(label, selected, flags, size);
}

bool Selectable(const char *label, bool *p_selected,
                ImGuiSelectableFlags flags, const ImVec2 &size) {
  return ImGui::Selectable(label, p_selected, flags, size);
}

bool InputText(const char *label, char *buf, size_t buf_size,
               ImGuiInputTextFlags flags, ImGuiInputTextCallback callback,
               void *user_data) {
  return ImGui::InputText(label, buf, buf_size, flags, callback, user_data);
}

bool InputTextMultiline(const char *label, char *buf, size_t buf_size,
                        const ImVec2 &size, ImGuiInputTextFlags flags,
                        ImGuiInputTextCallback callback, void *user_data) {
  return ImGui::InputTextMultiline(label, buf, buf_size, size, flags, callback,
                                   user_data);
}

bool InputInt(const char *label, int *v, int step, int step_fast,
              ImGuiInputTextFlags flags) {
  return ImGui::InputInt(label, v, step, step_fast, flags);
}

bool InputFloat(const char *label, float *v, float step, float step_fast,
                const char *format, ImGuiInputTextFlags flags) {
  return ImGui::InputFloat(label, v, step, step_fast, format, flags);
}

bool InputFloat2(const char *label, float v[2], const char *format,
                 ImGuiInputTextFlags flags) {
  return ImGui::InputFloat2(label, v, format, flags);
}

bool InputScalar(const char *label, ImGuiDataType data_type, void *p_data,
                 const void *p_step, const void *p_step_fast,
                 const char *format, ImGuiInputTextFlags flags) {
  return ImGui::InputScalar(label, data_type, p_data, p_step, p_step_fast,
                            format, flags);
}

bool DragFloat(const char *label, float *v, float v_speed, float v_min,
               float v_max, const char *format, ImGuiSliderFlags flags) {
  return ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, flags);
}

bool DragInt(const char *label, int *v, float v_speed, int v_min, int v_max,
             const char *format, ImGuiSliderFlags flags) {
  return ImGui::DragInt(label, v, v_speed, v_min, v_max, format, flags);
}

bool SliderFloat(const char *label, float *v, float v_min, float v_max,
                 const char *format, ImGuiSliderFlags flags) {
  return ImGui::SliderFloat(label, v, v_min, v_max, format, flags);
}

bool SliderInt(const char *label, int *v, int v_min, int v_max,
               const char *format, ImGuiSliderFlags flags) {
  return ImGui::SliderInt(label, v, v_min, v_max, format, flags);
}

bool SliderScalar(const char *label, ImGuiDataType data_type, void *p_data,
                  const void *p_min, const void *p_max, const char *format,
                  ImGuiSliderFlags flags) {
  return ImGui::SliderScalar(label, data_type, p_data, p_min, p_max, format,
                             flags);
}

bool SliderAngle(const char *label, float *v_rad, float v_degrees_min,
                 float v_degrees_max, const char *format,
                 ImGuiSliderFlags flags) {
  return ImGui::SliderAngle(label, v_rad, v_degrees_min, v_degrees_max, format,
                            flags);
}

bool Combo(const char *label, int *current_item, const char *const items[],
           int items_count, int popup_max_height_in_items) {
  return ImGui::Combo(label, current_item, items, items_count,
                      popup_max_height_in_items);
}

bool BeginCombo(const char *label, const char *preview_value,
                ImGuiComboFlags flags) {
  return ImGui::BeginCombo(label, preview_value, flags);
}

void EndCombo() { ImGui::EndCombo(); }

bool CollapsingHeader(const char *label, ImGuiTreeNodeFlags flags) {
  return ImGui::CollapsingHeader(label, flags);
}

bool CollapsingHeader(const char *label, bool *p_visible,
                      ImGuiTreeNodeFlags flags) {
  return ImGui::CollapsingHeader(label, p_visible, flags);
}

bool MenuItem(const char *label, const char *shortcut, bool selected,
              bool enabled) {
  return ImGui::MenuItem(label, shortcut, selected, enabled);
}

bool MenuItem(const char *label, const char *shortcut, bool *p_selected,
              bool enabled) {
  return ImGui::MenuItem(label, shortcut, p_selected, enabled);
}

void OpenPopup(const char *str_id, ImGuiPopupFlags popup_flags) {
  ImGui::OpenPopup(str_id, popup_flags);
}

bool BeginPopup(const char *str_id, ImGuiWindowFlags flags) {
  return ImGui::BeginPopup(str_id, flags);
}

bool BeginPopupModal(const char *name, bool *p_open, ImGuiWindowFlags flags) {
  return ImGui::BeginPopupModal(name, p_open, flags);
}

void EndPopup() { ImGui::EndPopup(); }

void CloseCurrentPopup() { ImGui::CloseCurrentPopup(); }

ScopedID::ScopedID(int id) { ImGui::PushID(id); }

ScopedID::ScopedID(const void *id) { ImGui::PushID(id); }

ScopedID::ScopedID(const char *id) { ImGui::PushID(id); }

ScopedID::ScopedID(ScopedID &&other) noexcept
    : active_(std::exchange(other.active_, false)) {}

ScopedID &ScopedID::operator=(ScopedID &&other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (active_) {
    ImGui::PopID();
  }
  active_ = std::exchange(other.active_, false);
  return *this;
}

ScopedID::~ScopedID() {
  if (active_) {
    ImGui::PopID();
  }
}

namespace Mutate {

ImDrawList *WindowDrawList() { return ImGui::GetWindowDrawList(); }

ImDrawList *ForegroundDrawList() { return ImGui::GetForegroundDrawList(); }

ImDrawList *BackgroundDrawList() { return ImGui::GetBackgroundDrawList(); }

void ClaimLayoutSpace(const ImVec2 &size) { ImGui::Dummy(size); }

bool BeginChild(const char *id, const ImVec2 &size, ImGuiChildFlags child_flags,
                ImGuiWindowFlags window_flags) {
  return ImGui::BeginChild(id, size, child_flags, window_flags);
}

void EndChild() { ImGui::EndChild(); }

} // namespace Mutate

#ifdef ROGUECITY_HAS_IMPLOT
#include <implot.h>

void InitImPlot()     { ImPlot::CreateContext(); }
void ShutdownImPlot() { ImPlot::DestroyContext(); }

void TimeSeries(const DataViz::ImPlotTimeSeriesSpec& spec) {
    DataViz::DrawImPlotTimeSeries(spec);
}
void Scatter(const DataViz::ImPlotScatterSpec& spec) {
    DataViz::DrawImPlotScatter(spec);
}
void Heatmap(const DataViz::ImPlotHeatmapSpec& spec) {
    DataViz::DrawImPlotHeatmap(spec);
}
void Histogram(const DataViz::ImPlotHistogramSpec& spec) {
    DataViz::DrawImPlotHistogram(spec);
}
#endif // ROGUECITY_HAS_IMPLOT

#ifdef ROGUECITY_HAS_IMPLOT3D
// implot3d.h is included via rc_ui_dataviz.h at global scope; ImPlot3D namespace is visible here.
void InitImPlot3D()     { ImPlot3D::CreateContext(); }
void ShutdownImPlot3D() { ImPlot3D::DestroyContext(); }
void Scatter3D(const DataViz::ImPlot3DScatterSpec& spec)  { DataViz::DrawImPlot3DScatter(spec); }
void Line3D(const DataViz::ImPlot3DLineSpec& spec)         { DataViz::DrawImPlot3DLine(spec); }
void Surface3D(const DataViz::ImPlot3DSurfaceSpec& spec)   { DataViz::DrawImPlot3DSurface(spec); }
#endif // ROGUECITY_HAS_IMPLOT3D

} // namespace RC_UI::API
