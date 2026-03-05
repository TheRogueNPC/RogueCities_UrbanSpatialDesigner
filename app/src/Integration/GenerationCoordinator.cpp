#include "RogueCity/App/Integration/GenerationCoordinator.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <iostream>
#include <sstream>
#include <utility>

namespace RogueCity::App {

GenerationCoordinator::GenerationCoordinator() = default;
GenerationCoordinator::~GenerationCoordinator() = default;

void GenerationCoordinator::Update(float delta_time) {
    const bool was_generating = preview_.is_generating();
    const RealTimePreview::GenerationPhase current_phase = preview_.phase();

    preview_.update(delta_time);

    const bool is_generating = preview_.is_generating();
    const RealTimePreview::GenerationPhase new_phase = preview_.phase();

    if (new_phase != last_phase_) {
        std::ostringstream ss;
        ss << "[GEN] Phase transition: " << PhaseName(last_phase_) << " -> " << PhaseName(new_phase);
        LogEvent(ss.str());
        last_phase_ = new_phase;
    }

    if (is_generating) {
        float progress = preview_.get_progress();
        int progress_pct = static_cast<int>(progress * 100.0f);
        if (progress_pct / 10 != last_progress_report_ / 10) {
            std::ostringstream ss;
            ss << "[GEN] Progress: " << progress_pct << "%";
            LogEvent(ss.str());
            last_progress_report_ = progress_pct;
        }
    } else {
        last_progress_report_ = -1;
    }

    if (!was_generating && is_generating) {
        inflight_serial_ = scheduled_serial_;
        inflight_reason_ = scheduled_reason_;
        inflight_depth_ = scheduled_depth_;

        std::ostringstream ss;
        ss << "[GEN] Starting generation serial #" << inflight_serial_
           << " (Reason: " << ReasonName(inflight_reason_) << ")";
        LogEvent(ss.str());
    }

    if (was_generating && !is_generating && inflight_serial_ > completed_serial_) {
        completed_serial_ = inflight_serial_;
        completed_reason_ = inflight_reason_;
        completed_depth_ = inflight_depth_;

        std::ostringstream ss;
        ss << "[GEN] Completed generation serial #" << completed_serial_;

        const auto* output = preview_.get_output();
        if (output) {
            if (output->plan_approved) {
                ss << " [SUCCESS]";
            } else {
                ss << " [REJECTED] - " << output->plan_violations.size() << " violations";
                if (!output->plan_violations.empty()) {
                    ss << " (First: " << output->plan_violations.front().message << ")";
                }
            }
        }

        LogEvent(ss.str());
    }

}

void GenerationCoordinator::SetDebounceDelay(float seconds) {
    preview_.set_debounce_delay(seconds);
}

void GenerationCoordinator::SetOnComplete(OnGenerationCompleteCallback callback) {
    preview_.set_on_complete(std::move(callback));
}

void GenerationCoordinator::RequestRegeneration(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config,
    GenerationDepth depth,
    GenerationRequestReason reason) {
    ++scheduled_serial_;
    scheduled_reason_ = reason;
    scheduled_depth_ = depth;

    std::ostringstream ss;
    ss << "[GEN] Requesting regeneration serial #" << scheduled_serial_
       << " (Reason: " << ReasonName(reason) << ")";
    LogEvent(ss.str());

    preview_.request_regeneration(axioms, config, depth);
}

void GenerationCoordinator::RequestRegenerationIncremental(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config,
    const Generators::StageMask& dirty_stages,
    GenerationDepth depth,
    GenerationRequestReason reason) {
    ++scheduled_serial_;
    scheduled_reason_ = reason;
    scheduled_depth_ = depth;

    std::ostringstream ss;
    ss << "[GEN] Requesting incremental regeneration serial #" << scheduled_serial_
       << " (Reason: " << ReasonName(reason) << ")";
    LogEvent(ss.str());

    preview_.request_regeneration_incremental(axioms, config, dirty_stages, depth);
}

void GenerationCoordinator::ForceRegeneration(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config,
    GenerationDepth depth,
    GenerationRequestReason reason) {
    ++scheduled_serial_;
    scheduled_reason_ = reason;
    scheduled_depth_ = depth;

    std::ostringstream ss;
    ss << "[GEN] Forcing regeneration serial #" << scheduled_serial_
       << " (Reason: " << ReasonName(reason) << ")";
    LogEvent(ss.str());

    preview_.force_regeneration(axioms, config, depth);
}

void GenerationCoordinator::ForceRegenerationIncremental(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config,
    const Generators::StageMask& dirty_stages,
    GenerationDepth depth,
    GenerationRequestReason reason) {
    ++scheduled_serial_;
    scheduled_reason_ = reason;
    scheduled_depth_ = depth;

    std::ostringstream ss;
    ss << "[GEN] Forcing incremental regeneration serial #" << scheduled_serial_
       << " (Reason: " << ReasonName(reason) << ")";
    LogEvent(ss.str());

    preview_.force_regeneration_incremental(axioms, config, dirty_stages, depth);
}

void GenerationCoordinator::CancelGeneration() {
    LogEvent("[GEN] Cancelling generation");
    preview_.cancel_generation();
}

void GenerationCoordinator::ClearOutput() {
    LogEvent("[GEN] Clearing generation output");
    preview_.clear_output();
    scheduled_serial_ = 0;
    inflight_serial_ = 0;
    completed_serial_ = 0;
    scheduled_reason_ = GenerationRequestReason::Unknown;
    inflight_reason_ = GenerationRequestReason::Unknown;
    completed_reason_ = GenerationRequestReason::Unknown;
    scheduled_depth_ = GenerationDepth::FullPipeline;
    inflight_depth_ = GenerationDepth::FullPipeline;
    completed_depth_ = GenerationDepth::FullPipeline;
    last_phase_ = RealTimePreview::GenerationPhase::Idle;
    last_progress_report_ = -1;
}

bool GenerationCoordinator::IsGenerating() const {
    return preview_.is_generating();
}

float GenerationCoordinator::GetProgress() const {
    return preview_.get_progress();
}

const Generators::CityGenerator::CityOutput* GenerationCoordinator::GetOutput() const {
    return preview_.get_output();
}

std::string GenerationCoordinator::GetLastError() const {
    const auto* output = GetOutput();
    if (!output || output->plan_approved || output->plan_violations.empty()) {
        return "";
    }

    std::ostringstream ss;
    ss << output->plan_violations.size() << " violations. First: "
       << output->plan_violations.front().message;
    return ss.str();
}

RealTimePreview::GenerationPhase GenerationCoordinator::Phase() const {
    return preview_.phase();
}

float GenerationCoordinator::PhaseElapsedSeconds() const {
    return preview_.phase_elapsed_seconds();
}

uint64_t GenerationCoordinator::LastScheduledSerial() const {
    return scheduled_serial_;
}

uint64_t GenerationCoordinator::LastCompletedSerial() const {
    return completed_serial_;
}

GenerationRequestReason GenerationCoordinator::LastScheduledReason() const {
    return scheduled_reason_;
}

GenerationRequestReason GenerationCoordinator::LastCompletedReason() const {
    return completed_reason_;
}

GenerationDepth GenerationCoordinator::LastScheduledDepth() const {
    return scheduled_depth_;
}

GenerationDepth GenerationCoordinator::LastCompletedDepth() const {
    return completed_depth_;
}

const char* GenerationCoordinator::ReasonName(GenerationRequestReason reason) {
    switch (reason) {
        case GenerationRequestReason::LivePreview:
            return "live_preview";
        case GenerationRequestReason::ForceGenerate:
            return "force_generate";
        case GenerationRequestReason::ExternalRequest:
            return "external_request";
        case GenerationRequestReason::Unknown:
        default:
            return "unknown";
    }
}

RealTimePreview* GenerationCoordinator::Preview() {
    return &preview_;
}

const RealTimePreview* GenerationCoordinator::Preview() const {
    return &preview_;
}

void GenerationCoordinator::LogEvent(const std::string& msg) {
    std::cout << msg << std::endl;
    RogueCity::Core::Editor::GetGlobalState().infomatrix.pushEvent(
        RogueCity::Core::Editor::InfomatrixEvent::Category::Runtime, msg);
}

const char* GenerationCoordinator::PhaseName(RealTimePreview::GenerationPhase phase) {
    switch (phase) {
        case RealTimePreview::GenerationPhase::Idle: return "Idle";
        case RealTimePreview::GenerationPhase::InitStreetSweeper: return "InitStreetSweeper";
        case RealTimePreview::GenerationPhase::Sweeping: return "Sweeping";
        case RealTimePreview::GenerationPhase::Cancelled: return "Cancelled";
        case RealTimePreview::GenerationPhase::StreetsSwept: return "StreetsSwept";
        default: return "Unknown";
    }
}

} // namespace RogueCity::App
