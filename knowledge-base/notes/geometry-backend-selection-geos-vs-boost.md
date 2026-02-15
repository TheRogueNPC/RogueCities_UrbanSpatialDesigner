---
tags: [roguecity, geometry, geos, boost-geometry, cmake-flags]
type: reference
created: 2026-02-15
---

# Geometry Backend Selection (GEOS vs Boost.Geometry)

Geometry processing is controlled by `USE_LEGACY_GEOS`; when enabled it prefers GEOS and falls back to a legacy shim, while disabled mode prefers Boost and also falls back to the legacy shim if Boost is unavailable.

## Relevant Build Flags
- `-DUSE_LEGACY_GEOS=ON` for GEOS-first mode
- `-DUSE_LEGACY_GEOS=OFF` for Boost.Geometry-first mode
- Fallback macros are defined when preferred backend is missing

## Source Files
- `CMakeLists.txt`
- `generators/CMakeLists.txt`
- `BUILD_INSTRUCTIONS.md`

## Related
- [[topics/procedural-generation-and-zoning]]
- [[topics/build-and-developer-workflows]]
- [[notes/build-workflows-visual-studio-and-cmake]]
