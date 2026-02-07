#include "Url.h"

#include <algorithm>
#include <cctype>

namespace RogueCity::AI {

static bool StartsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

static std::string Trim(const std::string& s) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    size_t b = 0;
    while (b < s.size() && isSpace(static_cast<unsigned char>(s[b]))) b++;
    size_t e = s.size();
    while (e > b && isSpace(static_cast<unsigned char>(s[e - 1]))) e--;
    return s.substr(b, e - b);
}

ParsedUrl ParseUrl(const std::string& rawUrl) {
    ParsedUrl out;
    std::string url = Trim(rawUrl);
    if (url.empty()) return out;

    size_t schemePos = url.find("://");
    if (schemePos == std::string::npos) return out;

    std::string scheme = url.substr(0, schemePos);
    std::transform(scheme.begin(), scheme.end(), scheme.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (scheme == "http") {
        out.secure = false;
        out.port = 80;
    } else if (scheme == "https") {
        out.secure = true;
        out.port = 443;
    } else {
        return out;
    }

    size_t authorityStart = schemePos + 3;
    size_t pathStart = url.find('/', authorityStart);
    std::string authority = (pathStart == std::string::npos)
        ? url.substr(authorityStart)
        : url.substr(authorityStart, pathStart - authorityStart);

    if (authority.empty()) return out;

    size_t colonPos = authority.rfind(':');
    if (colonPos != std::string::npos) {
        out.host = authority.substr(0, colonPos);
        std::string portStr = authority.substr(colonPos + 1);
        if (!portStr.empty()) {
            try {
                int portInt = std::stoi(portStr);
                if (portInt <= 0 || portInt > 65535) return ParsedUrl{};
                out.port = static_cast<uint16_t>(portInt);
            } catch (...) {
                return ParsedUrl{};
            }
        }
    } else {
        out.host = authority;
    }

    if (out.host.empty()) return ParsedUrl{};

    if (pathStart != std::string::npos) {
        out.path = url.substr(pathStart);
        if (out.path.empty()) out.path = "/";
    } else {
        out.path = "/";
    }

    out.valid = true;
    return out;
}

std::string JoinUrlPath(const std::string& baseUrl, const std::string& path) {
    if (path.empty()) return baseUrl;
    if (baseUrl.empty()) return path;

    if (path[0] == '/') {
        if (baseUrl.back() == '/') return baseUrl.substr(0, baseUrl.size() - 1) + path;
        return baseUrl + path;
    }

    if (baseUrl.back() == '/') return baseUrl + path;
    return baseUrl + "/" + path;
}

} // namespace RogueCity::AI

