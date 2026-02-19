# AESP Scoring Profiles

`AESPClassifier` now supports profile-driven district scoring through `ScoringProfile`.

## Built-in Profiles
- `Urban`
- `Suburban`
- `Rural`
- `Industrial`

## Deterministic Selection
Use `AESPClassifier::selectProfileForSeed(seed)`.
Profile selection is deterministic by `seed % 4`.

## Custom Profiles
Construct `ScoringProfile` and override per-district weight sets:
- `mixed`
- `residential`
- `commercial`
- `civic`
- `industrial`

Each weight set uses:
- `access`
- `exposure`
- `serviceability`
- `privacy`

Example pattern:
- emphasize industrial selection by increasing `industrial.access` and `industrial.serviceability`
- emphasize residential selection by increasing `residential.privacy`
