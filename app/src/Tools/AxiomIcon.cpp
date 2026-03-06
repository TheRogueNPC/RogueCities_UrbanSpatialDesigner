#include "RogueCity/App/Tools/AxiomIcon.hpp"
#include "RogueCity/App/Tools/AxiomTypeRegistry.hpp"

#include <vector>

namespace RogueCity::App {

namespace {
    // Fallback descriptor used when a type is not yet registered.
    static const AxiomTypeInfo kFallbackInfo{ AxiomType::Grid, "Grid", IM_COL32(220,220,220,255) };
}

std::span<const AxiomTypeInfo> GetAxiomTypeInfos() {
    // Build a transient vector from the registry each call.
    // Callers use this for iteration only; hot paths use GetAxiomTypeInfo() directly.
    static std::vector<AxiomTypeInfo> s_cache;
    s_cache.clear();
    for (const auto& d : AxiomTypeRegistry::Instance().All()) {
        s_cache.push_back({ d.type, d.name, d.primary_color });
    }
    return std::span<const AxiomTypeInfo>(s_cache.data(), s_cache.size());
}

const AxiomTypeInfo& GetAxiomTypeInfo(AxiomType type) {
    if (const auto* d = AxiomTypeRegistry::Instance().Get(type)) {
        // Return a stable reference backed by a static per-call storage.
        // Since descriptors_ is a std::vector with stable addresses after
        // all registrations are done, we can return a pointer into it.
        static AxiomTypeInfo s_info;
        s_info = { d->type, d->name, d->primary_color };
        return s_info;
    }
    return kFallbackInfo;
}

void DrawAxiomIcon(ImDrawList* draw_list, ImVec2 center, float radius, AxiomType type, ImU32 color) {
    if (!draw_list || radius <= 1.0f) return;
    if (const auto* d = AxiomTypeRegistry::Instance().Get(type)) {
        d->draw_icon(draw_list, center, radius, color);
    }
}

} // namespace RogueCity::App
