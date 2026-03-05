#pragma once
#include <imgui.h>
#include <string>
#include <unordered_map>

namespace RC {

/// Loads SVG files on demand, rasterizes them with nanosvg/nanosvgrast, and
/// uploads them as RGBA OpenGL textures. Results are cached by (path, size_px)
/// so each unique (icon, size) pair is only loaded once.
///
/// Must be called with an active OpenGL context. Call Clear() before the
/// GL context is destroyed (see main.cpp teardown).
///
/// Usage:
///   ImTextureID tex = RC::SvgTextureCache::Get().Load(LC::Home, 20.f);
///   if (tex) ImGui::Image(tex, ImVec2(20, 20));
class SvgTextureCache {
public:
    static SvgTextureCache& Get();

    /// Load (or retrieve from cache) an SVG file rasterized at size_px × size_px.
    /// @param svgPath  Path to the .svg file (relative to working dir or absolute).
    /// @param size_px  Desired output texture dimension in pixels (default 24).
    /// @return  ImTextureID (cast from GLuint) ready for ImGui::Image, or 0 on failure.
    ImTextureID Load(const std::string& svgPath, float size_px = 24.0f);

    /// Release all cached GL textures. Must be called before GL context teardown.
    void Clear();

    ~SvgTextureCache() { Clear(); }

private:
    struct Entry {
        unsigned int gl_id = 0;
        int          size  = 0;
    };
    // Cache key: "<svgPath>@<size_px_int>"
    std::unordered_map<std::string, Entry> m_cache;

    SvgTextureCache()  = default;
    SvgTextureCache(const SvgTextureCache&) = delete;
    SvgTextureCache& operator=(const SvgTextureCache&) = delete;
};

} // namespace RC
