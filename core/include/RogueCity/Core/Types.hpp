#pragma once

/**
 * @file Types.hpp
 * @brief Central import point for all RogueCity core types
 *
 * This header provides a single-include convenience for all core data structures
 * and mathematical utilities used in procedural city generation.
 */

#include "RogueCity/Core/Math/Vec2.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Data/TensorTypes.hpp"
#include "RogueCity/Core/Util/IndexVector.hpp"
#include "RogueCity/Core/Util/StableIndexVector.hpp"
#include "RogueCity/Core/Util/RogueWorker.hpp"

namespace RogueCity::Core {
    // All types are already in the RogueCity::Core namespace
    // This file just provides a convenient single-include entry point
    
    // ===== UTILITY CONTAINER ALIASES =====
    // Convenient aliases for common patterns
    
    /// Constant Index Vector - O(1) access, insertion, deletion with stable IDs
    template<typename T>
    using CIV = civ::IndexVector<T>;
    
    /// CIV Reference type for standalone access
    template<typename T>
    using CIVRef = civ::Ref<T>;
    
    /// Stable Index Vector - O(1) operations with validity checking
    template<typename T>
    using SIV = siv::Vector<T>;
    
    /// SIV Handle type for standalone access with validity
    template<typename T>
    using SIVHandle = siv::Handle<T>;
    
    /// Multithreading worker pool
    using RogueWorker = Rowk::RogueWorker;
    using WorkGroup = Rowk::ExecutionGroup;
    using WorkerFunc = Rowk::WorkerFunction;
}

