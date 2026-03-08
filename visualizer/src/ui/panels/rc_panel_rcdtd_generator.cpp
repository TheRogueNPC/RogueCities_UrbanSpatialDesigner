#include "ui/panels/rc_panel_rcdtd_generator.h"
#include "RogueCity/Generators/Urban/RCDTDGenerator.hpp"
#include "RogueCity/Visualizer/LucideIcons.hpp"
#include "RogueCity/Visualizer/SvgTextureCache.hpp"
#include "ui/api/rc_imgui_api.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

namespace RC_UI::Panels::RcdtdGenerator {

using namespace RogueCity::Generators::Urban;

static ImU32 GetZoneColor(ZoneType type) {
  // Y2K palette — matches canonical district color scheme where applicable.
  switch (type) {
  case ZoneType::CityCenter:   return WithAlpha(UITokens::YellowWarning,  200);
  case ZoneType::MixedResCom:  return WithAlpha(UITokens::InfoBlue,       180);
  case ZoneType::HighRes:      return WithAlpha(UITokens::CyanAccent,     180);
  case ZoneType::MedRes:       return WithAlpha(UITokens::MagentaHighlight, 150);
  case ZoneType::LowRes:       return WithAlpha(UITokens::TextSecondary,  160);
  case ZoneType::Commercial:   return WithAlpha(UITokens::SuccessGreen,   180);
  case ZoneType::Industrial:   return WithAlpha(UITokens::ErrorRed,       180);
  case ZoneType::Business:     return WithAlpha(UITokens::AmberGlow,      180);
  case ZoneType::Park:         return WithAlpha(UITokens::SuccessGreen, 160);
  default:                     return WithAlpha(UITokens::TextDisabled,   140);
  }
}

static ImU32 GetLevelColor(int levelIndex) {
  switch (levelIndex % 5) {
  case 0: return UITokens::InfoBlue;
  case 1: return UITokens::AmberGlow;
  case 2: return UITokens::MagentaHighlight;
  case 3: return UITokens::SuccessGreen;
  default: return UITokens::CyanAccent;
  }
}

enum class SliderMotionKind {
  BackInOut,
  ElasticOut,
  BounceOut,
};

struct SliderMotionProfile {
  SliderMotionKind valueKind;
  float valueDuration;
  float valueDelay;
  ImU32 color;
};

struct SliderMotionState {
  float fromNorm = 0.0f;
  float toNorm = 0.0f;
  float displayNorm = 0.0f;
  float elapsed = 0.0f;
  float duration = 0.35f;
  float delay = 0.0f;
  SliderMotionKind kind = SliderMotionKind::ElasticOut;
  uint32_t seedWaveSeen = 0;
  bool active = false;
  bool initialized = false;
};

static std::map<std::string, SliderMotionState> s_sliderMotionStates;
static std::map<std::string, ButtonFeedback> s_sliderRandomButtons;
static uint32_t s_seedWaveSerial = 0;

static float Clamp01(float v) {
  return std::clamp(v, 0.0f, 1.0f);
}

static float RandomUnit() {
  return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

static float RandomFloatRange(float minValue, float maxValue) {
  return minValue + (maxValue - minValue) * RandomUnit();
}

static int RandomIntRange(int minValue, int maxValue) {
  return minValue + (std::rand() % (maxValue - minValue + 1));
}

static float EaseOutBounce(float t) {
  constexpr float n1 = 7.5625f;
  constexpr float d1 = 2.75f;
  if (t < 1.0f / d1) {
    return n1 * t * t;
  }
  if (t < 2.0f / d1) {
    t -= 1.5f / d1;
    return n1 * t * t + 0.75f;
  }
  if (t < 2.5f / d1) {
    t -= 2.25f / d1;
    return n1 * t * t + 0.9375f;
  }
  t -= 2.625f / d1;
  return n1 * t * t + 0.984375f;
}

static float EaseOutElastic(float t) {
  if (t <= 0.0f)
    return 0.0f;
  if (t >= 1.0f)
    return 1.0f;
  constexpr float c4 = (2.0f * 3.14159265f) / 3.0f;
  return std::pow(2.0f, -10.0f * t) *
             std::sin((t * 10.0f - 0.75f) * c4) +
         1.0f;
}

static float EaseInOutBack(float t) {
  constexpr float c1 = 1.70158f;
  constexpr float c2 = c1 * 1.525f;
  if (t < 0.5f) {
    return (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) * 0.5f;
  }
  return (std::pow(2.0f * t - 2.0f, 2.0f) *
              ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) +
          2.0f) *
         0.5f;
}

static float ApplySliderEase(SliderMotionKind kind, float t) {
  switch (kind) {
  case SliderMotionKind::BackInOut:
    return EaseInOutBack(t);
  case SliderMotionKind::ElasticOut:
    return EaseOutElastic(t);
  case SliderMotionKind::BounceOut:
    return EaseOutBounce(t);
  default:
    return t;
  }
}

static void StartSliderMotion(SliderMotionState &state, float fromNorm,
                              float toNorm, SliderMotionKind kind,
                              float duration, float delay) {
  state.fromNorm = Clamp01(fromNorm);
  state.toNorm = Clamp01(toNorm);
  state.displayNorm = state.fromNorm;
  state.kind = kind;
  state.duration = std::max(0.05f, duration);
  state.delay = std::max(0.0f, delay);
  state.elapsed = 0.0f;
  state.active = true;
  state.initialized = true;
}

static float AdvanceSliderMotion(SliderMotionState &state, float targetNorm,
                                 float dt) {
  const float clampedTarget = Clamp01(targetNorm);
  if (!state.initialized) {
    state.fromNorm = clampedTarget;
    state.toNorm = clampedTarget;
    state.displayNorm = clampedTarget;
    state.initialized = true;
    return state.displayNorm;
  }

  if (!state.active) {
    state.displayNorm = clampedTarget;
    return state.displayNorm;
  }

  state.elapsed += dt;
  const float animTime = state.elapsed - state.delay;
  if (animTime <= 0.0f) {
    state.displayNorm = state.fromNorm;
    return state.displayNorm;
  }

  const float progress = Clamp01(animTime / state.duration);
  const float eased = ApplySliderEase(state.kind, progress);
  state.displayNorm = state.fromNorm + (state.toNorm - state.fromNorm) * eased;
  if (progress >= 1.0f) {
    state.displayNorm = state.toNorm;
    state.active = false;
  }
  return state.displayNorm;
}

static void DrawSliderMotionOverlay(const ImVec2 &sliderMin,
                                    const ImVec2 &sliderMax,
                                    float sliderWidth,
                                    float targetNorm,
                                    float displayNorm,
                                    ImU32 color) {
  ImDrawList *draw = ImGui::GetWindowDrawList();
  const float x0 = sliderMin.x + 6.0f;
  const float x1 = sliderMin.x + std::max(16.0f, sliderWidth - 6.0f);
  const float y = sliderMin.y + (sliderMax.y - sliderMin.y) * 0.5f;
  const float animatedX = x0 + (x1 - x0) * Clamp01(displayNorm);
  const float targetX = x0 + (x1 - x0) * Clamp01(targetNorm);
  draw->AddLine(ImVec2(x0, y), ImVec2(animatedX, y),
                WithAlpha(color, 180), 2.0f);
  draw->AddCircleFilled(ImVec2(animatedX, y), 4.0f, WithAlpha(color, 220));
  draw->AddCircle(ImVec2(animatedX, y), 7.0f, WithAlpha(color, 90), 0, 1.0f);
  if (std::abs(targetX - animatedX) > 1.0f) {
    draw->AddLine(ImVec2(animatedX, y + 5.0f), ImVec2(targetX, y + 5.0f),
                  WithAlpha(color, 85), 1.0f);
  }
}

static bool DrawAnimatedFloatSlider(const char *id, const char *label,
                                    float *value, float minValue,
                                    float maxValue, float dt,
                                    const SliderMotionProfile &profile) {
  auto &state = s_sliderMotionStates[id];
  auto &buttonFb = s_sliderRandomButtons[std::string(id) + ":rnd"];
  const float spacing = ImGui::GetStyle().ItemSpacing.x;
  const float buttonWidth = 48.0f;
  const float sliderWidth =
      std::max(96.0f, ImGui::GetContentRegionAvail().x - buttonWidth - spacing);
  const float currentNorm = Clamp01((*value - minValue) / (maxValue - minValue));

  if (state.seedWaveSeen != s_seedWaveSerial) {
    state.seedWaveSeen = s_seedWaveSerial;
    StartSliderMotion(state, 0.0f, currentNorm, SliderMotionKind::BackInOut,
                      0.38f, profile.valueDelay * 0.5f);
  }

  const float animatedNorm = AdvanceSliderMotion(state, currentNorm, dt);
  API::Text(UITokens::TextPrimary, "%s", label);
  API::SetNextItemWidth(sliderWidth);
  char hiddenLabel[64];
  std::snprintf(hiddenLabel, sizeof(hiddenLabel), "##%s", id);
  const bool changed = API::SliderFloat(hiddenLabel, value, minValue, maxValue);
  const ImVec2 sliderMin = ImGui::GetItemRectMin();
  const ImVec2 sliderMax = ImGui::GetItemRectMax();
  DrawSliderMotionOverlay(sliderMin, sliderMax, sliderWidth, currentNorm,
                          animatedNorm, profile.color);
  if (changed) {
    const float nextNorm = Clamp01((*value - minValue) / (maxValue - minValue));
    StartSliderMotion(state, animatedNorm, nextNorm, profile.valueKind,
                      profile.valueDuration, profile.valueDelay);
  }

  ImGui::SameLine(0.0f, spacing);
  char buttonId[72];
  std::snprintf(buttonId, sizeof(buttonId), "%s_rnd", id);
  bool randomized = false;
  if (Components::AnimatedActionButton(buttonId, "RND", buttonFb, dt, false,
                                       ImVec2(buttonWidth, UITokens::ResponsiveMinButton))) {
    const float oldNorm = Clamp01((*value - minValue) / (maxValue - minValue));
    *value = RandomFloatRange(minValue, maxValue);
    const float nextNorm = Clamp01((*value - minValue) / (maxValue - minValue));
    StartSliderMotion(state, oldNorm, nextNorm, profile.valueKind,
                      profile.valueDuration, profile.valueDelay);
    randomized = true;
  }
  API::Spacing();
  return changed || randomized;
}

static bool DrawAnimatedIntSlider(const char *id, const char *label,
                                  int *value, int minValue, int maxValue,
                                  float dt,
                                  const SliderMotionProfile &profile) {
  auto &state = s_sliderMotionStates[id];
  auto &buttonFb = s_sliderRandomButtons[std::string(id) + ":rnd"];
  const float spacing = ImGui::GetStyle().ItemSpacing.x;
  const float buttonWidth = 48.0f;
  const float sliderWidth =
      std::max(96.0f, ImGui::GetContentRegionAvail().x - buttonWidth - spacing);
  const float denom = std::max(1, maxValue - minValue);
  const float currentNorm =
      Clamp01(static_cast<float>(*value - minValue) / static_cast<float>(denom));

  if (state.seedWaveSeen != s_seedWaveSerial) {
    state.seedWaveSeen = s_seedWaveSerial;
    StartSliderMotion(state, 0.0f, currentNorm, SliderMotionKind::BackInOut,
                      0.38f, profile.valueDelay * 0.5f);
  }

  const float animatedNorm = AdvanceSliderMotion(state, currentNorm, dt);
  API::Text(UITokens::TextPrimary, "%s", label);
  API::SetNextItemWidth(sliderWidth);
  char hiddenLabel[64];
  std::snprintf(hiddenLabel, sizeof(hiddenLabel), "##%s", id);
  const bool changed = API::SliderInt(hiddenLabel, value, minValue, maxValue);
  const ImVec2 sliderMin = ImGui::GetItemRectMin();
  const ImVec2 sliderMax = ImGui::GetItemRectMax();
  DrawSliderMotionOverlay(sliderMin, sliderMax, sliderWidth, currentNorm,
                          animatedNorm, profile.color);
  if (changed) {
    const float nextNorm = Clamp01(
        static_cast<float>(*value - minValue) / static_cast<float>(denom));
    StartSliderMotion(state, animatedNorm, nextNorm, profile.valueKind,
                      profile.valueDuration, profile.valueDelay);
  }

  ImGui::SameLine(0.0f, spacing);
  char buttonId[72];
  std::snprintf(buttonId, sizeof(buttonId), "%s_rnd", id);
  bool randomized = false;
  if (Components::AnimatedActionButton(buttonId, "RND", buttonFb, dt, false,
                                       ImVec2(buttonWidth, UITokens::ResponsiveMinButton))) {
    const float oldNorm = Clamp01(
        static_cast<float>(*value - minValue) / static_cast<float>(denom));
    *value = RandomIntRange(minValue, maxValue);
    const float nextNorm = Clamp01(
        static_cast<float>(*value - minValue) / static_cast<float>(denom));
    StartSliderMotion(state, oldNorm, nextNorm, profile.valueKind,
                      profile.valueDuration, profile.valueDelay);
    randomized = true;
  }
  API::Spacing();
  return changed || randomized;
}

// Persistent state for the RCDTD generator
static RCDTDGenerator s_generator;
static RCDTDConfig s_config;
static bool s_initialized = false;

// Concept-showcase runtime state — animation clock, timing, phase tracking.
// s_anim_time is globally monotonic; never reset so scanline VFX stay continuous.
static float s_anim_time      = 0.0f; // Accumulates dt each frame (seconds)
static float s_gen_time_ms    = 0.0f; // Last Generate() wall-clock time in ms
static bool  s_generated_once = false; // Whether any output has been produced

// Per-action ButtonFeedback — one animation state per CTA.
static ButtonFeedback s_fb_generate;
static ButtonFeedback s_fb_step;
static ButtonFeedback s_fb_reset;
static ButtonFeedback s_fb_seed;
static ButtonFeedback s_fb_fit;

// Canvas pan/zoom state — persist across frames.
// Offset is in pixels relative to auto-fit center; zoom is a multiplier on top.
static ImVec2 s_canvas_offset{0.0f, 0.0f};
static float  s_canvas_zoom = 1.0f;

static void ResetCanvasView() {
  s_canvas_offset = {0.0f, 0.0f};
  s_canvas_zoom = 1.0f;
}

static void EnsureInitialized() {
  if (s_initialized)
    return;

  s_config.seed            = 42;
  s_config.minAngleDeg     = 55.0f; // Thesis recommended range 55-65
  s_config.recursionLevels = 2;
  s_config.growthBudget    = 1000.0f;
  s_config.straightnessWeight = 1.0f;
  s_config.distanceWeight  = 0.2f;
  s_config.capIntersectionsAt4 = true;

  // Pre-size to cover all supported recursion levels so UI edits never resize
  // the backing config during panel interaction.
  s_config.levels.resize(kMaxRcdtdRecursionLevels);
  s_config.levels[0] = {25.0f, {30.0f, 70.0f}, 60.0f, 100.0f};
  s_config.levels[1] = {12.0f, {15.0f, 35.0f}, 30.0f, 60.0f};
  s_config.levels[2] = { 6.0f, {10.0f, 20.0f}, 20.0f, 40.0f};
  s_config.levels[3] = { 4.0f, { 8.0f, 16.0f}, 16.0f, 24.0f};
  s_config.levels[4] = { 3.0f, { 6.0f, 12.0f}, 12.0f, 16.0f};

  s_config.zoneTypes = {
      ZoneType::CityCenter,  ZoneType::MixedResCom, ZoneType::HighRes,
      ZoneType::MedRes,      ZoneType::LowRes,      ZoneType::Commercial,
      ZoneType::Industrial,  ZoneType::Business,    ZoneType::Park};
  const size_t n = s_config.zoneTypes.size();
  s_config.targetAreaProportions.assign(n, 1.0f / static_cast<float>(n));
  s_config.adjacencyWeights.assign(n, std::vector<float>(n, 1.0f));
  // MixedResCom <-> Industrial penalty
  s_config.adjacencyWeights[1][6] = s_config.adjacencyWeights[6][1] = 0.1f;
  // Commercial <-> CityCenter affinity
  s_config.adjacencyWeights[5][0] = s_config.adjacencyWeights[0][5] = 2.0f;

  s_generator.SetConfig(s_config);
  s_initialized = true;
}

// DrawPipelineStatusBar — renders 3 stage chips (Roads → Blocks → Zones) that
// light up as generator output becomes populated. Derives state from live data;
// no additional tracking needed.
static void DrawPipelineStatusBar(
    const Graph &roads, const BlockGraph &blocks, const ZoningResult &zones) {
  struct Stage {
    const char *label;
    bool        done;
    ImU32       color;
  };
  const Stage stages[] = {
      {"  ROADS  ", !roads.edges().empty(),        UITokens::CyanAccent},
      {"  BLOCKS ", !blocks.faces.empty(),         UITokens::AmberGlow},
      {"  ZONES  ", !zones.assignments.empty(),    UITokens::SuccessGreen},
  };
  for (int i = 0; i < 3; ++i) {
    ImGui::PushID(i);
    Components::StatusChip(stages[i].label,
                           stages[i].done ? stages[i].color : UITokens::TextDisabled,
                           stages[i].done);
    if (i < 2) {
      ImGui::SameLine(0, 3);
      API::Text(UITokens::TextDisabled, "→"); // arrow; no API::TextUnformatted equiv, use API::Text
      ImGui::SameLine(0, 3);
    }
    ImGui::PopID();
  }
}

void DrawWindow(float dt) {
  s_anim_time += dt;
  EnsureInitialized();

  static RC_UI::DockableWindowState s_rcdtd_window;
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  if (!RC_UI::BeginDockableWindow("RC_DTD", s_rcdtd_window, "Right",
                                  ImGuiWindowFlags_NoCollapse)) {
    return;
  }

  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{
          "RC_DTD",
          "RC_DTD Downtown Generator",
          "generator",
          "Right",
          "visualizer/src/ui/panels/rc_panel_rcdtd_generator.cpp",
          {"rc_dtd", "downtown", "generator", "schema"}},
      true);
  Components::DrawPanelFrame(UITokens::CyanAccent);

  // ── Concept showcase subtitle ─────────────────────────────────────────────
  // Communicate the automated-pipeline intent at first glance.
  {
    if (auto ico = RC::SvgTextureCache::Get().Load(LC::Cpu, 11.f)) {
      ImGui::Image(ico, ImVec2(11, 11));
      ImGui::SameLine(0, 4);
    }
    Components::TextToken(WithAlpha(UITokens::TextSecondary, 160),
                          "AUTOMATED PIPELINE  /  CONCEPT SHOWCASE");
    API::Spacing();
  }

  // ── Pipeline status bar ───────────────────────────────────────────────────
  // Three stage chips; state is derived from live generator output.
  DrawPipelineStatusBar(s_generator.GetRoadGraph(),
                        s_generator.GetBlockGraph(),
                        s_generator.GetZoning());
  API::Spacing();

  // ── Generator Config ──────────────────────────────────────────────────────
  if (Components::DrawSectionHeader("Generator Config", UITokens::CyanAccent, true,
                                    RC::SvgTextureCache::Get().Load(LC::Sliders, 14.f))) {
    API::Indent();
    bool changed = false;
    const float panelWidth = ImGui::GetContentRegionAvail().x;
    const bool compactConfig = panelWidth < UITokens::BreakpointCompact;
    const float buttonHeight = UITokens::ResponsiveMinButton;
    const SliderMotionProfile kBaseElasticProfiles[] = {
      {SliderMotionKind::ElasticOut, 0.52f, 0.02f, UITokens::CyanAccent},
      {SliderMotionKind::ElasticOut, 0.54f, 0.05f, UITokens::AmberGlow},
      {SliderMotionKind::ElasticOut, 0.56f, 0.08f, UITokens::MagentaHighlight},
      {SliderMotionKind::ElasticOut, 0.58f, 0.11f, UITokens::InfoBlue},
    };
    const SliderMotionProfile kRecursionProfile{
      SliderMotionKind::ElasticOut, 0.50f, 0.14f, UITokens::SuccessGreen};

    // Seed row: slider + randomize button on the same line.
    {
      const float spacing  = ImGui::GetStyle().ItemSpacing.x;
      const float rand_w   = compactConfig ? ImGui::GetContentRegionAvail().x : 88.0f;
      const float seed_lbl = compactConfig ? 0.0f : ImGui::CalcTextSize("Seed ").x;
      const float slider_w = compactConfig
          ? ImGui::GetContentRegionAvail().x
          : std::max(96.0f,
                     ImGui::GetContentRegionAvail().x - rand_w - spacing - seed_lbl);

      if (!compactConfig) {
        API::Text(UITokens::TextPrimary, "Seed");
        API::SameLine(0, spacing);
      }
      API::SetNextItemWidth(slider_w);
      ImGui::PushStyleVar(
          ImGuiStyleVar_FramePadding,
          ImVec2(ImGui::GetStyle().FramePadding.x,
                 ImGui::GetStyle().FramePadding.y + 3.0f));
      changed |= API::SliderInt("##seed", (int*)&s_config.seed, 0, 9999);
      ImGui::PopStyleVar();

      if (compactConfig) {
        API::Spacing();
      } else {
        API::SameLine(0, spacing);
      }

      if (Components::AnimatedActionButton(
              "rand_seed", compactConfig ? "NEW SEED" : "  NEW SEED",
              s_fb_seed, dt, false, ImVec2(rand_w, buttonHeight))) {
        s_config.seed = static_cast<uint32_t>(std::rand() % 10000);
        ++s_seedWaveSerial;
        changed = true;
      }
      if (const auto ico = RC::SvgTextureCache::Get().Load(LC::RefreshCw, 11.f)) {
        const ImVec2 ip(ImGui::GetItemRectMin().x + 10.0f,
                        ImGui::GetItemRectMin().y + (buttonHeight - 11.0f) * 0.5f);
        ImGui::GetWindowDrawList()->AddImage(
            ico, ip, ImVec2(ip.x + 11.0f, ip.y + 11.0f),
            ImVec2(0, 0), ImVec2(1, 1), UITokens::TextSecondary);
      }
      if (ImGui::IsItemHovered()) ImGui::SetTooltip("Randomize seed");
    }

    ImGui::PushStyleVar(
      ImGuiStyleVar_FramePadding,
      ImVec2(ImGui::GetStyle().FramePadding.x,
           ImGui::GetStyle().FramePadding.y + 3.0f));
    changed |= DrawAnimatedFloatSlider(
      "growth_budget", "Growth Budget", &s_config.growthBudget,
      100.0f, 10000.0f, dt, kBaseElasticProfiles[0]);
    changed |= DrawAnimatedFloatSlider(
      "min_angle", "Min Angle (Deg)", &s_config.minAngleDeg,
      10.0f, 90.0f, dt, kBaseElasticProfiles[1]);
    changed |= DrawAnimatedFloatSlider(
      "straightness", "Straightness Wt", &s_config.straightnessWeight,
      0.0f, 5.0f, dt, kBaseElasticProfiles[2]);
    changed |= DrawAnimatedFloatSlider(
      "distance", "Distance Wt", &s_config.distanceWeight,
      0.0f, 5.0f, dt, kBaseElasticProfiles[3]);
    ImGui::PopStyleVar();
    
    changed |= API::Checkbox("Cap 4-Way Intersections", &s_config.capIntersectionsAt4);
    API::Spacing();

    // Recursion levels — clamp levels array size to match
    int rec = s_config.recursionLevels;
    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(ImGui::GetStyle().FramePadding.x,
               ImGui::GetStyle().FramePadding.y + 3.0f));
    if (DrawAnimatedIntSlider("recursion_levels", "Recursion Levels", &rec,
                  1, kMaxRcdtdRecursionLevels, dt,
                  kRecursionProfile)) {
      s_config.recursionLevels = rec;
      changed = true;
    }
    ImGui::PopStyleVar();
    
    if (changed)
      s_generator.SetConfig(s_config);
    API::Unindent();
    API::Spacing();
  }

  // ── Per-level configs (dynamic, driven by recursionLevels) ───────────────
  const int numLevels = std::clamp(s_config.recursionLevels, 1, kMaxRcdtdRecursionLevels);
  for (int li = 0; li < numLevels; ++li) {
    ImGui::PushID(li);
    char levelLabel[32];
    std::snprintf(levelLabel, sizeof(levelLabel), "Growth Level %d", li);
    if (Components::DrawSectionHeader(levelLabel, GetLevelColor(li), false,
                                      RC::SvgTextureCache::Get().Load(LC::Layers, 14.f))) {
      API::Indent();
      auto &l = s_config.levels[static_cast<size_t>(li)];
      bool lc = false;
      const SliderMotionProfile levelProfiles[] = {
          {SliderMotionKind::BounceOut, 0.42f, 0.00f, GetLevelColor(li)},
          {SliderMotionKind::BounceOut, 0.45f, 0.03f, GetLevelColor(li)},
          {SliderMotionKind::BounceOut, 0.48f, 0.06f, GetLevelColor(li)},
          {SliderMotionKind::BounceOut, 0.51f, 0.09f, GetLevelColor(li)},
          {SliderMotionKind::BounceOut, 0.54f, 0.12f, GetLevelColor(li)},
      };
      char idBuffer[64];
      
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, ImGui::GetStyle().FramePadding.y + 2.0f));
      std::snprintf(idBuffer, sizeof(idBuffer), "level_%d_clearance", li);
      lc |= DrawAnimatedFloatSlider(idBuffer, "Clearance", &l.clearance,
                                    1.0f, 100.0f, dt, levelProfiles[0]);
      std::snprintf(idBuffer, sizeof(idBuffer), "level_%d_conn_radius", li);
      lc |= DrawAnimatedFloatSlider(idBuffer, "Conn Radius", &l.connectionRadius,
                                    10.0f, 300.0f, dt, levelProfiles[1]);
      std::snprintf(idBuffer, sizeof(idBuffer), "level_%d_min_ext", li);
      lc |= DrawAnimatedFloatSlider(idBuffer, "Min Ext", &l.extensionRange.min,
                                    5.0f, 150.0f, dt, levelProfiles[2]);
      std::snprintf(idBuffer, sizeof(idBuffer), "level_%d_max_ext", li);
      lc |= DrawAnimatedFloatSlider(idBuffer, "Max Ext", &l.extensionRange.max,
                                    10.0f, 300.0f, dt, levelProfiles[3]);
      std::snprintf(idBuffer, sizeof(idBuffer), "level_%d_split_dist", li);
      lc |= DrawAnimatedFloatSlider(idBuffer, "Split Dist", &l.edgeSplitDistance,
                                    8.0f, 300.0f, dt, levelProfiles[4]);
      ImGui::PopStyleVar();

      if (l.extensionRange.min > l.extensionRange.max) {
        std::swap(l.extensionRange.min, l.extensionRange.max);
        lc = true;
      }
      
      if (lc) s_generator.SetConfig(s_config);
      API::Unindent();
      API::Spacing();
    }
    ImGui::PopID();
  }

  // ── Actions ───────────────────────────────────────────────────────────────
  // Generation is fully automated: one button produces the complete downtown.
  // Step allows incremental exploration; Reset clears all output.
  API::Spacing();
  {
    // Phase status chip + generation time.
    ImGui::PushID("phase_status");
    if (!s_generated_once) {
      Components::StatusChip("  IDLE  ", UITokens::TextDisabled, false);
    } else {
      Components::StatusChip("  COMPLETE  ", UITokens::SuccessGreen, true);
      ImGui::SameLine(0, 8);
      Components::TextToken(WithAlpha(UITokens::AmberGlow, 180), "%.1f ms", s_gen_time_ms);
    }
    ImGui::PopID();
    API::Spacing();

    // ── Hero: GENERATE DOWNTOWN ──────────────────────────────────────────
    // Scanline backdrop when idle: visual invitation to interact with the CTA.
    {
      const ImVec2 btn_origin = ImGui::GetCursorScreenPos();
      const float  btn_h      = 48.0f;
      const float  btn_w      = ImGui::GetContentRegionAvail().x;
      if (!s_generated_once) {
        Components::DrawScanlineBackdrop(
            btn_origin,
            ImVec2(btn_origin.x + btn_w, btn_origin.y + btn_h),
            s_anim_time,
            WithAlpha(UITokens::CyanAccent, 28));
      }

      const bool gen_clicked = Components::AnimatedActionButton(
          "generate_all", "  GENERATE DOWNTOWN SCHEMA",
          s_fb_generate, dt, false, ImVec2(btn_w, btn_h));

      // Overlay Zap icon on the left face of the button after drawing it.
      // AddImage is an out-of-order draw so we use the foreground draw list.
      if (const auto zap_ico = RC::SvgTextureCache::Get().Load(LC::Zap, 18.f)) {
        const ImVec2 ip(ImGui::GetItemRectMin().x + 10.0f,
                        ImGui::GetItemRectMin().y + (btn_h - 18.0f) * 0.5f);
        ImGui::GetWindowDrawList()->AddImage(
            zap_ico, ip, ImVec2(ip.x + 18.0f, ip.y + 18.0f),
            ImVec2(0, 0), ImVec2(1, 1),
            WithAlpha(UITokens::CyanAccent, 200));
      }

      if (gen_clicked) {
        const auto t0 = std::chrono::high_resolution_clock::now();
        s_generator.Generate();
        const auto t1 = std::chrono::high_resolution_clock::now();
        s_gen_time_ms = static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count())
            / 1000.0f;
        s_generated_once = true;
        ResetCanvasView();
      }
    }

    API::Spacing();

    // ── Secondary: STEP + RESET on a single row ───────────────────────────
    API::Spacing();
    {
      const float row_w  = ImGui::GetContentRegionAvail().x;
      const float spacing = UITokens::SpaceS;
      const bool compactActions = row_w < UITokens::BreakpointCompact;
      const float actionH = UITokens::ResponsiveMinButton + 4.0f;
      const float buttonW = compactActions ? row_w : (row_w - spacing) * 0.5f;

      // STEP button
      if (Components::AnimatedActionButton(
              "step_gen", compactActions ? "STEP NETWORK" : "  STEP NETWORK",
              s_fb_step, dt, false, ImVec2(buttonW, actionH))) {
        s_generator.GenerateStep();
        s_generated_once = !s_generator.GetRoadGraph().edges().empty()
                        || !s_generator.GetBlockGraph().faces.empty()
                        || !s_generator.GetZoning().assignments.empty();
      }
      if (const auto step_ico = RC::SvgTextureCache::Get().Load(LC::StepForward, 13.f)) {
        const ImVec2 ip(ImGui::GetItemRectMin().x + 10.0f,
                        ImGui::GetItemRectMin().y + (actionH - 13.0f) * 0.5f);
        ImGui::GetWindowDrawList()->AddImage(
            step_ico, ip, ImVec2(ip.x + 13.0f, ip.y + 13.0f),
            ImVec2(0, 0), ImVec2(1, 1), WithAlpha(UITokens::TextSecondary, 180));
      }

      if (compactActions) {
        API::Spacing();
      } else {
        ImGui::SameLine(0.0f, spacing);
      }

      // RESET button
      if (Components::AnimatedActionButton(
              "reset_gen", compactActions ? "RESET SCHEMA" : "  RESET SCHEMA",
              s_fb_reset, dt, false, ImVec2(buttonW, actionH))) {
        s_generator = RCDTDGenerator{};
        s_generator.SetConfig(s_config);
        s_generated_once = false;
        s_gen_time_ms    = 0.0f;
        ResetCanvasView();
      }
      if (const auto rst_ico = RC::SvgTextureCache::Get().Load(LC::RefreshCw, 13.f)) {
        const ImVec2 ip(ImGui::GetItemRectMin().x + 10.0f,
                        ImGui::GetItemRectMin().y + (actionH - 13.0f) * 0.5f);
        ImGui::GetWindowDrawList()->AddImage(
            rst_ico, ip, ImVec2(ip.x + 13.0f, ip.y + 13.0f),
            ImVec2(0, 0), ImVec2(1, 1), WithAlpha(UITokens::TextSecondary, 180));
      }
    }
    API::Spacing();
    API::Spacing();
  }

  // ── Stats ─────────────────────────────────────────────────────────────────
  API::Spacing();
  if (Components::DrawSectionHeader("Stats", UITokens::TextSecondary, false,
                                    RC::SvgTextureCache::Get().Load(LC::Activity, 14.f))) {
    API::Indent();
    API::Text(UITokens::TextPrimary, "Nodes: %zu",         s_generator.GetRoadGraph().vertices().size());
    API::Text(UITokens::TextPrimary, "Edges: %zu",         s_generator.GetRoadGraph().edges().size());
    API::Text(UITokens::TextPrimary, "Faces/Blocks: %zu",  s_generator.GetBlockGraph().faces.size());
    API::Text(UITokens::TextPrimary, "Zones assigned: %zu",s_generator.GetZoning().assignments.size());
    if (s_gen_time_ms > 0.0f)
      API::Text(UITokens::AmberGlow, "Last Generate(): %.2f ms", s_gen_time_ms);
    API::Unindent();
  }

  API::Spacing();

  // ── Canvas ────────────────────────────────────────────────────────────────
  // Contract: BeginChild/EndChild must be unconditional — content is gated
  // inside. Pan: right-click drag. Zoom: mouse wheel centered on cursor.
  const ImVec2 canvas_size = ImGui::GetContentRegionAvail();
  const ImVec2 child_size(canvas_size.x, std::max(canvas_size.y, 1.0f));
  ImGui::BeginChild("RcdtdCanvas", child_size, true,
                    ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse);

  if (canvas_size.y > 60.0f) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

    // Background
    draw_list->AddRectFilled(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        UITokens::BackgroundDark);


    // Y2K Warning stripe border
    draw_list->AddRect(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        UITokens::YellowWarning, 0.0f, 0, 2.0f);

    // ── Pan / Zoom interaction ────────────────────────────────────────────
    // Mouse wheel zooms centered on cursor; right-click drags to pan.
    if (ImGui::IsWindowHovered()) {
      const float scroll = ImGui::GetIO().MouseWheel;
      if (std::abs(scroll) > 0.0f) {
        const float k = (scroll > 0.0f) ? 1.15f : (1.0f / 1.15f);
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const float cx = canvas_pos.x + canvas_size.x * 0.5f;
        const float cy = canvas_pos.y + canvas_size.y * 0.5f;
        // Zoom centered on mouse: adjust offset so point under cursor stays fixed.
        s_canvas_offset.x = mouse.x - cx + (s_canvas_offset.x - (mouse.x - cx)) * k;
        s_canvas_offset.y = mouse.y - cy + (s_canvas_offset.y - (mouse.y - cy)) * k;
        s_canvas_zoom = std::clamp(s_canvas_zoom * k, 0.05f, 40.0f);
      }
      if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
        const ImVec2 d = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right, 0.0f);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
        s_canvas_offset.x += d.x;
        s_canvas_offset.y += d.y;
      }
    }
    // Fit button — top-right corner of canvas
    {
      const float btn_w = 48.0f;
      const float btn_h = UITokens::ResponsiveMinButton;
      ImGui::SetCursorScreenPos(ImVec2(
          canvas_pos.x + canvas_size.x - btn_w - 6.0f,
          canvas_pos.y + 6.0f));

      if (Components::AnimatedActionButton("fit_btn", "FIT", s_fb_fit, dt, false, ImVec2(btn_w, btn_h))) {
        ResetCanvasView();
      }
    }

    // ── Auto-fit bounds ───────────────────────────────────────────────────
    auto &roadGraph = s_generator.GetRoadGraph();
    bool has_bounds = false;
    RogueCity::Core::Vec2 min_b{0.0, 0.0};
    RogueCity::Core::Vec2 max_b{0.0, 0.0};
    for (const auto &v : roadGraph.vertices()) {
      if (!has_bounds) { min_b = max_b = v.pos; has_bounds = true; }
      else {
        min_b.x = std::min(min_b.x, v.pos.x); min_b.y = std::min(min_b.y, v.pos.y);
        max_b.x = std::max(max_b.x, v.pos.x); max_b.y = std::max(max_b.y, v.pos.y);
      }
    }
    double w = max_b.x - min_b.x;
    double h = max_b.y - min_b.y;
    const double pad = std::max(std::max(w, h) * 0.1, 10.0);
    min_b.x -= pad; min_b.y -= pad;
    max_b.x += pad; max_b.y += pad;
    w = max_b.x - min_b.x;
    h = max_b.y - min_b.y;

    // World-to-screen: auto-fit is the neutral frame, zoom+pan layered on top.
    const float cx = canvas_pos.x + canvas_size.x * 0.5f;
    const float cy = canvas_pos.y + canvas_size.y * 0.5f;
    const auto world_to_screen = [&](const RogueCity::Core::Vec2 &p) -> ImVec2 {
      const float bx = canvas_pos.x + static_cast<float>((p.x - min_b.x) / w) * canvas_size.x;
      const float by = canvas_pos.y + static_cast<float>((p.y - min_b.y) / h) * canvas_size.y;
      return ImVec2(
          cx + (bx - cx) * s_canvas_zoom + s_canvas_offset.x,
          cy + (by - cy) * s_canvas_zoom + s_canvas_offset.y);
    };

    if (has_bounds && w > 1e-6 && h > 1e-6) {
      // Draw block faces (zone wireframe & fill)
      auto &blockGraph = s_generator.GetBlockGraph();
      for (const auto &[id, face] : blockGraph.faces) {
        if (face.boundary.empty()) continue;
        const ImU32 zone_col = GetZoneColor(face.assignedZone);
        const ImU32 fill_col = WithAlpha(zone_col, 50); // Muted fill for the zone
        std::vector<ImVec2> pts;
        pts.reserve(face.boundary.size());
        for (const auto &bp : face.boundary)
          pts.push_back(world_to_screen(bp));
          
        draw_list->AddConvexPolyFilled(pts.data(), static_cast<int>(pts.size()), fill_col);
        draw_list->AddPolyline(pts.data(), static_cast<int>(pts.size()),
                               zone_col, ImDrawFlags_Closed, 1.5f);
      }
      // Draw road edges
      for (const auto &e : roadGraph.edges()) {
        const auto *vA = roadGraph.getVertex(e.a);
        const auto *vB = roadGraph.getVertex(e.b);
        const float degreeHint =
            static_cast<float>(vA->edges.size() + vB->edges.size()) * 0.5f;
        const float roadWidth = degreeHint >= 3.0f ? 2.4f : 1.6f;
        draw_list->AddLine(world_to_screen(vA->pos), world_to_screen(vB->pos),
                           WithAlpha(UITokens::CyanAccent, 72), roadWidth + 2.0f);
        draw_list->AddLine(world_to_screen(vA->pos), world_to_screen(vB->pos),
                           UITokens::CyanAccent, roadWidth);
      }
      // Draw nodes
      for (const auto &v : roadGraph.vertices()) {
        const float radius = v.edges.size() >= 3 ? 2.8f : 2.0f;
        draw_list->AddCircleFilled(world_to_screen(v.pos), radius, UITokens::TextPrimary);
      }

      const ImVec2 hudMin(canvas_pos.x + 10.0f, canvas_pos.y + 10.0f);
      const ImVec2 hudMax(hudMin.x + 186.0f, hudMin.y + 52.0f);
      draw_list->AddRectFilled(hudMin, hudMax,
                               WithAlpha(UITokens::PanelBackground, 220), 6.0f);
      draw_list->AddRect(hudMin, hudMax,
                         WithAlpha(UITokens::CyanAccent, 150), 6.0f, 0, 1.0f);
      draw_list->AddText(ImVec2(hudMin.x + 10.0f, hudMin.y + 8.0f),
                         UITokens::TextPrimary, "PAN: RMB drag");
      draw_list->AddText(ImVec2(hudMin.x + 10.0f, hudMin.y + 26.0f),
                         UITokens::TextSecondary, "ZOOM: Mouse wheel");

      // ── Zone legend (bottom-left) ─────────────────────────────────────
      struct LegendEntry { const char *name; ZoneType zone; };
      static constexpr LegendEntry kLegend[] = {
          {"City Center",  ZoneType::CityCenter},
          {"Mixed Res/Com",ZoneType::MixedResCom},
          {"High Res",     ZoneType::HighRes},
          {"Med Res",      ZoneType::MedRes},
          {"Low Res",      ZoneType::LowRes},
          {"Commercial",   ZoneType::Commercial},
          {"Industrial",   ZoneType::Industrial},
          {"Business",     ZoneType::Business},
          {"Park",         ZoneType::Park},
      };
      const float legendW = 162.0f;
      const float rowH = 16.0f;
      const float legendH = 14.0f + static_cast<float>(IM_ARRAYSIZE(kLegend)) * rowH;
      const float lg_x = canvas_pos.x + 8.0f;
      float lg_y = canvas_pos.y + canvas_size.y - 8.0f - legendH;
      draw_list->AddRectFilled(
          ImVec2(lg_x - 6.0f, lg_y - 6.0f),
          ImVec2(lg_x + legendW, lg_y + legendH),
          WithAlpha(UITokens::PanelBackground, 220), 6.0f);
      draw_list->AddRect(
          ImVec2(lg_x - 6.0f, lg_y - 6.0f),
          ImVec2(lg_x + legendW, lg_y + legendH),
          WithAlpha(UITokens::YellowWarning, 180), 6.0f, 0, 1.0f);
      for (const auto &entry : kLegend) {
        draw_list->AddRectFilled(
            {lg_x, lg_y + 2.0f}, {lg_x + 10.0f, lg_y + 12.0f},
            GetZoneColor(entry.zone));
        draw_list->AddText({lg_x + 14.0f, lg_y}, UITokens::TextSecondary, entry.name);
        lg_y += rowH;
      }
    } else {
      // No graph data — draw help text
      const char *hint = "Press GENERATE DOWNTOWN";
      const char *subHint = "to grow a downtown schema brush";
      const ImVec2 ts = ImGui::CalcTextSize(hint);
      const ImVec2 subTs = ImGui::CalcTextSize(subHint);
      const ImVec2 boxMin(canvas_pos.x + (canvas_size.x - 280.0f) * 0.5f,
                          canvas_pos.y + (canvas_size.y - 72.0f) * 0.5f);
      const ImVec2 boxMax(boxMin.x + 280.0f, boxMin.y + 72.0f);
      draw_list->AddRectFilled(boxMin, boxMax,
                               WithAlpha(UITokens::PanelBackground, 220), 6.0f);
      draw_list->AddRect(boxMin, boxMax,
                         WithAlpha(UITokens::CyanAccent, 180), 6.0f, 0, 1.0f);
      draw_list->AddText(
          ImVec2(canvas_pos.x + (canvas_size.x - ts.x) * 0.5f,
                 boxMin.y + 16.0f),
          UITokens::TextPrimary, hint);
      draw_list->AddText(
          ImVec2(canvas_pos.x + (canvas_size.x - subTs.x) * 0.5f,
                 boxMin.y + 38.0f),
          UITokens::TextSecondary, subHint);
    }
  } // end content block (canvas_size.y > 60.0f)

  // EndChild always paired with the unconditional BeginChild above.
  ImGui::EndChild();

  uiint.EndPanel();
  RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::RcdtdGenerator
