#pragma once

#include <cstdint>
#include <string>

namespace RogueCity::AI {

struct ParsedUrl {
    bool valid = false;
    bool secure = false; // https
    std::string host;
    uint16_t port = 0;
    std::string path = "/";
};

/// Minimal URL parser for http(s)://host[:port][/path]
/// Designed for local toolserver use; does not handle queries/fragments.
ParsedUrl ParseUrl(const std::string& url);

/// Join base URL (http(s)://host[:port][/optionalPath]) with a path like "/health".
/// Ensures exactly one slash between base and path.
std::string JoinUrlPath(const std::string& baseUrl, const std::string& path);

} // namespace RogueCity::AI

