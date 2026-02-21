#include "RogueCity/App/Integration/GeneratorBridge.hpp"
#include <cmath>

namespace RogueCity::App {

GeneratorBridge::GeneratorBridge() = default;

std::vector<Generators::CityGenerator::AxiomInput>
GeneratorBridge::convert_axioms(const std::vector<std::unique_ptr<AxiomVisual>>& visuals) {
    std::vector<Generators::CityGenerator::AxiomInput> inputs;
    inputs.reserve(visuals.size());
    
    for (const auto& visual : visuals) {
        inputs.push_back(convert_axiom(*visual));
    }
    
    return inputs;
}

Generators::CityGenerator::AxiomInput
GeneratorBridge::convert_axiom(const AxiomVisual& visual) {
    auto input = visual.to_axiom_input();
    const auto& lattice = visual.lattice();

    input.warp_lattice.topology_type = static_cast<int>(lattice.topology);
    input.warp_lattice.rows = lattice.rows;
    input.warp_lattice.cols = lattice.cols;
    input.warp_lattice.zone_inner_uv = lattice.zone_inner_uv;
    input.warp_lattice.zone_middle_uv = lattice.zone_middle_uv;
    input.warp_lattice.zone_outer_uv = lattice.zone_outer_uv;
    input.warp_lattice.vertices.clear();
    input.warp_lattice.vertices.reserve(lattice.vertices.size());
    for (const auto& vertex : lattice.vertices) {
        input.warp_lattice.vertices.push_back(vertex.world_pos);
    }

    return input;
}

bool GeneratorBridge::validate_axioms(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config) {
    
    // Validation rules:
    // 1. Axioms must be within city bounds
    // 2. Radii must be reasonable (50m - 1000m)
    // 3. No extreme overlap (>80% overlap is suspicious)
    
    for (const auto& axiom : axioms) {
        // Bounds check
        if (axiom.position.x < 0 || axiom.position.x > config.width ||
            axiom.position.y < 0 || axiom.position.y > config.height) {
            return false;  // Out of bounds
        }
        
        // Radius check
        if (axiom.radius < 50.0 || axiom.radius > 1000.0) {
            return false;  // Invalid radius
        }

        // Ring schema guardrails.
        if (axiom.ring_schema.core_ratio <= 0.0 ||
            axiom.ring_schema.falloff_ratio < axiom.ring_schema.core_ratio ||
            axiom.ring_schema.outskirts_ratio < axiom.ring_schema.falloff_ratio ||
            axiom.ring_schema.outskirts_ratio > 1.5 ||
            axiom.ring_schema.merge_band_ratio < 0.0 ||
            axiom.ring_schema.merge_band_ratio > 0.5) {
            return false;
        }

        // Lattice guardrails.
        if (axiom.warp_lattice.topology_type < 0 || axiom.warp_lattice.topology_type > 3) {
            return false;
        }
        if (axiom.warp_lattice.zone_inner_uv <= 0.0f ||
            axiom.warp_lattice.zone_middle_uv < axiom.warp_lattice.zone_inner_uv ||
            axiom.warp_lattice.zone_outer_uv < axiom.warp_lattice.zone_middle_uv ||
            axiom.warp_lattice.zone_outer_uv > 1.5f) {
            return false;
        }
        for (const auto& point : axiom.warp_lattice.vertices) {
            if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
                return false;
            }
        }

        const size_t vertex_count = axiom.warp_lattice.vertices.size();
        const int topology = axiom.warp_lattice.topology_type;
        if (topology == 0) {
            if (axiom.warp_lattice.rows <= 0 || axiom.warp_lattice.cols <= 0) {
                return false;
            }
            if (vertex_count != static_cast<size_t>(axiom.warp_lattice.rows * axiom.warp_lattice.cols)) {
                return false;
            }
        } else if (topology == 1 && vertex_count < 3) {
            return false;
        } else if (topology == 2 && vertex_count < 2) {
            return false;
        } else if (topology == 3 && vertex_count < 2) {
            return false;
        }
    }
    
    // Check for extreme overlaps
    for (size_t i = 0; i < axioms.size(); ++i) {
        for (size_t j = i + 1; j < axioms.size(); ++j) {
            const auto& a = axioms[i];
            const auto& b = axioms[j];
            
            const double dx = a.position.x - b.position.x;
            const double dy = a.position.y - b.position.y;
            const double dist = std::sqrt(dx * dx + dy * dy);
            const double combined_radius = static_cast<double>(a.radius + b.radius);
            
            // If distance < 20% of combined radius, it's extreme overlap
            if (dist < combined_radius * 0.2) {
                return false;  // Too much overlap
            }
        }
    }
    
    return true;  // All validations passed
}

double GeneratorBridge::compute_decay_from_rings(
    float ring1_radius, float ring2_radius, float ring3_radius) {
    
    // Decay parameter controls influence falloff
    // Higher decay = faster falloff
    // Compute from ring spacing: if rings are tightly packed, decay is high
    
    const float spacing1 = ring2_radius - ring1_radius;
    const float spacing2 = ring3_radius - ring2_radius;
    const float avg_spacing = (spacing1 + spacing2) * 0.5f;
    
    // Normalize by outer radius
    const float spacing_ratio = avg_spacing / ring3_radius;
    
    // Map to decay range [1.0, 4.0]
    // Tight spacing (0.1) → high decay (4.0)
    // Wide spacing (0.5) → low decay (1.0)
    const double decay = 1.0 + (0.5 - spacing_ratio) * 6.0;
    
    return std::max(1.0, std::min(decay, 4.0));
}

} // namespace RogueCity::App
