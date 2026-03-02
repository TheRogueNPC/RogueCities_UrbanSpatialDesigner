# Claude Assignment Prompt - Core + Generators (2026-03-01)

## Lane
Own `core + generators` 3D foundation and data-contract continuity.

## Primary Goal
Make generator output structurally 3D-ready and eliminate metadata drop points that currently block downstream 3D implementation.

## Scope
1. Extend core city data contracts so vertical/topology metadata can be preserved end-to-end.
2. Ensure `CityGenerator::CityOutput` carries new metadata with deterministic defaults.
3. Preserve road verticality/layer information from graph processing into emitted road output.
4. Surface intersection template outputs as first-class pipeline output instead of throwaway values.
5. Reduce fallback-only block generation behavior when road-cycle preference is requested.

## High-Signal Source Targets
1. `core/include/RogueCity/Core/Data/CityTypes.hpp`
2. `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp`
3. `generators/src/Generators/Pipeline/CityGenerator.cpp`
4. `generators/include/RogueCity/Generators/Urban/Graph.hpp`
5. `generators/src/Generators/Urban/RoadGenerator.cpp`
6. `generators/include/RogueCity/Generators/Roads/IntersectionTemplates.hpp`
7. `generators/src/Generators/Roads/IntersectionTemplates.cpp`
8. `generators/src/Generators/Urban/PolygonFinder.cpp`
9. `generators/src/Generators/Urban/BlockGenerator.cpp`

## Constraints
1. Do not break existing serialized/editor assumptions without compatibility defaults.
2. Keep all changed paths deterministic by seed.
3. Do not leave silent data drops in adapted pipelines.

## Acceptance Criteria
1. New core/generator fields compile and flow through output without regression.
2. Vertical/layer/interchange metadata survives graph-to-output conversion.
3. Block-generation config meaningfully affects behavior or has explicit non-stub fallback.
4. Tests added/updated for contract and pipeline continuity.
5. Collaboration brief + changelog entry completed.

## Required Report Back
1. Contract changes made.
2. Dataflow before/after (where metadata was previously dropped).
3. Tests run and output.
4. Any temporary compatibility shims and removal plan.

