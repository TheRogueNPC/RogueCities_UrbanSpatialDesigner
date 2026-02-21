#include "RogueCity/Generators/Roads/GraphSimplify.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace RogueCity::Generators::Roads {

    namespace {

        // Computes edge length from cached length, shape polyline, or endpoint distance.
        [[nodiscard]] float edgeLength(const Urban::Edge& e, const Urban::Graph& g) {
            if (e.length > 0.0f) {
                return e.length;
            }
            if (!e.shape.empty()) {
                double total = 0.0;
                for (size_t i = 1; i < e.shape.size(); ++i) {
                    total += e.shape[i - 1].distanceTo(e.shape[i]);
                }
                return static_cast<float>(total);
            }

            const auto* a = g.getVertex(e.a);
            const auto* b = g.getVertex(e.b);
            if (a == nullptr || b == nullptr) {
                return 0.0f;
            }
            return static_cast<float>(a->pos.distanceTo(b->pos));
        }

        // Hierarchy rank helper for retaining strongest class during merges.
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

        // Keeps the higher-ranked road class when collapsing edges.
        [[nodiscard]] Core::RoadType maxRoadClass(Core::RoadType a, Core::RoadType b) {
            return roadRank(a) >= roadRank(b) ? a : b;
        }

        // Recomputes vertex kind tags from current adjacency degree.
        void refreshVertexKinds(Urban::Graph& g) {
            for (Urban::VertexID vid = 0; vid < g.vertices().size(); ++vid) {
                auto* v = g.getVertexMutable(vid);
                if (v == nullptr) {
                    continue;
                }
                const size_t degree = v->edges.size();
                if (degree <= 1) {
                    v->kind = Urban::VertexKind::DeadEnd;
                } else if (degree >= 3) {
                    if (v->kind != Urban::VertexKind::Portal) {
                        v->kind = Urban::VertexKind::Intersection;
                    }
                } else if (v->kind != Urban::VertexKind::Portal) {
                    v->kind = Urban::VertexKind::Normal;
                }
            }
        }

        // Vertex welding pass: merges close same-layer vertices and remaps edges.
        Urban::Graph weldVertices(const Urban::Graph& in, const SimplifyConfig& cfg) {
            Urban::Graph out;
            if (in.vertices().empty()) {
                return out;
            }

            std::vector<Urban::VertexID> parent(in.vertices().size());
            for (Urban::VertexID i = 0; i < parent.size(); ++i) {
                parent[i] = i;
            }

            const double weld_r2 = static_cast<double>(cfg.weld_radius) * static_cast<double>(cfg.weld_radius);
            for (size_t i = 0; i < in.vertices().size(); ++i) {
                const auto& vi = in.vertices()[i];
                for (size_t j = i + 1; j < in.vertices().size(); ++j) {
                    if (parent[j] != j) {
                        continue;
                    }
                    const auto& vj = in.vertices()[j];
                    if (vi.layer_id != vj.layer_id) {
                        continue;
                    }
                    if (vi.pos.distanceToSquared(vj.pos) <= weld_r2) {
                        parent[j] = static_cast<Urban::VertexID>(i);
                    }
                }
            }

            std::vector<Urban::VertexID> remap(in.vertices().size(), std::numeric_limits<Urban::VertexID>::max());
            for (size_t i = 0; i < in.vertices().size(); ++i) {
                Urban::VertexID root = static_cast<Urban::VertexID>(i);
                while (parent[root] != root) {
                    root = parent[root];
                }
                if (remap[root] == std::numeric_limits<Urban::VertexID>::max()) {
                    Urban::Vertex v = in.vertices()[root];
                    v.edges.clear();
                    remap[root] = out.addVertex(v);
                }
                remap[i] = remap[root];
            }

            for (const auto& edge : in.edges()) {
                if (!out.isVertexValid(remap[edge.a]) || !out.isVertexValid(remap[edge.b])) {
                    continue;
                }
                Urban::Edge e = edge;
                e.a = remap[edge.a];
                e.b = remap[edge.b];
                if (e.a == e.b) {
                    continue;
                }
                e.length = edgeLength(e, in);
                if (e.length < 1e-3f) {
                    continue;
                }
                if (e.shape.empty()) {
                    const auto* a = out.getVertex(e.a);
                    const auto* b = out.getVertex(e.b);
                    if (a != nullptr && b != nullptr) {
                        e.shape = { a->pos, b->pos };
                    }
                }
                out.addEdge(e);
            }

            refreshVertexKinds(out);
            return out;
        }

        // Degree-2 collapse pass:
        // remove nearly-collinear intermediate vertex and replace by merged edge.
        bool collapseDegreeTwo(Urban::Graph& g, const SimplifyConfig& cfg) {
            struct PendingEdge {
                Urban::Edge e;
            };

            std::vector<bool> remove_vertices(g.vertices().size(), false);
            std::vector<bool> remove_edges(g.edges().size(), false);
            std::vector<PendingEdge> add_edges;

            const double pi = 3.14159265358979323846;
            bool changed = false;
            for (Urban::VertexID vid = 0; vid < g.vertices().size(); ++vid) {
                const auto* v = g.getVertex(vid);
                if (v == nullptr || v->kind == Urban::VertexKind::Portal || v->edges.size() != 2) {
                    continue;
                }

                const Urban::EdgeID e0_id = v->edges[0];
                const Urban::EdgeID e1_id = v->edges[1];
                if (!g.isEdgeValid(e0_id) || !g.isEdgeValid(e1_id) || remove_edges[e0_id] || remove_edges[e1_id]) {
                    continue;
                }

                const auto* e0 = g.getEdge(e0_id);
                const auto* e1 = g.getEdge(e1_id);
                if (e0 == nullptr || e1 == nullptr || e0->layer_id != e1->layer_id) {
                    continue;
                }

                const Urban::VertexID u = (e0->a == vid) ? e0->b : e0->a;
                const Urban::VertexID w = (e1->a == vid) ? e1->b : e1->a;
                if (u == w || !g.isVertexValid(u) || !g.isVertexValid(w)) {
                    continue;
                }

                const auto* vu = g.getVertex(u);
                const auto* vw = g.getVertex(w);
                if (vu == nullptr || vw == nullptr) {
                    continue;
                }

                Core::Vec2 v0 = (vu->pos - v->pos);
                Core::Vec2 v1 = (vw->pos - v->pos);
                const double l0 = v0.length();
                const double l1 = v1.length();
                if (l0 <= 1e-6 || l1 <= 1e-6) {
                    continue;
                }

                v0 /= l0;
                v1 /= l1;
                const double angle = std::acos(std::clamp(v0.dot(v1), -1.0, 1.0)) * (180.0 / pi);
                if (std::abs(180.0 - angle) > cfg.collapse_angle_deg) {
                    continue;
                }

                remove_vertices[vid] = true;
                remove_edges[e0_id] = true;
                remove_edges[e1_id] = true;

                Urban::Edge ne{};
                ne.a = u;
                ne.b = w;
                ne.layer_id = e0->layer_id;
                ne.type = maxRoadClass(e0->type, e1->type);
                ne.flow.v_base = std::max(e0->flow.v_base, e1->flow.v_base);
                ne.flow.cap_base = std::max(e0->flow.cap_base, e1->flow.cap_base);
                ne.flow.access_control = std::max(e0->flow.access_control, e1->flow.access_control);
                ne.shape = { vu->pos, v->pos, vw->pos };
                ne.length = edgeLength(ne, g);
                if (ne.length >= 1e-3f) {
                    add_edges.push_back({ ne });
                }
                changed = true;
            }

            if (!changed) {
                return false;
            }

            Urban::Graph compact;
            std::vector<Urban::VertexID> remap(g.vertices().size(), std::numeric_limits<Urban::VertexID>::max());
            for (Urban::VertexID vid = 0; vid < g.vertices().size(); ++vid) {
                if (remove_vertices[vid]) {
                    continue;
                }
                auto v = g.vertices()[vid];
                v.edges.clear();
                remap[vid] = compact.addVertex(v);
            }

            for (Urban::EdgeID eid = 0; eid < g.edges().size(); ++eid) {
                if (remove_edges[eid]) {
                    continue;
                }
                auto e = g.edges()[eid];
                if (!compact.isVertexValid(remap[e.a]) || !compact.isVertexValid(remap[e.b])) {
                    continue;
                }
                e.a = remap[e.a];
                e.b = remap[e.b];
                if (e.a == e.b) {
                    continue;
                }
                e.length = edgeLength(e, g);
                if (e.length < 1e-3f) {
                    continue;
                }
                compact.addEdge(e);
            }

            for (const auto& pending : add_edges) {
                auto e = pending.e;
                if (!compact.isVertexValid(remap[e.a]) || !compact.isVertexValid(remap[e.b])) {
                    continue;
                }
                e.a = remap[e.a];
                e.b = remap[e.b];
                if (e.a == e.b) {
                    continue;
                }
                compact.addEdge(e);
            }

            refreshVertexKinds(compact);
            g = std::move(compact);
            return true;
        }

        // Drops very short edges and compacts resulting graph.
        void pruneMicroEdges(Urban::Graph& g, float min_edge_length) {
            Urban::Graph compact;
            std::vector<Urban::VertexID> remap(g.vertices().size(), std::numeric_limits<Urban::VertexID>::max());

            for (Urban::EdgeID eid = 0; eid < g.edges().size(); ++eid) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr || e->length < min_edge_length) {
                    continue;
                }

                if (!compact.isVertexValid(remap[e->a])) {
                    auto va = g.vertices()[e->a];
                    va.edges.clear();
                    remap[e->a] = compact.addVertex(va);
                }
                if (!compact.isVertexValid(remap[e->b])) {
                    auto vb = g.vertices()[e->b];
                    vb.edges.clear();
                    remap[e->b] = compact.addVertex(vb);
                }

                Urban::Edge copied = *e;
                copied.a = remap[e->a];
                copied.b = remap[e->b];
                compact.addEdge(copied);
            }

            refreshVertexKinds(compact);
            g = std::move(compact);
        }

    } // namespace

    // Simplification pipeline: weld -> collapse (iterative) -> prune -> refresh.
    void simplifyGraph(Urban::Graph& g, const SimplifyConfig& cfg) {
        g = weldVertices(g, cfg);
        for (int i = 0; i < 3; ++i) {
            if (!collapseDegreeTwo(g, cfg)) {
                break;
            }
        }
        pruneMicroEdges(g, cfg.min_edge_length);
        refreshVertexKinds(g);
    }

} // namespace RogueCity::Generators::Roads
