# Tool Wiring & UX Contract

**Philosophy**: The Editor is a Game. The City is the Level.
**Goal**: Hide the complexity of urban planning behind tactile, reactive controls.

## 1. The "Game-Feel" Mandate
Tools must not feel like database entry forms. They must feel like physical controls in a cockpit.
- **Tactile Response**: Every interaction must have a visual reaction.
    - *Hover*: Elements glow or brighten.
    - *Active*: Tools pulse or emit a "live" signal (e.g., Amber/Cyan heartbeat).
    - *Click*: Immediate visual feedback (depression or flash).
- **No Dead Interactions**: If a button is disabled, it must explain *why* via tooltip (e.g., "Requires Road Layer").

## 2. The "Safety Rail" Protocol
We prevent "Perlin Slop" by guiding the user, not just restricting them.
- **Ghosting**: Before committing an action (placing a road), show a "Ghost" of the result.
    - *Valid*: Ghost is Blue/Green.
    - *Invalid*: Ghost is Red/Orange with a floating error label (e.g., "Too Steep").
- **Snap & Align**: Tools should magnetically snap to valid grid points, existing nodes, or geometry centers to ensure structural integrity.

## 3. Contextual Relevance (Hiding the Binder)
Don't show the entire rulebook. Show only what matters *now*.
- **Mode Isolation**: When in "Water Mode", hide "Zoning" controls.
- **Smart Inspection**: The Inspector panel should only show properties relevant to the active selection.
- **Heads-Up Data**: Vital stats (Cost, Slope, Length) should float near the cursor, not buried in a side panel.

## 4. Implementation Pattern
All tool buttons must use `RogueCity::Visualizer::UI::RogueToolButton`.
- **Do NOT** use raw `ImGui::Button`.
- **Do NOT** implement custom styling in individual panels.