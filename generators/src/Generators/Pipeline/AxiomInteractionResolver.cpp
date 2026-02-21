#include "RogueCity/Generators/Pipeline/AxiomInteractionResolver.hpp"

#include <algorithm>
#include <cmath>

namespace RogueCity::Generators {
namespace {

[[nodiscard]] double CoreRadius(const CityGenerator::AxiomInput& axiom) {
    return std::max(8.0, axiom.radius * std::clamp(axiom.ring_schema.core_ratio, 0.05, 1.0));
}

[[nodiscard]] double FalloffRadius(const CityGenerator::AxiomInput& axiom) {
    return std::max(CoreRadius(axiom), axiom.radius * std::clamp(axiom.ring_schema.falloff_ratio, 0.05, 1.0));
}

} // namespace

AxiomInteractionResult AxiomInteractionResolver::Resolve(
    const std::vector<CityGenerator::AxiomInput>& axioms) const {
    AxiomInteractionResult result{};
    result.resolved_axioms = axioms;

    if (result.resolved_axioms.size() < 2) {
        return result;
    }

    for (int relax_pass = 0; relax_pass < 6; ++relax_pass) {
        bool changed = false;
        for (size_t i = 0; i < result.resolved_axioms.size(); ++i) {
            for (size_t j = i + 1; j < result.resolved_axioms.size(); ++j) {
                auto& a = result.resolved_axioms[i];
                auto& b = result.resolved_axioms[j];

                Core::Vec2 delta = b.position - a.position;
                double dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
                if (dist < 1e-6) {
                    delta = Core::Vec2(1.0, 0.0);
                    dist = 1.0;
                }
                const Core::Vec2 unit = delta / dist;

                const double core_sum = CoreRadius(a) + CoreRadius(b);
                if (dist < core_sum) {
                    const double correction = (core_sum - dist) * 0.5 + 0.5;
                    a.position -= unit * correction;
                    b.position += unit * correction;
                    changed = true;
                    continue;
                }

                const double falloff_sum = FalloffRadius(a) + FalloffRadius(b);
                if (dist < falloff_sum) {
                    const Core::Vec2 midpoint = (a.position + b.position) * 0.5;
                    const Core::Vec2 normal(-unit.y, unit.x);
                    const double half_len = std::min(FalloffRadius(a), FalloffRadius(b)) * 0.40;
                    AxiomInteractionEdge edge{};
                    edge.axiom_a_id = a.id;
                    edge.axiom_b_id = b.id;
                    edge.border_start = midpoint - normal * half_len;
                    edge.border_end = midpoint + normal * half_len;
                    result.falloff_borders.push_back(edge);
                }
            }
        }
        if (!changed) {
            break;
        }
    }

    return result;
}

} // namespace RogueCity::Generators

