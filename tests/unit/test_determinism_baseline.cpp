#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Validation/DeterminismHash.hpp"

#include <cassert>
#include <string>

#ifndef ROGUECITY_SOURCE_DIR
#define ROGUECITY_SOURCE_DIR "."
#endif

int main() {
    using RogueCity::Core::Editor::GlobalState;
    using RogueCity::Core::Validation::ComputeDeterminismHash;
    using RogueCity::Core::Validation::ValidateAgainstBaseline;

    GlobalState gs{};
    const auto hash_a = ComputeDeterminismHash(gs);
    const auto hash_b = ComputeDeterminismHash(gs);
    assert(hash_a == hash_b);

    const std::string baseline_path =
        std::string(ROGUECITY_SOURCE_DIR) + "/tests/baselines/determinism_v0.10.txt";
    assert(ValidateAgainstBaseline(hash_a, baseline_path));

    return 0;
}
