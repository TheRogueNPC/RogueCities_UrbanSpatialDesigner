#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

namespace RogueCity::Generators {

/// Cooperative cancellation token shared across generation stages.
class CancellationToken {
public:
  /// Signal cancellation to all observers.
  void Cancel() noexcept {
    // release: publish cancellation to worker threads without full seq-cst
    // cost.
    cancelled_.store(true, std::memory_order_release);
  }

  /// Check whether cancellation has been signaled.
  [[nodiscard]] bool IsCancelled() const noexcept {
    // acquire pairs with Cancel() release to observe the latest cancellation
    // state.
    return cancelled_.load(std::memory_order_acquire);
  }

  /// Reset token to non-cancelled state for reuse.
  void Reset() noexcept { cancelled_.store(false, std::memory_order_release); }

private:
  std::atomic<bool> cancelled_{false};
};

/// Per-generation execution context for cancellation and budget limits.
struct GenerationContext {
  std::shared_ptr<CancellationToken> cancellation =
      std::make_shared<CancellationToken>();
  uint64_t iteration_count{0};
  uint64_t max_iterations{0};

  /// Return true when cancellation or iteration budget requires early abort.
  [[nodiscard]] bool ShouldAbort() const noexcept {
    // Called in stage loops (roughly once per seed/element) to keep
    // cancellation response low-latency.
    const bool cancelled = cancellation && cancellation->IsCancelled();
    const bool over_budget =
        max_iterations > 0u && iteration_count >= max_iterations;
    return cancelled || over_budget;
  }

  /// Increment iteration counter tracked by the generation loop.
  void BumpIterations(uint64_t delta = 1u) noexcept {
    iteration_count += delta;
  }
};

} // namespace RogueCity::Generators
