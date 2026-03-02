# RogueCities Collaboration SOP Manifest

## Intent
Provide one shared, low-friction protocol for Codex, Gemini, Claude, Copilot, and local agents to coordinate work without context loss.

## 1) Session Start
1. Read `AI/Agent-Contract.md`.
2. Read `.gemini/GEMINI.md`.
3. Read current files in `AI/collaboration/` to detect active workstreams and avoid overlap.
4. Declare planned layer target (`core`, `generators`, `app`, `visualizer`, or `meta/collaboration`).

## 2) Ownership Claim (Lightweight)
Create or update a short session brief in `AI/collaboration/` using:
- `agent_<date>_session.md` or `agent_<feature>_brief.md`

Include:
- Objective
- Files expected to change
- Layer ownership
- Risks/unknowns

## 3) Execution Rules
1. Respect layer boundaries exactly; do not cross ownership concerns.
2. Preserve determinism for generation/pipeline logic.
3. Use dev-shell commands for configure/build/test flows.
4. For UI changes, run perception/smoke gates before declaring done.

## 4) Validation Gates
- UI scope: run `rc-perceive-ui -Mode quick -Screenshot`; run `rc-full-smoke -Port 7222 -Runs 1` for integrated confidence.
- Generator/app contract changes: run targeted tests first, then integration tests if contracts changed.
- Crash reports: inspect `AI/diagnostics/runtime_crash.json` first.
- Build failures: inspect `.vscode/problems.export.json` or `rc-problems` first.

## 5) Handoff Format
When handing off to another agent, include:
1. Goal and explicit next action.
2. Layer boundary statement.
3. Files touched and files intentionally not touched.
4. Validation performed and results.
5. Open risks/questions.

## 6) Completion Protocol
1. Write a completion brief in `AI/collaboration/`.
2. Update `CHANGELOG.md` with a concise atomic entry (mandatory for any tangible change).
3. If work is partial, explicitly list the first unblocked next step for the next agent.
4. In the completion or handoff brief, explicitly state: `CHANGELOG updated: yes/no` and include reason if `no`.

## 6.1) Global Changelog Mandate
- All agents must follow: `AI/collaboration/CHANGELOG_MANDATE.md`.
- If there is any conflict, the stricter changelog requirement wins.

## 7) File Naming Convention
- Session brief: `codex_YYYY-MM-DD_session.md`, `gemini_YYYY-MM-DD_session.md`, `claude_YYYY-MM-DD_session.md`
- Feature brief: `agent_<feature>_brief.md`
- Handoff: `agent_handoff_<target>_<topic>_YYYY-MM-DD.md`

## 8) Decision Tie-Breakers
- Contract and layer rules override speed.
- Determinism and correctness override stylistic changes.
- If uncertain, choose the smallest reversible change and document assumptions.
