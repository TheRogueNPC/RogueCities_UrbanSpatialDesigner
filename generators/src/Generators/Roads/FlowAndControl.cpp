#include "RogueCity/Generators/Roads/FlowAndControl.hpp"

#include "RogueCity/Generators/Urban/GraphAlgorithms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace RogueCity::Generators::Roads {

    namespace {

        [[nodiscard]] int roadRank(Core::RoadType type) {
            switch (type) {
                case Core::RoadType::Highway: return 9;
                case Core::RoadType::Arterial: return 8;
                case Core::RoadType::Avenue: return 7;
                case Core::RoadType::Boulevard: return 6;
                case Core::RoadType::Street: return 5;
                case Core::RoadType::Lane: return 4;
                case Core::RoadType::Alleyway: return 3;
                case Core::RoadType::CulDeSac: return 2;
                case Core::RoadType::Drive: return 1;
                case Core::RoadType::Driveway: return 0;
                case Core::RoadType::M_Major: return 8;
                case Core::RoadType::M_Minor: return 5;
                default: return 0;
            }
        }

        [[nodiscard]] const RoadFlowDefaults& defaultsForType(
            const FlowControlConfig& cfg,
            Core::RoadType type) {
            static const std::array<RoadFlowDefaults, Core::road_type_count> kBuiltin{{
                {33.0f, 6.0f, 1.0f, false, false}, // Highway
                {22.0f, 4.0f, 0.6f, true, true},   // Arterial
                {18.0f, 3.4f, 0.4f, true, true},   // Avenue
                {16.0f, 2.8f, 0.25f, true, true},  // Boulevard
                {13.0f, 2.0f, 0.2f, true, true},   // Street
                {8.0f, 1.0f, 0.0f, false, false},  // Lane
                {7.0f, 1.0f, 0.0f, false, false},  // Alleyway
                {6.0f, 0.9f, 0.0f, false, false},  // CulDeSac
                {9.0f, 1.1f, 0.1f, false, false},  // Drive
                {5.0f, 0.6f, 0.0f, false, false},  // Driveway
                {24.0f, 4.0f, 0.6f, true, true},   // M_Major
                {13.0f, 2.0f, 0.2f, true, true},   // M_Minor
            }};

            const size_t idx = static_cast<size_t>(type);
            if (idx < cfg.road_defaults.size()) {
                return cfg.road_defaults[idx];
            }
            return kBuiltin[std::min(idx, kBuiltin.size() - 1)];
        }

        [[nodiscard]] float curvatureMultiplier(const Urban::Edge& e) {
            if (e.shape.size() < 3) {
                return 1.0f;
            }

            double avg_turn = 0.0;
            size_t turns = 0;
            for (size_t i = 1; i + 1 < e.shape.size(); ++i) {
                Core::Vec2 a = e.shape[i] - e.shape[i - 1];
                Core::Vec2 b = e.shape[i + 1] - e.shape[i];
                const double la = a.length();
                const double lb = b.length();
                if (la <= 1e-6 || lb <= 1e-6) {
                    continue;
                }
                a /= la;
                b /= lb;
                const double cosv = std::clamp(a.dot(b), -1.0, 1.0);
                avg_turn += std::acos(cosv);
                ++turns;
            }
            if (turns == 0) {
                return 1.0f;
            }
            avg_turn /= static_cast<double>(turns);
            return std::clamp(1.0f - static_cast<float>(avg_turn * 0.35), 0.45f, 1.0f);
        }

        [[nodiscard]] float intersectionDensityMultiplier(const Urban::Graph& g, const Urban::Edge& e) {
            const auto* va = g.getVertex(e.a);
            const auto* vb = g.getVertex(e.b);
            if (va == nullptr || vb == nullptr) {
                return 1.0f;
            }
            const float avg_deg = static_cast<float>(va->edges.size() + vb->edges.size()) * 0.5f;
            const float penalty = std::max(0.0f, avg_deg - 2.0f) * 0.08f;
            return std::clamp(1.0f - penalty, 0.5f, 1.0f);
        }

        [[nodiscard]] float controlDelaySeconds(Urban::ControlType c) {
            switch (c) {
                case Urban::ControlType::Uncontrolled: return 0.5f;
                case Urban::ControlType::Yield: return 1.5f;
                case Urban::ControlType::TwoWayStop: return 4.0f;
                case Urban::ControlType::AllWayStop: return 6.0f;
                case Urban::ControlType::Signal: return 12.0f;
                case Urban::ControlType::Roundabout: return 5.0f;
                case Urban::ControlType::GradeSep: return 1.0f;
                case Urban::ControlType::Interchange: return 0.5f;
                case Urban::ControlType::None:
                default:
                    return 0.0f;
            }
        }

        [[nodiscard]] float nodeConflictComplexity(const Urban::Graph& g, Urban::VertexID vid) {
            const auto* v = g.getVertex(vid);
            if (v == nullptr) {
                return 1.0f;
            }
            const size_t degree = v->edges.size();
            float complexity = 1.0f;
            if (degree >= 5) {
                complexity += 0.8f;
            } else if (degree == 4) {
                complexity += 0.45f;
            } else if (degree == 3) {
                complexity += 0.2f;
            }

            // Angle sanity: acute crossings increase conflict.
            double min_angle = 3.14159265358979323846;
            for (size_t i = 0; i < degree; ++i) {
                const auto* ei = g.getEdge(v->edges[i]);
                if (ei == nullptr) {
                    continue;
                }
                const Urban::VertexID ni = (ei->a == vid) ? ei->b : ei->a;
                const auto* pni = g.getVertex(ni);
                if (pni == nullptr) {
                    continue;
                }
                Core::Vec2 di = pni->pos - v->pos;
                const double li = di.length();
                if (li <= 1e-6) {
                    continue;
                }
                di /= li;

                for (size_t j = i + 1; j < degree; ++j) {
                    const auto* ej = g.getEdge(v->edges[j]);
                    if (ej == nullptr) {
                        continue;
                    }
                    const Urban::VertexID nj = (ej->a == vid) ? ej->b : ej->a;
                    const auto* pnj = g.getVertex(nj);
                    if (pnj == nullptr) {
                        continue;
                    }
                    Core::Vec2 dj = pnj->pos - v->pos;
                    const double lj = dj.length();
                    if (lj <= 1e-6) {
                        continue;
                    }
                    dj /= lj;
                    const double angle = std::acos(std::clamp(di.dot(dj), -1.0, 1.0));
                    min_angle = std::min(min_angle, angle);
                }
            }

            if (min_angle < 0.5) { // < ~28.6deg
                complexity += 0.55f;
            } else if (min_angle < 0.8) {
                complexity += 0.3f;
            }
            return complexity;
        }

    } // namespace

    void applyFlowAndControl(Urban::Graph& g, const FlowControlConfig& cfg) {
        if (g.edges().empty()) {
            return;
        }

        const auto centrality = Urban::GraphAlgorithms::sampledEdgeCentrality(
            g,
            std::max<size_t>(16, g.vertices().size() / 2),
            2026u);

        float max_len = 0.0f;
        for (const auto& e : g.edges()) {
            max_len = std::max(max_len, e.length);
        }
        max_len = std::max(1.0f, max_len);

        for (Urban::EdgeID eid = 0; eid < g.edges().size(); ++eid) {
            auto* e = g.getEdgeMutable(eid);
            if (e == nullptr) {
                continue;
            }

            const auto& defaults = defaultsForType(cfg, e->type);
            e->flow.v_base = defaults.v_base;
            e->flow.cap_base = defaults.cap_base;
            e->flow.access_control = defaults.access_control;

            const float curvature_mult = curvatureMultiplier(*e);
            const float slope_mult = 1.0f;
            const float intersection_mult = intersectionDensityMultiplier(g, *e);
            const float control_mult = 1.0f;
            e->flow.v_eff = e->flow.v_base *
                cfg.district_speed_mult *
                cfg.zone_speed_mult *
                curvature_mult *
                slope_mult *
                intersection_mult *
                control_mult;
            e->flow.v_eff = std::max(2.0f, e->flow.v_eff);

            const auto* va = g.getVertex(e->a);
            const auto* vb = g.getVertex(e->b);
            const float degree_boost = (va != nullptr && vb != nullptr)
                ? static_cast<float>(va->edges.size() + vb->edges.size()) * 0.08f
                : 0.0f;
            const float length_norm = std::clamp(e->length / max_len, 0.05f, 1.0f);
            const float cent = eid < centrality.size() ? centrality[eid] : 0.0f;
            const float speed_mult = e->flow.v_eff / std::max(1.0f, e->flow.v_base);

            e->flow.flow_score =
                (0.45f * length_norm + 0.55f * cent) *
                (1.0f + degree_boost) *
                speed_mult *
                std::max(0.8f, defaults.cap_base * 0.5f);
        }

        for (Urban::VertexID vid = 0; vid < g.vertices().size(); ++vid) {
            auto* v = g.getVertexMutable(vid);
            if (v == nullptr) {
                continue;
            }

            float demand = 0.0f;
            float speed_sq_sum = 0.0f;
            int fastest_rank = -1;
            float fastest_design_speed = 0.0f;
            bool has_access_control = false;
            for (const auto eid : v->edges) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                demand += e->flow.flow_score;
                speed_sq_sum += (e->flow.v_eff * e->flow.v_eff);
                fastest_rank = std::max(fastest_rank, roadRank(e->type));
                fastest_design_speed = std::max(fastest_design_speed, e->flow.v_base);
                has_access_control = has_access_control || (e->flow.access_control >= 0.8f);
            }

            const float district_activity = 1.0f + std::min(0.7f, static_cast<float>(v->edges.size()) * 0.06f);
            const float frontage_pressure = 1.0f + std::min(0.8f, static_cast<float>(std::max<int>(0, fastest_rank - 4)) * 0.08f);
            v->demand_D = demand * district_activity * frontage_pressure;

            const float conflict = nodeConflictComplexity(g, vid);
            const float sight_penalty = 1.0f + std::max(0.0f, static_cast<float>(v->edges.size()) - 3.0f) * 0.06f;
            v->risk_R = speed_sq_sum * conflict * sight_penalty * 0.02f;

            const auto& t = cfg.thresholds;
            Urban::ControlType selected = Urban::ControlType::Uncontrolled;

            if (v->demand_D <= t.uncontrolled_d_max && v->risk_R <= t.uncontrolled_r_max) {
                selected = Urban::ControlType::Uncontrolled;
            } else if (v->demand_D <= t.yield_d_max && v->risk_R <= t.yield_r_max) {
                selected = has_access_control ? Urban::ControlType::TwoWayStop : Urban::ControlType::Yield;
            } else if (v->demand_D <= t.allway_d_max && v->risk_R <= t.allway_r_max) {
                selected = Urban::ControlType::AllWayStop;
            } else if (v->demand_D >= t.interchange_d_min && v->risk_R >= t.interchange_r_min &&
                       (has_access_control || fastest_design_speed >= 28.0f)) {
                selected = Urban::ControlType::Interchange;
            } else if (v->demand_D >= t.roundabout_d_min && v->risk_R >= t.roundabout_r_min && fastest_design_speed < 30.0f) {
                selected = Urban::ControlType::Roundabout;
            } else if (v->demand_D <= t.signal_d_max && v->risk_R <= t.signal_r_max) {
                selected = Urban::ControlType::Signal;
            } else {
                selected = has_access_control ? Urban::ControlType::GradeSep : Urban::ControlType::Signal;
            }

            if (fastest_design_speed >= 32.0f && selected != Urban::ControlType::Interchange) {
                selected = Urban::ControlType::GradeSep;
            }

            v->control = selected;
        }

        // Feed control-delay back into edge effective speed and flow score.
        for (Urban::EdgeID eid = 0; eid < g.edges().size(); ++eid) {
            auto* e = g.getEdgeMutable(eid);
            if (e == nullptr) {
                continue;
            }
            const auto* va = g.getVertex(e->a);
            const auto* vb = g.getVertex(e->b);
            const float da = va != nullptr ? controlDelaySeconds(va->control) : 0.0f;
            const float db = vb != nullptr ? controlDelaySeconds(vb->control) : 0.0f;
            const float delay = (da + db) * 0.5f;
            const float delay_mult = std::clamp(e->flow.v_base / (e->flow.v_base + delay), 0.35f, 1.0f);
            e->flow.v_eff *= delay_mult;
            e->flow.flow_score *= delay_mult;
        }
    }

} // namespace RogueCity::Generators::Roads
