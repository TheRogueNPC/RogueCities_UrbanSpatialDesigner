#include "RogueCity/App/Tools/AxiomIcon.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace RogueCity::App {

namespace {
    constexpr std::array<AxiomTypeInfo, 10> kTypeInfos{ {
        { AxiomType::Organic,        "Organic",         IM_COL32( 60, 220, 120, 255) },
        { AxiomType::Grid,           "Grid",            IM_COL32(220, 220, 220, 255) },
        { AxiomType::Radial,         "Radial",          IM_COL32(255, 170,   0, 255) },
        { AxiomType::Hexagonal,      "Hexagonal",       IM_COL32(180, 100, 255, 255) },
        { AxiomType::Stem,           "Stem",            IM_COL32(150, 255,  80, 255) },
        { AxiomType::LooseGrid,      "Loose Grid",      IM_COL32( 80, 200, 255, 255) },
        { AxiomType::Suburban,       "Suburban",        IM_COL32(255,  80, 160, 255) },
        { AxiomType::Superblock,     "Superblock",      IM_COL32(255, 255,  80, 255) },
        { AxiomType::Linear,         "Linear",          IM_COL32( 80, 255, 255, 255) },
        { AxiomType::GridCorrective, "Grid Corrective", IM_COL32(255,  60,  60, 255) },
    } };

    void add_poly(ImDrawList* dl, ImVec2 c, const std::vector<ImVec2>& pts, ImU32 col, float thickness) {
        if (!dl || pts.size() < 2) return;
        dl->AddPolyline(pts.data(), static_cast<int>(pts.size()), col, ImDrawFlags_None, thickness);
    }
}

std::span<const AxiomTypeInfo> GetAxiomTypeInfos() {
    return std::span<const AxiomTypeInfo>(kTypeInfos.data(), kTypeInfos.size());
}

const AxiomTypeInfo& GetAxiomTypeInfo(AxiomType type) {
    for (const auto& info : kTypeInfos) {
        if (info.type == type) {
            return info;
        }
    }
    return kTypeInfos[1]; // Grid fallback
}

void DrawAxiomIcon(ImDrawList* draw_list, ImVec2 center, float radius, AxiomType type, ImU32 color) {
    if (!draw_list || radius <= 1.0f) return;

    const float r = radius;
    const float t = std::max(1.0f, r * 0.12f);

    switch (type) {
    case AxiomType::Organic: {
        std::vector<ImVec2> pts;
        pts.reserve(9);
        for (int i = 0; i <= 8; ++i) {
            const float x = (static_cast<float>(i) / 8.0f) * (2.0f * r) - r;
            const float y = std::sin((x / r) * std::numbers::pi_v<float> * 1.5f) * (0.35f * r);
            pts.emplace_back(center.x + x, center.y + y);
        }
        add_poly(draw_list, center, pts, color, t);
        break;
    }
    case AxiomType::Grid: {
        const float s = r * 0.65f;
        for (int i = -1; i <= 1; ++i) {
            const float o = i * (s / 2.0f);
            draw_list->AddLine(ImVec2(center.x - s, center.y + o), ImVec2(center.x + s, center.y + o), color, t);
            draw_list->AddLine(ImVec2(center.x + o, center.y - s), ImVec2(center.x + o, center.y + s), color, t);
        }
        break;
    }
    case AxiomType::Radial: {
        draw_list->AddCircle(center, r * 0.75f, color, 18, t);
        constexpr int kSpokes = 8;
        for (int i = 0; i < kSpokes; ++i) {
            const float a = (2.0f * std::numbers::pi_v<float>) * (static_cast<float>(i) / static_cast<float>(kSpokes));
            const ImVec2 d(std::cos(a), std::sin(a));
            draw_list->AddLine(
                center,
                ImVec2(center.x + d.x * r * 0.75f, center.y + d.y * r * 0.75f),
                color,
                t
            );
        }
        break;
    }
    case AxiomType::Hexagonal: {
        ImVec2 pts[6];
        for (int i = 0; i < 6; ++i) {
            const float a = std::numbers::pi_v<float> / 6.0f + (2.0f * std::numbers::pi_v<float>) * (static_cast<float>(i) / 6.0f);
            pts[i] = ImVec2(center.x + std::cos(a) * r * 0.8f, center.y + std::sin(a) * r * 0.8f);
        }
        draw_list->AddPolyline(pts, 6, color, ImDrawFlags_Closed, t);
        break;
    }
    case AxiomType::Stem: {
        const float trunk = r * 0.85f;
        draw_list->AddLine(ImVec2(center.x, center.y + trunk * 0.4f), ImVec2(center.x, center.y - trunk * 0.55f), color, t);
        draw_list->AddLine(ImVec2(center.x, center.y - trunk * 0.2f), ImVec2(center.x - trunk * 0.45f, center.y - trunk * 0.65f), color, t);
        draw_list->AddLine(ImVec2(center.x, center.y - trunk * 0.2f), ImVec2(center.x + trunk * 0.45f, center.y - trunk * 0.65f), color, t);
        break;
    }
    case AxiomType::LooseGrid: {
        const float s = r * 0.65f;
        for (int i = -1; i <= 1; ++i) {
            const float o = i * (s / 2.0f);
            const float j = (i == 0) ? 0.0f : (i * 0.18f * r);
            draw_list->AddLine(ImVec2(center.x - s, center.y + o + j), ImVec2(center.x + s, center.y + o - j), color, t);
            draw_list->AddLine(ImVec2(center.x + o + j, center.y - s), ImVec2(center.x + o - j, center.y + s), color, t);
        }
        break;
    }
    case AxiomType::Suburban: {
        draw_list->AddLine(ImVec2(center.x - r * 0.7f, center.y), ImVec2(center.x + r * 0.2f, center.y), color, t);
        draw_list->AddCircle(ImVec2(center.x + r * 0.45f, center.y), r * 0.35f, color, 16, t);
        break;
    }
    case AxiomType::Superblock: {
        const float s = r * 0.75f;
        draw_list->AddRect(ImVec2(center.x - s, center.y - s), ImVec2(center.x + s, center.y + s), color, 0.0f, 0, t);
        draw_list->AddLine(ImVec2(center.x - s, center.y), ImVec2(center.x + s, center.y), color, t);
        draw_list->AddLine(ImVec2(center.x, center.y - s), ImVec2(center.x, center.y + s), color, t);
        break;
    }
    case AxiomType::Linear: {
        const float s = r * 0.75f;
        for (int i = -1; i <= 1; ++i) {
            const float o = i * (r * 0.25f);
            draw_list->AddLine(ImVec2(center.x - s, center.y + o), ImVec2(center.x + s, center.y + o), color, t);
        }
        break;
    }
    case AxiomType::GridCorrective: {
        const float s = r * 0.8f;
        draw_list->AddLine(ImVec2(center.x - s, center.y), ImVec2(center.x + s, center.y), color, t);
        draw_list->AddLine(ImVec2(center.x, center.y - s), ImVec2(center.x, center.y + s), color, t);
        draw_list->AddRect(ImVec2(center.x - s * 0.35f, center.y - s * 0.35f), ImVec2(center.x + s * 0.35f, center.y + s * 0.35f), color, 0.0f, 0, t);
        break;
    }
    case AxiomType::COUNT:
        break;
    }
}

} // namespace RogueCity::App
