#include "RogueCity/Generators/Roads/StreamlineTracer.hpp"
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"

#include <cassert>
#include <vector>

using RogueCity::Core::Vec2;
using RogueCity::Generators::StreamlineTracer;
using RogueCity::Generators::TensorFieldGenerator;

namespace {

bool PolylineEqual(const std::vector<Vec2>& a, const std::vector<Vec2>& b, double eps = 1e-6) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (!a[i].equals(b[i], eps)) {
            return false;
        }
    }
    return true;
}

} // namespace

int main() {
    TensorFieldGenerator::Config cfg{};
    cfg.width = 240;
    cfg.height = 240;
    cfg.cell_size = 10.0;

    TensorFieldGenerator field(cfg);
    field.addGridField(Vec2(1200.0, 1200.0), 600.0, 0.35, 2.0);
    field.addRadialField(Vec2(900.0, 900.0), 450.0, 8, 2.2);
    field.generateField();

    StreamlineTracer tracer{};
    StreamlineTracer::Params fixed{};
    fixed.step_size = 6.0;
    fixed.max_length = 500.0;
    fixed.max_iterations = 220;
    fixed.bidirectional = true;

    StreamlineTracer::Params adaptive = fixed;
    adaptive.adaptive_step_size = true;
    adaptive.min_step_size = 2.0;
    adaptive.max_step_size = 12.0;
    adaptive.curvature_gain = 2.2;

    const Vec2 seed(1100.0, 1100.0);
    const auto fixed_trace = tracer.traceMajor(seed, field, fixed);
    const auto adaptive_a = tracer.traceMajor(seed, field, adaptive);
    const auto adaptive_b = tracer.traceMajor(seed, field, adaptive);

    assert(!fixed_trace.empty());
    assert(!adaptive_a.empty());
    assert(PolylineEqual(adaptive_a, adaptive_b));

    bool differs_from_fixed = fixed_trace.size() != adaptive_a.size();
    if (!differs_from_fixed) {
        for (size_t i = 0; i < fixed_trace.size(); ++i) {
            if (!fixed_trace[i].equals(adaptive_a[i], 1e-4)) {
                differs_from_fixed = true;
                break;
            }
        }
    }
    assert(differs_from_fixed);

    std::vector<Vec2> dense_seeds{
        Vec2(1000.0, 1000.0),
        Vec2(1008.0, 1008.0),
        Vec2(1016.0, 1016.0),
        Vec2(1024.0, 1024.0)
    };

    auto unconstrained = fixed;
    unconstrained.enforce_network_separation = false;
    const auto roads_unconstrained = tracer.traceNetwork(dense_seeds, field, unconstrained);

    auto constrained = adaptive;
    constrained.enforce_network_separation = true;
    constrained.min_separation = 18.0;
    constrained.separation_cell_size = 18.0;
    const auto roads_constrained_a = tracer.traceNetwork(dense_seeds, field, constrained);
    const auto roads_constrained_b = tracer.traceNetwork(dense_seeds, field, constrained);

    assert(roads_constrained_a.size() <= roads_unconstrained.size());
    assert(roads_constrained_a.size() == roads_constrained_b.size());

    return 0;
}
