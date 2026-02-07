#pragma once
#include <string>

namespace RogueCity::AI {

/// Simple HTTP POST helper for JSON payloads
/// TODO: Replace this stub with actual HTTP client implementation
/// Options: cpr, libcurl, httplib.h, or WinHTTP on Windows
class HttpClient {
public:
    /// POST JSON to URL and return response body
    /// @param url Target URL (e.g. "http://127.0.0.1:7077/ui_agent")
    /// @param bodyJson JSON string to send as body
    /// @return Response body as string (empty on error)
    static std::string PostJson(const std::string& url, const std::string& bodyJson);
};

} // namespace RogueCity::AI
