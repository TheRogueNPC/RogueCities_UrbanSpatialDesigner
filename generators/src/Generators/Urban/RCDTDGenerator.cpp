#include "RogueCity/Generators/Urban/RCDTDGenerator.hpp"
#include "RogueCity/Generators/Urban/GridStorage.hpp"
#include <algorithm>
#include <cmath>
#include <map>
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

  currentPhase_ = Phase::Faces;
  detectFaces();

  currentPhase_ = Phase::Infill;
  performRecursiveInfill();

  currentPhase_ = Phase::Zoning;
  assignZoning();

  currentPhase_ = Phase::Done;
}

void RCDTDGenerator::GenerateStep() {
  switch (currentPhase_) {
  case Phase::Idle:
    roadGraph_.clear();
    currentPhase_ = Phase::NodesAndEdges;
    generateNodesAndEdges();
    break;
  case Phase::NodesAndEdges:
    currentPhase_ = Phase::Faces;
    detectFaces();
    break;
  case Phase::Faces:
    currentPhase_ = Phase::Infill;
    performRecursiveInfill();
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
  const auto &lCfg = config_.levels[0];
  GridStorage nodeIndex(lCfg.clearance);

  struct CandidateNode {
    uint32_t id;
    float budget;
  };

  std::vector<uint32_t> candidateNodes;

  // 1. Initial Seeds
  if (startingGraph_ && !startingGraph_->vertices().empty()) {
    for (const auto &v : startingGraph_->vertices()) {
      uint32_t id = roadGraph_.addVertex(v);
      candidateNodes.push_back(id);
      nodeIndex.insert(v.pos);
    }
  } else {
    Vertex v;
    v.pos = {1000.0, 1000.0};
    uint32_t id = roadGraph_.addVertex(v);
    candidateNodes.push_back(id);
    nodeIndex.insert(v.pos);
  }

  // 2. Growth Driver
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t i = 0; i < candidateNodes.size();) {
      uint32_t originId = candidateNodes[i];
      const Vertex *origin = roadGraph_.getVertex(originId);

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
          double score = sDir.dot(currentDir) * 100.0 - dist * 0.1;
          if (score > bestScore) {
            bestScore = score;
            bestId = sId;
          }
        }

        Edge e;
        e.a = originId;
        e.b = bestId;
        e.type = Core::RoadType::Street;
        roadGraph_.addEdge(e);
        changed = true;
      } else {
        // Create new node if budget allows (Investment Model)
        // Simplified: just try to expand once
        float angle = currentAngle + rng.uniform(-0.1, 0.1);
        float dist =
            rng.uniform(lCfg.extensionRange.min, lCfg.extensionRange.max);
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
            Vertex v;
            v.pos = newPos;
            uint32_t newId = roadGraph_.addVertex(v);
            nodeIndex.insert(newPos);
            candidateNodes.push_back(newId);
            Edge e;
            e.a = originId;
            e.b = newId;
            e.type = Core::RoadType::Street;
            roadGraph_.addEdge(e);
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
      double area = 0.0;
      for (size_t i = 0; i < face.boundary.size(); ++i) {
        area += (face.boundary[i].x *
                     face.boundary[(i + 1) % face.boundary.size()].y -
                 face.boundary[(i + 1) % face.boundary.size()].x *
                     face.boundary[i].y);
      }
      if (area > 0.0) {
        face.area = area * 0.5;
        blockGraph_.faces[face.id] = std::move(face);
      }
    }
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
}

void RCDTDGenerator::performRecursiveInfill() {
  // Phase 1: Split existing edges
  // Future work: recursion level handling
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

  // Seed near center
  Core::Vec2 center(1000, 1000);
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

      // Requirement function (distance from center)
      double req = 1.0;
      if (z == ZoneType::CityCenter)
        req = std::exp(-distFromCenter * 0.005);
      else if (z == ZoneType::Industrial)
        req = 1.0 - std::exp(-distFromCenter * 0.002);

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
        selectedIdx = i;
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
