#include "RogueCity/Generators/Urban/RCDTDGenerator.hpp"
#include "RogueCity/Generators/Urban/GridStorage.hpp"
#include "RogueCity/Generators/Urban/PolygonUtil.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>

namespace RogueCity::Generators::Urban {

namespace {

/**
 * @brief Checks if two line segments (a0-a1) and (b0-b1) intersect.
 */
bool SegmentsIntersect(const Core::Vec2 &a0, const Core::Vec2 &a1,
                       const Core::Vec2 &b0, const Core::Vec2 &b1) {
  auto ccw = [](const Core::Vec2 &A, const Core::Vec2 &B, const Core::Vec2 &C) {
    return (C.y - A.y) * (B.x - A.x) > (B.y - A.y) * (C.x - A.x);
  };
  return ccw(a0, b0, b1) != ccw(a1, b0, b1) &&
         ccw(a0, a1, b0) != ccw(a0, a1, b1);
}

[[nodiscard]] Core::Vec2 SafeNormalized(const Core::Vec2 &v) {
  Core::Vec2 out = v;
  if (out.length() <= 1e-9)
    return {};
  out.normalize();
  return out;
}

[[nodiscard]] Core::Vec2 ComputeRoadBoundsCenter(const Graph &graph) {
  if (graph.vertices().empty())
    return {1000.0, 1000.0};

  Core::Vec2 minP = graph.vertices().front().pos;
  Core::Vec2 maxP = graph.vertices().front().pos;
  for (const auto &v : graph.vertices()) {
    minP.x = std::min(minP.x, v.pos.x);
    minP.y = std::min(minP.y, v.pos.y);
    maxP.x = std::max(maxP.x, v.pos.x);
    maxP.y = std::max(maxP.y, v.pos.y);
  }

  return {(minP.x + maxP.x) * 0.5, (minP.y + maxP.y) * 0.5};
}

[[nodiscard]] double ComputeSignedPolygonArea(
    const std::vector<Core::Vec2> &boundary) {
  double area = 0.0;
  for (size_t i = 0; i < boundary.size(); ++i) {
    const auto &a = boundary[i];
    const auto &b = boundary[(i + 1) % boundary.size()];
    area += a.x * b.y - b.x * a.y;
  }
  return area * 0.5;
}

} // namespace

void RCDTDGenerator::SetConfig(const RCDTDConfig &config) { config_ = config; }

void RCDTDGenerator::SetStartingGraph(const Graph *graph) {
  startingGraph_ = graph;
}

void RCDTDGenerator::Generate() {
  roadGraph_.clear();
  blockGraph_.faces.clear();
  zoningResult_.assignments.clear();

  currentPhase_ = Phase::NodesAndEdges;
  generateNodesAndEdges();
  // Thesis Part 2: "split all existing edges to a smaller size" so that split
  // vertices become intersection anchors for infill inner streets.
  splitEdges(0);

  currentPhase_ = Phase::Faces;
  detectFaces();

  currentPhase_ = Phase::Infill;
  performRecursiveInfill();

  // Infill mutates the road graph, so faces must be rebuilt from the final
  // planar graph before zoning runs. Without this pass, zoning can observe the
  // pre-infill face set (often empty for tree-like skeletons).
  blockGraph_.faces.clear();
  detectFaces();

  currentPhase_ = Phase::Zoning;
  assignZoning();

  currentPhase_ = Phase::Done;
}

void RCDTDGenerator::GenerateStep() {
  switch (currentPhase_) {
  case Phase::Idle:
    roadGraph_.clear();
    blockGraph_.faces.clear();
    zoningResult_.assignments.clear();
    currentPhase_ = Phase::NodesAndEdges;
    generateNodesAndEdges();
    splitEdges(0); // Thesis Part 2: anchor nodes before face detection
    break;
  case Phase::NodesAndEdges:
    currentPhase_ = Phase::Faces;
    detectFaces();
    break;
  case Phase::Faces:
    currentPhase_ = Phase::Infill;
    performRecursiveInfill();
    blockGraph_.faces.clear();
    detectFaces();
    break;
  case Phase::Infill:
    currentPhase_ = Phase::Zoning;
    assignZoning();
    break;
  case Phase::Zoning:
    currentPhase_ = Phase::Done;
    break;
  case Phase::Done:
    currentPhase_ = Phase::Idle;
    break;
  }
}

void RCDTDGenerator::generateNodesAndEdges() {
  Core::RNG rng(config_.seed);
  const auto &lCfg = config_.levels.front();
  GridStorage nodeIndex(lCfg.clearance);

  struct CandidateNode {
    uint32_t id;
    float budget;
  };

  std::vector<CandidateNode> candidateNodes;
  double remainingGrowthBudget = std::max<double>(config_.growthBudget, 1.0);

  auto push_seed = [&](const Core::Vec2 &pos, float budget) {
    if (!nodeIndex.queryNear(pos, lCfg.clearance).empty())
      return;
    Vertex v;
    v.pos = pos;
    const uint32_t id = roadGraph_.addVertex(v);
    candidateNodes.push_back({id, budget});
    nodeIndex.insert(pos);
  };

  // 1. Initial Seeds
  const float avgSeedSpan = std::max(
      1.0f, (lCfg.extensionRange.min + lCfg.extensionRange.max) * 0.5f);
  const float baseSeedBudget = std::max(
      6.0f, static_cast<float>(config_.growthBudget / avgSeedSpan) * 0.18f);

  if (startingGraph_ && !startingGraph_->vertices().empty()) {
    for (const auto &v : startingGraph_->vertices()) {
      uint32_t id = roadGraph_.addVertex(v);
      candidateNodes.push_back({id, baseSeedBudget});
      nodeIndex.insert(v.pos);
    }
  } else {
    // Thesis default: central seed + deterministic radial capital seeds.
    // Budget-scaled rings create a stronger downtown scaffold without adding
    // nondeterministic branching policy.
    const Core::Vec2 center{1000.0, 1000.0};
    const double ringRadius = std::max<double>(
        lCfg.clearance * 2.5,
        (lCfg.extensionRange.min + lCfg.extensionRange.max) * 0.35);

    push_seed(center, baseSeedBudget + 2.0f);

    const std::vector<Core::Vec2> primaryDirs = {
        {1.0, 0.0}, {-1.0, 0.0}, {0.0, 1.0}, {0.0, -1.0}};
    for (const auto &dir : primaryDirs)
      push_seed(center + dir * ringRadius, baseSeedBudget);

    if (config_.growthBudget >= avgSeedSpan * 18.0f) {
      const double diagRadius = ringRadius * 0.82;
      const double invDiag = 0.7071067811865476;
      const std::vector<Core::Vec2> diagDirs = {
          { invDiag,  invDiag},
          {-invDiag,  invDiag},
          { invDiag, -invDiag},
          {-invDiag, -invDiag}};
      for (const auto &dir : diagDirs)
        push_seed(center + dir * diagRadius, baseSeedBudget - 1.0f);
    }

    if (config_.growthBudget >= avgSeedSpan * 28.0f) {
      const double outerRadius = ringRadius * 1.65;
      for (const auto &dir : primaryDirs)
        push_seed(center + dir * outerRadius, baseSeedBudget - 2.0f);
    }
  }

  // 2. Growth Driver
  bool changed = !candidateNodes.empty();
  while (changed && remainingGrowthBudget > lCfg.clearance) {
    changed = false;
    for (size_t i = 0; i < candidateNodes.size();) {
      CandidateNode &candidate = candidateNodes[i];
      const uint32_t originId = candidate.id;
      const Vertex *origin = roadGraph_.getVertex(originId);

      if (candidate.budget <= 0.0f ||
          (candidate.budget < 1.0f && rng.uniform() > candidate.budget)) {
        candidateNodes.erase(candidateNodes.begin() + i);
        continue;
      }

      if (origin->edges.size() >= 4) {
        candidateNodes.erase(candidateNodes.begin() + i);
        continue;
      }

      // Propose a direction (Investment Model)
      double currentAngle = rng.uniform(0, 2.0 * M_PI);
      if (!origin->edges.empty()) {
        const Edge *e = roadGraph_.getEdge(origin->edges.back());
        uint32_t otherId = (e->a == originId) ? e->b : e->a;
        const Vertex *other = roadGraph_.getVertex(otherId);
        Core::Vec2 dir = origin->pos - other->pos;
        currentAngle = std::atan2(dir.y, dir.x);
      }

      // Gather existing nodes within connectionRadius
      std::vector<uint32_t> survivors;
      for (uint32_t targetId = 0; targetId < roadGraph_.vertices().size();
           ++targetId) {
        if (targetId == originId)
          continue;
        const Vertex *target = roadGraph_.getVertex(targetId);
        double dist = origin->pos.distanceTo(target->pos);
        if (dist > lCfg.connectionRadius || dist < lCfg.clearance)
          continue;

        if (config_.capIntersectionsAt4 && target->edges.size() >= 4)
          continue;

        // Planar constraints
        bool duplicate = false;
        for (uint32_t eid : origin->edges) {
          const Edge *ee = roadGraph_.getEdge(eid);
          if (ee->a == targetId || ee->b == targetId) {
            duplicate = true;
            break;
          }
        }
        if (duplicate)
          continue;

        // Triangle check
        bool sharedNeighbor = false;
        for (uint32_t eidO : origin->edges) {
          const Edge *eeO = roadGraph_.getEdge(eidO);
          uint32_t neighborO = (eeO->a == originId) ? eeO->b : eeO->a;
          for (uint32_t eidT : target->edges) {
            const Edge *eeT = roadGraph_.getEdge(eidT);
            uint32_t neighborT = (eeT->a == targetId) ? eeT->b : eeT->a;
            if (neighborO == neighborT) {
              sharedNeighbor = true;
              break;
            }
          }
          if (sharedNeighbor)
            break;
        }
        if (sharedNeighbor)
          continue;

        // Small angle check
        auto checkAngle = [&](uint32_t vId, const Core::Vec2 &queryPos) {
          const Vertex *v = roadGraph_.getVertex(vId);
          Core::Vec2 queryDir = queryPos - v->pos;
          queryDir.normalize();
          for (uint32_t eid : v->edges) {
            const Edge *ee = roadGraph_.getEdge(eid);
            uint32_t neighbor = (ee->a == vId) ? ee->b : ee->a;
            Core::Vec2 edgeDir = roadGraph_.getVertex(neighbor)->pos - v->pos;
            edgeDir.normalize();
            double ang =
                std::acos(std::clamp(queryDir.dot(edgeDir), -1.0, 1.0));
            if (ang < config_.minAngleDeg * M_PI / 180.0)
              return true;
          }
          return false;
        };
        if (checkAngle(originId, target->pos) ||
            checkAngle(targetId, origin->pos))
          continue;

        // Crossing check
        bool crossing = false;
        for (const auto &e : roadGraph_.edges()) {
          uint32_t va = e.a, vb = e.b;
          if (va == originId || vb == originId || va == targetId ||
              vb == targetId)
            continue;
          if (SegmentsIntersect(origin->pos, target->pos,
                                roadGraph_.getVertex(va)->pos,
                                roadGraph_.getVertex(vb)->pos)) {
            crossing = true;
            break;
          }
        }
        if (crossing)
          continue;

        survivors.push_back(targetId);
      }

      if (!survivors.empty()) {
        uint32_t bestId = survivors[0];
        double bestScore = -1e9;
        Core::Vec2 currentDir(std::cos(currentAngle), std::sin(currentAngle));
        for (uint32_t sId : survivors) {
          Core::Vec2 sDir = roadGraph_.getVertex(sId)->pos - origin->pos;
          double dist = sDir.length();
          sDir.normalize();
          const double normalizedDist =
              dist / std::max(1.0, static_cast<double>(lCfg.connectionRadius));
          const double alignScore =
              ((sDir.dot(currentDir) + 1.0) * 0.5) * config_.straightnessWeight;
          const double distanceScore = normalizedDist * config_.distanceWeight;
          const double degreeBonus =
              std::max(0.0, 3.0 - static_cast<double>(roadGraph_.getVertex(sId)->edges.size())) *
              0.08;
          double score = alignScore + degreeBonus - distanceScore;
          if (score > bestScore) {
            bestScore = score;
            bestId = sId;
          }
        }

        const double edgeCost = origin->pos.distanceTo(roadGraph_.getVertex(bestId)->pos);
        if (edgeCost > remainingGrowthBudget) {
          candidateNodes.erase(candidateNodes.begin() + i);
          continue;
        }

        Edge e;
        e.a = originId;
        e.b = bestId;
        e.type = Core::RoadType::Street;
        roadGraph_.addEdge(e);
        remainingGrowthBudget -= edgeCost;
        changed = true;
      } else {
        // Create new node if budget allows (Investment Model). Children inherit
        // a decremented split budget, matching the thesis' chain-of-investment
        // model while the global growth budget caps total street length.
        const float branchJitter = static_cast<float>(rng.uniform(-0.55, 0.55));
        const float angle = static_cast<float>(currentAngle + branchJitter);
        const float dist = static_cast<float>(
          rng.uniform(lCfg.extensionRange.min, lCfg.extensionRange.max));
        if (dist > remainingGrowthBudget) {
          candidateNodes.erase(candidateNodes.begin() + i);
          continue;
        }
        Core::Vec2 newPos =
            origin->pos + Core::Vec2(std::cos(angle), std::sin(angle)) * dist;

        if (nodeIndex.queryNear(newPos, lCfg.clearance).empty()) {
          bool crossing = false;
          for (const auto &e : roadGraph_.edges()) {
            if (SegmentsIntersect(origin->pos, newPos,
                                  roadGraph_.getVertex(e.a)->pos,
                                  roadGraph_.getVertex(e.b)->pos)) {
              crossing = true;
              break;
            }
          }
          if (!crossing) {
            float oldBudget = candidate.budget; // cache before vector reallocation
            Vertex v;
            v.pos = newPos;
            uint32_t newId = roadGraph_.addVertex(v);
            nodeIndex.insert(newPos);
            const float budgetCost = std::max(
                0.65f, dist / std::max(1.0f, avgSeedSpan));
            const float childBudget = oldBudget - budgetCost;
            if (childBudget > 0.0f) {
              candidateNodes.push_back({newId, childBudget});
            }
            Edge e;
            e.a = originId;
            e.b = newId;
            e.type = Core::RoadType::Street;
            roadGraph_.addEdge(e);
            remainingGrowthBudget -= dist;
            changed = true;
          }
        }
      }

      if (roadGraph_.getVertex(originId)->edges.size() >= 4) {
        candidateNodes.erase(candidateNodes.begin() + i);
      } else {
        ++i;
      }
    }
  }
}

void RCDTDGenerator::splitEdges(int levelIndex) {
  // Thesis Part 2: "Let's pick a smaller size for edges, and split all existing
  // edges to that size. This creates new Nodes that can be used as intersections
  // for the next part of the algorithm."
  // Since Graph has no remove API, we snapshot + rebuild the whole graph.
  if (levelIndex < 0 ||
      levelIndex >= static_cast<int>(config_.levels.size()))
    return;

  const float threshold =
      config_.levels[static_cast<size_t>(levelIndex)].edgeSplitDistance;
  if (threshold <= 0.0f)
    return; // Splitting disabled for this level.

  const auto oldVerts = roadGraph_.vertices();
  const auto oldEdges = roadGraph_.edges();

  // Bail early if nothing needs splitting — avoids the rebuild cost.
  bool anySplit = false;
  for (const auto &e : oldEdges) {
    if (oldVerts[e.a].pos.distanceTo(oldVerts[e.b].pos) >
        static_cast<double>(threshold)) {
      anySplit = true;
      break;
    }
  }
  if (!anySplit)
    return;

  roadGraph_.clear();

  // Re-add vertices; Graph::addEdge rebuilds adjacency, so edges must be cleared.
  std::vector<uint32_t> vertexRemap(oldVerts.size());
  for (size_t i = 0; i < oldVerts.size(); ++i) {
    Vertex vCopy   = oldVerts[i];
    vCopy.edges.clear(); // adjacency rebuilt by addEdge
    vertexRemap[i] = roadGraph_.addVertex(vCopy);
  }

  for (const auto &oldEdge : oldEdges) {
    const Core::Vec2 a   = oldVerts[oldEdge.a].pos;
    const Core::Vec2 b   = oldVerts[oldEdge.b].pos;
    const double    len  = a.distanceTo(b);

    if (len <= static_cast<double>(threshold)) {
      // No split needed — remap endpoint IDs and re-add.
      Edge ne = oldEdge;
      ne.a    = vertexRemap[oldEdge.a];
      ne.b    = vertexRemap[oldEdge.b];
      roadGraph_.addEdge(ne);
    } else {
      // Subdivide into ceil(len / threshold) equal segments.
      // Each midpoint becomes a new vertex that the infill phase can snap to.
      const int    nSegs  = static_cast<int>(
          std::ceil(len / static_cast<double>(threshold)));
      uint32_t prevId = vertexRemap[oldEdge.a];

      for (int s = 1; s < nSegs; ++s) {
        const double t = static_cast<double>(s) / nSegs;
        Vertex mid;
        mid.pos          = a + (b - a) * t; // lerp along original edge
        const uint32_t midId = roadGraph_.addVertex(mid);
        Edge ne = oldEdge;
        ne.a    = prevId;
        ne.b    = midId;
        roadGraph_.addEdge(ne);
        prevId = midId;
      }
      // Final segment: last split node → original b.
      Edge ne = oldEdge;
      ne.a    = prevId;
      ne.b    = vertexRemap[oldEdge.b];
      roadGraph_.addEdge(ne);
    }
  }
}

void RCDTDGenerator::detectFaces() {
  if (roadGraph_.edges().empty())
    return;

  struct HalfEdge {
    uint32_t id;
    uint32_t source, target;
    uint32_t next = ~0u;
    uint32_t face = ~0u;
    double angle;
    bool visited = false;
  };

  std::vector<HalfEdge> hes;
  std::vector<std::vector<uint32_t>> adj(roadGraph_.vertices().size());
  std::vector<uint32_t> edgeToHe(roadGraph_.edges().size(), ~0u);

  for (size_t i = 0; i < roadGraph_.edges().size(); ++i) {
    const auto &e = roadGraph_.edges()[i];
    uint32_t idAB = static_cast<uint32_t>(hes.size());
    uint32_t idBA = idAB + 1;
    auto pA = roadGraph_.getVertex(e.a)->pos,
         pB = roadGraph_.getVertex(e.b)->pos;
    hes.push_back(
        {idAB, e.a, e.b, ~0u, ~0u, std::atan2(pB.y - pA.y, pB.x - pA.x)});
    hes.push_back(
        {idBA, e.b, e.a, ~0u, ~0u, std::atan2(pA.y - pB.y, pA.x - pB.x)});
    adj[e.a].push_back(idAB);
    adj[e.b].push_back(idBA);
    edgeToHe[i] = idAB;
  }

  for (size_t i = 0; i < adj.size(); ++i) {
    if (adj[i].empty())
      continue;
    std::sort(adj[i].begin(), adj[i].end(), [&](uint32_t a, uint32_t b) {
      return hes[a].angle < hes[b].angle;
    });
    for (size_t j = 0; j < adj[i].size(); ++j) {
      uint32_t outE = adj[i][j];
      uint32_t inE = outE ^ 1;
      size_t prevIdx = (j == 0) ? adj[i].size() - 1 : j - 1;
      hes[inE].next = adj[i][prevIdx];
    }
  }

  uint32_t faceIdCounter = 0;
  std::vector<std::pair<uint32_t, double>> acceptedFaces;
  for (auto &he : hes) {
    if (he.visited || he.next == ~0u)
      continue;
    BlockGraph::FaceNode face;
    face.id = faceIdCounter++;
    uint32_t curr = he.id;
    do {
      hes[curr].visited = true;
      hes[curr].face = face.id;
      face.boundary.push_back(roadGraph_.getVertex(hes[curr].source)->pos);
      curr = hes[curr].next;
    } while (curr != he.id && !hes[curr].visited);

    if (curr == he.id && face.boundary.size() >= 3) {
      const double signedArea = ComputeSignedPolygonArea(face.boundary);
      const double absArea = std::abs(signedArea);
      if (absArea > 1e-4) {
        face.area = static_cast<float>(absArea);
        acceptedFaces.push_back({face.id, absArea});
        blockGraph_.faces[face.id] = std::move(face);
      }
    }
  }

  // Remove the exterior face when face extraction returns both outer and inner
  // cycles with opposite winding. Keeping the largest face causes zoning to
  // collapse onto the outside hull instead of downtown blocks.
  if (acceptedFaces.size() > 1) {
    const auto largest = std::max_element(
        acceptedFaces.begin(), acceptedFaces.end(),
        [](const auto &lhs, const auto &rhs) { return lhs.second < rhs.second; });
    if (largest != acceptedFaces.end())
      blockGraph_.faces.erase(largest->first);
  }

  // Adjacency
  for (size_t i = 0; i < hes.size(); i += 2) {
    uint32_t fL = hes[i].face;
    uint32_t fR = hes[i + 1].face;
    if (fL != ~0u && fR != ~0u && fL != fR) {
      if (blockGraph_.faces.count(fL) && blockGraph_.faces.count(fR)) {
        blockGraph_.faces[fL].neighbors.push_back(fR);
        blockGraph_.faces[fR].neighbors.push_back(fL);
      }
    }
  }

  for (auto &[id, face] : blockGraph_.faces) {
    std::sort(face.neighbors.begin(), face.neighbors.end());
    face.neighbors.erase(
        std::unique(face.neighbors.begin(), face.neighbors.end()),
        face.neighbors.end());
  }
}

void RCDTDGenerator::performRecursiveInfill() {
  if (roadGraph_.edges().empty())
    return;

  auto segment_crosses_existing = [&](const Core::Vec2 &a,
                                      const Core::Vec2 &b,
                                      uint32_t allow_v0 = ~0u,
                                      uint32_t allow_v1 = ~0u) {
    for (const auto &e : roadGraph_.edges()) {
      if (e.a == allow_v0 || e.b == allow_v0 || e.a == allow_v1 ||
          e.b == allow_v1) {
        continue;
      }
      if (SegmentsIntersect(a, b, roadGraph_.getVertex(e.a)->pos,
                            roadGraph_.getVertex(e.b)->pos)) {
        return true;
      }
    }
    return false;
  };

  auto try_fallback_block = [&]() {
    if (!blockGraph_.faces.empty())
      return false;

    const auto &lCfg = config_.levels[0];
    const double depth = std::clamp<double>(
        lCfg.edgeSplitDistance,
        std::max<double>(lCfg.clearance * 2.0, 24.0),
        std::max<double>(lCfg.connectionRadius * 0.75, lCfg.clearance * 2.0));

    std::vector<uint32_t> edge_order(roadGraph_.edges().size());
    std::iota(edge_order.begin(), edge_order.end(), 0u);
    std::sort(edge_order.begin(), edge_order.end(), [&](uint32_t lhs, uint32_t rhs) {
      return roadGraph_.edges()[lhs].length > roadGraph_.edges()[rhs].length;
    });

    for (uint32_t edge_idx : edge_order) {
      const auto &edge = roadGraph_.edges()[edge_idx];
      const Core::Vec2 a = roadGraph_.getVertex(edge.a)->pos;
      const Core::Vec2 b = roadGraph_.getVertex(edge.b)->pos;
      Core::Vec2 normal(-(b - a).y, (b - a).x);
      if (normal.length() <= 1e-6)
        continue;
      normal.normalize();

      for (const Core::Vec2 side : {normal, normal.negated()}) {
        const Core::Vec2 c = b + side * depth;
        const Core::Vec2 d = a + side * depth;
        if (segment_crosses_existing(b, c, edge.a, edge.b) ||
            segment_crosses_existing(c, d) ||
            segment_crosses_existing(d, a, edge.a, edge.b)) {
          continue;
        }

        Vertex vc; vc.pos = c;
        Vertex vd; vd.pos = d;
        const uint32_t cId = roadGraph_.addVertex(vc);
        const uint32_t dId = roadGraph_.addVertex(vd);
        Edge e1; e1.a = edge.b; e1.b = cId; e1.type = Core::RoadType::Street; roadGraph_.addEdge(e1);
        Edge e2; e2.a = cId; e2.b = dId; e2.type = Core::RoadType::Street; roadGraph_.addEdge(e2);
        Edge e3; e3.a = dId; e3.b = edge.a; e3.type = Core::RoadType::Street; roadGraph_.addEdge(e3);
        return true;
      }
    }
    return false;
  };

  if (blockGraph_.faces.empty()) {
    if (!try_fallback_block())
      return;
    blockGraph_.faces.clear();
    detectFaces();
  }

  const int activeLevels = std::clamp(
      config_.recursionLevels, 1,
      std::min(kMaxRcdtdRecursionLevels,
               static_cast<int>(config_.levels.size())));
  // Separate RNG so seed offset does not collide with the outer growth phase.
  Core::RNG infillRng(config_.seed + 31337u);

  for (int li = 1; li < activeLevels; ++li) {
    splitEdges(li);
    blockGraph_.faces.clear();
    detectFaces();
    if (blockGraph_.faces.empty()) {
      if (!try_fallback_block())
        break;
      blockGraph_.faces.clear();
      detectFaces();
    }

    const auto  &lCfg        = config_.levels[static_cast<size_t>(li)];
    const double minAngleRad = config_.minAngleDeg * M_PI / 180.0;
    const double connRad     = static_cast<double>(lCfg.connectionRadius);
    const auto   facesSnapshot = blockGraph_.faces;
    bool addedAny = false;

    for (const auto &[faceId, face] : facesSnapshot) {
      if (face.boundary.size() < 3)
        continue;

      // ── Thesis Part 3: "Nodes inside a loop" scatter algorithm ──────────
      // Seed the open list with the face centroid. For each open-list node,
      // generate candidate positions at random distances in a circle; accept
      // those that are inside the face boundary and far enough from all
      // already-placed inner nodes. Accepted candidates join the road graph
      // and the open list so the scatter can radiate outward organically.
      const Core::Vec2 faceCentroid = PolygonUtil::centroid(face.boundary);
      if (!PolygonUtil::insidePolygon(faceCentroid, face.boundary))
        continue; // degenerate face — skip

      // Safety caps: scale interior scatter with face area while keeping
      // runtime finite and deterministic for large downtown blocks.
      const double faceScale = std::sqrt(std::max(1.0f, face.area));
      const int kMaxInnerNodes = std::clamp(
          static_cast<int>(faceScale /
                           std::max(4.0, static_cast<double>(lCfg.clearance))),
          18, 96);
      constexpr int kTriesPerSeed   = 40;
      constexpr int kMaxNewPerSeed  =  5;

      std::vector<uint32_t> innerIds;
      {
        Vertex cv;
        cv.pos = faceCentroid;
        innerIds.push_back(roadGraph_.addVertex(cv));
      }

      std::vector<Core::Vec2> openList = {faceCentroid};
      while (!openList.empty() &&
             static_cast<int>(innerIds.size()) < kMaxInnerNodes) {
        const Core::Vec2 source = openList.front();
        openList.erase(openList.begin());

        int newThisSeed = 0;
        for (int k = 0;
             k < kTriesPerSeed && newThisSeed < kMaxNewPerSeed &&
             static_cast<int>(innerIds.size()) < kMaxInnerNodes;
             ++k) {
          const double angle =
              infillRng.uniform(0.0, 2.0 * M_PI);
          const double dist =
              infillRng.uniform(static_cast<double>(lCfg.extensionRange.min),
                                static_cast<double>(lCfg.extensionRange.max));
          Core::Vec2 cand;
          cand.x = source.x + std::cos(angle) * dist;
          cand.y = source.y + std::sin(angle) * dist;

          if (!PolygonUtil::insidePolygon(cand, face.boundary))
            continue;

          // Clearance invariant: must be at least lCfg.clearance from every
          // inner node already placed in this face.
          bool tooClose = false;
          for (uint32_t id : innerIds) {
            if (roadGraph_.getVertex(id)->pos.distanceTo(cand) <
                static_cast<double>(lCfg.clearance)) {
              tooClose = true;
              break;
            }
          }
          if (tooClose)
            continue;

          Vertex v;
          v.pos = cand;
          innerIds.push_back(roadGraph_.addVertex(v));
          openList.push_back(cand);
          ++newThisSeed;
        }
      }

      if (innerIds.size() < 2)
        continue;

      // ── Thesis Part 3 (cont): "connect them in the same way as Part 1" ──
      // Build a candidate pool of inner nodes + nearby boundary vertices, then
      // apply the same planar-graph rules used in generateNodesAndEdges():
      // no crossing, no triangle, no small angle, degree-4 cap.

      // Locate graph vertex IDs for the outer boundary polygon points so that
      // inner streets can snap to the existing road network.
      std::vector<uint32_t> boundaryVertIds;
      boundaryVertIds.reserve(face.boundary.size());
      for (const Core::Vec2 &bp : face.boundary) {
        uint32_t bestId   = 0;
        double   bestDist = std::numeric_limits<double>::max();
        const auto nverts = static_cast<uint32_t>(roadGraph_.vertices().size());
        for (uint32_t vid = 0; vid < nverts; ++vid) {
          const double d = roadGraph_.getVertex(vid)->pos.distanceTo(bp);
          if (d < bestDist) {
            bestDist = d;
            bestId   = vid;
          }
        }
        // Only include vertices that are close to a known boundary point
        // (clearance * 0.6 avoids adding unrelated nearby vertices).
        if (bestDist < static_cast<double>(lCfg.clearance) * 0.6)
          boundaryVertIds.push_back(bestId);
      }
      // Deduplicate in case multiple boundary points map to the same vertex.
      std::sort(boundaryVertIds.begin(), boundaryVertIds.end());
      boundaryVertIds.erase(
          std::unique(boundaryVertIds.begin(), boundaryVertIds.end()),
          boundaryVertIds.end());

      // Combined pool: inner nodes first, then boundary snap vertices.
      std::vector<uint32_t> pool;
      pool.insert(pool.end(), innerIds.begin(), innerIds.end());
      for (uint32_t bId : boundaryVertIds)
        if (std::find(innerIds.begin(), innerIds.end(), bId) == innerIds.end())
          pool.push_back(bId);

      // Inline helpers mirror the constraint logic in generateNodesAndEdges().
      auto hasEdge = [&](uint32_t aId, uint32_t bId) {
        for (uint32_t eid : roadGraph_.getVertex(aId)->edges) {
          const Edge *e = roadGraph_.getEdge(eid);
          if (e->a == bId || e->b == bId)
            return true;
        }
        return false;
      };

      // Returns true when the angle at vId toward queryPos is below minAngleDeg.
      auto angTooSmall = [&](uint32_t vId, const Core::Vec2 &queryPos) {
        const Vertex *v = roadGraph_.getVertex(vId);
        Core::Vec2 qDir{queryPos.x - v->pos.x, queryPos.y - v->pos.y};
        if (qDir.length() < 1e-9)
          return true;
        qDir.normalize();
        for (uint32_t eid : v->edges) {
          const Edge *ee    = roadGraph_.getEdge(eid);
          uint32_t  neighbor = (ee->a == vId) ? ee->b : ee->a;
          Core::Vec2 eDir{
              roadGraph_.getVertex(neighbor)->pos.x - v->pos.x,
              roadGraph_.getVertex(neighbor)->pos.y - v->pos.y};
          if (eDir.length() < 1e-9)
            continue;
          eDir.normalize();
          const double ang =
              std::acos(std::clamp(qDir.dot(eDir), -1.0, 1.0));
          if (ang < minAngleRad)
            return true;
        }
        return false;
      };

      auto pickBestCandidate = [&](uint32_t innerId,
                                   const std::vector<uint32_t> &candidates,
                                   bool preferBoundary,
                                   uint32_t excludedId) -> uint32_t {
        const Core::Vec2 originPos = roadGraph_.getVertex(innerId)->pos;
        uint32_t bestId = ~0u;
        double bestScore = -std::numeric_limits<double>::max();
        for (uint32_t cId : candidates) {
          if (cId == excludedId)
            continue;
          const bool isBoundary = std::find(boundaryVertIds.begin(),
                                            boundaryVertIds.end(),
                                            cId) != boundaryVertIds.end();
          const double dist = originPos.distanceTo(roadGraph_.getVertex(cId)->pos);
          const double normalizedDist = dist / std::max(1.0, connRad);
          const double boundaryBias = preferBoundary
              ? (isBoundary ? 0.65 : 0.0)
              : (isBoundary ? -0.15 : 0.35);
          const double degreeBias = std::max(
              0.0, 3.0 - static_cast<double>(roadGraph_.getVertex(cId)->edges.size())) * 0.05;
          const double score = boundaryBias + degreeBias - normalizedDist;
          if (score > bestScore) {
            bestScore = score;
            bestId = cId;
          }
        }
        return bestId;
      };

      for (uint32_t innerId : innerIds) {
        const Core::Vec2 oPos = roadGraph_.getVertex(innerId)->pos;
        if (config_.capIntersectionsAt4 &&
            roadGraph_.getVertex(innerId)->edges.size() >= 4)
          continue;

        // Gather all valid connection targets from the pool.
        std::vector<uint32_t> candidates;
        for (uint32_t tid : pool) {
          if (tid == innerId)
            continue;
          const double dist =
              oPos.distanceTo(roadGraph_.getVertex(tid)->pos);
          if (dist < static_cast<double>(lCfg.clearance) || dist > connRad)
            continue;
          if (config_.capIntersectionsAt4 &&
              roadGraph_.getVertex(tid)->edges.size() >= 4)
            continue;
          if (hasEdge(innerId, tid))
            continue;

          // Triangle check: reject if origin and target share any neighbor.
          bool sharedNeighbor = false;
          for (uint32_t eidO : roadGraph_.getVertex(innerId)->edges) {
            const Edge *eeO = roadGraph_.getEdge(eidO);
            uint32_t    nO  = (eeO->a == innerId) ? eeO->b : eeO->a;
            for (uint32_t eidT : roadGraph_.getVertex(tid)->edges) {
              const Edge *eeT = roadGraph_.getEdge(eidT);
              uint32_t    nT  = (eeT->a == tid) ? eeT->b : eeT->a;
              if (nO == nT) { sharedNeighbor = true; break; }
            }
            if (sharedNeighbor) break;
          }
          if (sharedNeighbor)
            continue;

          if (angTooSmall(innerId, roadGraph_.getVertex(tid)->pos) ||
              angTooSmall(tid, oPos))
            continue;

          if (segment_crosses_existing(oPos, roadGraph_.getVertex(tid)->pos,
                                       innerId, tid))
            continue;

          candidates.push_back(tid);
        }

        if (candidates.empty())
          continue;

        const uint32_t primaryId = pickBestCandidate(innerId, candidates, true, ~0u);
        if (primaryId != ~0u) {
          Edge e;
          e.a    = innerId;
          e.b    = primaryId;
          e.type = Core::RoadType::Street;
          roadGraph_.addEdge(e);
          addedAny = true;
        }

        if (config_.capIntersectionsAt4 &&
            roadGraph_.getVertex(innerId)->edges.size() >= 4)
          continue;

        std::vector<uint32_t> secondaryCandidates;
        secondaryCandidates.reserve(candidates.size());
        for (uint32_t candidateId : candidates) {
          if (candidateId == primaryId || hasEdge(innerId, candidateId))
            continue;
          if (angTooSmall(innerId, roadGraph_.getVertex(candidateId)->pos) ||
              angTooSmall(candidateId, oPos))
            continue;
          if (segment_crosses_existing(oPos, roadGraph_.getVertex(candidateId)->pos,
                                       innerId, candidateId))
            continue;
          secondaryCandidates.push_back(candidateId);
        }

        const uint32_t secondaryId = pickBestCandidate(
            innerId, secondaryCandidates, false, primaryId);
        if (secondaryId != ~0u) {
          Edge e;
          e.a    = innerId;
          e.b    = secondaryId;
          e.type = Core::RoadType::Street;
          roadGraph_.addEdge(e);
          addedAny = true;
        }
      }
    }

    if (!addedAny)
      break;

    blockGraph_.faces.clear();
    detectFaces();
  }
}

void RCDTDGenerator::assignZoning() {
  if (blockGraph_.faces.empty())
    return;
  Core::RNG rng(config_.seed + 1);

  double totalInteriorArea = 0.0;
  for (const auto &[id, face] : blockGraph_.faces)
    totalInteriorArea += face.area;

  std::map<ZoneType, double> assignedArea;
  for (auto z : config_.zoneTypes)
    assignedArea[z] = 0.0;

  auto getGlobalWeight = [&](ZoneType type) {
    size_t idx = static_cast<size_t>(type);
    double target_proportion = idx < config_.targetAreaProportions.size()
                                   ? config_.targetAreaProportions[idx]
                                   : 0.0;
    double targetArea = totalInteriorArea * target_proportion;
    double diff = targetArea - assignedArea[type];
    return std::max(0.01, diff / (targetArea + 1e-6));
  };

  std::vector<uint32_t> openFaces;
  std::set<uint32_t> remainingFaces;
  for (const auto &pair : blockGraph_.faces)
    remainingFaces.insert(pair.first);

  // Seed near generated center rather than a fixed thesis sketch origin.
  Core::Vec2 center = ComputeRoadBoundsCenter(roadGraph_);
  uint32_t seedId = blockGraph_.faces.begin()->first;
  double minDist = 1e9;
  for (uint32_t id : remainingFaces) {
    Core::Vec2 faceCenter(0, 0);
    for (const auto &p : blockGraph_.faces.at(id).boundary)
      faceCenter = faceCenter + p;
    faceCenter = faceCenter * (1.0 / blockGraph_.faces.at(id).boundary.size());
    double d = faceCenter.distanceTo(center);
    if (d < minDist) {
      minDist = d;
      seedId = id;
    }
  }
  openFaces.push_back(seedId);
  remainingFaces.erase(seedId);

  while (!remainingFaces.empty() || !openFaces.empty()) {
    if (openFaces.empty()) {
      uint32_t nextId = *remainingFaces.begin();
      openFaces.push_back(nextId);
      remainingFaces.erase(nextId);
    }

    uint32_t currId = openFaces.back();
    openFaces.pop_back();
    auto &face = blockGraph_.faces[currId];

    std::vector<double> scores(config_.zoneTypes.size(), 1.0);
    Core::Vec2 faceCenter(0, 0);
    for (auto &p : face.boundary)
      faceCenter = faceCenter + p;
    faceCenter = faceCenter * (1.0 / face.boundary.size());
    double distFromCenter = faceCenter.distanceTo(center);

    for (size_t i = 0; i < config_.zoneTypes.size(); ++i) {
      ZoneType z = config_.zoneTypes[i];
      double localWeight = 0.0;
      for (uint32_t nId : face.neighbors) {
        if (zoningResult_.assignments.count(nId)) {
          ZoneType nZ = zoningResult_.assignments[nId];
          // localWeight += adjacencyWeights[nZ][z] * sharedLength
          localWeight += config_.adjacencyWeights[static_cast<size_t>(nZ)][i];
        }
      }
      if (localWeight == 0.0)
        localWeight = 1.0;

      // Requirement function (distance from center).
      // Stronger radial zoning makes the preview read as a downtown schema:
      // intense center/business core, mixed/commercial ring, parks as relief,
      // then lower-intensity outward districts.
      const double centerBand = std::max(40.0, std::sqrt(std::max(1.0, totalInteriorArea)) * 0.08);
      const double parkBand = centerBand * 2.1;
      double req = 1.0;
      switch (z) {
      case ZoneType::CityCenter:
        req = std::exp(-distFromCenter / std::max(1.0, centerBand * 0.75));
        break;
      case ZoneType::Business:
        req = std::exp(-distFromCenter / std::max(1.0, centerBand * 1.15));
        break;
      case ZoneType::Commercial:
        req = std::exp(-std::abs(distFromCenter - centerBand * 0.9) /
                       std::max(1.0, centerBand * 0.9));
        break;
      case ZoneType::MixedResCom:
        req = std::exp(-std::abs(distFromCenter - centerBand * 1.25) /
                       std::max(1.0, centerBand));
        break;
      case ZoneType::HighRes:
        req = std::exp(-distFromCenter / std::max(1.0, centerBand * 1.8));
        break;
      case ZoneType::MedRes:
        req = 0.65 + 0.35 * std::exp(-std::abs(distFromCenter - parkBand) /
                                     std::max(1.0, centerBand * 1.2));
        break;
      case ZoneType::LowRes:
        req = 0.30 + (1.0 - std::exp(-distFromCenter /
                                     std::max(1.0, centerBand * 2.8)));
        break;
      case ZoneType::Industrial:
        req = 0.25 + (1.0 - std::exp(-distFromCenter /
                                     std::max(1.0, centerBand * 2.2)));
        break;
      case ZoneType::Park:
        req = 0.25 + std::exp(-std::abs(distFromCenter - parkBand) /
                              std::max(1.0, centerBand * 0.8));
        break;
      default:
        break;
      }

      scores[i] = localWeight * req * getGlobalWeight(z);
    }

    // Weighted random sample
    double sum = 0.0;
    for (auto s : scores)
      sum += s;
    double r = rng.uniform() * sum;
    uint32_t selectedIdx = 0;
    double acc = 0.0;
    for (size_t i = 0; i < scores.size(); ++i) {
      acc += scores[i];
      if (r <= acc) {
        selectedIdx = static_cast<uint32_t>(i);
        break;
      }
    }

    ZoneType selectedZone = config_.zoneTypes[selectedIdx];
    zoningResult_.assignments[currId] = selectedZone;
    face.assignedZone = selectedZone;
    assignedArea[selectedZone] += face.area;

    // Push unassigned neighbors
    for (uint32_t nId : face.neighbors) {
      if (remainingFaces.count(nId)) {
        openFaces.push_back(nId);
        remainingFaces.erase(nId);
      }
    }
  }
}

} // namespace RogueCity::Generators::Urban
