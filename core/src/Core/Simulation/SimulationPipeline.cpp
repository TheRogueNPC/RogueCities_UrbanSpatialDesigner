/**
 * @class SimulationPipeline
 * @brief Manages the simulation step execution pipeline for RogueCity.
 *
 * The SimulationPipeline class is responsible for executing simulation steps,
 * including physics, agent, and system updates, using a fixed timestep and
 * configurable maximum substeps. It accumulates time and ensures simulation
 * stages are executed in order, handling errors gracefully.
 *
 * @param config_ Simulation configuration parameters, including fixed timestep and max substeps.
 * @param time_accumulator_ Accumulates time between simulation steps.
 * @param frame_counter_ Tracks the number of simulation frames executed.
 *
 * @constructor
 * SimulationPipeline(const SimulationConfig& config)
 * Initializes the pipeline with the given configuration, setting sensible defaults if needed.
 *
 * @method
 * StepResult ExecuteStep(Editor::GlobalState& gs, float dt_seconds)
 * Executes simulation steps based on accumulated time and configuration.
 * Handles errors and updates frame counters.
 *
 * @method
 * void Reset() noexcept
 * Resets the time accumulator and frame counter.
 *
 * @method
 * bool StepPhysics(Editor::GlobalState& gs)
 * Executes the physics simulation stage. Returns true if successful.
 *
 * @method
 * bool StepAgents(Editor::GlobalState& gs)
 * Executes the agent simulation stage. Returns true if successful.
 *
 * @method
 * bool StepSystems(Editor::GlobalState& gs)
 * Executes the system simulation stage. Returns true if successful.
 */
 
#include "RogueCity/Core/Simulation/SimulationPipeline.hpp"

#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <algorithm>
#include <exception>

namespace RogueCity::Core::Simulation {

SimulationPipeline::SimulationPipeline(const SimulationConfig& config)
    : config_(config) {
    if (config_.fixed_timestep <= 0.0f) {
        config_.fixed_timestep = 1.0f / 60.0f;
    }
    if (config_.max_substeps == 0u) {
        config_.max_substeps = 1u;
    }
}

StepResult SimulationPipeline::ExecuteStep(Editor::GlobalState& gs, float dt_seconds) {
    StepResult result{};

    const float safe_dt = (dt_seconds > 0.0f) ? dt_seconds : config_.fixed_timestep;
    time_accumulator_ += safe_dt;

    try {
        while (time_accumulator_ + 1e-9f >= config_.fixed_timestep &&
               result.executed_substeps < config_.max_substeps) {
            if (!StepPhysics(gs) || !StepAgents(gs) || !StepSystems(gs)) {
                result.success = false;
                result.error_message = "Simulation stage failed";
                break;
            }

            time_accumulator_ = std::max(0.0f, time_accumulator_ - config_.fixed_timestep);
            ++result.executed_substeps;
            ++frame_counter_;
            ++gs.frame_counter;
        }
    } catch (const std::exception& ex) {
        result.success = false;
        result.error_message = ex.what();
    } catch (...) {
        result.success = false;
        result.error_message = "Unknown simulation error";
    }

    result.frame_counter_after = gs.frame_counter;
    return result;
}

void SimulationPipeline::Reset() noexcept {
    time_accumulator_ = 0.0f;
    frame_counter_ = 0u;
}

bool SimulationPipeline::StepPhysics(Editor::GlobalState& gs) {
    (void)gs;
    return true;
}

bool SimulationPipeline::StepAgents(Editor::GlobalState& gs) {
    (void)gs;
    return true;
}

bool SimulationPipeline::StepSystems(Editor::GlobalState& gs) {
    (void)gs;
    return true;
}

} // namespace RogueCity::Core::Simulation
