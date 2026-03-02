# Claude Code: Gemma Suite Expansion

**Agent:** Claude Code
**Date:** 2026-03-01
**Layer:** meta/ai-knowledge
**Topic:** Expand internal Gemma suite for four-layer codebase understanding

---

## Objective

Expand the Gemma test suite and symbol knowledge so the local Ollama stack
(gemma3:4b, codegemma:2b, gemma3:12b, embeddinggemma) has comprehensive coverage
of all four codebase layers: core, generators, app, visualizer.

---

## Files Changed

### Modified
- `tests/test_ai.cpp` — Added 12 new test cases (17 total, up from 5)
- `AI/docs/symbol_index.json` — Added 29 new symbol entries

### Not Touched
- All generator, core, app, and visualizer source files (read-only)
- CMakeLists.txt — test_ai was already registered; no new targets needed

---

## New Test Cases (tests/test_ai.cpp)

### Group A — Core-layer vocabulary
| Test | Covers |
|------|--------|
| `test_core_cityspec_full_district_types` | All 5 district type strings (residential/commercial/industrial/civic/luxury) |
| `test_core_cityspec_style_tags_axiom_mapping` | 8 axiom style tags (organic/grid/radial/hexagonal/stem/dendritic/spine/pinwheel) |
| `test_core_cityspec_boundary_seed_and_density` | seed=0 preserved, seed=UINT64_MAX roundtrips, density extremes |

### Group B — Generators-layer model routing
| Test | Covers |
|------|--------|
| `test_generators_aiconfig_pipeline_v2_models` | 7 Pipeline V2 model roles (controller/triage/synth_fast/synth_escalation/embedding/vision/ocr) |
| `test_generators_aiconfig_pipeline_v2_flags` | pipeline_v2_enabled, audit_strict_enabled, embedding_dimensions=768 |
| `test_generators_cityspec_all_scale_values` | "hamlet"/"town"/"city"/"metro" all roundtrip via CitySpecClient |

### Group C — App-layer model vocabulary and UI state
| Test | Covers |
|------|--------|
| `test_app_aiconfig_all_model_fields` | 4 app-model fields (ui_agent/city_spec/code_assistant/naming) + bridge_base_url |
| `test_app_uisnapshot_state_model_cross_layer_keys` | 7 cross-layer state_model keys (axiom/road/district/zoning/simulation/theme/workspace) |
| `test_app_uisnapshot_header_modes_and_filters` | 3 header modes (SOLITON/REACTIVE/SATELLITE) + 4 filters (NORMAL/CAUTION/EVASION/ALERT) |

### Group D — Visualizer-layer panels, commands, dock positions
| Test | Covers |
|------|--------|
| `test_vis_uisnapshot_all_real_panels_and_roles` | 12 canonical panels with roles (toolbox/inspector/nav/control/diagnostics) |
| `test_vis_uicommand_all_cmd_types_roundtrip` | All 5 UiCommand types (SetHeader/RenamePanel/DockPanel/SetState/Request) via CommandsFromJson |
| `test_vis_uisnapshot_all_dock_positions` | All 5 dock values (Left/Right/Bottom/Top/Center) serialise correctly |

---

## New symbol_index.json Entries

### AI Layer (9 new files — was completely absent)
- `AI/protocol/UiAgentProtocol.h` → UiPanelInfo, UiHeaderInfo, UiStateInfo, UiSnapshot, UiCommand, UiAgentJson
- `AI/config/AiConfig.h` → AiConfig, AiConfigManager
- `AI/client/CitySpecClient.h` → CitySpecClient
- `AI/client/UiAgentClient.h` → UiAgentClient
- `AI/client/UiDesignAssistant.h` → UiDesignAssistant
- `AI/integration/AiAssist.h` → AiAssist
- `AI/runtime/AiBridgeRuntime.h` → AiBridgeRuntime
- `AI/runtime/AiAvailability.h` → AiAvailability
- `AI/tools/Url.h` → ParsedUrl, ParseUrl, JoinUrlPath

### Missing Visualizer Panels (20 new files)
Inspector, InspectorSidebar, Workspace, DevShell, BuildingControl, LotControl,
WaterControl, Tools, SystemMap, Log, Telemetry, AxiomBar, UiSettings, Validation,
BuildingIndex, DistrictIndex, LotIndex, RoadIndex, RiverIndex

---

## Validation

1. `rc-bld-core` — Build test_ai; all 17 tests must pass
2. `python -c "import json; json.load(open('AI/docs/symbol_index.json'))"` — JSON must parse
3. `rc-full-smoke -Port 7222 -Runs 1` — pipeline/query should return richer answers

CHANGELOG updated: yes
