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
    return visual.to_axiom_input();
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
