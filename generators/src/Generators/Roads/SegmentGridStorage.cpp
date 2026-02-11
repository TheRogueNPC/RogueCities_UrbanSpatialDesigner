#include "RogueCity/Generators/Roads/SegmentGridStorage.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace RogueCity::Generators::Roads {

    namespace {

        [[nodiscard]] double pointSegmentDistanceSquared(
            const Core::Vec2& p,
            const Core::Vec2& a,
            const Core::Vec2& b) {
            const Core::Vec2 ab = b - a;
            const double ab_len2 = ab.lengthSquared();
            if (ab_len2 <= 1e-12) {
                return p.distanceToSquared(a);
            }
            const double t = std::clamp((p - a).dot(ab) / ab_len2, 0.0, 1.0);
            const Core::Vec2 q = a + (ab * t);
            return p.distanceToSquared(q);
        }

    } // namespace

    SegmentGridStorage::SegmentGridStorage(int w, int h, float cell_size)
        : w_(std::max(1, w))
        , h_(std::max(1, h))
        , cell_size_(std::max(1.0f, cell_size))
        , cells_(static_cast<size_t>(w_ * h_)) {
    }

    void SegmentGridStorage::clear() {
        for (auto& cell : cells_) {
            cell.clear();
        }
    }

    void SegmentGridStorage::insert(const SegmentRef& seg) {
        const float min_x = static_cast<float>(std::min(seg.a.x, seg.b.x));
        const float min_y = static_cast<float>(std::min(seg.a.y, seg.b.y));
        const float max_x = static_cast<float>(std::max(seg.a.x, seg.b.x));
        const float max_y = static_cast<float>(std::max(seg.a.y, seg.b.y));

        const int cx0 = std::clamp(cellX(min_x), 0, w_ - 1);
        const int cy0 = std::clamp(cellY(min_y), 0, h_ - 1);
        const int cx1 = std::clamp(cellX(max_x), 0, w_ - 1);
        const int cy1 = std::clamp(cellY(max_y), 0, h_ - 1);

        for (int cy = cy0; cy <= cy1; ++cy) {
            for (int cx = cx0; cx <= cx1; ++cx) {
                cells_[static_cast<size_t>(idx(cx, cy))].push_back(seg);
            }
        }
    }

    void SegmentGridStorage::queryRadius(
        const Core::Vec2& p,
        float r,
        int layer_id,
        std::vector<SegmentRef>& out) const {
        out.clear();
        const float clamped_r = std::max(0.0f, r);
        const float min_x = static_cast<float>(p.x) - clamped_r;
        const float min_y = static_cast<float>(p.y) - clamped_r;
        const float max_x = static_cast<float>(p.x) + clamped_r;
        const float max_y = static_cast<float>(p.y) + clamped_r;

        const int cx0 = std::clamp(cellX(min_x), 0, w_ - 1);
        const int cy0 = std::clamp(cellY(min_y), 0, h_ - 1);
        const int cx1 = std::clamp(cellX(max_x), 0, w_ - 1);
        const int cy1 = std::clamp(cellY(max_y), 0, h_ - 1);

        std::unordered_set<uint32_t> seen;
        const double r2 = static_cast<double>(clamped_r) * static_cast<double>(clamped_r);
        for (int cy = cy0; cy <= cy1; ++cy) {
            for (int cx = cx0; cx <= cx1; ++cx) {
                const auto& cell = cells_[static_cast<size_t>(idx(cx, cy))];
                for (const auto& seg : cell) {
                    if (seg.layer_id != layer_id) {
                        continue;
                    }
                    if (!seen.insert(seg.edge_id).second) {
                        continue;
                    }
                    if (pointSegmentDistanceSquared(p, seg.a, seg.b) <= r2) {
                        out.push_back(seg);
                    }
                }
            }
        }
    }

    int SegmentGridStorage::cellX(float x) const {
        return static_cast<int>(std::floor(x / cell_size_));
    }

    int SegmentGridStorage::cellY(float y) const {
        return static_cast<int>(std::floor(y / cell_size_));
    }

    bool SegmentGridStorage::inBounds(int cx, int cy) const {
        return cx >= 0 && cx < w_ && cy >= 0 && cy < h_;
    }

    int SegmentGridStorage::idx(int cx, int cy) const {
        if (!inBounds(cx, cy)) {
            return 0;
        }
        return cy * w_ + cx;
    }

} // namespace RogueCity::Generators::Roads
