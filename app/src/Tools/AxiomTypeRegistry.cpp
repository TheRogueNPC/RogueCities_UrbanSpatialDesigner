// FILE: app/src/Tools/AxiomTypeRegistry.cpp
// PURPOSE: Implements AxiomTypeRegistry and registers all 10 built-in axiom types.
//
// To add a new axiom type:
//   1. Add a static DrawIcon_NewType() function below (copy an existing one as template)
//   2. Add one Register({...}) call in RegisterBuiltInAxiomTypes()
//   3. Update the 3 generator/visualizer files listed in AxiomTypeRegistry.hpp

#include "RogueCity/App/Tools/AxiomTypeRegistry.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace RogueCity::App {

namespace {

// Shared polyline helper (mirrors the one that was in AxiomIcon.cpp)
void add_poly(ImDrawList* dl, const std::vector<ImVec2>& pts, ImU32 col, float thickness) {
    if (!dl || pts.size() < 2) return;
    dl->AddPolyline(pts.data(), static_cast<int>(pts.size()), col, ImDrawFlags_None, thickness);
}

// ── Per-type icon draw functions ───────────────────────────────────────────────
// Each function receives the draw list, center point, radius, and fill color.
// Geometry is extracted verbatim from the original DrawAxiomIcon() switch statement.

static void DrawIcon_Organic(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const float t = std::max(1.0f, r * 0.12f);
    std::vector<ImVec2> pts;
    pts.reserve(9);
    for (int i = 0; i <= 8; ++i) {
        const float x = (static_cast<float>(i) / 8.0f) * (2.0f * r) - r;
        const float y = std::sin((x / r) * std::numbers::pi_v<float> * 1.5f) * (0.35f * r);
        pts.emplace_back(c.x + x, c.y + y);
    }
    add_poly(dl, pts, col, t);
}

static void DrawIcon_Grid(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const float t = std::max(1.0f, r * 0.12f);
    const float s = r * 0.65f;
    for (int i = -1; i <= 1; ++i) {
        const float o = i * (s / 2.0f);
        dl->AddLine(ImVec2(c.x - s, c.y + o), ImVec2(c.x + s, c.y + o), col, t);
        dl->AddLine(ImVec2(c.x + o, c.y - s), ImVec2(c.x + o, c.y + s), col, t);
    }
}

static void DrawIcon_Radial(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const float t = std::max(1.0f, r * 0.12f);
    dl->AddCircle(c, r * 0.75f, col, 18, t);
    constexpr int kSpokes = 8;
    for (int i = 0; i < kSpokes; ++i) {
        const float a = (2.0f * std::numbers::pi_v<float>) * (static_cast<float>(i) / static_cast<float>(kSpokes));
        dl->AddLine(c, ImVec2(c.x + std::cos(a) * r * 0.75f, c.y + std::sin(a) * r * 0.75f), col, t);
    }
}

static void DrawIcon_Hexagonal(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const float t = std::max(1.0f, r * 0.12f);
    ImVec2 pts[6];
    for (int i = 0; i < 6; ++i) {
        const float a = std::numbers::pi_v<float> / 6.0f + (2.0f * std::numbers::pi_v<float>) * (static_cast<float>(i) / 6.0f);
        pts[i] = ImVec2(c.x + std::cos(a) * r * 0.8f, c.y + std::sin(a) * r * 0.8f);
    }
    dl->AddPolyline(pts, 6, col, ImDrawFlags_Closed, t);
}

static void DrawIcon_Stem(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const float t = std::max(1.0f, r * 0.12f);
    const float trunk = r * 0.85f;
    dl->AddLine(ImVec2(c.x, c.y + trunk * 0.4f), ImVec2(c.x, c.y - trunk * 0.55f), col, t);
    dl->AddLine(ImVec2(c.x, c.y - trunk * 0.2f), ImVec2(c.x - trunk * 0.45f, c.y - trunk * 0.65f), col, t);
    dl->AddLine(ImVec2(c.x, c.y - trunk * 0.2f), ImVec2(c.x + trunk * 0.45f, c.y - trunk * 0.65f), col, t);
}

static void DrawIcon_LooseGrid(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const float t = std::max(1.0f, r * 0.12f);
    const float s = r * 0.65f;
    for (int i = -1; i <= 1; ++i) {
        const float o = i * (s / 2.0f);
        const float j = (i == 0) ? 0.0f : (i * 0.18f * r);
        dl->AddLine(ImVec2(c.x - s, c.y + o + j), ImVec2(c.x + s, c.y + o - j), col, t);
        dl->AddLine(ImVec2(c.x + o + j, c.y - s), ImVec2(c.x + o - j, c.y + s), col, t);
    }
}

static void DrawIcon_Suburban(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const float t = std::max(1.0f, r * 0.12f);
    dl->AddLine(ImVec2(c.x - r * 0.7f, c.y), ImVec2(c.x + r * 0.2f, c.y), col, t);
    dl->AddCircle(ImVec2(c.x + r * 0.45f, c.y), r * 0.35f, col, 16, t);
}

static void DrawIcon_Superblock(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const float t = std::max(1.0f, r * 0.12f);
    const float s = r * 0.75f;
    dl->AddRect(ImVec2(c.x - s, c.y - s), ImVec2(c.x + s, c.y + s), col, 0.0f, 0, t);
    dl->AddLine(ImVec2(c.x - s, c.y), ImVec2(c.x + s, c.y), col, t);
    dl->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x, c.y + s), col, t);
}

static void DrawIcon_Linear(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const float t = std::max(1.0f, r * 0.12f);
    const float s = r * 0.75f;
    for (int i = -1; i <= 1; ++i) {
        const float o = i * (r * 0.25f);
        dl->AddLine(ImVec2(c.x - s, c.y + o), ImVec2(c.x + s, c.y + o), col, t);
    }
}

static void DrawIcon_GridCorrective(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const float t = std::max(1.0f, r * 0.12f);
    const float s = r * 0.8f;
    dl->AddLine(ImVec2(c.x - s, c.y), ImVec2(c.x + s, c.y), col, t);
    dl->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x, c.y + s), col, t);
    dl->AddRect(ImVec2(c.x - s * 0.35f, c.y - s * 0.35f), ImVec2(c.x + s * 0.35f, c.y + s * 0.35f), col, 0.0f, 0, t);
}

} // anonymous namespace

// ── AxiomTypeRegistry implementation ──────────────────────────────────────────

AxiomTypeRegistry& AxiomTypeRegistry::Instance() {
    static AxiomTypeRegistry s_instance;
    return s_instance;
}

void AxiomTypeRegistry::Register(AxiomTypeDescriptor desc) {
    // Replace if same type already registered (idempotent re-registration support)
    for (auto& d : descriptors_) {
        if (d.type == desc.type) {
            d = desc;
            return;
        }
    }
    descriptors_.push_back(desc);
}

const AxiomTypeDescriptor* AxiomTypeRegistry::Get(AxiomType type) const {
    for (const auto& d : descriptors_) {
        if (d.type == type) return &d;
    }
    return nullptr;
}

std::span<const AxiomTypeDescriptor> AxiomTypeRegistry::All() const {
    return std::span<const AxiomTypeDescriptor>(descriptors_.data(), descriptors_.size());
}

// ── Built-in registration ──────────────────────────────────────────────────────

void RegisterBuiltInAxiomTypes() {
    static bool s_done = false;
    if (s_done) return;
    s_done = true;

    auto& reg = AxiomTypeRegistry::Instance();

    reg.Register({
        AxiomType::Organic,
        "Organic",
        IM_COL32(60, 220, 120, 255),
        "Terminal: soften flow or blend hierarchy in nearby cells.",
        { AxiomType::LooseGrid, AxiomType::Suburban, AxiomType::Stem },
        DrawIcon_Organic
    });

    reg.Register({
        AxiomType::Grid,
        "Grid",
        IM_COL32(220, 220, 220, 255),
        "Terminal: lock alignment, then escalate density only if needed.",
        { AxiomType::Superblock, AxiomType::Hexagonal, AxiomType::GridCorrective },
        DrawIcon_Grid
    });

    reg.Register({
        AxiomType::Radial,
        "Radial",
        IM_COL32(255, 170, 0, 255),
        "Terminal: radial hubs pair best with corrective or suburban buffers.",
        { AxiomType::GridCorrective, AxiomType::Suburban, AxiomType::Organic },
        DrawIcon_Radial
    });

    reg.Register({
        AxiomType::Hexagonal,
        "Hexagonal",
        IM_COL32(180, 100, 255, 255),
        "Terminal: preserve angular continuity while tuning block regularity.",
        { AxiomType::Grid, AxiomType::Superblock, AxiomType::Radial },
        DrawIcon_Hexagonal
    });

    reg.Register({
        AxiomType::Stem,
        "Stem",
        IM_COL32(150, 255, 80, 255),
        "Terminal: maintain trunk direction before widening network spread.",
        { AxiomType::Linear, AxiomType::Organic, AxiomType::Suburban },
        DrawIcon_Stem
    });

    reg.Register({
        AxiomType::LooseGrid,
        "Loose Grid",
        IM_COL32(80, 200, 255, 255),
        "Terminal: tighten only the zones that need navigation regularity.",
        { AxiomType::Organic, AxiomType::Grid, AxiomType::Suburban },
        DrawIcon_LooseGrid
    });

    reg.Register({
        AxiomType::Suburban,
        "Suburban",
        IM_COL32(255, 80, 160, 255),
        "Terminal: transition between loop neighborhoods and arterial anchors.",
        { AxiomType::Radial, AxiomType::Organic, AxiomType::LooseGrid },
        DrawIcon_Suburban
    });

    reg.Register({
        AxiomType::Superblock,
        "Superblock",
        IM_COL32(255, 255, 80, 255),
        "Terminal: shift between macro parcels and local permeability layers.",
        { AxiomType::Grid, AxiomType::Hexagonal, AxiomType::Linear },
        DrawIcon_Superblock
    });

    reg.Register({
        AxiomType::Linear,
        "Linear",
        IM_COL32(80, 255, 255, 255),
        "Terminal: tune corridor spine, then branch into local fabric.",
        { AxiomType::Stem, AxiomType::GridCorrective, AxiomType::Grid },
        DrawIcon_Linear
    });

    reg.Register({
        AxiomType::GridCorrective,
        "Grid Corrective",
        IM_COL32(255, 60, 60, 255),
        "Terminal: corrective overlays should resolve conflict, not dominate.",
        { AxiomType::Grid, AxiomType::Radial, AxiomType::Superblock },
        DrawIcon_GridCorrective
    });
}

} // namespace RogueCity::App
