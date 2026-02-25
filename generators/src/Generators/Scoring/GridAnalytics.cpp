#include "RogueCity/Generators/Scoring/GridAnalytics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_set>
#include <array>

namespace RogueCity::Generators::Scoring {

namespace {
    const double kPi = 3.14159265358979323846;
    const double kDegToRad = kPi / 180.0;
    const double kRadToDeg = 180.0 / kPi;

    // Angle between two segments (0-180), where 0 is perfectly straight.
    [[nodiscard]] double AngleDegrees(const Core::Vec2& a, const Core::Vec2& b, const Core::Vec2& c) {
        Core::Vec2 v1 = (b - a);
        Core::Vec2 v2 = (c - b);
        const double l1 = v1.length();
        const double l2 = v2.length();
        if (l1 <= 1e-6 || l2 <= 1e-6) return 0.0;
        
        const double dot = (v1.x * v2.x + v1.y * v2.y) / (l1 * l2);
        return std::acos(std::clamp(dot, -1.0, 1.0)) * kRadToDeg;
    }
}

std::vector<Core::RoadStroke> GridAnalytics::ExtractStrokes(
    const Urban::Graph& graph, 
    float turn_tolerance_deg) {
    
    std::vector<Core::RoadStroke> strokes;
    std::unordered_set<Urban::EdgeID> visited_edges;
    
    for (Urban::EdgeID eid = 0; eid < graph.edges().size(); ++eid) {
        if (visited_edges.count(eid)) continue;
        
        const auto* start_edge = graph.getEdge(eid);
        if (start_edge == nullptr) continue;
        
        Core::RoadStroke stroke;
        stroke.type = start_edge->type;
        
        // Build stroke greedily in both directions.
        std::vector<Urban::EdgeID> stroke_edges = { eid };
        visited_edges.insert(eid);
        
        // Forward from b
        Urban::VertexID current_v = start_edge->b;
        Urban::EdgeID last_eid = eid;
        while (true) {
            const auto* v = graph.getVertex(current_v);
            if (v == nullptr || v->edges.size() != 2) break; // Terminate at intersection/dead-end
            
            Urban::EdgeID next_eid = (v->edges[0] == last_eid) ? v->edges[1] : v->edges[0];
            if (visited_edges.count(next_eid)) break;
            
            const auto* next_edge = graph.getEdge(next_eid);
            if (next_edge == nullptr || next_edge->type != stroke.type) break;
            
            // Check turn angle (must be fairly straight)
            const auto* prev_edge = graph.getEdge(last_eid);
            Urban::VertexID p_id = (prev_edge->a == current_v) ? prev_edge->b : prev_edge->a;
            Urban::VertexID n_id = (next_edge->a == current_v) ? next_edge->b : next_edge->a;
            
            if (AngleDegrees(graph.getVertex(p_id)->pos, v->pos, graph.getVertex(n_id)->pos) > turn_tolerance_deg) {
                break;
            }
            
            stroke_edges.push_back(next_eid);
            visited_edges.insert(next_eid);
            last_eid = next_eid;
            current_v = n_id;
        }
        
        // Backward from a (Reverse and then continue)
        std::reverse(stroke_edges.begin(), stroke_edges.end());
        current_v = start_edge->a;
        last_eid = eid;
        while (true) {
            const auto* v = graph.getVertex(current_v);
            if (v == nullptr || v->edges.size() != 2) break;
            
            Urban::EdgeID next_eid = (v->edges[0] == last_eid) ? v->edges[1] : v->edges[0];
            if (visited_edges.count(next_eid)) break;
            
            const auto* next_edge = graph.getEdge(next_eid);
            if (next_edge == nullptr || next_edge->type != stroke.type) break;
            
            const auto* prev_edge = graph.getEdge(last_eid);
            Urban::VertexID p_id = (prev_edge->a == current_v) ? prev_edge->b : prev_edge->a;
            Urban::VertexID n_id = (next_edge->a == current_v) ? next_edge->b : next_edge->a;
            
            if (AngleDegrees(graph.getVertex(p_id)->pos, v->pos, graph.getVertex(n_id)->pos) > turn_tolerance_deg) {
                break;
            }
            
            stroke_edges.push_back(next_eid);
            visited_edges.insert(next_eid);
            last_eid = next_eid;
            current_v = n_id;
        }
        
        // Materialize points
        if (stroke_edges.empty()) continue;
        
        // Re-orient edges to form a single continuous point list
        Urban::VertexID next_v = graph.getEdge(stroke_edges.front())->a;
        // Check if a is actually the connection to the rest (we need the outer leaf first)
        // If we have more than one edge, pick the vertex not shared with the second edge
        if (stroke_edges.size() > 1) {
            const auto* e0 = graph.getEdge(stroke_edges[0]);
            const auto* e1 = graph.getEdge(stroke_edges[1]);
            if (e0->b == e1->a || e0->b == e1->b) {
                next_v = e0->a;
            } else {
                next_v = e0->b;
            }
        }
        
        stroke.points.push_back(graph.getVertex(next_v)->pos);
        for (auto seid : stroke_edges) {
            const auto* e = graph.getEdge(seid);
            Urban::VertexID entry_v = next_v;
            next_v = (e->a == entry_v) ? e->b : e->a;
            
            // Add internal shape points if they exist (preserving curvatures)
            if (e->shape.size() > 2) {
                // If the edge direction is reversed (starts at b), add shape points in reverse
                if (e->a != entry_v) {
                    for (int i = static_cast<int>(e->shape.size()) - 2; i >= 1; --i) {
                        stroke.points.push_back(e->shape[i]);
                    }
                } else {
                    for (size_t i = 1; i < e->shape.size() - 1; ++i) {
                        stroke.points.push_back(e->shape[i]);
                    }
                }
            }
            stroke.points.push_back(graph.getVertex(next_v)->pos);
        }
        
        // Calculate length/displacement
        stroke.total_length = 0.0f;
        for (size_t i = 1; i < stroke.points.size(); ++i) {
            stroke.total_length += static_cast<float>(stroke.points[i-1].distanceTo(stroke.points[i]));
        }
        stroke.displacement = static_cast<float>(stroke.points.front().distanceTo(stroke.points.back()));
        
        strokes.push_back(std::move(stroke));
    }
    
    return strokes;
}

float GridAnalytics::CalculateOrientationOrder(const std::vector<Core::RoadStroke>& strokes) {
    if (strokes.empty()) return 0.0f;
    
    // Bin bearings (0-180) into 36 bins of 5 degrees.
    std::array<double, 36> histogram{};
    histogram.fill(0.0);
    double total_weight = 0.0;
    
    for (const auto& stroke : strokes) {
        if (stroke.points.size() < 2) continue;
        float bearing = stroke.bearing_degrees();
        int bin = std::clamp(static_cast<int>(bearing / 5.0f), 0, 35);
        
        // Weight by length to prioritize major network alignment.
        histogram[bin] += static_cast<double>(stroke.total_length);
        total_weight += static_cast<double>(stroke.total_length);
    }
    
    if (total_weight <= 1e-4) return 0.0f;
    
    // Shannon Entropy
    double entropy = 0.0;
    for (double count : histogram) {
        if (count > 0.0) {
            double p = count / total_weight;
            entropy -= p * std::log(p);
        }
    }
    
    // Max entropy for 36 bins is ln(36)
    double h_max = std::log(36.0);
    return static_cast<float>(std::clamp(1.0 - (entropy / h_max), 0.0, 1.0));
}

void GridAnalytics::CalculateIntersectionStats(const Urban::Graph& graph, uint32_t& total, uint32_t& four_way) {
    total = 0;
    four_way = 0;
    
    for (const auto& v : graph.vertices()) {
        const size_t degree = v.edges.size();
        if (degree >= 3) {
            total++;
            if (degree == 4) {
                four_way++;
            }
        }
    }
}

float GridAnalytics::CalculateAverageStraightness(const std::vector<Core::RoadStroke>& strokes) {
    if (strokes.empty()) return 1.0f;
    
    double total_s = 0.0;
    double total_len = 0.0;
    
    for (const auto& stroke : strokes) {
        total_s += static_cast<double>(stroke.straightness() * stroke.total_length);
        total_len += static_cast<double>(stroke.total_length);
    }
    
    if (total_len <= 1e-4) return 1.0f;
    return static_cast<float>(total_s / total_len);
}

namespace {
    // Counts disconnected components in the road graph using BFS.
    uint32_t CountIslands(const Urban::Graph& graph) {
        if (graph.vertices().empty()) return 0;
        
        std::vector<bool> visited(graph.vertices().size(), false);
        uint32_t islands = 0;
        
        for (Urban::VertexID i = 0; i < graph.vertices().size(); ++i) {
            if (visited[i]) continue;
            
            islands++;
            std::vector<Urban::VertexID> q = { i };
            visited[i] = true;
            size_t head = 0;
            
            while(head < q.size()) {
                Urban::VertexID curr = q[head++];
                const auto* v = graph.getVertex(curr);
                if (!v) continue;
                
                for (auto eid : v->edges) {
                    const auto* e = graph.getEdge(eid);
                    if (!e) continue;
                    Urban::VertexID next = (e->a == curr) ? e->b : e->a;
                    if (!visited[next]) {
                        visited[next] = true;
                        q.push_back(next);
                    }
                }
            }
        }
        return islands;
    }
}

Core::GridQualityReport GridAnalytics::Evaluate(const Urban::Graph& graph) {
    Core::GridQualityReport report;
    if (graph.vertices().empty()) return report;

    std::vector<Core::RoadStroke> strokes = ExtractStrokes(graph);
    report.total_strokes = static_cast<uint32_t>(strokes.size());
    
    report.straightness = CalculateAverageStraightness(strokes);
    report.orientation_order = CalculateOrientationOrder(strokes);
    
    // Intersection & Dead-End Stats
    uint32_t val1 = 0;
    uint32_t val3plus = 0;
    for (const auto& v : graph.vertices()) {
        const size_t deg = v.edges.size();
        if (deg == 1) val1++;
        if (deg >= 3) {
            val3plus++;
            if (deg == 4) report.four_way_intersections++;
        }
    }
    report.total_intersections = val3plus;
    report.dead_end_proportion = static_cast<float>(val1) / static_cast<float>(std::max<size_t>(1, graph.vertices().size()));
    
    if (report.total_intersections > 0) {
        report.four_way_proportion = static_cast<float>(report.four_way_intersections) / static_cast<float>(report.total_intersections);
    }

    // Connectivity: Gamma Index (E / 3(V-2))
    const size_t v_count = graph.vertices().size();
    const size_t e_count = graph.edges().size();
    if (v_count > 2) {
        report.connectivity_index = static_cast<float>(e_count) / static_cast<float>(3 * (v_count - 2));
    }
    
    // Island Count
    report.island_count = CountIslands(graph);

    // Micro-segments Check
    for (const auto& e : graph.edges()) {
        if (e.length < 1.0f) report.micro_segment_count++;
    }
    
    // Composite Grid Index
    report.composite_index = std::pow(report.straightness * report.orientation_order * report.four_way_proportion, 1.0f / 3.0f);
    
    return report;
}

} // namespace RogueCity::Generators::Scoring
