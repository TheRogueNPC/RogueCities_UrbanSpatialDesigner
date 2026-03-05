#pragma once

#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Policy/FbczRuleEngine.hpp"

#include <string>
#include <vector>

namespace RogueCity::Generators::Policy {

struct AuditIssue {
  std::string code{};
  std::string message{};
  int severity{1};
};

struct AuditReport {
  std::vector<AuditIssue> connectivity_issues{};
  std::vector<AuditIssue> zoning_issues{};

  [[nodiscard]] bool empty() const {
    return connectivity_issues.empty() && zoning_issues.empty();
  }

  [[nodiscard]] std::string ToJson() const;
  [[nodiscard]] std::string ToHumanReadable() const;
};

class BehaviorTreeAuditors {
public:
  struct Config {
    bool run_connectivity_auditor{true};
    bool run_zoning_enforcer{true};
    bool include_adjustment_proposals{true};
  };

  [[nodiscard]] static AuditReport
  Run(const CityGenerator::CityOutput &city, const Config &config,
      const FbczRuleSet *rules = nullptr);

  [[nodiscard]] static bool WriteReports(const AuditReport &report,
                                         const std::string &json_path,
                                         const std::string &text_path,
                                         std::string *error = nullptr);
};

} // namespace RogueCity::Generators::Policy
