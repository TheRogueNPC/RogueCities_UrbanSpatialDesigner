#pragma once

#include "RogueCity/Core/Types.hpp"

#include <vector>

namespace RogueCity::Generators::Urban {

    class Graph {
    public:
        struct Edge {
            Core::Vec2 a{};
            Core::Vec2 b{};
        };

        void clear();
        void addEdge(const Core::Vec2& a, const Core::Vec2& b);
        [[nodiscard]] const std::vector<Edge>& edges() const { return edges_; }

    private:
        std::vector<Edge> edges_;
    };

} // namespace RogueCity::Generators::Urban

