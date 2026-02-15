# Migration Log

## What Was Reorganized

### Moved to Active Canonical Structure
- `Progam_Specs/*` -> `20_specs/*` and `30_architecture/*`
- `Intergration Notes/*` -> `40_implementation_completed/*` and `41_plans_todos/*`
- `Tests/*` -> `50_testing/*`
- `Operation Notes/*` -> `60_operations/*`
- `Plans/*` -> `41_plans_todos/*` (or archive for non-doc placeholders)
- `Doxy/*` -> `generated/doxygen/*`

### Most Important Separation
- All planning/TODO docs are now consolidated in:
  - `docs/41_plans_todos`

### Archived
- Legacy structure moved to:
  - `docs/90_archive/legacy_temp/old_structure`
- Duplicate and stub files moved to:
  - `docs/90_archive/stubs_and_duplicates`

## Preservation Notes
- No content was deleted.
- Duplicate and low-value variants were archived, not removed.
