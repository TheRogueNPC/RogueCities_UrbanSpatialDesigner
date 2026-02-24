---
tags: [roguecity, architecture, modules, layer-separation]
type: concept
created: 2026-02-15
---

# Layered Module Structure (Core, Generators, App, AI, Visualizer)

The repo is intentionally divided into five major layers so low-level data and algorithms remain reusable while UI and assistant features can iterate without contaminating core model boundaries.

## Module Boundaries
- `core/`: math, data, editor state, validation
- `generators/`: tensor/road/district/urban/pipeline generation
- `app/`: tooling, docking, viewport sync, generator bridges
- `AI/`: runtime, protocol, clients, HTTP tooling
- `visualizer/`: headless and GUI executables

## Source Files
- `CMakeLists.txt`
- `core/CMakeLists.txt`
- `generators/CMakeLists.txt`
- `app/CMakeLists.txt`
- `AI/CMakeLists.txt`
- `visualizer/CMakeLists.txt`

## Related
- [[topics/project-overview-and-architecture]]
- [[notes/core-library-data-and-editor-types]]
- [[notes/module-visualizer-executables]]
