---
tags: [roguecity, generators, pipeline, tensors, districts]
type: concept
created: 2026-02-15
---

# Generators Pipeline City Generation Stages (Tensor to Sites)

The generators module implements staged city synthesis: tensors and basis fields guide roads, road graphs are classified and simplified, district/frontage logic is applied, and urban pipeline generators produce blocks, lots, and sites.

## Pipeline Stages
- Tensors: `TensorFieldGenerator`, `BasisFields`
- Roads: tracing, classification, graph operations, verticality
- District/urban: `AESPClassifier`, frontage profiles, graph/blocks/lots/sites
- End-to-end pipeline: `CityGenerator`, `CitySpecAdapter`, `ZoningGenerator`

## Source Files
- `generators/CMakeLists.txt`

## Related
- [[topics/procedural-generation-and-zoning]]
- [[notes/geometry-backend-selection-geos-vs-boost]]
- [[notes/rogue-city-project-overview]]
