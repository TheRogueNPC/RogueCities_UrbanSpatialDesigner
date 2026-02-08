#include "Windows.hpp"
#include "ImGuiCompat.hpp"
#include "Icons.hpp"
#include "DebugFlags.hpp"
#include "DebugLog.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace RCG
{
    namespace
    {
        float dist_sq(ImVec2 a, ImVec2 b);

        void help_tooltip(const char *text)
        {
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", text);
            }
        }

        const char *axiom_type_label_ui(int type)
        {
            switch (type)
            {
            case 0:
                return "Radial (Blue Circle)";
            case 1:
                return "Delta (Green Triangle)";
            case 2:
                return "Block (Hollow Square)";
            case 3:
            default:
                return "Grid Corrective";
            }
        }

        const char *river_type_label_ui(int type)
        {
            switch (type)
            {
            case 0:
                return "Meandering";
            case 1:
                return "Channelized";
            case 2:
                return "Braided";
            case 3:
            default:
                return "Anabranching";
            }
        }

        const char *district_type_label(CityModel::DistrictType type)
        {
            switch (type)
            {
            case CityModel::DistrictType::Mixed:
                return "Mixed";
            case CityModel::DistrictType::Residential:
                return "Residential";
            case CityModel::DistrictType::Commercial:
                return "Commercial";
            case CityModel::DistrictType::Civic:
                return "Civic";
            case CityModel::DistrictType::Industrial:
                return "Industrial";
            default:
                return "Mixed";
            }
        }

        const char *lot_type_label(CityModel::LotType type)
        {
            switch (type)
            {
            case CityModel::LotType::None:
                return "None";
            case CityModel::LotType::Residential:
                return "Residential";
            case CityModel::LotType::RowhomeCompact:
                return "Rowhome Compact";
            case CityModel::LotType::RetailStrip:
                return "Retail Strip";
            case CityModel::LotType::MixedUse:
                return "Mixed Use";
            case CityModel::LotType::LogisticsIndustrial:
                return "Logistics Industrial";
            case CityModel::LotType::CivicCultural:
                return "Civic Cultural";
            case CityModel::LotType::LuxuryScenic:
                return "Luxury Scenic";
            case CityModel::LotType::BufferStrip:
                return "Buffer Strip";
            default:
                return "Unknown";
            }
        }

        const char *building_type_label(CityModel::BuildingType type)
        {
            switch (type)
            {
            case CityModel::BuildingType::None:
                return "None";
            case CityModel::BuildingType::Residential:
                return "Residential";
            case CityModel::BuildingType::Rowhome:
                return "Rowhome";
            case CityModel::BuildingType::Retail:
                return "Retail";
            case CityModel::BuildingType::MixedUse:
                return "Mixed Use";
            case CityModel::BuildingType::Industrial:
                return "Industrial";
            case CityModel::BuildingType::Civic:
                return "Civic";
            case CityModel::BuildingType::Luxury:
                return "Luxury";
            case CityModel::BuildingType::Utility:
                return "Utility";
            default:
                return "Unknown";
            }
        }

        std::string district_name(const CityModel::District &district)
        {
            std::string name = "A" + std::to_string(district.primary_axiom_id);
            if (district.secondary_axiom_id >= 0)
            {
                name += "+A" + std::to_string(district.secondary_axiom_id);
            }
            return name;
        }

        std::string axiom_display_name(const ViewportWindow::AxiomData &axiom)
        {
            if (axiom.tag_id > 0)
            {
                return axiom.name + " #" + std::to_string(axiom.tag_id);
            }
            return axiom.name;
        }

        const ViewportWindow::AxiomData *find_axiom_by_id(const std::vector<ViewportWindow::AxiomData> &axioms, int id)
        {
            for (const auto &axiom : axioms)
            {
                if (axiom.id == id)
                {
                    return &axiom;
                }
            }
            return nullptr;
        }

        std::string district_label(const CityModel::District &district,
                                   const std::vector<ViewportWindow::AxiomData> &axioms)
        {
            auto label_for = [&](int id)
            {
                if (id < 0)
                {
                    return std::string("None");
                }
                const auto *ax = find_axiom_by_id(axioms, id);
                if (ax)
                {
                    return axiom_display_name(*ax);
                }
                return std::string("A") + std::to_string(id);
            };

            std::string name = label_for(district.primary_axiom_id);
            if (district.secondary_axiom_id >= 0)
            {
                name += "+" + label_for(district.secondary_axiom_id);
            }
            return name;
        }

        void render_tab_visibility(const char *id,
                                   UserPreferences::AppPrefs::IndexTabs &tabs,
                                   bool allow_details,
                                   bool allow_settings)
        {
            ImGui::PushID(id);
            ImGui::Separator();
            ImGui::Text("Tabs");
            ImGui::Checkbox("List", &tabs.show_list);
            if (allow_details)
            {
                ImGui::SameLine();
                ImGui::Checkbox("Details", &tabs.show_details);
            }
            if (allow_settings)
            {
                ImGui::SameLine();
                ImGui::Checkbox("Settings", &tabs.show_settings);
            }
            if (!tabs.show_list && !tabs.show_details && !tabs.show_settings)
            {
                tabs.show_list = true;
            }
            ImGui::PopID();
        }

        uint64_t hash_combine(uint64_t seed, uint64_t value)
        {
            seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
            return seed;
        }

        std::string building_key(uint32_t district_id, uint32_t lot_id, uint32_t idx)
        {
            uint64_t h = 0;
            h = hash_combine(h, district_id);
            h = hash_combine(h, lot_id);
            h = hash_combine(h, idx);
            static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            std::string out(4, '0');
            for (int i = 3; i >= 0; --i)
            {
                out[i] = digits[h % 36];
                h /= 36;
            }
            return out;
        }

        float distance_to_segment(ImVec2 p, ImVec2 a, ImVec2 b)
        {
            float vx = b.x - a.x;
            float vy = b.y - a.y;
            float wx = p.x - a.x;
            float wy = p.y - a.y;
            float c1 = wx * vx + wy * vy;
            if (c1 <= 0.0f)
                return std::sqrt(dist_sq(p, a));
            float c2 = vx * vx + vy * vy;
            if (c2 <= c1)
                return std::sqrt(dist_sq(p, b));
            float t = c1 / c2;
            ImVec2 proj = ImVec2(a.x + t * vx, a.y + t * vy);
            return std::sqrt(dist_sq(p, proj));
        }

        struct NearestRoadInfo
        {
            bool found{false};
            CityModel::RoadType type{CityModel::RoadType::Street};
            uint32_t road_id{0};
            int endpoint_index{0};
            float distance{1e9f};
        };

        NearestRoadInfo find_nearest_road(const CityModel::City &city, ImVec2 pos, bool want_major)
        {
            NearestRoadInfo best;
            for (auto type : CityModel::generated_road_order)
            {
                const bool is_major = CityModel::is_major_group(type);
                if (is_major != want_major)
                {
                    continue;
                }
                const auto &segments = city.segment_roads_by_type[CityModel::road_type_index(type)];
                for (const auto &road : segments)
                {
                    if (road.points.size() < 2)
                    {
                        continue;
                    }
                    float best_dist = 1e9f;
                    for (std::size_t i = 0; i + 1 < road.points.size(); ++i)
                    {
                        ImVec2 a(static_cast<float>(road.points[i].x), static_cast<float>(road.points[i].y));
                        ImVec2 b(static_cast<float>(road.points[i + 1].x), static_cast<float>(road.points[i + 1].y));
                        best_dist = std::min(best_dist, distance_to_segment(pos, a, b));
                    }
                    if (best_dist < best.distance)
                    {
                        best.found = true;
                        best.distance = best_dist;
                        best.type = type;
                        best.road_id = road.id;
                        ImVec2 start(static_cast<float>(road.points.front().x), static_cast<float>(road.points.front().y));
                        ImVec2 end(static_cast<float>(road.points.back().x), static_cast<float>(road.points.back().y));
                        float d0 = std::sqrt(dist_sq(pos, start));
                        float d1 = std::sqrt(dist_sq(pos, end));
                        best.endpoint_index = (d1 < d0) ? static_cast<int>(road.points.size() - 1) : 0;
                    }
                }
            }
            return best;
        }

        // Slider with shift modifier for precision (halves speed when shift held)
        bool SliderFloatPrecision(const char *label, float *v, float v_min, float v_max, const char *format = "%.2f", ImGuiSliderFlags flags = 0)
        {
            float speed_mult = ImGui::GetIO().KeyShift ? 0.5f : 1.0f;
            float range = v_max - v_min;
            float power = (range > 100.0f) ? 1.0f : 0.5f;
            // Use DragFloat for precision control
            float drag_speed = (range / 200.0f) * speed_mult;
            return ImGui::DragFloat(label, v, drag_speed, v_min, v_max, format, flags);
        }

        bool SliderIntPrecision(const char *label, int *v, int v_min, int v_max)
        {
            float speed_mult = ImGui::GetIO().KeyShift ? 0.5f : 1.0f;
            float range = static_cast<float>(v_max - v_min);
            float drag_speed = std::max(0.1f, (range / 100.0f) * speed_mult);
            return ImGui::DragInt(label, v, drag_speed, v_min, v_max);
        }

        bool ComboAnchored(const char *label, int *current_item, const char *const items[], int items_count, int popup_max_height_in_items = -1)
        {
#if RCG_IMGUI_HAS_VIEWPORTS
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                if (ImGuiViewport *viewport = ImGui::GetWindowViewport())
                {
                    ImGui::SetNextWindowViewport(viewport->ID);
                }
            }
#endif
            return ImGui::Combo(label, current_item, items, items_count, popup_max_height_in_items);
        }

        void render_district_settings(UserPreferences &prefs, bool include_visibility)
        {
            auto &app = prefs.app();
            if (include_visibility)
            {
                ImGui::Checkbox("Show District Borders", &app.show_district_borders);
                help_tooltip("Toggle district border rendering in the viewport.");
                ImGui::Checkbox("Show Block Polygons", &app.show_block_polygons);
                help_tooltip("Render polygonized blocks/zones in the viewport.");
            }

            const char *grid_labels[] = {"64", "128", "256"};
            int grid_index = (app.district_grid_resolution <= 96)    ? 0
                             : (app.district_grid_resolution <= 192) ? 1
                                                                     : 2;
            ImGui::SetNextItemWidth(120.0f);
            if (ComboAnchored("Grid Resolution", &grid_index, grid_labels, 3))
            {
                app.district_grid_resolution = (grid_index == 0) ? 64 : (grid_index == 1) ? 128
                                                                                          : 256;
            }
            help_tooltip("Sampling resolution for district grid. Higher = smoother borders, slower generation.");

            ImGui::SliderFloat("Secondary Threshold", &app.district_secondary_threshold, 0.0f, 1.0f, "%.2f");
            help_tooltip("Max power-score gap to assign a secondary axiom. Higher = more paired districts.");

            ImGui::SliderFloat("Weight Scale", &app.district_weight_scale, 0.2f, 2.0f, "%.2f");
            help_tooltip("Scales axiom radius -> influence weight. Higher = larger axiom influence.");

            ImGui::Checkbox("RD Mode", &app.district_rd_mode);
            help_tooltip("Enable reaction-diffusion shaping for organic district borders.");
            if (app.district_rd_mode)
            {
                ImGui::SliderFloat("RD Mix", &app.district_rd_mix, 0.0f, 1.0f, "Noise: %.2f");
                help_tooltip("0 = jagged power-diagram borders, 1 = more organic smoothing.");
            }

            ImGui::Text("Desire Weights");
            ImGui::SliderFloat("Axiom Weight", &app.district_desire_weight_axiom, 0.0f, 2.0f, "%.2f");
            help_tooltip("Influence of axiom bias when choosing DistrictType (normalized).");
            ImGui::SliderFloat("Frontage Weight", &app.district_desire_weight_frontage, 0.0f, 2.0f, "%.2f");
            help_tooltip("Influence of frontage bias when choosing DistrictType (normalized).");
        }

        double polygon_area(const std::vector<CityModel::Vec2> &poly)
        {
            if (poly.size() < 3)
            {
                return 0.0;
            }
            std::size_t n = poly.size();
            if (n > 2 && poly.front().x == poly.back().x && poly.front().y == poly.back().y)
            {
                n -= 1;
            }
            double area = 0.0;
            for (std::size_t i = 0; i < n; ++i)
            {
                const auto &a = poly[i];
                const auto &b = poly[(i + 1) % n];
                area += (a.x * b.y - b.x * a.y);
            }
            return std::abs(area) * 0.5;
        }

        float dist_sq(ImVec2 a, ImVec2 b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            return dx * dx + dy * dy;
        }

        struct DockRequest
        {
            bool undock{false};
            bool dock_main{false};
        };

        static std::unordered_map<std::string, DockRequest> dock_requests;

        void queue_window_undock(const char *name)
        {
            auto &req = dock_requests[name];
            req.undock = true;
            req.dock_main = false;
        }

        void queue_window_dock_main(const char *name)
        {
            auto &req = dock_requests[name];
            req.dock_main = true;
            req.undock = false;
        }

        void apply_window_dock_request(const char *name)
        {
            auto it = dock_requests.find(name);
            if (it == dock_requests.end())
            {
                return;
            }
            if (it->second.undock)
            {
                ImGui::SetNextWindowDockID(0, ImGuiCond_Always);
            }
            if (it->second.dock_main)
            {
                ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_Always);
            }
            dock_requests.erase(it);
        }

        void render_window_context_menu(const char *name)
        {
#if RCG_IMGUI_HAS_VIEWPORTS
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                if (ImGuiViewport *viewport = ImGui::GetWindowViewport())
                {
                    ImGui::SetNextWindowViewport(viewport->ID);
                }
            }
#endif
            std::string popup_id = std::string("WindowContext##") + name;
            if (ImGui::BeginPopupContextWindow(popup_id.c_str(), ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
            {
                ImGui::TextDisabled("Window");
                ImGui::Separator();
                if (ImGui::MenuItem("Move to New Window"))
                {
                    queue_window_undock(name);
                }
                if (ImGui::MenuItem("Merge into Main Dock"))
                {
                    queue_window_dock_main(name);
                }
                ImGui::EndPopup();
            }
        }

        void render_window_lock_toggle(const char *name, bool &locked)
        {
            ImVec2 cursor = ImGui::GetCursorPos();
            ImVec2 window_size = ImGui::GetWindowSize();
            ImVec2 padding = ImGui::GetStyle().WindowPadding;
            float button_height = ImGui::GetFrameHeight();
            float button_width = 54.0f;
            ImGui::SetCursorPos(ImVec2(window_size.x - button_width - padding.x, padding.y));
            ImGui::PushID(name);
            if (ImGui::SmallButton(locked ? "Unlock" : "Lock"))
            {
                locked = !locked;
            }
            help_tooltip(locked ? "Window is locked open." : "Lock this window open.");
            ImGui::PopID();
            ImGui::SetCursorPos(cursor);
            ImGui::Dummy(ImVec2(0.0f, button_height + 2.0f));
        }

        bool segment_intersection(const ImVec2 &a, const ImVec2 &b,
                                  const ImVec2 &c, const ImVec2 &d,
                                  ImVec2 &out)
        {
            const float s1_x = b.x - a.x;
            const float s1_y = b.y - a.y;
            const float s2_x = d.x - c.x;
            const float s2_y = d.y - c.y;

            const float denom = (-s2_x * s1_y + s1_x * s2_y);
            if (std::fabs(denom) < 1e-6f)
                return false;
            const float s = (-s1_y * (a.x - c.x) + s1_x * (a.y - c.y)) / denom;
            const float t = (s2_x * (a.y - c.y) - s2_y * (a.x - c.x)) / denom;
            if (s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f)
            {
                out.x = a.x + (t * s1_x);
                out.y = a.y + (t * s1_y);
                return true;
            }
            return false;
        }

        void compute_road_intersections(RoadIndexEntry &target, const std::vector<RoadIndexEntry> &roads)
        {
            target.intersections.clear();
            target.intersections_computed = true;
            if (target.nodes.size() < 2)
            {
                return;
            }

            const float dedupe_radius_sq = 0.25f;
            for (const auto &other : roads)
            {
                if (other.id == target.id || other.nodes.size() < 2)
                {
                    continue;
                }
                for (size_t i = 0; i + 1 < target.nodes.size(); ++i)
                {
                    const ImVec2 a = target.nodes[i];
                    const ImVec2 b = target.nodes[i + 1];
                    for (size_t j = 0; j + 1 < other.nodes.size(); ++j)
                    {
                        const ImVec2 c = other.nodes[j];
                        const ImVec2 d = other.nodes[j + 1];
                        ImVec2 inter;
                        if (segment_intersection(a, b, c, d, inter))
                        {
                            bool duplicate = false;
                            for (const auto &existing : target.intersections)
                            {
                                if (existing.road_id == other.id &&
                                    dist_sq(ImVec2(existing.x, existing.y), inter) <= dedupe_radius_sq)
                                {
                                    duplicate = true;
                                    break;
                                }
                            }
                            if (!duplicate)
                            {
                                target.intersections.push_back(RoadIntersection{
                                    other.id,
                                    other.road_type,
                                    inter.x,
                                    inter.y});
                            }
                        }
                    }
                }
            }
        }
    } // namespace

    // ============ ToolsWindow ============

    ToolsWindow::ToolsWindow(UserPreferences &prefs) : user_prefs(prefs)
    {
    }

    ToolsWindow::~ToolsWindow()
    {
    }

    void ToolsWindow::render()
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("Tools");
        if (ImGui::Begin("Tools"))
        {
            render_window_context_menu("Tools");
            render_window_lock_toggle("Tools", user_prefs.windows().tools_locked);
            ImGui::Text("Generation Mode");
            const char *modes[] = {"Live", "Manual"};
            ImGui::SetNextItemWidth(-1);
            ComboAnchored("##road_mode", &user_prefs.app().generation_mode, modes, 2);
            help_tooltip("Live: regenerate on changes. Manual: regenerate only on button.");
            ImGui::SetNextItemWidth(-1);
            ComboAnchored("##river_mode", &user_prefs.app().river_generation_mode, modes, 2);
            help_tooltip("Live: update splines on edits. Manual: finalize on button.");

            ImGui::Separator();
            render_tool_buttons();
            ImGui::Separator();
            render_brush_controls();
            if (user_prefs.tools().selected_tool == 3)
            {
                ImGui::Separator();
                ImGui::Text("Axiom Type");
                const char *axiom_types[] = {"Radial (Blue Circle)", "Delta (Green Triangle)", "Block (Hollow Square)", "Grid Corrective"};
                ComboAnchored("##axiom_type", &user_prefs.tools().selected_axiom_type, axiom_types, 4);
                help_tooltip("Shape/color used for new axioms.");
                SliderFloatPrecision("Axiom Size", &user_prefs.tools().axiom_size, 2.0f, 30.0f);
                help_tooltip("Controls axiom influence radius. Hold Shift for precision.");
                user_prefs.tools().axiom_size = std::clamp(user_prefs.tools().axiom_size, 2.0f, 30.0f);
                ImGui::SliderFloat("Axiom Opacity", &user_prefs.tools().axiom_opacity, 0.05f, 1.0f);
                help_tooltip("Controls axiom influence strength.");
                if (user_prefs.tools().selected_axiom_type == 1)
                {
                    ImGui::Text("Delta Terminal");
                    const char *terminal_labels[] = {"Top", "Bottom Left", "Bottom Right"};
                    ComboAnchored("##delta_terminal", &user_prefs.tools().selected_delta_terminal, terminal_labels, 3);
                }
                if (user_prefs.tools().selected_axiom_type == 0)
                {
                    ImGui::Text("Radial Mode");
                    const char *radial_modes[] = {"Roundabout", "Center", "Both"};
                    ComboAnchored("##radial_mode", &user_prefs.tools().selected_radial_mode, radial_modes, 3);
                }
                if (user_prefs.tools().selected_axiom_type == 2)
                {
                    ImGui::Text("Block Mode");
                    const char *block_modes[] = {"Strict", "Free", "Corner"};
                    ComboAnchored("##block_mode", &user_prefs.tools().selected_block_mode, block_modes, 3);
                }
                ImGui::Separator();
                ImGui::Text(ICON_FA_LANDMARK " District Influencer");
                const char *influencer_types[] = {
                    ICON_FA_BAN " None",
                    ICON_DISTRICT_MARKET " Market",
                    ICON_DISTRICT_KEEP " Keep",
                    ICON_DISTRICT_TEMPLE " Temple",
                    ICON_DISTRICT_HARBOR " Harbor",
                    ICON_DISTRICT_PARK " Park",
                    ICON_DISTRICT_GATE " Gate",
                    ICON_DISTRICT_WELL " Well"};
                ComboAnchored("##influencer_type", &user_prefs.tools().selected_influencer_type, influencer_types, 8);
                help_tooltip("Landmark type that biases nearby district assignment.\nMarket=Commercial, Keep/Temple=Civic, Harbor=Industrial,\nPark=Residential/Luxury, Gate=Mixed, Well=Residential.");
                ImGui::Checkbox("Snap Axioms to Grid", &user_prefs.tools().grid_snap);
                help_tooltip("Aligns axiom placement to the grid.");
            }
            if (user_prefs.tools().selected_tool == 4)
            {
                ImGui::Separator();
                ImGui::Text("River Type");
                const char *river_types[] = {"Meandering", "Channelized", "Braided", "Anabranching"};
                ComboAnchored("##river_type", &user_prefs.tools().selected_river_type, river_types, 4);
                help_tooltip("Type used for new river splines.");
                ImGui::SliderFloat("River Width", &user_prefs.tools().river_width, 1.0f, 60.0f);
                help_tooltip("Controls river stroke width.");
                ImGui::SliderFloat("River Opacity", &user_prefs.tools().river_opacity, 0.05f, 1.0f);
                help_tooltip("Controls river opacity.");
                ImGui::Checkbox("Snap Rivers to Grid", &user_prefs.tools().grid_snap);
                help_tooltip("Aligns river control points to the grid.");
            }
            if (user_prefs.tools().selected_tool == 6)
            {
                ImGui::Separator();
                ImGui::Text("Road Edit");
                ImGui::TextDisabled("Drag curve handles to shape the path (Alt to break symmetry). Ctrl+Click a node to simplify handles. Alt+Click a node to delete.");
                ImGui::Checkbox("Snap to Grid", &user_prefs.app().road_snap_to_grid);
                help_tooltip("Snap road nodes to grid points while editing.");
                ImGui::Checkbox("Snap to Axioms", &user_prefs.app().road_snap_to_axioms);
                help_tooltip("Snap road nodes to axiom centers.");
                ImGui::Checkbox("Snap to Nodes", &user_prefs.app().road_dynamic_snap);
                help_tooltip("Snap road nodes to nearby road nodes.");
                ImGui::SliderFloat("Snap Radius", &user_prefs.app().road_snap_distance, 5.0f, 80.0f);
                help_tooltip("Distance threshold for snapping road nodes.");
            }
            if (user_prefs.tools().selected_tool == 7)
            {
                ImGui::Separator();
                ImGui::Text(ICON_TOOL_LOT " Lot Edit");
                ImGui::TextDisabled("Click to place lots or buildings. Shift+Click to remove.");

                ImGui::Text("Placement Mode");
                int &placement_mode = user_prefs.tools().lot_placement_mode;
                ImGui::RadioButton(ICON_FA_VECTOR_SQUARE " Lot", &placement_mode, 0);
                ImGui::SameLine();
                ImGui::RadioButton(ICON_FA_BUILDING " Building", &placement_mode, 1);

                if (placement_mode == 0)
                {
                    ImGui::Text(ICON_FA_LAYER_GROUP " Lot Type");
                    const char *lot_types[] = {
                        ICON_CITY_HOUSE " Residential",
                        ICON_FA_HOUSE_CHIMNEY " Rowhome Compact",
                        ICON_CITY_SHOP " Retail Strip",
                        ICON_FA_BUILDING_COLUMNS " Mixed Use",
                        ICON_CITY_INDUSTRY " Logistics Industrial",
                        ICON_FA_GRADUATION_CAP " Civic Cultural",
                        ICON_FA_GEM " Luxury Scenic",
                        ICON_FA_TREE " Buffer Strip"};
                    ComboAnchored("##lot_type", &user_prefs.tools().selected_lot_type, lot_types, 8);
                    help_tooltip("Type assigned to newly placed lots.");
                }
                else
                {
                    ImGui::Text(ICON_FA_BUILDING " Building Type");
                    const char *building_types[] = {
                        ICON_CITY_HOUSE " Residential",
                        ICON_FA_HOUSE_CHIMNEY " Rowhome",
                        ICON_CITY_SHOP " Retail",
                        ICON_FA_BUILDING_COLUMNS " Mixed Use",
                        ICON_CITY_INDUSTRY " Industrial",
                        ICON_FA_GRADUATION_CAP " Civic",
                        ICON_FA_GEM " Luxury",
                        ICON_FA_PLUG " Utility"};
                    ComboAnchored("##building_type", &user_prefs.tools().selected_building_type, building_types, 8);
                    help_tooltip("Type assigned to newly placed buildings.");
                }

                ImGui::Separator();
                ImGui::Checkbox(ICON_UI_LOCK " Lock User Types", &user_prefs.tools().lock_user_placed_types);
                help_tooltip("When enabled, user-placed lots/buildings always keep their assigned type.\nWhen disabled, the generator may override types based on context.");

                ImGui::Checkbox(ICON_FA_BORDER_ALL " Snap to Grid", &user_prefs.tools().grid_snap);
                help_tooltip("Snap placement to grid points.");
            }
        }
        ImGui::End();
    }

    void ToolsWindow::render_tool_buttons()
    {
        ImGui::Text("Tool Selection");

        const char *tools[] = {
            ICON_TOOL_SELECT " View",
            ICON_FA_PAINTBRUSH " Paint Rules",
            ICON_FA_BRUSH " Paint Density",
            ICON_TOOL_DISTRICT " Place Districts",
            ICON_TOOL_RIVER " Draw Rivers",
            ICON_FA_MAP " Mark Zones",
            ICON_TOOL_ROAD " Road Edit",
            ICON_TOOL_LOT " Lot Edit"};

        int &tool = user_prefs.tools().selected_tool;
        for (int i = 0; i < 8; i++)
        {
            if (ImGui::RadioButton(tools[i], &tool, i))
            {
                // Tool changed
                if (tool == 6)
                {
                    user_prefs.app().show_road_intersections = true;
                }
            }
        }
    }

    void ToolsWindow::render_brush_controls()
    {
        ImGui::Text("Brush Settings");

        float &size = user_prefs.tools().brush_size;
        float &opacity = user_prefs.tools().brush_opacity;

        ImGui::SliderFloat("Brush Size", &size, 5.0f, 100.0f);
        help_tooltip("Adjusts brush footprint.");
        ImGui::SliderFloat("Opacity", &opacity, 0.1f, 1.0f);
        help_tooltip("Adjusts brush strength.");

        ImGui::Checkbox("Grid Snap", &user_prefs.tools().grid_snap);
        help_tooltip("Align brush painting to the grid.");
    }

    void ToolsWindow::render_rule_selector()
    {
    }

    // ============ ParametersWindow ============

    ParametersWindow::ParametersWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          export_button_clicked(false),
          generate_city_button_clicked(false),
          regenerate_lots_button_clicked(false)
    {
    }

    ParametersWindow::~ParametersWindow()
    {
    }

    void ParametersWindow::render(CityParams &params)
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("Parameters");
        if (ImGui::Begin("Parameters"))
        {
            render_window_context_menu("Parameters");
            render_window_lock_toggle("Parameters", user_prefs.windows().parameters_locked);
            render_basic_params(params);
            ImGui::Separator();
            render_advanced_params(params);
            ImGui::Separator();
            render_action_buttons();
        }
        ImGui::End();
    }

    void ParametersWindow::render_basic_params(CityParams &params)
    {
        ImGui::Text("Basic Parameters");

        ImGui::SliderFloat("Noise Scale", &params.noise_scale, 0.1f, 10.0f);
        const int grid_bounds_values[] = {1024, 2048, 4096, 8192};
        const char *grid_bounds_labels[] = {"1024", "2048", "4096", "8192"};
        int grid_bounds_index = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (params.city_size == grid_bounds_values[i])
            {
                grid_bounds_index = i;
                break;
            }
        }

        int previous_size = params.city_size;
        if (ComboAnchored("Texture Size", &grid_bounds_index, grid_bounds_labels, 4))
        {
            params.city_size = grid_bounds_values[grid_bounds_index];
        }
        user_prefs.app().grid_bounds = params.city_size;

        if (params.city_size != previous_size)
        {
            const float base = 1000.0f;
            float scale = static_cast<float>(params.city_size) / 1024.0f;
            scale *= scale;
            uint32_t auto_total = static_cast<uint32_t>(std::clamp(base * scale, 1000.0f, 20000.0f));
            params.maxTotalRoads = auto_total;
            params.maxMajorRoads = static_cast<uint32_t>(std::clamp(auto_total * params.majorToMinorRatio, 0.0f, static_cast<float>(auto_total)));
        }

        ImGui::Separator();
        ImGui::Text("Road Limits");
        float ratio = params.majorToMinorRatio;
        if (ImGui::SliderFloat("Major Ratio", &ratio, 0.05f, 0.95f))
        {
            params.majorToMinorRatio = ratio;
            params.maxMajorRoads = static_cast<uint32_t>(std::clamp(params.maxTotalRoads * params.majorToMinorRatio, 0.0f,
                                                                    static_cast<float>(params.maxTotalRoads)));
        }

        int max_total = static_cast<int>(params.maxTotalRoads);
        if (ImGui::SliderInt("Max Total Roads", &max_total, 0, 20000))
        {
            params.maxTotalRoads = static_cast<uint32_t>(std::max(0, max_total));
            if (params.maxMajorRoads > params.maxTotalRoads)
            {
                params.maxMajorRoads = params.maxTotalRoads;
            }
        }

        int max_major = static_cast<int>(params.maxMajorRoads);
        if (ImGui::SliderInt("Max Major Roads", &max_major, 0, 20000))
        {
            params.maxMajorRoads = static_cast<uint32_t>(std::clamp(max_major, 0, static_cast<int>(params.maxTotalRoads)));
        }
    }

    void ParametersWindow::render_advanced_params(CityParams &params)
    {
        ImGui::Text("Advanced Parameters");
        const char *modes[] = {"By Segment", "By Polyline"};
        int mode = static_cast<int>(params.roadDefinitionMode);
        if (ComboAnchored("Road Definition", &mode, modes, 2))
        {
            params.roadDefinitionMode = (mode == 0) ? CityParams::RoadDefinitionMode::BySegment
                                                    : CityParams::RoadDefinitionMode::ByPolyline;
        }
    }

    void ParametersWindow::render_action_buttons()
    {
        export_button_clicked = false;
        generate_city_button_clicked = false;
        regenerate_lots_button_clicked = false;

        const float checkbox_width = ImGui::GetFrameHeight() + ImGui::CalcTextSize("Skip Clip").x + ImGui::GetStyle().ItemInnerSpacing.x;
        const float avail_width = ImGui::GetContentRegionAvail().x;
        const float button_width = std::max(120.0f, avail_width - checkbox_width - ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("Generate City", ImVec2(button_width, 0)))
        {
            generate_city_button_clicked = true;
        }
        help_tooltip("Run full city generation (roads, districts, lots, buildings).");

        ImGui::SameLine();
        ImGui::Checkbox("Use Segments", &user_prefs.app().debug_use_segment_roads_for_blocks);
        help_tooltip("Debug: use segment roads for block polygonization.");

        ImGui::SameLine();
        // Minimal geometry pipeline toggle: disable most healing steps
        bool prev_disable = RCG::g_disable_geometry_healing;
        if (ImGui::Checkbox("Minimal Pipeline", &RCG::g_disable_geometry_healing))
        {
            RCG::DebugLog::printf("[UI] Minimal Geometry Pipeline %s\n", RCG::g_disable_geometry_healing ? "ENABLED" : "DISABLED");
            (void)prev_disable;
        }
        help_tooltip("When enabled: disable buffer/unbuffer, projection, dangle-prune and other healing steps for debugging.");

        if (ImGui::Button("Regenerate Lots", ImVec2(-1, 0)))
        {
            regenerate_lots_button_clicked = true;
        }
        help_tooltip("Regenerate lots/buildings (currently triggers full city regeneration).");

        if (ImGui::Button("Export JSON", ImVec2(-1, 0)))
        {
            export_button_clicked = true;
        }
    }

    // ============ ExportWindow ============

    ExportWindow::ExportWindow(UserPreferences &prefs)
        : user_prefs(prefs), export_format(0), export_button_clicked(false)
    {
    }

    ExportWindow::~ExportWindow()
    {
    }

    void ExportWindow::render()
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("Export & Status");
        if (ImGui::Begin("Export & Status"))
        {
            render_window_context_menu("Export & Status");
            render_window_lock_toggle("Export & Status", user_prefs.windows().export_locked);
            render_export_options();
            ImGui::Separator();
            render_status_bar();
        }
        ImGui::End();
    }

    void ExportWindow::render_export_options()
    {
        ImGui::Text("Export Format");

        const char *formats[] = {"JSON", "OBJ", "SVG"};
        ComboAnchored("##export_format", &export_format, formats, 3);

        export_button_clicked = false;
        if (ImGui::Button("Export", ImVec2(-1, 0)))
        {
            export_button_clicked = true;
        }
    }

    void ExportWindow::render_status_bar()
    {
        ImGui::Text("Status");

        ImGui::TextWrapped("%s", status_message.c_str());
    }

    // ============ ViewOptionsWindow ============

    ViewOptionsWindow::ViewOptionsWindow(UserPreferences &prefs) : user_prefs(prefs)
    {
    }

    ViewOptionsWindow::~ViewOptionsWindow()
    {
    }

    void ViewOptionsWindow::render()
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("View Options");
        if (ImGui::Begin("View Options"))
        {
            render_window_context_menu("View Options");
            render_window_lock_toggle("View Options", user_prefs.windows().view_options_locked);
            ImGui::Checkbox("Show Grid", &user_prefs.app().show_grid);
            help_tooltip("Toggle grid visibility.");
            ImGui::Checkbox("Show Axioms", &user_prefs.app().show_axioms);
            help_tooltip("Toggle axiom markers.");
            ImGui::Checkbox("Show Roads", &user_prefs.app().show_roads);
            help_tooltip("Toggle generated roads.");
            ImGui::Checkbox("Show Road Nodes", &user_prefs.app().show_road_intersections);
            help_tooltip("Show road node markers for editing.");
            ImGui::Checkbox("Show River Splines", &user_prefs.app().show_river_splines);
            help_tooltip("Toggle river spline rendering.");
            ImGui::Checkbox("Show River Markers", &user_prefs.app().show_river_markers);
            help_tooltip("Toggle river control point markers.");
            ImGui::ColorEdit4("River Spline Color", &user_prefs.app().river_spline_color.x,
                              ImGuiColorEditFlags_NoInputs);
            help_tooltip("Color used for river splines.");
            ImGui::Checkbox("Show Influence Rings", &user_prefs.app().show_axiom_influence);
            help_tooltip("Show axiom influence radius.");
            ImGui::Checkbox("Show Selected Road Overlay", &user_prefs.app().show_selected_road_overlay);
            help_tooltip("Highlight the currently selected road in Road Index.");
            ImGui::ColorEdit4("Selected Road Color", &user_prefs.app().selected_road_color.x,
                              ImGuiColorEditFlags_NoInputs);
            help_tooltip("Highlight color for selected road.");
            ImGui::Checkbox("Show Density Overlay", &user_prefs.app().show_density_overlay);
            help_tooltip("Toggle density overlay.");
            ImGui::Checkbox("Show Rules Overlay", &user_prefs.app().show_rules_overlay);
            help_tooltip("Toggle growth rules overlay.");
            ImGui::Checkbox("Show Block Polygons", &user_prefs.app().show_block_polygons);
            help_tooltip("Render polygonized blocks/zones in the viewport.");
            ImGui::Checkbox("Show Block Faces (Debug)", &user_prefs.app().show_block_faces_debug);
            help_tooltip("Overlay all detected faces, even if they fail closure rules.");
            ImGui::Checkbox("Show Block Closable Status", &user_prefs.app().show_block_closable_faces);
            help_tooltip("Highlight closable vs non-closable faces in different colors.");
            ImGui::Checkbox("Enable Debug Logging", &user_prefs.app().enable_debug_logging);
            help_tooltip("Global toggle for generator debug logs.");
            ImGui::Separator();
            ImGui::Text("District Debug Overlays");
            ImGui::Checkbox("Show District Grid", &user_prefs.app().show_district_grid_overlay);
            help_tooltip("Show the district generation grid cells.");
            ImGui::Checkbox("Show District IDs", &user_prefs.app().show_district_ids);
            help_tooltip("Display district ID numbers on the grid.");
            ImGui::Separator();
            ImGui::Checkbox("Enable Multi-Viewport Windows", &user_prefs.app().enable_viewports);
            help_tooltip("When enabled, ImGui can create OS-level popout windows. Restart required.");
        }
        ImGui::End();
    }

    // ============ RoadsWindow ============

    RoadsWindow::RoadsWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          generate_button_clicked(false),
          clear_button_clicked(false),
          clear_all_button_clicked(false)
    {
    }

    RoadsWindow::~RoadsWindow()
    {
    }

    void RoadsWindow::render(CityParams &params, bool has_city)
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("Roads");
        generate_button_clicked = false;
        clear_button_clicked = false;
        clear_all_button_clicked = false;

        if (ImGui::Begin("Roads"))
        {
            render_window_context_menu("Roads");
            render_window_lock_toggle("Roads", user_prefs.windows().roads_locked);
            const char *generate_label = has_city ? "Regenerate Roads" : "Generate Roads";
            if (ImGui::Button(generate_label, ImVec2(-1, 0)))
            {
                generate_button_clicked = true;
            }
            help_tooltip("Generate or rebuild road network.");

            if (ImGui::Button("Clear Roads", ImVec2(-1, 0)))
            {
                clear_button_clicked = true;
            }
            help_tooltip("Clear generated roads (preserves user-created roads).");

            if (ImGui::Button("Clear All Roads", ImVec2(-1, 0)))
            {
                clear_all_button_clicked = true;
            }
            help_tooltip("Clear ALL roads including user-created roads.");

            if (ImGui::Button("Toggle Roads##toggle_roads", ImVec2(-1, 0)))
            {
                user_prefs.app().show_roads = !user_prefs.app().show_roads;
            }
            ImGui::Checkbox("Show Roads", &user_prefs.app().show_roads);

            ImGui::Separator();
            ImGui::Text("Road Parameters");

            // Road Density as parallel spacing
            ImGui::Text("Parallel Spacing (Density)");
            ImGui::SliderFloat("Min Spacing", &user_prefs.app().min_parallel_spacing, 5.0f, 100.0f);
            help_tooltip("Minimum distance between parallel roads.");
            ImGui::SliderFloat("Max Spacing", &user_prefs.app().max_parallel_spacing,
                               user_prefs.app().min_parallel_spacing, 200.0f);
            help_tooltip("Maximum distance between parallel roads.");

            ImGui::SliderInt("Major Roads", &params.major_road_iterations, 10, 100);
            help_tooltip("Controls road streamline length for major tiers.");
            ImGui::SliderInt("Minor Roads", &params.minor_road_iterations, 10, 50);
            help_tooltip("Controls road streamline length for minor tiers.");
            ImGui::SliderFloat("Snap Distance", &params.vertex_snap_distance, 1.0f, 50.0f);
            help_tooltip("Merge nearby vertices to reduce clutter.");

            ImGui::Separator();
            ImGui::Text("Random Seed");
            int seed_int = static_cast<int>(params.seed);
            if (SliderIntPrecision("Seed##seed_slider", &seed_int, 1, 999999))
            {
                params.seed = static_cast<unsigned int>(seed_int);
            }
            help_tooltip("Random seed for reproducible generation. Hold Shift for precision.");
            ImGui::SameLine();
            if (ImGui::Button("Random##randomize_seed"))
            {
                // Generate random seed using current time
                params.seed = static_cast<unsigned int>(std::time(nullptr) % 999999 + 1);
            }
            help_tooltip("Generate a new random seed.");

            ImGui::Separator();
            ImGui::Text("Generation Phases");
            ImGui::Checkbox("Roads Phase", &params.phase_enabled[0]);
            help_tooltip("Enable/disable road generation phase.");
            ImGui::Checkbox("Districts Phase", &params.phase_enabled[1]);
            help_tooltip("Enable/disable district generation phase.");
            ImGui::Checkbox("Blocks Phase", &params.phase_enabled[2]);
            help_tooltip("Enable/disable block extraction phase.");
            ImGui::Checkbox("Lots Phase", &params.phase_enabled[3]);
            help_tooltip("Enable/disable lot placement phase.");
            ImGui::Checkbox("Buildings Phase", &params.phase_enabled[4]);
            help_tooltip("Enable/disable building site generation phase.");

            ImGui::Separator();
            ImGui::Text("Block Generator Mode");
            const char *block_gen_modes[] = {"Legacy", "GEOS Only"};
            ImGui::Combo("##block_gen_mode", &params.block_gen_mode, block_gen_modes, 2);
            help_tooltip("Legacy: Original PolygonFinder. GEOS Only: Uses GEOS polygonize.");

            ImGui::Separator();
            ImGui::Text("Road Snapping");
            ImGui::Checkbox("Snap to Grid", &user_prefs.app().road_snap_to_grid);
            help_tooltip("User road endpoints snap to grid intersections.");
            ImGui::Checkbox("Snap to Axioms", &user_prefs.app().road_snap_to_axioms);
            help_tooltip("User road endpoints snap to axiom centers.");
            ImGui::Checkbox("Dynamic Snap", &user_prefs.app().road_dynamic_snap);
            help_tooltip("User road endpoints snap to nearby road intersections.");
            ImGui::SliderFloat("Snap Radius", &user_prefs.app().road_snap_distance, 5.0f, 50.0f);
            help_tooltip("Distance threshold for snapping.");

            ImGui::Separator();
            if (ImGui::CollapsingHeader("Block Rules", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Controls which road types split space and which can close blocks.");
                if (ImGui::BeginTable("BlockRulesTable", 3, ImGuiTableFlags_BordersInner | ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Road Type", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Barrier", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                    ImGui::TableSetupColumn("Closure", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                    ImGui::TableHeadersRow();
                    for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
                    {
                        auto type = static_cast<CityModel::RoadType>(i);
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(CityModel::road_type_label(type));
                        ImGui::TableNextColumn();
                        ImGui::PushID(static_cast<int>(i));
                        ImGui::Checkbox("##barrier", &params.block_barrier[i]);
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("##closure", &params.block_closure[i]);
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                ImGui::Spacing();
                ImGui::TextUnformatted("Barrier: participates in the block graph. Closure: can complete a valid block.");
                ImGui::Separator();
                ImGui::SliderFloat("Merge Radius", &params.merge_radius, 1.0f, 50.0f, "%.1f");
                help_tooltip("Radius for merging nearby points in block generation. Lower = more precise geometry, higher = smoother blocks.");
                ImGui::SliderFloat("Snap Tolerance Factor", &params.block_snap_tolerance_factor, 0.1f, 1.0f, "%.2f");
                help_tooltip("Multiplier for merge_radius to set coordinate snapping grid. Lower = more precise but may miss connections. Higher = more merging but may over-simplify.");
                ImGui::Checkbox("Verbose GEOS Diagnostics", &params.verbose_geos_diagnostics);
                help_tooltip("Enable detailed stage-by-stage logging for GEOS polygonization.");
            }
        }
        ImGui::End();
    }

    // ============ RiversWindow ============

    RiversWindow::RiversWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          generate_button_clicked(false),
          clear_button_clicked(false)
    {
    }

    RiversWindow::~RiversWindow()
    {
    }

    void RiversWindow::render(CityParams &params, bool has_rivers)
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("Rivers");
        generate_button_clicked = false;
        clear_button_clicked = false;

        if (ImGui::Begin("Rivers"))
        {
            render_window_context_menu("Rivers");
            render_window_lock_toggle("Rivers", user_prefs.windows().rivers_locked);
            const char *generate_label = has_rivers ? "Regenerate Rivers" : "Generate Rivers";
            if (ImGui::Button(generate_label, ImVec2(-1, 0)))
            {
                generate_button_clicked = true;
            }
            help_tooltip("Finalize river splines from control points.");

            if (ImGui::Button("Clear Rivers", ImVec2(-1, 0)))
            {
                clear_button_clicked = true;
            }
            help_tooltip("Clear all river splines and control points.");

            if (ImGui::Button("Toggle Rivers##toggle_rivers", ImVec2(-1, 0)))
            {
                user_prefs.app().show_river_splines = !user_prefs.app().show_river_splines;
            }

            ImGui::Checkbox("Show River Splines", &user_prefs.app().show_river_splines);
            ImGui::Checkbox("Show River Markers", &user_prefs.app().show_river_markers);

            ImGui::Separator();
            ImGui::Text("Generator Parameters");
            ImGui::Checkbox("Generate Rivers##generator_toggle", &params.generate_rivers);
            help_tooltip("Enable tensor-field water generation.");
            ImGui::SliderInt("River Count", &params.river_count, 1, 10);
            help_tooltip("Controls generator river seeding attempts.");
            ImGui::SliderFloat("River Width", &params.river_width, 5.0f, 50.0f);
            help_tooltip("Width for generated river channels.");
        }
        ImGui::End();
    }

    // ============ DistrictIndexWindow ============

    DistrictIndexWindow::DistrictIndexWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          last_selected_id(-1),
          hovered_axiom_id(-1),
          selected_axiom_ids(),
          active_axiom_id(-1),
          selected_district_id(-1),
          hovered_district_id(-1),
          selected_district_ids(),
          active_district_id(-1),
          axiom_filter_index(0),
          district_filter_index(0),
          show_axioms(true),
          show_districts(true),
          name_buffer{}
    {
        name_buffer[0] = '\0';
    }

    DistrictIndexWindow::~DistrictIndexWindow()
    {
    }

    void DistrictIndexWindow::render(ViewportWindow &viewport, CityParams &city_params, CityModel::City &city)
    {
        const auto &axioms = viewport.get_axioms();
        auto &districts = city.districts;
        auto &tabs = user_prefs.app().district_tabs;

        const auto &viewport_selected_axiom_ids = viewport.get_selected_axiom_ids();
        const auto &viewport_selected_district_ids = viewport.get_selected_district_ids();
        int viewport_active_axiom = viewport.get_active_axiom_id();
        int viewport_active_district = viewport.get_active_district_id();
        int viewport_selected_district = viewport.get_selected_district_id();

        if (viewport_selected_axiom_ids != selected_axiom_ids)
        {
            selected_axiom_ids = viewport_selected_axiom_ids;
        }
        if (viewport_active_axiom != active_axiom_id)
        {
            active_axiom_id = viewport_active_axiom;
        }

        if (viewport_selected_district_ids != selected_district_ids)
        {
            selected_district_ids = viewport_selected_district_ids;
        }
        if (viewport_active_district != active_district_id)
        {
            active_district_id = viewport_active_district;
        }
        if (viewport_selected_district != selected_district_id)
        {
            selected_district_id = viewport_selected_district;
        }

        hovered_axiom_id = -1;
        hovered_district_id = -1;

#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("District Index");
        if (ImGui::Begin("District Index"))
        {
            render_window_context_menu("District Index");
            render_window_lock_toggle("District Index", user_prefs.windows().district_locked);
            ImGui::Text("Axioms: %d  |  Districts: %d", static_cast<int>(axioms.size()), static_cast<int>(districts.size()));

            ImGui::Checkbox("Show Axioms", &show_axioms);
            ImGui::SameLine();
            ImGui::Checkbox("Show Districts", &show_districts);
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear Selection"))
            {
                selected_axiom_ids.clear();
                selected_district_ids.clear();
                active_axiom_id = -1;
                active_district_id = -1;
                selected_district_id = -1;
                viewport.set_selected_axiom_id(-1);
                viewport.set_selected_district_id(-1);
                viewport.set_active_axiom_id(-1);
                viewport.set_active_district_id(-1);
            }

            const char *axiom_filters[] = {"All", "Radial", "Delta", "Block", "Grid Corrective"};
            ImGui::SetNextItemWidth(160.0f);
            ComboAnchored("Axiom Filter", &axiom_filter_index, axiom_filters, 5);
            const char *district_filters[] = {"All", "Mixed", "Residential", "Commercial", "Civic", "Industrial"};
            ImGui::SetNextItemWidth(160.0f);
            ComboAnchored("District Filter", &district_filter_index, district_filters, 6);

            render_tab_visibility("district_tabs", tabs, true, true);

            if (ImGui::BeginTabBar("DistrictIndexTabs"))
            {
                if (tabs.show_list && ImGui::BeginTabItem("List"))
                {
                    auto selection_empty = [&]()
                    {
                        return selected_axiom_ids.empty() && selected_district_ids.empty();
                    };

                    if (ImGui::BeginTable("DistrictIndexTable", 10,
                                          ImGuiTableFlags_Borders |
                                              ImGuiTableFlags_RowBg |
                                              ImGuiTableFlags_ScrollY |
                                              ImGuiTableFlags_Resizable,
                                          ImVec2(0.0f, 0.0f)))
                    {
                        ImGui::TableSetupScrollFreeze(0, 1);
                        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                        ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                        ImGui::TableSetupColumn("Tag/Primary", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                        ImGui::TableSetupColumn("Secondary", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                        ImGui::TableSetupColumn("X/Area", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                        ImGui::TableSetupColumn("Y/Border", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                        ImGui::TableSetupColumn("Size/Orient", ImGuiTableColumnFlags_WidthFixed, 95.0f);
                        ImGui::TableSetupColumn("Opacity", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableHeadersRow();

                        if (show_axioms)
                        {
                            for (const auto &axiom : axioms)
                            {
                                if (axiom_filter_index > 0 && axiom.type != (axiom_filter_index - 1))
                                {
                                    continue;
                                }

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);

                                bool is_in_selection_set = selected_axiom_ids.count(axiom.id) > 0;
                                bool is_active = (axiom.id == active_axiom_id);
                                bool selected = is_in_selection_set || (selected_axiom_ids.empty() && is_active);

                                char row_id[32];
                                std::snprintf(row_id, sizeof(row_id), "##axiom_row_%d", axiom.id);
                                if (ImGui::Selectable(row_id, selected, ImGuiSelectableFlags_SpanAllColumns))
                                {
                                    if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
                                    {
                                        selected_axiom_ids.clear();
                                        active_axiom_id = axiom.id;
                                        selected_district_ids.clear();
                                        active_district_id = -1;
                                        selected_district_id = -1;
                                        viewport.set_selected_district_ids(selected_district_ids);
                                        viewport.set_active_district_id(-1);
                                        viewport.set_selected_district_id(-1);
                                    }
                                    else if (ImGui::GetIO().KeyCtrl)
                                    {
                                        if (selected_axiom_ids.count(axiom.id))
                                            selected_axiom_ids.erase(axiom.id);
                                        else
                                            selected_axiom_ids.insert(axiom.id);
                                        active_axiom_id = axiom.id;
                                    }
                                    viewport.set_selected_axiom_id(axiom.id);
                                }

                                if (ImGui::IsItemHovered() && selection_empty())
                                {
                                    hovered_axiom_id = axiom.id;
                                }

                                ImGui::SameLine();
                                ImGui::Text("%d", axiom.id);
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted("Axiom");
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextUnformatted(axiom.name.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextUnformatted(axiom_type_label_ui(axiom.type));
                                ImGui::TableSetColumnIndex(4);
                                if (axiom.tag_id > 0)
                                {
                                    ImGui::Text("%d", axiom.tag_id);
                                }
                                else
                                {
                                    ImGui::TextUnformatted("-");
                                }
                                ImGui::TableSetColumnIndex(5);
                                ImGui::TextUnformatted("-");
                                ImGui::TableSetColumnIndex(6);
                                ImGui::Text("%.1f", axiom.position.x);
                                ImGui::TableSetColumnIndex(7);
                                ImGui::Text("%.1f", axiom.position.y);
                                ImGui::TableSetColumnIndex(8);
                                ImGui::Text("%.1f", axiom.size);
                                ImGui::TableSetColumnIndex(9);
                                ImGui::Text("%.2f", axiom.opacity);
                            }
                        }

                        if (show_districts)
                        {
                            for (const auto &district : districts)
                            {
                                if (district_filter_index > 0 && static_cast<int>(district.type) != (district_filter_index - 1))
                                {
                                    continue;
                                }

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);

                                int id = static_cast<int>(district.id);
                                bool is_in_selection_set = selected_district_ids.count(id) > 0;
                                bool is_active = (id == active_district_id);
                                bool selected = is_in_selection_set || (selected_district_ids.empty() && is_active);

                                char row_id[32];
                                std::snprintf(row_id, sizeof(row_id), "##district_row_%u", district.id);
                                if (ImGui::Selectable(row_id, selected, ImGuiSelectableFlags_SpanAllColumns))
                                {
                                    if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
                                    {
                                        selected_district_ids.clear();
                                        active_district_id = id;
                                        selected_district_id = id;
                                        selected_axiom_ids.clear();
                                        active_axiom_id = -1;
                                        viewport.set_selected_axiom_ids(selected_axiom_ids);
                                        viewport.set_active_axiom_id(-1);
                                        viewport.set_selected_axiom_id(-1);
                                    }
                                    else if (ImGui::GetIO().KeyCtrl)
                                    {
                                        if (selected_district_ids.count(id))
                                            selected_district_ids.erase(id);
                                        else
                                            selected_district_ids.insert(id);
                                        active_district_id = id;
                                        selected_district_id = id;
                                    }
                                    viewport.set_selected_district_id(id);
                                }

                                if (ImGui::IsItemHovered() && selection_empty())
                                {
                                    hovered_district_id = id;
                                }

                                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                                {
                                    selected_district_ids.clear();
                                    selected_district_ids.insert(id);
                                    active_district_id = id;
                                    selected_district_id = id;
                                    selected_axiom_ids.clear();
                                    active_axiom_id = -1;
                                    viewport.set_selected_axiom_ids(selected_axiom_ids);
                                    viewport.set_active_axiom_id(-1);
                                    viewport.set_selected_axiom_id(-1);
                                    ImGui::OpenPopup("DistrictContextMenu");
                                }

                                ImGui::SameLine();
                                ImGui::Text("%u", district.id);
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted("District");
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextUnformatted(district_label(district, axioms).c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextUnformatted(district_type_label(district.type));
                                ImGui::TableSetColumnIndex(4);
                                const auto *primary = find_axiom_by_id(axioms, district.primary_axiom_id);
                                ImGui::TextUnformatted(primary ? axiom_display_name(*primary).c_str() : "-");
                                ImGui::TableSetColumnIndex(5);
                                if (district.secondary_axiom_id >= 0)
                                {
                                    const auto *secondary = find_axiom_by_id(axioms, district.secondary_axiom_id);
                                    ImGui::TextUnformatted(secondary ? axiom_display_name(*secondary).c_str() : "-");
                                }
                                else
                                {
                                    ImGui::TextUnformatted("-");
                                }
                                ImGui::TableSetColumnIndex(6);
                                ImGui::Text("%.0f", polygon_area(district.border));
                                ImGui::TableSetColumnIndex(7);
                                ImGui::Text("%d", static_cast<int>(district.border.size()));
                                ImGui::TableSetColumnIndex(8);
                                if (district.orientation.lengthSquared() > 1e-6)
                                {
                                    double angle = std::atan2(district.orientation.y, district.orientation.x) * 57.29577951308232;
                                    ImGui::Text("%.0f deg", angle);
                                }
                                else
                                {
                                    ImGui::TextUnformatted("-");
                                }
                                ImGui::TableSetColumnIndex(9);
                                ImGui::TextUnformatted("-");
                            }
                        }

                        ImGui::EndTable();
                    }

                    if (ImGui::BeginPopup("DistrictContextMenu"))
                    {
                        ImGui::TextDisabled("District Actions");
                        ImGui::Separator();
                        ImGui::TextUnformatted("it works");
                        ImGui::EndPopup();
                    }

                    if (selection_empty())
                    {
                        viewport.set_hovered_axiom_id(hovered_axiom_id);
                        viewport.set_hovered_district_id(hovered_district_id);
                    }
                    else
                    {
                        viewport.set_hovered_axiom_id(-1);
                        viewport.set_hovered_district_id(-1);
                    }

                    viewport.set_selected_axiom_ids(selected_axiom_ids);
                    viewport.set_active_axiom_id(active_axiom_id);
                    viewport.set_selected_district_ids(selected_district_ids);
                    viewport.set_active_district_id(active_district_id);
                    viewport.set_selected_district_id(selected_district_id);

                    ImGui::EndTabItem();
                }

                if (tabs.show_details && ImGui::BeginTabItem("Details"))
                {
                    int selected_id = viewport.get_selected_axiom_id();
                    int selected_index = -1;
                    for (size_t i = 0; i < axioms.size(); ++i)
                    {
                        if (axioms[i].id == selected_id)
                        {
                            selected_index = static_cast<int>(i);
                            break;
                        }
                    }

                    if (selected_index >= 0)
                    {
                        const auto &selected_axiom = axioms[selected_index];
                        if (selected_id != last_selected_id)
                        {
                            std::snprintf(name_buffer, sizeof(name_buffer), "%s", selected_axiom.name.c_str());
                            last_selected_id = selected_id;
                        }

                        ImGui::Text("Axiom Settings");
                        ImGui::Separator();
                        ImGui::Text("Name");
                        if (ImGui::InputText("##axiom_name", name_buffer, sizeof(name_buffer)))
                        {
                            viewport.set_axiom_name(selected_id, std::string(name_buffer));
                        }
                        ImGui::Text("Tag ID: %d", selected_axiom.tag_id);

                        if (selected_axiom.type == 1)
                        {
                            int terminal = selected_axiom.terminal_type;
                            const char *terminal_labels[] = {"Top", "Bottom Left", "Bottom Right"};
                            if (ComboAnchored("Delta Terminal", &terminal, terminal_labels, 3))
                            {
                                viewport.set_axiom_terminal(selected_id, terminal);
                            }
                        }
                        if (selected_axiom.type == 0)
                        {
                            int mode = selected_axiom.mode;
                            const char *radial_modes[] = {"Roundabout", "Center", "Both"};
                            if (ComboAnchored("Radial Mode", &mode, radial_modes, 3))
                            {
                                viewport.set_axiom_mode(selected_id, mode);
                            }
                        }
                        if (selected_axiom.type == 2)
                        {
                            int mode = selected_axiom.mode;
                            const char *block_modes[] = {"Strict", "Free", "Corner"};
                            if (ComboAnchored("Block Mode", &mode, block_modes, 3))
                            {
                                viewport.set_axiom_mode(selected_id, mode);
                            }
                        }

                        ImVec2 position = selected_axiom.position;
                        if (ImGui::DragFloat("X", &position.x, 1.0f))
                        {
                            viewport.set_axiom_position(selected_id, position);
                        }
                        if (ImGui::DragFloat("Y", &position.y, 1.0f))
                        {
                            viewport.set_axiom_position(selected_id, position);
                        }
                        float size = selected_axiom.size;
                        if (SliderFloatPrecision("Size", &size, 2.0f, 30.0f))
                        {
                            viewport.set_axiom_size(selected_id, std::clamp(size, 2.0f, 30.0f));
                        }
                        help_tooltip("Hold Shift for precision.");
                        float opacity = selected_axiom.opacity;
                        if (ImGui::SliderFloat("Opacity", &opacity, 0.05f, 1.0f))
                        {
                            viewport.set_axiom_opacity(selected_id, opacity);
                        }

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Text("Road Count Settings");
                        bool fuzzy = selected_axiom.fuzzy_mode;
                        if (ImGui::Checkbox("Fuzzy Mode", &fuzzy))
                        {
                            viewport.set_axiom_fuzzy_mode(selected_id, fuzzy);
                        }
                        help_tooltip("Enable min/max range instead of exact count.");

                        if (fuzzy)
                        {
                            int min_roads = selected_axiom.min_roads;
                            int max_roads = selected_axiom.max_roads;
                            if (SliderIntPrecision("Min Roads", &min_roads, 1, 500))
                            {
                                viewport.set_axiom_road_range(selected_id, min_roads, max_roads);
                            }
                            if (SliderIntPrecision("Max Roads", &max_roads, min_roads, 500))
                            {
                                viewport.set_axiom_road_range(selected_id, min_roads, max_roads);
                            }
                            help_tooltip("Hold Shift for precision.");
                        }
                        else
                        {
                            int major_count = selected_axiom.major_road_count;
                            int minor_count = selected_axiom.minor_road_count;

                            ImGui::Text("Major Roads:");
                            ImGui::SameLine();
                            if (ImGui::Button("-##major_dec") && major_count > 0)
                            {
                                viewport.set_axiom_major_road_count(selected_id, major_count - 1);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("+##major_inc") && major_count < 500)
                            {
                                viewport.set_axiom_major_road_count(selected_id, major_count + 1);
                            }
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(60.0f);
                            if (ImGui::InputInt("##major_input", &major_count, 0, 0))
                            {
                                viewport.set_axiom_major_road_count(selected_id, std::clamp(major_count, 0, 500));
                            }

                            ImGui::Text("Minor Roads:");
                            ImGui::SameLine();
                            if (ImGui::Button("-##minor_dec") && minor_count > 0)
                            {
                                viewport.set_axiom_minor_road_count(selected_id, minor_count - 1);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("+##minor_inc") && minor_count < 500)
                            {
                                viewport.set_axiom_minor_road_count(selected_id, minor_count + 1);
                            }
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(60.0f);
                            if (ImGui::InputInt("##minor_input", &minor_count, 0, 0))
                            {
                                viewport.set_axiom_minor_road_count(selected_id, std::clamp(minor_count, 0, 500));
                            }
                        }
                    }
                    else
                    {
                        ImGui::TextUnformatted("Select an axiom to edit its settings.");
                        last_selected_id = -1;
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Text("District Settings");
                    int selected_district_index = -1;
                    if (selected_district_id >= 0)
                    {
                        for (size_t i = 0; i < districts.size(); ++i)
                        {
                            if (static_cast<int>(districts[i].id) == selected_district_id)
                            {
                                selected_district_index = static_cast<int>(i);
                                break;
                            }
                        }
                    }

                    if (selected_district_index >= 0)
                    {
                        auto &selected_district = districts[selected_district_index];
                        ImGui::Text("Name: %s", district_label(selected_district, axioms).c_str());
                        ImGui::Text("Type: %s", district_type_label(selected_district.type));
                        ImGui::Text("Area: %.0f", polygon_area(selected_district.border));
                        ImGui::Text("Border Pts: %d", static_cast<int>(selected_district.border.size()));
                        if (selected_district.orientation.lengthSquared() > 1e-6)
                        {
                            double angle = std::atan2(selected_district.orientation.y, selected_district.orientation.x) * 57.29577951308232;
                            ImGui::Text("Orientation: %.0f deg", angle);
                        }
                        else
                        {
                            ImGui::TextUnformatted("Orientation: -");
                        }

                        std::vector<int> axiom_ids;
                        std::vector<std::string> axiom_labels;
                        std::vector<const char *> axiom_items;
                        axiom_ids.push_back(-1);
                        axiom_labels.push_back("None");
                        for (const auto &axiom : axioms)
                        {
                            axiom_ids.push_back(axiom.id);
                            axiom_labels.push_back(axiom_display_name(axiom));
                        }
                        axiom_items.reserve(axiom_labels.size());
                        for (const auto &label : axiom_labels)
                        {
                            axiom_items.push_back(label.c_str());
                        }

                        int primary_index = 0;
                        for (size_t i = 0; i < axiom_ids.size(); ++i)
                        {
                            if (axiom_ids[i] == selected_district.primary_axiom_id)
                            {
                                primary_index = static_cast<int>(i);
                                break;
                            }
                        }
                        if (ComboAnchored("Primary Axiom", &primary_index, axiom_items.data(), static_cast<int>(axiom_items.size())))
                        {
                            selected_district.primary_axiom_id = axiom_ids[primary_index];
                        }

                        int secondary_index = 0;
                        for (size_t i = 0; i < axiom_ids.size(); ++i)
                        {
                            if (axiom_ids[i] == selected_district.secondary_axiom_id)
                            {
                                secondary_index = static_cast<int>(i);
                                break;
                            }
                        }
                        if (ComboAnchored("Secondary Axiom", &secondary_index, axiom_items.data(), static_cast<int>(axiom_items.size())))
                        {
                            selected_district.secondary_axiom_id = axiom_ids[secondary_index];
                        }
                        help_tooltip("Reassign district ownership using axiom labels.");

                        if (active_axiom_id >= 0)
                        {
                            if (ImGui::Button("Use Selected Axiom as Primary", ImVec2(-1, 0)))
                            {
                                selected_district.primary_axiom_id = active_axiom_id;
                            }
                            if (ImGui::Button("Use Selected Axiom as Secondary", ImVec2(-1, 0)))
                            {
                                selected_district.secondary_axiom_id = active_axiom_id;
                            }
                        }
                    }
                    else
                    {
                        ImGui::TextUnformatted("Select a district to edit ownership.");
                    }

                    ImGui::EndTabItem();
                }

                if (tabs.show_settings && ImGui::BeginTabItem("Settings"))
                {
                    render_district_settings(user_prefs, true);

                    ImGui::Separator();
                    ImGui::Text("Road Limits");
                    const float base = 1000.0f;
                    float scale = static_cast<float>(viewport.get_info().world_size.x) / 1024.0f;
                    scale *= scale;
                    uint32_t max_cap = static_cast<uint32_t>(std::clamp(base * scale, 1000.0f, 20000.0f));
                    ImGui::Text("Texture cap: %u", max_cap);
                    int max_total = static_cast<int>(city_params.maxTotalRoads);
                    if (ImGui::SliderInt("Max Total Roads", &max_total, 0, static_cast<int>(max_cap)))
                    {
                        city_params.maxTotalRoads = static_cast<uint32_t>(std::clamp(max_total, 0, static_cast<int>(max_cap)));
                        if (city_params.maxMajorRoads > city_params.maxTotalRoads)
                        {
                            city_params.maxMajorRoads = city_params.maxTotalRoads;
                        }
                    }
                    int max_major = static_cast<int>(city_params.maxMajorRoads);
                    if (ImGui::SliderInt("Max Major Roads", &max_major, 0, static_cast<int>(city_params.maxTotalRoads)))
                    {
                        city_params.maxMajorRoads = static_cast<uint32_t>(std::clamp(max_major, 0, static_cast<int>(city_params.maxTotalRoads)));
                    }

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    // ============ RiverIndexWindow ============

    RiverIndexWindow::RiverIndexWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          last_selected_id(-1),
          name_buffer{}
    {
        name_buffer[0] = '\0';
    }

    RiverIndexWindow::~RiverIndexWindow()
    {
    }

    void RiverIndexWindow::render(ViewportWindow &viewport)
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        const auto &rivers = viewport.get_rivers();
        apply_window_dock_request("River Index");
        if (ImGui::Begin("River Index"))
        {
            render_window_context_menu("River Index");
            render_window_lock_toggle("River Index", user_prefs.windows().river_index_locked);
            ImGui::Text("Total Rivers: %d", static_cast<int>(rivers.size()));
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float detail_height = 180.0f;
            ImVec2 table_size = ImVec2(0.0f, std::max(120.0f, avail.y - detail_height));
            if (ImGui::BeginTable("RiverTable", 6,
                                  ImGuiTableFlags_Borders |
                                      ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_ScrollY |
                                      ImGuiTableFlags_Resizable,
                                  table_size))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Width", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Opacity", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Points", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableHeadersRow();

                for (const auto &river : rivers)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    bool selected = (river.id == viewport.get_selected_river_id());
                    char row_id[32];
                    std::snprintf(row_id, sizeof(row_id), "##river_row_%d", river.id);
                    if (ImGui::Selectable(row_id, selected, ImGuiSelectableFlags_SpanAllColumns))
                    {
                        viewport.set_selected_river_id(river.id);
                    }
                    ImGui::SameLine();
                    ImGui::Text("%d", river.id);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(river.name.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(river_type_label_ui(river.type));
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.2f", river.width);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%.2f", river.opacity);
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%d", static_cast<int>(river.control_points.size()));
                }
                ImGui::EndTable();
            }

            int selected_id = viewport.get_selected_river_id();
            int selected_index = -1;
            for (size_t i = 0; i < rivers.size(); ++i)
            {
                if (rivers[i].id == selected_id)
                {
                    selected_index = static_cast<int>(i);
                    break;
                }
            }

            if (selected_index >= 0)
            {
                const auto &river = rivers[selected_index];
                if (selected_id != last_selected_id)
                {
                    std::snprintf(name_buffer, sizeof(name_buffer), "%s", river.name.c_str());
                    last_selected_id = selected_id;
                }

                ImGui::Separator();
                ImGui::Text("Selected River");
                if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer)))
                {
                    viewport.set_river_name(selected_id, std::string(name_buffer));
                }
                float width = river.width;
                if (ImGui::SliderFloat("Width", &width, 1.0f, 60.0f))
                {
                    viewport.set_river_width(selected_id, width);
                }
                float opacity = river.opacity;
                if (ImGui::SliderFloat("Opacity", &opacity, 0.05f, 1.0f))
                {
                    viewport.set_river_opacity(selected_id, opacity);
                }

                ImGui::Separator();
                ImGui::Text("Control Points");
                for (size_t i = 0; i < river.control_points.size(); ++i)
                {
                    ImVec2 cp = river.control_points[i];
                    char label[32];
                    std::snprintf(label, sizeof(label), "P%zu X", i);
                    if (ImGui::DragFloat(label, &cp.x, 1.0f))
                    {
                        viewport.set_river_point(selected_id, i, cp);
                    }
                    std::snprintf(label, sizeof(label), "P%zu Y", i);
                    if (ImGui::DragFloat(label, &cp.y, 1.0f))
                    {
                        viewport.set_river_point(selected_id, i, cp);
                    }
                }

                if (ImGui::Button("Add Control Point"))
                {
                    if (!river.control_points.empty())
                    {
                        ImVec2 last = river.control_points.back();
                        viewport.insert_river_point(selected_id, river.control_points.size(), last);
                    }
                }
                help_tooltip("Insert a new control point at the end.");
                ImGui::SameLine();
                if (ImGui::Button("Remove Last") && river.control_points.size() > 2)
                {
                    viewport.remove_river_point(selected_id, river.control_points.size() - 1);
                }
                help_tooltip("Remove the last control point (minimum 2).");
            }
            else
            {
                last_selected_id = -1;
            }
        }
        ImGui::End();
    }

    // ============ EventLogWindow ============

    EventLogWindow::EventLogWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          entries(),
          auto_scroll(true),
          max_entries(500)
    {
    }

    EventLogWindow::~EventLogWindow()
    {
    }

    void EventLogWindow::add_entry(const std::string &message)
    {
        entries.push_back(message);
        if (static_cast<int>(entries.size()) > max_entries)
        {
            int overflow = static_cast<int>(entries.size()) - max_entries;
            entries.erase(entries.begin(), entries.begin() + overflow);
        }
    }

    void EventLogWindow::clear()
    {
        entries.clear();
    }

    void EventLogWindow::render()
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("Event Log");
        if (ImGui::Begin("Event Log"))
        {
            render_window_context_menu("Event Log");
            render_window_lock_toggle("Event Log", user_prefs.windows().event_log_locked);
            if (ImGui::Button("Clear"))
            {
                clear();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Auto-scroll", &auto_scroll);

            ImGui::Separator();
            ImGui::BeginChild("EventLogScroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto &entry : entries)
            {
                ImGui::TextUnformatted(entry.c_str());
            }
            if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
            {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }

    // ============ PresetsWindow ============

    PresetsWindow::PresetsWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          selected_preset_index(-1),
          selected_preset_name(""),
          apply_clicked(false),
          save_clicked(false)
    {
        std::memset(new_preset_name, 0, sizeof(new_preset_name));
        std::memset(new_preset_description, 0, sizeof(new_preset_description));
    }

    PresetsWindow::~PresetsWindow()
    {
    }

    void PresetsWindow::render(CityParams &params)
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_clicked = false;
        save_clicked = false;

        apply_window_dock_request("Presets");
        if (ImGui::Begin("Presets"))
        {
            render_window_context_menu("Presets");
            render_window_lock_toggle("Presets", user_prefs.windows().presets_locked);
            ImGui::Text("City Generation Presets");
            ImGui::Separator();

            // Preset selection dropdown
            const char *preset_names[] = {
                "Urban Dense",
                "Suburban",
                "Rural",
                "Organic",
                "Grid City"};

            int num_presets = sizeof(preset_names) / sizeof(preset_names[0]);

            if (ComboAnchored("##preset_select", &selected_preset_index, preset_names, num_presets))
            {
                if (selected_preset_index >= 0 && selected_preset_index < num_presets)
                {
                    selected_preset_name = preset_names[selected_preset_index];
                }
            }
            help_tooltip("Select a preset to view its description and apply it.");

            // Show description for selected preset
            if (selected_preset_index >= 0 && selected_preset_index < num_presets)
            {
                ImGui::Separator();
                ImGui::TextWrapped("Description:");

                switch (selected_preset_index)
                {
                case 0: // Urban Dense
                    ImGui::TextWrapped("High-density urban environment with tight grid patterns. "
                                       "Suitable for downtown areas and dense cities.");
                    break;
                case 1: // Suburban
                    ImGui::TextWrapped("Moderate density suburban layout with balanced road distribution. "
                                       "Good for residential neighborhoods.");
                    break;
                case 2: // Rural
                    ImGui::TextWrapped("Low-density rural roads with organic growth patterns. "
                                       "Suitable for countryside and small towns.");
                    break;
                case 3: // Organic
                    ImGui::TextWrapped("Natural, flowing road patterns with minimal grid influence. "
                                       "Creates European-style old town layouts.");
                    break;
                case 4: // Grid City
                    ImGui::TextWrapped("Pure grid-based Manhattan-style layout with strong orthogonal patterns. "
                                       "Perfect for modern planned cities.");
                    break;
                }

                ImGui::Separator();
                if (ImGui::Button("Apply Preset", ImVec2(-1, 0)))
                {
                    apply_clicked = true;

                    // Apply the selected preset parameters
                    switch (selected_preset_index)
                    {
                    case 0: // Urban Dense
                        params.noise_scale = 0.8f;
                        params.city_size = 2048;
                        params.road_density = 0.8f;
                        params.major_road_iterations = 75;
                        params.minor_road_iterations = 40;
                        params.vertex_snap_distance = 10.0f;
                        params.maxMajorRoads = 2000;
                        params.maxTotalRoads = 6000;
                        params.majorToMinorRatio = 0.35f;
                        params.building_density = 0.9f;
                        params.max_building_height = 80.0f;
                        params.grid_rule_weight = 2;
                        params.radial_rule_weight = 1;
                        params.organic_rule_weight = 1;
                        params.seed = 12345;
                        break;
                    case 1: // Suburban
                        params.noise_scale = 1.0f;
                        params.city_size = 1024;
                        params.road_density = 0.5f;
                        params.major_road_iterations = 50;
                        params.minor_road_iterations = 25;
                        params.vertex_snap_distance = 15.0f;
                        params.maxMajorRoads = 1000;
                        params.maxTotalRoads = 3000;
                        params.majorToMinorRatio = 0.3f;
                        params.building_density = 0.6f;
                        params.max_building_height = 30.0f;
                        params.grid_rule_weight = 1;
                        params.radial_rule_weight = 1;
                        params.organic_rule_weight = 1;
                        params.seed = 23456;
                        break;
                    case 2: // Rural
                        params.noise_scale = 1.5f;
                        params.city_size = 1024;
                        params.road_density = 0.3f;
                        params.major_road_iterations = 30;
                        params.minor_road_iterations = 15;
                        params.vertex_snap_distance = 20.0f;
                        params.maxMajorRoads = 500;
                        params.maxTotalRoads = 1500;
                        params.majorToMinorRatio = 0.25f;
                        params.building_density = 0.3f;
                        params.max_building_height = 15.0f;
                        params.grid_rule_weight = 1;
                        params.radial_rule_weight = 1;
                        params.organic_rule_weight = 2;
                        params.seed = 34567;
                        break;
                    case 3: // Organic
                        params.noise_scale = 1.8f;
                        params.city_size = 1024;
                        params.road_density = 0.6f;
                        params.major_road_iterations = 60;
                        params.minor_road_iterations = 30;
                        params.vertex_snap_distance = 12.0f;
                        params.maxMajorRoads = 1200;
                        params.maxTotalRoads = 3500;
                        params.majorToMinorRatio = 0.3f;
                        params.building_density = 0.7f;
                        params.max_building_height = 40.0f;
                        params.grid_rule_weight = 0;
                        params.radial_rule_weight = 2;
                        params.organic_rule_weight = 3;
                        params.seed = 45678;
                        break;
                    case 4: // Grid City
                        params.noise_scale = 0.5f;
                        params.city_size = 2048;
                        params.road_density = 0.7f;
                        params.major_road_iterations = 80;
                        params.minor_road_iterations = 35;
                        params.vertex_snap_distance = 8.0f;
                        params.maxMajorRoads = 1800;
                        params.maxTotalRoads = 5000;
                        params.majorToMinorRatio = 0.35f;
                        params.building_density = 0.85f;
                        params.max_building_height = 60.0f;
                        params.grid_rule_weight = 4;
                        params.radial_rule_weight = 0;
                        params.organic_rule_weight = 0;
                        params.seed = 56789;
                        break;
                    }
                }
                help_tooltip("Apply this preset's parameters to the current city generation.");
            }

            ImGui::Separator();
            ImGui::Text("Save Current as Preset");
            ImGui::InputText("Name##preset_name", new_preset_name, sizeof(new_preset_name));
            ImGui::InputTextMultiline("Description##preset_desc", new_preset_description,
                                      sizeof(new_preset_description), ImVec2(-1, 60));

            if (ImGui::Button("Save as New Preset", ImVec2(-1, 0)))
            {
                save_clicked = true;
                // TODO: Implement custom preset saving
            }
            help_tooltip("Save current parameters as a new custom preset (not implemented yet).");
        }
        ImGui::End();
    }

    // ============ ProgressWindow ============

    ProgressWindow::ProgressWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          is_visible(false)
    {
    }

    ProgressWindow::~ProgressWindow()
    {
    }

    void ProgressWindow::set_progress(float progress)
    {
        user_prefs.app().generation_progress = std::clamp(progress, 0.0f, 1.0f);
    }

    void ProgressWindow::set_operation(const std::string &operation)
    {
        user_prefs.app().current_operation = operation;
    }

    void ProgressWindow::set_stats(int roads_generated, float time_ms)
    {
        user_prefs.app().roads_generated = roads_generated;
        user_prefs.app().generation_time_ms = time_ms;
    }

    void ProgressWindow::set_visible(bool visible)
    {
        is_visible = visible;
    }

    void ProgressWindow::render()
    {
        if (!is_visible || !user_prefs.app().show_progress)
            return;

        // Render as overlay in center of screen
        ImGuiIO &io = ImGui::GetIO();
        ImVec2 window_pos = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        ImVec2 window_pos_pivot = ImVec2(0.5f, 0.5f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowBgAlpha(0.9f);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                        ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoFocusOnAppearing |
                                        ImGuiWindowFlags_NoNav;

        if (ImGui::Begin("##ProgressOverlay", nullptr, window_flags))
        {
            ImGui::Text("Generating City...");
            ImGui::Separator();

            // Operation description
            if (!user_prefs.app().current_operation.empty())
            {
                ImGui::TextWrapped("%s", user_prefs.app().current_operation.c_str());
            }

            // Progress bar
            char progress_text[64];
            snprintf(progress_text, sizeof(progress_text), "%.0f%%",
                     user_prefs.app().generation_progress * 100.0f);
            ImGui::ProgressBar(user_prefs.app().generation_progress,
                               ImVec2(300.0f, 0.0f), progress_text);

            // Statistics
            if (user_prefs.app().show_generation_stats)
            {
                ImGui::Separator();
                ImGui::Text("Roads Generated: %d", user_prefs.app().roads_generated);
                ImGui::Text("Time: %.1f ms", user_prefs.app().generation_time_ms);
            }
        }
        ImGui::End();
    }

    // ============ ColorSettingsWindow ============

    ColorSettingsWindow::ColorSettingsWindow(UserPreferences &prefs)
        : user_prefs(prefs)
    {
    }

    ColorSettingsWindow::~ColorSettingsWindow()
    {
    }

    void ColorSettingsWindow::reset_to_defaults()
    {
        user_prefs.display().background_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        user_prefs.display().grid_color = ImVec4(0.3f, 0.3f, 0.3f, 0.3f);
        user_prefs.display().highway_road_color = ImVec4(0.95f, 0.35f, 0.2f, 1.0f);
        user_prefs.display().arterial_road_color = ImVec4(0.95f, 0.55f, 0.25f, 1.0f);
        user_prefs.display().avenue_road_color = ImVec4(0.95f, 0.7f, 0.3f, 1.0f);
        user_prefs.display().boulevard_road_color = ImVec4(0.9f, 0.8f, 0.35f, 1.0f);
        user_prefs.display().street_road_color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        user_prefs.display().lane_road_color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        user_prefs.display().alleyway_road_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        user_prefs.display().culdesac_road_color = ImVec4(0.65f, 0.65f, 0.5f, 1.0f);
        user_prefs.display().drive_road_color = ImVec4(0.55f, 0.55f, 0.6f, 1.0f);
        user_prefs.display().driveway_road_color = ImVec4(0.5f, 0.5f, 0.55f, 1.0f);
        user_prefs.display().user_major_road_color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
        user_prefs.display().user_minor_road_color = ImVec4(0.1f, 0.6f, 0.1f, 1.0f);
        user_prefs.display().radial_axiom_color = ImVec4(0.3f, 0.5f, 1.0f, 0.85f);
        user_prefs.display().delta_axiom_color = ImVec4(0.2f, 1.0f, 0.3f, 0.85f);
        user_prefs.display().block_axiom_color = ImVec4(1.0f, 0.5f, 0.2f, 0.85f);
        user_prefs.display().grid_axiom_color = ImVec4(1.0f, 0.2f, 0.5f, 0.85f);
    }

    void ColorSettingsWindow::render()
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("Color Settings");

        if (ImGui::Begin("Color Settings"))
        {
            render_window_context_menu("Color Settings");
            render_window_lock_toggle("Color Settings", user_prefs.windows().color_settings_locked);
            ImGui::Text("Visualization Colors");
            ImGui::Separator();

            // Road colors (picker only, no RGBA inputs)
            ImGui::Text("Road Colors");
            const float label_width = 130.0f;
            auto color_row = [&](const char *label, const char *id, ImVec4 &color, const char *tip)
            {
                ImGui::TextUnformatted(label);
                ImGui::SameLine(label_width);
                ImGui::ColorEdit4(id, (float *)&color, ImGuiColorEditFlags_NoInputs);
                help_tooltip(tip);
            };

            color_row("Highway", "##road_highway", user_prefs.display().highway_road_color, "Color for highway roads.");
            color_row("Arterial", "##road_arterial", user_prefs.display().arterial_road_color, "Color for arterial roads.");
            color_row("Avenue", "##road_avenue", user_prefs.display().avenue_road_color, "Color for avenue roads.");
            color_row("Boulevard", "##road_boulevard", user_prefs.display().boulevard_road_color, "Color for boulevard roads.");
            color_row("Street", "##road_street", user_prefs.display().street_road_color, "Color for street roads.");
            color_row("Lane", "##road_lane", user_prefs.display().lane_road_color, "Color for lane roads.");
            color_row("Alleyway", "##road_alley", user_prefs.display().alleyway_road_color, "Color for alleyway roads.");
            color_row("Cul-de-Sac", "##road_culdesac", user_prefs.display().culdesac_road_color, "Color for cul-de-sac roads.");
            color_row("Drive", "##road_drive", user_prefs.display().drive_road_color, "Color for drive roads.");
            color_row("Driveway", "##road_driveway", user_prefs.display().driveway_road_color, "Color for driveway roads.");
            color_row("User Major", "##user_major", user_prefs.display().user_major_road_color, "Color for user-created major roads.");
            color_row("User Minor", "##user_minor", user_prefs.display().user_minor_road_color, "Color for user-created minor roads.");

            ImGui::Separator();

            // Axiom colors (picker only)
            ImGui::Text("Axiom Colors");
            color_row("Radial Axiom", "##radial", user_prefs.display().radial_axiom_color, "Color for radial (blue circle) axioms.");
            color_row("Delta Axiom", "##delta", user_prefs.display().delta_axiom_color, "Color for delta (green triangle) axioms.");
            color_row("Block Axiom", "##block", user_prefs.display().block_axiom_color, "Color for block (hollow square) axioms.");
            color_row("Grid Axiom", "##grid_axiom", user_prefs.display().grid_axiom_color, "Color for grid corrective axioms.");

            ImGui::Separator();

            // Environment colors (picker only)
            ImGui::Text("Environment Colors");
            color_row("Background", "##bg", user_prefs.display().background_color, "Viewport background color.");
            color_row("Grid", "##grid", user_prefs.display().grid_color, "Grid line color.");

            ImGui::Separator();

            if (ImGui::Button("Reset to Defaults", ImVec2(-1, 0)))
            {
                reset_to_defaults();
            }
            help_tooltip("Reset all colors to default values.");
        }
        ImGui::End();
    }

    // ============ RoadIndexWindow ============

    RoadIndexWindow::RoadIndexWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          selected_road_id(-1),
          hovered_road_id(-1),
          selected_road_ids(),
          active_road_id(-1),
          add_major_button_clicked(false),
          add_minor_button_clicked(false),
          remove_road_button_clicked(false),
          road_to_remove_id(-1),
          lock_road_button_clicked(false),
          road_to_lock_id(-1),
          unlock_road_button_clicked(false),
          road_to_unlock_id(-1)
    {
    }

    RoadIndexWindow::~RoadIndexWindow()
    {
    }

    static const char *road_category_label(RoadCategory cat)
    {
        switch (cat)
        {
        case RoadCategory::User:
            return "User";
        default:
            return "Gen";
        }
    }

    void RoadIndexWindow::render(ViewportWindow &viewport, CityParams &params, std::vector<RoadIndexEntry> &roads, uint32_t max_total_roads, uint32_t current_road_count)
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("Road Index");

        // Reset button states each frame
        add_major_button_clicked = false;
        add_minor_button_clicked = false;
        remove_road_button_clicked = false;
        road_to_remove_id = -1;
        lock_road_button_clicked = false;
        road_to_lock_id = -1;
        unlock_road_button_clicked = false;
        road_to_unlock_id = -1;

        if (!ImGui::Begin("Road Index"))
        {
            ImGui::End();
            return;
        }

        render_window_context_menu("Road Index");
        render_window_lock_toggle("Road Index", user_prefs.windows().road_index_locked);

        uint32_t remaining = (max_total_roads > current_road_count) ? (max_total_roads - current_road_count) : 0;
        ImGui::Text("Roads: %u / %u", current_road_count, max_total_roads);
        ImGui::SameLine();
        ImGui::TextDisabled("(-%u)", remaining);

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 140);
        if (ImGui::Button("+Major"))
        {
            add_major_button_clicked = true;
        }
        help_tooltip("Add M_Major road at selected axiom");
        ImGui::SameLine();
        if (ImGui::Button("+Minor"))
        {
            add_minor_button_clicked = true;
        }
        help_tooltip("Add M_Minor road at selected axiom");

        // Sync selection from viewport clicks
        const auto &viewport_selected_ids = viewport.get_selected_road_ids();
        int viewport_active = viewport.get_active_road_id();
        int viewport_selected = viewport.get_selected_road_id();

        if (viewport_selected >= 0 && viewport_selected != selected_road_id)
        {
            selected_road_id = viewport_selected;
        }
        if (viewport_selected_ids != selected_road_ids)
        {
            selected_road_ids = viewport_selected_ids;
        }
        if (viewport_active >= 0 && viewport_active != active_road_id)
        {
            active_road_id = viewport_active;
        }

        auto &tabs = user_prefs.app().road_tabs;
        render_tab_visibility("road_tabs", tabs, true, true);

        if (ImGui::BeginTabBar("RoadIndexTabs"))
        {
            if (tabs.show_list && ImGui::BeginTabItem("List"))
            {
                ImGui::Text("Highlight:");
                ImGui::SameLine();
                ImGui::Checkbox("Origin##hl", &user_prefs.app().show_origin_axiom_highlight);
                ImGui::SameLine();
                ImGui::Checkbox("End##hl", &user_prefs.app().show_end_axiom_highlight);
                ImGui::SameLine();
                ImGui::Checkbox("Influences##hl", &user_prefs.app().show_influence_highlight);
                ImGui::SameLine();
                ImGui::Checkbox("Nodes##hl", &user_prefs.app().show_intersection_highlight);

                ImGui::Separator();

                if (ImGui::BeginTable("RoadTable", 8,
                                      ImGuiTableFlags_Borders |
                                          ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_ScrollY |
                                          ImGuiTableFlags_Resizable,
                                      ImVec2(0.0f, 0.0f)))
                {
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                    ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Origin", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("End", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                    ImGui::TableSetupColumn("Nodes", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("Xing", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                    ImGui::TableHeadersRow();

                    for (auto &road : roads)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);

                        bool is_in_selection_set = selected_road_ids.count(road.id) > 0;
                        bool is_active = (road.id == active_road_id);
                        bool selected = is_in_selection_set || (selected_road_ids.empty() && is_active);

                        char row_id[32];
                        std::snprintf(row_id, sizeof(row_id), "##road_row_%d", road.id);
                        if (ImGui::Selectable(row_id, selected, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
                            {
                                selected_road_ids.clear();
                                active_road_id = road.id;
                                selected_road_id = road.id;
                                viewport.set_selected_road_id(road.id);
                            }
                            else if (ImGui::GetIO().KeyCtrl)
                            {
                                if (selected_road_ids.count(road.id))
                                    selected_road_ids.erase(road.id);
                                else
                                    selected_road_ids.insert(road.id);
                                active_road_id = road.id;
                                selected_road_id = road.id;
                                viewport.set_selected_road_id(road.id);
                            }
                        }

                        if (ImGui::IsItemHovered())
                        {
                            if (selected_road_ids.empty())
                            {
                                hovered_road_id = road.id;
                                viewport.set_hovered_road_id(road.id);
                            }
                        }
                        else
                        {
                            if (hovered_road_id == road.id)
                            {
                                hovered_road_id = -1;
                                viewport.set_hovered_road_id(-1);
                            }
                        }

                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            selected_road_ids.clear();
                            selected_road_ids.insert(road.id);
                            active_road_id = road.id;
                            selected_road_id = road.id;
                            viewport.set_selected_road_id(road.id);
                            viewport.set_selected_road_ids(selected_road_ids);
                            ImGui::OpenPopup("RoadContextMenu");
                        }

                        ImGui::SameLine();
                        ImGui::Text("%d", road.id);

                        ImGui::TableSetColumnIndex(1);
                        if (road.category == RoadCategory::User)
                        {
                            ImVec4 user_color = CityModel::is_user_road_type(road.road_type)
                                                    ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f)
                                                    : ImVec4(0.25f, 0.6f, 1.0f, 1.0f);
                            ImGui::TextColored(user_color, "%s", road_category_label(road.category));
                        }
                        else
                        {
                            ImGui::TextUnformatted(road_category_label(road.category));
                        }

                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(CityModel::road_type_label(road.road_type));

                        ImGui::TableSetColumnIndex(3);
                        if (road.axiom_origin_id >= 0)
                            ImGui::Text("%d", road.axiom_origin_id);
                        else
                            ImGui::TextUnformatted("-");

                        ImGui::TableSetColumnIndex(4);
                        if (road.axiom_end_id >= 0)
                            ImGui::Text("%d", road.axiom_end_id);
                        else
                            ImGui::TextUnformatted("-");

                        ImGui::TableSetColumnIndex(5);
                        ImGui::Text("%d", static_cast<int>(road.nodes.size()));

                        ImGui::TableSetColumnIndex(6);
                        if (road.intersections_computed)
                            ImGui::Text("%d", static_cast<int>(road.intersections.size()));
                        else
                            ImGui::TextUnformatted("-");

                        ImGui::TableSetColumnIndex(7);
                        if (road.category == RoadCategory::Generated)
                        {
                            char lock_id[32];
                            std::snprintf(lock_id, sizeof(lock_id), "Lock##%d", road.id);
                            if (ImGui::SmallButton(lock_id))
                            {
                                lock_road_button_clicked = true;
                                road_to_lock_id = road.id;
                            }
                        }
                        else
                        {
                            char unlock_id[32];
                            std::snprintf(unlock_id, sizeof(unlock_id), "Unlock##%d", road.id);
                            if (ImGui::SmallButton(unlock_id))
                            {
                                unlock_road_button_clicked = true;
                                road_to_unlock_id = road.id;
                            }
                        }
                    }

                    ImGui::EndTable();

                    viewport.set_selected_road_ids(selected_road_ids);
                    viewport.set_active_road_id(active_road_id);
                }

                if (ImGui::BeginPopup("RoadContextMenu"))
                {
                    ImGui::TextDisabled("Road Actions");
                    ImGui::Separator();
                    if (ImGui::MenuItem("Focus Camera"))
                    {
                        // TODO: implement camera focus
                    }
                    if (ImGui::MenuItem("Duplicate"))
                    {
                        // TODO: implement duplicate
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete", "Del"))
                    {
                        // TODO: implement delete from context
                    }
                    ImGui::EndPopup();
                }

                ImGui::EndTabItem();
            }

            if (tabs.show_details && ImGui::BeginTabItem("Details"))
            {
                if (selected_road_id >= 0)
                {
                    RoadIndexEntry *selected_road = nullptr;
                    for (auto &road : roads)
                    {
                        if (road.id == selected_road_id)
                        {
                            selected_road = &road;
                            break;
                        }
                    }

                    if (selected_road)
                    {
                        if (!selected_road->intersections_computed)
                        {
                            compute_road_intersections(*selected_road, roads);
                        }

                        ImGui::Text("Road %d", selected_road->id);
                        ImGui::SameLine();
                        ImGui::TextDisabled("(%s)", road_category_label(selected_road->category));

                        if (selected_road->category == RoadCategory::Generated)
                        {
                            if (ImGui::Button("Lock to User", ImVec2(-1, 0)))
                            {
                                lock_road_button_clicked = true;
                                road_to_lock_id = selected_road->id;
                            }
                        }
                        else
                        {
                            if (ImGui::Button("Unlock", ImVec2(90, 0)))
                            {
                                unlock_road_button_clicked = true;
                                road_to_unlock_id = selected_road->id;
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Delete", ImVec2(80, 0)))
                            {
                                remove_road_button_clicked = true;
                                road_to_remove_id = selected_road->id;
                                selected_road_id = -1;
                                selected_road_ids.clear();
                                active_road_id = -1;
                                viewport.set_selected_road_id(-1);
                                viewport.set_selected_road_ids(selected_road_ids);
                                viewport.set_active_road_id(-1);
                            }
                        }

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::Text("Type");
                        ImGui::SetNextItemWidth(-1);
                        const char *type_labels[] = {"Highway", "Arterial", "Avenue", "Boulevard", "Street", "Lane", "Alleyway", "CulDeSac", "Drive", "Driveway", "M_Major", "M_Minor"};
                        int type_index = static_cast<int>(selected_road->road_type);
                        if (ComboAnchored("##road_type", &type_index, type_labels, static_cast<int>(CityModel::road_type_count)))
                        {
                            selected_road->road_type = static_cast<CityModel::RoadType>(type_index);
                            viewport.submit_road_edit(selected_road->id, selected_road->road_type, selected_road->nodes, selected_road->category != RoadCategory::Generated);
                        }

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::Columns(2, nullptr, false);
                        ImGui::Text("Nodes:");
                        ImGui::NextColumn();
                        ImGui::Text("%d", static_cast<int>(selected_road->nodes.size()));
                        ImGui::NextColumn();

                        if (!selected_road->influenced_by_axioms.empty())
                        {
                            ImGui::Text("Axiom Influences:");
                            ImGui::NextColumn();
                            for (size_t i = 0; i < selected_road->influenced_by_axioms.size(); ++i)
                            {
                                if (i > 0)
                                    ImGui::SameLine();
                                ImGui::Text("%d", selected_road->influenced_by_axioms[i]);
                                if (i < selected_road->influenced_by_axioms.size() - 1)
                                {
                                    ImGui::SameLine();
                                    ImGui::TextUnformatted(",");
                                }
                            }
                            ImGui::NextColumn();
                        }

                        ImGui::Text("Crossings:");
                        ImGui::NextColumn();
                        ImGui::Text("%d", static_cast<int>(selected_road->intersections.size()));
                        ImGui::Columns(1);

                        if (!selected_road->intersections.empty())
                        {
                            ImGui::Spacing();
                            ImGui::Text("Intersection Details:");
                            ImGui::BeginChild("CrossingsList", ImVec2(0, 80), true);
                            for (const auto &isect : selected_road->intersections)
                            {
                                ImGui::BulletText("#%d (%s) @ (%.0f, %.0f)",
                                                  isect.road_id,
                                                  CityModel::road_type_label(isect.road_type),
                                                  isect.x, isect.y);
                            }
                            ImGui::EndChild();
                        }

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        auto ensure_nodes = [&]() -> std::vector<ImVec2> &
                        {
                            if (selected_road->nodes.empty())
                            {
                                selected_road->nodes.push_back(ImVec2(selected_road->start_x, selected_road->start_y));
                                selected_road->nodes.push_back(ImVec2(selected_road->end_x, selected_road->end_y));
                            }
                            return selected_road->nodes;
                        };
                        auto update_endpoints = [&]()
                        {
                            if (!selected_road->nodes.empty())
                            {
                                selected_road->start_x = selected_road->nodes.front().x;
                                selected_road->start_y = selected_road->nodes.front().y;
                                selected_road->end_x = selected_road->nodes.back().x;
                                selected_road->end_y = selected_road->nodes.back().y;
                            }
                        };
                        auto &nodes = ensure_nodes();

                        ImGui::Text("Edit nodes by dragging X/Y values:");
                        ImGui::Spacing();

                        ImGui::BeginChild("NodesList", ImVec2(0, 120), true);
                        for (size_t i = 0; i < nodes.size(); ++i)
                        {
                            ImGui::PushID(static_cast<int>(i));
                            ImGui::Text("Node %zu:", i);
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(80);
                            if (ImGui::DragFloat("##x", &nodes[i].x, 1.0f))
                            {
                                update_endpoints();
                                viewport.submit_road_edit(selected_road->id, selected_road->road_type, nodes, selected_road->category != RoadCategory::Generated);
                            }
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(80);
                            if (ImGui::DragFloat("##y", &nodes[i].y, 1.0f))
                            {
                                update_endpoints();
                                viewport.submit_road_edit(selected_road->id, selected_road->road_type, nodes, selected_road->category != RoadCategory::Generated);
                            }
                            ImGui::PopID();
                        }
                        ImGui::EndChild();

                        if (ImGui::Button("Add Node", ImVec2(-1, 0)))
                        {
                            ImVec2 tail = nodes.empty() ? ImVec2(selected_road->end_x, selected_road->end_y) : nodes.back();
                            nodes.push_back(tail);
                            update_endpoints();
                            viewport.submit_road_edit(selected_road->id, selected_road->road_type, nodes, selected_road->category != RoadCategory::Generated);
                        }
                        if (ImGui::Button("Remove Last Node", ImVec2(-1, 0)) && nodes.size() > 2)
                        {
                            nodes.pop_back();
                            update_endpoints();
                            viewport.submit_road_edit(selected_road->id, selected_road->road_type, nodes, selected_road->category != RoadCategory::Generated);
                        }
                    }
                }
                else
                {
                    ImGui::TextUnformatted("Select a road to edit its settings.");
                }

                ImGui::EndTabItem();
            }

            if (tabs.show_settings && ImGui::BeginTabItem("Settings"))
            {
                ImGui::Text("Road Generation Parameters");
                ImGui::Separator();

                ImGui::Text("Parallel Spacing (Density)");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##min_sp", &user_prefs.app().min_parallel_spacing, 5.0f, 100.0f, "Min: %.1f");
                help_tooltip("Minimum distance between parallel roads.");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##max_sp", &user_prefs.app().max_parallel_spacing,
                                   user_prefs.app().min_parallel_spacing, 200.0f, "Max: %.1f");
                help_tooltip("Maximum distance between parallel roads.");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Road Iterations");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderInt("##major_it", &params.major_road_iterations, 10, 100, "Major: %d");
                help_tooltip("Controls streamline length for major road tiers.");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderInt("##minor_it", &params.minor_road_iterations, 10, 50, "Minor: %d");
                help_tooltip("Controls streamline length for minor road tiers.");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Snap Distance");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##snap_dist", &params.vertex_snap_distance, 1.0f, 50.0f, "%.1f");
                help_tooltip("Merge nearby vertices to reduce clutter.");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Random Seed");
                int seed_int = static_cast<int>(params.seed);
                ImGui::SetNextItemWidth(-80);
                if (SliderIntPrecision("##seed", &seed_int, 1, 999999))
                {
                    params.seed = static_cast<unsigned int>(seed_int);
                }
                help_tooltip("Random seed for reproducible generation. Hold Shift for precision.");
                ImGui::SameLine();
                if (ImGui::Button("Random"))
                {
                    params.seed = static_cast<unsigned int>(std::time(nullptr) % 999999 + 1);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Road Snapping");
                ImGui::Checkbox("Snap to Grid", &user_prefs.app().road_snap_to_grid);
                help_tooltip("User road endpoints snap to grid intersections.");
                ImGui::Checkbox("Snap to Axioms", &user_prefs.app().road_snap_to_axioms);
                help_tooltip("User road endpoints snap to axiom centers.");
                ImGui::Checkbox("Dynamic Snap", &user_prefs.app().road_dynamic_snap);
                help_tooltip("User road endpoints snap to nearby road nodes.");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##snap_rad", &user_prefs.app().road_snap_distance, 5.0f, 50.0f, "Radius: %.1f");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Lot Generation");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderInt("##min_lots_side", &params.minLotsPerRoadSide, 1, 5, "Min Lots/Side: %d");
                help_tooltip("Minimum lots placed per road per side. Higher values ensure coverage along short roads.");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##lot_spacing_mult", &params.lotSpacingMultiplier, 0.5f, 3.0f, "Spacing: %.2fx");
                help_tooltip("Multiplier for lot spacing. Larger values spread lots further apart along roads.");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    // ============ LotIndexWindow ============

    LotIndexWindow::LotIndexWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          selected_lot_id(-1),
          hovered_lot_id(-1),
          selected_lot_ids(),
          active_lot_id(-1),
          district_filter_index(0),
          lot_filter_index(0),
          color_by_district(true)
    {
    }

    LotIndexWindow::~LotIndexWindow()
    {
    }

    void LotIndexWindow::render(ViewportWindow &viewport, const CityModel::City &city, uint32_t max_lots)
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("Lot Index");

        const auto &lots = city.lots;
        const auto &districts = city.districts;
        auto &tabs = user_prefs.app().lot_tabs;

        if (!ImGui::Begin("Lot Index"))
        {
            ImGui::End();
            return;
        }

        render_window_context_menu("Lot Index");
        render_window_lock_toggle("Lot Index", user_prefs.windows().lot_index_locked);

        ImGui::Text("Lots: %d / %u", static_cast<int>(lots.size()), max_lots);
        render_tab_visibility("lot_tabs", tabs, true, true);

        if (ImGui::BeginTabBar("LotIndexTabs"))
        {
            if (tabs.show_list && ImGui::BeginTabItem("List"))
            {
                if (ImGui::BeginTable("LotTable", 6,
                                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                          ImGuiTableFlags_ScrollY,
                                      ImVec2(0.0f, 0.0f)))
                {
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("District", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("District Type", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Lot Type", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Building Key", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Nearest Roads", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();

                    auto district_color = [](CityModel::DistrictType type) -> ImU32
                    {
                        switch (type)
                        {
                        case CityModel::DistrictType::Residential:
                            return IM_COL32(110, 190, 120, 50);
                        case CityModel::DistrictType::Commercial:
                            return IM_COL32(220, 170, 90, 50);
                        case CityModel::DistrictType::Civic:
                            return IM_COL32(120, 160, 220, 50);
                        case CityModel::DistrictType::Industrial:
                            return IM_COL32(180, 180, 120, 50);
                        case CityModel::DistrictType::Mixed:
                        default:
                            return IM_COL32(140, 140, 140, 35);
                        }
                    };

                    auto lot_color = [](CityModel::LotType type) -> ImU32
                    {
                        switch (type)
                        {
                        case CityModel::LotType::Residential:
                            return IM_COL32(120, 200, 130, 45);
                        case CityModel::LotType::RowhomeCompact:
                            return IM_COL32(100, 160, 120, 45);
                        case CityModel::LotType::RetailStrip:
                            return IM_COL32(210, 170, 90, 45);
                        case CityModel::LotType::MixedUse:
                            return IM_COL32(180, 170, 120, 45);
                        case CityModel::LotType::LogisticsIndustrial:
                            return IM_COL32(170, 170, 110, 45);
                        case CityModel::LotType::CivicCultural:
                            return IM_COL32(120, 160, 220, 45);
                        case CityModel::LotType::LuxuryScenic:
                            return IM_COL32(200, 190, 140, 45);
                        case CityModel::LotType::BufferStrip:
                            return IM_COL32(130, 130, 130, 45);
                        default:
                            return IM_COL32(140, 140, 140, 35);
                        }
                    };

                    for (const auto &lot : lots)
                    {
                        if (district_filter_index > 0)
                        {
                            int district_type_index = 0;
                            if (lot.district_id > 0 && lot.district_id <= districts.size())
                            {
                                district_type_index = static_cast<int>(districts[lot.district_id - 1].type) + 1;
                            }
                            if (district_filter_index != district_type_index)
                            {
                                continue;
                            }
                        }
                        if (lot_filter_index > 0 && static_cast<int>(lot.lot_type) != (lot_filter_index - 1))
                        {
                            continue;
                        }

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);

                        bool is_selected = selected_lot_ids.count(static_cast<int>(lot.id)) > 0;
                        char row_id[32];
                        std::snprintf(row_id, sizeof(row_id), "##lot_%u", lot.id);
                        if (ImGui::Selectable(row_id, is_selected, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
                            {
                                selected_lot_ids.clear();
                                active_lot_id = static_cast<int>(lot.id);
                                selected_lot_id = static_cast<int>(lot.id);
                            }
                            else if (ImGui::GetIO().KeyCtrl)
                            {
                                if (selected_lot_ids.count(static_cast<int>(lot.id)))
                                    selected_lot_ids.erase(static_cast<int>(lot.id));
                                else
                                    selected_lot_ids.insert(static_cast<int>(lot.id));
                                active_lot_id = static_cast<int>(lot.id);
                                selected_lot_id = static_cast<int>(lot.id);
                            }
                        }

                        if (ImGui::IsItemHovered())
                        {
                            hovered_lot_id = static_cast<int>(lot.id);
                        }

                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            selected_lot_ids.clear();
                            selected_lot_ids.insert(static_cast<int>(lot.id));
                            active_lot_id = static_cast<int>(lot.id);
                            selected_lot_id = static_cast<int>(lot.id);
                            ImGui::OpenPopup("LotContextMenu");
                        }

                        ImGui::SameLine();
                        ImGui::Text("%u", lot.id);

                        ImGui::TableSetColumnIndex(1);
                        CityModel::DistrictType dist_type = CityModel::DistrictType::Mixed;
                        const CityModel::District *district = nullptr;
                        if (lot.district_id > 0 && lot.district_id <= districts.size())
                        {
                            district = &districts[lot.district_id - 1];
                            dist_type = district->type;
                        }
                        if (district)
                        {
                            ImGui::TextUnformatted(district_name(*district).c_str());
                        }
                        else
                        {
                            ImGui::TextUnformatted("-");
                        }

                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(district_type_label(dist_type));

                        ImGui::TableSetColumnIndex(3);
                        ImGui::TextUnformatted(lot_type_label(lot.lot_type));

                        ImGui::TableSetColumnIndex(4);
                        std::string key = building_key(lot.district_id, lot.id, 0);
                        ImGui::TextUnformatted(key.c_str());

                        ImGui::TableSetColumnIndex(5);
                        ImVec2 pos(static_cast<float>(lot.centroid.x), static_cast<float>(lot.centroid.y));
                        NearestRoadInfo major = find_nearest_road(city, pos, true);
                        NearestRoadInfo minor = find_nearest_road(city, pos, false);
                        if (major.found)
                        {
                            ImGui::Text("%s/%d", CityModel::road_type_label(major.type), major.endpoint_index);
                        }
                        else
                        {
                            ImGui::TextUnformatted("-");
                        }
                        ImGui::SameLine();
                        ImGui::TextUnformatted(" | ");
                        ImGui::SameLine();
                        if (minor.found)
                        {
                            ImGui::Text("%s/%d", CityModel::road_type_label(minor.type), minor.endpoint_index);
                        }
                        else
                        {
                            ImGui::TextUnformatted("-");
                        }

                        ImU32 row_color = color_by_district ? district_color(dist_type) : lot_color(lot.lot_type);
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_color);
                    }

                    ImGui::EndTable();
                }

                if (ImGui::BeginPopup("LotContextMenu"))
                {
                    ImGui::TextDisabled("Lot Actions");
                    ImGui::Separator();
                    ImGui::TextUnformatted("it works");
                    ImGui::EndPopup();
                }

                if (selected_lot_ids.empty())
                {
                    viewport.set_hovered_lot_id(hovered_lot_id);
                }
                else
                {
                    viewport.set_hovered_lot_id(-1);
                }

                viewport.set_selected_lot_ids(selected_lot_ids);
                viewport.set_active_lot_id(active_lot_id);
                viewport.set_selected_lot_id(selected_lot_id);

                ImGui::EndTabItem();
            }

            if (tabs.show_details && ImGui::BeginTabItem("Details"))
            {
                if (selected_lot_id >= 0)
                {
                    const CityModel::LotToken *selected_lot = nullptr;
                    for (const auto &lot : lots)
                    {
                        if (static_cast<int>(lot.id) == selected_lot_id)
                        {
                            selected_lot = &lot;
                            break;
                        }
                    }

                    if (selected_lot)
                    {
                        ImGui::Text("Lot %u", selected_lot->id);
                        ImGui::Text("Type: %s", lot_type_label(selected_lot->lot_type));
                        ImGui::Text("District: %u", selected_lot->district_id);
                        ImGui::Text("Primary Road: %s", CityModel::road_type_label(selected_lot->primary_road));
                        ImGui::Text("Secondary Road: %s", CityModel::road_type_label(selected_lot->secondary_road));
                        ImGui::Text("Centroid: (%.1f, %.1f)", selected_lot->centroid.x, selected_lot->centroid.y);
                        ImGui::Text("Access: %.2f", selected_lot->access);
                        ImGui::Text("Exposure: %.2f", selected_lot->exposure);
                        ImGui::Text("Serviceability: %.2f", selected_lot->serviceability);
                        ImGui::Text("Privacy: %.2f", selected_lot->privacy);
                    }
                }
                else
                {
                    ImGui::TextUnformatted("Select a lot to view details.");
                }
                ImGui::EndTabItem();
            }

            if (tabs.show_settings && ImGui::BeginTabItem("Settings"))
            {
                const char *district_filters[] = {"All", "Mixed", "Residential", "Commercial", "Civic", "Industrial"};
                ImGui::SetNextItemWidth(160.0f);
                ComboAnchored("District Type", &district_filter_index, district_filters, 6);
                const char *lot_filters[] = {"All", "Residential", "Rowhome Compact", "Retail Strip", "Mixed Use", "Logistics Industrial", "Civic Cultural", "Luxury Scenic", "Buffer Strip"};
                ImGui::SetNextItemWidth(180.0f);
                ComboAnchored("Lot Type", &lot_filter_index, lot_filters, 9);
                ImGui::Checkbox("Color by District", &color_by_district);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    // ============ BuildingIndexWindow ============

    BuildingIndexWindow::BuildingIndexWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          selected_building_id(-1)
    {
    }

    BuildingIndexWindow::~BuildingIndexWindow()
    {
    }

    void BuildingIndexWindow::render(const CityModel::City &city, uint32_t max_building_sites)
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        apply_window_dock_request("Building Index");

        if (ImGui::Begin("Building Index"))
        {
            render_window_context_menu("Building Index");
            render_window_lock_toggle("Building Index", user_prefs.windows().building_index_locked);
            auto &tabs = user_prefs.app().building_tabs;
            render_tab_visibility("building_tabs", tabs, true, true);

            if (ImGui::BeginTabBar("BuildingIndexTabs"))
            {
                if (tabs.show_list && ImGui::BeginTabItem("List"))
                {
                    ImGui::Text("Building sites: %zu / %u", city.building_sites.size(), max_building_sites);
                    if (ImGui::BeginTable("BuildingIndexTable", 6,
                                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
                    {
                        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                        ImGui::TableSetupColumn("Lot", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableSetupColumn("District", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                        ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableHeadersRow();

                        for (const auto &site : city.building_sites)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            char id_label[32];
                            std::snprintf(id_label, sizeof(id_label), "%u", site.id);
                            if (ImGui::Selectable(id_label, static_cast<int>(site.id) == selected_building_id, ImGuiSelectableFlags_SpanAllColumns))
                            {
                                selected_building_id = static_cast<int>(site.id);
                            }
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%u", site.lot_id);
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%u", site.district_id);
                            ImGui::TableSetColumnIndex(3);
                            ImGui::Text("%s", building_type_label(site.type));
                            ImGui::TableSetColumnIndex(4);
                            ImGui::Text("%.1f", site.position.x);
                            ImGui::TableSetColumnIndex(5);
                            ImGui::Text("%.1f", site.position.y);
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if (tabs.show_details && ImGui::BeginTabItem("Details"))
                {
                    if (selected_building_id < 0)
                    {
                        ImGui::TextUnformatted("Select a building site from the list.");
                    }
                    else
                    {
                        const CityModel::BuildingSite *selected_site = nullptr;
                        for (const auto &site : city.building_sites)
                        {
                            if (static_cast<int>(site.id) == selected_building_id)
                            {
                                selected_site = &site;
                                break;
                            }
                        }
                        if (selected_site)
                        {
                            ImGui::Text("Site %u", selected_site->id);
                            ImGui::Text("Lot: %u", selected_site->lot_id);
                            ImGui::Text("District: %u", selected_site->district_id);
                            ImGui::Text("Type: %s", building_type_label(selected_site->type));
                            ImGui::Text("Position: (%.1f, %.1f)", selected_site->position.x, selected_site->position.y);
                        }
                        else
                        {
                            ImGui::TextUnformatted("Selected building site no longer exists.");
                            selected_building_id = -1;
                        }
                    }
                    ImGui::EndTabItem();
                }

                if (tabs.show_settings && ImGui::BeginTabItem("Settings"))
                {
                    ImGui::TextUnformatted("Building site settings coming later.");
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

} // namespace RCG
