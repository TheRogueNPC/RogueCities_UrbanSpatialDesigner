#include "RogueCity/App/Integration/RealTimePreview.hpp"

#include <algorithm>
#include <utility>

namespace RogueCity::App {

RealTimePreview::RealTimePreview() = default;
RealTimePreview::~RealTimePreview() = default;

void RealTimePreview::update(float delta_time) {
    time_since_last_request_ += delta_time;
    phase_elapsed_seconds_ += delta_time;

    // Apply completed output on the main thread.
    {
        std::unique_ptr<Generators::CityGenerator::CityOutput> completed;
        {
            std::lock_guard<std::mutex> lock(completed_mutex_);
            if (completed_output_ready_) {
                completed = std::move(completed_output_);
                completed_output_ready_ = false;
            }
        }

        if (completed) {
            current_output_ = std::move(completed);
            is_generating_.store(false, std::memory_order_relaxed);
            generation_progress_.store(1.0f, std::memory_order_relaxed);

            phase_ = GenerationPhase::StreetsSwept;
            phase_elapsed_seconds_ = 0.0f;

            if (on_complete_ && current_output_) {
                on_complete_(*current_output_);
            }
        }
    }

    // Fade the completion message back to Idle.
    if (phase_ == GenerationPhase::StreetsSwept) {
        constexpr float kCompleteHoldSeconds = 1.25f;
        if (phase_elapsed_seconds_ >= kCompleteHoldSeconds && !regeneration_pending_ && !is_generating()) {
            phase_ = GenerationPhase::Idle;
            phase_elapsed_seconds_ = 0.0f;
        }
    }

    // Debounce -> init phase.
    if (!is_generating() && regeneration_pending_ && !init_pending_start_ &&
        time_since_last_request_ >= debounce_delay_) {
        phase_ = GenerationPhase::InitStreetSweeper;
        phase_elapsed_seconds_ = 0.0f;
        init_pending_start_ = true;
    }

    // Init text reveal -> begin generation.
    if (phase_ == GenerationPhase::InitStreetSweeper && init_pending_start_) {
        constexpr float kInitRevealSeconds = 0.25f;
        if (phase_elapsed_seconds_ >= kInitRevealSeconds) {
            init_pending_start_ = false;
            phase_ = GenerationPhase::Sweeping;
            phase_elapsed_seconds_ = 0.0f;
            start_generation();
        }
    }

    // If we finished generating but more changes arrived while sweeping, restart after debounce.
    if (!is_generating() && regeneration_pending_ && phase_ == GenerationPhase::Sweeping) {
        phase_ = GenerationPhase::Idle;
        phase_elapsed_seconds_ = 0.0f;
    }
}

void RealTimePreview::request_regeneration(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config) {
    pending_axioms_ = axioms;
    pending_config_ = config;
    regeneration_pending_ = true;
    time_since_last_request_ = 0.0f;

    if (phase_ == GenerationPhase::StreetsSwept) {
        phase_ = GenerationPhase::Idle;
        phase_elapsed_seconds_ = 0.0f;
    }
}

void RealTimePreview::force_regeneration(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config) {
    pending_axioms_ = axioms;
    pending_config_ = config;
    regeneration_pending_ = true;
    time_since_last_request_ = debounce_delay_;

    // Jump straight into init/start sequence next update.
    phase_ = GenerationPhase::InitStreetSweeper;
    phase_elapsed_seconds_ = 0.0f;
    init_pending_start_ = true;
}

void RealTimePreview::set_debounce_delay(float seconds) {
    debounce_delay_ = std::max(0.0f, seconds);
}

void RealTimePreview::set_on_complete(OnGenerationCompleteCallback callback) {
    on_complete_ = std::move(callback);
}

bool RealTimePreview::is_generating() const {
    return is_generating_.load(std::memory_order_relaxed);
}

float RealTimePreview::get_progress() const {
    return generation_progress_.load(std::memory_order_relaxed);
}

const Generators::CityGenerator::CityOutput* RealTimePreview::get_output() const {
    return current_output_.get();
}

RealTimePreview::GenerationPhase RealTimePreview::phase() const {
    return phase_;
}

float RealTimePreview::phase_elapsed_seconds() const {
    return phase_elapsed_seconds_;
}

void RealTimePreview::start_generation() {
    if (is_generating_.exchange(true, std::memory_order_relaxed)) {
        return;
    }

    regeneration_pending_ = false;
    time_since_last_request_ = 0.0f;

    generation_progress_.store(0.0f, std::memory_order_relaxed);

    // Move the latest request into the worker.
    std::vector<Generators::CityGenerator::AxiomInput> axioms = std::move(pending_axioms_);
    Generators::CityGenerator::Config config = pending_config_;
    pending_axioms_.clear();

    const uint64_t token = generation_token_.fetch_add(1, std::memory_order_relaxed) + 1;

    generation_thread_ = std::jthread(
        [this, token, axioms = std::move(axioms), config](std::stop_token st) mutable {
            if (st.stop_requested()) {
                return;
            }

            generation_progress_.store(0.1f, std::memory_order_relaxed);

            Generators::CityGenerator generator;
            auto output = generator.generate(axioms, config);

            if (st.stop_requested()) {
                return;
            }

            generation_progress_.store(0.95f, std::memory_order_relaxed);
            on_generation_complete(token, std::move(output));
        }
    );
}

void RealTimePreview::on_generation_complete(uint64_t token, Generators::CityGenerator::CityOutput output) {
    // Drop stale completions if a newer generation was scheduled.
    if (token != generation_token_.load(std::memory_order_relaxed)) {
        return;
    }

    std::lock_guard<std::mutex> lock(completed_mutex_);
    completed_output_ = std::make_unique<Generators::CityGenerator::CityOutput>(std::move(output));
    completed_output_ready_ = true;
}

} // namespace RogueCity::App
