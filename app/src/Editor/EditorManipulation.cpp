#include "RogueCity/App/Editor/EditorManipulation.hpp"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <unordered_set>

namespace RogueCity::App::EditorManipulation {

namespace {

using RogueCity::Core::Vec2;
using RogueCity::Core::Editor::SelectionItem;
using RogueCity::Core::Editor::VpEntityKind;

Vec2 RotatePoint(const Vec2& point, const Vec2& pivot, double radians) {
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    const Vec2 d = point - pivot;
    return Vec2(
        pivot.x + d.x * c - d.y * s,
        pivot.y + d.x * s + d.y * c);
}

Vec2 ScalePoint(const Vec2& point, const Vec2& pivot, double factor) {
    return Vec2(
        pivot.x + (point.x - pivot.x) * factor,
        pivot.y + (point.y - pivot.y) * factor);
}

template <typename TContainer, typename TFn>
void ForEachById(TContainer& container, uint32_t id, TFn&& fn) {
    for (auto& value : container) {
        if (value.id == id) {
            fn(value);
            return;
        }
    }
}

uint64_t SelectionKey(const SelectionItem& item) {
    return (static_cast<uint64_t>(static_cast<uint8_t>(item.kind)) << 32ull) | static_cast<uint64_t>(item.id);
}

} // namespace

std::vector<Vec2> BuildCatmullRomSpline(
    const std::vector<Vec2>& control_points,
    const SplineOptions& options) {
    std::vector<Vec2> out;
    if (control_points.size() < 2) {
        return control_points;
    }

    const int samples = std::clamp(options.samples_per_segment, 2, 64);
    const bool closed = options.closed;

    auto point_at = [&](int idx) -> const Vec2& {
        const int count = static_cast<int>(control_points.size());
        if (closed) {
            int wrapped = idx % count;
            if (wrapped < 0) {
                wrapped += count;
            }
            return control_points[static_cast<size_t>(wrapped)];
        }
        const int clamped = std::clamp(idx, 0, count - 1);
        return control_points[static_cast<size_t>(clamped)];
    };

    const int segment_count = closed
        ? static_cast<int>(control_points.size())
        : static_cast<int>(control_points.size()) - 1;
    out.reserve(static_cast<size_t>(segment_count * samples) + 1);

    for (int seg = 0; seg < segment_count; ++seg) {
        const Vec2& p0 = point_at(seg - 1);
        const Vec2& p1 = point_at(seg);
        const Vec2& p2 = point_at(seg + 1);
        const Vec2& p3 = point_at(seg + 2);
        for (int i = 0; i < samples; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(samples);
            const double t2 = t * t;
            const double t3 = t2 * t;
            const double tension = std::clamp(static_cast<double>(options.tension), 0.0, 1.0);
            const double alpha = 0.5 * (1.0 - 0.5 * tension);
            const Vec2 value =
                (p1 * 2.0 +
                 (p2 - p0) * t +
                 (p0 * 2.0 - p1 * 5.0 + p2 * 4.0 - p3) * t2 +
                 (p3 - p0 + (p1 - p2) * 3.0) * t3) * alpha;
            out.push_back(value);
        }
    }

    if (!closed) {
        out.push_back(control_points.back());
    }

    return out;
}

bool ApplyTranslate(
    Core::Editor::GlobalState& gs,
    std::span<const SelectionItem> selection,
    const Vec2& delta) {
    if (selection.empty()) {
        return false;
    }

    bool changed = false;
    std::unordered_set<uint64_t> seen;
    seen.reserve(selection.size());

    for (const auto& item : selection) {
        if (!seen.insert(SelectionKey(item)).second) {
            continue;
        }
        switch (item.kind) {
        case VpEntityKind::Road:
            ForEachById(gs.roads, item.id, [&](RogueCity::Core::Road& road) {
                for (auto& p : road.points) {
                    p += delta;
                }
                changed = !road.points.empty();
            });
            break;
        case VpEntityKind::District:
            ForEachById(gs.districts, item.id, [&](RogueCity::Core::District& district) {
                for (auto& p : district.border) {
                    p += delta;
                }
                changed = !district.border.empty();
            });
            break;
        case VpEntityKind::Lot:
            ForEachById(gs.lots, item.id, [&](RogueCity::Core::LotToken& lot) {
                lot.centroid += delta;
                for (auto& p : lot.boundary) {
                    p += delta;
                }
                changed = true;
            });
            break;
        case VpEntityKind::Building:
            for (auto& building : gs.buildings) {
                if (building.id == item.id) {
                    building.position += delta;
                    changed = true;
                    break;
                }
            }
            break;
        case VpEntityKind::Water:
            ForEachById(gs.waterbodies, item.id, [&](RogueCity::Core::WaterBody& water) {
                for (auto& p : water.boundary) {
                    p += delta;
                }
                changed = !water.boundary.empty();
            });
            break;
        default:
            break;
        }
    }

    return changed;
}

bool ApplyRotate(
    Core::Editor::GlobalState& gs,
    std::span<const SelectionItem> selection,
    const Vec2& pivot,
    double radians) {
    if (selection.empty()) {
        return false;
    }

    bool changed = false;
    std::unordered_set<uint64_t> seen;
    seen.reserve(selection.size());

    for (const auto& item : selection) {
        if (!seen.insert(SelectionKey(item)).second) {
            continue;
        }
        switch (item.kind) {
        case VpEntityKind::Road:
            ForEachById(gs.roads, item.id, [&](RogueCity::Core::Road& road) {
                for (auto& p : road.points) {
                    p = RotatePoint(p, pivot, radians);
                }
                changed = !road.points.empty();
            });
            break;
        case VpEntityKind::District:
            ForEachById(gs.districts, item.id, [&](RogueCity::Core::District& district) {
                for (auto& p : district.border) {
                    p = RotatePoint(p, pivot, radians);
                }
                changed = !district.border.empty();
            });
            break;
        case VpEntityKind::Lot:
            ForEachById(gs.lots, item.id, [&](RogueCity::Core::LotToken& lot) {
                lot.centroid = RotatePoint(lot.centroid, pivot, radians);
                for (auto& p : lot.boundary) {
                    p = RotatePoint(p, pivot, radians);
                }
                changed = true;
            });
            break;
        case VpEntityKind::Building:
            for (auto& building : gs.buildings) {
                if (building.id == item.id) {
                    building.position = RotatePoint(building.position, pivot, radians);
                    building.rotation_radians += static_cast<float>(radians);
                    changed = true;
                    break;
                }
            }
            break;
        case VpEntityKind::Water:
            ForEachById(gs.waterbodies, item.id, [&](RogueCity::Core::WaterBody& water) {
                for (auto& p : water.boundary) {
                    p = RotatePoint(p, pivot, radians);
                }
                changed = !water.boundary.empty();
            });
            break;
        default:
            break;
        }
    }

    return changed;
}

bool ApplyScale(
    Core::Editor::GlobalState& gs,
    std::span<const SelectionItem> selection,
    const Vec2& pivot,
    double factor) {
    if (selection.empty()) {
        return false;
    }

    const double clamped_factor = std::clamp(factor, 0.01, 100.0);
    bool changed = false;
    std::unordered_set<uint64_t> seen;
    seen.reserve(selection.size());

    for (const auto& item : selection) {
        if (!seen.insert(SelectionKey(item)).second) {
            continue;
        }
        switch (item.kind) {
        case VpEntityKind::Road:
            ForEachById(gs.roads, item.id, [&](RogueCity::Core::Road& road) {
                for (auto& p : road.points) {
                    p = ScalePoint(p, pivot, clamped_factor);
                }
                changed = !road.points.empty();
            });
            break;
        case VpEntityKind::District:
            ForEachById(gs.districts, item.id, [&](RogueCity::Core::District& district) {
                for (auto& p : district.border) {
                    p = ScalePoint(p, pivot, clamped_factor);
                }
                changed = !district.border.empty();
            });
            break;
        case VpEntityKind::Lot:
            ForEachById(gs.lots, item.id, [&](RogueCity::Core::LotToken& lot) {
                lot.centroid = ScalePoint(lot.centroid, pivot, clamped_factor);
                for (auto& p : lot.boundary) {
                    p = ScalePoint(p, pivot, clamped_factor);
                }
                lot.area = static_cast<float>(lot.area * (clamped_factor * clamped_factor));
                changed = true;
            });
            break;
        case VpEntityKind::Building:
            for (auto& building : gs.buildings) {
                if (building.id == item.id) {
                    building.position = ScalePoint(building.position, pivot, clamped_factor);
                    building.uniform_scale = static_cast<float>(building.uniform_scale * clamped_factor);
                    changed = true;
                    break;
                }
            }
            break;
        case VpEntityKind::Water:
            ForEachById(gs.waterbodies, item.id, [&](RogueCity::Core::WaterBody& water) {
                for (auto& p : water.boundary) {
                    p = ScalePoint(p, pivot, clamped_factor);
                }
                changed = !water.boundary.empty();
            });
            break;
        default:
            break;
        }
    }

    return changed;
}

bool MoveRoadVertex(RogueCity::Core::Road& road, size_t vertex_index, const Vec2& new_position) {
    if (vertex_index >= road.points.size()) {
        return false;
    }
    road.points[vertex_index] = new_position;
    return true;
}

bool InsertDistrictVertex(RogueCity::Core::District& district, size_t edge_index, const Vec2& new_position) {
    if (district.border.empty()) {
        district.border.push_back(new_position);
        return true;
    }
    const size_t insert_at = std::min(edge_index + 1, district.border.size());
    district.border.insert(district.border.begin() + static_cast<std::ptrdiff_t>(insert_at), new_position);
    return true;
}

bool RemoveDistrictVertex(RogueCity::Core::District& district, size_t vertex_index) {
    if (district.border.size() <= 3 || vertex_index >= district.border.size()) {
        return false;
    }
    district.border.erase(district.border.begin() + static_cast<std::ptrdiff_t>(vertex_index));
    return true;
}

} // namespace RogueCity::App::EditorManipulation
