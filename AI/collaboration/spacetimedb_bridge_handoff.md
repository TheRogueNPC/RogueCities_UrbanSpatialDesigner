# SpacetimeDB Bridge — Handoff Brief (2026-03-06)

## What Was Done
Full SpacetimeDB integration for the RogueCities project using a Rust FFI bridge.

## Architecture
- **Server**: Rust module at `server/src/lib.rs` with 7 tables, 10 reducers
- **FFI**: `rc_db_client/src/lib.rs` — 12 extern C functions
- **C++ Wrapper**: `core/include/RogueCity/Core/Database/SpacetimeClient.hpp`
- **Compute Worker**: `app/src/Integration/DatabaseComputeWorker.cpp` — runs CityGenerator and pushes output to DB

## Key Contracts
- Vec2 data crosses C ABI as interleaved flat `double*` arrays: `[x0,y0,x1,y1,...]`
- `generation_serial` (uint64_t) is stamped on every pushed row — use to detect stale data
- `ClearGenerated()` wipes all generator-produced rows before each pass
- 200ms debounce prevents flooding during rapid axiom edits

## Tables
Axiom, Road, District, Lot, Building, GenerationStats, GenerationIntent

## Build
- Rust crate builds via `cargo build` invoked from CMake/MSBuild
- `rc_db_client.lib` is an imported static library linked to `RogueCityCore`
- System libs: ws2_32, userenv, bcrypt, advapi32, ntdll, crypt32, secur32, ncrypt

## Future Work
- Wire UI tool actions (axiom placement) through `SpacetimeClient::PlaceAxiom()` instead of direct GlobalState mutation
- Add subscription callbacks to sync DB changes back into `GlobalState` for UI rendering
- Consider WebSocket connection pooling for multi-client scenarios
