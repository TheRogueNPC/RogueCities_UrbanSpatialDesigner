# FBCZ Schema (JSON)

Top-level object:
- `version`: string
- `zones`: array of zone rules

Zone rule object fields:
- `zone_type`: string (required)
- `build_to_min`: float meters
- `build_to_max`: float meters (must be >= `build_to_min`)
- `max_setback`: float meters
- `frontage_occupancy_min`: float [0, 1]
- `height_min`: float meters
- `height_max`: float meters (must be >= `height_min`)
- `massing_floor_area_ratio`: float (> 0)

Runtime mapping:
- Districts and lots carry `form_district` attributes.
- Lots cache resolved FBCZ bounds (`fbcz_build_to_*`, `fbcz_max_setback`, `fbcz_height_*`).
- Buildings store enforcement outputs (`fbcz_height_min/max`, frontage occupancy, violation flag).

Determinism:
- Rule evaluation is deterministic for fixed lot/building ordering and fixed JSON content.
- Invalid or missing zone entries are rejected at load time.
