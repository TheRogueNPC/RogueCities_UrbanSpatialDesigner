#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "../Tools/AxiomVisual.hpp"
#include <vector>

namespace RogueCity::App {

/// Adapter between UI (AxiomVisual) and Generator (AxiomInput)
/// Handles data conversion and validation
class GeneratorBridge {
public:
    GeneratorBridge();

    /// Convert UI axioms to generator inputs
    [[nodiscard]] static std::vector<Generators::CityGenerator::AxiomInput>
    convert_axioms(const std::vector<std::unique_ptr<AxiomVisual>>& visuals);

    /// Convert single axiom
    [[nodiscard]] static Generators::CityGenerator::AxiomInput
    convert_axiom(const AxiomVisual& visual);

    /// Validate axiom inputs (check bounds, conflicts, etc.)
    [[nodiscard]] static bool validate_axioms(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config);

    /// Compute decay parameter from ring distribution
    [[nodiscard]] static double compute_decay_from_rings(
        float ring1_radius, float ring2_radius, float ring3_radius);

private:
    /// Screen-to-world coordinate conversion helpers
    [[nodiscard]] static Core::Vec2 screen_to_world(const Core::Vec2& screen_pos,
                                                      const class PrimaryViewport& viewport);
};

} // namespace RogueCity::App
