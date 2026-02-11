#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Data/CitySpec.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/CitySpecAdapter.hpp"
#include "RogueCity/Generators/Roads/RoadNoder.hpp"
#include "RogueCity/Generators/Roads/GraphSimplify.hpp"
#include "RogueCity/Generators/Roads/FlowAndControl.hpp"
#include "RogueCity/Generators/Roads/Verticality.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"
#include <cassert>
#include <iostream>

using namespace RogueCity;
using namespace RogueCity::Generators;

int main() {
    std::cout << "=== RogueCity Generators Test ===" << std::endl;

    // Create axioms
    std::vector<CityGenerator::AxiomInput> axioms;

    // Radial axiom at city center (Paris-style)
    CityGenerator::AxiomInput radial;
    radial.type = CityGenerator::AxiomInput::Type::Radial;
    radial.position = Core::Vec2(1000, 1000);
    radial.radius = 500.0;
    radial.decay = 2.0;
    axioms.push_back(radial);

    // Grid axiom offset (Manhattan-style)
    CityGenerator::AxiomInput grid;
    grid.type = CityGenerator::AxiomInput::Type::Grid;
    grid.position = Core::Vec2(1500, 500);
    grid.radius = 400.0;
    grid.theta = 0.0;  // North-south alignment
    grid.decay = 2.0;
    axioms.push_back(grid);

    // Generate city
    CityGenerator generator;
    CityGenerator::Config config;
    config.width = 2000;
    config.height = 2000;
    config.cell_size = 10.0;
    config.num_seeds = 15;
    config.seed = 42;

    std::cout << "Generating city..." << std::endl;
    auto output = generator.generate(axioms, config);

    std::cout << "Tensor field generated: " << output.tensor_field.getWidth()
        << "x" << output.tensor_field.getHeight() << " cells" << std::endl;
    std::cout << "Roads traced: " << output.roads.size() << " roads" << std::endl;
    std::cout << "Buildable area: " << (output.site_profile.buildable_fraction * 100.0f) << "%" << std::endl;
    std::cout << "Generation mode: " << static_cast<int>(output.site_profile.mode) << std::endl;
    std::cout << "Plan approved: " << (output.plan_approved ? "yes" : "no")
              << " (violations=" << output.plan_violations.size() << ")" << std::endl;
    assert(output.world_constraints.isValid());

    // Sample tensors at test points
    Core::Vec2 test_points[] = {
        Core::Vec2(1000, 1000),  // Center (radial)
        Core::Vec2(1500, 500),   // Grid center
        Core::Vec2(500, 500),    // Corner
    };

    std::cout << "\n=== Tensor Sampling ===" << std::endl;
    for (const auto& pt : test_points) {
        Core::Tensor2D t = output.tensor_field.sampleTensor(pt);
        Core::Vec2 major = t.majorEigenvector();
        std::cout << "  Point (" << pt.x << ", " << pt.y << "): "
            << "major=(" << major.x << ", " << major.y << ")" << std::endl;
    }

    // Road statistics
    if (output.roads.size() > 0) {
        std::cout << "\n=== Road Statistics ===" << std::endl;
        int major_count = 0;
        int minor_count = 0;
        double total_length = 0.0;

        for (const auto& road : output.roads) {
            if (road.type == Core::RoadType::M_Major) major_count++;
            if (road.type == Core::RoadType::M_Minor) minor_count++;
            total_length += road.length();
        }

        std::cout << "  Major roads: " << major_count << std::endl;
        std::cout << "  Minor roads: " << minor_count << std::endl;
        std::cout << "  Total length: " << total_length << " meters" << std::endl;
    }

    std::cout << "\n=== CitySpec Adapter Test ===" << std::endl;
    Core::CitySpec spec;
    spec.intent.description = "Coastal mixed-use city with dense downtown";
    spec.intent.scale = "city";
    spec.intent.climate = "temperate";
    spec.intent.styleTags = {"modern", "coastal"};
    spec.districts.push_back({"downtown", 0.9f});
    spec.districts.push_back({"residential", 0.55f});
    spec.seed = 2026;
    spec.roadDensity = 0.72f;

    auto request = CitySpecAdapter::BuildRequest(spec);
    if (request.axioms.empty()) {
        std::cerr << "CitySpec adapter failed to produce axioms" << std::endl;
        return 1;
    }

    auto spec_output = generator.generate(request.axioms, request.config);
    std::cout << "CitySpec generated axioms: " << request.axioms.size() << std::endl;
    std::cout << "CitySpec config: " << request.config.width << "x" << request.config.height
              << " seeds=" << request.config.num_seeds << std::endl;
    std::cout << "CitySpec roads traced: " << spec_output.roads.size() << std::endl;
    assert(spec_output.world_constraints.isValid());

    std::cout << "\nPhase 2 generators test PASSED" << std::endl;

    std::cout << "\n=== Road Noding Determinism ===" << std::endl;
    {
        Roads::NoderConfig noder_cfg;
        noder_cfg.max_layers = 2;
        noder_cfg.global_weld_radius = 0.25f;
        noder_cfg.type_params.resize(Core::road_type_count);
        for (auto& params : noder_cfg.type_params) {
            params.weld_radius = 0.25f;
        }
        Roads::RoadNoder noder(noder_cfg);

        Roads::PolylineRoadCandidate a;
        a.layer_hint = 0;
        a.pts = { Core::Vec2(0.0, 5.0), Core::Vec2(10.0, 5.0) };

        Roads::PolylineRoadCandidate b;
        b.layer_hint = 0;
        b.pts = { Core::Vec2(5.0, 0.0), Core::Vec2(5.0, 10.0) };

        Urban::Graph graph;
        noder.buildGraph({ a, b }, graph);
        bool found_intersection = false;
        for (const auto& v : graph.vertices()) {
            if (v.pos.distanceTo(Core::Vec2(5.0, 5.0)) < 1e-3 && v.edges.size() >= 4) {
                found_intersection = true;
                break;
            }
        }
        assert(found_intersection);

        b.layer_hint = 1;
        Urban::Graph split_layers;
        noder.buildGraph({ a, b }, split_layers);
        size_t layer0_vertices = 0;
        size_t layer1_vertices = 0;
        for (const auto& v : split_layers.vertices()) {
            if (v.layer_id == 0) {
                ++layer0_vertices;
            } else if (v.layer_id == 1) {
                ++layer1_vertices;
            }
        }
        assert(layer0_vertices > 0 && layer1_vertices > 0);
        assert(split_layers.edges().size() == 2);
    }

    std::cout << "\n=== Graph Simplification ===" << std::endl;
    {
        Urban::Graph g;
        Urban::VertexID v0 = g.addVertex({ Core::Vec2(0.0, 0.0) });
        Urban::VertexID v1 = g.addVertex({ Core::Vec2(1.0, 0.0) });
        Urban::VertexID v2 = g.addVertex({ Core::Vec2(12.0, 0.0) });

        Urban::Edge e0{};
        e0.a = v0;
        e0.b = v1;
        e0.length = 1.0f;
        e0.shape = { Core::Vec2(0.0, 0.0), Core::Vec2(1.0, 0.0) };
        g.addEdge(e0);

        Urban::Edge e1{};
        e1.a = v1;
        e1.b = v2;
        e1.length = 11.0f;
        e1.shape = { Core::Vec2(1.0, 0.0), Core::Vec2(12.0, 0.0) };
        g.addEdge(e1);

        Roads::SimplifyConfig simplify_cfg;
        simplify_cfg.min_edge_length = 5.0f;
        Roads::simplifyGraph(g, simplify_cfg);
        assert(!g.edges().empty());
        for (const auto& edge : g.edges()) {
            assert(edge.length >= simplify_cfg.min_edge_length - 1e-3f);
        }
    }

    std::cout << "\n=== Control Ladder ===" << std::endl;
    {
        Urban::Graph g;
        const Urban::VertexID c = g.addVertex({ Core::Vec2(0.0, 0.0) });
        const Urban::VertexID a = g.addVertex({ Core::Vec2(0.0, 20.0) });
        const Urban::VertexID b = g.addVertex({ Core::Vec2(20.0, 0.0) });
        const Urban::VertexID d = g.addVertex({ Core::Vec2(-20.0, 0.0) });
        const Urban::VertexID e = g.addVertex({ Core::Vec2(0.0, -20.0) });

        auto make_edge = [&](Urban::VertexID v) {
            Urban::Edge edge{};
            edge.a = c;
            edge.b = v;
            edge.type = Core::RoadType::Street;
            edge.length = 20.0f;
            edge.shape = { g.getVertex(c)->pos, g.getVertex(v)->pos };
            g.addEdge(edge);
        };

        make_edge(a);
        make_edge(b);
        make_edge(d);
        make_edge(e);

        Roads::FlowControlConfig control_cfg;
        control_cfg.thresholds.uncontrolled_d_max = 0.05f;
        control_cfg.thresholds.uncontrolled_r_max = 0.05f;
        control_cfg.thresholds.yield_d_max = 0.05f;
        control_cfg.thresholds.yield_r_max = 0.05f;
        control_cfg.thresholds.allway_d_max = 0.12f;
        control_cfg.thresholds.allway_r_max = 0.12f;
        control_cfg.thresholds.signal_d_max = 1000.0f;
        control_cfg.thresholds.signal_r_max = 1000.0f;
        control_cfg.thresholds.roundabout_d_min = 0.1f;
        control_cfg.thresholds.roundabout_r_min = 0.1f;
        control_cfg.thresholds.interchange_d_min = 1000.0f;
        control_cfg.thresholds.interchange_r_min = 1000.0f;
        Roads::applyFlowAndControl(g, control_cfg);
        assert(g.getVertex(c)->control == Urban::ControlType::Roundabout);

        Roads::FlowControlConfig low_cfg;
        low_cfg.thresholds.uncontrolled_d_max = 1000.0f;
        low_cfg.thresholds.uncontrolled_r_max = 1000.0f;
        Roads::applyFlowAndControl(g, low_cfg);
        assert(g.getVertex(c)->control == Urban::ControlType::Uncontrolled);

        Roads::FlowControlConfig stop_cfg;
        stop_cfg.thresholds.uncontrolled_d_max = 0.0f;
        stop_cfg.thresholds.uncontrolled_r_max = 0.0f;
        stop_cfg.thresholds.yield_d_max = 0.0f;
        stop_cfg.thresholds.yield_r_max = 0.0f;
        stop_cfg.thresholds.allway_d_max = 1000.0f;
        stop_cfg.thresholds.allway_r_max = 1000.0f;
        stop_cfg.thresholds.signal_d_max = 1000.0f;
        stop_cfg.thresholds.signal_r_max = 1000.0f;
        Roads::applyFlowAndControl(g, stop_cfg);
        assert(g.getVertex(c)->control == Urban::ControlType::AllWayStop);
    }

    std::cout << "\n=== Grade Separation Decision ===" << std::endl;
    {
        Urban::Graph g;
        const Urban::VertexID center = g.addVertex({ Core::Vec2(0.0, 0.0) });
        const Urban::VertexID north = g.addVertex({ Core::Vec2(0.0, 30.0) });
        const Urban::VertexID south = g.addVertex({ Core::Vec2(0.0, -30.0) });
        const Urban::VertexID east = g.addVertex({ Core::Vec2(30.0, 0.0) });
        const Urban::VertexID west = g.addVertex({ Core::Vec2(-30.0, 0.0) });

        auto major_edge = [&](Urban::VertexID v) {
            Urban::Edge edge{};
            edge.a = center;
            edge.b = v;
            edge.type = Core::RoadType::Arterial;
            edge.length = 30.0f;
            edge.flow.v_base = 22.0f;
            edge.shape = { g.getVertex(center)->pos, g.getVertex(v)->pos };
            g.addEdge(edge);
        };

        major_edge(north);
        major_edge(south);
        major_edge(east);
        major_edge(west);

        Roads::FlowControlConfig control_cfg;
        Roads::applyFlowAndControl(g, control_cfg);

        Roads::VerticalityConfig vcfg;
        vcfg.max_layers = 2;
        Roads::applyVerticality(g, vcfg);

        bool has_portal = false;
        for (const auto& v : g.vertices()) {
            if (v.kind == Urban::VertexKind::Portal) {
                has_portal = true;
                break;
            }
        }
        assert(has_portal);
    }

    return 0;
}
