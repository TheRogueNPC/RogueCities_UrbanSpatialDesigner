#pragma once
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <functional>
#include <memory>

namespace RogueCity::App {

/// Manages debounced real-time preview of city generation
/// Uses RogueWorker for background generation (>10ms operations)
class RealTimePreview {
public:
    using OnGenerationCompleteCallback = 
        std::function<void(const Generators::CityGenerator::CityOutput&)>;

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

private:
    float debounce_delay_{ 0.5f };
    float time_since_last_request_{ 0.0f };
    bool regeneration_pending_{ false };
    bool is_generating_{ false };
    float generation_progress_{ 0.0f };

    std::vector<Generators::CityGenerator::AxiomInput> pending_axioms_;
    Generators::CityGenerator::Config pending_config_;

    std::unique_ptr<Generators::CityGenerator::CityOutput> current_output_;
    std::unique_ptr<Generators::CityGenerator> generator_;

    OnGenerationCompleteCallback on_complete_;

    /// Start background generation task
    void start_generation();

    /// Handle generation completion (called from worker thread)
    void on_generation_complete(Generators::CityGenerator::CityOutput output);
};

} // namespace RogueCity::App
