  
/**
 * @file CityTypes.cpp
 * @brief Implements data structures and types for representing cities, roads, and related entities in RogueCity.
 *
 * This file contains the implementation of the CityTypes.hpp header, providing member functions and utilities
 * for city-related data structures. The goal is to maintain clear, organized, and efficient code for handling
 * cities, roads, and other spatial entities within the RogueCity game.
 */

namespace RogueCity::Core {

    /**
     * @brief Calculates the total length of the road.
     *
     * Computes the sum of distances between consecutive points along the road.
     * Returns 0.0 if the road has fewer than two points.
     *
     * @return The total length of the road as a double.
     */
    double Road::length() const;

} // namespace RogueCity::Core
