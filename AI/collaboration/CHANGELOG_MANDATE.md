# Collaboration Changelog Mandate

This mandate applies to all agents operating in `AI/collaboration/` (Codex, Gemini, Claude, Copilot, and local agents).

## Rule
For every tangible code, configuration, workflow, or documentation change, the responsible agent must append a concise entry to `CHANGELOG.md` before handoff or completion.

## Entry Standard
- Keep entries atomic and specific.
- Capture architectural or behavioral impact, not just filenames.
- Use one to two sentences per atomic change where possible.
- Do not batch unrelated work into one vague line.

## Timing
- Update `CHANGELOG.md` in the same session as the change.
- Do not defer changelog updates to a later agent unless explicitly directed by the user.

## Handoff Requirement
Every handoff brief in `AI/collaboration/` must state whether `CHANGELOG.md` was updated, and if not, why.
