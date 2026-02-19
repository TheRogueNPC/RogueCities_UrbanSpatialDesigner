# RC 0.10 Verification Report

Date: 2026-02-19

## Build and Test Status
- Clean build (`build_vs_clean`): PASS
- Full test suite (`ctest -C Debug`): PASS (26/26)

## Performance Verification
Source: `bin/test_generation_latency.exe`

- `full_generation_ms`: 211
- `incremental_roads_ms`: 139
- `cancel_response_ms`: 1

Budget file: `tests/baselines/performance_budgets_v0.10.json`

- `full_generation_ms <= 5000`: PASS
- `incremental_roads_ms <= 1000`: PASS
- `cancel_response_ms <= 100`: PASS

## Memory/Sanitizer Verification
Configuration: `build_vs_asan` (`/fsanitize=address`)

Representative ASAN runs (Windows cmd environment with ASAN runtime path):
- `bin/test_core.exe`: PASS
- `bin/test_generation_latency.exe`: PASS

No AddressSanitizer runtime errors were emitted during these runs.

## Lua Compatibility Verification
Standalone Lua 5.4.8 smoke (`build_vs_clean/lua_smoke`):
- `luaL_newstate`, `luaL_openlibs`, `luaL_dostring(\"return 40 + 2\")`: PASS
- Result value: `42`

## Dependency Graph Verification
Target dependency graph: `build_vs_clean/target_graph.dot`

- Nodes: 41
- Edges: 65
- Cycles detected: 0
