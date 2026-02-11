#include "RogueCity/Generators/Roads/Verticality.hpp"

#include <algorithm>

namespace RogueCity::Generators::Roads {

    namespace {

        [[nodiscard]] int roadRank(Core::RoadType type) {
            switch (type) {
                case Core::RoadType::Highway: return 9;
                case Core::RoadType::Arterial: return 8;
                case Core::RoadType::Avenue: return 7;
                case Core::RoadType::Boulevard: return 6;
                case Core::RoadType::M_Major: return 8;
                default: return 0;
            }
        }

        [[nodiscard]] bool hasPortalNeighbor(const Urban::Graph& g, Urban::VertexID vid) {
            const auto* v = g.getVertex(vid);
            if (v == nullptr) {
                return false;
            }
            for (const auto eid : v->edges) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                const Urban::VertexID other = (e->a == vid) ? e->b : e->a;
                const auto* ov = g.getVertex(other);
                if (ov != nullptr && ov->kind == Urban::VertexKind::Portal) {
                    return true;
                }
            }
            return false;
        }

    } // namespace

    void applyVerticality(Urban::Graph& g, const VerticalityConfig& cfg) {
        if (cfg.max_layers <= 1 || g.vertices().empty()) {
            return;
        }

        for (Urban::EdgeID eid = 0; eid < g.edges().size(); ++eid) {
            auto* e = g.getEdgeMutable(eid);
            if (e == nullptr) {
                continue;
            }
            const auto* a = g.getVertex(e->a);
            const auto* b = g.getVertex(e->b);
            if (a == nullptr || b == nullptr) {
                continue;
            }
            if (a->layer_id != b->layer_id) {
                // Keep graph mostly planar per-layer: mismatched edge layers are resolved to source.
                e->layer_id = a->layer_id;
            } else {
                e->layer_id = a->layer_id;
            }
        }

        if (!cfg.emit_portals) {
            return;
        }

        for (Urban::VertexID vid = 0; vid < g.vertices().size(); ++vid) {
            auto* v = g.getVertexMutable(vid);
            if (v == nullptr || v->layer_id >= (cfg.max_layers - 1)) {
                continue;
            }
            if (v->edges.size() < 4 || hasPortalNeighbor(g, vid)) {
                continue;
            }

            int major_incident = 0;
            float incident_speed = 0.0f;
            for (const auto eid : v->edges) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                if (roadRank(e->type) >= 7) {
                    ++major_incident;
                }
                incident_speed = std::max(incident_speed, e->flow.v_base);
            }

            const float at_grade_cost =
                (v->demand_D * 0.45f) +
                (v->risk_R * 0.55f) +
                (incident_speed * 0.2f);
            const float grade_sep_cost =
                (40.0f * cfg.ramp_cost_mult) +
                (20.0f * cfg.visual_intrusion_weight) -
                (major_incident * 8.0f * cfg.grade_sep_bias);

            const bool should_grade_separate =
                major_incident >= 2 &&
                (at_grade_cost > grade_sep_cost ||
                 v->control == Urban::ControlType::Interchange ||
                 v->control == Urban::ControlType::GradeSep);

            if (!should_grade_separate) {
                continue;
            }

            v->control = (v->control == Urban::ControlType::Interchange)
                ? Urban::ControlType::Interchange
                : Urban::ControlType::GradeSep;

            Urban::Vertex portal{};
            portal.pos = Core::Vec2(v->pos.x + cfg.portal_offset, v->pos.y + cfg.portal_offset);
            portal.layer_id = std::min(v->layer_id + 1, cfg.max_layers - 1);
            portal.kind = Urban::VertexKind::Portal;
            portal.control = Urban::ControlType::GradeSep;
            const Urban::VertexID portal_id = g.addVertex(portal);

            Urban::Edge ramp{};
            ramp.a = vid;
            ramp.b = portal_id;
            ramp.type = Core::RoadType::Drive;
            ramp.layer_id = v->layer_id;
            ramp.shape = { v->pos, portal.pos };
            ramp.length = static_cast<float>(v->pos.distanceTo(portal.pos));
            ramp.flow.v_base = 10.0f;
            ramp.flow.cap_base = 0.6f;
            ramp.flow.access_control = 0.2f;
            ramp.flow.v_eff = 9.0f;
            ramp.flow.flow_score = 0.2f;
            g.addEdge(ramp);
        }
    }

} // namespace RogueCity::Generators::Roads
