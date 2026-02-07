#include "HttpClient.h"
#include <iostream>

// TODO: Implement actual HTTP client using:
// - cpr: https://github.com/libcpr/cpr (modern C++ wrapper for libcurl)
// - httplib.h: https://github.com/yhirose/cpp-httplib (header-only)
// - WinHTTP (Windows only)
// - libcurl (cross-platform)

namespace RogueCity::AI {

std::string HttpClient::PostJson(const std::string& url, const std::string& bodyJson) {
    std::cerr << "[HttpClient] STUB: Would POST to " << url << "\n";
    std::cerr << "[HttpClient] Body: " << bodyJson << "\n";
    std::cerr << "[HttpClient] TODO: Implement actual HTTP POST\n";
    
    // Return empty for now (caller should handle gracefully)
    return "[]"; // Empty command list
}

} // namespace RogueCity::AI
