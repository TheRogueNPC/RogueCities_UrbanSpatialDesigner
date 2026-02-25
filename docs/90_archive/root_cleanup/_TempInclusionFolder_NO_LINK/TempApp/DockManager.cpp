#include "DockManager.hpp"

#include <algorithm>
#include <imgui.h>

namespace RCG
{
    void DockManager::clear()
    {
        docks.clear();
        dock_index.clear();
    }

    void DockManager::register_dock(const DockSpec &spec)
    {
        if (spec.id.empty() || spec.visible == nullptr)
        {
            return;
        }
        if (dock_index.find(spec.id) != dock_index.end())
        {
            docks[dock_index[spec.id]] = spec;
            return;
        }
        dock_index[spec.id] = docks.size();
        docks.push_back(spec);
    }

    void DockManager::render_visible()
    {
        for (const auto &dock : docks)
        {
            const bool locked = dock.locked && *dock.locked;
            if ((dock.always_visible || locked || (dock.visible && *dock.visible)) && dock.render)
            {
                if (dock.visible && (dock.always_visible || locked))
                {
                    *dock.visible = true;
                }
                dock.render();
            }
        }
    }

    void DockManager::set_all_visible(bool visible)
    {
        for (auto &dock : docks)
        {
            if (dock.visible)
            {
                if (dock.always_visible || (dock.locked && *dock.locked))
                {
                    *dock.visible = true;
                }
                else
                {
                    *dock.visible = visible;
                }
            }
        }
    }

    void DockManager::reset_visibility()
    {
        for (auto &dock : docks)
        {
            if (dock.visible)
            {
                if (dock.always_visible || (dock.locked && *dock.locked))
                {
                    *dock.visible = true;
                }
                else
                {
                    *dock.visible = dock.default_visible;
                }
            }
        }
    }

    void DockManager::draw_view_menu()
    {
        if (ImGui::MenuItem("Show All Docks"))
        {
            set_all_visible(true);
        }
        if (ImGui::MenuItem("Hide All Docks"))
        {
            set_all_visible(false);
        }
        if (ImGui::MenuItem("Reset Dock Visibility"))
        {
            reset_visibility();
        }

        ImGui::Separator();

        std::string current_group;
        auto docks_sorted = docks;
        std::sort(docks_sorted.begin(), docks_sorted.end(),
                  [](const DockSpec &a, const DockSpec &b)
                  {
                      if (a.group == b.group)
                          return a.label < b.label;
                      return a.group < b.group;
                  });

        for (auto &dock : docks_sorted)
        {
            if (dock.group != current_group)
            {
                current_group = dock.group;
                if (!current_group.empty())
                {
                    ImGui::Separator();
                    ImGui::TextDisabled("%s", current_group.c_str());
                }
            }

            bool visible = dock.visible ? *dock.visible : false;
            const bool locked = dock.locked && *dock.locked;
            std::string label = dock.label;
            if (dock.always_visible)
            {
                label += " (Always On)";
            }
            else if (locked)
            {
                label += " (Locked)";
            }
            if (dock.always_visible || locked)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::MenuItem(label.c_str(), nullptr, visible))
            {
                if (dock.visible)
                {
                    *dock.visible = !visible;
                }
            }
            if (dock.always_visible || locked)
            {
                ImGui::EndDisabled();
            }
        }
    }
} // namespace RCG
