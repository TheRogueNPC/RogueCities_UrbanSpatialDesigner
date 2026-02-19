#include "RogueCity/Core/Data/CityTypes.hpp"
#include <cmath>
//this file contains the implementation of the CityTypes.hpp header, which defines the data structures and types used to represent cities, roads, and other related entities in the RogueCity game. The implementation includes any necessary member functions or methods that are declared in the header, as well as any additional helper functions or utilities that are needed to work with these types. The goal is to provide a clear and organized implementation of the city-related data structures while keeping the code maintainable and efficient.
namespace RogueCity::Core { 

    double Road::length() const { // Calculate the total length of the road by summing the distances between consecutive points
        if (points.size() < 2) return 0.0;

        double total = 0.0;
        for (size_t i = 1; i < points.size(); ++i) {
            total += points[i].distanceTo(points[i - 1]);
        }
        return total;
    }

} // namespace RogueCity::Core
