#pragma once

#include "RogueCity/App/Integration/RealTimePreview.hpp"

#include <cstdint>
#include <vector>

namespace RogueCity::App {

// this class manages city generation requests, including debouncing, tracking in-flight generation, and invoking callbacks on completion. It serves as the main interface for panels/tools that need to trigger city generation and receive results asynchronously.
enum class GenerationRequestReason : uint8_t {
    Unknown = 0,
    LivePreview,
    ForceGenerate,
    ExternalRequest
};
// Represents the depth of generation to perform for a given request, allowing callers to specify whether they want to run the full pipeline or just specific stages.
enum class GenerationDepth : uint8_t {
    FullPipeline = 0,
    PartialPipeline
};
// Coordinates city generation requests, manages their lifecycle, and provides status updates and results to the rest of the application. It handles debouncing of rapid requests, cancellation of in-flight generation, and ensures that callbacks are invoked with the correct context when generation completes.
class GenerationCoordinator {
public:
    using OnGenerationCompleteCallback = RealTimePreview::OnGenerationCompleteCallback;

    GenerationCoordinator();
    ~GenerationCoordinator();

    void Update(float delta_time);

    void SetDebounceDelay(float seconds);
    void SetOnComplete(OnGenerationCompleteCallback callback);

    void RequestRegeneration(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config,
        GenerationDepth depth = GenerationDepth::FullPipeline,
        GenerationRequestReason reason = GenerationRequestReason::LivePreview);

    void RequestRegenerationIncremental(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config,
        const Generators::StageMask& dirty_stages,
        GenerationDepth depth = GenerationDepth::FullPipeline,
        GenerationRequestReason reason = GenerationRequestReason::LivePreview);

    void ForceRegeneration(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config,
        GenerationDepth depth = GenerationDepth::FullPipeline,
        GenerationRequestReason reason = GenerationRequestReason::ForceGenerate);

    void ForceRegenerationIncremental(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config,
        const Generators::StageMask& dirty_stages,
        GenerationDepth depth = GenerationDepth::FullPipeline,
        GenerationRequestReason reason = GenerationRequestReason::ForceGenerate);
    void CancelGeneration();
//todo these status querys need to display to the event log and to the debug console for accurate tracking of whats doing what and why. 
    [[nodiscard]] bool IsGenerating() const; 
    [[nodiscard]] float GetProgress() const;
    [[nodiscard]] const Generators::CityGenerator::CityOutput* GetOutput() const;
    [[nodiscard]] RealTimePreview::GenerationPhase Phase() const;
    [[nodiscard]] float PhaseElapsedSeconds() const;

    [[nodiscard]] uint64_t LastScheduledSerial() const;
    [[nodiscard]] uint64_t LastCompletedSerial() const;
    [[nodiscard]] GenerationRequestReason LastScheduledReason() const;
    [[nodiscard]] GenerationRequestReason LastCompletedReason() const;
    [[nodiscard]] GenerationDepth LastScheduledDepth() const;
    [[nodiscard]] GenerationDepth LastCompletedDepth() const;
    [[nodiscard]] static const char* ReasonName(GenerationRequestReason reason);

    [[nodiscard]] RealTimePreview* Preview();
    [[nodiscard]] const RealTimePreview* Preview() const;

    //this private section is for tracking the state of generation requests and their associated metadata, allowing the coordinator to manage debouncing, cancellation, and callback invocation correctly based on the lifecycle of each request. The serial numbers help ensure that callbacks are only invoked for the most recent request
private:
    RealTimePreview preview_{};
    uint64_t scheduled_serial_{ 0 };
    uint64_t inflight_serial_{ 0 };
    uint64_t completed_serial_{ 0 };
    GenerationRequestReason scheduled_reason_{ GenerationRequestReason::Unknown };
    GenerationRequestReason inflight_reason_{ GenerationRequestReason::Unknown };
    GenerationRequestReason completed_reason_{ GenerationRequestReason::Unknown };
    GenerationDepth scheduled_depth_{ GenerationDepth::FullPipeline };
    GenerationDepth inflight_depth_{ GenerationDepth::FullPipeline };
    GenerationDepth completed_depth_{ GenerationDepth::FullPipeline };
};

} // namespace RogueCity::App
