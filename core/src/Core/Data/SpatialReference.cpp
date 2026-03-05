#include "RogueCity/Core/Data/SpatialReference.hpp"

#include <utility>

namespace RogueCity::Core::Data {

SpatialReference SpatialReference::LocalPlanarMeters() {
  SpatialReference ref{};
  ref.kind_ = SpatialReferenceKind::LocalPlanarMeters;
  ref.name_ = "RCSD Local Planar Meters";
  ref.authority_name_ = "RCSD";
  ref.epsg_code_ = 0;
  ref.proj4_.clear();
  ref.wkt_.clear();
  ref.units_meters_ = true;
  return ref;
}

SpatialReference SpatialReference::FromEPSG(int epsg_code, std::string name) {
  SpatialReference ref{};
  ref.kind_ = SpatialReferenceKind::EPSG;
  ref.authority_name_ = "EPSG";
  ref.epsg_code_ = epsg_code > 0 ? epsg_code : 0;
  ref.name_ = std::move(name);
  ref.units_meters_ = true;
  return ref;
}

SpatialReference SpatialReference::FromProj4(std::string proj4,
                                             std::string name) {
  SpatialReference ref{};
  ref.kind_ = SpatialReferenceKind::Proj4;
  ref.name_ = std::move(name);
  ref.authority_name_.clear();
  ref.epsg_code_ = 0;
  ref.proj4_ = std::move(proj4);
  ref.units_meters_ = true;
  return ref;
}

SpatialReference SpatialReference::FromWkt(std::string wkt, std::string name) {
  SpatialReference ref{};
  ref.kind_ = SpatialReferenceKind::WKT;
  ref.name_ = std::move(name);
  ref.authority_name_.clear();
  ref.epsg_code_ = 0;
  ref.wkt_ = std::move(wkt);
  ref.units_meters_ = true;
  return ref;
}

bool SpatialReference::empty() const noexcept {
  switch (kind_) {
  case SpatialReferenceKind::LocalPlanarMeters:
    return false;
  case SpatialReferenceKind::EPSG:
    return epsg_code_ <= 0;
  case SpatialReferenceKind::Proj4:
    return proj4_.empty();
  case SpatialReferenceKind::WKT:
    return wkt_.empty();
  }
  return true;
}

bool SpatialReference::isLocalPlanarMeters() const noexcept {
  return kind_ == SpatialReferenceKind::LocalPlanarMeters;
}

std::string SpatialReference::canonicalId() const {
  switch (kind_) {
  case SpatialReferenceKind::LocalPlanarMeters:
    return "LOCAL:RCSD_PLANAR_METERS";
  case SpatialReferenceKind::EPSG:
    return epsg_code_ > 0 ? "EPSG:" + std::to_string(epsg_code_)
                          : "EPSG:UNKNOWN";
  case SpatialReferenceKind::Proj4:
    return proj4_.empty() ? "PROJ4:UNKNOWN" : "PROJ4:" + proj4_;
  case SpatialReferenceKind::WKT:
    return wkt_.empty() ? "WKT:UNKNOWN" : "WKT:" + wkt_;
  }
  return "UNKNOWN";
}

bool SpatialReference::operator==(const SpatialReference &other) const
    noexcept {
  return kind_ == other.kind_ && name_ == other.name_ &&
         authority_name_ == other.authority_name_ &&
         epsg_code_ == other.epsg_code_ && proj4_ == other.proj4_ &&
         wkt_ == other.wkt_ && units_meters_ == other.units_meters_;
}

} // namespace RogueCity::Core::Data
