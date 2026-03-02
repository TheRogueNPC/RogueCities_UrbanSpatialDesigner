# Handoff to Gemini - UI Fixes/Changes Implementation Phase

**From:** Codex  
**To:** Gemini  
**Date:** 2026-02-28  
**Topic:** Move into UI fixes, UI changes, and implementation execution

## Context
User confirmed readiness to proceed to the next phase focused on UI work.  
User will provide task-specific UI requirements directly to Gemini.

## Requested Direction
Please take point on the next UI phase:
1. Receive the user’s concrete UI task list.
2. Implement UI fixes/changes in the correct layer (`visualizer/` and, where applicable, `app/` integration only).
3. Preserve architectural boundaries and existing compatibility paths.

## Mandatory Guardrails
- Follow `AI/Agent-Contract.md` and `.gemini/GEMINI.md` as binding constraints.
- Do not introduce generation policy into UI layer.
- Do not bypass `UiIntrospector` instrumentation in panels.
- Keep existing panel registration and structural patterns consistent.

## Validation Expectations
- After UI modifications, run:
  - `rc-perceive-ui -Mode quick -Screenshot`
  - `rc-full-smoke -Port 7222 -Runs 1` for integrated verification when scope warrants
- Verify required UI sections remain present (Viewport, Titlebar, Status).

## Documentation Expectations
- Write a post-task brief to `AI/collaboration/` with:
  - Layer worked
  - Files changed
  - Validation results
  - Open risks/follow-ups
- Append concise changelog notes to `CHANGELOG.md` (mandatory for tangible changes).
- Follow `AI/collaboration/CHANGELOG_MANDATE.md`.

## Current Status from Codex
- Collaboration alignment complete.
- SOP manifest added to support cross-agent coordination:
  - `AI/collaboration/collaboration_sop_manifest.md`
