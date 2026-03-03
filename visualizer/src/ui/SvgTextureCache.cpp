// SvgTextureCache.cpp
// nanosvg implementation defines must appear exactly once (this translation unit).
#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#include <GL/gl3w.h>
#include <RogueCity/Visualizer/SvgTextureCache.hpp>

#include <vector>
#include <string>

namespace RC {

SvgTextureCache& SvgTextureCache::Get() {
    static SvgTextureCache s_instance;
    return s_instance;
}

ImTextureID SvgTextureCache::Load(const std::string& svgPath, float size_px) {
    // Build cache key: "path@size"
    std::string key = svgPath + "@" + std::to_string(static_cast<int>(size_px));

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        return static_cast<ImTextureID>(it->second.gl_id);
    }

    // Parse SVG from file
    NSVGimage* svg = nsvgParseFromFile(svgPath.c_str(), "px", 96.0f);
    if (!svg || svg->width <= 0.0f || svg->height <= 0.0f) {
        if (svg) nsvgDelete(svg);
        return 0;
    }

    // Lucide icons are always square (24×24 viewBox); scale to requested size.
    const int  tex_w = static_cast<int>(size_px);
    const int  tex_h = static_cast<int>(size_px);
    const float scale = size_px / svg->width;

    // Rasterize → RGBA pixel buffer
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    std::vector<unsigned char> pixels(static_cast<size_t>(tex_w * tex_h * 4), 0);
    nsvgRasterize(rast, svg, 0.0f, 0.0f, scale, pixels.data(), tex_w, tex_h, tex_w * 4);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(svg);

    // Upload to OpenGL (requires active GL context)
    GLuint tex_id = 0;
    glGenTextures(1, &tex_id);
    if (tex_id == 0) return 0;

    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_w, tex_h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    m_cache[key] = Entry{tex_id, tex_w};
    return static_cast<ImTextureID>(tex_id);
}

void SvgTextureCache::Clear() {
    for (auto& [key, entry] : m_cache) {
        if (entry.gl_id != 0)
            glDeleteTextures(1, &entry.gl_id);
    }
    m_cache.clear();
}

} // namespace RC
