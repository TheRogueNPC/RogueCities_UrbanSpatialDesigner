#pragma once

#include "RogueCity/Core/Types.hpp"

#include <cstdint>
#include <vector>

namespace RogueCity::Generators::Urban {

    using VertexID = uint32_t;
    using EdgeID = uint32_t;

    enum class VertexKind : uint8_t {
        Normal = 0,
        DeadEnd,
        Intersection,
        RoundaboutCenter,
        Portal
    };

    enum class ControlType : uint8_t {
        None = 0,
        Uncontrolled,
        Yield,
        TwoWayStop,
        AllWayStop,
        Signal,
        Roundabout,
        GradeSep,
        Interchange
    };

    struct FlowStats {
        float v_base = 0.0f;
        float cap_base = 0.0f;
        float access_control = 0.0f;
        float v_eff = 0.0f;
        float flow_score = 0.0f;
    };

    struct Vertex {
        Core::Vec2 pos{};
        VertexKind kind = VertexKind::Normal;
        int layer_id = 0;
        std::vector<EdgeID> edges;

        float demand_D = 0.0f;
        float risk_R = 0.0f;
        ControlType control = ControlType::None;
    };

    struct Edge {
        VertexID a = 0;
        VertexID b = 0;

        Core::RoadType type = Core::RoadType::Street;
        int layer_id = 0;
        float length = 0.0f;
        std::vector<Core::Vec2> shape;
        FlowStats flow{};
    };

    class Graph {
    public:
        void clear();
        VertexID addVertex(const Vertex& v);
        EdgeID addEdge(const Edge& e);

        // Compatibility helper for legacy call sites that used raw line segments.
        EdgeID addEdge(
            const Core::Vec2& a,
            const Core::Vec2& b,
            Core::RoadType type = Core::RoadType::Street,
            int layer_id = 0);

        [[nodiscard]] const std::vector<Vertex>& vertices() const { return vertices_; }
        [[nodiscard]] const std::vector<Edge>& edges() const { return edges_; }

        [[nodiscard]] const Vertex* getVertex(VertexID id) const;
        [[nodiscard]] const Edge* getEdge(EdgeID id) const;
        [[nodiscard]] Vertex* getVertexMutable(VertexID id);
        [[nodiscard]] Edge* getEdgeMutable(EdgeID id);
        [[nodiscard]] bool isVertexValid(VertexID id) const;
        [[nodiscard]] bool isEdgeValid(EdgeID id) const;

    private:
        std::vector<Vertex> vertices_;
        std::vector<Edge> edges_;
    };

} // namespace RogueCity::Generators::Urban
