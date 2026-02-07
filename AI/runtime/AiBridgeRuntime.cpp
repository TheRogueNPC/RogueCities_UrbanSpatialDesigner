#include "AiBridgeRuntime.h"
#include "config/AiConfig.h"
#include "tools/Url.h"
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace RogueCity::AI {

AiBridgeRuntime& AiBridgeRuntime::Instance() {
    static AiBridgeRuntime instance;
    return instance;
}

std::string AiBridgeRuntime::GetStatusString() const {
    switch (m_status) {
        case BridgeStatus::Offline: return "Offline";
        case BridgeStatus::Starting: return "Starting...";
        case BridgeStatus::Online: return "Online";
        case BridgeStatus::Failed: return "Failed";
        default: return "Unknown";
    }
}

static std::string Win32ErrorToString(DWORD err) {
#ifdef _WIN32
    if (err == 0) return {};
    LPSTR buffer = nullptr;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD len = FormatMessageA(flags, nullptr, err, 0, (LPSTR)&buffer, 0, nullptr);
    std::string msg = (len && buffer) ? std::string(buffer, buffer + len) : std::string("Unknown error");
    if (buffer) LocalFree(buffer);
    // Trim trailing newlines from FormatMessage
    while (!msg.empty() && (msg.back() == '\r' || msg.back() == '\n')) msg.pop_back();
    return msg;
#else
    (void)err;
    return {};
#endif
}

bool AiBridgeRuntime::ExecuteCommand(const std::string& command, std::string* outError) {
#ifdef _WIN32
    STARTUPINFOA si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Hide console window
    
    if (!CreateProcessA(
        nullptr,
        const_cast<char*>(command.c_str()),
        nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi)) {
        DWORD err = ::GetLastError();
        std::ostringstream oss;
        oss << "CreateProcess failed (" << err << "): " << Win32ErrorToString(err);
        if (outError) *outError = oss.str();
        return false;
    }
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
#else
    int result = system((command + " &").c_str());
    return result == 0;
#endif
}

bool AiBridgeRuntime::TryStartWithPwsh(const std::string& scriptPath) {
    std::string command = "pwsh -NoProfile -ExecutionPolicy Bypass -File \"" + scriptPath + "\"";
    std::cout << "[AI] Attempting to start with pwsh: " << command << std::endl;
    std::string err;
    bool ok = ExecuteCommand(command, &err);
    if (!ok && !err.empty()) std::cout << "[AI] pwsh start error: " << err << std::endl;
    return ok;
}

bool AiBridgeRuntime::TryStartWithPowershell(const std::string& scriptPath) {
    std::string command = "powershell -NoProfile -ExecutionPolicy Bypass -File \"" + scriptPath + "\"";
    std::cout << "[AI] Attempting to start with powershell: " << command << std::endl;
    std::string err;
    bool ok = ExecuteCommand(command, &err);
    if (!ok && !err.empty()) std::cout << "[AI] powershell start error: " << err << std::endl;
    return ok;
}

bool AiBridgeRuntime::PollHealthEndpoint(int timeoutSec) {
    auto& config = AiConfigManager::Instance().GetConfig();
    std::string healthUrl = JoinUrlPath(config.bridgeBaseUrl, "/health");
    ParsedUrl parsed = ParseUrl(healthUrl);
    if (!parsed.valid) {
        m_lastError = "Invalid bridge_base_url (cannot poll /health): " + config.bridgeBaseUrl;
        return false;
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime
        ).count();
        
        if (elapsed >= timeoutSec) {
            m_lastError = "Health check timeout";
            return false;
        }
        
#ifdef _WIN32
        // Simple WinHTTP GET request
        HINTERNET hSession = WinHttpOpen(
            L"RogueCity/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );
        
        if (hSession) {
            std::wstring whost(parsed.host.begin(), parsed.host.end());
            HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(), parsed.port, 0);
            if (hConnect) {
                std::wstring wpath(parsed.path.begin(), parsed.path.end());
                DWORD flags = parsed.secure ? WINHTTP_FLAG_SECURE : 0;
                HINTERNET hRequest = WinHttpOpenRequest(
                    hConnect, L"GET", wpath.c_str(),
                    nullptr, WINHTTP_NO_REFERER,
                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                    flags
                );
                
                if (hRequest) {
                    BOOL sent = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                                   WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
                    if (sent) {
                        BOOL received = WinHttpReceiveResponse(hRequest, nullptr);
                        if (received) {
                            DWORD statusCode = 0;
                            DWORD statusCodeSize = sizeof(statusCode);
                            WinHttpQueryHeaders(hRequest,
                                              WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                              nullptr, &statusCode, &statusCodeSize, nullptr);
                            
                            WinHttpCloseHandle(hRequest);
                            WinHttpCloseHandle(hConnect);
                            WinHttpCloseHandle(hSession);
                            
                            if (statusCode == 200) {
                                std::cout << "[AI] Bridge health check passed" << std::endl;
                                return true;
                            } else {
                                if (config.debugLogHttp) {
                                    std::cout << "[AI] Bridge health check status: " << statusCode << std::endl;
                                }
                            }
                        }
                    }
                    WinHttpCloseHandle(hRequest);
                }
                WinHttpCloseHandle(hConnect);
            }
            WinHttpCloseHandle(hSession);
        }
#endif
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

bool AiBridgeRuntime::StartBridge() {
    if (m_status == BridgeStatus::Online || m_status == BridgeStatus::Starting) {
        std::cout << "[AI] Bridge already " << GetStatusString() << std::endl;
        return m_status == BridgeStatus::Online;
    }
    
    m_status = BridgeStatus::Starting;
    m_lastError.clear();
    
    auto& config = AiConfigManager::Instance().GetConfig();
    std::string scriptPath = config.startScript;
    
    bool started = false;
    std::string firstError;
    std::string secondError;

    auto tryPwsh = [&]() {
        std::string command = "pwsh -NoProfile -ExecutionPolicy Bypass -File \"" + scriptPath + "\"";
        return ExecuteCommand(command, &firstError);
    };
    auto tryPowershell = [&]() {
        std::string command = "powershell -NoProfile -ExecutionPolicy Bypass -File \"" + scriptPath + "\"";
        return ExecuteCommand(command, &secondError);
    };
    
    auto tryBatFallback = [&]() {
        // Try BAT fallback script if PowerShell fails
        std::string batScript = "tools/Start_Ai_Bridge_Fallback.bat";
        std::string command = "cmd /c \"" + batScript + "\"";
        std::string batError;
        std::cout << "[AI] Attempting BAT fallback: " << batScript << std::endl;
        return ExecuteCommand(command, &batError);
    };

    if (config.preferPwsh) {
        std::cout << "[AI] Attempting to start with pwsh..." << std::endl;
        started = tryPwsh();
        if (!started) {
            std::cout << "[AI] pwsh failed (" << firstError << "), trying powershell..." << std::endl;
            started = tryPowershell();
        }
        if (!started) {
            std::cout << "[AI] powershell failed (" << secondError << "), trying BAT fallback..." << std::endl;
            started = tryBatFallback();
        }
    } else {
        std::cout << "[AI] Attempting to start with powershell..." << std::endl;
        started = tryPowershell();
        if (!started) {
            std::cout << "[AI] powershell failed (" << secondError << "), trying pwsh..." << std::endl;
            started = tryPwsh();
        }
        if (!started) {
            std::cout << "[AI] Both PowerShell variants failed, trying BAT fallback..." << std::endl;
            started = tryBatFallback();
        }
    }
    
    if (!started) {
        m_status = BridgeStatus::Failed;
        if (!firstError.empty() && !secondError.empty()) {
            m_lastError = "pwsh: " + firstError + " | powershell: " + secondError + " | BAT fallback also failed";
        } else if (!firstError.empty()) {
            m_lastError = firstError;
        } else if (!secondError.empty()) {
            m_lastError = secondError;
        } else {
            m_lastError = "Failed to start bridge (all methods failed)";
        }
        return false;
    }
    
    // Poll health endpoint in background
    m_startupThread = std::make_unique<std::thread>([this, &config]() {
        if (PollHealthEndpoint(config.healthCheckTimeoutSec)) {
            m_status = BridgeStatus::Online;
            m_lastError.clear();
        } else {
            m_status = BridgeStatus::Failed;
        }
    });
    m_startupThread->detach();
    
    return true;
}

void AiBridgeRuntime::StopBridge() {
    if (m_status == BridgeStatus::Offline) {
        return;
    }
    
    auto& config = AiConfigManager::Instance().GetConfig();
    std::string scriptPath = config.stopScript;
    
    if (config.preferPwsh) {
        TryStartWithPwsh(scriptPath);
    } else {
        TryStartWithPowershell(scriptPath);
    }
    
    m_status = BridgeStatus::Offline;
    m_lastError.clear();
    std::cout << "[AI] Bridge stopped" << std::endl;
}

} // namespace RogueCity::AI
