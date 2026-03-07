#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include <memory>
#include <string>
#include <vector>

namespace RogueCity::Core::Editor {
    struct EditorAxiom;
}

namespace RogueCity::Core::Database {

    enum class BridgeStatus {
        Disconnected,
        Connecting,
        Connected,
        Syncing,
        Error
    };

    struct BridgeStats {
        size_t total_roads_synced = 0;
        size_t total_districts_synced = 0;
        size_t total_axioms_synced = 0;
        double last_sync_duration_ms = 0.0;
        uint64_t last_sync_frame = 0;
        uint32_t reconnection_attempts = 0;
    };

    struct BridgeConfig {
        std::string db_name;
        float sync_interval_seconds = 0.5f; // Throttling
        uint32_t max_reconnect_attempts = 5;
        bool auto_reconnect = true;
    };

    // Cold Logic: The bridge should be an asynchronous observer.
    // It should not block the main HFSM loop.
    class SpacetimeBridge {
    public:
        static SpacetimeBridge& Instance();

        // Lifecycle management
        void Connect(const BridgeConfig& config);
        void Disconnect();

        // Main loop hook to process background results and status updates
        // Called by EditorHFSM::Update
        void Update(float dt);
        
        // Non-blocking sync points called by the HFSM during "Systems-Step"
        // These methods queue data and return immediately.
        void SyncRoads(const std::vector<Road>& roads);
        void SyncDistricts(const std::vector<District>& districts);
        void SyncAxioms(const std::vector<Editor::EditorAxiom>& axioms);

        // Status & Diagnostics
        BridgeStatus GetStatus() const;
        bool IsConnected() const;
        const BridgeStats& GetStats() const;
        std::string GetLastError() const;

    private:
        SpacetimeBridge();
        ~SpacetimeBridge();

        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace RogueCity::Core::Database
