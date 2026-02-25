#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace RCG
{
    class DockManager
    {
    public:
        struct DockSpec
        {
            std::string id;
            std::string label;
            std::string group;
            bool default_visible{true};
            bool *visible{nullptr};
            bool *locked{nullptr};
            bool always_visible{false};
            std::function<void()> render;
        };

        void clear();
        void register_dock(const DockSpec &spec);
        void render_visible();
        void draw_view_menu();
        void set_all_visible(bool visible);
        void reset_visibility();

    private:
        std::vector<DockSpec> docks;
        std::unordered_map<std::string, std::size_t> dock_index;
    };
} // namespace RCG
