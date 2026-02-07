#include "HttpClient.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace RogueCity::AI {

std::string HttpClient::PostJson(const std::string& url, const std::string& bodyJson) {
#ifdef _WIN32
    std::cout << "[HttpClient] POST to " << url << std::endl;
    
    // Parse URL (simplified for http://127.0.0.1:7077/endpoint)
    HINTERNET hSession = WinHttpOpen(
        L"RogueCity/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    
    if (!hSession) {
        std::cerr << "[HttpClient] Failed to open WinHTTP session" << std::endl;
        return "[]";
    }
    
    HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 7077, 0);
    if (!hConnect) {
        std::cerr << "[HttpClient] Failed to connect" << std::endl;
        WinHttpCloseHandle(hSession);
        return "[]";
    }
    
    // Extract path from URL (everything after :7077)
    size_t pathStart = url.find("/", url.find("7077") + 4);
    std::string path = (pathStart != std::string::npos) ? url.substr(pathStart) : "/";
    std::wstring wpath(path.begin(), path.end());
    
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"POST", wpath.c_str(),
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );
    
    if (!hRequest) {
        std::cerr << "[HttpClient] Failed to open request" << std::endl;
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "[]";
    }
    
    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL sent = WinHttpSendRequest(
        hRequest,
        headers.c_str(), -1,
        (LPVOID)bodyJson.c_str(), bodyJson.size(),
        bodyJson.size(), 0
    );
    
    if (!sent) {
        std::cerr << "[HttpClient] Failed to send request" << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "[]";
    }
    
    BOOL received = WinHttpReceiveResponse(hRequest, nullptr);
    if (!received) {
        std::cerr << "[HttpClient] Failed to receive response" << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "[]";
    }
    
    std::string response;
    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        char* buffer = new char[bytesAvailable + 1];
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buffer, bytesAvailable, &bytesRead)) {
            buffer[bytesRead] = '\0';
            response += buffer;
        }
        delete[] buffer;
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    std::cout << "[HttpClient] Response: " << response.substr(0, 100) << "..." << std::endl;
    return response;
#else
    std::cerr << "[HttpClient] Non-Windows not implemented yet" << std::endl;
    return "[]";
#endif
}

} // namespace RogueCity::AI
