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
#include "RogueCity/Core/Validation/EditorOverlayValidation.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <unordered_set>

namespace RogueCity::Core::Simulation {

namespace {

[[nodiscard]] bool IsFiniteVec(const Vec2& v) {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

[[nodiscard]] uint64_t SelectionKey(const Editor::SelectionItem& item) {
    return (static_cast<uint64_t>(static_cast<uint8_t>(item.kind)) << 32ull) |
        static_cast<uint64_t>(item.id);
}

} // namespace

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
    last_stage_error_.clear();

    if (!config_.deterministic_mode) {
        if (!StepPhysics(gs) || !StepAgents(gs) || !StepSystems(gs)) {
            result.success = false;
            result.error_message = last_stage_error_.empty()
                ? "Simulation stage failed"
                : last_stage_error_;
            result.frame_counter_after = gs.frame_counter;
            return result;
        }
        result.executed_substeps = 1u;
        ++frame_counter_;
        ++gs.frame_counter;
        result.frame_counter_after = gs.frame_counter;
        return result;
    }

    const float safe_dt = (dt_seconds > 0.0f) ? dt_seconds : config_.fixed_timestep;
    time_accumulator_ += safe_dt;

    try {
        while (time_accumulator_ + 1e-9f >= config_.fixed_timestep &&
               result.executed_substeps < config_.max_substeps) {
            if (!StepPhysics(gs) || !StepAgents(gs) || !StepSystems(gs)) {
                result.success = false;
                result.error_message = last_stage_error_.empty()
                    ? "Simulation stage failed"
                    : last_stage_error_;
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
    last_stage_error_.clear();
}

bool SimulationPipeline::StepPhysics(Editor::GlobalState& gs) {
    for (const auto& road : gs.roads) {
        for (const auto& point : road.points) {
            if (!IsFiniteVec(point)) {
                last_stage_error_ = "Simulation physics failed: non-finite road coordinate";
                return false;
            }
        }
    }

    for (const auto& district : gs.districts) {
        for (const auto& point : district.border) {
            if (!IsFiniteVec(point)) {
                last_stage_error_ = "Simulation physics failed: non-finite district coordinate";
                return false;
            }
        }
    }

    for (const auto& lot : gs.lots) {
        if (!IsFiniteVec(lot.centroid)) {
            last_stage_error_ = "Simulation physics failed: non-finite lot centroid";
            return false;
        }
        for (const auto& point : lot.boundary) {
            if (!IsFiniteVec(point)) {
                last_stage_error_ = "Simulation physics failed: non-finite lot boundary coordinate";
                return false;
            }
        }
    }

    for (const auto& building : gs.buildings) {
        if (!IsFiniteVec(building.position)) {
            last_stage_error_ = "Simulation physics failed: non-finite building coordinate";
            return false;
        }
    }

    return true;
}

bool SimulationPipeline::StepAgents(Editor::GlobalState& gs) {
    std::unordered_set<uint32_t> road_ids;
    std::unordered_set<uint32_t> district_ids;
    std::unordered_set<uint32_t> lot_ids;
    std::unordered_set<uint32_t> building_ids;
    std::unordered_set<uint32_t> water_ids;

    road_ids.reserve(gs.roads.size());
    district_ids.reserve(gs.districts.size());
    lot_ids.reserve(gs.lots.size());
    building_ids.reserve(gs.buildings.size());
    water_ids.reserve(gs.waterbodies.size());

    for (const auto& road : gs.roads) {
        road_ids.insert(road.id);
    }
    for (const auto& district : gs.districts) {
        district_ids.insert(district.id);
    }
    for (const auto& lot : gs.lots) {
        lot_ids.insert(lot.id);
    }
    for (const auto& building : gs.buildings) {
        building_ids.insert(building.id);
    }
    for (const auto& water : gs.waterbodies) {
        water_ids.insert(water.id);
    }
    auto is_valid = [&](const Editor::SelectionItem& item) {
        if (item.id == 0u) {
            return false;
        }
        switch (item.kind) {
            case Editor::VpEntityKind::Axiom: return true;
            case Editor::VpEntityKind::Road: return road_ids.contains(item.id);
            case Editor::VpEntityKind::District: return district_ids.contains(item.id);
            case Editor::VpEntityKind::Lot: return lot_ids.contains(item.id);
            case Editor::VpEntityKind::Building: return building_ids.contains(item.id);
            case Editor::VpEntityKind::Water: return water_ids.contains(item.id);
            case Editor::VpEntityKind::Block: return true;
            case Editor::VpEntityKind::Unknown:
            default:
                return false;
        }
    };

    std::vector<Editor::SelectionItem> filtered;
    filtered.reserve(gs.selection_manager.Count());
    std::unordered_set<uint64_t> dedupe;
    dedupe.reserve(gs.selection_manager.Count());
    for (const auto& item : gs.selection_manager.Items()) {
        if (!is_valid(item)) {
            continue;
        }
        const uint64_t key = SelectionKey(item);
        if (dedupe.insert(key).second) {
            filtered.push_back(item);
        }
    }
    gs.selection_manager.SetItems(std::move(filtered));

    if (gs.hovered_entity.has_value() && !is_valid(*gs.hovered_entity)) {
        gs.hovered_entity.reset();
    }

    return true;
}

bool SimulationPipeline::StepSystems(Editor::GlobalState& gs) {
    if (!gs.validation_overlay.enabled) {
        gs.validation_overlay.errors.clear();
        return true;
    }

    float min_lot_area = 1.0f;
    if (gs.active_city_spec.has_value()) {
        min_lot_area = std::max(1.0f, gs.active_city_spec->zoningConstraints.minLotArea);
    }
    auto errors = Validation::CollectOverlayValidationErrors(gs, min_lot_area);

    if (!gs.validation_overlay.show_warnings) {
        errors.erase(
            std::remove_if(
                errors.begin(),
                errors.end(),
                [](const Editor::ValidationError& error) {
                    return error.severity == Editor::ValidationSeverity::Warning;
                }),
            errors.end());
    }

    if (config_.deterministic_mode) {
        std::sort(
            errors.begin(),
            errors.end(),
            [](const Editor::ValidationError& a, const Editor::ValidationError& b) {
                if (a.severity != b.severity) {
                    return static_cast<uint8_t>(a.severity) > static_cast<uint8_t>(b.severity);
                }
                if (a.entity_kind != b.entity_kind) {
                    return static_cast<uint8_t>(a.entity_kind) < static_cast<uint8_t>(b.entity_kind);
                }
                if (a.entity_id != b.entity_id) {
                    return a.entity_id < b.entity_id;
                }
                if (a.message != b.message) {
                    return a.message < b.message;
                }
                if (a.world_position.x != b.world_position.x) {
                    return a.world_position.x < b.world_position.x;
                }
                return a.world_position.y < b.world_position.y;
            });
    }

    gs.validation_overlay.errors = std::move(errors);
    return true;
}

} // namespace RogueCity::Core::Simulation
