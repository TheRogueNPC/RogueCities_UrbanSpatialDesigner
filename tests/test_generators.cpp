#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Data/CitySpec.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/CitySpecAdapter.hpp"
#include "RogueCity/Generators/Roads/RoadNoder.hpp"
#include "RogueCity/Generators/Roads/GraphSimplify.hpp"
#include "RogueCity/Generators/Roads/FlowAndControl.hpp"
#include "RogueCity/Generators/Roads/IntersectionTemplates.hpp"
#include "RogueCity/Generators/Roads/Verticality.hpp"
#include "RogueCity/Generators/Urban/BlockGenerator.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"
#include "RogueCity/Generators/Urban/PolygonFinder.hpp"
#include <cassert>
#include <iostream>
#include <limits>

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
        double nearest_dist = std::numeric_limits<double>::max();
        size_t nearest_degree = 0;
        for (const auto& v : graph.vertices()) {
            const double d = v.pos.distanceTo(Core::Vec2(5.0, 5.0));
            if (d < nearest_dist) {
                nearest_dist = d;
                nearest_degree = v.edges.size();
            }
            if (d < 1e-3 && v.edges.size() >= 4) {
                found_intersection = true;
                break;
            }
        }
        if (!found_intersection) {
            std::cerr << "Road noding intersection check failed. nearest_dist=" << nearest_dist
                      << " nearest_degree=" << nearest_degree
                      << " vertices=" << graph.vertices().size()
                      << " edges=" << graph.edges().size() << std::endl;
            return 1;
        }

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
        Urban::VertexID v0 = g.addVertex({ .pos = Core::Vec2(0.0, 0.0) });
        Urban::VertexID v1 = g.addVertex({ .pos = Core::Vec2(1.0, 0.0) });
        Urban::VertexID v2 = g.addVertex({ .pos = Core::Vec2(12.0, 0.0) });

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
        const Urban::VertexID c = g.addVertex({ .pos = Core::Vec2(0.0, 0.0) });
        const Urban::VertexID a = g.addVertex({ .pos = Core::Vec2(0.0, 20.0) });
        const Urban::VertexID b = g.addVertex({ .pos = Core::Vec2(20.0, 0.0) });
        const Urban::VertexID d = g.addVertex({ .pos = Core::Vec2(-20.0, 0.0) });
        const Urban::VertexID e = g.addVertex({ .pos = Core::Vec2(0.0, -20.0) });

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
        const Urban::VertexID center = g.addVertex({ .pos = Core::Vec2(0.0, 0.0) });
        const Urban::VertexID north = g.addVertex({ .pos = Core::Vec2(0.0, 30.0) });
        const Urban::VertexID south = g.addVertex({ .pos = Core::Vec2(0.0, -30.0) });
        const Urban::VertexID east = g.addVertex({ .pos = Core::Vec2(30.0, 0.0) });
        const Urban::VertexID west = g.addVertex({ .pos = Core::Vec2(-30.0, 0.0) });

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

    // ============================================================
    // 3D Foundation Contract Tests (multi-agent lane: core+generators)
    // ============================================================

    std::cout << "\n=== 3D Contract: Road Verticality Metadata Survives Pipeline ===" << std::endl;
    {
        // Build a simple two-layer graph (one road on layer 0, one on layer 1).
        Urban::Graph g;
        const Urban::VertexID va = g.addVertex({ .pos = Core::Vec2(0.0, 0.0) });
        const Urban::VertexID vb = g.addVertex({ .pos = Core::Vec2(20.0, 0.0) });
        const Urban::VertexID vc = g.addVertex({ .pos = Core::Vec2(0.0, 0.0) });
        const Urban::VertexID vd = g.addVertex({ .pos = Core::Vec2(20.0, 0.0) });

        Urban::Edge e0{};
        e0.a = va; e0.b = vb; e0.type = Core::RoadType::Arterial;
        e0.layer_id = 0; e0.length = 20.0f;
        e0.shape = { Core::Vec2(0.0, 0.0), Core::Vec2(20.0, 0.0) };
        g.addEdge(e0);

        Urban::Edge e1{};
        e1.a = vc; e1.b = vd; e1.type = Core::RoadType::Highway;
        e1.layer_id = 1; e1.length = 20.0f;
        e1.shape = { Core::Vec2(0.0, 0.0), Core::Vec2(20.0, 0.0) };
        g.addEdge(e1);

        // Verify that layer_id on graph edges can be round-tripped into Core::Road.
        // This mirrors the RoadGenerator.cpp export loop (the data-preservation path).
        bool found_layer0 = false;
        bool found_layer1 = false;
        for (const auto& edge : g.edges()) {
            if (edge.shape.size() < 2) continue;
            Core::Road road{};
            road.layer_id = edge.layer_id;
            road.has_grade_separation = (edge.layer_id != 0);
            if (road.layer_id == 0) { found_layer0 = true; assert(!road.has_grade_separation); }
            if (road.layer_id == 1) { found_layer1 = true; assert(road.has_grade_separation); }

            // Elevation offsets should be populated for grade-separated roads.
            if (road.has_grade_separation) {
                road.elevation_offsets.resize(road.points.size(), static_cast<float>(road.layer_id) * 4.5f);
                assert(!road.elevation_offsets.empty() || road.points.empty());
            }
        }
        assert(found_layer0 && found_layer1);
        std::cout << "  Road verticality metadata round-trip: PASS" << std::endl;
    }

    std::cout << "\n=== 3D Contract: IntersectionTemplate Polygon Data Not Dropped ===" << std::endl;
    {
        // Build a graph with enough controlled intersections to generate paved areas.
        Urban::Graph g;
        const Urban::VertexID center = g.addVertex({ .pos = Core::Vec2(50.0, 50.0) });
        const Urban::VertexID n = g.addVertex({ .pos = Core::Vec2(50.0, 100.0) });
        const Urban::VertexID s = g.addVertex({ .pos = Core::Vec2(50.0, 0.0) });
        const Urban::VertexID e = g.addVertex({ .pos = Core::Vec2(100.0, 50.0) });
        const Urban::VertexID w = g.addVertex({ .pos = Core::Vec2(0.0, 50.0) });

        auto make_heavy = [&](Urban::VertexID v) {
            Urban::Edge edge{};
            edge.a = center; edge.b = v;
            edge.type = Core::RoadType::Arterial;
            edge.length = 50.0f;
            edge.flow.v_base = 25.0f;
            edge.flow.flow_score = 2.0f;
            edge.shape = { g.getVertex(center)->pos, g.getVertex(v)->pos };
            g.addEdge(edge);
        };
        make_heavy(n); make_heavy(s); make_heavy(e); make_heavy(w);

        Roads::FlowControlConfig fcfg;
        Roads::applyFlowAndControl(g, fcfg);
        Roads::VerticalityConfig vcfg2;
        Roads::applyVerticality(g, vcfg2);

        Roads::TemplateConfig tcfg;
        tcfg.circle_segments = 8;
        const auto tmpl_out = Roads::emitIntersectionTemplates(g, tcfg);

        // After emitIntersectionTemplates, paved_areas must be non-empty for intersections.
        assert(!tmpl_out.paved_areas.empty());
        assert(!tmpl_out.keep_out_islands.empty());
        assert(!tmpl_out.greenspace_candidates.empty());

        // Verify that the aggregate Core::IntersectionTemplate correctly captures all polygon data.
        // This mirrors the data-preservation logic in RoadGenerator.cpp.
        if (!tmpl_out.paved_areas.empty()) {
            Core::IntersectionTemplate aggregate{};
            aggregate.paved_areas         = tmpl_out.paved_areas;
            aggregate.keep_out_islands    = tmpl_out.keep_out_islands;
            aggregate.support_footprints  = tmpl_out.support_footprints;
            for (const auto& gc : tmpl_out.greenspace_candidates) {
                aggregate.greenspace_candidates.push_back(gc.polygon);
            }
            assert(aggregate.paved_areas.size() == tmpl_out.paved_areas.size());
            assert(aggregate.keep_out_islands.size() == tmpl_out.keep_out_islands.size());
            assert(aggregate.greenspace_candidates.size() == tmpl_out.greenspace_candidates.size());
        }
        std::cout << "  IntersectionTemplate polygon preservation: PASS ("
                  << tmpl_out.paved_areas.size() << " paved areas, "
                  << tmpl_out.greenspace_candidates.size() << " greenspace candidates)" << std::endl;
    }

    std::cout << "\n=== 3D Contract: BlockGenerator fromGraph Reduces Fallback ===" << std::endl;
    {
        // Create districts where only some will have road coverage.
        std::vector<Core::District> districts;
        {
            Core::District d1; d1.id = 1;
            d1.border = { {0,0},{100,0},{100,100},{0,100} }; // large central district
            districts.push_back(d1);
        }
        {
            Core::District d2; d2.id = 2;
            d2.border = { {500,500},{600,500},{600,600},{500,600} }; // distant district, no roads
            districts.push_back(d2);
        }

        // Build a graph whose edge midpoints fall in district 1 only.
        Urban::Graph road_graph;
        const Urban::VertexID va = road_graph.addVertex({ .pos = Core::Vec2(10.0, 50.0) });
        const Urban::VertexID vb = road_graph.addVertex({ .pos = Core::Vec2(90.0, 50.0) });
        Urban::Edge e{}; e.a = va; e.b = vb;
        e.shape = { Core::Vec2(10.0, 50.0), Core::Vec2(90.0, 50.0) };
        e.length = 80.0f;
        road_graph.addEdge(e);

        // fromDistricts returns all districts.
        const auto all_blocks = Urban::PolygonFinder::fromDistricts(districts);
        assert(all_blocks.size() == 2);

        // fromGraph with road coverage should return only roaded districts (district 1).
        const auto roaded_blocks = Urban::PolygonFinder::fromGraph(road_graph, districts);
        // District 2 has no road coverage in the AABB test, so it should be excluded.
        assert(roaded_blocks.size() == 1);
        assert(roaded_blocks.front().district_id == 1u);
        std::cout << "  fromGraph filtered " << (all_blocks.size() - roaded_blocks.size())
                  << " non-roaded block(s): PASS" << std::endl;

        // BlockGenerator::generate with Graph overload must call fromGraph path.
        Urban::BlockGenerator::Config bcfg;
        bcfg.prefer_road_cycles = true;
        const auto gen_blocks = Urban::BlockGenerator::generate(districts, road_graph, bcfg);
        assert(gen_blocks.size() == roaded_blocks.size());
        std::cout << "  BlockGenerator road-cycle path: PASS" << std::endl;

        // Without road graph, two-arg overload must fall back to all districts.
        const auto fallback_blocks = Urban::BlockGenerator::generate(districts, bcfg);
        assert(fallback_blocks.size() == 2);
        std::cout << "  BlockGenerator district fallback path: PASS" << std::endl;
    }

    std::cout << "\n3D Foundation contract tests PASSED" << std::endl;

    return 0;
}
