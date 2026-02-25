#pragma once

#include "CityModel.h"

// District influencer archetypes that bias nearby district types
enum class InfluencerType
{
    None = 0, // No special influence (pure tensor field axiom)
    Market,   // Biases toward Commercial
    Keep,     // Biases toward Civic
    Temple,   // Biases toward Civic/Mixed
    Harbor,   // Biases toward Industrial/Commercial
    Park,     // Biases toward Residential/Luxury
    Gate,     // Biases toward Commercial/Mixed
    Well      // Biases toward Residential
};

struct AxiomInput
{
    int id;
    int type;                                        // matches viewport axiom type indices (Radial, Delta, Block, Grid)
    CityModel::Vec2 pos;                             // world coords in generator space
    double radius;                                   // influence radius
    InfluencerType influencer{InfluencerType::None}; // district type influencer (landmark seeding)
};
