// FILE: MockupWatcher.cpp
// PURPOSE: Poll-based file watcher — no threads, no OS APIs.

#include "RogueCity/Visualizer/MockupWatcher.hpp"

namespace RogueCity::Visualizer {

void MockupWatcher::SetFile(const std::string& path) {
    m_path           = path;
    m_elapsed        = 0.0f;
    m_last_write_time = {};
    m_force_reload   = true; // trigger immediate load on next Update()
}

void MockupWatcher::Update(float dt) {
    if (m_path.empty() || !m_callback) return;

    // Forced reload: fire immediately, reset state
    if (m_force_reload) {
        m_force_reload = false;
        m_elapsed      = 0.0f;
        try {
            m_last_write_time = std::filesystem::last_write_time(m_path);
        } catch (...) {
            // File doesn't exist yet — silent
        }
        m_callback();
        return;
    }

    m_elapsed += dt;
    if (m_elapsed < m_poll_interval) return;
    m_elapsed = 0.0f;

    // Check modification time
    try {
        const auto mtime = std::filesystem::last_write_time(m_path);
        if (mtime != m_last_write_time) {
            m_last_write_time = mtime;
            m_callback();
        }
    } catch (...) {
        // File not found — silent fail, keep watching
    }
}

} // namespace RogueCity::Visualizer
