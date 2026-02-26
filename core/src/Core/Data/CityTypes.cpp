#include "RogueCity/Core/Data/CityTypes.hpp"

namespace RogueCity::Core {

double Road::length() const {
    if (points.size() < 2) {
        return 0.0;
    }

    double total = 0.0;
    for (size_t i = 1; i < points.size(); ++i) {
        total += distance(points[i - 1], points[i]);
    }
    return total;
}

} // namespace RogueCity::Core
