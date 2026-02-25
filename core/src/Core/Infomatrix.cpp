 /**
 * @class Infomatrix
 * @brief Manages a fixed-size circular buffer of InfomatrixEvent objects for event logging.
 *
 * The Infomatrix class provides functionality to store, retrieve, and manage a history of events
 * within a circular buffer. It supports pushing new events, reconstructing the linear order of events,
 * and viewing the event history.
 *
 * - @constructor Infomatrix() Initializes the event buffer and sets up internal counters.
 * - @method InfomatrixEventView events() const
 *   Returns a view of the current events in linear order, reconstructing from the circular buffer.
 * - @method void pushEvent(InfomatrixEvent::Category cat, std::string msg)
 *   Adds a new event to the buffer with the specified category and message, timestamped with the current time.
 *
 * @note The buffer overwrites the oldest events when full, maintaining a maximum of kMaxEvents.
 */
 
#include "RogueCity/Core/Infomatrix.hpp"
#include <algorithm>
#include <chrono>

namespace RogueCity::Core::Editor {

Infomatrix::Infomatrix() {
    m_events.resize(kMaxEvents);
}

InfomatrixEventView Infomatrix::events() const {
    InfomatrixEventView view;
    view.data.reserve(m_event_count);
    
    // Reconstruct linear order from circular buffer
    if (m_event_count < kMaxEvents) {
        for (size_t i = 0; i < m_event_head; ++i) {
            view.data.push_back(m_events[i]);
        }
    } else {
        for (size_t i = 0; i < kMaxEvents; ++i) {
            view.data.push_back(m_events[(m_event_head + i) % kMaxEvents]);
        }
    }
    
    view.offset = 0;
    return view;
}

void Infomatrix::pushEvent(InfomatrixEvent::Category cat, std::string msg) {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    double seconds = std::chrono::duration<double>(duration).count();

    m_events[m_event_head] = InfomatrixEvent{cat, std::move(msg), seconds};
    m_event_head = (m_event_head + 1) % kMaxEvents;
    m_event_count = std::min(m_event_count + 1, kMaxEvents);
}

} // namespace RogueCity::Core::Editor
