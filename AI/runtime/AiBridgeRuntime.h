#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <memory>

namespace RogueCity::AI {

enum class BridgeStatus {
    Offline,
    Starting,
    Online,
    Failed
};

/// Controls the AI bridge PowerShell process lifecycle
/// Handles pwsh/powershell fallback and health check polling
class AiBridgeRuntime {
public:
    static AiBridgeRuntime& Instance();
    
    bool StartBridge();
    void StopBridge();
    
    BridgeStatus GetStatus() const { return m_status; }
    bool IsOnline() const { return m_status == BridgeStatus::Online; }
    std::string GetStatusString() const;
    std::string GetLastError() const { return m_lastError; }
    
private:
    AiBridgeRuntime() = default;
    
    bool TryStartWithPwsh(const std::string& scriptPath);
    bool TryStartWithPowershell(const std::string& scriptPath);
    bool PollHealthEndpoint(int timeoutSec);
    bool ExecuteCommand(const std::string& command, std::string* outError);
    
    std::atomic<BridgeStatus> m_status{BridgeStatus::Offline};
    std::string m_lastError;
    std::unique_ptr<std::thread> m_startupThread;
};

} // namespace RogueCity::AI
