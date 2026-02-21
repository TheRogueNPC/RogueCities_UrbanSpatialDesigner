#include "RogueCity/Generators/Urban/GridStorage.hpp"

#include <cmath>

namespace RogueCity::Generators::Urban {

    // Clears all spatial buckets.
    void GridStorage::clear() {
        buckets_.clear();
    }

    // Inserts a point into its discretized cell bucket.
    void GridStorage::insert(const Core::Vec2& p) {
        buckets_[toKey(p)].push_back(p);
    }

    // Radius query over neighboring buckets with exact distance filtering.
    std::vector<Core::Vec2> GridStorage::queryNear(const Core::Vec2& p, double radius) const {
        std::vector<Core::Vec2> out;
        const int r = std::max(1, static_cast<int>(std::ceil(radius / cell_size_)));
        const Key center = toKey(p);
        const double r2 = radius * radius;

        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                const Key key{ center.x + dx, center.y + dy };
                auto it = buckets_.find(key);
                if (it == buckets_.end()) {
                    continue;
                }
                for (const auto& candidate : it->second) {
                    if (candidate.distanceToSquared(p) <= r2) {
                        out.push_back(candidate);
                    }
                }
            }
        }

        return out;
    }

    // Maps world position to integer grid key at configured cell size.
    GridStorage::Key GridStorage::toKey(const Core::Vec2& p) const {
        return Key{
            static_cast<int>(std::floor(p.x / cell_size_)),
            static_cast<int>(std::floor(p.y / cell_size_))
        };
    }

} // namespace RogueCity::Generators::Urban
