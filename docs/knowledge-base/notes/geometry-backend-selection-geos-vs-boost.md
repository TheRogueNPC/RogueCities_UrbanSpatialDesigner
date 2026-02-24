---
tags: [roguecity, geometry, boost-geometry, cmake-flags]
type: reference
created: 2026-02-15
---

# Geometry Backend Selection (Boost.Geometry)

Geometry processing uses Boost.Geometry in this repo. GEOS is not part of the active build path.

## Relevant Build Flags
- No GEOS toggle is supported.
- `ROGUECITY_USE_BOOST_GEOMETRY=1` is defined for generators.

## Source Files
- `CMakeLists.txt`
- `generators/CMakeLists.txt`
- `BUILD_INSTRUCTIONS.md`

## Related
- [[topics/procedural-generation-and-zoning]]
- [[topics/build-and-developer-workflows]]
- [[notes/build-workflows-visual-studio-and-cmake]]
