#include "RogueCity/Core/Roads/RoadMDP.hpp"
#include "RogueCity/Generators/Roads/Policies.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <chrono>

using namespace RogueCity;
using namespace RogueCity::Core::Roads;
using namespace RogueCity::Generators::Roads;

// ============================================================================
// TEST UTILITIES (consistent with tests/test_core.cpp)
// ============================================================================

struct TestResult {
  bool passed{true};
  std::string name;
  std::string error_message;
  double execution_time_ms{0.0};
};

class TestRunner {
public:
  static void runTest(const std::string &test_name,
                      std::function<void()> test_func) {
    TestResult result;
    result.name = test_name;

    auto start = std::chrono::high_resolution_clock::now();
    try {
      test_func();
      result.passed = true;
    } catch (const std::exception &e) {
      result.passed = false;
      result.error_message = e.what();
    }
    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time_ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    printResult(result);
    results_.push_back(result);
  }

  static void printSummary() {
    std::cout << "\n========================================\n";
    std::cout << "TEST SUMMARY\n";
    std::cout << "========================================\n";

    int passed = 0;
    int failed = 0;
    double total_time = 0.0;

    for (const auto &result : results_) {
      if (result.passed)
        passed++;
      else
        failed++;
      total_time += result.execution_time_ms;
    }

    std::cout << "Total tests: " << results_.size() << "\n";
    std::cout << "Passed: " << passed << "\n";
    std::cout << "Failed: " << failed << "\n";
    std::cout << "Total time: " << total_time << " ms\n";
    std::cout << "========================================\n";
  }

  static bool allPassed() {
    for (const auto &result : results_) {
      if (!result.passed) return false;
    }
    return true;
  }

private:
  static void printResult(const TestResult &result) {
    if (result.passed) {
      std::cout << "[PASS] " << result.name << " (" << result.execution_time_ms
                << " ms)\n";
    } else {
      std::cout << "[FAIL] " << result.name << " - " << result.error_message
                << " (" << result.execution_time_ms << " ms)\n";
    }
  }

  static std::vector<TestResult> results_;
};

std::vector<TestResult> TestRunner::results_;

#define TEST(name) TestRunner::runTest(#name, name)

#define ASSERT_EQUAL(a, b)                                                     \
  if ((a) != (b))                                                              \
  throw std::runtime_error("Assertion failed: " #a " != " #b)

// ============================================================================
// POLICY TESTS
// ============================================================================

void test_organic_policy() {
  OrganicPolicy policy;
  RoadState state{};
  uint32_t seed = 42;

  // Case 1: Low density area
  state.local_density = 0.5f;
  ASSERT_EQUAL(policy.ChooseAction(state, seed), RoadAction::FOLLOW_TENSOR);

  // Case 2: Borderline low density area
  state.local_density = 0.8f;
  ASSERT_EQUAL(policy.ChooseAction(state, seed), RoadAction::FOLLOW_TENSOR);

  // Case 3: High density area
  state.local_density = 0.81f;
  ASSERT_EQUAL(policy.ChooseAction(state, seed), RoadAction::TERMINATE_CUL_DE_SAC);

  // Case 4: Very high density area
  state.local_density = 1.0f;
  ASSERT_EQUAL(policy.ChooseAction(state, seed), RoadAction::TERMINATE_CUL_DE_SAC);
}

void test_grid_policy() {
  GridPolicy policy;
  RoadState state{};
  uint32_t seed = 42;

  // Case 1: Close to major road
  state.distance_to_major = 10.0f;
  ASSERT_EQUAL(policy.ChooseAction(state, seed), RoadAction::FORCE_GRID);

  // Case 2: Borderline close to major road
  state.distance_to_major = 49.9f;
  ASSERT_EQUAL(policy.ChooseAction(state, seed), RoadAction::FORCE_GRID);

  // Case 3: Far from major road
  state.distance_to_major = 50.0f;
  ASSERT_EQUAL(policy.ChooseAction(state, seed), RoadAction::SNAP_TO_INTERSECTION);

  // Case 4: Very far from major road
  state.distance_to_major = 1000.0f;
  ASSERT_EQUAL(policy.ChooseAction(state, seed), RoadAction::SNAP_TO_INTERSECTION);
}

void test_follow_tensor_policy() {
  FollowTensorPolicy policy;
  RoadState state{};
  uint32_t seed = 42;

  // Should always follow tensor regardless of state
  state.local_density = 0.9f;
  state.distance_to_major = 10.0f;
  ASSERT_EQUAL(policy.ChooseAction(state, seed), RoadAction::FOLLOW_TENSOR);

  state.local_density = 0.1f;
  state.distance_to_major = 100.0f;
  ASSERT_EQUAL(policy.ChooseAction(state, seed), RoadAction::FOLLOW_TENSOR);
}

int main() {
  std::cout << "========================================\n";
  std::cout << "ROAD POLICIES UNIT TESTS\n";
  std::cout << "========================================\n\n";

  TEST(test_organic_policy);
  TEST(test_grid_policy);
  TEST(test_follow_tensor_policy);

  TestRunner::printSummary();

  return TestRunner::allPassed() ? 0 : 1;
}
