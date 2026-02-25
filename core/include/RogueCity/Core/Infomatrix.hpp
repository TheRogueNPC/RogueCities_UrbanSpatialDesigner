// FILE: RogueCity/Core/Infomatrix.hpp
// PURPOSE: Centralized event logging, telemetry, and validation for the city generator.
// WHY: Decouples UI logging from data generation; UI becomes a consumer of Infomatrix events.
// WHERE: New file

#pragma once

#include <vector>
#include <string>
#include <cstddef>

namespace RogueCity::Core::Editor {

struct InfomatrixEvent {
    enum class Category {
        Runtime,     // Generation events
        Validation,  // Validation events
        Dirty,       // Dirty state change events
        Telemetry    // Telemetry events
    } cat;
    std::string msg;
    double time;
};

// Simple ring-buffer view.
struct InfomatrixEventView {
    using iterator = const InfomatrixEvent*;
    iterator begin() const { return data.data(); }
    iterator end() const { return data.data() + data.size(); }
    
    std::vector<InfomatrixEvent> data;
    size_t offset = 0;
};

class Infomatrix {
public:
    Infomatrix();

    // Read-only view of events.
    InfomatrixEventView events() const;

    // Push an event to the Infomatrix.
    void pushEvent(InfomatrixEvent::Category cat, std::string msg);

private:
    static constexpr size_t kMaxEvents = 220u;
    std::vector<InfomatrixEvent> m_events;
    size_t m_event_head = 0;
    size_t m_event_count = 0;
};

} // namespace RogueCity::Core::Editor
