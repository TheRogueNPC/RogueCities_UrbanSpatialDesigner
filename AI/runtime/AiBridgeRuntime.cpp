#include "AiBridgeRuntime.h"
#include "config/AiConfig.h"
#include <iostream>
#include <chrono>
#include <cstdlib>

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

bool AiBridgeRuntime::ExecuteCommand(const std::string& command) {
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
        m_lastError = "Failed to create process";
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
    return ExecuteCommand(command);
}

bool AiBridgeRuntime::TryStartWithPowershell(const std::string& scriptPath) {
    std::string command = "powershell -NoProfile -ExecutionPolicy Bypass -File \"" + scriptPath + "\"";
    std::cout << "[AI] Attempting to start with powershell: " << command << std::endl;
    return ExecuteCommand(command);
}

bool AiBridgeRuntime::PollHealthEndpoint(int timeoutSec) {
    auto& config = AiConfigManager::Instance().GetConfig();
    std::string healthUrl = config.bridgeBaseUrl + "/health";
    
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
            HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 7077, 0);
            if (hConnect) {
                HINTERNET hRequest = WinHttpOpenRequest(
                    hConnect, L"GET", L"/health",
                    nullptr, WINHTTP_NO_REFERER,
                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                    0
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
    if (config.preferPwsh) {
        started = TryStartWithPwsh(scriptPath);
        if (!started) {
            std::cout << "[AI] pwsh failed, trying powershell..." << std::endl;
            started = TryStartWithPowershell(scriptPath);
        }
    } else {
        started = TryStartWithPowershell(scriptPath);
        if (!started) {
            std::cout << "[AI] powershell failed, trying pwsh..." << std::endl;
            started = TryStartWithPwsh(scriptPath);
        }
    }
    
    if (!started) {
        m_status = BridgeStatus::Failed;
        m_lastError = "Failed to start PowerShell process";
        return false;
    }
    
    // Poll health endpoint in background
    m_startupThread = std::make_unique<std::thread>([this, &config]() {
        if (PollHealthEndpoint(config.healthCheckTimeoutSec)) {
            m_status = BridgeStatus::Online;
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
    std::cout << "[AI] Bridge stopped" << std::endl;
}

} // namespace RogueCity::AI
