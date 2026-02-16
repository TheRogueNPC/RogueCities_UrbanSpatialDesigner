# RogueProfiler Spec (Compatibility Phase)

Purpose: introduce a universal scoring API without breaking current frontage/AESP behavior.

## Current Phase Scope

1. `RogueProfiler` is introduced as the public scoring entrypoint.
2. Implementation is compatibility-backed by `AESPClassifier`.
3. Default output behavior must remain parity-equivalent to prior AESP path.

## API

Header: `generators/include/RogueCity/Generators/Scoring/RogueProfiler.hpp`

1. `RogueProfiler::Scores`
2. `computeScores(primary, secondary)`
3. `classifyDistrict(scores)`
4. `classifyLot(lot)`
5. `roadTypeToAccess/Exposure/Serviceability/Privacy`

## Migrated Call Sites

1. `DistrictGenerator`
2. `LotGenerator`
3. `ZoningGenerator`
4. `RoadClassifier`

## Migration Contract

1. New generator code should call `RogueProfiler` directly.
2. `AESPClassifier` remains as compatibility backend during this phase.
3. Future weighting/profile extension must be implemented in `RogueProfiler`, not by reintroducing direct `AESPClassifier` usage.
