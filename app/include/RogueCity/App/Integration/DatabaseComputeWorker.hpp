#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <string_view>
#include <thread>

namespace RogueCity::App::Integration {

/**
 * @brief Background compute thread that bridges SpacetimeDB ↔ CityGenerator.
 *
 * Scalability notes:
 * - Push calls are batched per entity type in tight loops (no per-entity thread
 *   dispatch). SpacetimeDB multiplexes reducer calls over a single WebSocket.
 * - A monotonic `generation_serial` is stamped on every pushed row so
 *   downstream subscribers can detect stale data without full table scans.
 * - Push is throttled by `min_push_interval_ms` to avoid flooding the DB
 *   when the dirty flag fires rapidly during interactive axiom editing.
 */
class DatabaseComputeWorker {
public:
  DatabaseComputeWorker(Core::Editor::GlobalState &global_state,
                        std::string_view module_name,
                        std::string_view server_url);
  ~DatabaseComputeWorker();

  void Start();
  void Stop();

  /// Called by FFI subscription callbacks when DB state changes.
  void OnAxiomsChanged();

  /// Current generation serial (monotonically increasing).
  [[nodiscard]] uint64_t GetGenerationSerial() const {
    return generation_serial_.load(std::memory_order_relaxed);
  }

  // Scalability configuration
  uint32_t min_push_interval_ms{200}; // Debounce rapid edits
  uint32_t batch_log_threshold{1000}; // Log progress every N entities

private:
  void ComputeLoop();
  void RunGeneratorPass();

  Core::Editor::GlobalState &global_state_;
  std::string module_name_;
  std::string server_url_;

  std::atomic<bool> is_running_{false};
  std::atomic<bool> generator_dirty_flag_{false};
  std::atomic<uint64_t> generation_serial_{0};
  std::unique_ptr<std::thread> compute_thread_;
};

} // namespace RogueCity::App::Integration
