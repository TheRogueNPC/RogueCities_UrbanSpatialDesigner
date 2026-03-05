#include "RogueCity/Generators/IO/GeoPackageIO.hpp"

#include <array>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <utility>

#if defined(ROGUECITY_ENABLE_GDAL_GPKG)
#include <gdal.h>
#include <ogrsf_frmts.h>
#endif

namespace RogueCity::Generators::IO {

namespace {

void SetError(std::string *error, const std::string &message) {
  if (error != nullptr) {
    *error = message;
  }
}

Core::Vec2 PolygonCentroid(const std::vector<Core::Vec2> &ring) {
  if (ring.empty()) {
    return {};
  }
  Core::Vec2 centroid{};
  for (const auto &p : ring) {
    centroid += p;
  }
  centroid /= static_cast<double>(ring.size());
  return centroid;
}

#if defined(ROGUECITY_ENABLE_GDAL_GPKG)

bool FillOgrSpatialReference(const Core::Data::SpatialReference &input,
                             OGRSpatialReference &out) {
  if (input.kind() == Core::Data::SpatialReferenceKind::EPSG &&
      input.epsgCode() > 0) {
    return out.importFromEPSG(input.epsgCode()) == OGRERR_NONE;
  }

  if (input.kind() == Core::Data::SpatialReferenceKind::Proj4 &&
      !input.proj4().empty()) {
    return out.importFromProj4(input.proj4().c_str()) == OGRERR_NONE;
  }

  if (input.kind() == Core::Data::SpatialReferenceKind::WKT &&
      !input.wkt().empty()) {
    const std::string wkt = input.wkt();
    char *wkt_mutable = const_cast<char *>(wkt.c_str());
    return out.importFromWkt(&wkt_mutable) == OGRERR_NONE;
  }

  return false;
}

Core::Data::SpatialReference ReadSpatialReference(const OGRLayer *layer) {
  if (layer == nullptr || layer->GetSpatialRef() == nullptr) {
    return Core::Data::SpatialReference::LocalPlanarMeters();
  }

  const OGRSpatialReference *srs = layer->GetSpatialRef();
  const char *authority = srs->GetAuthorityName(nullptr);
  const char *code = srs->GetAuthorityCode(nullptr);
  if (authority != nullptr && code != nullptr && std::string(authority) == "EPSG") {
    return Core::Data::SpatialReference::FromEPSG(std::atoi(code));
  }

  char *proj4 = nullptr;
  if (srs->exportToProj4(&proj4) == OGRERR_NONE && proj4 != nullptr) {
    std::string proj4_copy(proj4);
    CPLFree(proj4);
    if (!proj4_copy.empty()) {
      return Core::Data::SpatialReference::FromProj4(std::move(proj4_copy));
    }
  }

  char *wkt = nullptr;
  if (srs->exportToWkt(&wkt) == OGRERR_NONE && wkt != nullptr) {
    std::string wkt_copy(wkt);
    CPLFree(wkt);
    if (!wkt_copy.empty()) {
      return Core::Data::SpatialReference::FromWkt(std::move(wkt_copy));
    }
  }

  return Core::Data::SpatialReference::LocalPlanarMeters();
}

bool WriteRoadsLayer(GDALDataset *dataset,
                     const Core::Data::SpatialReference &spatial_reference,
                     const fva::Container<Core::Road> &roads,
                     std::string *error) {
  OGRSpatialReference ogr_srs{};
  OGRSpatialReference *srs_ptr = nullptr;
  if (FillOgrSpatialReference(spatial_reference, ogr_srs)) {
    srs_ptr = &ogr_srs;
  }

  OGRLayer *layer = dataset->CreateLayer("roads", srs_ptr, wkbLineString,
                                         nullptr);
  if (layer == nullptr) {
    SetError(error, "Unable to create 'roads' layer");
    return false;
  }

  OGRFieldDefn id_field("id", OFTInteger64);
  OGRFieldDefn type_field("road_type", OFTInteger);
  if (layer->CreateField(&id_field) != OGRERR_NONE ||
      layer->CreateField(&type_field) != OGRERR_NONE) {
    SetError(error, "Unable to add fields to 'roads' layer");
    return false;
  }

  for (const auto &road : roads) {
    if (road.points.size() < 2) {
      continue;
    }

    OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
    feature->SetField("id", static_cast<GIntBig>(road.id));
    feature->SetField("road_type", static_cast<int>(road.type));

    OGRLineString line{};
    for (const auto &point : road.points) {
      line.addPoint(point.x, point.y);
    }

    feature->SetGeometry(&line);
    if (layer->CreateFeature(feature) != OGRERR_NONE) {
      OGRFeature::DestroyFeature(feature);
      SetError(error, "Unable to write a road feature");
      return false;
    }
    OGRFeature::DestroyFeature(feature);
  }

  return true;
}

bool WritePolygonLayer(const char *name, GDALDataset *dataset,
                       const Core::Data::SpatialReference &spatial_reference,
                       const std::vector<Core::District> &districts,
                       const std::vector<Core::LotToken> &lots,
                       std::string *error) {
  OGRSpatialReference ogr_srs{};
  OGRSpatialReference *srs_ptr = nullptr;
  if (FillOgrSpatialReference(spatial_reference, ogr_srs)) {
    srs_ptr = &ogr_srs;
  }

  OGRLayer *layer =
      dataset->CreateLayer(name, srs_ptr, wkbPolygon, nullptr);
  if (layer == nullptr) {
    SetError(error, std::string("Unable to create '") + name + "' layer");
    return false;
  }

  OGRFieldDefn id_field("id", OFTInteger64);
  if (layer->CreateField(&id_field) != OGRERR_NONE) {
    SetError(error, std::string("Unable to add fields to '") + name + "' layer");
    return false;
  }

  if (std::string(name) == "districts") {
    for (const auto &district : districts) {
      if (district.border.size() < 3) {
        continue;
      }

      OGRLinearRing ring{};
      for (const auto &p : district.border) {
        ring.addPoint(p.x, p.y);
      }
      if (!district.border.front().equals(district.border.back())) {
        ring.addPoint(district.border.front().x, district.border.front().y);
      }

      OGRPolygon polygon{};
      polygon.addRing(&ring);

      OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
      feature->SetField("id", static_cast<GIntBig>(district.id));
      feature->SetGeometry(&polygon);
      if (layer->CreateFeature(feature) != OGRERR_NONE) {
        OGRFeature::DestroyFeature(feature);
        SetError(error, "Unable to write a district feature");
        return false;
      }
      OGRFeature::DestroyFeature(feature);
    }
    return true;
  }

  for (const auto &lot : lots) {
    if (lot.boundary.size() < 3) {
      continue;
    }

    OGRLinearRing ring{};
    for (const auto &p : lot.boundary) {
      ring.addPoint(p.x, p.y);
    }
    if (!lot.boundary.front().equals(lot.boundary.back())) {
      ring.addPoint(lot.boundary.front().x, lot.boundary.front().y);
    }

    OGRPolygon polygon{};
    polygon.addRing(&ring);

    OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
    feature->SetField("id", static_cast<GIntBig>(lot.id));
    feature->SetGeometry(&polygon);
    if (layer->CreateFeature(feature) != OGRERR_NONE) {
      OGRFeature::DestroyFeature(feature);
      SetError(error, "Unable to write a lot feature");
      return false;
    }
    OGRFeature::DestroyFeature(feature);
  }

  return true;
}

void WriteMetricsLayer(GDALDataset *dataset, const CityGenerator::CityOutput &city) {
  OGRLayer *layer = dataset->CreateLayer("rc_metrics", nullptr, wkbNone, nullptr);
  if (layer == nullptr) {
    return;
  }

  OGRFieldDefn key_field("key", OFTString);
  key_field.SetWidth(64);
  OGRFieldDefn value_field("value", OFTString);
  value_field.SetWidth(64);
  if (layer->CreateField(&key_field) != OGRERR_NONE ||
      layer->CreateField(&value_field) != OGRERR_NONE) {
    return;
  }

  const std::array<std::pair<const char *, std::string>, 3> metrics{{
      {"roads", std::to_string(city.roads.size())},
      {"districts", std::to_string(city.districts.size())},
      {"lots", std::to_string(city.lots.size())},
  }};

  for (const auto &entry : metrics) {
    OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
    feature->SetField("key", entry.first);
    feature->SetField("value", entry.second.c_str());
    layer->CreateFeature(feature);
    OGRFeature::DestroyFeature(feature);
  }
}

#endif

} // namespace

bool GeoPackageIO::IsSupported() {
#if defined(ROGUECITY_ENABLE_GDAL_GPKG)
  return true;
#else
  return false;
#endif
}

bool GeoPackageIO::ExportCity(const std::string &gpkg_path,
                              const CityGenerator::CityOutput &city,
                              std::string *error) {
#if !defined(ROGUECITY_ENABLE_GDAL_GPKG)
  SetError(error, "GeoPackage export unavailable: build without GDAL support");
  (void)gpkg_path;
  (void)city;
  return false;
#else
  GDALAllRegister();
  std::remove(gpkg_path.c_str());

  GDALDriver *driver =
      GetGDALDriverManager()->GetDriverByName("GPKG");
  if (driver == nullptr) {
    SetError(error, "GDAL GPKG driver not available");
    return false;
  }

  GDALDataset *dataset = driver->Create(gpkg_path.c_str(), 0, 0, 0, GDT_Unknown,
                                        nullptr);
  if (dataset == nullptr) {
    SetError(error, "Unable to create GeoPackage dataset");
    return false;
  }

  const Core::Data::SpatialReference spatial_reference = city.spatial_reference;

  const bool ok = WriteRoadsLayer(dataset, spatial_reference, city.roads, error) &&
                  WritePolygonLayer("districts", dataset, spatial_reference,
                                    city.districts, city.lots, error) &&
                  WritePolygonLayer("lots", dataset, spatial_reference,
                                    city.districts, city.lots, error);
  if (ok) {
    WriteMetricsLayer(dataset, city);
  }

  GDALClose(dataset);
  return ok;
#endif
}

bool GeoPackageIO::ImportCity(const std::string &gpkg_path,
                              GeoPackageImportResult &out,
                              std::string *error) {
#if !defined(ROGUECITY_ENABLE_GDAL_GPKG)
  SetError(error, "GeoPackage import unavailable: build without GDAL support");
  (void)gpkg_path;
  (void)out;
  return false;
#else
  GDALAllRegister();

  GDALDataset *dataset = static_cast<GDALDataset *>(
      GDALOpenEx(gpkg_path.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
  if (dataset == nullptr) {
    SetError(error, "Unable to open GeoPackage dataset");
    return false;
  }

  out = GeoPackageImportResult{};

  if (OGRLayer *roads_layer = dataset->GetLayerByName("roads");
      roads_layer != nullptr) {
    out.spatial_reference = ReadSpatialReference(roads_layer);
    roads_layer->ResetReading();
    OGRFeature *feature = nullptr;
    while ((feature = roads_layer->GetNextFeature()) != nullptr) {
      Core::Road road{};
      road.id = static_cast<uint32_t>(feature->GetFieldAsInteger64("id"));
      const int type_raw = feature->GetFieldAsInteger("road_type");
      road.type = static_cast<Core::RoadType>(std::clamp(type_raw, 0, static_cast<int>(Core::road_type_count - 1)));
      if (OGRGeometry *geometry = feature->GetGeometryRef(); geometry != nullptr) {
        OGRwkbGeometryType type = wkbFlatten(geometry->getGeometryType());
        if (type == wkbLineString) {
          if (auto *line = dynamic_cast<OGRLineString *>(geometry); line != nullptr) {
            for (int i = 0; i < line->getNumPoints(); ++i) {
              road.points.emplace_back(line->getX(i), line->getY(i));
            }
          }
        } else if (type == wkbMultiLineString) {
          if (auto *multi = dynamic_cast<OGRMultiLineString *>(geometry);
              multi != nullptr && multi->getNumGeometries() > 0) {
            if (auto *line =
                    dynamic_cast<OGRLineString *>(multi->getGeometryRef(0));
                line != nullptr) {
              for (int i = 0; i < line->getNumPoints(); ++i) {
                road.points.emplace_back(line->getX(i), line->getY(i));
              }
            }
          }
        }
      }
      if (road.points.size() >= 2) {
        out.roads.push_back(std::move(road));
      }
      OGRFeature::DestroyFeature(feature);
    }
  }

  auto read_polygon_layer = [&](const char *name, bool lots_layer) {
    OGRLayer *layer = dataset->GetLayerByName(name);
    if (layer == nullptr) {
      return;
    }
    if (out.spatial_reference.isLocalPlanarMeters()) {
      out.spatial_reference = ReadSpatialReference(layer);
    }

    layer->ResetReading();
    OGRFeature *feature = nullptr;
    while ((feature = layer->GetNextFeature()) != nullptr) {
      OGRGeometry *geometry = feature->GetGeometryRef();
      if (geometry == nullptr || wkbFlatten(geometry->getGeometryType()) != wkbPolygon) {
        OGRFeature::DestroyFeature(feature);
        continue;
      }
      auto *polygon = dynamic_cast<OGRPolygon *>(geometry);
      if (polygon == nullptr) {
        OGRFeature::DestroyFeature(feature);
        continue;
      }
      OGRLinearRing *ring = polygon->getExteriorRing();
      if (ring == nullptr || ring->getNumPoints() < 4) {
        OGRFeature::DestroyFeature(feature);
        continue;
      }

      std::vector<Core::Vec2> border;
      border.reserve(static_cast<size_t>(ring->getNumPoints()));
      for (int i = 0; i < ring->getNumPoints(); ++i) {
        border.emplace_back(ring->getX(i), ring->getY(i));
      }
      if (!border.empty() && border.front().equals(border.back())) {
        border.pop_back();
      }

      if (lots_layer) {
        Core::LotToken lot{};
        lot.id = static_cast<uint32_t>(feature->GetFieldAsInteger64("id"));
        lot.boundary = border;
        lot.centroid = PolygonCentroid(border);
        out.lots.push_back(std::move(lot));
      } else {
        Core::District district{};
        district.id = static_cast<uint32_t>(feature->GetFieldAsInteger64("id"));
        district.border = border;
        out.districts.push_back(std::move(district));
      }

      OGRFeature::DestroyFeature(feature);
    }
  };

  read_polygon_layer("districts", false);
  read_polygon_layer("lots", true);

  GDALClose(dataset);
  return true;
#endif
}

} // namespace RogueCity::Generators::IO
