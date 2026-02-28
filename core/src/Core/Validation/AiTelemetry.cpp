#include "RogueCity/Core/Validation/AiTelemetry.hpp"

#include <chrono>
#include <cstdlib> // for std::abort()
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>


namespace RogueCity::Core::Validation {

namespace {
std::string GetCurrentTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#ifdef _WIN32
  localtime_s(&tm, &t);
#else
  tm = *std::localtime(&t);
#endif
  std::ostringstream ts;
  ts << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return ts.str();
}
} // namespace

AiTelemetry &AiTelemetry::Instance() {
  static AiTelemetry instance;
  return instance;
}

void AiTelemetry::DumpCrashContext(const std::string &module_name,
                                   const char *file, int line,
                                   const std::string &message) const {
  std::lock_guard<std::mutex> lock(m_mutex);

  nlohmann::json payload = {{"event", "fatal_assertion"},
                            {"timestamp", GetCurrentTimestamp()},
                            {"module", module_name},
                            {"file", file},
                            {"line", line},
                            {"message", message}};

  try {
    std::filesystem::create_directories("AI/diagnostics");
    std::ofstream out("AI/diagnostics/runtime_crash.json");
    if (out.is_open()) {
      out << payload.dump(2);
    }
  } catch (...) {
    // Suppress secondary exceptions during a crash dump
  }
}

void AiTelemetry::Assert(bool condition, const std::string &module_name,
                         const char *file, int line,
                         const std::string &message) {
  if (!condition) {
    Instance().DumpCrashContext(module_name, file, line, message);
    std::abort();
  }
}

} // namespace RogueCity::Core::Validation
