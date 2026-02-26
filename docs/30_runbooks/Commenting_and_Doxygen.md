# Commenting and Doxygen Runbook

## Goals
- Keep source structure intact while adding documentation.
- Generate API docs from comments with a repeatable command.
- Separate short API comments from long design rationale.

## Safe Commenting Rules
1. Never replace code with comments.
- Do not remove `#pragma once`, includes, namespaces, class/struct declarations, or member fields.
2. Comment declarations, not empty space.
- Put `///` comments directly above the symbol they describe.
3. Keep API comments concise.
- `@brief` for purpose, optional `@param` and `@return` only when they add value.
4. Put long explanations in Markdown docs.
- Architecture/background belongs in `docs/`, not large header blocks.
5. Make changes in small batches and build after each batch.
- Treat documentation edits like code edits.

## Preferred Header Style
```cpp
/// @brief Handles mouse drag updates in world coordinates.
/// @param world_pos Current mouse position in world space.
void on_mouse_move(const Core::Vec2& world_pos) override;
```

## What To Avoid
```cpp
// Bad: descriptive text but declaration removed.
// class Foo does bar and baz...
// namespace ... (comment only)
```

## Doxygen Setup In This Repository
- Config file: `Doxyfile` at repo root.
- CMake target: `docs` (available when Doxygen is installed).
- Output directory: `build/docs/doxygen/html/index.html`.

## Commands
1. Configure build system:
```bash
"/mnt/c/Program Files/CMake/bin/cmake.exe" -S . -B build_vs
```
2. Build documentation target:
```bash
"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_vs --target docs
```
3. Direct Doxygen run (without CMake target):
```bash
doxygen Doxyfile
```

## Review Checklist
1. Project builds after doc edits.
2. `docs` target runs without parse failures.
3. No declarations were replaced by prose.
4. New comments explain behavior/contract, not obvious syntax.
