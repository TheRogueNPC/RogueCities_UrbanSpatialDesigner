#include "RogueCity/App/Integration/DatabaseComputeWorker.hpp"
#include "RogueCity/Core/Database/SpacetimeClient.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <chrono>
#include <iostream>
#include <vector>

namespace RogueCity::App::Integration {

using namespace RogueCity::Core;
using DB = Database::SpacetimeClient;

DatabaseComputeWorker::DatabaseComputeWorker(
    Core::Editor::GlobalState &global_state, std::string_view module_name,
    std::string_view server_url)
    : global_state_(global_state), module_name_(module_name),
      server_url_(server_url) {}

DatabaseComputeWorker::~DatabaseComputeWorker() { Stop(); }

void DatabaseComputeWorker::Start() {
  if (is_running_)
    return;

  std::cout << "[DatabaseComputeWorker] Connecting to SpacetimeDB @ "
            << server_url_ << "\n";
  bool connected = DB::Connect(module_name_, server_url_);
  if (!connected) {
    std::cerr << "[DatabaseComputeWorker] WARNING: Failed to establish "
                 "SpacetimeDB connection. Operating in offline mode.\n";
  }

  is_running_ = true;
  compute_thread_ =
      std::make_unique<std::thread>(&DatabaseComputeWorker::ComputeLoop, this);
}

void DatabaseComputeWorker::Stop() {
  if (!is_running_)
    return;

  is_running_ = false;
  if (compute_thread_ && compute_thread_->joinable()) {
    compute_thread_->join();
  }

  DB::Disconnect();
  std::cout << "[DatabaseComputeWorker] Disconnected from SpacetimeDB.\n";
}

void DatabaseComputeWorker::OnAxiomsChanged() {
  generator_dirty_flag_.store(true, std::memory_order_release);
}

void DatabaseComputeWorker::ComputeLoop() {
  std::cout << "[DatabaseComputeWorker] Compute loop started.\n";

  auto last_push_time = std::chrono::steady_clock::now();

  while (is_running_) {
    if (generator_dirty_flag_.exchange(false, std::memory_order_acquire)) {
      // Debounce: skip if we pushed too recently (rapid axiom edits)
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - last_push_time)
                         .count();
      if (elapsed < min_push_interval_ms) {
        // Re-set dirty so we catch it next iteration after the interval
        generator_dirty_flag_.store(true, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(
            min_push_interval_ms - static_cast<uint32_t>(elapsed)));
        continue;
      }

      RunGeneratorPass();
      last_push_time = std::chrono::steady_clock::now();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

void DatabaseComputeWorker::RunGeneratorPass() {
  namespace Gen = RogueCity::Generators;

  const uint64_t serial =
      generation_serial_.fetch_add(1, std::memory_order_relaxed) + 1;

  std::cout << "[DatabaseComputeWorker] Generation pass #" << serial
            << " starting...\n";

  auto pass_start = std::chrono::steady_clock::now();

  // ── 1. Gather axiom inputs ──────────────────────────────────────────
  std::vector<Gen::CityGenerator::AxiomInput> inputs;
  for (const auto &axiom : global_state_.axioms) {
    Gen::CityGenerator::AxiomInput inp;
    inp.id = static_cast<int>(axiom.id);
    inp.type = static_cast<Gen::CityGenerator::AxiomInput::Type>(axiom.type);
    inp.position = axiom.position;
    inp.radius = axiom.radius;
    inp.theta = axiom.theta;
    inp.decay = axiom.decay;
    inputs.push_back(std::move(inp));
  }

  if (inputs.empty()) {
    std::cout
        << "[DatabaseComputeWorker] No axioms. Clearing generated data.\n";
    DB::ClearGenerated();
    DB::PushGenerationStats(serial, 0, 0, 0, 0, 0.0f);
    return;
  }

  // ── 2. Run generator ────────────────────────────────────────────────
  Gen::CityGenerator generator;
  Gen::CityGenerator::Config config;
  // Use default config — in future, pull from GlobalState or DB.
  auto output = generator.generate(inputs, config);

  auto gen_end = std::chrono::steady_clock::now();
  float gen_time_ms =
      std::chrono::duration<float, std::milli>(gen_end - pass_start).count();

  std::cout << "[DatabaseComputeWorker] Generation complete in " << gen_time_ms
            << "ms. Roads=" << output.roads.size()
            << " Districts=" << output.districts.size()
            << " Lots=" << output.lots.size()
            << " Buildings=" << output.buildings.size() << "\n";

  // ── 3. Clear stale generated data ───────────────────────────────────
  DB::ClearGenerated();

  // ── 4. Push roads ───────────────────────────────────────────────────
  uint32_t road_count = 0;
  for (const auto &road : output.roads) {
    // Flatten Vec2 points to interleaved [x0,y0,x1,y1,...]
    std::vector<double> flat_pts;
    flat_pts.reserve(road.points.size() * 2);
    for (const auto &p : road.points) {
      flat_pts.push_back(p.x);
      flat_pts.push_back(p.y);
    }

    DB::PushRoad(road.id, static_cast<uint32_t>(road.type),
                 road.source_axiom_id, flat_pts.data(),
                 static_cast<uint32_t>(road.points.size()), road.layer_id,
                 road.has_grade_separation, road.contains_signal,
                 road.contains_crosswalk, serial);

    ++road_count;
    if (batch_log_threshold > 0 && road_count % batch_log_threshold == 0) {
      std::cout << "[DatabaseComputeWorker]   Pushed " << road_count
                << " roads...\n";
    }
  }

  // ── 5. Push districts ───────────────────────────────────────────────
  uint32_t dist_count = 0;
  for (const auto &dist : output.districts) {
    std::vector<double> flat_border;
    flat_border.reserve(dist.border.size() * 2);
    for (const auto &p : dist.border) {
      flat_border.push_back(p.x);
      flat_border.push_back(p.y);
    }

    DB::PushDistrict(dist.id, static_cast<uint32_t>(dist.type),
                     dist.primary_axiom_id, flat_border.data(),
                     static_cast<uint32_t>(dist.border.size()),
                     dist.budget_allocated, dist.projected_population,
                     dist.desirability, dist.average_elevation, serial);

    ++dist_count;
  }

  // ── 6. Push lots ────────────────────────────────────────────────────
  uint32_t lot_count = 0;
  for (const auto &lot : output.lots) {
    std::vector<double> flat_boundary;
    flat_boundary.reserve(lot.boundary.size() * 2);
    for (const auto &p : lot.boundary) {
      flat_boundary.push_back(p.x);
      flat_boundary.push_back(p.y);
    }

    DB::PushLot(lot.id, lot.district_id, static_cast<uint32_t>(lot.lot_type),
                lot.centroid.x, lot.centroid.y, flat_boundary.data(),
                static_cast<uint32_t>(lot.boundary.size()), lot.area,
                lot.access, lot.exposure, lot.serviceability, lot.privacy,
                serial);

    ++lot_count;
    if (batch_log_threshold > 0 && lot_count % batch_log_threshold == 0) {
      std::cout << "[DatabaseComputeWorker]   Pushed " << lot_count
                << " lots...\n";
    }
  }

  // ── 7. Push buildings ───────────────────────────────────────────────
  uint32_t bldg_count = 0;
  for (const auto &bldg : output.buildings) {
    DB::PushBuilding(bldg.id, bldg.lot_id, bldg.district_id,
                     static_cast<uint32_t>(bldg.type), bldg.position.x,
                     bldg.position.y, bldg.rotation_radians,
                     bldg.footprint_area, bldg.suggested_height,
                     bldg.estimated_cost, serial);

    ++bldg_count;
    if (batch_log_threshold > 0 && bldg_count % batch_log_threshold == 0) {
      std::cout << "[DatabaseComputeWorker]   Pushed " << bldg_count
                << " buildings...\n";
    }
  }

  // ── 8. Push generation stats ────────────────────────────────────────
  auto push_end = std::chrono::steady_clock::now();
  float total_time_ms =
      std::chrono::duration<float, std::milli>(push_end - pass_start).count();

  DB::PushGenerationStats(serial, road_count, dist_count, lot_count, bldg_count,
                          total_time_ms);

  std::cout << "[DatabaseComputeWorker] Pass #" << serial << " complete. "
            << "Total time: " << total_time_ms << "ms "
            << "(gen: " << gen_time_ms << "ms, "
            << "push: " << (total_time_ms - gen_time_ms) << "ms)\n";
}

} // namespace RogueCity::App::Integration
