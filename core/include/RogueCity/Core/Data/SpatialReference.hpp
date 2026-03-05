#pragma once

#include <cstdint>
#include <string>

namespace RogueCity::Core::Data {

enum class SpatialReferenceKind : uint8_t {
  LocalPlanarMeters = 0,
  EPSG,
  Proj4,
  WKT
};

class SpatialReference {
public:
  SpatialReference() = default;

  [[nodiscard]] static SpatialReference LocalPlanarMeters();
  [[nodiscard]] static SpatialReference FromEPSG(int epsg_code,
                                                 std::string name = {});
  [[nodiscard]] static SpatialReference FromProj4(std::string proj4,
                                                  std::string name = {});
  [[nodiscard]] static SpatialReference FromWkt(std::string wkt,
                                                std::string name = {});

  [[nodiscard]] SpatialReferenceKind kind() const noexcept { return kind_; }
  [[nodiscard]] const std::string &name() const noexcept { return name_; }
  [[nodiscard]] const std::string &authorityName() const noexcept {
    return authority_name_;
  }
  [[nodiscard]] int epsgCode() const noexcept { return epsg_code_; }
  [[nodiscard]] const std::string &proj4() const noexcept { return proj4_; }
  [[nodiscard]] const std::string &wkt() const noexcept { return wkt_; }
  [[nodiscard]] bool unitsMeters() const noexcept { return units_meters_; }

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] bool isLocalPlanarMeters() const noexcept;
  [[nodiscard]] std::string canonicalId() const;

  bool operator==(const SpatialReference &other) const noexcept;
  bool operator!=(const SpatialReference &other) const noexcept {
    return !(*this == other);
  }

private:
  SpatialReferenceKind kind_{SpatialReferenceKind::LocalPlanarMeters};
  std::string name_{};
  std::string authority_name_{};
  int epsg_code_{0};
  std::string proj4_{};
  std::string wkt_{};
  bool units_meters_{true};
};

} // namespace RogueCity::Core::Data
