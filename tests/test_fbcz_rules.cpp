#include "RogueCity/Generators/Policy/FbczRuleEngine.hpp"

#include <cassert>
#include <string>
#include <vector>

int main() {
  using RogueCity::Core::BuildingSite;
  using RogueCity::Core::District;
  using RogueCity::Core::DistrictType;
  using RogueCity::Core::LotToken;
  using RogueCity::Core::Vec2;
  using RogueCity::Generators::Policy::FbczRuleEngine;
  using RogueCity::Generators::Policy::FbczRuleSet;

  const std::string path =
      std::string(ROGUECITY_SOURCE_DIR) +
      "/tests/fixtures/fbcz/basic_form_rules.json";

  FbczRuleSet rules{};
  std::string error;
  assert(FbczRuleEngine::LoadFromJsonFile(path, rules, &error));
  assert(!rules.rules.empty());

  District district{};
  district.id = 1u;
  district.type = DistrictType::Residential;
  district.border = {
      Vec2(0.0, 0.0),
      Vec2(20.0, 0.0),
      Vec2(20.0, 20.0),
      Vec2(0.0, 20.0),
  };

  LotToken lot{};
  lot.id = 10u;
  lot.district_id = district.id;
  lot.centroid = Vec2(10.0, 10.0);
  lot.boundary = district.border;

  std::vector<District> districts{district};
  std::vector<LotToken> lots{lot};

  FbczRuleEngine::AttachDistrictAndLotAttributes(districts, lots, rules);
  assert(!districts.front().form_district.empty());
  assert(!lots.front().form_district.empty());
  assert(lots.front().fbcz_height_max >= lots.front().fbcz_height_min);

  siv::Vector<BuildingSite> buildings{};
  BuildingSite building{};
  building.id = 100u;
  building.lot_id = lot.id;
  building.district_id = lot.district_id;
  building.position = Vec2(10.0, 18.0);
  building.suggested_height = 120.0f;
  buildings.push_back(building);

  const auto violations = FbczRuleEngine::EnforceBuildings(lots, buildings, rules);
  assert(!violations.empty());
  assert(buildings[0].suggested_height <= lots[0].fbcz_height_max + 1e-4f);
  assert(buildings[0].suggested_height >= lots[0].fbcz_height_min - 1e-4f);
  assert(buildings[0].position.y <= lots[0].fbcz_max_setback + 1e-4);

  return 0;
}
