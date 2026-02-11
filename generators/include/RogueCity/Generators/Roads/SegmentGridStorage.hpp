#pragma once

#include "RogueCity/Core/Types.hpp"

#include <vector>

namespace RogueCity::Generators::Roads {

    struct SegmentRef {
        uint32_t edge_id = 0;
        Core::Vec2 a{};
        Core::Vec2 b{};
        int layer_id = 0;
    };

    class SegmentGridStorage {
    public:
        SegmentGridStorage(int w, int h, float cell_size);

        void clear();
        void insert(const SegmentRef& seg);

        void queryRadius(
            const Core::Vec2& p,
            float r,
            int layer_id,
            std::vector<SegmentRef>& out) const;

    private:
        int w_ = 1;
        int h_ = 1;
        float cell_size_ = 1.0f;

        std::vector<std::vector<SegmentRef>> cells_;

        [[nodiscard]] int cellX(float x) const;
        [[nodiscard]] int cellY(float y) const;
        [[nodiscard]] bool inBounds(int cx, int cy) const;
        [[nodiscard]] int idx(int cx, int cy) const;
    };

} // namespace RogueCity::Generators::Roads
