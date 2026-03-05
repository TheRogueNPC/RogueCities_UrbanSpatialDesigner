#include "RogueCity/Generators/Policy/BehaviorTreeAuditors.hpp"

#include "RogueCity/Generators/Urban/PolygonUtil.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#if defined(ROGUECITY_ENABLE_BEHAVIORTREE_AUDITORS)
#if __has_include(<behaviortree_cpp_v3/bt_factory.h>)
#include <behaviortree_cpp_v3/bt_factory.h>
#define ROGUECITY_HAS_BTCPP 1
#elif __has_include(<behaviortree_cpp/bt_factory.h>)
#include <behaviortree_cpp/bt_factory.h>
#define ROGUECITY_HAS_BTCPP 1
#else
#define ROGUECITY_HAS_BTCPP 0
#endif
#else
#define ROGUECITY_HAS_BTCPP 0
#endif

namespace RogueCity::Generators::Policy {

namespace {

struct DisjointSet {
  std::vector<uint32_t> parent{};
  std::vector<uint32_t> rank{};

  uint32_t Add() {
    const uint32_t id = static_cast<uint32_t>(parent.size());
    parent.push_back(id);
    rank.push_back(0u);
    return id;
  }

  uint32_t Find(uint32_t x) {
    if (parent[x] != x) {
      parent[x] = Find(parent[x]);
    }
    return parent[x];
  }

  void Union(uint32_t a, uint32_t b) {
    const uint32_t root_a = Find(a);
    const uint32_t root_b = Find(b);
    if (root_a == root_b) {
      return;
    }

    if (rank[root_a] < rank[root_b]) {
      parent[root_a] = root_b;
    } else if (rank[root_a] > rank[root_b]) {
      parent[root_b] = root_a;
    } else {
      parent[root_b] = root_a;
      rank[root_a] += 1u;
    }
  }
};

[[nodiscard]] std::string NodeKey(const Core::Vec2 &point) {
  const int x = static_cast<int>(std::llround(point.x));
  const int y = static_cast<int>(std::llround(point.y));
  return std::to_string(x) + ":" + std::to_string(y);
}

void RunConnectivityAudit(const CityGenerator::CityOutput &city,
                          AuditReport &report,
                          bool include_adjustments) {
  std::unordered_map<std::string, uint32_t> node_ids;
  node_ids.reserve(city.roads.size() * 2u);

  DisjointSet dsu{};
  auto get_node = [&](const Core::Vec2 &point) {
    const std::string key = NodeKey(point);
    const auto it = node_ids.find(key);
    if (it != node_ids.end()) {
      return it->second;
    }
    const uint32_t id = dsu.Add();
    node_ids[key] = id;
    return id;
  };

  for (const auto &road : city.roads) {
    if (road.points.size() < 2) {
      continue;
    }
    for (size_t i = 1; i < road.points.size(); ++i) {
      const uint32_t a = get_node(road.points[i - 1]);
      const uint32_t b = get_node(road.points[i]);
      dsu.Union(a, b);
    }
  }

  if (!dsu.parent.empty()) {
    std::unordered_map<uint32_t, uint32_t> component_sizes;
    for (uint32_t i = 0; i < dsu.parent.size(); ++i) {
      component_sizes[dsu.Find(i)] += 1u;
    }

    if (component_sizes.size() > 1u) {
      AuditIssue issue{};
      issue.code = "connectivity.multiple_components";
      issue.severity = 2;
      issue.message = "Road graph has " +
                      std::to_string(component_sizes.size()) +
                      " disconnected components";
      if (include_adjustments) {
        issue.message += "; propose adding arterial stitch connectors";
      }
      report.connectivity_issues.push_back(std::move(issue));
    }
  }

  if (!city.districts.empty() && city.roads.size() > 0) {
    for (const auto &district : city.districts) {
      if (district.border.empty()) {
        continue;
      }
      const Core::Vec2 centroid = Urban::PolygonUtil::centroid(district.border);
      double best_dist = std::numeric_limits<double>::infinity();
      for (const auto &road : city.roads) {
        for (const auto &point : road.points) {
          best_dist = std::min(best_dist, centroid.distanceTo(point));
        }
      }
      if (best_dist > 220.0) {
        AuditIssue issue{};
        issue.code = "connectivity.unreachable_district";
        issue.severity = 1;
        issue.message = "District " + std::to_string(district.id) +
                        " appears unreachable from current road graph";
        report.connectivity_issues.push_back(std::move(issue));
      }
    }
  }
}

void RunZoningAudit(const CityGenerator::CityOutput &city,
                    const FbczRuleSet *rules,
                    AuditReport &report,
                    bool include_adjustments) {
  if (rules == nullptr) {
    return;
  }

  std::unordered_map<uint32_t, const Core::LotToken *> lot_by_id;
  lot_by_id.reserve(city.lots.size());
  for (const auto &lot : city.lots) {
    lot_by_id[lot.id] = &lot;
    if (!FbczRuleEngine::FindRule(*rules, lot.form_district).has_value()) {
      AuditIssue issue{};
      issue.code = "zoning.missing_form_district_rule";
      issue.severity = 1;
      issue.message = "Lot " + std::to_string(lot.id) +
                      " references unknown form district '" + lot.form_district +
                      "'";
      report.zoning_issues.push_back(std::move(issue));
    }
  }

  for (const auto &building : city.buildings) {
    const auto lot_it = lot_by_id.find(building.lot_id);
    if (lot_it == lot_by_id.end()) {
      continue;
    }

    const Core::LotToken &lot = *lot_it->second;
    const auto rule = FbczRuleEngine::FindRule(*rules, lot.form_district);
    if (!rule.has_value()) {
      continue;
    }

    if (building.suggested_height + 1e-4f < rule->height_min ||
        building.suggested_height - 1e-4f > rule->height_max) {
      AuditIssue issue{};
      issue.code = "zoning.height_violation";
      issue.severity = 2;
      issue.message = "Building " + std::to_string(building.id) +
                      " violates FBCZ height bounds";
      if (include_adjustments) {
        issue.message += "; propose clamp to [" +
                         std::to_string(rule->height_min) + ", " +
                         std::to_string(rule->height_max) + "]";
      }
      report.zoning_issues.push_back(std::move(issue));
    }

    if (building.fbcz_violation) {
      AuditIssue issue{};
      issue.code = "zoning.frontage_violation";
      issue.severity = 1;
      issue.message = "Building " + std::to_string(building.id) +
                      " frontage occupancy below configured minimum";
      report.zoning_issues.push_back(std::move(issue));
    }
  }
}

} // namespace

std::string AuditReport::ToJson() const {
  nlohmann::json json{};
  json["connectivity"] = nlohmann::json::array();
  json["zoning"] = nlohmann::json::array();

  for (const auto &issue : connectivity_issues) {
    json["connectivity"].push_back(
        {{"code", issue.code}, {"message", issue.message}, {"severity", issue.severity}});
  }
  for (const auto &issue : zoning_issues) {
    json["zoning"].push_back(
        {{"code", issue.code}, {"message", issue.message}, {"severity", issue.severity}});
  }

  return json.dump(2);
}

std::string AuditReport::ToHumanReadable() const {
  std::ostringstream out;
  out << "Connectivity Auditor:\n";
  if (connectivity_issues.empty()) {
    out << "- No connectivity issues detected.\n";
  } else {
    for (const auto &issue : connectivity_issues) {
      out << "- [" << issue.code << "] " << issue.message << "\n";
    }
  }

  out << "Zoning Enforcer:\n";
  if (zoning_issues.empty()) {
    out << "- No zoning issues detected.\n";
  } else {
    for (const auto &issue : zoning_issues) {
      out << "- [" << issue.code << "] " << issue.message << "\n";
    }
  }
  return out.str();
}

AuditReport BehaviorTreeAuditors::Run(const CityGenerator::CityOutput &city,
                                      const Config &config,
                                      const FbczRuleSet *rules) {
  AuditReport report{};

#if ROGUECITY_HAS_BTCPP
  BT::BehaviorTreeFactory factory;

  factory.registerSimpleAction("ConnectivityAudit", [&]() {
    if (config.run_connectivity_auditor) {
      RunConnectivityAudit(city, report, config.include_adjustment_proposals);
    }
    return BT::NodeStatus::SUCCESS;
  });

  factory.registerSimpleAction("ZoningAudit", [&]() {
    if (config.run_zoning_enforcer) {
      RunZoningAudit(city, rules, report, config.include_adjustment_proposals);
    }
    return BT::NodeStatus::SUCCESS;
  });

  static constexpr const char *kTreeText =
      "<root main_tree_to_execute='Main'>"
      "<BehaviorTree ID='Main'>"
      "  <Sequence>"
      "    <ConnectivityAudit/>"
      "    <ZoningAudit/>"
      "  </Sequence>"
      "</BehaviorTree>"
      "</root>";

  auto tree = factory.createTreeFromText(kTreeText);
  tree.tickWhileRunning();
#else
  if (config.run_connectivity_auditor) {
    RunConnectivityAudit(city, report, config.include_adjustment_proposals);
  }
  if (config.run_zoning_enforcer) {
    RunZoningAudit(city, rules, report, config.include_adjustment_proposals);
  }
#endif

  return report;
}

bool BehaviorTreeAuditors::WriteReports(const AuditReport &report,
                                        const std::string &json_path,
                                        const std::string &text_path,
                                        std::string *error) {
  std::ofstream json_out(json_path);
  if (!json_out.good()) {
    if (error != nullptr) {
      *error = "Unable to write JSON report: " + json_path;
    }
    return false;
  }

  std::ofstream text_out(text_path);
  if (!text_out.good()) {
    if (error != nullptr) {
      *error = "Unable to write text report: " + text_path;
    }
    return false;
  }

  json_out << report.ToJson();
  text_out << report.ToHumanReadable();
  return true;
}

} // namespace RogueCity::Generators::Policy
