#include "RogueCity/Generators/Policy/FbczRuleEngine.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace RogueCity::Generators::Policy {

namespace {

void SetError(std::string *error, const std::string &message) {
  if (error != nullptr) {
    *error = message;
  }
}

std::string NormalizeKey(std::string key) {
  std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return key;
}

std::string DefaultZoneType(Core::DistrictType type) {
  switch (type) {
  case Core::DistrictType::Residential:
    return "residential_neighborhood";
  case Core::DistrictType::Commercial:
    return "mixed_corridor";
  case Core::DistrictType::Industrial:
    return "industrial_edge";
  case Core::DistrictType::Civic:
    return "civic_core";
  case Core::DistrictType::Mixed:
  default:
    return "mixed_corridor";
  }
}

} // namespace

bool FbczRuleEngine::LoadFromJsonFile(const std::string &path,
                                      FbczRuleSet &out,
                                      std::string *error) {
  std::ifstream input(path);
  if (!input.good()) {
    SetError(error, "Unable to open FBCZ rules file: " + path);
    return false;
  }

  nlohmann::json json_root;
  try {
    input >> json_root;
  } catch (const std::exception &ex) {
    SetError(error, std::string("Unable to parse FBCZ rules JSON: ") +
                        ex.what());
    return false;
  }

  if (!json_root.is_object() || !json_root.contains("zones") ||
      !json_root["zones"].is_array()) {
    SetError(error,
             "FBCZ rules JSON must contain array field 'zones' at top level");
    return false;
  }

  FbczRuleSet parsed{};
  parsed.source_path = path;

  for (const auto &zone : json_root["zones"]) {
    if (!zone.is_object()) {
      continue;
    }

    FbczRule rule{};
    rule.zone_type = zone.value("zone_type", std::string{});
    if (rule.zone_type.empty()) {
      continue;
    }

    rule.build_to_min =
        std::max(0.0f, zone.value("build_to_min", rule.build_to_min));
    rule.build_to_max =
        std::max(rule.build_to_min, zone.value("build_to_max", rule.build_to_max));
    rule.max_setback =
        std::max(0.0f, zone.value("max_setback", rule.max_setback));
    rule.frontage_occupancy_min = std::clamp(
        zone.value("frontage_occupancy_min", rule.frontage_occupancy_min),
        0.0f, 1.0f);
    rule.height_min = std::max(0.0f, zone.value("height_min", rule.height_min));
    rule.height_max =
        std::max(rule.height_min, zone.value("height_max", rule.height_max));
    rule.massing_floor_area_ratio =
        std::max(0.1f,
                 zone.value("massing_floor_area_ratio", rule.massing_floor_area_ratio));

    parsed.rules.push_back(std::move(rule));
  }

  if (parsed.rules.empty()) {
    SetError(error, "FBCZ rules file contains no valid zone entries");
    return false;
  }

  out = std::move(parsed);
  return true;
}

std::optional<FbczRule>
FbczRuleEngine::FindRule(const FbczRuleSet &rules, const std::string &zone_type) {
  const std::string key = NormalizeKey(zone_type);
  for (const auto &rule : rules.rules) {
    if (NormalizeKey(rule.zone_type) == key) {
      return rule;
    }
  }
  return std::nullopt;
}

void FbczRuleEngine::AttachDistrictAndLotAttributes(
    std::vector<Core::District> &districts, std::vector<Core::LotToken> &lots,
    const FbczRuleSet &rules) {
  std::unordered_map<uint32_t, std::string> district_zone;
  district_zone.reserve(districts.size());

  for (auto &district : districts) {
    if (district.form_district.empty()) {
      district.form_district = DefaultZoneType(district.type);
    }
    if (!FindRule(rules, district.form_district).has_value() &&
        !rules.rules.empty()) {
      district.form_district = rules.rules.front().zone_type;
    }
    district_zone[district.id] = district.form_district;
  }

  for (auto &lot : lots) {
    const auto district_it = district_zone.find(lot.district_id);
    if (district_it != district_zone.end()) {
      lot.form_district = district_it->second;
    }

    const auto rule = FindRule(rules, lot.form_district);
    if (!rule.has_value()) {
      continue;
    }

    lot.fbcz_build_to_min = rule->build_to_min;
    lot.fbcz_build_to_max = rule->build_to_max;
    lot.fbcz_max_setback = rule->max_setback;
    lot.fbcz_frontage_occupancy_min = rule->frontage_occupancy_min;
    lot.fbcz_height_min = rule->height_min;
    lot.fbcz_height_max = rule->height_max;
  }
}

std::vector<FbczViolation> FbczRuleEngine::EnforceBuildings(
    const std::vector<Core::LotToken> &lots, siv::Vector<Core::BuildingSite> &buildings,
    const FbczRuleSet &rules) {
  std::unordered_map<uint32_t, const Core::LotToken *> lot_by_id;
  lot_by_id.reserve(lots.size());
  for (const auto &lot : lots) {
    lot_by_id[lot.id] = &lot;
  }

  std::vector<FbczViolation> violations;

  for (auto &building : buildings) {
    const auto lot_it = lot_by_id.find(building.lot_id);
    if (lot_it == lot_by_id.end()) {
      continue;
    }

    const Core::LotToken &lot = *lot_it->second;
    const auto rule = FindRule(rules, lot.form_district);
    if (!rule.has_value()) {
      continue;
    }

    building.fbcz_height_min = rule->height_min;
    building.fbcz_height_max = rule->height_max;

    const float original_height = building.suggested_height;
    if (building.suggested_height <= 0.0f) {
      building.suggested_height = rule->height_min;
    }
    building.suggested_height =
        std::clamp(building.suggested_height, rule->height_min, rule->height_max);

    if (std::abs(building.suggested_height - original_height) > 1e-4f) {
      FbczViolation violation{};
      violation.lot_id = lot.id;
      violation.building_id = building.id;
      violation.message = "Adjusted building height to FBCZ range";
      violations.push_back(std::move(violation));
    }

    if (lot.boundary.size() >= 3 && rule->max_setback > 0.0f) {
      double min_y = lot.boundary.front().y;
      double max_y = lot.boundary.front().y;
      for (const auto &point : lot.boundary) {
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
      }

      const double allowed_back = min_y + static_cast<double>(rule->max_setback);
      if (building.position.y > allowed_back) {
        building.position.y = allowed_back;
        FbczViolation violation{};
        violation.lot_id = lot.id;
        violation.building_id = building.id;
        violation.message = "Adjusted building position to satisfy max setback";
        violations.push_back(std::move(violation));
      }

      building.fbcz_frontage_occupancy = std::clamp(
          (max_y - building.position.y) / std::max(1e-6, max_y - min_y), 0.0,
          1.0);
    }

    if (building.fbcz_frontage_occupancy + 1e-6 <
        lot.fbcz_frontage_occupancy_min) {
      building.fbcz_violation = true;
      FbczViolation violation{};
      violation.lot_id = lot.id;
      violation.building_id = building.id;
      violation.message = "Frontage occupancy below FBCZ minimum";
      violations.push_back(std::move(violation));
    } else {
      building.fbcz_violation = false;
    }
  }

  return violations;
}

} // namespace RogueCity::Generators::Policy
