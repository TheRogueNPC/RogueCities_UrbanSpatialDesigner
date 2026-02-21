#include "RogueCity/App/Tools/AxiomVisual.hpp"
#include "RogueCity/App/Tools/AxiomAnimationController.hpp"
#include "RogueCity/App/Tools/AxiomIcon.hpp"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

namespace RogueCity::App {
namespace {

constexpr float kMinRadius = 1.0f;
constexpr float kVertexHoverRadiusMeters = 14.0f;

[[nodiscard]] ImU32 ColorWithAlpha(ImU32 base, uint8_t alpha) {
    return IM_COL32(
        static_cast<int>((base >> IM_COL32_R_SHIFT) & 0xFF),
        static_cast<int>((base >> IM_COL32_G_SHIFT) & 0xFF),
        static_cast<int>((base >> IM_COL32_B_SHIFT) & 0xFF),
        alpha);
}

[[nodiscard]] ImU32 ScaleMonochrome(ImU32 base, float rgb_scale, uint8_t alpha) {
    const float clamped_scale = std::clamp(rgb_scale, 0.1f, 2.5f);
    const int r = static_cast<int>(std::clamp(
        static_cast<float>((base >> IM_COL32_R_SHIFT) & 0xFF) * clamped_scale,
        0.0f,
        255.0f));
    const int g = static_cast<int>(std::clamp(
        static_cast<float>((base >> IM_COL32_G_SHIFT) & 0xFF) * clamped_scale,
        0.0f,
        255.0f));
    const int b = static_cast<int>(std::clamp(
        static_cast<float>((base >> IM_COL32_B_SHIFT) & 0xFF) * clamped_scale,
        0.0f,
        255.0f));
    return IM_COL32(r, g, b, alpha);
}

[[nodiscard]] Generators::CityGenerator::AxiomInput::RingSchema RingSchemaForType(AxiomVisual::AxiomType type) {
    using RingSchema = Generators::CityGenerator::AxiomInput::RingSchema;
    switch (type) {
        case AxiomVisual::AxiomType::Organic:
            return RingSchema{0.30, 0.62, 1.0, 0.14, true};
        case AxiomVisual::AxiomType::Grid:
            return RingSchema{0.34, 0.70, 1.0, 0.10, true};
        case AxiomVisual::AxiomType::Radial:
            return RingSchema{0.33, 0.67, 1.0, 0.12, true};
        case AxiomVisual::AxiomType::Hexagonal:
            return RingSchema{0.32, 0.66, 1.0, 0.12, true};
        case AxiomVisual::AxiomType::Stem:
            return RingSchema{0.28, 0.64, 1.0, 0.16, true};
        case AxiomVisual::AxiomType::LooseGrid:
            return RingSchema{0.26, 0.60, 1.0, 0.18, true};
        case AxiomVisual::AxiomType::Suburban:
            return RingSchema{0.25, 0.58, 1.0, 0.20, true};
        case AxiomVisual::AxiomType::Superblock:
            return RingSchema{0.38, 0.74, 1.0, 0.10, true};
        case AxiomVisual::AxiomType::Linear:
            return RingSchema{0.27, 0.60, 1.0, 0.16, true};
        case AxiomVisual::AxiomType::GridCorrective:
            return RingSchema{0.30, 0.65, 1.0, 0.14, true};
        case AxiomVisual::AxiomType::COUNT:
        default:
            return RingSchema{};
    }
}

[[nodiscard]] Core::Vec2 AnimatePoint(const Core::Vec2& point, const Core::Vec2& center, float alpha) {
    const double clamped = std::clamp(static_cast<double>(alpha), 0.0, 1.0);
    return Core::Vec2(
        center.x + (point.x - center.x) * clamped,
        center.y + (point.y - center.y) * clamped);
}

void DrawScaledBoundary(
    ImDrawList* draw_list,
    const PrimaryViewport& viewport,
    const std::vector<Core::Vec2>& boundary,
    const Core::Vec2& center,
    float scale,
    ImU32 color,
    float thickness,
    float alpha) {
    if (boundary.size() < 3 || scale <= 0.0f) {
        return;
    }

    std::vector<ImVec2> points;
    points.reserve(boundary.size());
    for (const auto& p : boundary) {
        const Core::Vec2 scaled = center + (p - center) * static_cast<double>(scale);
        points.push_back(viewport.world_to_screen(AnimatePoint(scaled, center, alpha)));
    }
    draw_list->AddPolyline(points.data(), static_cast<int>(points.size()), color, ImDrawFlags_Closed, thickness);
}

void DrawBezierPatch(
    ImDrawList* draw_list,
    const PrimaryViewport& viewport,
    const ControlLattice& lattice,
    const Core::Vec2& center,
    float alpha,
    ImU32 color,
    ImU32 zone_inner_color,
    ImU32 zone_middle_color) {
    if (lattice.rows < 2 || lattice.cols < 2) {
        return;
    }

    auto point_at = [&](int row, int col) -> Core::Vec2 {
        const size_t idx = static_cast<size_t>(row * lattice.cols + col);
        return AnimatePoint(lattice.vertices[idx].world_pos, center, alpha);
    };

    // Horizontal splines.
    for (int row = 0; row < lattice.rows; ++row) {
        for (int col = 0; col < lattice.cols - 1; ++col) {
            const int prev_col = std::max(0, col - 1);
            const int next_col = std::min(lattice.cols - 1, col + 2);

            const Core::Vec2 p0 = point_at(row, col);
            const Core::Vec2 p1 = point_at(row, col + 1);
            const Core::Vec2 prev = point_at(row, prev_col);
            const Core::Vec2 next = point_at(row, next_col);

            const Core::Vec2 t0 = (p1 - prev) * 0.5;
            const Core::Vec2 t1 = (next - p0) * 0.5;
            const Core::Vec2 c0 = p0 + t0 / 3.0;
            const Core::Vec2 c1 = p1 - t1 / 3.0;

            draw_list->AddBezierCubic(
                viewport.world_to_screen(p0),
                viewport.world_to_screen(c0),
                viewport.world_to_screen(c1),
                viewport.world_to_screen(p1),
                color,
                1.6f,
                18);
        }
    }

    // Vertical splines.
    for (int col = 0; col < lattice.cols; ++col) {
        for (int row = 0; row < lattice.rows - 1; ++row) {
            const int prev_row = std::max(0, row - 1);
            const int next_row = std::min(lattice.rows - 1, row + 2);

            const Core::Vec2 p0 = point_at(row, col);
            const Core::Vec2 p1 = point_at(row + 1, col);
            const Core::Vec2 prev = point_at(prev_row, col);
            const Core::Vec2 next = point_at(next_row, col);

            const Core::Vec2 t0 = (p1 - prev) * 0.5;
            const Core::Vec2 t1 = (next - p0) * 0.5;
            const Core::Vec2 c0 = p0 + t0 / 3.0;
            const Core::Vec2 c1 = p1 - t1 / 3.0;

            draw_list->AddBezierCubic(
                viewport.world_to_screen(p0),
                viewport.world_to_screen(c0),
                viewport.world_to_screen(c1),
                viewport.world_to_screen(p1),
                color,
                1.6f,
                18);
        }
    }

    std::vector<Core::Vec2> perimeter;
    perimeter.reserve(static_cast<size_t>(2 * lattice.rows + 2 * lattice.cols));

    for (int col = 0; col < lattice.cols; ++col) {
        perimeter.push_back(point_at(0, col));
    }
    for (int row = 1; row < lattice.rows; ++row) {
        perimeter.push_back(point_at(row, lattice.cols - 1));
    }
    for (int col = lattice.cols - 2; col >= 0; --col) {
        perimeter.push_back(point_at(lattice.rows - 1, col));
    }
    for (int row = lattice.rows - 2; row > 0; --row) {
        perimeter.push_back(point_at(row, 0));
    }

    DrawScaledBoundary(
        draw_list,
        viewport,
        perimeter,
        center,
        lattice.zone_inner_uv,
        zone_inner_color,
        1.0f,
        alpha);
    DrawScaledBoundary(
        draw_list,
        viewport,
        perimeter,
        center,
        lattice.zone_middle_uv,
        zone_middle_color,
        1.1f,
        alpha);
}

void DrawPolygonWireframe(
    ImDrawList* draw_list,
    const PrimaryViewport& viewport,
    const ControlLattice& lattice,
    const Core::Vec2& center,
    float alpha,
    ImU32 color,
    ImU32 zone_inner_color,
    ImU32 zone_middle_color) {
    if (lattice.vertices.size() < 3) {
        return;
    }

    std::vector<ImVec2> polygon;
    polygon.reserve(lattice.vertices.size());

    std::vector<Core::Vec2> boundary;
    boundary.reserve(lattice.vertices.size());

    for (const auto& v : lattice.vertices) {
        const Core::Vec2 p = AnimatePoint(v.world_pos, center, alpha);
        boundary.push_back(p);
        polygon.push_back(viewport.world_to_screen(p));
    }

    draw_list->AddPolyline(polygon.data(), static_cast<int>(polygon.size()), color, ImDrawFlags_Closed, 2.0f);

    DrawScaledBoundary(
        draw_list,
        viewport,
        boundary,
        center,
        lattice.zone_inner_uv,
        zone_inner_color,
        1.0f,
        alpha);
    DrawScaledBoundary(
        draw_list,
        viewport,
        boundary,
        center,
        lattice.zone_middle_uv,
        zone_middle_color,
        1.1f,
        alpha);
}

void DrawLinearWireframe(
    ImDrawList* draw_list,
    const PrimaryViewport& viewport,
    const ControlLattice& lattice,
    const Core::Vec2& center,
    float alpha,
    ImU32 color,
    ImU32 auxiliary_color) {
    if (lattice.vertices.size() < 2) {
        return;
    }

    std::vector<ImVec2> spine;
    spine.reserve(2);
    spine.push_back(viewport.world_to_screen(AnimatePoint(lattice.vertices[0].world_pos, center, alpha)));
    spine.push_back(viewport.world_to_screen(AnimatePoint(lattice.vertices[1].world_pos, center, alpha)));
    draw_list->AddPolyline(spine.data(), static_cast<int>(spine.size()), color, ImDrawFlags_None, 2.0f);

    if (lattice.vertices.size() >= 6) {
        const std::array<std::pair<size_t, size_t>, 2> width_pairs{{{2, 3}, {4, 5}}};
        for (const auto [a, b] : width_pairs) {
            const ImVec2 p0 = viewport.world_to_screen(AnimatePoint(lattice.vertices[a].world_pos, center, alpha));
            const ImVec2 p1 = viewport.world_to_screen(AnimatePoint(lattice.vertices[b].world_pos, center, alpha));
            const ImVec2 pair[2] = {p0, p1};
            draw_list->AddPolyline(pair, 2, auxiliary_color, ImDrawFlags_None, 1.5f);
        }
    }
}

void DrawRadialWireframe(
    ImDrawList* draw_list,
    const PrimaryViewport& viewport,
    const ControlLattice& lattice,
    const Core::Vec2& center,
    float alpha,
    ImU32 color,
    ImU32 zone_inner_color,
    ImU32 zone_middle_color,
    ImU32 zone_outer_color,
    bool draw_zone_circles) {
    if (lattice.vertices.size() < 2) {
        return;
    }

    const Core::Vec2 animated_center = AnimatePoint(lattice.vertices[0].world_pos, center, alpha);
    const ImVec2 screen_center = viewport.world_to_screen(animated_center);

    double max_radius = 0.0;
    for (size_t i = 1; i < lattice.vertices.size(); ++i) {
        const Core::Vec2 spoke = AnimatePoint(lattice.vertices[i].world_pos, center, alpha);
        const ImVec2 screen_spoke = viewport.world_to_screen(spoke);
        const ImVec2 line[2] = {screen_center, screen_spoke};
        draw_list->AddPolyline(line, 2, color, ImDrawFlags_None, 1.6f);
        max_radius = std::max(max_radius, animated_center.distanceTo(spoke));
    }

    std::vector<Core::Vec2> boundary;
    boundary.reserve(lattice.vertices.size() - 1);
    for (size_t i = 1; i < lattice.vertices.size(); ++i) {
        boundary.push_back(AnimatePoint(lattice.vertices[i].world_pos, center, alpha));
    }

    if (draw_zone_circles) {
        const float outer = viewport.world_to_screen_scale(static_cast<float>(max_radius));
        const float inner = outer * std::clamp(lattice.zone_inner_uv, 0.05f, 1.0f);
        const float middle = outer * std::clamp(lattice.zone_middle_uv, 0.05f, 1.0f);
        draw_list->AddCircle(screen_center, inner, zone_inner_color, 64, 1.0f);
        draw_list->AddCircle(screen_center, middle, zone_middle_color, 64, 1.1f);
        draw_list->AddCircle(screen_center, outer, zone_outer_color, 64, 1.3f);
    } else {
        DrawScaledBoundary(
            draw_list,
            viewport,
            boundary,
            animated_center,
            lattice.zone_inner_uv,
            zone_inner_color,
            1.0f,
            alpha);
        DrawScaledBoundary(
            draw_list,
            viewport,
            boundary,
            animated_center,
            lattice.zone_middle_uv,
            zone_middle_color,
            1.1f,
            alpha);
        DrawScaledBoundary(
            draw_list,
            viewport,
            boundary,
            animated_center,
            lattice.zone_outer_uv,
            zone_outer_color,
            1.3f,
            alpha);
    }
}

void RenderLattice(
    ImDrawList* draw_list,
    const PrimaryViewport& viewport,
    const ControlLattice& lattice,
    AxiomVisual::AxiomType type,
    ImU32 base_color,
    const Core::Vec2& center,
    float alpha) {
    if (draw_list == nullptr) {
        return;
    }

    const ImU32 line_color = ColorWithAlpha(base_color, 175u);
    const ImU32 zone_inner_color = ColorWithAlpha(base_color, 95u);
    const ImU32 zone_middle_color = ColorWithAlpha(base_color, 130u);
    const ImU32 zone_outer_color = ColorWithAlpha(base_color, 160u);
    const ImU32 auxiliary_color = ColorWithAlpha(base_color, 145u);

    switch (lattice.topology) {
        case LatticeTopology::BezierPatch:
            DrawBezierPatch(
                draw_list,
                viewport,
                lattice,
                center,
                alpha,
                line_color,
                zone_inner_color,
                zone_middle_color);
            break;
        case LatticeTopology::Polygon:
            DrawPolygonWireframe(
                draw_list,
                viewport,
                lattice,
                center,
                alpha,
                line_color,
                zone_inner_color,
                zone_middle_color);
            break;
        case LatticeTopology::Radial:
            DrawRadialWireframe(
                draw_list,
                viewport,
                lattice,
                center,
                alpha,
                line_color,
                zone_inner_color,
                zone_middle_color,
                zone_outer_color,
                type == AxiomVisual::AxiomType::Radial || type == AxiomVisual::AxiomType::GridCorrective);
            break;
        case LatticeTopology::Linear:
            DrawLinearWireframe(draw_list, viewport, lattice, center, alpha, line_color, auxiliary_color);
            break;
    }
}

enum class PreviewGhostStyle {
    Axes,
    Diagonal,
    Spiral,
    Rings,
    Boundary,
    Bundle
};

[[nodiscard]] PreviewGhostStyle PreviewStyleForFeature(Generators::TerminalFeature feature) {
    using TF = Generators::TerminalFeature;
    switch (feature) {
        case TF::Grid_AxisAlignmentLock:
        case TF::GridCorrective_AbsoluteOverride:
        case TF::GridCorrective_MagneticAlignment:
        case TF::GridCorrective_OrthogonalCull:
        case TF::Hex_HoneycombStrictness:
        case TF::Stem_DirectionalFlowBias:
            return PreviewGhostStyle::Axes;
        case TF::Grid_DiagonalSlicing:
        case TF::Grid_AlleywayBisection:
        case TF::Hex_TriangularSubdivision:
        case TF::Linear_PerpendicularRungs:
            return PreviewGhostStyle::Diagonal;
        case TF::Organic_MeanderBias:
        case TF::Radial_SpiralDominance:
        case TF::Linear_RibbonBraiding:
        case TF::Stem_CanopyWeave:
            return PreviewGhostStyle::Spiral;
        case TF::Radial_CoreVoiding:
        case TF::Radial_ConcentricWaveDensity:
        case TF::Superblock_CourtyardVoid:
        case TF::Suburban_LollipopTerminals:
            return PreviewGhostStyle::Rings;
        case TF::Suburban_ArterialIsolation:
        case TF::Superblock_PermeableEdges:
        case TF::GridCorrective_BoundaryStitching:
        case TF::Superblock_ArterialTrenching:
            return PreviewGhostStyle::Boundary;
        default:
            return PreviewGhostStyle::Bundle;
    }
}

void DrawTerminalFeatureGhost(
    ImDrawList* draw_list,
    const PrimaryViewport& viewport,
    const Core::Vec2& center,
    float radius_world,
    Generators::TerminalFeature feature,
    float alpha,
    ImU32 color) {
    if (draw_list == nullptr || alpha <= 0.01f || radius_world <= 1.0f) {
        return;
    }

    const ImVec2 c = viewport.world_to_screen(center);
    const float r = std::max(8.0f, viewport.world_to_screen_scale(radius_world));
    const ImU32 ghost = ColorWithAlpha(color, static_cast<uint8_t>(std::clamp(alpha * 190.0f, 32.0f, 255.0f)));
    const ImU32 ghost_soft = ColorWithAlpha(color, static_cast<uint8_t>(std::clamp(alpha * 110.0f, 20.0f, 200.0f)));

    switch (PreviewStyleForFeature(feature)) {
        case PreviewGhostStyle::Axes: {
            draw_list->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), ghost, 2.0f);
            draw_list->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y + r), ghost, 2.0f);
            draw_list->AddCircle(c, r * 0.75f, ghost_soft, 48, 1.2f);
            break;
        }
        case PreviewGhostStyle::Diagonal: {
            draw_list->AddLine(ImVec2(c.x - r * 0.9f, c.y - r * 0.9f), ImVec2(c.x + r * 0.9f, c.y + r * 0.9f), ghost, 2.0f);
            draw_list->AddLine(ImVec2(c.x - r * 0.9f, c.y + r * 0.9f), ImVec2(c.x + r * 0.9f, c.y - r * 0.9f), ghost_soft, 1.5f);
            draw_list->AddRect(ImVec2(c.x - r * 0.6f, c.y - r * 0.6f), ImVec2(c.x + r * 0.6f, c.y + r * 0.6f), ghost_soft, 2.0f, 0, 1.0f);
            break;
        }
        case PreviewGhostStyle::Spiral: {
            constexpr int segments = 56;
            std::array<ImVec2, segments> points{};
            for (int i = 0; i < segments; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(segments - 1);
                const float angle = t * 7.0f;
                const float rr = r * (0.08f + t * 0.86f);
                points[static_cast<size_t>(i)] = ImVec2(
                    c.x + std::cos(angle) * rr,
                    c.y + std::sin(angle) * rr);
            }
            draw_list->AddPolyline(points.data(), segments, ghost, ImDrawFlags_None, 1.8f);
            break;
        }
        case PreviewGhostStyle::Rings: {
            draw_list->AddCircle(c, r * 0.28f, ghost, 48, 1.8f);
            draw_list->AddCircle(c, r * 0.56f, ghost_soft, 48, 1.4f);
            draw_list->AddCircle(c, r * 0.85f, ghost_soft, 48, 1.1f);
            break;
        }
        case PreviewGhostStyle::Boundary: {
            draw_list->AddCircle(c, r * 0.92f, ghost, 64, 2.0f);
            draw_list->AddCircle(c, r * 0.74f, ghost_soft, 64, 1.2f);
            draw_list->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x - r * 0.6f, c.y), ghost_soft, 2.0f);
            draw_list->AddLine(ImVec2(c.x + r * 0.6f, c.y), ImVec2(c.x + r, c.y), ghost_soft, 2.0f);
            break;
        }
        case PreviewGhostStyle::Bundle: {
            draw_list->AddCircle(c, r * 0.80f, ghost_soft, 48, 1.2f);
            draw_list->AddCircle(c, r * 0.48f, ghost_soft, 48, 1.0f);
            draw_list->AddLine(ImVec2(c.x - r * 0.95f, c.y - r * 0.2f), ImVec2(c.x + r * 0.95f, c.y - r * 0.2f), ghost, 1.6f);
            draw_list->AddLine(ImVec2(c.x - r * 0.95f, c.y + r * 0.2f), ImVec2(c.x + r * 0.95f, c.y + r * 0.2f), ghost, 1.6f);
            break;
        }
    }
}

void InitializeLatticeForType(
    AxiomVisual::AxiomType type,
    ControlLattice& lattice,
    const Core::Vec2& center,
    float base_radius,
    int radial_spokes,
    float radial_rotation) {
    lattice.vertices.clear();
    lattice.rows = 0;
    lattice.cols = 0;

    const float safe_radius = std::max(base_radius, kMinRadius);
    int vertex_id = 0;

    switch (type) {
        case AxiomVisual::AxiomType::Organic:
        case AxiomVisual::AxiomType::LooseGrid:
            lattice.topology = LatticeTopology::BezierPatch;
            lattice.rows = 4;
            lattice.cols = 4;
            for (int y = 0; y < lattice.rows; ++y) {
                for (int x = 0; x < lattice.cols; ++x) {
                    const double u = static_cast<double>(x) / static_cast<double>(lattice.cols - 1);
                    const double v = static_cast<double>(y) / static_cast<double>(lattice.rows - 1);
                    const Core::Vec2 w_pos = {
                        center.x + (u - 0.5) * static_cast<double>(safe_radius * 2.0f),
                        center.y + (v - 0.5) * static_cast<double>(safe_radius * 2.0f)
                    };
                    lattice.vertices.push_back({vertex_id++, w_pos, {u, v}});
                }
            }
            break;

        case AxiomVisual::AxiomType::Grid:
        case AxiomVisual::AxiomType::Superblock:
            lattice.topology = LatticeTopology::Polygon;
            lattice.vertices = {
                {vertex_id++, {center.x - safe_radius, center.y - safe_radius}, {0.0, 0.0}},
                {vertex_id++, {center.x + safe_radius, center.y - safe_radius}, {1.0, 0.0}},
                {vertex_id++, {center.x + safe_radius, center.y + safe_radius}, {1.0, 1.0}},
                {vertex_id++, {center.x - safe_radius, center.y + safe_radius}, {0.0, 1.0}}
            };
            break;

        case AxiomVisual::AxiomType::Hexagonal:
            lattice.topology = LatticeTopology::Polygon;
            for (int i = 0; i < 6; ++i) {
                const float angle = (std::numbers::pi_v<float> / 3.0f) * static_cast<float>(i);
                lattice.vertices.push_back({
                    vertex_id++,
                    {
                        center.x + static_cast<double>(safe_radius * std::cos(angle)),
                        center.y + static_cast<double>(safe_radius * std::sin(angle))
                    },
                    {
                        0.5 + static_cast<double>(0.5f * std::cos(angle)),
                        0.5 + static_cast<double>(0.5f * std::sin(angle))
                    }
                });
            }
            break;

        case AxiomVisual::AxiomType::Radial:
        case AxiomVisual::AxiomType::Suburban:
        case AxiomVisual::AxiomType::GridCorrective: {
            const int spokes = std::clamp(radial_spokes, 3, 24);
            lattice.topology = LatticeTopology::Radial;
            lattice.vertices.push_back({vertex_id++, center, {0.5, 0.5}});
            for (int i = 0; i < spokes; ++i) {
                const float angle = radial_rotation +
                    (2.0f * std::numbers::pi_v<float> * static_cast<float>(i)) / static_cast<float>(spokes);
                const double cx = static_cast<double>(std::cos(angle));
                const double sy = static_cast<double>(std::sin(angle));
                lattice.vertices.push_back({
                    vertex_id++,
                    {center.x + safe_radius * cx, center.y + safe_radius * sy},
                    {0.5 + 0.5 * cx, 0.5 + 0.5 * sy}
                });
            }
            break;
        }

        case AxiomVisual::AxiomType::Stem:
        case AxiomVisual::AxiomType::Linear:
            lattice.topology = LatticeTopology::Linear;
            lattice.vertices = {
                {vertex_id++, {center.x - safe_radius, center.y}, {0.0, 0.5}},
                {vertex_id++, {center.x + safe_radius, center.y}, {1.0, 0.5}},
                {vertex_id++, {center.x - safe_radius * 0.4, center.y - safe_radius * 0.45}, {0.3, 0.2}},
                {vertex_id++, {center.x + safe_radius * 0.4, center.y - safe_radius * 0.45}, {0.7, 0.2}},
                {vertex_id++, {center.x - safe_radius * 0.4, center.y + safe_radius * 0.45}, {0.3, 0.8}},
                {vertex_id++, {center.x + safe_radius * 0.4, center.y + safe_radius * 0.45}, {0.7, 0.8}}
            };
            break;

        case AxiomVisual::AxiomType::COUNT:
        default:
            lattice.topology = LatticeTopology::Polygon;
            lattice.vertices = {
                {vertex_id++, {center.x - safe_radius, center.y - safe_radius}, {0.0, 0.0}},
                {vertex_id++, {center.x + safe_radius, center.y - safe_radius}, {1.0, 0.0}},
                {vertex_id++, {center.x + safe_radius, center.y + safe_radius}, {1.0, 1.0}},
                {vertex_id++, {center.x - safe_radius, center.y + safe_radius}, {0.0, 1.0}}
            };
            break;
    }
}

} // namespace

AxiomVisual::AxiomVisual(int id, AxiomType type)
    : id_(id)
    , type_(type)
    , animator_(std::make_unique<AxiomAnimationController>()) {
    refresh_zone_defaults_from_type();
    initialize_lattice_for_type();
}

AxiomVisual::~AxiomVisual() = default;

void AxiomVisual::initialize_lattice_for_type() {
    InitializeLatticeForType(type_, lattice_, position_, radius_, radial_spokes_, radial_ring_rotation_);
}

void AxiomVisual::refresh_zone_defaults_from_type() {
    const auto schema = RingSchemaForType(type_);
    lattice_.zone_inner_uv = static_cast<float>(schema.core_ratio);
    lattice_.zone_middle_uv = static_cast<float>(schema.falloff_ratio);
    lattice_.zone_outer_uv = static_cast<float>(schema.outskirts_ratio);
}

void AxiomVisual::clear_vertex_interaction_flags() {
    for (auto& vertex : lattice_.vertices) {
        vertex.is_hovered = false;
        if (!vertex.is_dragging) {
            vertex.is_dragging = false;
        }
    }
}

void AxiomVisual::recalculate_radius_from_lattice() {
    double max_dist = 0.0;
    for (const auto& vertex : lattice_.vertices) {
        max_dist = std::max(max_dist, position_.distanceTo(vertex.world_pos));
    }
    radius_ = std::max(kMinRadius, static_cast<float>(max_dist));
}

void AxiomVisual::update(float delta_time) {
    if (!animation_enabled_) {
        lattice_animation_alpha_ = 1.0f;
    } else if (lattice_animation_alpha_ < 1.0f) {
        const float blend_speed = 3.5f;
        lattice_animation_alpha_ += (1.0f - lattice_animation_alpha_) * (1.0f - std::exp(-blend_speed * delta_time));
        lattice_animation_alpha_ = std::clamp(lattice_animation_alpha_, 0.0f, 1.0f);
    }

    const float target_preview = preview_feature_.has_value() ? 1.0f : 0.0f;
    const float preview_blend = 1.0f - std::exp(-8.0f * delta_time);
    preview_alpha_ += (target_preview - preview_alpha_) * preview_blend;
    preview_alpha_ = std::clamp(preview_alpha_, 0.0f, 1.0f);
}

void AxiomVisual::render(ImDrawList* draw_list, const PrimaryViewport& viewport) {
    if (draw_list == nullptr) {
        return;
    }

    const ImU32 type_color = GetAxiomTypeInfo(type_).primary_color;
    RenderLattice(draw_list, viewport, lattice_, type_, type_color, position_, lattice_animation_alpha_);
    if (preview_feature_.has_value() && preview_alpha_ > 0.01f) {
        DrawTerminalFeatureGhost(draw_list, viewport, position_, radius_, *preview_feature_, preview_alpha_, type_color);
    }

    // Center marker.
    const ImVec2 screen_center = viewport.world_to_screen(position_);
    const float marker_size = hovered_ ? 12.0f : 10.0f;
    const ImU32 marker_color = selected_ ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255);

    draw_list->AddRectFilled(
        ImVec2(screen_center.x - marker_size, screen_center.y - marker_size),
        ImVec2(screen_center.x + marker_size, screen_center.y + marker_size),
        IM_COL32(0, 0, 0, 180),
        3.0f);
    draw_list->AddRect(
        ImVec2(screen_center.x - marker_size, screen_center.y - marker_size),
        ImVec2(screen_center.x + marker_size, screen_center.y + marker_size),
        marker_color,
        3.0f,
        ImDrawFlags_None,
        2.0f);
    draw_list->AddLine(
        ImVec2(screen_center.x - marker_size * 0.65f, screen_center.y),
        ImVec2(screen_center.x + marker_size * 0.65f, screen_center.y),
        marker_color,
        1.5f);
    draw_list->AddLine(
        ImVec2(screen_center.x, screen_center.y - marker_size * 0.65f),
        ImVec2(screen_center.x, screen_center.y + marker_size * 0.65f),
        marker_color,
        1.5f);

    const ImU32 icon_color = type_color;
    DrawAxiomIcon(draw_list, screen_center, marker_size * 0.85f, type_, icon_color);

    for (const auto& vertex : lattice_.vertices) {
        const Core::Vec2 animated_world = AnimatePoint(vertex.world_pos, position_, lattice_animation_alpha_);
        const ImVec2 screen_pos = viewport.world_to_screen(animated_world);
        const float radius = vertex.is_hovered ? 6.0f : 4.5f;
        const ImU32 color = vertex.is_dragging
            ? IM_COL32(255, 255, 0, 255)
            : (vertex.is_hovered
                ? ScaleMonochrome(type_color, 1.25f, 255u)
                : ScaleMonochrome(type_color, 1.0f, 238u));

        draw_list->AddCircleFilled(screen_pos, radius, color);
        draw_list->AddCircle(screen_pos, radius, IM_COL32(0, 0, 0, 255), 0, 1.4f);
    }
}

bool AxiomVisual::is_hovered(const Core::Vec2& world_pos, float world_radius) const {
    const double dx = world_pos.x - position_.x;
    const double dy = world_pos.y - position_.y;
    const double dist_sq = dx * dx + dy * dy;
    const double radius_sq = static_cast<double>(world_radius) * static_cast<double>(world_radius);
    return dist_sq <= radius_sq;
}

ControlVertex* AxiomVisual::get_hovered_vertex(const Core::Vec2& world_pos) {
    clear_vertex_interaction_flags();

    ControlVertex* hovered = nullptr;
    double best_dist_sq = std::numeric_limits<double>::max();
    const double snap = static_cast<double>(kVertexHoverRadiusMeters);
    const double snap_sq = snap * snap;

    for (auto& vertex : lattice_.vertices) {
        const double dx = world_pos.x - vertex.world_pos.x;
        const double dy = world_pos.y - vertex.world_pos.y;
        const double dist_sq = dx * dx + dy * dy;
        if (dist_sq <= snap_sq && dist_sq < best_dist_sq) {
            best_dist_sq = dist_sq;
            hovered = &vertex;
        }
    }

    if (hovered != nullptr) {
        hovered->is_hovered = true;
    }
    return hovered;
}

const ControlLattice& AxiomVisual::lattice() const {
    return lattice_;
}

ControlLattice& AxiomVisual::lattice() {
    return lattice_;
}

bool AxiomVisual::update_vertex_world_position(int vertex_id, const Core::Vec2& world_pos) {
    for (auto& vertex : lattice_.vertices) {
        if (vertex.id != vertex_id) {
            continue;
        }
        vertex.world_pos = world_pos;

        const double safe_radius = std::max(1e-6, static_cast<double>(std::max(radius_, kMinRadius)));
        const Core::Vec2 local = world_pos - position_;
        vertex.uv.x = (local.x / (safe_radius * 2.0)) + 0.5;
        vertex.uv.y = (local.y / (safe_radius * 2.0)) + 0.5;

        recalculate_radius_from_lattice();
        return true;
    }
    return false;
}

void AxiomVisual::set_hovered(bool hovered) {
    hovered_ = hovered;
}

void AxiomVisual::set_selected(bool selected) {
    selected_ = selected;
}

void AxiomVisual::set_position(const Core::Vec2& pos) {
    const Core::Vec2 delta = pos - position_;
    position_ = pos;
    for (auto& vertex : lattice_.vertices) {
        vertex.world_pos += delta;
    }
}

void AxiomVisual::set_radius(float radius) {
    const float clamped = std::max(radius, kMinRadius);

    if (lattice_.vertices.empty()) {
        radius_ = clamped;
        initialize_lattice_for_type();
        return;
    }

    const double old_radius = std::max(static_cast<double>(radius_), static_cast<double>(kMinRadius));
    const double scale = static_cast<double>(clamped) / old_radius;

    for (auto& vertex : lattice_.vertices) {
        const Core::Vec2 delta = vertex.world_pos - position_;
        vertex.world_pos = position_ + delta * scale;
    }

    radius_ = clamped;
}

void AxiomVisual::set_type(AxiomType type) {
    type_ = type;
    terminal_features_ = {};
    preview_feature_.reset();
    preview_alpha_ = 0.0f;
    refresh_zone_defaults_from_type();
    initialize_lattice_for_type();
}

void AxiomVisual::set_rotation(float theta) {
    rotation_ = theta;
}

int AxiomVisual::id() const { return id_; }
const Core::Vec2& AxiomVisual::position() const { return position_; }
float AxiomVisual::radius() const { return radius_; }
AxiomVisual::AxiomType AxiomVisual::type() const { return type_; }
float AxiomVisual::rotation() const { return rotation_; }

void AxiomVisual::set_organic_curviness(float value) { organic_curviness_ = std::clamp(value, 0.0f, 1.0f); }
void AxiomVisual::set_radial_spokes(int spokes) {
    radial_spokes_ = std::clamp(spokes, 3, 24);
    if (type_ == AxiomType::Radial || type_ == AxiomType::Suburban || type_ == AxiomType::GridCorrective) {
        initialize_lattice_for_type();
    }
}
void AxiomVisual::set_loose_grid_jitter(float value) { loose_grid_jitter_ = std::clamp(value, 0.0f, 1.0f); }
void AxiomVisual::set_suburban_loop_strength(float value) { suburban_loop_strength_ = std::clamp(value, 0.0f, 1.0f); }
void AxiomVisual::set_stem_branch_angle(float radians) { stem_branch_angle_ = std::clamp(radians, 0.0f, std::numbers::pi_v<float>); }
void AxiomVisual::set_superblock_block_size(float meters) { superblock_block_size_ = std::max(50.0f, meters); }
void AxiomVisual::set_radial_ring_rotation(float radians) {
    radial_ring_rotation_ = radians;
    if (type_ == AxiomType::Radial || type_ == AxiomType::Suburban || type_ == AxiomType::GridCorrective) {
        initialize_lattice_for_type();
    }
}
void AxiomVisual::set_radial_ring_knob_weight(size_t ring_index, size_t knob_index, float value) {
    if (ring_index >= radial_ring_knob_weights_.size() || knob_index >= radial_ring_knob_weights_[ring_index].size()) {
        return;
    }
    radial_ring_knob_weights_[ring_index][knob_index] = std::clamp(value, 0.25f, 2.5f);
}
void AxiomVisual::set_terminal_feature(Generators::TerminalFeature feature, bool enabled) {
    if (!Generators::featureAllowedForType(type_, feature)) {
        return;
    }
    terminal_features_.set(feature, enabled);
}
void AxiomVisual::set_terminal_features(const Generators::TerminalFeatureSet& features) {
    terminal_features_ = {};
    const auto allowed = Generators::featuresForAxiomType(type_);
    for (const auto feature : allowed) {
        terminal_features_.set(feature, features.has(feature));
    }
}
void AxiomVisual::set_preview_feature(std::optional<Generators::TerminalFeature> feature) {
    if (feature.has_value() && !Generators::featureAllowedForType(type_, *feature)) {
        preview_feature_.reset();
        return;
    }
    preview_feature_ = feature;
}

float AxiomVisual::organic_curviness() const { return organic_curviness_; }
int AxiomVisual::radial_spokes() const { return radial_spokes_; }
float AxiomVisual::loose_grid_jitter() const { return loose_grid_jitter_; }
float AxiomVisual::suburban_loop_strength() const { return suburban_loop_strength_; }
float AxiomVisual::stem_branch_angle() const { return stem_branch_angle_; }
float AxiomVisual::superblock_block_size() const { return superblock_block_size_; }
float AxiomVisual::radial_ring_rotation() const { return radial_ring_rotation_; }
float AxiomVisual::radial_ring_knob_weight(size_t ring_index, size_t knob_index) const {
    if (ring_index >= radial_ring_knob_weights_.size() || knob_index >= radial_ring_knob_weights_[ring_index].size()) {
        return 1.0f;
    }
    return radial_ring_knob_weights_[ring_index][knob_index];
}
Generators::TerminalFeatureSet AxiomVisual::terminal_features() const { return terminal_features_; }
bool AxiomVisual::terminal_feature_enabled(Generators::TerminalFeature feature) const {
    return terminal_features_.has(feature);
}

void AxiomVisual::trigger_placement_animation() {
    if (!animation_enabled_) {
        lattice_animation_alpha_ = 1.0f;
        return;
    }
    lattice_animation_alpha_ = 0.0f;
}

void AxiomVisual::set_animation_enabled(bool enabled) {
    animation_enabled_ = enabled;
    if (!enabled) {
        lattice_animation_alpha_ = 1.0f;
    }
}

Generators::CityGenerator::AxiomInput AxiomVisual::to_axiom_input() const {
    Generators::CityGenerator::AxiomInput input;
    input.id = id_;
    input.type = type_;
    input.position = position_;
    input.radius = radius_;
    input.theta = rotation_;
    input.decay = decay_;
    input.ring_schema = RingSchemaForType(type_);
    input.ring_schema.core_ratio = std::clamp(static_cast<double>(lattice_.zone_inner_uv), 0.05, 1.0);
    input.ring_schema.falloff_ratio = std::clamp(
        static_cast<double>(lattice_.zone_middle_uv),
        input.ring_schema.core_ratio,
        1.0);
    input.ring_schema.outskirts_ratio = std::clamp(
        static_cast<double>(lattice_.zone_outer_uv),
        input.ring_schema.falloff_ratio,
        1.5);
    input.lock_generated_roads = false;
    input.organic_curviness = organic_curviness_;
    input.radial_spokes = radial_spokes_;
    input.loose_grid_jitter = loose_grid_jitter_;
    input.suburban_loop_strength = suburban_loop_strength_;
    input.stem_branch_angle = stem_branch_angle_;
    input.superblock_block_size = superblock_block_size_;
    input.terminal_features = terminal_features_;
    input.radial_ring_rotation = static_cast<double>(radial_ring_rotation_);
    input.radial_ring_knob_weights = radial_ring_knob_weights_;

    input.warp_lattice.topology_type = static_cast<int>(lattice_.topology);
    input.warp_lattice.rows = lattice_.rows;
    input.warp_lattice.cols = lattice_.cols;
    input.warp_lattice.zone_inner_uv = lattice_.zone_inner_uv;
    input.warp_lattice.zone_middle_uv = lattice_.zone_middle_uv;
    input.warp_lattice.zone_outer_uv = lattice_.zone_outer_uv;
    input.warp_lattice.vertices.clear();
    input.warp_lattice.vertices.reserve(lattice_.vertices.size());
    for (const auto& vertex : lattice_.vertices) {
        input.warp_lattice.vertices.push_back(vertex.world_pos);
    }

    return input;
}

} // namespace RogueCity::App
