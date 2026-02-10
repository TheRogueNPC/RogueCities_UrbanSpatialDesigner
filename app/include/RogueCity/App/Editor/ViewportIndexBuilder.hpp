// FILE: ViewportIndexBuilder.hpp
// PURPOSE: Populate GlobalState viewport index after generation.
#pragma once

namespace RogueCity::Core::Editor { struct GlobalState; }

namespace RogueCity::App {

struct ViewportIndexBuilder {
    static void Build(RogueCity::Core::Editor::GlobalState& gs);
};

} // namespace RogueCity::App
