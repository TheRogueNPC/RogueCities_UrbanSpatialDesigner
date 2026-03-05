#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationInput.hpp"
#include "RogueCity/Generators/Policy/BehaviorTreeAuditors.hpp"
#include "RogueCity/Generators/Policy/FbczRuleEngine.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv) {
  using RogueCity::Generators::CityGenerator;
  using RogueCity::Generators::GenerationInput;
  using RogueCity::Generators::Policy::BehaviorTreeAuditors;
  using RogueCity::Generators::Policy::FbczRuleEngine;
  using RogueCity::Generators::Policy::FbczRuleSet;

  if (argc < 2) {
    std::cerr << "Usage: rc_policy_audit_tool <fbcz_rules.json> [out_prefix]\n";
    return 1;
  }

  const std::string rules_path = argv[1];
  const std::string prefix = (argc >= 3) ? argv[2] : "policy_audit";

  FbczRuleSet rules{};
  std::string error;
  if (!FbczRuleEngine::LoadFromJsonFile(rules_path, rules, &error)) {
    std::cerr << "Unable to load rules: " << error << '\n';
    return 2;
  }

  CityGenerator::Config cfg{};
  cfg.width = 1800;
  cfg.height = 1800;
  cfg.seed = 9001u;
  cfg.num_seeds = 18;
  cfg.enable_fbcz_rules = true;
  cfg.fbcz_rules_path = rules_path;

  GenerationInput input{};
  input.config = cfg;
  input.deterministic_seed = cfg.seed;

  CityGenerator generator{};
  auto city = generator.generate(input);

  auto report =
      BehaviorTreeAuditors::Run(city, BehaviorTreeAuditors::Config{}, &rules);
  if (!BehaviorTreeAuditors::WriteReports(report, prefix + ".json",
                                          prefix + ".txt", &error)) {
    std::cerr << "Unable to write reports: " << error << '\n';
    return 3;
  }

  std::cout << "Wrote policy reports: " << prefix << ".json and " << prefix
            << ".txt\n";
  std::cout << report.ToHumanReadable();
  return 0;
}
