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

// ── Section helpers ────────────────────────────────────────────────────────

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
        ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border);

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
    }
    ImGui::Separator();
    ImGui::Spacing();

    const float avail_w = ImGui::GetContentRegionAvail().x;

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

    // ── Footer ────────────────────────────────────────────────────────────
    ImGui::Separator();
    Components::TextToken(UITokens::TextDisabled,
        "RC_UI::DataViz  |  visualizer/src/ui/api/rc_ui_dataviz.h");

    ImGui::End();
}

} // namespace RC_UI::DataVizGallery
