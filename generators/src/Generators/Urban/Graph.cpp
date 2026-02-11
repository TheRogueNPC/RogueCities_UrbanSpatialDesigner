#include "RogueCity/Generators/Urban/Graph.hpp"

#include <limits>

namespace RogueCity::Generators::Urban {

    namespace {

        [[nodiscard]] float polylineLength(const std::vector<Core::Vec2>& pts) {
            if (pts.size() < 2) {
                return 0.0f;
            }

            double total = 0.0;
            for (size_t i = 1; i < pts.size(); ++i) {
                total += pts[i - 1].distanceTo(pts[i]);
            }
            return static_cast<float>(total);
        }

    } // namespace

    void Graph::clear() {
        vertices_.clear();
        edges_.clear();
    }

    VertexID Graph::addVertex(const Vertex& v) {
        const VertexID id = static_cast<VertexID>(vertices_.size());
        vertices_.push_back(v);
        return id;
    }

    EdgeID Graph::addEdge(const Edge& e) {
        if (!isVertexValid(e.a) || !isVertexValid(e.b) || e.a == e.b) {
            return std::numeric_limits<EdgeID>::max();
        }

        Edge edge = e;
        if (edge.shape.empty()) {
            edge.shape.push_back(vertices_[edge.a].pos);
            edge.shape.push_back(vertices_[edge.b].pos);
        }
        if (edge.length <= 0.0f) {
            edge.length = polylineLength(edge.shape);
            if (edge.length <= 0.0f) {
                edge.length = static_cast<float>(vertices_[edge.a].pos.distanceTo(vertices_[edge.b].pos));
            }
        }

        const EdgeID id = static_cast<EdgeID>(edges_.size());
        edges_.push_back(std::move(edge));
        vertices_[e.a].edges.push_back(id);
        vertices_[e.b].edges.push_back(id);
        return id;
    }

    EdgeID Graph::addEdge(
        const Core::Vec2& a,
        const Core::Vec2& b,
        Core::RoadType type,
        int layer_id) {
        if (a.equals(b)) {
            return std::numeric_limits<EdgeID>::max();
        }
        Vertex va{};
        va.pos = a;
        va.layer_id = layer_id;
        Vertex vb{};
        vb.pos = b;
        vb.layer_id = layer_id;

        const VertexID ida = addVertex(va);
        const VertexID idb = addVertex(vb);

        Edge e{};
        e.a = ida;
        e.b = idb;
        e.type = type;
        e.layer_id = layer_id;
        e.shape = { a, b };
        e.length = static_cast<float>(a.distanceTo(b));
        return addEdge(e);
    }

    const Vertex* Graph::getVertex(VertexID id) const {
        if (!isVertexValid(id)) {
            return nullptr;
        }
        return &vertices_[id];
    }

    const Edge* Graph::getEdge(EdgeID id) const {
        if (!isEdgeValid(id)) {
            return nullptr;
        }
        return &edges_[id];
    }

    Vertex* Graph::getVertexMutable(VertexID id) {
        if (!isVertexValid(id)) {
            return nullptr;
        }
        return &vertices_[id];
    }

    Edge* Graph::getEdgeMutable(EdgeID id) {
        if (!isEdgeValid(id)) {
            return nullptr;
        }
        return &edges_[id];
    }

    bool Graph::isVertexValid(VertexID id) const {
        return id < vertices_.size();
    }

    bool Graph::isEdgeValid(EdgeID id) const {
        return id < edges_.size();
    }

} // namespace RogueCity::Generators::Urban
