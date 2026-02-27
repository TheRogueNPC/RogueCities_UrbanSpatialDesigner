// FILE: MockupWatcher.hpp
// PURPOSE: Poll-based file watcher using std::filesystem::last_write_time only.
//
// Constraints (by design):
//   - No threads, no OS-specific APIs (no inotify, no ReadDirectoryChangesW).
//   - No new vcpkg dependencies.
//   - Call Update(dt) once per frame; callback fires when the watched file changes.
//   - Silent fail if the file does not exist.

#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace RogueCity::Visualizer {

class MockupWatcher {
public:
    using ChangeCallback = std::function<void()>;

    MockupWatcher() = default;

    /// Set (or replace) the file path to watch.
    /// Resets elapsed time and triggers an immediate reload on the next Update().
    void SetFile(const std::string& path);

    /// Set the callback invoked when a change is detected (or on Invalidate()).
    void SetCallback(ChangeCallback cb) { m_callback = std::move(cb); }

    /// Set the polling interval in seconds (default: 1.0 s).
    void SetPollInterval(float seconds) { m_poll_interval = seconds; }

    /// Accumulate dt and check for file changes. Call once per frame.
    void Update(float dt);

    /// Force the callback to fire on the next Update() regardless of mtime.
    void Invalidate() { m_force_reload = true; }

    bool        IsWatching() const { return !m_path.empty(); }
    const std::string& GetPath() const { return m_path; }

private:
    std::string       m_path;
    ChangeCallback    m_callback;
    float             m_poll_interval = 1.0f;
    float             m_elapsed       = 0.0f;
    bool              m_force_reload  = false;
    std::filesystem::file_time_type m_last_write_time{};
};

} // namespace RogueCity::Visualizer
