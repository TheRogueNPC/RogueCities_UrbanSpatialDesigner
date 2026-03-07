// FILE: rc_panel_dataviz_gallery.cpp
// PURPOSE: RCDV Gallery — self-contained demo window for all DataViz
// components. All demo data lives here; no engine state is accessed.
//
// Window: "RCDV Gallery##rcdv_gallery"
// Open via: Help > RCDV Gallery...
// ImGui coding standard compliant:
//   - Static-local state only (s_open, demo data arrays, sim state)
//   - snprintf for label text
//   - PushID/PopID per section
//   - Dummy() to claim layout space

#include "ui/panels/rc_panel_dataviz_gallery.h"

#include "ui/api/rc_imgui_api.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_tokens.h"

#include <cmath>
#include <limits>
#include <imgui.h>

namespace RC_UI::DataVizGallery {

namespace {

// ── Window open/close state ────────────────────────────────────────────────
static bool s_open = false;

// ── Bar chart demo data ────────────────────────────────────────────────────
// City metric snapshot — raw values (not normalized; chart normalizes internally)
static const float kBarVals[]          = {72.f, 45.f, 88.f, 31.f, 60.f, 55.f};
static const char* const kBarLabels[]  = {"Res", "Com", "Ind", "Park", "Util", "Civic"};
static constexpr int kBarCount         = 6;

// ── Sparkline demo data ────────────────────────────────────────────────────
// Simulated time-series telemetry (24 ticks)
static const float kSparkVals[] = {
    42.f, 45.f, 41.f, 48.f, 52.f, 50.f, 55.f, 53.f,
    58.f, 62.f, 60.f, 65.f, 61.f, 68.f, 72.f, 70.f,
    74.f, 71.f, 78.f, 82.f, 80.f, 85.f, 83.f, 88.f
};
static constexpr int kSparkCount = 24;

// ── Donut demo data ────────────────────────────────────────────────────────
// District zoning breakdown
static const float kDonutVals[]         = {35.f, 20.f, 18.f, 15.f, 12.f};
static const char* const kDonutLabels[] = {"Res", "Com", "Ind", "Park", "Civic"};
static constexpr int kDonutCount        = 5;

// ── Force graph demo data ──────────────────────────────────────────────────
// City district network — 10 nodes, 12 links.
// Groups: 0 = Residential, 1 = Commercial, 2 = Industrial, 3 = Civic
static RC_UI::DataViz::FDNode kFGNodes[] = {
    // x, y initialized in InitForceGraph(); vx=vy=0, fx=fy=NaN, group, id
    {0, 0, 0, 0, {}, {}, 3, "City Hall"},
    {0, 0, 0, 0, {}, {}, 1, "Market Sq"},
    {0, 0, 0, 0, {}, {}, 0, "Res. North"},
    {0, 0, 0, 0, {}, {}, 0, "Res. South"},
    {0, 0, 0, 0, {}, {}, 2, "Ind. Zone"},
    {0, 0, 0, 0, {}, {}, 1, "Tech Park"},
    {0, 0, 0, 0, {}, {}, 3, "Parklands"},
    {0, 0, 0, 0, {}, {}, 0, "Res. East"},
    {0, 0, 0, 0, {}, {}, 2, "Port"},
    {0, 0, 0, 0, {}, {}, 2, "Airport"},
};
static constexpr int kFGNodeCount = 10;

static RC_UI::DataViz::FDLink kFGLinks[] = {
    {0, 1, 2.f},  // City Hall  ↔ Market Sq
    {0, 2, 1.f},  // City Hall  ↔ Res. North
    {0, 3, 1.f},  // City Hall  ↔ Res. South
    {1, 4, 3.f},  // Market Sq  ↔ Ind. Zone
    {1, 5, 2.f},  // Market Sq  ↔ Tech Park
    {2, 6, 1.f},  // Res. North ↔ Parklands
    {2, 7, 1.f},  // Res. North ↔ Res. East
    {4, 8, 4.f},  // Ind. Zone  ↔ Port
    {5, 9, 3.f},  // Tech Park  ↔ Airport
    {1, 2, 1.f},  // Market Sq  ↔ Res. North
    {3, 6, 1.f},  // Res. South ↔ Parklands
    {7, 8, 2.f},  // Res. East  ↔ Port
};
static constexpr int kFGLinkCount = 12;

// Simulation spec — alpha starts at 1.0, cools to rest automatically.
static RC_UI::DataViz::ForceGraphSpec s_fg_spec = {
    "##rcdv_fg",
    kFGNodes,
    kFGLinks,
    kFGNodeCount,
    kFGLinkCount,
    /*width*/        0.0f,    // resolved at draw time
    /*height*/       280.0f,
    /*charge*/       -45.0f,
    /*link_distance*/40.0f,
    /*center_strength*/0.08f,
    /*alpha*/        1.0f,
    /*alpha_decay*/  0.0228f,
    /*velocity_decay*/0.4f,
};
static bool s_fg_initialized = false;

// Initialize node positions in a circle around the canvas center.
// Called once when the gallery opens (or on reset).
static void InitForceGraph(float canvas_w, float canvas_h) {
    constexpr float kTwoPi = 6.28318530718f;
    const float cx = canvas_w * 0.5f;
    const float cy = canvas_h * 0.5f;
    const float r  = std::min(canvas_w, canvas_h) * 0.30f;

    for (int i = 0; i < kFGNodeCount; ++i) {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(kFGNodeCount);
        kFGNodes[i].x  = cx + r * std::cos(angle);
        kFGNodes[i].y  = cy + r * std::sin(angle);
        kFGNodes[i].vx = 0.0f;
        kFGNodes[i].vy = 0.0f;
        // fx/fy: NaN = free (use quiet_NaN so isnan() check in sim works)
        kFGNodes[i].fx = std::numeric_limits<float>::quiet_NaN();
        kFGNodes[i].fy = std::numeric_limits<float>::quiet_NaN();
    }
    s_fg_spec.alpha = 1.0f; // reheat
}

// ── Drag-collision demo data ───────────────────────────────────────────────
// 100 circles (10×10 grid), radius 11px.  Mirrors D3's drag-collision example.
static constexpr int   kColCols   = 10;
static constexpr int   kColRows   = 10;
static constexpr int   kColCount  = kColCols * kColRows;
static constexpr float kColRadius = 11.0f;  // display + collision radius

static RC_UI::DataViz::CollisionNode kCollisionNodes[kColCount];
static bool s_col_initialized = false;

static RC_UI::DataViz::CollisionSpec s_col_spec = {
    "##rcdv_col",
    kCollisionNodes,
    kColCount,
    /*width*/         0.0f,   // resolved at draw time
    /*height*/        260.0f,
    /*alpha*/         1.0f,
    /*alpha_target*/  0.0f,
    /*alpha_decay*/   0.0228f,
    /*velocity_decay*/0.35f,
    /*iterations*/    4,
};

static void InitCollision(float canvas_w) {
    // Place circles in a centered grid with 1px gap between radii
    constexpr float gap  = kColRadius * 2.0f + 2.0f; // diameter + 2px gap
    const float start_x  = (canvas_w - kColCols * gap) * 0.5f + kColRadius;
    const float start_y  = (s_col_spec.height - kColRows * gap) * 0.5f + kColRadius;

    for (int i = 0; i < kColCount; ++i) {
        const int col = i % kColCols;
        const int row = i / kColCols;
        kCollisionNodes[i].x      = start_x + col * gap;
        kCollisionNodes[i].y      = start_y + row * gap;
        kCollisionNodes[i].vx     = 0.0f;
        kCollisionNodes[i].vy     = 0.0f;
        kCollisionNodes[i].fx     = std::numeric_limits<float>::quiet_NaN();
        kCollisionNodes[i].fy     = std::numeric_limits<float>::quiet_NaN();
        kCollisionNodes[i].radius = kColRadius;
        // 4-quadrant color groups — matches D3 d.type visual
        kCollisionNodes[i].group  = (col < kColCols / 2 ? 0 : 1)
                                  + (row < kColRows / 2 ? 0 : 2);
    }
    s_col_spec.alpha        = 1.0f;
    s_col_spec.alpha_target = 0.0f;
}

// ── Smooth zoom demo data ──────────────────────────────────────────────────
// 2000-point phyllotaxis spiral. step=12, canvas 680×300 nominal.
// Mirrors D3's "Smooth Zooming" example (data layout + camera transitions).
static constexpr int kZoomCount = 2000;
static RC_UI::DataViz::ZoomPoint kZoomPoints[kZoomCount];
static bool s_zoom_initialized = false;

static RC_UI::DataViz::SmoothZoomSpec s_zoom_spec = {
    "##rcdv_zoom",
    kZoomPoints,
    kZoomCount,
    /*canvas_w*/  0.0f,   // resolved at draw time
    /*canvas_h*/  300.0f,
    /*point_r*/   6.0f,   // D3 example radius = 6
};

// ── Easing animation demo data ─────────────────────────────────────────────
// Mirrors D3's easing principle: one shared normalized timeline, many
// different time->progress mappings.
static constexpr RC_UI::DataViz::EasingType kEaseModes[] = {
    RC_UI::DataViz::EasingType::Linear,
    RC_UI::DataViz::EasingType::PolyIn,
    RC_UI::DataViz::EasingType::PolyOut,
    RC_UI::DataViz::EasingType::PolyInOut,
    RC_UI::DataViz::EasingType::CubicInOut,
    RC_UI::DataViz::EasingType::SinInOut,
    RC_UI::DataViz::EasingType::ExpInOut,
    RC_UI::DataViz::EasingType::CircleInOut,
    RC_UI::DataViz::EasingType::BackInOut,
    RC_UI::DataViz::EasingType::ElasticOut,
    RC_UI::DataViz::EasingType::BounceOut,
};
static const char* const kEaseModeLabels[] = {
    "Linear", "Poly In", "Poly Out", "Poly InOut", "Cubic InOut",
    "Sin InOut", "Exp InOut", "Circle InOut", "Back InOut",
    "Elastic Out", "Bounce Out",
};
static constexpr int kEaseModeCount =
    static_cast<int>(sizeof(kEaseModes) / sizeof(kEaseModes[0]));

static constexpr RC_UI::DataViz::EasingType kEaseRaceModes[] = {
    RC_UI::DataViz::EasingType::Linear,
    RC_UI::DataViz::EasingType::PolyInOut,
    RC_UI::DataViz::EasingType::CubicInOut,
    RC_UI::DataViz::EasingType::BackInOut,
    RC_UI::DataViz::EasingType::ElasticOut,
    RC_UI::DataViz::EasingType::BounceOut,
};
static constexpr int kEaseRaceCount =
    static_cast<int>(sizeof(kEaseRaceModes) / sizeof(kEaseRaceModes[0]));

static int   s_ease_mode_index = 4; // CubicInOut
static float s_ease_clock      = 0.0f;
static float s_ease_duration   = 2.2f;
static RC_UI::DataViz::EasingParams s_ease_params = {
    /*poly_exponent*/     3.0f,
    /*back_overshoot*/    1.70158f,
    /*elastic_amplitude*/ 1.0f,
    /*elastic_period*/    0.30f,
};

// ── Section helpers ────────────────────────────────────────────────────────

#ifdef ROGUECITY_HAS_IMPLOT
// ── ImPlot: Time series demo data ─────────────────────────────────────────
// Three city metrics over 32 ticks: road count, district count, lot fill %
static const float kIPTSX[32]  = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };
static const float kIPTSRoads[32] = {
    12,14,15,17,19,22,24,28,31,34,38,42,45,49,51,55,
    57,60,63,67,70,73,76,79,82,84,86,88,90,91,92,93 };
static const float kIPTSDistricts[32] = {
    3,3,3,4,4,5,5,6,6,7,7,8,8,8,9,9,
    10,10,11,11,12,12,13,13,14,14,15,15,16,16,16,17 };
static const float kIPTSLotFill[32] = {
    10,12,15,18,22,26,30,35,40,44,48,52,55,58,61,64,
    66,68,70,72,74,76,77,78,79,80,81,82,83,84,85,86 };
static const float* const kIPTSYs[3] = { kIPTSRoads, kIPTSDistricts, kIPTSLotFill };
static const char* const  kIPTSLabels[3] = { "Roads", "Districts", "Lot Fill %" };
static const ImU32        kIPTSColors[3] = {
    UITokens::CyanAccent, UITokens::AmberGlow, UITokens::GreenHUD };

// ── ImPlot: Scatter demo (AESP-style Access vs Service scores) ─────────────
static constexpr int kIPScatterCount = 48;
static float         kIPScatterXs[kIPScatterCount];
static float         kIPScatterYs[kIPScatterCount];
static bool          s_scatter_initialized = false;
static void InitIPScatter() {
    uint32_t seed = 0xA5F5u;
    for (int i = 0; i < kIPScatterCount; ++i) {
        seed = seed * 1664525u + 1013904223u;
        kIPScatterXs[i] = static_cast<float>(seed & 0xFFFF) / 65535.0f * 100.0f;
        seed = seed * 1664525u + 1013904223u;
        kIPScatterYs[i] = static_cast<float>(seed & 0xFFFF) / 65535.0f * 100.0f;
    }
    s_scatter_initialized = true;
}

// ── ImPlot: Heatmap demo (8x8 zoning density grid, lot coverage 0-1) ───────
static constexpr int kIPHMRows = 8;
static constexpr int kIPHMCols = 8;
static const float   kIPHMValues[kIPHMRows * kIPHMCols] = {
    0.1f,0.2f,0.3f,0.5f,0.6f,0.5f,0.3f,0.2f,
    0.2f,0.4f,0.6f,0.8f,0.9f,0.8f,0.5f,0.3f,
    0.3f,0.5f,0.8f,1.0f,1.0f,0.9f,0.7f,0.4f,
    0.4f,0.6f,0.9f,1.0f,1.0f,1.0f,0.8f,0.5f,
    0.4f,0.6f,0.8f,1.0f,1.0f,0.9f,0.7f,0.5f,
    0.3f,0.5f,0.7f,0.8f,0.9f,0.8f,0.6f,0.4f,
    0.2f,0.3f,0.5f,0.6f,0.7f,0.6f,0.4f,0.3f,
    0.1f,0.2f,0.3f,0.4f,0.5f,0.4f,0.3f,0.2f,
};

// ── ImPlot: Histogram demo (synthetic building heights, storeys) ────────────
static constexpr int kIPHistCount = 64;
static float         kIPHistValues[kIPHistCount];
static bool          s_hist_initialized = false;
static void InitIPHist() {
    // Gaussian cluster around 10 storeys (deterministic LCG + Box-Muller)
    uint32_t s = 0xDEADBEEFu;
    for (int i = 0; i < kIPHistCount; ++i) {
        s = s * 1664525u + 1013904223u;
        const float u1 = static_cast<float>((s >> 8) & 0xFFFF) / 65535.0f;
        s = s * 1664525u + 1013904223u;
        const float u2 = static_cast<float>((s >> 8) & 0xFFFF) / 65535.0f;
        const float z  = std::sqrt(-2.0f * std::log(u1 + 1e-5f))
                       * std::cos(6.28318f * u2);
        kIPHistValues[i] = std::clamp(10.0f + z * 4.0f, 1.0f, 30.0f);
    }
    s_hist_initialized = true;
}
#endif // ROGUECITY_HAS_IMPLOT

#ifdef ROGUECITY_HAS_IMPLOT3D
// -- ImPlot3D: building centroid cloud (scatter) ----------------------------
static constexpr int k3DScatterCount = 80;
static float         k3DScatterXs[k3DScatterCount];
static float         k3DScatterYs[k3DScatterCount];
static float         k3DScatterZs[k3DScatterCount]; // elevation (metres)
static bool          s_scatter3d_initialized = false;
static void InitScatter3D() {
    uint32_t seed = 0xC0FFEE42u;
    for (int i = 0; i < k3DScatterCount; ++i) {
        seed = seed * 1664525u + 1013904223u;
        k3DScatterXs[i] = static_cast<float>(seed & 0xFFFF) / 65535.0f * 500.0f;
        seed = seed * 1664525u + 1013904223u;
        k3DScatterYs[i] = static_cast<float>(seed & 0xFFFF) / 65535.0f * 500.0f;
        seed = seed * 1664525u + 1013904223u;
        // Heights: mix of low residential + occasional high-rise
        k3DScatterZs[i] = 3.0f + static_cast<float>(seed & 0xFF) / 255.0f * 80.0f;
    }
    s_scatter3d_initialized = true;
}

// -- ImPlot3D: AESP accessibility surface (6x6 grid) -----------------------
static constexpr int k3DSurfN = 6;
static const float   k3DSurfXs[k3DSurfN] = {   0, 100, 200, 300, 400, 500 };
static const float   k3DSurfYs[k3DSurfN] = {   0, 100, 200, 300, 400, 500 };
// Row-major z[y][x]: Gaussian peak at city centre (indices 2-3)
static const float k3DSurfZs[k3DSurfN * k3DSurfN] = {
    0.2f,0.3f,0.4f,0.4f,0.3f,0.2f,
    0.3f,0.5f,0.7f,0.7f,0.5f,0.3f,
    0.4f,0.7f,1.0f,1.0f,0.7f,0.4f,
    0.4f,0.7f,1.0f,1.0f,0.7f,0.4f,
    0.3f,0.5f,0.7f,0.7f,0.5f,0.3f,
    0.2f,0.3f,0.4f,0.4f,0.3f,0.2f,
};
#endif // ROGUECITY_HAS_IMPLOT3D

// Thin framed child region for a single chart demo.
// Returns true when open; caller must call EndDemoCard() regardless.
static bool BeginDemoCard(const char* label) {
    ImGui::PushID(label);
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
        ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::BackgroundDark, 200)));
    ImGui::PushStyleColor(ImGuiCol_Border,
        ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::CyanAccent, 80)));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, UITokens::RoundingSubtle);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, UITokens::BorderThin);

    const bool open = ImGui::BeginChild("##card",
        ImVec2(ImGui::GetContentRegionAvail().x, 0.0f),
        ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);

    if (open) {
        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        Components::TextToken(UITokens::CyanAccent, "%s", label);
        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
    }
    return open;
}

static void EndDemoCard() {
    ImGui::Spacing();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::PopID();
    ImGui::Spacing();
}

} // anonymous namespace

// ── Public API ─────────────────────────────────────────────────────────────

bool IsOpen() { return s_open; }
void Toggle() { s_open = !s_open; }

void Draw() {
    if (!s_open)
        return;

    ImGui::SetNextWindowSize(ImVec2(720.0f, 620.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_FirstUseEver,
        ImVec2(0.5f, 0.5f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg,
        ImGui::ColorConvertU32ToFloat4(UITokens::PanelBackground));
    ImGui::PushStyleColor(ImGuiCol_Border,
        ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::CyanAccent, 160)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, UITokens::BorderNormal);

    const bool window_open = ImGui::Begin(
        "RCDV Gallery##rcdv_gallery", &s_open,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    if (!window_open) {
        ImGui::End();
        return;
    }

    // ── Header ────────────────────────────────────────────────────────────
    Components::DrawPanelFrame(UITokens::CyanAccent);

    Components::TextToken(UITokens::CyanAccent,
        "RC Data Viz Gallery  |  D3-style primitives in ImGui");
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 90.0f);
    if (ImGui::SmallButton("Reset Sims")) {
        const float avail_w = ImGui::GetContentRegionAvail().x;
        InitForceGraph(avail_w, s_fg_spec.height);
        InitCollision(avail_w);
        // Zoom: reinit to reset camera + re-randomize target sequence
        s_zoom_spec.seed = 42;
        API::InitSmoothZoom(s_zoom_spec, avail_w * 0.5f,
                            s_zoom_spec.canvas_h * 0.5f);
        s_ease_clock = 0.0f;
    }
    ImGui::Separator();
    ImGui::Spacing();

    // ── Bar Chart ─────────────────────────────────────────────────────────
    if (BeginDemoCard("Bar Chart — city zone metrics")) {
        const float inner_w = ImGui::GetContentRegionAvail().x - 12.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        API::BarChart({
            "##gallery_bar",
            kBarVals,
            kBarLabels,
            kBarCount,
            inner_w, 80.0f,
            UITokens::CyanAccent,
            UITokens::AmberGlow,
            true
        });
    }
    EndDemoCard();

    // ── Sparklines — two side by side ─────────────────────────────────────
    if (BeginDemoCard("Sparklines — line + area variants")) {
        const float half_w = (ImGui::GetContentRegionAvail().x - 24.0f) * 0.5f;

        // Line variant
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        Components::TextToken(UITokens::TextSecondary, "Line");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        API::Sparkline({
            "##spark_line",
            kSparkVals, kSparkCount,
            half_w, 50.0f,
            UITokens::GreenHUD,
            false
        });

        ImGui::SameLine(0.0f, 12.0f);

        // Area variant
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 18.0f); // align labels
        Components::TextToken(UITokens::TextSecondary, "Area fill");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()); // stay right column
        API::Sparkline({
            "##spark_area",
            kSparkVals, kSparkCount,
            half_w, 50.0f,
            UITokens::InfoBlue,
            true
        });
    }
    EndDemoCard();

    // ── Donut ─────────────────────────────────────────────────────────────
    if (BeginDemoCard("Donut — zoning breakdown")) {
        // Center the donut in the card
        constexpr float kRadius = 60.0f;
        const float card_w = ImGui::GetContentRegionAvail().x - 12.0f;
        const float x_offset = (card_w - kRadius * 2.0f) * 0.5f + 6.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x_offset);
        API::Donut({
            "##gallery_donut",
            kDonutVals, nullptr, kDonutLabels,
            kDonutCount,
            kRadius, 0.50f,
            true
        });
    }
    EndDemoCard();

    // ── Force-directed graph ───────────────────────────────────────────────
    if (BeginDemoCard("Force Graph — city district network (drag nodes)")) {
        const float canvas_w = ImGui::GetContentRegionAvail().x - 12.0f;

        // One-time init: place nodes in a circle
        if (!s_fg_initialized) {
            InitForceGraph(canvas_w, s_fg_spec.height);
            s_fg_initialized = true;
        }

        // Update canvas width (resolves fluid layout)
        s_fg_spec.width = canvas_w;

        // Step simulation (3 ticks/frame while hot; zero cost when cooled)
        API::StepForceGraph(s_fg_spec, 3);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        API::DrawForceGraph(s_fg_spec);

        // Alpha heat indicator
        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        {
            char heat_buf[48];
            snprintf(heat_buf, sizeof(heat_buf), "sim heat: %.3f", s_fg_spec.alpha);
            Components::TextToken(
                s_fg_spec.alpha > 0.05f ? UITokens::AmberGlow : UITokens::TextDisabled,
                "%s", heat_buf);
        }
    }
    EndDemoCard();

    // ── Drag-collision ────────────────────────────────────────────────────
    if (BeginDemoCard("Drag Collision — push circles, release to cool")) {
        const float canvas_w = ImGui::GetContentRegionAvail().x - 12.0f;

        if (!s_col_initialized) {
            InitCollision(canvas_w);
            s_col_initialized = true;
        }

        s_col_spec.width = canvas_w;

        // 1 tick/frame — collision is cheap with 100 circles; step more if desired
        API::StepCollision(s_col_spec, 1);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        API::DrawCollision(s_col_spec);

        // Heat/drag status row
        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        {
            const bool dragging = s_col_spec.alpha_target > 0.01f;
            char buf[64];
            if (dragging)
                snprintf(buf, sizeof(buf), "DRAGGING — heat: %.3f → %.3f",
                         s_col_spec.alpha, s_col_spec.alpha_target);
            else
                snprintf(buf, sizeof(buf), "sim heat: %.3f", s_col_spec.alpha);
            Components::TextToken(
                dragging           ? UITokens::AmberGlow :
                s_col_spec.alpha > 0.05f ? UITokens::GreenHUD :
                                           UITokens::TextDisabled,
                "%s", buf);
        }
    }
    EndDemoCard();

    // ── Smooth zoom ───────────────────────────────────────────────────────
    if (BeginDemoCard("Smooth Zoom — phyllotaxis spiral (2000 pts, auto-orbit)")) {
        const float canvas_w = ImGui::GetContentRegionAvail().x - 12.0f;

        if (!s_zoom_initialized) {
            s_zoom_spec.canvas_w = canvas_w;
            API::InitSmoothZoom(s_zoom_spec,
                                canvas_w * 0.5f,
                                s_zoom_spec.canvas_h * 0.5f);
            s_zoom_initialized = true;
        }

        s_zoom_spec.canvas_w = canvas_w;
        const float dt = ImGui::GetIO().DeltaTime;
        API::StepSmoothZoom(s_zoom_spec, dt);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        API::DrawSmoothZoom(s_zoom_spec);

        // Status row: current view radius + scale
        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        {
            const float scale = s_zoom_spec.canvas_h
                              / std::max(s_zoom_spec.vr, 0.001f);
            char buf[80];
            snprintf(buf, sizeof(buf),
                     "view r: %.1f  scale: %.2fx  target: (%.0f, %.0f)",
                     s_zoom_spec.vr, scale,
                     s_zoom_spec.dst_vx, s_zoom_spec.dst_vy);
            Components::TextToken(UITokens::TextDisabled, "%s", buf);
        }
    }
    EndDemoCard();

    // ── Easing animations ────────────────────────────────────────────────
    if (BeginDemoCard("Easing Animations — shared timeline, different velocity")) {
        const float inner_w = ImGui::GetContentRegionAvail().x - 12.0f;
        s_ease_duration = std::max(0.20f, s_ease_duration);
        s_ease_clock += ImGui::GetIO().DeltaTime;
        const float t = std::fmod(s_ease_clock, s_ease_duration) / s_ease_duration;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        ImGui::SetNextItemWidth(220.0f);
        ImGui::Combo("Curve##ease_curve", &s_ease_mode_index,
                     kEaseModeLabels, kEaseModeCount);

        ImGui::SameLine(0.0f, 12.0f);
        ImGui::SetNextItemWidth(140.0f);
        ImGui::DragFloat("Duration (s)##ease_duration", &s_ease_duration,
                         0.01f, 0.20f, 8.00f, "%.2f");

        const RC_UI::DataViz::EasingType selected_ease = kEaseModes[s_ease_mode_index];
        if (selected_ease == RC_UI::DataViz::EasingType::PolyIn ||
            selected_ease == RC_UI::DataViz::EasingType::PolyOut ||
            selected_ease == RC_UI::DataViz::EasingType::PolyInOut) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
            ImGui::SetNextItemWidth(220.0f);
            ImGui::DragFloat("Exponent##ease_exp", &s_ease_params.poly_exponent,
                             0.02f, 0.25f, 8.0f, "%.2f");
        } else if (selected_ease == RC_UI::DataViz::EasingType::BackInOut) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
            ImGui::SetNextItemWidth(220.0f);
            ImGui::DragFloat("Overshoot##ease_back", &s_ease_params.back_overshoot,
                             0.02f, 0.0f, 4.0f, "%.2f");
        } else if (selected_ease == RC_UI::DataViz::EasingType::ElasticOut) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
            ImGui::SetNextItemWidth(220.0f);
            ImGui::DragFloat("Amplitude##ease_amp", &s_ease_params.elastic_amplitude,
                             0.02f, 0.0f, 4.0f, "%.2f");
            ImGui::SameLine(0.0f, 12.0f);
            ImGui::SetNextItemWidth(160.0f);
            ImGui::DragFloat("Period##ease_period", &s_ease_params.elastic_period,
                             0.002f, 0.02f, 1.0f, "%.3f");
        }

        RC_UI::DataViz::EasingCurveSpec curve_spec{};
        curve_spec.id                   = "##ease_curve_view";
        curve_spec.easing               = selected_ease;
        curve_spec.params               = s_ease_params;
        curve_spec.width                = inner_w;
        curve_spec.height               = 150.0f;
        curve_spec.t                    = t;
        curve_spec.auto_range           = true;
        curve_spec.show_linear_reference = true;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        API::EasingCurve(curve_spec);

        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        {
            const float eased_t = API::EaseValue(selected_ease, t, s_ease_params);
            char status[128];
            snprintf(status, sizeof(status), "t: %.3f  eased(t): %.3f  curve: %s",
                     t, eased_t, RC_UI::DataViz::EasingTypeName(selected_ease));
            Components::TextToken(UITokens::TextDisabled, "%s", status);
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        Components::TextToken(
            UITokens::TextSecondary,
            "Comparison rows (all share the same t, only easing changes)");

        for (int i = 0; i < kEaseRaceCount; ++i) {
            RC_UI::DataViz::EasingMotionSpec motion_spec{};
            motion_spec.id                    = "##ease_motion_row";
            motion_spec.easing                = kEaseRaceModes[i];
            motion_spec.params                = s_ease_params;
            motion_spec.width                 = inner_w;
            motion_spec.height                = 38.0f;
            motion_spec.t                     = t;
            motion_spec.label                 = RC_UI::DataViz::EasingTypeName(kEaseRaceModes[i]);
            motion_spec.show_linear_reference = true;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
            ImGui::PushID(i);
            API::EasingMotion(motion_spec);
            ImGui::PopID();
        }
    }
    EndDemoCard();

#ifdef ROGUECITY_HAS_IMPLOT
    // ── ImPlot: Time Series ───────────────────────────────────────────────
    if (BeginDemoCard("ImPlot \xe2\x80\x94 Time Series (roads / districts / lot fill %)")) {
        RC_UI::DataViz::ImPlotTimeSeriesSpec spec{};
        spec.id           = "##ip_ts";
        spec.title        = "City Growth History";
        spec.ys           = kIPTSYs;
        spec.series_labels= kIPTSLabels;
        spec.xs           = kIPTSX;
        spec.series_count = 3;
        spec.value_count  = 32;
        spec.width        = 0.0f;
        spec.height       = 200.0f;
        spec.colors       = kIPTSColors;
        spec.fill_area    = true;
        API::TimeSeries(spec);
    }
    EndDemoCard();

    // ── ImPlot: Scatter ───────────────────────────────────────────────────
    if (BeginDemoCard("ImPlot \xe2\x80\x94 Scatter (AESP Access vs Service score)")) {
        if (!s_scatter_initialized) InitIPScatter();
        RC_UI::DataViz::ImPlotScatterSpec spec{};
        spec.id     = "##ip_sc";
        spec.title  = "AESP Distribution";
        spec.label  = "Lots";
        spec.xs     = kIPScatterXs;
        spec.ys     = kIPScatterYs;
        spec.count  = kIPScatterCount;
        spec.width  = 0.0f;
        spec.height = 200.0f;
        spec.color  = UITokens::CyanAccent;
        API::Scatter(spec);
    }
    EndDemoCard();

    // ── ImPlot: Heatmap ───────────────────────────────────────────────────
    if (BeginDemoCard("ImPlot \xe2\x80\x94 Heatmap (8\xc3\x97" "8 zoning density, Viridis)")) {
        RC_UI::DataViz::ImPlotHeatmapSpec spec{};
        spec.id        = "##ip_hm";
        spec.title     = "Lot Coverage Density";
        spec.label     = "Coverage";
        spec.values    = kIPHMValues;
        spec.rows      = kIPHMRows;
        spec.cols      = kIPHMCols;
        spec.scale_min = 0.0;
        spec.scale_max = 1.0;
        spec.width     = 0.0f;
        spec.height    = 220.0f;
        spec.colormap  = ImPlotColormap_Viridis;
        spec.label_fmt = nullptr;
        API::Heatmap(spec);
    }
    EndDemoCard();

    // ── ImPlot: Histogram ─────────────────────────────────────────────────
    if (BeginDemoCard("ImPlot \xe2\x80\x94 Histogram (building height distribution)")) {
        if (!s_hist_initialized) InitIPHist();
        RC_UI::DataViz::ImPlotHistogramSpec spec{};
        spec.id         = "##ip_hist";
        spec.title      = "Building Heights";
        spec.label      = "Height (storeys)";
        spec.values     = kIPHistValues;
        spec.count      = kIPHistCount;
        spec.bins       = ImPlotBin_Sturges; // Sturges: k = 1 + log2(n), sensible default for building-height samples
        spec.width      = 0.0f;
        spec.height     = 180.0f;
        spec.cumulative = false;
        spec.density    = false;
        spec.bar_color  = UITokens::MagentaHighlight;
        API::Histogram(spec);
    }
    EndDemoCard();
#endif // ROGUECITY_HAS_IMPLOT

#ifdef ROGUECITY_HAS_IMPLOT3D
    // -- ImPlot3D: Scatter (building centroid cloud) -----------------------
    if (BeginDemoCard("ImPlot3D -- Scatter (building centroids, XYZ)")) {
        if (!s_scatter3d_initialized) InitScatter3D();
        RC_UI::DataViz::ImPlot3DScatterSpec spec{};
        spec.id     = "##ip3d_sc";
        spec.title  = "Building Centroids";
        spec.label  = "Buildings";
        spec.xs     = k3DScatterXs;
        spec.ys     = k3DScatterYs;
        spec.zs     = k3DScatterZs;
        spec.count  = k3DScatterCount;
        spec.width  = 0.0f;
        spec.height = 260.0f;
        spec.color  = UITokens::AmberGlow;
        API::Scatter3D(spec);
    }
    EndDemoCard();

    // -- ImPlot3D: Surface (AESP accessibility) ----------------------------
    if (BeginDemoCard("ImPlot3D -- Surface (AESP accessibility, Viridis)")) {
        RC_UI::DataViz::ImPlot3DSurfaceSpec spec{};
        spec.id        = "##ip3d_srf";
        spec.title     = "AESP Accessibility";
        spec.label     = "Score";
        spec.xs        = k3DSurfXs;
        spec.ys        = k3DSurfYs;
        spec.zs        = k3DSurfZs;
        spec.x_count   = k3DSurfN;
        spec.y_count   = k3DSurfN;
        spec.scale_min = 0.0;
        spec.scale_max = 1.0;
        spec.width     = 0.0f;
        spec.height    = 260.0f;
        spec.colormap  = ImPlot3DColormap_Viridis;
        API::Surface3D(spec);
    }
    EndDemoCard();
#endif // ROGUECITY_HAS_IMPLOT3D

    // -- Footer ----
    ImGui::Separator();
    Components::TextToken(UITokens::TextDisabled,
        "RC_UI::DataViz (inline)  |  visualizer/src/ui/api/rc_ui_dataviz.h");
#ifdef ROGUECITY_HAS_IMPLOT
    API::SameLine(0.0f, 16.0f);
    Components::TextToken(UITokens::CyanAccent,
        "+ ImPlot  |  3rdparty/implot / RC_UI::API::{TimeSeries,Scatter,Heatmap,Histogram}");
#endif
#ifdef ROGUECITY_HAS_IMPLOT3D
    API::SameLine(0.0f, 16.0f);
    Components::TextToken(UITokens::GreenHUD,
        "+ ImPlot3D  |  3rdparty/implot3d / RC_UI::API::{Scatter3D,Line3D,Surface3D}");
#endif

    ImGui::End();
}

} // namespace RC_UI::DataVizGallery
