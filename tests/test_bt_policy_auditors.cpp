#include "RogueCity/Generators/Policy/BehaviorTreeAuditors.hpp"
#include "RogueCity/Generators/Policy/FbczRuleEngine.hpp"

#include <cassert>
#include <string>
#include <vector>

int main() {
  using RogueCity::Core::BuildingSite;
  using RogueCity::Core::District;
  using RogueCity::Core::DistrictType;
  using RogueCity::Core::LotToken;
  using RogueCity::Core::Road;
  using RogueCity::Core::RoadType;
  using RogueCity::Core::Vec2;
  using RogueCity::Generators::CityGenerator;
  using RogueCity::Generators::Policy::BehaviorTreeAuditors;
  using RogueCity::Generators::Policy::FbczRuleEngine;
  using RogueCity::Generators::Policy::FbczRuleSet;

  const std::string rules_path =
      std::string(ROGUECITY_SOURCE_DIR) +
      "/tests/fixtures/fbcz/basic_form_rules.json";

  FbczRuleSet rules{};
  std::string error;
  assert(FbczRuleEngine::LoadFromJsonFile(rules_path, rules, &error));

  CityGenerator::CityOutput city{};

  Road road_a{};
  road_a.id = 1u;
  road_a.type = RoadType::Street;
  road_a.points = {Vec2(0.0, 0.0), Vec2(20.0, 0.0)};
  city.roads.add(road_a);

  Road road_b{};
  road_b.id = 2u;
  road_b.type = RoadType::Street;
  road_b.points = {Vec2(200.0, 200.0), Vec2(220.0, 200.0)};
  city.roads.add(road_b);

  District district{};
  district.id = 1u;
  district.type = DistrictType::Residential;
  district.border = {Vec2(0.0, 0.0), Vec2(30.0, 0.0), Vec2(30.0, 30.0), Vec2(0.0, 30.0)};
  city.districts.push_back(district);

  LotToken lot{};
  lot.id = 5u;
  lot.district_id = district.id;
  lot.boundary = district.border;
  lot.centroid = Vec2(15.0, 15.0);
  city.lots.push_back(lot);

  FbczRuleEngine::AttachDistrictAndLotAttributes(city.districts, city.lots, rules);

  BuildingSite building{};
  building.id = 77u;
  building.lot_id = lot.id;
  building.district_id = lot.district_id;
  building.position = Vec2(15.0, 25.0);
  building.suggested_height = 120.0f;
  city.buildings.push_back(building);

  const auto fbcz_violations =
      FbczRuleEngine::EnforceBuildings(city.lots, city.buildings, rules);
  assert(!fbcz_violations.empty());

  const auto report = BehaviorTreeAuditors::Run(
      city, BehaviorTreeAuditors::Config{}, &rules);
  assert(!report.empty());
  assert(!report.ToJson().empty());
  assert(!report.ToHumanReadable().empty());

  return 0;
}
