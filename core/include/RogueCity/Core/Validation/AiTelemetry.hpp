#pragma once

#include <mutex>
#include <string>


namespace RogueCity::Core::Validation {

/// @brief Global diagnostic sink for AI agents to parse.
/// Dumps runtime crashes and assertions into a structured JSON file for AI
/// agents to instantly ingest.
class AiTelemetry {
public:
  static AiTelemetry &Instance();

  /// @brief Manually dump a JSON payload describing an error.
  void DumpCrashContext(const std::string &module_name, const char *file,
                        int line, const std::string &message) const;

  /// @brief Evaluates an expression, and on failure, dumps context and aborts.
  static void Assert(bool condition, const std::string &module_name,
                     const char *file, int line, const std::string &message);

private:
  AiTelemetry() = default;
  ~AiTelemetry() = default;

  AiTelemetry(const AiTelemetry &) = delete;
  AiTelemetry &operator=(const AiTelemetry &) = delete;

  // Mutex to prevent interleaved dumps if multiple threads crash simultaneously
  mutable std::mutex m_mutex;
};

// Convenience macros for use across the project
#define RC_AI_ASSERT(condition, module_name, message)                          \
  ::RogueCity::Core::Validation::AiTelemetry::Assert(                          \
      (condition), (module_name), __FILE__, __LINE__, (message))

#define RC_AI_FATAL(module_name, message)                                      \
  ::RogueCity::Core::Validation::AiTelemetry::Instance().DumpCrashContext(     \
      (module_name), __FILE__, __LINE__, (message));                           \
  std::abort()

} // namespace RogueCity::Core::Validation
