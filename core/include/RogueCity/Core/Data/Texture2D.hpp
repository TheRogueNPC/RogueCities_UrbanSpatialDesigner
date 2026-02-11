#pragma once

#include "RogueCity/Core/Math/Vec2.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace RogueCity::Core::Data {

    template <typename T>
    class Texture2D {
    public:
        Texture2D() = default;
        Texture2D(int width, int height, const T& initial_value = T{}) {
            resize(width, height, initial_value);
        }

        void resize(int width, int height, const T& initial_value = T{}) {
            width_ = std::max(0, width);
            height_ = std::max(0, height);
            data_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_), initial_value);
        }

        [[nodiscard]] int width() const { return width_; }
        [[nodiscard]] int height() const { return height_; }
        [[nodiscard]] bool empty() const { return data_.empty(); }
        [[nodiscard]] size_t size() const { return data_.size(); }
        [[nodiscard]] size_t sizeInBytes() const { return data_.size() * sizeof(T); }

        [[nodiscard]] const std::vector<T>& data() const { return data_; }
        [[nodiscard]] std::vector<T>& data() { return data_; }

        [[nodiscard]] const T& at(int x, int y) const {
            assert(x >= 0 && x < width_ && y >= 0 && y < height_);
            return data_[indexOf(x, y)];
        }

        [[nodiscard]] T& at(int x, int y) {
            assert(x >= 0 && x < width_ && y >= 0 && y < height_);
            return data_[indexOf(x, y)];
        }

        void fill(const T& value) {
            std::fill(data_.begin(), data_.end(), value);
        }

        [[nodiscard]] T sampleNearest(const Vec2& uv) const {
            if (empty()) {
                return T{};
            }

            const double u = std::clamp(uv.x, 0.0, 1.0);
            const double v = std::clamp(uv.y, 0.0, 1.0);
            const int px = std::clamp(
                static_cast<int>(std::round(u * static_cast<double>(width_ - 1))),
                0,
                width_ - 1);
            const int py = std::clamp(
                static_cast<int>(std::round(v * static_cast<double>(height_ - 1))),
                0,
                height_ - 1);
            return at(px, py);
        }

        template <typename U = T>
        [[nodiscard]] std::enable_if_t<std::is_same_v<U, float>, float> sampleBilinear(const Vec2& uv) const {
            if (empty()) {
                return 0.0f;
            }

            const double u = std::clamp(uv.x, 0.0, 1.0);
            const double v = std::clamp(uv.y, 0.0, 1.0);
            const double x = u * static_cast<double>(width_ - 1);
            const double y = v * static_cast<double>(height_ - 1);

            const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, width_ - 1);
            const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, height_ - 1);
            const int x1 = std::clamp(x0 + 1, 0, width_ - 1);
            const int y1 = std::clamp(y0 + 1, 0, height_ - 1);

            const double tx = x - static_cast<double>(x0);
            const double ty = y - static_cast<double>(y0);

            const double c00 = static_cast<double>(at(x0, y0));
            const double c10 = static_cast<double>(at(x1, y0));
            const double c01 = static_cast<double>(at(x0, y1));
            const double c11 = static_cast<double>(at(x1, y1));

            const double a = c00 * (1.0 - tx) + c10 * tx;
            const double b = c01 * (1.0 - tx) + c11 * tx;
            return static_cast<float>(a * (1.0 - ty) + b * ty);
        }

        template <typename U = T>
        [[nodiscard]] std::enable_if_t<std::is_same_v<U, uint8_t>, float> sampleBilinearU8(const Vec2& uv) const {
            if (empty()) {
                return 0.0f;
            }

            const double u = std::clamp(uv.x, 0.0, 1.0);
            const double v = std::clamp(uv.y, 0.0, 1.0);
            const double x = u * static_cast<double>(width_ - 1);
            const double y = v * static_cast<double>(height_ - 1);

            const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, width_ - 1);
            const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, height_ - 1);
            const int x1 = std::clamp(x0 + 1, 0, width_ - 1);
            const int y1 = std::clamp(y0 + 1, 0, height_ - 1);

            const double tx = x - static_cast<double>(x0);
            const double ty = y - static_cast<double>(y0);

            const double c00 = static_cast<double>(at(x0, y0));
            const double c10 = static_cast<double>(at(x1, y0));
            const double c01 = static_cast<double>(at(x0, y1));
            const double c11 = static_cast<double>(at(x1, y1));

            const double a = c00 * (1.0 - tx) + c10 * tx;
            const double b = c01 * (1.0 - tx) + c11 * tx;
            return static_cast<float>(a * (1.0 - ty) + b * ty);
        }

        template <typename U = T>
        [[nodiscard]] std::enable_if_t<std::is_same_v<U, Vec2>, Vec2> sampleBilinearVec2(const Vec2& uv) const {
            if (empty()) {
                return Vec2{};
            }

            const double u = std::clamp(uv.x, 0.0, 1.0);
            const double v = std::clamp(uv.y, 0.0, 1.0);
            const double x = u * static_cast<double>(width_ - 1);
            const double y = v * static_cast<double>(height_ - 1);

            const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, width_ - 1);
            const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, height_ - 1);
            const int x1 = std::clamp(x0 + 1, 0, width_ - 1);
            const int y1 = std::clamp(y0 + 1, 0, height_ - 1);

            const double tx = x - static_cast<double>(x0);
            const double ty = y - static_cast<double>(y0);

            const Vec2 c00 = at(x0, y0);
            const Vec2 c10 = at(x1, y0);
            const Vec2 c01 = at(x0, y1);
            const Vec2 c11 = at(x1, y1);

            const Vec2 a = c00 * (1.0 - tx) + c10 * tx;
            const Vec2 b = c01 * (1.0 - tx) + c11 * tx;
            return a * (1.0 - ty) + b * ty;
        }

    private:
        [[nodiscard]] size_t indexOf(int x, int y) const {
            return static_cast<size_t>(y) * static_cast<size_t>(width_) + static_cast<size_t>(x);
        }

        int width_{ 0 };
        int height_{ 0 };
        std::vector<T> data_{};
    };

} // namespace RogueCity::Core::Data
