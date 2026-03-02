#include "RogueCity/Generators/Urban/PolygonFinder.hpp"
#include "RogueCity/Generators/Urban/PolygonUtil.hpp"

#include <algorithm>
#include <unordered_map>

namespace RogueCity::Generators::Urban {

// Converts district borders into block polygons.
// Borders are normalized to closed rings for downstream polygon operations.
std::vector<Core::BlockPolygon>
PolygonFinder::fromDistricts(const std::vector<Core::District> &districts) {
  std::vector<Core::BlockPolygon> blocks;
  blocks.reserve(districts.size());
  for (const auto &district : districts) {
    if (district.border.size() < 3) {
      continue;
    }
    Core::BlockPolygon block;
    block.district_id = district.id;
    block.outer = PolygonUtil::closed(district.border);
    blocks.push_back(std::move(block));
  }
  return blocks;
}

// Road-graph-aware block extraction — Phase 2 implementation.
// Full planar face traversal (DCEL / Half-edge) to extract minimal-cycle
// polygons directly from the road graph topology.
std::vector<Core::BlockPolygon>
PolygonFinder::fromGraph(const Graph &graph,
                         const std::vector<Core::District> &districts) {

  if (graph.edges().empty() || graph.vertices().empty()) {
    return fromDistricts(districts);
  }

  struct HalfEdge {
    uint32_t id;
    VertexID source;
    VertexID target;
    uint32_t twin_id;
    uint32_t next_id = ~0u;
    double angle;                  // Outgoing angle from source
    std::vector<Core::Vec2> shape; // Forward path from source to target
    bool visited = false;
  };

  std::vector<HalfEdge> half_edges;
  half_edges.reserve(graph.edges().size() * 2);

  // Map vertex ID to list of outgoing half-edge IDs
  std::unordered_map<VertexID, std::vector<uint32_t>> adj;

  // Build half-edges
  for (const auto &e : graph.edges()) {
    if (e.a == e.b)
      continue; // Skip self-loops

    const auto *vA = graph.getVertex(e.a);
    const auto *vB = graph.getVertex(e.b);
    if (!vA || !vB)
      continue;

    uint32_t idAB = static_cast<uint32_t>(half_edges.size());
    uint32_t idBA = idAB + 1;

    HalfEdge heAB;
    heAB.id = idAB;
    heAB.source = e.a;
    heAB.target = e.b;
    heAB.twin_id = idBA;
    heAB.shape = e.shape;

    HalfEdge heBA;
    heBA.id = idBA;
    heBA.source = e.b;
    heBA.target = e.a;
    heBA.twin_id = idAB;
    heBA.shape = e.shape;
    std::reverse(heBA.shape.begin(), heBA.shape.end());

    // Calculate outgoing angles
    Core::Vec2 dirAB, dirBA;
    if (e.shape.size() >= 2) {
      dirAB = {e.shape[1].x - e.shape[0].x, e.shape[1].y - e.shape[0].y};
      dirBA = {e.shape[e.shape.size() - 2].x - e.shape.back().x,
               e.shape[e.shape.size() - 2].y - e.shape.back().y};
    } else {
      dirAB = {vB->pos.x - vA->pos.x, vB->pos.y - vA->pos.y};
      dirBA = {vA->pos.x - vB->pos.x, vA->pos.y - vB->pos.y};
    }

    heAB.angle = std::atan2(dirAB.y, dirAB.x);
    heBA.angle = std::atan2(dirBA.y, dirBA.x);

    half_edges.push_back(std::move(heAB));
    half_edges.push_back(std::move(heBA));

    adj[e.a].push_back(idAB);
    adj[e.b].push_back(idBA);
  }

  // Sort outgoing edges and set 'next' pointers (right-most turn)
  for (auto &pair : adj) {
    auto &out_edges = pair.second;
    // Sort CCW by angle
    std::sort(out_edges.begin(), out_edges.end(),
              [&half_edges](uint32_t a, uint32_t b) {
                return half_edges[a].angle < half_edges[b].angle;
              });

    // For each incoming edge, the "next" edge is the right-most outgoing edge.
    // In a CCW sorted adjacency list, the right-most outgoing edge from the
    // perspective of incoming edge E is the one that immediately precedes
    // E.twin in the sorted list.
    for (size_t i = 0; i < out_edges.size(); ++i) {
      uint32_t outE = out_edges[i];
      uint32_t inE = half_edges[outE].twin_id;

      // The right-most turn from inE is the next outgoing edge in CW order,
      // which means the PREVIOUS one in our CCW sorted array.
      size_t prev_idx = (i == 0) ? out_edges.size() - 1 : i - 1;
      uint32_t nextE = out_edges[prev_idx];

      half_edges[inE].next_id = nextE;
    }
  }

  std::vector<Core::BlockPolygon> result;
  uint32_t poly_id = 1;

  // Traverse faces
  for (auto &he : half_edges) {
    if (he.visited || he.next_id == ~0u)
      continue;

    std::vector<Core::Vec2> face_ring;
    uint32_t curr = he.id;

    do {
      half_edges[curr].visited = true;

      // Append shape of curr half-edge, skipping the last point to avoid
      // duplication
      const auto &shape = half_edges[curr].shape;
      if (!shape.empty()) {
        for (size_t i = 0; i < shape.size() - 1; ++i) {
          face_ring.push_back(shape[i]);
        }
      } else {
        face_ring.push_back(graph.getVertex(half_edges[curr].source)->pos);
      }

      curr = half_edges[curr].next_id;
    } while (curr != he.id && !half_edges[curr].visited);

    // Cycle detected
    if (curr == he.id && face_ring.size() >= 3) {
      // Determine if it's an inner face (positive area for typical right-hand
      // traversal) Actually, the easiest way to filter the infinite outer face
      // is by signed area. A right-most traversal of inner faces yields CW
      // perimeter (negative area in Y-up, pos in Y-down). Let's rely on
      // standard Shoelace formula which the prompt identified outer as
      // 'negative signed area'.
      double a = PolygonUtil::area(face_ring);
      if (a > 0.0) { // Keep positive area faces (internal blocks)
        Core::BlockPolygon bp;
        bp.district_id =
            poly_id++; // Temporary ID, could map to nearest district if needed
        bp.outer = PolygonUtil::closed(face_ring);
        result.push_back(std::move(bp));
      }
    }
  }

  // If no blocks were extracted (e.g., disjoint graph), fallback to districts
  if (result.empty()) {
    return fromDistricts(districts);
  }

  // Optional post-process: assign nearest District ID to each block
  for (auto &block : result) {
    if (block.outer.empty())
      continue;
    Core::Vec2 centroid = PolygonUtil::centroid(block.outer);
    uint32_t best_district = 0;
    double best_dist2 = std::numeric_limits<double>::max();

    for (const auto &d : districts) {
      if (d.border.empty())
        continue;
      Core::Vec2 dc = PolygonUtil::centroid(d.border);
      double dx = centroid.x - dc.x;
      double dy = centroid.y - dc.y;
      double dist2 = dx * dx + dy * dy;
      if (dist2 < best_dist2) {
        best_dist2 = dist2;
        best_district = d.id;
      }
    }
    block.district_id = best_district;
  }

  return result;
}

} // namespace RogueCity::Generators::Urban
