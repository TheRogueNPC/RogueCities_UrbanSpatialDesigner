#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"

#include <map>
#include <string>
#include <vector>

namespace RogueCity::Generators::Urban {

inline constexpr int kMaxRcdtdRecursionLevels = 5;

/**
 * @brief Zoning types for the RCDTD module.
 */
enum class ZoneType : uint8_t {
  CityCenter = 0,
  MixedResCom,
  HighRes,
  MedRes,
  LowRes,
  Commercial,
  Industrial,
  Business,
  Park,
  COUNT
};

/**
 * @brief Configuration for a specific recursion level in RCDTD.
 */
struct RCDTDLevelConfig {
  float clearance{20.0f};
  struct FloatRange {
    float min{40.0f};
    float max{120.0f};
  } extensionRange;
  float connectionRadius{150.0f};
  float edgeSplitDistance{100.0f};
};

/**
 * @brief Global configuration for the RCDTD – Rogue Cities Downtown Generator.
 */
struct RCDTDConfig {
  uint32_t seed{12345u};

  // Global parameters
  float minAngleDeg{55.0f}; // Thesis recommends 55-65 initially
  int recursionLevels{2};

  // Thesis v1 Generation Parameters
  float growthBudget{1000.0f};
  float straightnessWeight{1.0f};
  float distanceWeight{0.2f};
  bool capIntersectionsAt4{true};

  // Per-level parameters
  std::vector<RCDTDLevelConfig> levels;

  // Zoning parameters
  std::vector<ZoneType> zoneTypes;
  std::vector<std::vector<float>> adjacencyWeights; // Symmetric matrix
  std::vector<float> targetAreaProportions;         // Normalized

  // Default constructor for sane defaults
  RCDTDConfig() {
    levels.resize(kMaxRcdtdRecursionLevels);
    zoneTypes = {
        ZoneType::CityCenter, ZoneType::MixedResCom, ZoneType::HighRes,
        ZoneType::MedRes,     ZoneType::LowRes,      ZoneType::Commercial,
        ZoneType::Industrial, ZoneType::Business,    ZoneType::Park};

    const size_t n = zoneTypes.size();
    adjacencyWeights.assign(n, std::vector<float>(n, 1.0f));
    targetAreaProportions.assign(n, 1.0f / static_cast<float>(n));
  }
};

/**
 * @brief Lightweight representation of a face adjacency graph.
 */
struct BlockGraph {
  struct FaceNode {
    uint32_t id;
    std::vector<Core::Vec2> boundary;
    ZoneType assignedZone{ZoneType::LowRes};
    float area{0.0f};
    std::vector<uint32_t> neighbors; // IDs of adjacent faces
  };
  std::map<uint32_t, FaceNode> faces;
};

/**
 * @brief Results of the zoning phase.
 */
struct ZoningResult {
  std::map<uint32_t, ZoneType> assignments; // FaceID -> ZoneType
};

/**
 * @brief RCDTD – Rogue Cities Downtown Generator v1.
 *
 * Implements the investment-model and planar-graph-based algorithms
 * for intelligent downtown generating.
 */
class RCDTDGenerator {
public:
  RCDTDGenerator() = default;

  void SetConfig(const RCDTDConfig &config);
  void SetStartingGraph(const Graph *graph); // Optional seed graph

  /**
   * @brief Runs the full generation pipeline.
   */
  void Generate();

  /**
   * @brief Steps through major generation phases.
   */
  void GenerateStep();

  const Graph &GetRoadGraph() const { return roadGraph_; }
  const BlockGraph &GetBlockGraph() const { return blockGraph_; }
  const ZoningResult &GetZoning() const { return zoningResult_; }

private:
  RCDTDConfig config_;
  const Graph *startingGraph_{nullptr};

  Graph roadGraph_;
  BlockGraph blockGraph_;
  ZoningResult zoningResult_;

  // Phase-specific implementation helpers
  void generateNodesAndEdges();
  void splitEdges(int levelIndex); // Thesis Part 2: subdivide long edges → inner-street anchor nodes
  void detectFaces();
  void performRecursiveInfill();
  void assignZoning();

  // Internal state for stepped generation
  enum class Phase {
    Idle,
    NodesAndEdges,
    Faces,
    Infill,
    Zoning,
    Done
  } currentPhase_{Phase::Idle};
};

} // namespace RogueCity::Generators::Urban
