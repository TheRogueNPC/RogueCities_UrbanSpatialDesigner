# CRS Contract

## Internal Canonical Space
- Canonical runtime geometry space is local planar XY in meter-like units.
- All generator metric operations (length, area, bearing, adjacency distance) are Euclidean in this local plane.
- Default canonical CRS identity is `LOCAL:RCSD_PLANAR_METERS`.

## Import Reprojection Policy
- Importers may provide CRS metadata (`EPSG`, `proj4`, or `WKT`) through `Core::Data::SpatialReference`.
- OpenDRIVE import currently preserves CRS metadata only; it does not execute runtime reprojection into canonical space.
- If incoming data is non-planar or geodetic and no explicit reprojection stage is enabled, coordinates are treated as local planar values and ingested as-is.

## Export Metadata Policy
- `CityGenerator::CityOutput` carries `spatial_reference`.
- GeoPackage export writes CRS metadata to OGR layer SRS when GDAL support is enabled.
- When no explicit CRS is present, exports use canonical local planar metadata (`LOCAL:RCSD_PLANAR_METERS`).

## Non-Planar Caveats
- Great-circle/geodesic length and area are out-of-scope in the base pipeline.
- Bearing and curvature constraints are interpreted in planar XY; high-latitude or large-area geographic coordinates can distort planning metrics unless reprojected upstream.
- Policy/auditor thresholds assume meter-scale planar units.
