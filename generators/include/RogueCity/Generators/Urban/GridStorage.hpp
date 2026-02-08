#pragma once

#include "RogueCity/Core/Types.hpp"

#include <unordered_map>
#include <vector>

namespace RogueCity::Generators::Urban {

    class GridStorage {
    public:
        explicit GridStorage(double cell_size = 25.0) : cell_size_(cell_size) {}

        void clear();
        void insert(const Core::Vec2& p);
        [[nodiscard]] std::vector<Core::Vec2> queryNear(const Core::Vec2& p, double radius) const;

    private:
        struct Key {
            int x{ 0 };
            int y{ 0 };
            bool operator==(const Key& other) const { return x == other.x && y == other.y; }
        };

        struct KeyHash {
            size_t operator()(const Key& key) const {
                return (static_cast<size_t>(key.x) << 32) ^ static_cast<size_t>(key.y + 0x9e3779b9);
            }
        };

        [[nodiscard]] Key toKey(const Core::Vec2& p) const;

        double cell_size_{ 25.0 };
        std::unordered_map<Key, std::vector<Core::Vec2>, KeyHash> buckets_;
    };

} // namespace RogueCity::Generators::Urban

