#include "RogueCity/Generators/Urban/Graph.hpp"

namespace RogueCity::Generators::Urban {

    void Graph::clear() {
        edges_.clear();
    }

    void Graph::addEdge(const Core::Vec2& a, const Core::Vec2& b) {
        if (a.equals(b)) {
            return;
        }
        edges_.push_back(Edge{ a, b });
    }

} // namespace RogueCity::Generators::Urban

