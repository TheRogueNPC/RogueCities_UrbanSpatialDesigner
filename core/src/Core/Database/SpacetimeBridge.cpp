#include "RogueCity/Core/Database/SpacetimeBridge.hpp"
#include "RogueCity/Core/Database/SpacetimeClient.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp" // For EditorAxiom
#include <mutex>
#include <thread>
#include <atomic>
#include <queue>
#include <chrono>

namespace RogueCity::Core::Database {

struct SyncTask {
    enum class Type { Connect, Disconnect, SyncRoads, SyncDistricts, SyncAxioms };
    Type type;
    BridgeConfig config;
    std::vector<Road> roads;
    std::vector<District> districts;
    std::vector<Editor::EditorAxiom> axioms;
};

struct SpacetimeBridge::Impl {
    std::mutex mutex_;
    std::queue<SyncTask> task_queue_;
    std::thread worker_thread_;
    std::atomic<bool> running_{false};
    
    std::atomic<BridgeStatus> status_{BridgeStatus::Disconnected};
    BridgeStats stats_{};
    std::string last_error_{};
    uint64_t current_serial_{0};

    Impl() {
        running_ = true;
        worker_thread_ = std::thread(&Impl::WorkerLoop, this);
    }

    ~Impl() {
        running_ = false;
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        SpacetimeClient::Disconnect();
    }

    void EnqueueTask(SyncTask task) {
        std::lock_guard<std::mutex> lock(mutex_);
        task_queue_.push(std::move(task));
    }

    void WorkerLoop() {
        while (running_) {
            SyncTask task;
            bool has_task = false;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!task_queue_.empty()) {
                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                    has_task = true;
                }
            }

            if (!has_task) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            auto start_time = std::chrono::high_resolution_clock::now();

            try {
                switch (task.type) {
                    case SyncTask::Type::Connect:
                        status_ = BridgeStatus::Connecting;
                        if (SpacetimeClient::Connect(task.config.db_name, "http://127.0.0.1:3000")) {
                            status_ = BridgeStatus::Connected;
                            last_error_ = "";
                        } else {
                            status_ = BridgeStatus::Error;
                            last_error_ = "Failed to connect to SpacetimeDB.";
                        }
                        break;
                        
                    case SyncTask::Type::Disconnect:
                        SpacetimeClient::Disconnect();
                        status_ = BridgeStatus::Disconnected;
                        break;
                        
                    case SyncTask::Type::SyncAxioms: {
                        if (status_ != BridgeStatus::Connected) break;
                        status_ = BridgeStatus::Syncing;
                        SpacetimeClient::ClearAxioms();
                        for (const auto& ax : task.axioms) {
                            SpacetimeClient::PlaceAxiom(static_cast<uint32_t>(ax.type), ax.position.x, ax.position.y, ax.radius, ax.theta, ax.decay);
                        }
                        stats_.total_axioms_synced += task.axioms.size();
                        status_ = BridgeStatus::Connected;
                        break;
                    }
                        
                    case SyncTask::Type::SyncRoads: {
                        if (status_ != BridgeStatus::Connected) break;
                        status_ = BridgeStatus::Syncing;
                        current_serial_++;
                        for (const auto& r : task.roads) {
                            std::vector<double> flat_pts;
                            flat_pts.reserve(r.points.size() * 2);
                            for (const auto& p : r.points) {
                                flat_pts.push_back(p.x);
                                flat_pts.push_back(p.y);
                            }
                            SpacetimeClient::PushRoad(r.id, static_cast<uint32_t>(r.type), r.source_axiom_id, flat_pts.data(), static_cast<uint32_t>(r.points.size()), r.layer_id, r.has_grade_separation, r.contains_signal, r.contains_crosswalk, current_serial_);
                        }
                        stats_.total_roads_synced += task.roads.size();
                        status_ = BridgeStatus::Connected;
                        break;
                    }

                    case SyncTask::Type::SyncDistricts: {
                        if (status_ != BridgeStatus::Connected) break;
                        status_ = BridgeStatus::Syncing;
                        for (const auto& d : task.districts) {
                            std::vector<double> flat_pts;
                            flat_pts.reserve(d.border.size() * 2);
                            for (const auto& p : d.border) {
                                flat_pts.push_back(p.x);
                                flat_pts.push_back(p.y);
                            }
                            SpacetimeClient::PushDistrict(d.id, static_cast<uint32_t>(d.type), d.primary_axiom_id, flat_pts.data(), static_cast<uint32_t>(d.border.size()), d.budget_allocated, d.projected_population, d.desirability, d.average_elevation, current_serial_);
                        }
                        stats_.total_districts_synced += task.districts.size();
                        status_ = BridgeStatus::Connected;
                        break;
                    }
                }
            } catch (const std::exception& e) {
                status_ = BridgeStatus::Error;
                last_error_ = e.what();
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            stats_.last_sync_duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        }
    }
};

SpacetimeBridge& SpacetimeBridge::Instance() {
    static SpacetimeBridge instance;
    return instance;
}

SpacetimeBridge::SpacetimeBridge() : impl_(std::make_unique<Impl>()) {}

SpacetimeBridge::~SpacetimeBridge() = default;

void SpacetimeBridge::Connect(const BridgeConfig& config) {
    SyncTask task;
    task.type = SyncTask::Type::Connect;
    task.config = config;
    impl_->EnqueueTask(std::move(task));
}

void SpacetimeBridge::Disconnect() {
    SyncTask task;
    task.type = SyncTask::Type::Disconnect;
    impl_->EnqueueTask(std::move(task));
}

void SpacetimeBridge::Update(float /*dt*/) {
    // In a fuller implementation, this could poll for completed events,
    // invoke callbacks on the main thread, or check for reconnection.
    impl_->stats_.last_sync_frame++;
}

void SpacetimeBridge::SyncRoads(const std::vector<Road>& roads) {
    SyncTask task;
    task.type = SyncTask::Type::SyncRoads;
    task.roads = roads;
    impl_->EnqueueTask(std::move(task));
}

void SpacetimeBridge::SyncDistricts(const std::vector<District>& districts) {
    SyncTask task;
    task.type = SyncTask::Type::SyncDistricts;
    task.districts = districts;
    impl_->EnqueueTask(std::move(task));
}

void SpacetimeBridge::SyncAxioms(const std::vector<Editor::EditorAxiom>& axioms) {
    SyncTask task;
    task.type = SyncTask::Type::SyncAxioms;
    task.axioms = axioms;
    impl_->EnqueueTask(std::move(task));
}

BridgeStatus SpacetimeBridge::GetStatus() const {
    return impl_->status_.load();
}

bool SpacetimeBridge::IsConnected() const {
    return impl_->status_.load() == BridgeStatus::Connected || impl_->status_.load() == BridgeStatus::Syncing;
}

const BridgeStats& SpacetimeBridge::GetStats() const {
    return impl_->stats_; // Note: Reading this concurrently might be slightly racey but generally fine for stats.
}

std::string SpacetimeBridge::GetLastError() const {
    return impl_->last_error_;
}

} // namespace RogueCity::Core::Database