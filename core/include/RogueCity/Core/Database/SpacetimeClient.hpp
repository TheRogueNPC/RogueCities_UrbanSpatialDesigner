#pragma once

#include <cstdint>
#include <string_view>

// Forward declarations of C-ABI FFI functions exported by rc_db_client Rust
// crate
extern "C" {
bool rc_db_connect(const char *database_name, const char *server_url);
void rc_db_disconnect();

// Axiom mutations
void rc_db_place_axiom(uint32_t axiom_type, double pos_x, double pos_y,
                       double radius, double theta, double decay);
void rc_db_clear_axioms();

// Bulk push from headless compute worker
void rc_db_push_road(uint32_t id, uint32_t road_type, int32_t source_axiom_id,
                     const double *points_xy, uint32_t num_points,
                     int32_t layer_id, bool has_grade_sep, bool has_signal,
                     bool has_crosswalk, uint64_t generation_serial);
void rc_db_push_district(uint32_t id, uint32_t district_type,
                         int32_t primary_axiom_id, const double *border_xy,
                         uint32_t num_border_pts, float budget,
                         uint32_t population, float desirability,
                         float avg_elevation, uint64_t generation_serial);
void rc_db_push_lot(uint32_t id, uint32_t district_id, uint32_t lot_type,
                    double centroid_x, double centroid_y,
                    const double *boundary_xy, uint32_t num_boundary_pts,
                    float area, float access, float exposure,
                    float serviceability, float privacy,
                    uint64_t generation_serial);
void rc_db_push_building(uint32_t id, uint32_t lot_id, uint32_t district_id,
                         uint32_t building_type, double pos_x, double pos_y,
                         float rotation, float footprint_area,
                         float suggested_height, float estimated_cost,
                         uint64_t generation_serial);
void rc_db_push_generation_stats(uint64_t serial, uint32_t roads,
                                 uint32_t districts, uint32_t lots,
                                 uint32_t buildings, float time_ms);
void rc_db_clear_generated();

#ifdef RC_FEATURE_GUIDE_DB_SYNC
// Divider guide table reducers — implemented in server/src/lib.rs
void rc_db_push_div_guide(int id, bool horizontal, double world_pos,
                          const char *label, bool enabled);
void rc_db_remove_div_guide(int id);
void rc_db_clear_div_guides();
#endif
}

namespace RogueCity::Core::Database {

/**
 * @brief Thread-safe C++ wrapper for the SpacetimeDB Rust SDK client.
 * Provides connection management, reducer dispatches, and data-viz push
 * functions.
 */
class SpacetimeClient {
public:
  // === Connection ===
  static bool Connect(std::string_view databaseName,
                      std::string_view serverUrl) {
    return ::rc_db_connect(databaseName.data(), serverUrl.data());
  }
  static void Disconnect() { ::rc_db_disconnect(); }

  // === Axiom Intent Reducers ===
  static void PlaceAxiom(uint32_t typeInt, double x, double y, double radius,
                         double theta, double decay) {
    ::rc_db_place_axiom(typeInt, x, y, radius, theta, decay);
  }
  static void ClearAxioms() { ::rc_db_clear_axioms(); }

  // === Bulk Push (called by DatabaseComputeWorker after generation) ===

  /** @brief Push a single road to the database. `points_xy` is interleaved
   * [x0,y0,x1,y1,...]. */
  static void PushRoad(uint32_t id, uint32_t roadType, int32_t sourceAxiomId,
                       const double *pointsXY, uint32_t numPoints,
                       int32_t layerId, bool gradesSep, bool signal,
                       bool crosswalk, uint64_t serial) {
    ::rc_db_push_road(id, roadType, sourceAxiomId, pointsXY, numPoints, layerId,
                      gradesSep, signal, crosswalk, serial);
  }

  static void PushDistrict(uint32_t id, uint32_t distType,
                           int32_t primaryAxiomId, const double *borderXY,
                           uint32_t numBorderPts, float budget, uint32_t pop,
                           float desirability, float avgElev, uint64_t serial) {
    ::rc_db_push_district(id, distType, primaryAxiomId, borderXY, numBorderPts,
                          budget, pop, desirability, avgElev, serial);
  }

  static void PushLot(uint32_t id, uint32_t districtId, uint32_t lotType,
                      double cx, double cy, const double *boundaryXY,
                      uint32_t numBoundaryPts, float area, float access,
                      float exposure, float serviceability, float privacy,
                      uint64_t serial) {
    ::rc_db_push_lot(id, districtId, lotType, cx, cy, boundaryXY,
                     numBoundaryPts, area, access, exposure, serviceability,
                     privacy, serial);
  }

  static void PushBuilding(uint32_t id, uint32_t lotId, uint32_t districtId,
                           uint32_t bldgType, double px, double py, float rot,
                           float footprint, float height, float cost,
                           uint64_t serial) {
    ::rc_db_push_building(id, lotId, districtId, bldgType, px, py, rot,
                          footprint, height, cost, serial);
  }

  static void PushGenerationStats(uint64_t serial, uint32_t roads,
                                  uint32_t districts, uint32_t lots,
                                  uint32_t buildings, float timeMs) {
    ::rc_db_push_generation_stats(serial, roads, districts, lots, buildings,
                                  timeMs);
  }

  /** @brief Clear all generated entity rows (roads, districts, lots,
   * buildings). */
  static void ClearGenerated() { ::rc_db_clear_generated(); }

#ifdef RC_FEATURE_GUIDE_DB_SYNC
  // === Divider Guide Sync (enable with -DRC_FEATURE_GUIDE_DB_SYNC=ON) ===
  static void PushDivGuide(int id, bool horizontal, double world_pos,
                           const char *label, bool enabled) {
    ::rc_db_push_div_guide(id, horizontal, world_pos, label, enabled);
  }
  static void RemoveDivGuide(int id) { ::rc_db_remove_div_guide(id); }
  static void ClearDivGuides()       { ::rc_db_clear_div_guides(); }
#endif
};

} // namespace RogueCity::Core::Database
