#include "RogueCity/Core/Editor/GlobalState.hpp"

namespace RogueCity::Core::Editor {

    GlobalState& GetGlobalState()
    {
        static GlobalState gs{};
        return gs;
    }

} // namespace RogueCity::Core::Editor

