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
    const bool canvas_hovered = ImGui::IsItemHovered();
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

} // namespace RC_UI::DataViz
