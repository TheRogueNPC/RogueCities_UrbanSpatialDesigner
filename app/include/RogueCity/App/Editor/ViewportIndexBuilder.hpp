/**
 * @brief Utility for building viewport indices within the editor.
 *
 * The ViewportIndexBuilder provides a static method to construct or update
 * viewport indices based on the current global editor state.
 */
 
#pragma once

namespace RogueCity::Core::Editor { struct GlobalState; }

namespace RogueCity::App {

struct ViewportIndexBuilder {
    static void Build(RogueCity::Core::Editor::GlobalState& gs);
};

} // namespace RogueCity::App
