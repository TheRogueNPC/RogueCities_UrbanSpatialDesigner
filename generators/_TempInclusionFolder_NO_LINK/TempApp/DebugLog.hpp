#pragma once

#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>

namespace RCG
{
    namespace DebugLog
    {
        inline bool enabled = false;
        inline std::ofstream log_file;
        inline std::string log_path;
        inline std::function<void(const std::string &)> sink;

        inline void set_enabled(bool value)
        {
            enabled = value;
            if (!enabled && log_file.is_open())
            {
                log_file.close();
            }
        }

        inline void set_log_file(const std::string &path)
        {
            log_path = path;
            if (!enabled)
            {
                return;
            }
            if (log_file.is_open())
            {
                log_file.close();
            }
            if (!log_path.empty())
            {
                std::filesystem::path fs_path(log_path);
                if (fs_path.has_parent_path())
                {
                    std::error_code ec;
                    std::filesystem::create_directories(fs_path.parent_path(), ec);
                }
                log_file.open(log_path, std::ios::app);
                if (!log_file.is_open())
                {
                    std::fprintf(stderr, "[DebugLog] Failed to open log file: %s\n", log_path.c_str());
                }
            }
        }

        inline void set_sink(std::function<void(const std::string &)> callback)
        {
            sink = std::move(callback);
        }

        inline bool is_enabled()
        {
            return enabled;
        }

        inline void printf(const char *fmt, ...)
        {
            if (!enabled || !fmt)
            {
                return;
            }
            char buffer[4096];
            va_list args;
            va_start(args, fmt);
            const int count = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
            va_end(args);
            if (count <= 0)
            {
                return;
            }
            std::fputs(buffer, stderr);
            if (log_file.is_open())
            {
                log_file.write(buffer, static_cast<std::streamsize>(count));
                log_file.flush();
            }
            if (sink)
            {
                sink(std::string(buffer, static_cast<std::size_t>(count)));
            }
        }
    } // namespace DebugLog
} // namespace RCG
