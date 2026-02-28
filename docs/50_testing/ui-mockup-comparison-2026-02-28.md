# UI Mockup Comparison Report (2026-02-28)

## Scope
- Compare runtime visualizer UI shape against `visualizer/RC_UI_Mockup.html`.
- Use concrete runtime snapshot output from headless executable:
  - `AI/docs/ui/ui_introspection_headless_latest.json`

## Method
1. Rebuilt `RogueCityVisualizerHeadless` with snapshot export CLI support.
2. Ran headless visualizer and exported UI introspection snapshot.
3. Compared mockup shell contract (titlebar, master, viewport, inspector, status) against exported runtime panels and dock areas.
4. Queried local model (`deepseek-coder-v2:16b`) for auxiliary comparison signal.

## Runtime Snapshot (Observed)
- `Master Panel` (`Left`, visible)
- `[/ / / RC_VISUALIZER / / /]` (`Center`, visible)
- `System Map` (`Left`, not visible)
- `WorkspaceSelector` (`Left`, visible)

## Mockup Contract (Expected High-Level)
- Top `titlebar`
- Left `master` panel with tabs/sub-tabs
- Center `viewport`
- Right `inspector` panel (with `Inspector` / `System Map` tabs)
- Bottom `status` bar

## Matches
- Runtime has left-side master container and center viewport matching mockup shell intent.
- Dock layout ratios in code align with mockup token defaults (`master-ratio 0.32`, `right-ratio 0.22`, `tool-deck-ratio 0.24`).
- `System Map` concept exists in runtime panel set.

## Gaps
- No explicit runtime `Inspector` panel captured in exported introspection snapshot.
- No explicit runtime `Status Bar` panel captured in exported introspection snapshot.
- No explicit runtime `Title Bar` panel captured in exported introspection snapshot.
- `System Map` appears in snapshot as `Left`/hidden, while mockup places it as a right-column inspector tab.
- Runtime includes `WorkspaceSelector` panel not represented as a primary shell section in mockup.

## Risks
- Snapshot-based layout audits can under-report parity if important shell elements are not introspection-registered.
- Dock-area metadata may drift from actual docking behavior if panel meta tags are stale.
- Automated AI comparison quality is prompt/context sensitive; model output should be treated as advisory, not source of truth.

## Top Actions
1. Register `Inspector`, `TitleBar`, and `StatusBar` as introspection panels/widgets so parity checks are complete.
2. Ensure `System Map` introspection dock metadata matches actual dock contract (`Right` tab group with Inspector).
3. Add a deterministic `ui_layout_contract` export command that emits shell sections independent of panel visibility.
4. Add CI smoke check: fail when required shell sections (`master`, `viewport`, `inspector`, `status`) are absent from exported contract.

## Evidence Commands
- Build: `cmake --build build_vs --config Release --target RogueCityVisualizerHeadless --parallel 8`
- Snapshot export: `RogueCityVisualizerHeadless.exe --frames 5 --export-ui-snapshot AI/docs/ui/ui_introspection_headless_latest.json`
- Local AI query: `rc-ai-query -Model deepseek-coder-v2:16b ...`
