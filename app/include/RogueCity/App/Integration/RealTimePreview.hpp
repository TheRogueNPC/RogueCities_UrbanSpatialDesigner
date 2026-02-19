#pragma once
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <functional>
#include <memory>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

namespace RogueCity::App {

/// Manages debounced real-time preview of city generation
/// Uses RogueWorker for background generation (>10ms operations)
class RealTimePreview {
public:
    using OnGenerationCompleteCallback = 
        std::function<void(const Generators::CityGenerator::CityOutput&)>;

    enum class GenerationPhase : uint8_t {
        Idle,
        InitStreetSweeper,
        Sweeping,
        Cancelled,
        StreetsSwept
    };

    RealTimePreview();
    ~RealTimePreview();

    /// Update (handles debouncing and background tasks)
    void update(float delta_time);

    /// Request city regeneration (debounced)
    void request_regeneration(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config);

    /// Force immediate regeneration (no debounce)
    void force_regeneration(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config);

    /// Set debounce delay (default: 0.5s)
    void set_debounce_delay(float seconds);

    /// Set callback for generation complete
    void set_on_complete(OnGenerationCompleteCallback callback);

    /// Check if generation is in progress
    [[nodiscard]] bool is_generating() const;

    /// Get generation progress (0.0 - 1.0)
    [[nodiscard]] float get_progress() const;

    /// Get last generated output (or nullptr)
    [[nodiscard]] const Generators::CityGenerator::CityOutput* get_output() const;

    /// Status phase for UI overlays
    [[nodiscard]] GenerationPhase phase() const;
    [[nodiscard]] float phase_elapsed_seconds() const;
    void cancel_generation();

private:
    float debounce_delay_{ 0.5f };
    float time_since_last_request_{ 0.0f };
    bool regeneration_pending_{ false };
    std::atomic<bool> is_generating_{ false };
    std::atomic<float> generation_progress_{ 0.0f };

    GenerationPhase phase_{ GenerationPhase::Idle };
    float phase_elapsed_seconds_{ 0.0f };
    bool init_pending_start_{ false };

    std::vector<Generators::CityGenerator::AxiomInput> pending_axioms_;
    Generators::CityGenerator::Config pending_config_;

    std::unique_ptr<Generators::CityGenerator::CityOutput> current_output_;

    std::unique_ptr<Generators::CityGenerator::CityOutput> completed_output_;
    bool completed_output_ready_{ false };
    mutable std::mutex completed_mutex_;

    std::jthread generation_thread_;
    std::atomic<uint64_t> generation_token_{ 0 };
    std::shared_ptr<Generators::CancellationToken> cancellation_token_{};

    OnGenerationCompleteCallback on_complete_;

    /// Start background generation task
    void start_generation();
    void cancel_inflight_generation();

    /// Handle generation completion (called from worker thread)
    void on_generation_complete(uint64_t token, Generators::CityGenerator::CityOutput output);
};

} // namespace RogueCity::App
