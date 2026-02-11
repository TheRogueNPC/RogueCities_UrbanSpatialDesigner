#include "RogueCity/Generators/Roads/IntersectionTemplates.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace RogueCity::Generators::Roads {

    namespace {

        [[nodiscard]] Core::Polygon makeCircle(
            const Core::Vec2& c,
            float radius,
            int segments) {
            Core::Polygon poly;
            const int seg = std::max(6, segments);
            poly.points.reserve(static_cast<size_t>(seg));
            const double tau = 6.28318530717958647692;
            for (int i = 0; i < seg; ++i) {
                const double t = (static_cast<double>(i) / static_cast<double>(seg)) * tau;
                poly.points.emplace_back(
                    c.x + static_cast<double>(radius) * std::cos(t),
                    c.y + static_cast<double>(radius) * std::sin(t));
            }
            return poly;
        }

        [[nodiscard]] float controlRadiusBoost(Urban::ControlType control) {
            switch (control) {
                case Urban::ControlType::Roundabout: return 1.7f;
                case Urban::ControlType::Interchange: return 2.2f;
                case Urban::ControlType::GradeSep: return 1.8f;
                case Urban::ControlType::Signal: return 1.3f;
                default: return 1.0f;
            }
        }

        [[nodiscard]] float greenspaceScore(
            const Urban::Graph& g,
            Urban::VertexID vid,
            float intrusion_penalty) {
            const auto* v = g.getVertex(vid);
            if (v == nullptr) {
                return 0.0f;
            }

            float flow_adj = 0.0f;
            int high_frontage_edges = 0;
            for (const auto eid : v->edges) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                flow_adj += e->flow.flow_score;
                if (e->flow.flow_score > 1.0f) {
                    ++high_frontage_edges;
                }
            }

            const float visibility = 1.0f + std::min(1.0f, static_cast<float>(v->edges.size()) * 0.15f);
            const float frontage = 1.0f + static_cast<float>(high_frontage_edges) * 0.2f;
            const float score = (flow_adj * 0.25f + visibility * frontage) / std::max(0.5f, intrusion_penalty);
            return std::max(0.0f, score);
        }

    } // namespace

    TemplateOutput emitIntersectionTemplates(const Urban::Graph& g, const TemplateConfig& cfg) {
        TemplateOutput out;
        if (g.vertices().empty()) {
            return out;
        }

        for (Urban::VertexID vid = 0; vid < g.vertices().size(); ++vid) {
            const auto* v = g.getVertex(vid);
            if (v == nullptr) {
                continue;
            }
            if (v->kind != Urban::VertexKind::Intersection &&
                v->control != Urban::ControlType::Roundabout &&
                v->control != Urban::ControlType::Interchange &&
                v->control != Urban::ControlType::GradeSep) {
                continue;
            }

            const float boost = controlRadiusBoost(v->control);
            const float paved_r = cfg.paved_radius_base * boost;
            const float keep_out_r = cfg.keep_out_radius_base * boost;
            const float buffer_r = cfg.buffer_radius_base * boost;

            out.paved_areas.push_back(makeCircle(v->pos, paved_r, cfg.circle_segments));
            out.keep_out_islands.push_back(makeCircle(v->pos, keep_out_r, cfg.circle_segments));

            if (v->control == Urban::ControlType::Interchange || v->control == Urban::ControlType::GradeSep) {
                constexpr std::array<double, 4> offsets{ 0.0, 1.57079632679, 3.14159265359, 4.71238898038 };
                for (const double theta : offsets) {
                    Core::Vec2 support_center(
                        v->pos.x + std::cos(theta) * static_cast<double>(paved_r * 0.75f),
                        v->pos.y + std::sin(theta) * static_cast<double>(paved_r * 0.75f));
                    out.support_footprints.push_back(makeCircle(support_center, cfg.support_radius_base, cfg.circle_segments));
                }
            }

            GreenspaceCandidate green{};
            green.polygon = makeCircle(v->pos, buffer_r, cfg.circle_segments);
            green.score = greenspaceScore(g, vid, 1.0f + 0.25f * boost);
            out.greenspace_candidates.push_back(std::move(green));
        }

        return out;
    }

} // namespace RogueCity::Generators::Roads
