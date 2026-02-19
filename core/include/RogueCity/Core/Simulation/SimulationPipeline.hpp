#pragma once

#include <cstdint>
#include <string>

namespace RogueCity::Core::Editor {
struct GlobalState;
}

namespace RogueCity::Core::Simulation {

/// Fixed-step simulation runtime configuration.
struct SimulationConfig {
  /// Fixed simulation delta in seconds.
  float fixed_timestep{1.0f / 60.0f};
  /// Safety cap for accumulated substeps in a single frame update.
  uint32_t max_substeps{4};
};

/// Result object returned by each simulation tick.
struct StepResult {
  /// True when all subsystems stepped without fatal errors.
  bool success{true};
  /// Number of fixed substeps executed for this frame.
  uint32_t executed_substeps{0};
  /// Global frame counter value after successful stepping.
  uint64_t frame_counter_after{0};
  /// Diagnostic message populated on failures.
  std::string error_message{};
};

/// Deterministic fixed-step simulation orchestrator.
class SimulationPipeline {
public:
  /// Construct pipeline with fixed-step configuration.
  explicit SimulationPipeline(
      const SimulationConfig &config = SimulationConfig{});

  /// Execute a frame update using fixed-step accumulation.
  [[nodiscard]] StepResult ExecuteStep(Editor::GlobalState &gs,
                                       float dt_seconds);
  /// Reset internal accumulators and counters.
  void Reset() noexcept;

  /// Access active pipeline configuration.
  [[nodiscard]] const SimulationConfig &config() const noexcept {
    return config_;
  }

private:
  /// Step physics systems for one fixed timestep.
  [[nodiscard]] bool StepPhysics(Editor::GlobalState &gs);
  /// Step agent systems for one fixed timestep.
  [[nodiscard]] bool StepAgents(Editor::GlobalState &gs);
  /// Step remaining systems for one fixed timestep.
  [[nodiscard]] bool StepSystems(Editor::GlobalState &gs);

  SimulationConfig config_{};
  float time_accumulator_{0.0f};
  uint64_t frame_counter_{0};
};

} // namespace RogueCity::Core::Simulation
