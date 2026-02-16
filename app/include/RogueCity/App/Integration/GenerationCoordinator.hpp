#pragma once

#include "RogueCity/App/Integration/RealTimePreview.hpp"

#include <cstdint>
#include <vector>

namespace RogueCity::App {

enum class GenerationRequestReason : uint8_t {
    Unknown = 0,
    LivePreview,
    ForceGenerate,
    ExternalRequest
};

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
        GenerationRequestReason reason = GenerationRequestReason::LivePreview);

    void ForceRegeneration(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config,
        GenerationRequestReason reason = GenerationRequestReason::ForceGenerate);

    [[nodiscard]] bool IsGenerating() const;
    [[nodiscard]] float GetProgress() const;
    [[nodiscard]] const Generators::CityGenerator::CityOutput* GetOutput() const;
    [[nodiscard]] RealTimePreview::GenerationPhase Phase() const;
    [[nodiscard]] float PhaseElapsedSeconds() const;

    [[nodiscard]] uint64_t LastScheduledSerial() const;
    [[nodiscard]] uint64_t LastCompletedSerial() const;
    [[nodiscard]] GenerationRequestReason LastScheduledReason() const;
    [[nodiscard]] GenerationRequestReason LastCompletedReason() const;
    [[nodiscard]] static const char* ReasonName(GenerationRequestReason reason);

    [[nodiscard]] RealTimePreview* Preview();
    [[nodiscard]] const RealTimePreview* Preview() const;

private:
    RealTimePreview preview_{};
    uint64_t scheduled_serial_{ 0 };
    uint64_t inflight_serial_{ 0 };
    uint64_t completed_serial_{ 0 };
    GenerationRequestReason scheduled_reason_{ GenerationRequestReason::Unknown };
    GenerationRequestReason inflight_reason_{ GenerationRequestReason::Unknown };
    GenerationRequestReason completed_reason_{ GenerationRequestReason::Unknown };
};

} // namespace RogueCity::App
