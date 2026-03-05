#pragma once

#include "RogueCity/Core/Types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace RogueCity::Generators::Policy {

struct FbczRule {
  std::string zone_type{};
  float build_to_min{0.0f};
  float build_to_max{0.0f};
  float max_setback{0.0f};
  float frontage_occupancy_min{0.0f};
  float height_min{0.0f};
  float height_max{0.0f};
  float massing_floor_area_ratio{1.0f};
};

struct FbczRuleSet {
  std::vector<FbczRule> rules{};
  std::string source_path{};
};

struct FbczViolation {
  uint32_t lot_id{0};
  uint32_t building_id{0};
  std::string message{};
};

class FbczRuleEngine {
public:
  [[nodiscard]] static bool LoadFromJsonFile(const std::string &path,
                                             FbczRuleSet &out,
                                             std::string *error = nullptr);

  [[nodiscard]] static std::optional<FbczRule>
  FindRule(const FbczRuleSet &rules, const std::string &zone_type);

  static void AttachDistrictAndLotAttributes(std::vector<Core::District> &districts,
                                             std::vector<Core::LotToken> &lots,
                                             const FbczRuleSet &rules);

  [[nodiscard]] static std::vector<FbczViolation>
  EnforceBuildings(const std::vector<Core::LotToken> &lots,
                   siv::Vector<Core::BuildingSite> &buildings,
                   const FbczRuleSet &rules);
};

} // namespace RogueCity::Generators::Policy
