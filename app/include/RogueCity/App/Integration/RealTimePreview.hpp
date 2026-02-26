/**
 * @file RealTimePreview.hpp
 * @brief Manages debounced real-time preview of city generation in RogueCities.
 *
 * Provides an interface for requesting city regeneration (debounced or immediate),
 * tracking generation progress, handling background generation tasks, and notifying
 * when generation completes. Supports incremental regeneration for dirty stages,
 * cancellation, and UI overlay status reporting.
 *
 * Key Features:
 * - Debounced and immediate city regeneration requests.
 * - Incremental regeneration for dirty stages.
 * - Background generation using worker threads.
 * - Progress tracking and phase reporting.
 * - Callback notification on generation completion.
 * - Cancellation and output management.
 *
 * Usage:
 * - Call `request_regeneration` or `force_regeneration` to trigger city generation.
 * - Use `update` to handle debouncing and background task management.
 * - Set a callback with `set_on_complete` to receive generation results.
 * - Query progress, phase, and output for UI integration.
 */
 
#pragma once
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <functional>
#include <memory>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

namespace RogueCity::App {

enum class GenerationDepth : uint8_t {
    AxiomBounds = 0,
    FullPipeline
};

/// Manages debounced real-time preview of city generation
/// Uses RogueWorker for background generation (>10ms operations)
class RealTimePreview {
public:
    using OnGenerationCompleteCallback = 
        std::function<void(const Generators::CityGenerator::CityOutput&, GenerationDepth)>;

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
        const Generators::CityGenerator::Config& config,
        GenerationDepth depth = GenerationDepth::FullPipeline);

    /// Request incremental regeneration for dirty stages (debounced).
    void request_regeneration_incremental(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config,
        const Generators::StageMask& dirty_stages,
        GenerationDepth depth = GenerationDepth::FullPipeline);

    /// Force immediate regeneration (no debounce)
    void force_regeneration(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config,
        GenerationDepth depth = GenerationDepth::FullPipeline);

    /// Force immediate incremental regeneration for dirty stages.
    void force_regeneration_incremental(
        const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
        const Generators::CityGenerator::Config& config,
        const Generators::StageMask& dirty_stages,
        GenerationDepth depth = GenerationDepth::FullPipeline);

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
    void clear_output();

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
    bool pending_incremental_{ false };
    Generators::StageMask pending_dirty_stages_{ Generators::FullStageMask() };
    GenerationDepth pending_depth_{ GenerationDepth::FullPipeline };

    std::unique_ptr<Generators::CityGenerator::CityOutput> current_output_;

    std::unique_ptr<Generators::CityGenerator::CityOutput> completed_output_;
    GenerationDepth completed_output_depth_{ GenerationDepth::FullPipeline };
    bool completed_output_ready_{ false };
    mutable std::mutex completed_mutex_;

    std::jthread generation_thread_;
    std::atomic<uint64_t> generation_token_{ 0 };
    std::shared_ptr<Generators::CancellationToken> cancellation_token_{};
    Generators::CityGenerator incremental_generator_cache_{};
    std::mutex incremental_generator_mutex_{};

    OnGenerationCompleteCallback on_complete_;

    /// Start background generation task
    void start_generation();
    void cancel_inflight_generation();

    /// Handle generation completion (called from worker thread)
    void on_generation_complete(
        uint64_t token,
        Generators::CityGenerator::CityOutput output,
        GenerationDepth depth);
};

} // namespace RogueCity::App
