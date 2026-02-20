#include "RogueCity/App/Integration/GenerationCoordinator.hpp"

#include <utility>

namespace RogueCity::App {

GenerationCoordinator::GenerationCoordinator() = default;
GenerationCoordinator::~GenerationCoordinator() = default;

void GenerationCoordinator::Update(float delta_time) {
    const bool was_generating = preview_.is_generating();
    preview_.update(delta_time);
    const bool is_generating = preview_.is_generating();

    if (!was_generating && is_generating) {
        inflight_serial_ = scheduled_serial_;
        inflight_reason_ = scheduled_reason_;
    }

    if (was_generating && !is_generating && inflight_serial_ > completed_serial_) {
        completed_serial_ = inflight_serial_;
        completed_reason_ = inflight_reason_;
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
    GenerationRequestReason reason) {
    ++scheduled_serial_;
    scheduled_reason_ = reason;
    preview_.request_regeneration(axioms, config);
}

void GenerationCoordinator::RequestRegenerationIncremental(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config,
    const Generators::StageMask& dirty_stages,
    GenerationRequestReason reason) {
    ++scheduled_serial_;
    scheduled_reason_ = reason;
    preview_.request_regeneration_incremental(axioms, config, dirty_stages);
}

void GenerationCoordinator::ForceRegeneration(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config,
    GenerationRequestReason reason) {
    ++scheduled_serial_;
    scheduled_reason_ = reason;
    preview_.force_regeneration(axioms, config);
}

void GenerationCoordinator::ForceRegenerationIncremental(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config,
    const Generators::StageMask& dirty_stages,
    GenerationRequestReason reason) {
    ++scheduled_serial_;
    scheduled_reason_ = reason;
    preview_.force_regeneration_incremental(axioms, config, dirty_stages);
}

void GenerationCoordinator::CancelGeneration() {
    preview_.cancel_generation();
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

} // namespace RogueCity::App
