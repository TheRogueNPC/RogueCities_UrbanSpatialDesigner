// FILE: rc_ui_dataviz.h
// PURPOSE: D3-style data visualization primitives for RC_UI — display-only v1.
//
// All components are inline, zero-heap, ImGui-compliant:
//   - PushID/PopID per call
//   - Dummy() to claim layout space after custom draws
//   - snprintf for label text (no std::string per-frame)
//   - Colors via UITokens:: or caller-provided ImU32
//
// TODO(interactivity): hover/click stubs are marked throughout.
//   Wire IsMouseHoveringRect + SetTooltip in a future pass once the
//   database/core refactor settles.
#pragma once

#include "ui/rc_ui_tokens.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <imgui.h>
#ifdef ROGUECITY_HAS_IMPLOT
#include <implot.h> // must precede namespace RC_UI::DataViz
#endif
#ifdef ROGUECITY_HAS_IMPLOT3D
#include <implot3d.h> // must precede namespace RC_UI::DataViz
#endif

namespace RC_UI::DataViz {

// ---------------------------------------------------------------------------
// Spec structs — value-type, no heap, pass by const-ref
// ---------------------------------------------------------------------------

struct BarChartSpec {
    const char*        id;           // ImGui PushID key (must be non-null)
    const float*       values;       // data array
    const char* const* labels;       // per-bar axis labels (nullable)
    int                count;        // length of values[] and labels[]
    float              width;        // total chart width; 0 = ContentRegionAvail.x
    float              height;       // chart area height (bars only, not axis text)
    ImU32              bar_color;    // fill color; default UITokens::CyanAccent
    ImU32              peak_color;   // highlight for the max-value bar; default UITokens::AmberGlow
    bool               show_values;  // overlay value text centered on bar
};

struct SparklineSpec {
    const char*  id;          // ImGui PushID key
    const float* values;      // data array (time-ordered, left→right)
    int          count;       // length of values[]
    float        width;       // 0 = ContentRegionAvail.x
    float        height;      // total pixel height of the sparkline
    ImU32        line_color;  // line/fill color; default UITokens::GreenHUD
    bool         fill_area;   // true = area-under-curve (D3 area chart variant)
};

struct DonutSpec {
    const char*        id;           // ImGui PushID key
    const float*       values;       // slice magnitudes (any positive scale)
    const ImU32*       colors;       // per-slice colors (nullable → auto palette)
    const char* const* labels;       // per-slice labels (nullable)
    int                count;        // number of slices
    float              radius;       // outer radius in pixels
    float              inner_ratio;  // 0 = solid pie; 0.4–0.6 = donut hole
    bool               show_labels;  // render label text outside the ring
};

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace detail {

// Four-color auto-palette cycling when caller provides no colors array.
inline ImU32 AutoSliceColor(int i) {
    constexpr ImU32 kPalette[4] = {
        UITokens::CyanAccent,
        UITokens::AmberGlow,
        UITokens::GreenHUD,
        UITokens::InfoBlue,
    };
    return kPalette[i % 4];
}

} // namespace detail

// ---------------------------------------------------------------------------
// DrawBarChart
// ---------------------------------------------------------------------------
inline void DrawBarChart(const BarChartSpec& spec) {
    if (!spec.values || spec.count <= 0)
        return;

    ImGui::PushID(spec.id);

    ImDrawList* draw   = ImGui::GetWindowDrawList();
    const ImVec2 cursor = ImGui::GetCursorScreenPos();

    const float total_w = (spec.width > 0.0f)
                              ? spec.width
                              : ImGui::GetContentRegionAvail().x;
    const float gap     = 4.0f;
    const float bar_w   = (total_w - gap * static_cast<float>(spec.count - 1))
                          / static_cast<float>(spec.count);
    const float chart_h = spec.height;

    // Find max value for normalisation
    float max_val = 1e-6f;
    for (int i = 0; i < spec.count; ++i)
        max_val = std::max(max_val, spec.values[i]);

    // Find peak bar index
    int peak_idx = 0;
    for (int i = 1; i < spec.count; ++i)
        if (spec.values[i] > spec.values[peak_idx])
            peak_idx = i;

    const float font_size = ImGui::GetFontSize();

    for (int i = 0; i < spec.count; ++i) {
        const float x0 = cursor.x + static_cast<float>(i) * (bar_w + gap);
        const float fill_h = chart_h * std::clamp(spec.values[i] / max_val, 0.0f, 1.0f);

        // Track (background)
        draw->AddRectFilled(
            ImVec2(x0, cursor.y),
            ImVec2(x0 + bar_w, cursor.y + chart_h),
            WithAlpha(UITokens::BackgroundDark, 200),
            UITokens::RoundingSubtle);

        if (fill_h > 0.0f) {
            const ImU32 fill_color = (i == peak_idx) ? spec.peak_color : spec.bar_color;
            const float y_top = cursor.y + (chart_h - fill_h);

            // Fill
            draw->AddRectFilled(
                ImVec2(x0, y_top),
                ImVec2(x0 + bar_w, cursor.y + chart_h),
                fill_color,
                UITokens::RoundingSubtle);

            // Peak bar glow
            if (i == peak_idx) {
                draw->AddRect(
                    ImVec2(x0, y_top),
                    ImVec2(x0 + bar_w, cursor.y + chart_h),
                    WithAlpha(spec.peak_color, 160),
                    UITokens::RoundingSubtle,
                    0,
                    UITokens::BorderThin);
            }

            // Value text centered on bar
            if (spec.show_values) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%.0f", spec.values[i]);
                const ImVec2 tsz = ImGui::CalcTextSize(buf);
                const float tx   = x0 + (bar_w - tsz.x) * 0.5f;
                const float ty   = y_top - tsz.y - 2.0f;
                if (ty >= cursor.y)
                    draw->AddText(ImVec2(tx, ty), UITokens::TextPrimary, buf);
            }
        }

        // Axis label below bar
        if (spec.labels && spec.labels[i]) {
            const ImVec2 tsz = ImGui::CalcTextSize(spec.labels[i]);
            const float tx   = x0 + (bar_w - tsz.x) * 0.5f;
            draw->AddText(
                ImVec2(tx, cursor.y + chart_h + 3.0f),
                UITokens::TextSecondary,
                spec.labels[i]);
        }

        // TODO(interactivity): IsMouseHoveringRect per-bar → SetTooltip(label + value)
    }

    // Claim layout space (bars + optional axis label row)
    const float label_h = (spec.labels) ? (font_size + 3.0f) : 0.0f;
    ImGui::Dummy(ImVec2(total_w, chart_h + label_h));

    ImGui::PopID();
}

// ---------------------------------------------------------------------------
// DrawSparkline
// ---------------------------------------------------------------------------
inline void DrawSparkline(const SparklineSpec& spec) {
    if (!spec.values || spec.count < 2)
        return;

    ImGui::PushID(spec.id);

    ImDrawList* draw    = ImGui::GetWindowDrawList();
    const ImVec2 cursor = ImGui::GetCursorScreenPos();

    const float total_w = (spec.width > 0.0f)
                              ? spec.width
                              : ImGui::GetContentRegionAvail().x;
    const float total_h = spec.height;

    // Normalise: find min/max
    float min_val = spec.values[0];
    float max_val = spec.values[0];
    for (int i = 1; i < spec.count; ++i) {
        min_val = std::min(min_val, spec.values[i]);
        max_val = std::max(max_val, spec.values[i]);
    }
    const float range = std::max(max_val - min_val, 1e-6f);

    // Build screen-space point list (stack buffer; cap at 512 samples)
    constexpr int kMaxPts = 512;
    const int n = std::min(spec.count, kMaxPts);
    ImVec2 pts[kMaxPts];

    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n - 1);
        const float norm = (spec.values[i] - min_val) / range;
        pts[i] = ImVec2(
            cursor.x + t * total_w,
            cursor.y + total_h - norm * total_h);
    }

    // Area fill (D3 area chart)
    if (spec.fill_area) {
        // Build closed path: points L→R then bottom-right → bottom-left
        draw->PathClear();
        for (int i = 0; i < n; ++i)
            draw->PathLineTo(pts[i]);
        draw->PathLineTo(ImVec2(pts[n - 1].x, cursor.y + total_h)); // bottom-right
        draw->PathLineTo(ImVec2(pts[0].x,     cursor.y + total_h)); // bottom-left
        draw->PathFillConvex(WithAlpha(spec.line_color, 40));
    }

    // Line
    draw->AddPolyline(pts, n, spec.line_color, ImDrawFlags_None, 2.0f);

    // Dot at last (most recent) value
    draw->AddCircleFilled(pts[n - 1], 3.0f, spec.line_color, 8);

    // Claim layout space
    ImGui::Dummy(ImVec2(total_w, total_h));

    // TODO(interactivity): hover crosshair — find nearest x-index, draw vertical
    //   line and SetTooltip with value + index label.

    ImGui::PopID();
}

// ---------------------------------------------------------------------------
// DrawDonut
// ---------------------------------------------------------------------------
inline void DrawDonut(const DonutSpec& spec) {
    if (!spec.values || spec.count <= 0)
        return;

    // Guard zero-sum
    float total = 0.0f;
    for (int i = 0; i < spec.count; ++i)
        total += std::max(0.0f, spec.values[i]);
    if (total < 1e-6f) {
        ImGui::Dummy(ImVec2(spec.radius * 2.0f, spec.radius * 2.0f));
        return;
    }

    ImGui::PushID(spec.id);

    const float diam   = spec.radius * 2.0f;
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    const ImVec2 center(cursor.x + spec.radius, cursor.y + spec.radius);

    // Claim layout space first so ImGui registers the bounding rect
    ImGui::Dummy(ImVec2(diam, diam));

    ImDrawList* draw       = ImGui::GetWindowDrawList();
    const float inner_r    = spec.radius * std::clamp(spec.inner_ratio, 0.0f, 0.95f);
    const float label_r    = spec.radius * 1.15f; // label arc midpoint distance

    constexpr int   kArcSegments = 48;
    constexpr float kTwoPi       = 6.28318530718f;
    constexpr float kHalfPi      = 1.57079632679f;

    float angle = -kHalfPi; // start at 12-o'clock (matches D3 default)

    for (int i = 0; i < spec.count; ++i) {
        if (spec.values[i] <= 0.0f)
            continue;

        const float slice_angle = (spec.values[i] / total) * kTwoPi;
        const float a_start     = angle;
        const float a_end       = angle + slice_angle;
        const float a_mid       = (a_start + a_end) * 0.5f;

        const ImU32 fill_color = spec.colors ? spec.colors[i] : detail::AutoSliceColor(i);

        // Build outer arc as a filled polygon using PathArcTo
        draw->PathClear();
        draw->PathArcTo(center, spec.radius, a_start, a_end, kArcSegments);
        if (inner_r > 0.0f) {
            // Donut: trace inner arc backwards to close the ring
            draw->PathArcTo(center, inner_r, a_end, a_start, kArcSegments);
        } else {
            // Pie: close at center
            draw->PathLineTo(center);
        }
        draw->PathFillConvex(fill_color);

        // Thin separator between slices
        const float sx = center.x + std::cos(a_end) * spec.radius;
        const float sy = center.y + std::sin(a_end) * spec.radius;
        const float ix = center.x + std::cos(a_end) * inner_r;
        const float iy = center.y + std::sin(a_end) * inner_r;
        draw->AddLine(
            ImVec2(ix, iy), ImVec2(sx, sy),
            WithAlpha(UITokens::BackgroundDark, 200),
            UITokens::BorderThin);

        // Optional label positioned at arc midpoint outside ring
        if (spec.show_labels && spec.labels && spec.labels[i]) {
            const float lx = center.x + std::cos(a_mid) * label_r;
            const float ly = center.y + std::sin(a_mid) * label_r;
            const ImVec2 tsz = ImGui::CalcTextSize(spec.labels[i]);
            draw->AddText(
                ImVec2(lx - tsz.x * 0.5f, ly - tsz.y * 0.5f),
                UITokens::TextSecondary,
                spec.labels[i]);
        }

        // TODO(interactivity): approximate hover per slice using angular test on
        //   mouse position relative to center — IsMouseHoveringRect won't work for
        //   arcs. Compute atan2(mouse-center) → check a_start ≤ angle < a_end,
        //   then SetTooltip(label + percentage).

        angle = a_end;
    }

    // Donut hole background (overdraw the center circle)
    if (inner_r > 0.0f) {
        draw->AddCircleFilled(center, inner_r - 1.0f, UITokens::PanelBackground, kArcSegments);
    }

    ImGui::PopID();
}

// ---------------------------------------------------------------------------
// Force-directed graph
// ---------------------------------------------------------------------------

/// FDNode — one simulation particle.
/// fx/fy: set to a finite value to pin the node, NaN = free to move.
struct FDNode {
    float       x  = 0.0f;
    float       y  = 0.0f;
    float       vx = 0.0f;
    float       vy = 0.0f;
    float       fx = std::numeric_limits<float>::quiet_NaN(); // NaN = not fixed
    float       fy = std::numeric_limits<float>::quiet_NaN();
    int         group = 0;    // color group index (auto-palette by group % 4)
    const char* id    = nullptr; // tooltip label (nullable)
};

/// FDLink — directed edge (treated as undirected for physics).
struct FDLink {
    int   source = 0;
    int   target = 0;
    float value  = 1.0f; // stroke-width = sqrt(value), matches D3 convention
};

/// ForceGraphSpec — simulation parameters + mutable node/link arrays.
/// The caller owns both arrays and must keep them alive for the lifetime of
/// any StepForceGraph / DrawForceGraph calls.
struct ForceGraphSpec {
    const char* id;           // ImGui PushID key
    FDNode*     nodes;        // mutable — simulation writes x,y,vx,vy
    FDLink*     links;        // read-only after init
    int         node_count;
    int         link_count;
    float       width;        // canvas pixel width  (0 = ContentRegionAvail.x)
    float       height;       // canvas pixel height
    float       charge;       // many-body repulsion strength, e.g. -30
    float       link_distance;   // spring rest length, e.g. 30
    float       center_strength; // pull toward canvas center, e.g. 0.1
    float       alpha;           // simulation heat; starts at 1, cools toward 0
    float       alpha_decay;     // cooling per tick, D3 default ≈ 0.0228
    float       velocity_decay;  // friction: multiply vx/vy by (1-decay)/tick, e.g. 0.4
};

/// StepForceGraph — advance the simulation by `iterations` ticks.
/// Call once per frame (typically iterations=3–6 for fast layout convergence).
/// When alpha < 0.001 the simulation has cooled — no further work is done.
inline void StepForceGraph(ForceGraphSpec& spec, int iterations = 3) {
    const float cx = spec.width  * 0.5f;
    const float cy = spec.height * 0.5f;

    for (int iter = 0; iter < iterations; ++iter) {
        if (spec.alpha < 0.001f)
            break;

        // Cool alpha toward zero
        spec.alpha += (0.0f - spec.alpha) * spec.alpha_decay;

        // ── Many-body repulsion (O(n²), fine for n < 200) ─────────────────
        for (int i = 0; i < spec.node_count; ++i) {
            for (int j = i + 1; j < spec.node_count; ++j) {
                float dx = spec.nodes[j].x - spec.nodes[i].x;
                float dy = spec.nodes[j].y - spec.nodes[i].y;
                float d2 = dx * dx + dy * dy;
                if (d2 < 1.0f) d2 = 1.0f;
                const float f = spec.charge * spec.alpha / d2;
                const float fx = f * dx;
                const float fy = f * dy;
                spec.nodes[i].vx -= fx;
                spec.nodes[i].vy -= fy;
                spec.nodes[j].vx += fx;
                spec.nodes[j].vy += fy;
            }
        }

        // ── Link force (spring toward rest length) ─────────────────────────
        for (int li = 0; li < spec.link_count; ++li) {
            auto& lk  = spec.links[li];
            auto& src = spec.nodes[lk.source];
            auto& tgt = spec.nodes[lk.target];
            float dx = tgt.x - src.x + tgt.vx - src.vx;
            float dy = tgt.y - src.y + tgt.vy - src.vy;
            float d  = std::sqrt(dx * dx + dy * dy);
            if (d < 1e-6f)
                continue;
            d = (d - spec.link_distance) / d * spec.alpha * 0.5f;
            dx *= d;
            dy *= d;
            src.vx += dx;
            src.vy += dy;
            tgt.vx -= dx;
            tgt.vy -= dy;
        }

        // ── Center force ───────────────────────────────────────────────────
        const float cs = spec.center_strength * spec.alpha;
        for (int i = 0; i < spec.node_count; ++i) {
            spec.nodes[i].vx += (cx - spec.nodes[i].x) * cs;
            spec.nodes[i].vy += (cy - spec.nodes[i].y) * cs;
        }

        // ── Velocity integration ───────────────────────────────────────────
        const float friction = 1.0f - spec.velocity_decay;
        for (int i = 0; i < spec.node_count; ++i) {
            auto& n  = spec.nodes[i];
            n.vx *= friction;
            n.vy *= friction;
            if (!std::isnan(n.fx)) {
                n.x  = n.fx;
                n.vx = 0.0f;
            } else {
                n.x += n.vx;
            }
            if (!std::isnan(n.fy)) {
                n.y  = n.fy;
                n.vy = 0.0f;
            } else {
                n.y += n.vy;
            }
        }
    }
}

/// DrawForceGraph — render the current simulation state and handle node drag.
/// Call StepForceGraph each frame before DrawForceGraph to animate.
/// Node positions are in canvas-local space [0, spec.width] x [0, spec.height].
inline void DrawForceGraph(ForceGraphSpec& spec) {
    ImGui::PushID(spec.id);

    const float W = (spec.width > 0.0f)
                        ? spec.width
                        : ImGui::GetContentRegionAvail().x;
    const float H = spec.height;

    ImDrawList* draw    = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    // Canvas background
    draw->AddRectFilled(
        origin, ImVec2(origin.x + W, origin.y + H),
        WithAlpha(UITokens::BackgroundDark, 220),
        UITokens::RoundingSubtle);
    draw->AddRect(
        origin, ImVec2(origin.x + W, origin.y + H),
        WithAlpha(UITokens::CyanAccent, 60),
        UITokens::RoundingSubtle,
        0, UITokens::BorderThin);

    // Claim canvas as interactive region — allows mouse hit-testing below
    ImGui::InvisibleButton("##fdg_canvas", ImVec2(W, H));
    const bool canvas_hovered = ImGui::IsItemHovered();
    const bool canvas_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

    // Per-instance drag state keyed on the spec pointer
    static const ForceGraphSpec* s_drag_owner  = nullptr;
    static int                   s_dragged_node = -1;

    // Pick node on click
    if (canvas_clicked) {
        const ImVec2 mp = ImGui::GetMousePos();
        constexpr float kHitR2 = 64.0f; // 8px hit radius squared
        for (int i = 0; i < spec.node_count; ++i) {
            float dx = mp.x - (origin.x + spec.nodes[i].x);
            float dy = mp.y - (origin.y + spec.nodes[i].y);
            if (dx * dx + dy * dy < kHitR2) {
                s_drag_owner   = &spec;
                s_dragged_node = i;
                spec.nodes[i].fx = spec.nodes[i].x;
                spec.nodes[i].fy = spec.nodes[i].y;
                spec.alpha = std::max(spec.alpha, 0.3f); // reheat on drag start
                break;
            }
        }
    }

    // Update or release drag
    if (s_drag_owner == &spec && s_dragged_node >= 0) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const ImVec2 mp = ImGui::GetMousePos();
            float px = std::clamp(mp.x - origin.x, 0.0f, W);
            float py = std::clamp(mp.y - origin.y, 0.0f, H);
            spec.nodes[s_dragged_node].fx = px;
            spec.nodes[s_dragged_node].fy = py;
            spec.nodes[s_dragged_node].x  = px;
            spec.nodes[s_dragged_node].y  = py;
        } else {
            // Release: unfix so simulation takes over again
            spec.nodes[s_dragged_node].fx = std::numeric_limits<float>::quiet_NaN();
            spec.nodes[s_dragged_node].fy = std::numeric_limits<float>::quiet_NaN();
            spec.alpha     = std::max(spec.alpha, 0.0f);
            s_drag_owner   = nullptr;
            s_dragged_node = -1;
        }
    }

    // ── Draw links ────────────────────────────────────────────────────────────
    for (int li = 0; li < spec.link_count; ++li) {
        const auto& lk = spec.links[li];
        const ImVec2 p1(origin.x + spec.nodes[lk.source].x,
                        origin.y + spec.nodes[lk.source].y);
        const ImVec2 p2(origin.x + spec.nodes[lk.target].x,
                        origin.y + spec.nodes[lk.target].y);
        const float sw = std::sqrt(std::max(1.0f, lk.value));
        draw->AddLine(p1, p2, WithAlpha(UITokens::TextDisabled, 140), sw);
    }

    // ── Draw nodes ────────────────────────────────────────────────────────────
    constexpr float kNodeR = 5.0f;
    const ImVec2 mp = ImGui::GetMousePos();
    for (int i = 0; i < spec.node_count; ++i) {
        const ImVec2 nc(origin.x + spec.nodes[i].x,
                        origin.y + spec.nodes[i].y);
        const ImU32  fill = detail::AutoSliceColor(spec.nodes[i].group);
        const bool   pinned = (s_drag_owner == &spec && s_dragged_node == i);

        draw->AddCircleFilled(nc, kNodeR, fill, 16);
        draw->AddCircle(nc, kNodeR,
                        pinned ? UITokens::AmberGlow : UITokens::TextPrimary,
                        16, pinned ? 2.0f : 1.5f);

        // Tooltip on hover
        if (canvas_hovered && spec.nodes[i].id) {
            float dx = mp.x - nc.x;
            float dy = mp.y - nc.y;
            if (dx * dx + dy * dy < (kNodeR + 3.0f) * (kNodeR + 3.0f))
                ImGui::SetTooltip("%s", spec.nodes[i].id);
        }
    }

    ImGui::PopID();
}

// ---------------------------------------------------------------------------
// Drag-collision simulation
// Mirrors D3's forceCollide — circles with positional collision resolution
// and drag-to-push interaction.  No charge/link forces needed.
// ---------------------------------------------------------------------------

/// CollisionNode — one particle in the collision simulation.
struct CollisionNode {
    float       x  = 0.0f;
    float       y  = 0.0f;
    float       vx = 0.0f;
    float       vy = 0.0f;
    float       fx = std::numeric_limits<float>::quiet_NaN(); // NaN = free
    float       fy = std::numeric_limits<float>::quiet_NaN();
    float       radius = 10.0f; // display + collision radius
    int         group  = 0;     // color group (auto-palette by group % 4)
};

/// CollisionSpec — simulation parameters for the drag-collision demo.
/// The caller owns `nodes` and must keep it alive.
struct CollisionSpec {
    const char*    id;
    CollisionNode* nodes;
    int            node_count;
    float          width;        // canvas pixel width  (0 = ContentRegionAvail.x)
    float          height;       // canvas pixel height
    float          alpha;        // simulation heat; cools toward alpha_target
    float          alpha_target; // 0 = cool; set to 0.3 during drag (D3 alphaTarget)
    float          alpha_decay;  // cooling rate ≈ 0.0228 (D3 default)
    float          velocity_decay; // friction per tick, e.g. 0.4
    int            collision_iterations; // constraint passes per tick, D3 default = 4
};

/// StepCollision — advance the collision simulation by `ticks` ticks.
/// Implements D3's forceCollide: anticipatory velocity-based separation.
/// O(n²) per collision iteration — suitable for n < 500.
inline void StepCollision(CollisionSpec& spec, int ticks = 1) {
    for (int t = 0; t < ticks; ++t) {
        // Always run at least one tick while dragging (alpha_target > 0)
        if (spec.alpha < 0.001f && spec.alpha_target < 0.001f)
            break;

        // Cool alpha toward alpha_target (decays up or down)
        spec.alpha += (spec.alpha_target - spec.alpha) * spec.alpha_decay;

        // ── Collision force (D3 forceCollide, velocity-based) ──────────────
        // Uses anticipated positions (x+vx, y+vy) like D3.
        for (int ci = 0; ci < spec.collision_iterations; ++ci) {
            for (int i = 0; i < spec.node_count; ++i) {
                auto& ni = spec.nodes[i];
                for (int j = i + 1; j < spec.node_count; ++j) {
                    auto& nj = spec.nodes[j];
                    // Vector from nj → ni (anticipated)
                    float dx = ni.x + ni.vx - nj.x - nj.vx;
                    float dy = ni.y + ni.vy - nj.y - nj.vy;
                    float d2 = dx * dx + dy * dy;
                    const float min_d = ni.radius + nj.radius;
                    if (d2 >= min_d * min_d)
                        continue;

                    // Jiggle when perfectly coincident
                    if (d2 < 1e-10f) { dx = 1e-4f; dy = 1e-4f; d2 = 2e-8f; }
                    const float d = std::sqrt(d2);

                    // Separation impulse scaled by alpha, split equally by mass
                    const float l = (min_d - d) / d * spec.alpha * 0.5f;
                    const float px = dx * l;
                    const float py = dy * l;

                    ni.vx += px;
                    ni.vy += py;
                    nj.vx -= px;
                    nj.vy -= py;
                }
            }
        }

        // ── Velocity integration + wall boundary ──────────────────────────
        const float friction = 1.0f - spec.velocity_decay;
        const float W = spec.width;
        const float H = spec.height;
        for (int i = 0; i < spec.node_count; ++i) {
            auto& n = spec.nodes[i];
            n.vx *= friction;
            n.vy *= friction;
            if (!std::isnan(n.fx)) {
                n.x  = n.fx;
                n.vx = 0.0f;
            } else {
                n.x += n.vx;
            }
            if (!std::isnan(n.fy)) {
                n.y  = n.fy;
                n.vy = 0.0f;
            } else {
                n.y += n.vy;
            }
            // Soft wall — keep circles inside canvas
            n.x = std::clamp(n.x, n.radius, W - n.radius);
            n.y = std::clamp(n.y, n.radius, H - n.radius);
        }
    }
}

/// DrawCollision — render collision particles and handle drag-to-push.
/// Drag any circle to push the cloud; release to let simulation cool.
/// Call StepCollision each frame before DrawCollision.
inline void DrawCollision(CollisionSpec& spec) {
    ImGui::PushID(spec.id);

    const float W = (spec.width > 0.0f)
                        ? spec.width
                        : ImGui::GetContentRegionAvail().x;
    const float H = spec.height;

    ImDrawList*  draw   = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    // Canvas background
    draw->AddRectFilled(
        origin, ImVec2(origin.x + W, origin.y + H),
        WithAlpha(UITokens::BackgroundDark, 220),
        UITokens::RoundingSubtle);
    draw->AddRect(
        origin, ImVec2(origin.x + W, origin.y + H),
        WithAlpha(UITokens::CyanAccent, 60),
        UITokens::RoundingSubtle, 0, UITokens::BorderThin);

    // Claim interactive canvas region
    ImGui::InvisibleButton("##col_canvas", ImVec2(W, H));
    (void)ImGui::IsItemHovered(); // reserved for future drag-over highlight
    const bool canvas_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

    // Per-instance drag state
    static const CollisionSpec* s_drag_owner  = nullptr;
    static int                  s_dragged_node = -1;

    // Pick nearest node within its radius on click
    if (canvas_clicked) {
        const ImVec2 mp  = ImGui::GetMousePos();
        const float  mx  = mp.x - origin.x;
        const float  my  = mp.y - origin.y;
        float best_d2    = std::numeric_limits<float>::max();
        int   best_i     = -1;

        for (int i = 0; i < spec.node_count; ++i) {
            float dx = mx - spec.nodes[i].x;
            float dy = my - spec.nodes[i].y;
            float d2 = dx * dx + dy * dy;
            float r  = spec.nodes[i].radius;
            if (d2 < r * r && d2 < best_d2) {
                best_d2 = d2;
                best_i  = i;
            }
        }

        if (best_i >= 0) {
            s_drag_owner   = &spec;
            s_dragged_node = best_i;
            spec.nodes[best_i].fx = spec.nodes[best_i].x;
            spec.nodes[best_i].fy = spec.nodes[best_i].y;
            spec.alpha_target = 0.3f;           // D3: alphaTarget(0.3).restart()
            spec.alpha = std::max(spec.alpha, 0.3f);
        }
    }

    if (s_drag_owner == &spec && s_dragged_node >= 0) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const ImVec2 mp = ImGui::GetMousePos();
            float r  = spec.nodes[s_dragged_node].radius;
            float px = std::clamp(mp.x - origin.x, r, W - r);
            float py = std::clamp(mp.y - origin.y, r, H - r);
            spec.nodes[s_dragged_node].fx = px;
            spec.nodes[s_dragged_node].fy = py;
            spec.nodes[s_dragged_node].x  = px;
            spec.nodes[s_dragged_node].y  = py;
        } else {
            // Release — unfix and cool simulation (D3: alphaTarget(0))
            spec.nodes[s_dragged_node].fx = std::numeric_limits<float>::quiet_NaN();
            spec.nodes[s_dragged_node].fy = std::numeric_limits<float>::quiet_NaN();
            spec.alpha_target = 0.0f;
            s_drag_owner      = nullptr;
            s_dragged_node    = -1;
        }
    }

    // ── Render circles ────────────────────────────────────────────────────────
    for (int i = 0; i < spec.node_count; ++i) {
        const auto&  n    = spec.nodes[i];
        const ImVec2 c(origin.x + n.x, origin.y + n.y);
        const ImU32  fill = detail::AutoSliceColor(n.group);
        const bool   pinned = (s_drag_owner == &spec && s_dragged_node == i);

        draw->AddCircleFilled(c, n.radius, fill, 20);
        // White border (D3: context.strokeStyle = "#fff")
        draw->AddCircle(c, n.radius,
                        pinned ? UITokens::AmberGlow : WithAlpha(UITokens::TextPrimary, 180),
                        20, pinned ? 2.0f : 1.0f);
    }

    ImGui::PopID();
}

// ---------------------------------------------------------------------------
// Smooth zoom — phyllotaxis spiral + interpolateZoom-style camera animation.
// Mirrors D3's "Smooth Zooming" example:
//   - 2000-point sunflower spiral (golden-angle phyllotaxis)
//   - Per-point rainbow color (hue cycled by index)
//   - Auto-transition: pick a random point, zoom smoothly to it, repeat
//   - Transform: translate(W/2, H/2) scale(H/vr) translate(-vx, -vy)
// ---------------------------------------------------------------------------

/// One point in the phyllotaxis layout.
struct ZoomPoint {
    float x    = 0.0f;
    float y    = 0.0f;
    ImU32 color = 0xFFFFFFFF;
};

/// SmoothZoomSpec — all state for the animated zoom camera.
/// Caller allocates `points[count]`; call InitSmoothZoom() once to populate.
struct SmoothZoomSpec {
    const char* id;
    ZoomPoint*  points;    // phyllotaxis array (owner-allocated)
    int         count;     // number of points (e.g. 2000)
    float       canvas_w;  // pixel canvas width  (0 = ContentRegionAvail.x)
    float       canvas_h;  // pixel canvas height
    float       point_r;   // display radius per point

    // Current view state — interpolated each frame by StepSmoothZoom
    float vx = 0.0f, vy = 0.0f, vr = 1.0f; // center + view-radius

    // Transition source / destination
    float src_vx = 0.0f, src_vy = 0.0f, src_vr = 1.0f;
    float dst_vx = 0.0f, dst_vy = 0.0f, dst_vr = 1.0f;

    float t        = 1.0f; // [0,1] — 1 = done, triggers next target pick
    float duration = 2.0f; // transition length in seconds
    float delay    = 0.25f;// pre-transition pause (D3's .delay(250))
    int   seed     = 42;   // LCG state for deterministic target selection
};

namespace detail {

/// Smoothstep easing (cubic).
inline float SmoothStep(float t) { return t * t * (3.0f - 2.0f * t); }

/// LCG next-int in [0, count).
inline int LcgNext(int& seed, int count) {
    seed = (seed * 1664525 + 1013904223) & 0x7FFFFFFF;
    return seed % count;
}

} // namespace detail

/// InitSmoothZoom — populate phyllotaxis point positions and colors.
/// Call once after allocating spec.points[spec.count].
/// cx, cy: logical canvas center (typically canvas_w/2, canvas_h/2).
/// step: radial spacing between rings, D3 default = 12.
inline void InitSmoothZoom(SmoothZoomSpec& spec, float cx, float cy,
                           float step = 12.0f) {
    constexpr float kTheta = 2.399963229728653f; // π(3 − √5), golden angle

    for (int i = 0; i < spec.count; ++i) {
        const float fi = static_cast<float>(i) + 0.5f;
        const float r  = step * std::sqrt(fi);
        const float a  = kTheta * fi;
        spec.points[i].x = cx + r * std::cos(a);
        spec.points[i].y = cy + r * std::sin(a);

        // Rainbow hue: cycles every 360 points (matches d3.interpolateRainbow)
        const float hue = std::fmod(static_cast<float>(i) / 360.0f, 1.0f);
        float R, G, B;
        ImGui::ColorConvertHSVtoRGB(hue, 0.85f, 1.0f, R, G, B);
        spec.points[i].color = IM_COL32(
            static_cast<int>(R * 255.0f),
            static_cast<int>(G * 255.0f),
            static_cast<int>(B * 255.0f),
            220);
    }

    // Initial view: whole canvas (scale = 1)
    spec.vx  = cx;  spec.vy  = cy;  spec.vr  = spec.canvas_h;
    spec.src_vx = cx; spec.src_vy = cy; spec.src_vr = spec.canvas_h;
    spec.dst_vx = cx; spec.dst_vy = cy; spec.dst_vr = spec.canvas_h;
    spec.t     = 1.0f;  // trigger first transition immediately
    spec.delay = 0.0f;
}

/// StepSmoothZoom — advance the zoom animation by dt seconds.
/// Call once per frame before DrawSmoothZoom.
inline void StepSmoothZoom(SmoothZoomSpec& spec, float dt) {
    if (spec.delay > 0.0f) {
        spec.delay -= dt;
        return;
    }

    if (spec.t < 1.0f) {
        // Advance interpolation
        spec.t = std::min(1.0f, spec.t + dt / std::max(spec.duration, 0.01f));
        const float te = detail::SmoothStep(spec.t);

        // Linear position interpolation
        spec.vx = spec.src_vx + (spec.dst_vx - spec.src_vx) * te;
        spec.vy = spec.src_vy + (spec.dst_vy - spec.src_vy) * te;

        // Log-linear radius interpolation → smooth exponential zoom feel
        const float log_r = std::log(std::max(spec.src_vr, 1e-4f))
                          + (std::log(std::max(spec.dst_vr, 1e-4f))
                           - std::log(std::max(spec.src_vr, 1e-4f))) * te;
        spec.vr = std::exp(log_r);

    } else {
        // Transition complete — freeze for delay, then pick next target
        spec.delay = 0.25f; // D3's .delay(250)

        spec.src_vx = spec.vx;
        spec.src_vy = spec.vy;
        spec.src_vr = spec.vr;

        const int idx = detail::LcgNext(spec.seed, spec.count);
        spec.dst_vx = spec.points[idx].x;
        spec.dst_vy = spec.points[idx].y;
        spec.dst_vr = spec.point_r * 2.0f + 1.0f; // D3: [...d, radius*2+1]

        // Duration scaled by distance (approximates D3's i.duration from interpolateZoom)
        const float dx   = spec.dst_vx - spec.src_vx;
        const float dy   = spec.dst_vy - spec.src_vy;
        const float dist = std::sqrt(dx * dx + dy * dy);
        spec.duration = 1.5f + dist * 0.004f; // 1.5–4 s
        spec.t = 0.0f;
    }
}

/// DrawSmoothZoom — render the current frame of the animated zoom.
/// Pure display — does not modify spec state (call StepSmoothZoom first).
inline void DrawSmoothZoom(const SmoothZoomSpec& spec) {
    ImGui::PushID(spec.id);

    const float W = (spec.canvas_w > 0.0f)
                        ? spec.canvas_w
                        : ImGui::GetContentRegionAvail().x;
    const float H = spec.canvas_h;

    ImDrawList*  draw   = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    // Very dark background — D3 shows on white, we invert for Y2K aesthetic
    draw->AddRectFilled(origin, ImVec2(origin.x + W, origin.y + H),
                        IM_COL32(6, 6, 12, 255),
                        UITokens::RoundingSubtle);
    draw->AddRect(origin, ImVec2(origin.x + W, origin.y + H),
                  WithAlpha(UITokens::CyanAccent, 60),
                  UITokens::RoundingSubtle, 0, UITokens::BorderThin);

    // Claim layout space before custom draw
    ImGui::Dummy(ImVec2(W, H));

    // Camera transform: translate(W/2, H/2) scale(H/vr) translate(-vx, -vy)
    const float scale = H / std::max(spec.vr, 0.001f);
    const float tx    = origin.x + W * 0.5f - spec.vx * scale;
    const float ty    = origin.y + H * 0.5f - spec.vy * scale;

    // Clip to canvas bounds
    draw->PushClipRect(origin, ImVec2(origin.x + W, origin.y + H), true);

    const float pr     = spec.point_r * scale;
    const float pr_vis = std::max(pr, 1.0f); // minimum 1px so dots stay visible

    for (int i = 0; i < spec.count; ++i) {
        const float sx = spec.points[i].x * scale + tx;
        const float sy = spec.points[i].y * scale + ty;

        // Frustum cull — skip circles fully outside canvas
        if (sx + pr_vis < origin.x || sx - pr_vis > origin.x + W ||
            sy + pr_vis < origin.y || sy - pr_vis > origin.y + H)
            continue;

        // Fewer segments when small (perf)
        const int segs = pr_vis > 10.0f ? 20 : (pr_vis > 4.0f ? 14 : 8);
        draw->AddCircleFilled(ImVec2(sx, sy), pr_vis,
                              spec.points[i].color, segs);
    }

    draw->PopClipRect();

    ImGui::PopID();
}

// ---------------------------------------------------------------------------
// Easing animation primitives
// Mirrors D3's easing principles:
//   - Normalized time input t in [0,1]
//   - Easing remaps temporal progression to output y (not position directly)
//   - Same timeline can drive multiple motions for visual comparison
// ---------------------------------------------------------------------------

enum class EasingType {
    Linear = 0,
    PolyIn,
    PolyOut,
    PolyInOut,
    CubicInOut,
    SinInOut,
    ExpInOut,
    CircleInOut,
    BackInOut,
    ElasticOut,
    BounceOut,
};

struct EasingParams {
    // Used by Poly* curves (must be > 0).
    float poly_exponent    = 3.0f;
    // Used by BackInOut (controls overshoot amount).
    float back_overshoot   = 1.70158f;
    // Used by ElasticOut.
    float elastic_amplitude = 1.0f;
    float elastic_period    = 0.30f;
};

inline const char* EasingTypeName(EasingType easing) {
    switch (easing) {
        case EasingType::Linear:      return "Linear";
        case EasingType::PolyIn:      return "Poly In";
        case EasingType::PolyOut:     return "Poly Out";
        case EasingType::PolyInOut:   return "Poly InOut";
        case EasingType::CubicInOut:  return "Cubic InOut";
        case EasingType::SinInOut:    return "Sin InOut";
        case EasingType::ExpInOut:    return "Exp InOut";
        case EasingType::CircleInOut: return "Circle InOut";
        case EasingType::BackInOut:   return "Back InOut";
        case EasingType::ElasticOut:  return "Elastic Out";
        case EasingType::BounceOut:   return "Bounce Out";
    }
    return "Unknown";
}

namespace detail {

inline float EaseBounceOut(float t) {
    constexpr float n1 = 7.5625f;
    constexpr float d1 = 2.75f;

    if (t < 1.0f / d1)
        return n1 * t * t;
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

} // namespace detail

/// EvaluateEasing — map normalized time t->[0,1] through a selected easing.
/// Some curves (back/elastic) intentionally overshoot outside [0,1].
inline float EvaluateEasing(EasingType easing, float t,
                            const EasingParams& params = {}) {
    constexpr float kPi  = 3.14159265359f;
    constexpr float kTau = 6.28318530718f;

    t = std::clamp(t, 0.0f, 1.0f);
    if (t <= 0.0f)
        return 0.0f;
    if (t >= 1.0f)
        return 1.0f;

    switch (easing) {
        case EasingType::Linear:
            return t;
        case EasingType::PolyIn: {
            const float e = std::max(params.poly_exponent, 0.001f);
            return std::pow(t, e);
        }
        case EasingType::PolyOut: {
            const float e = std::max(params.poly_exponent, 0.001f);
            return 1.0f - std::pow(1.0f - t, e);
        }
        case EasingType::PolyInOut: {
            const float e = std::max(params.poly_exponent, 0.001f);
            if (t < 0.5f)
                return 0.5f * std::pow(2.0f * t, e);
            return 1.0f - 0.5f * std::pow(2.0f * (1.0f - t), e);
        }
        case EasingType::CubicInOut:
            if (t < 0.5f) {
                const float u = 2.0f * t;
                return 0.5f * u * u * u;
            } else {
                const float u = -2.0f * t + 2.0f;
                return 1.0f - 0.5f * u * u * u;
            }
        case EasingType::SinInOut:
            return -(std::cos(kPi * t) - 1.0f) * 0.5f;
        case EasingType::ExpInOut:
            if (t < 0.5f)
                return 0.5f * std::pow(2.0f, 20.0f * t - 10.0f);
            return 1.0f - 0.5f * std::pow(2.0f, -20.0f * t + 10.0f);
        case EasingType::CircleInOut:
            if (t < 0.5f)
                return (1.0f - std::sqrt(std::max(0.0f, 1.0f - 4.0f * t * t))) * 0.5f;
            {
                const float u = -2.0f * t + 2.0f;
                return (std::sqrt(std::max(0.0f, 1.0f - u * u)) + 1.0f) * 0.5f;
            }
        case EasingType::BackInOut: {
            const float s = std::max(params.back_overshoot, 0.0f) * 1.525f;
            if (t < 0.5f) {
                const float u = 2.0f * t;
                return 0.5f * (u * u * ((s + 1.0f) * u - s));
            }
            const float u = 2.0f * t - 2.0f;
            return 0.5f * (u * u * ((s + 1.0f) * u + s) + 2.0f);
        }
        case EasingType::ElasticOut: {
            const float amplitude = std::max(params.elastic_amplitude, 0.0f);
            const float period    = std::max(params.elastic_period, 0.01f);
            const float expo      = std::pow(2.0f, -10.0f * t);
            const float angle     = (t - period * 0.25f) * (kTau / period);
            return amplitude * expo * std::sin(angle) + 1.0f;
        }
        case EasingType::BounceOut:
            return detail::EaseBounceOut(t);
    }
    return t;
}

struct EasingCurveSpec {
    const char*   id = nullptr;      // ImGui PushID key
    EasingType    easing = EasingType::CubicInOut;
    EasingParams  params{};
    float         width = 0.0f;      // 0 = ContentRegionAvail.x
    float         height = 150.0f;
    float         t = 0.0f;          // marker time in [0,1]
    bool          auto_range = true; // true = fit y-range from sampled curve
    float         min_output = -0.25f;
    float         max_output = 1.25f;
    bool          show_linear_reference = true;
    ImU32         background_color = WithAlpha(UITokens::BackgroundDark, 220);
    ImU32         border_color     = WithAlpha(UITokens::CyanAccent, 60);
    ImU32         curve_color      = UITokens::CyanAccent;
    ImU32         reference_color  = WithAlpha(UITokens::TextDisabled, 160);
    ImU32         marker_color     = UITokens::AmberGlow;
};

/// DrawEasingCurve — chart y = ease(t) with optional linear reference + marker.
inline void DrawEasingCurve(const EasingCurveSpec& spec) {
    if (!spec.id)
        return;

    ImGui::PushID(spec.id);

    const float W = (spec.width > 0.0f)
                        ? spec.width
                        : ImGui::GetContentRegionAvail().x;
    const float H = spec.height;
    if (W <= 0.0f || H <= 0.0f) {
        ImGui::PopID();
        return;
    }

    ImDrawList*  draw   = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    draw->AddRectFilled(origin, ImVec2(origin.x + W, origin.y + H),
                        spec.background_color, UITokens::RoundingSubtle);
    draw->AddRect(origin, ImVec2(origin.x + W, origin.y + H),
                  spec.border_color, UITokens::RoundingSubtle, 0,
                  UITokens::BorderThin);

    // Claim layout before issuing custom draws.
    ImGui::Dummy(ImVec2(W, H));

    constexpr int kSamples = 96;
    float ys[kSamples + 1];

    float y_min = std::min(0.0f, 1.0f);
    float y_max = std::max(0.0f, 1.0f);

    for (int i = 0; i <= kSamples; ++i) {
        const float x = static_cast<float>(i) / static_cast<float>(kSamples);
        const float y = EvaluateEasing(spec.easing, x, spec.params);
        ys[i] = y;
        if (spec.auto_range) {
            y_min = std::min(y_min, y);
            y_max = std::max(y_max, y);
        }
    }

    if (spec.auto_range) {
        const float span = std::max(0.10f, y_max - y_min);
        const float pad  = span * 0.08f;
        y_min -= pad;
        y_max += pad;
    } else {
        y_min = std::min(spec.min_output, spec.max_output);
        y_max = std::max(spec.min_output, spec.max_output);
        if (y_max - y_min < 1e-4f)
            y_max = y_min + 1e-4f;
    }

    const float left_pad   = 10.0f;
    const float right_pad  = 10.0f;
    const float top_pad    = 8.0f;
    const float bottom_pad = 14.0f;
    const ImVec2 plot_min(origin.x + left_pad, origin.y + top_pad);
    const ImVec2 plot_max(origin.x + W - right_pad, origin.y + H - bottom_pad);
    const float plot_w = std::max(1.0f, plot_max.x - plot_min.x);
    const float plot_h = std::max(1.0f, plot_max.y - plot_min.y);
    const float y_span = std::max(1e-6f, y_max - y_min);

    const auto ToScreen = [&](float x, float y) -> ImVec2 {
        const float px = plot_min.x + std::clamp(x, 0.0f, 1.0f) * plot_w;
        const float ny = (y - y_min) / y_span;
        const float py = plot_max.y - ny * plot_h;
        return ImVec2(px, py);
    };

    // Guides at y=0 and y=1.
    draw->AddLine(ToScreen(0.0f, 0.0f), ToScreen(1.0f, 0.0f),
                  spec.reference_color, 1.0f);
    draw->AddLine(ToScreen(0.0f, 1.0f), ToScreen(1.0f, 1.0f),
                  WithAlpha(spec.reference_color, 96), 1.0f);

    // Optional linear reference: y=x.
    if (spec.show_linear_reference) {
        draw->AddLine(ToScreen(0.0f, 0.0f), ToScreen(1.0f, 1.0f),
                      WithAlpha(UITokens::InfoBlue, 150), 1.0f);
    }

    // Easing curve polyline.
    ImVec2 prev = ToScreen(0.0f, ys[0]);
    for (int i = 1; i <= kSamples; ++i) {
        const float x = static_cast<float>(i) / static_cast<float>(kSamples);
        const ImVec2 p = ToScreen(x, ys[i]);
        draw->AddLine(prev, p, spec.curve_color, 2.0f);
        prev = p;
    }

    // Animated marker.
    const float t  = std::clamp(spec.t, 0.0f, 1.0f);
    const float yt = EvaluateEasing(spec.easing, t, spec.params);
    draw->AddCircleFilled(ToScreen(t, yt), 4.0f, spec.marker_color, 16);
    draw->AddText(ImVec2(plot_min.x + 2.0f, plot_min.y + 2.0f),
                  UITokens::TextSecondary, EasingTypeName(spec.easing));

    ImGui::PopID();
}

struct EasingMotionSpec {
    const char*   id = nullptr;      // ImGui PushID key
    EasingType    easing = EasingType::CubicInOut;
    EasingParams  params{};
    float         width = 0.0f;      // 0 = ContentRegionAvail.x
    float         height = 36.0f;
    float         t = 0.0f;          // normalized timeline [0,1]
    const char*   label = nullptr;   // optional row label
    bool          show_linear_reference = true;
    float         marker_radius = 5.0f;
    ImU32         background_color = WithAlpha(UITokens::BackgroundDark, 190);
    ImU32         border_color     = WithAlpha(UITokens::CyanAccent, 45);
    ImU32         track_color      = WithAlpha(UITokens::TextDisabled, 140);
    ImU32         linear_color     = WithAlpha(UITokens::InfoBlue, 180);
    ImU32         eased_color      = UITokens::AmberGlow;
};

/// DrawEasingMotion — one horizontal motion track driven by an easing curve.
/// Linear ghost marker can be shown to contrast timing distortion.
inline void DrawEasingMotion(const EasingMotionSpec& spec) {
    if (!spec.id)
        return;

    ImGui::PushID(spec.id);

    const float W = (spec.width > 0.0f)
                        ? spec.width
                        : ImGui::GetContentRegionAvail().x;
    const float H = spec.height;
    if (W <= 0.0f || H <= 0.0f) {
        ImGui::PopID();
        return;
    }

    ImDrawList*  draw   = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    draw->AddRectFilled(origin, ImVec2(origin.x + W, origin.y + H),
                        spec.background_color, UITokens::RoundingSubtle);
    draw->AddRect(origin, ImVec2(origin.x + W, origin.y + H),
                  spec.border_color, UITokens::RoundingSubtle, 0,
                  UITokens::BorderThin);

    // Claim layout space before drawing custom content.
    ImGui::Dummy(ImVec2(W, H));

    const bool has_label = (spec.label && spec.label[0] != '\0');
    if (has_label) {
        draw->AddText(ImVec2(origin.x + 8.0f, origin.y + 3.0f),
                      UITokens::TextSecondary, spec.label);
    }

    const float x0 = origin.x + 10.0f;
    const float x1 = origin.x + W - 10.0f;
    const float y  = has_label ? (origin.y + H * 0.68f) : (origin.y + H * 0.50f);

    draw->AddLine(ImVec2(x0, y), ImVec2(x1, y), spec.track_color, 1.5f);
    draw->AddLine(ImVec2(x0, y - 4.0f), ImVec2(x0, y + 4.0f), spec.track_color, 1.0f);
    draw->AddLine(ImVec2(x1, y - 4.0f), ImVec2(x1, y + 4.0f), spec.track_color, 1.0f);

    const float t_linear = std::clamp(spec.t, 0.0f, 1.0f);
    const float t_eased  = EvaluateEasing(spec.easing, t_linear, spec.params);
    const float x_linear = x0 + (x1 - x0) * t_linear;
    const float x_eased  = x0 + (x1 - x0) * t_eased;

    if (spec.show_linear_reference) {
        draw->AddCircleFilled(ImVec2(x_linear, y), std::max(2.0f, spec.marker_radius - 2.0f),
                              spec.linear_color, 12);
    }
    draw->AddCircleFilled(ImVec2(x_eased, y), std::max(2.0f, spec.marker_radius),
                          spec.eased_color, 16);
    ImGui::PopID();
}

// ===========================================================================
// ImPlot-backed primitives  (requires ROGUECITY_HAS_IMPLOT=1)
// ===========================================================================
// Pan/zoom, crosshair, tooltip, legend — a richer complement to the inline
// custom-draw primitives above.  Each Draw* wraps BeginPlot/EndPlot.
// Call RC_UI::API::InitImPlot() / ShutdownImPlot() around the Dear ImGui
// context lifetime; see rc_imgui_api.h.
// ===========================================================================
#ifdef ROGUECITY_HAS_IMPLOT
// Multi-series line chart with optional area fill.
struct ImPlotTimeSeriesSpec {
    const char*         id;             // ImGui PushID key (unique per panel)
    const char*         title;          // plot title (nullptr = hidden)
    const float* const* ys;             // [series_count][value_count] arrays
    const char* const*  series_labels;  // per-series legend labels (nullable)
    const float*        xs;             // shared x-axis (nullptr = 0,1,2,...)
    int                 series_count;
    int                 value_count;
    float               width;          // 0 = ContentRegionAvail.x
    float               height;
    const ImU32*        colors;         // per-series color override (nullable)
    bool                fill_area;      // shade area under each line, alpha=0.2
};

// Single-series scatter plot.
struct ImPlotScatterSpec {
    const char*  id;
    const char*  title;
    const char*  label;     // series name in legend (nullable)
    const float* xs;
    const float* ys;
    int          count;
    float        width;
    float        height;
    ImU32        color;     // 0 = ImPlot auto-cycle
};

// 2-D heatmap — ideal for AESP spatial readouts and zoning density grids.
struct ImPlotHeatmapSpec {
    const char*     id;
    const char*     title;
    const char*     label;      // series label for colorbar tooltip
    const float*    values;     // row-major: values[row * cols + col]
    int             rows;
    int             cols;
    double          scale_min;
    double          scale_max;
    float           width;      // 0 = auto (leaves 70px for colorbar)
    float           height;
    ImPlotColormap  colormap;   // e.g. ImPlotColormap_Viridis
    const char*     label_fmt;  // per-cell annotation (nullptr = no labels)
};

// Histogram with auto-binning, density/cumulative modes.
struct ImPlotHistogramSpec {
    const char*  id;
    const char*  title;
    const char*  label;
    const float* values;
    int          count;
    int          bins;      // ImPlotBin_Auto (-1) for automatic binning
    float        width;
    float        height;
    bool         cumulative;
    bool         density;   // normalize to probability density
    ImU32        bar_color; // 0 = ImPlot auto-cycle
};

// ---------------------------------------------------------------------------
// Inline Draw functions — each is self-contained (PushID/BeginPlot/EndPlot).
// ---------------------------------------------------------------------------

inline void DrawImPlotTimeSeries(const ImPlotTimeSeriesSpec& spec) {
    if (!spec.ys || spec.series_count <= 0 || spec.value_count <= 0) return;
    ImGui::PushID(spec.id);
    const float w = (spec.width > 0.0f) ? spec.width : ImGui::GetContentRegionAvail().x;
    if (ImPlot::BeginPlot(spec.title ? spec.title : "##ts",
                          ImVec2(w, spec.height), ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes(nullptr, nullptr,
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        for (int s = 0; s < spec.series_count; ++s) {
            ImGui::PushID(s);
            // v0.18: per-call ImPlotSpec replaces PushStyleColor(ImPlotCol_Line/Fill)
            // and the obsoleted SetNextFillStyle()
            ImPlotSpec ispec;
            if (spec.colors && spec.colors[s]) {
                ImVec4 c = ImGui::ColorConvertU32ToFloat4(spec.colors[s]);
                ispec.LineColor = c;
                if (spec.fill_area) {
                    ispec.FillColor = c;
                    ispec.FillAlpha = 0.2f;
                }
            }
            if (spec.fill_area)
                ispec.Flags = ImPlotLineFlags_Shaded;
            const char* lbl = (spec.series_labels && spec.series_labels[s])
                                  ? spec.series_labels[s] : "##s";
            if (spec.xs)
                ImPlot::PlotLine(lbl, spec.xs, spec.ys[s], spec.value_count, ispec);
            else
                ImPlot::PlotLine(lbl, spec.ys[s], spec.value_count, 1.0, 0.0, ispec);
            ImGui::PopID();
        }
        ImPlot::EndPlot();
    }
    ImGui::PopID();
}

inline void DrawImPlotScatter(const ImPlotScatterSpec& spec) {
    if (!spec.xs || !spec.ys || spec.count <= 0) return;
    ImGui::PushID(spec.id);
    const float w = (spec.width > 0.0f) ? spec.width : ImGui::GetContentRegionAvail().x;
    if (ImPlot::BeginPlot(spec.title ? spec.title : "##sc",
                          ImVec2(w, spec.height), ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes(nullptr, nullptr,
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        // v0.18: ImPlotCol_MarkerFill/MarkerOutline removed; use ImPlotSpec fields
        ImPlotSpec ispec;
        if (spec.color) {
            ImVec4 c = ImGui::ColorConvertU32ToFloat4(spec.color);
            ispec.MarkerFillColor = c;
            ispec.MarkerLineColor = c;
        }
        ispec.Marker = ImPlotMarker_Circle;
        ImPlot::PlotScatter(spec.label ? spec.label : "##sc",
                            spec.xs, spec.ys, spec.count, ispec);
        ImPlot::EndPlot();
    }
    ImGui::PopID();
}

inline void DrawImPlotHeatmap(const ImPlotHeatmapSpec& spec) {
    if (!spec.values || spec.rows <= 0 || spec.cols <= 0) return;
    ImGui::PushID(spec.id);
    const float w = (spec.width > 0.0f)
                        ? spec.width
                        : ImGui::GetContentRegionAvail().x;
    ImPlot::PushColormap(spec.colormap);
    if (ImPlot::BeginPlot(spec.title ? spec.title : "##hm",
                          ImVec2(w - 70.0f, spec.height),
                          ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes(nullptr, nullptr,
                          ImPlotAxisFlags_Lock, ImPlotAxisFlags_Lock);
        ImPlot::SetupAxesLimits(0.0, static_cast<double>(spec.cols),
                                0.0, static_cast<double>(spec.rows));
        ImPlot::PlotHeatmap(spec.label ? spec.label : "##hm",
                            spec.values, spec.rows, spec.cols,
                            spec.scale_min, spec.scale_max,
                            spec.label_fmt,
                            ImPlotPoint(0.0, 0.0),
                            ImPlotPoint(spec.cols, spec.rows));
        ImPlot::EndPlot();
    }
    ImGui::SameLine();
    ImPlot::ColormapScale("##cb", spec.scale_min, spec.scale_max,
                          ImVec2(60.0f, spec.height));
    ImPlot::PopColormap();
    ImGui::PopID();
}

inline void DrawImPlotHistogram(const ImPlotHistogramSpec& spec) {
    if (!spec.values || spec.count <= 0) return;
    ImGui::PushID(spec.id);
    const float w = (spec.width > 0.0f) ? spec.width : ImGui::GetContentRegionAvail().x;
    if (ImPlot::BeginPlot(spec.title ? spec.title : "##hist",
                          ImVec2(w, spec.height), ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes(nullptr, spec.density ? "Density" : "Count",
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        // v0.18: ImPlotCol_Fill removed; use ImPlotSpec.FillColor + flags in Flags field
        ImPlotSpec ispec;
        if (spec.bar_color)
            ispec.FillColor = ImGui::ColorConvertU32ToFloat4(spec.bar_color);
        ImPlotHistogramFlags flags = ImPlotHistogramFlags_None;
        if (spec.cumulative) flags |= ImPlotHistogramFlags_Cumulative;
        if (spec.density)    flags |= ImPlotHistogramFlags_Density;
        ispec.Flags = flags;
        ImPlot::PlotHistogram(spec.label ? spec.label : "##hist",
                              spec.values, spec.count,
                              spec.bins, 1.0, ImPlotRange(), ispec);
        ImPlot::EndPlot();
    }
    ImGui::PopID();
}

#endif // ROGUECITY_HAS_IMPLOT

// ===========================================================================
// ImPlot3D-backed primitives  (requires ROGUECITY_HAS_IMPLOT3D=1)
// ===========================================================================
// 3D scatter/line/surface plots for spatial city data (building volumes,
// terrain elevation, AESP accessibility surfaces, lot distribution clouds).
// Each Draw* wraps ImPlot3D::BeginPlot / EndPlot.
// Call RC_UI::API::InitImPlot3D() / ShutdownImPlot3D() alongside the ImGui
// context lifetime; see rc_imgui_api.h.
// ===========================================================================
#ifdef ROGUECITY_HAS_IMPLOT3D

// 3D scatter — world-space point cloud (e.g. building centroid cloud).
struct ImPlot3DScatterSpec {
    const char*  id;            // ImGui PushID key (unique per panel)
    const char*  title;         // plot title (nullptr = hidden)
    const char*  label;         // series legend label (nullable)
    const float* xs;            // world X (meters or normalised)
    const float* ys;            // world Y
    const float* zs;            // world Z / elevation
    int          count;
    float        width;         // 0 = ContentRegionAvail.x
    float        height;
    ImU32        color;         // 0 = ImPlot3D auto-cycle
};

// 3D line — trajectory or road path in 3D space.
struct ImPlot3DLineSpec {
    const char*  id;
    const char*  title;
    const char*  label;
    const float* xs;
    const float* ys;
    const float* zs;
    int          count;
    float        width;
    float        height;
    ImU32        color;         // 0 = auto
    float        line_weight;   // 0 = default (1px)
};

// 3D surface — AESP score grid, terrain DEM, or density field.
// xs[x_count], ys[y_count] define the axis ticks;
// zs[y_count][x_count] (row-major) holds the surface heights.
struct ImPlot3DSurfaceSpec {
    const char*  id;
    const char*  title;
    const char*  label;
    const float* xs;            // x-axis values [x_count]
    const float* ys;            // y-axis values [y_count]
    const float* zs;            // surface heights [y_count * x_count], row-major
    int          x_count;
    int          y_count;
    double       scale_min;     // colormap low bound  (0 = auto)
    double       scale_max;     // colormap high bound (0 = auto)
    float        width;
    float        height;
    ImPlot3DColormap colormap;  // e.g. ImPlot3DColormap_Viridis
};

// ---------------------------------------------------------------------------

inline void DrawImPlot3DScatter(const ImPlot3DScatterSpec& spec) {
    if (!spec.xs || !spec.ys || !spec.zs || spec.count <= 0) return;
    ImGui::PushID(spec.id);
    const float w = (spec.width > 0.0f) ? spec.width : ImGui::GetContentRegionAvail().x;
    if (ImPlot3D::BeginPlot(spec.title ? spec.title : "##3dsc",
                            ImVec2(w, spec.height), ImPlot3DFlags_NoMenus)) {
        ImPlot3D::SetupAxes("X", "Y", "Z");
        ImPlot3DSpec ispec;
        if (spec.color) {
            ImVec4 c = ImGui::ColorConvertU32ToFloat4(spec.color);
            ispec.MarkerFillColor = c;
            ispec.MarkerLineColor = c;
        }
        ispec.Marker = ImPlot3DMarker_Circle;
        ImPlot3D::PlotScatter(spec.label ? spec.label : "##3dsc",
                              spec.xs, spec.ys, spec.zs, spec.count, ispec);
        ImPlot3D::EndPlot();
    }
    ImGui::PopID();
}

inline void DrawImPlot3DLine(const ImPlot3DLineSpec& spec) {
    if (!spec.xs || !spec.ys || !spec.zs || spec.count <= 0) return;
    ImGui::PushID(spec.id);
    const float w = (spec.width > 0.0f) ? spec.width : ImGui::GetContentRegionAvail().x;
    if (ImPlot3D::BeginPlot(spec.title ? spec.title : "##3dln",
                            ImVec2(w, spec.height), ImPlot3DFlags_NoMenus)) {
        ImPlot3D::SetupAxes("X", "Y", "Z");
        ImPlot3DSpec ispec;
        if (spec.color) ispec.LineColor = ImGui::ColorConvertU32ToFloat4(spec.color);
        if (spec.line_weight > 0.0f) ispec.LineWeight = spec.line_weight;
        ImPlot3D::PlotLine(spec.label ? spec.label : "##3dln",
                           spec.xs, spec.ys, spec.zs, spec.count, ispec);
        ImPlot3D::EndPlot();
    }
    ImGui::PopID();
}

inline void DrawImPlot3DSurface(const ImPlot3DSurfaceSpec& spec) {
    if (!spec.zs || spec.x_count <= 0 || spec.y_count <= 0) return;
    ImGui::PushID(spec.id);
    const float w = (spec.width > 0.0f) ? spec.width : ImGui::GetContentRegionAvail().x;
    ImPlot3D::PushColormap(spec.colormap);
    if (ImPlot3D::BeginPlot(spec.title ? spec.title : "##3dsrf",
                            ImVec2(w, spec.height), ImPlot3DFlags_NoMenus)) {
        ImPlot3D::SetupAxes("X", "Y", "Z");
        ImPlot3D::PlotSurface(spec.label ? spec.label : "##3dsrf",
                              spec.xs, spec.ys, spec.zs,
                              spec.x_count, spec.y_count,
                              spec.scale_min, spec.scale_max);
        ImPlot3D::EndPlot();
    }
    ImPlot3D::PopColormap();
    ImGui::PopID();
}

#endif // ROGUECITY_HAS_IMPLOT3D

} // namespace RC_UI::DataViz
