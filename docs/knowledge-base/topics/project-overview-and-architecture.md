---
tags: [roguecity, architecture, modules]
type: concept
created: 2026-02-15
---

# Project Overview and Architecture (Rogue City Designer C++20)

Rogue City Designer is structured as a layered C++20 system where data models, procedural generation, app logic, AI features, and visualizer UI are intentionally separated so each layer can evolve with limited coupling.

## Scope
- Core data types and editor state: `core/`
- Generation algorithms and city pipeline: `generators/`
- App integration and tooling layer: `app/`
- AI protocol/runtime clients: `AI/`
- Headless + GUI visualizer executables: `visualizer/`

## Related Notes
- [[notes/rogue-city-project-overview]]
- [[notes/layered-module-structure-core-generators-app-ai-visualizer]]
- [[notes/module-visualizer-executables]]
- [[topics/build-and-developer-workflows]]
