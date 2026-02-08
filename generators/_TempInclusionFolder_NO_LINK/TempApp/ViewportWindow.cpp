#include "ViewportWindow.hpp"
#include "DistrictGenerator.h"
#include "ImGuiCompat.hpp"
#include <cmath>
#include <imgui.h>
#include <imGuIZMO.quat.h>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <random>
#include <string>
#include <utility>

namespace RCG
{

    namespace
    {
        int generate_axiom_tag()
        {
            static std::mt19937 rng(std::random_device{}());
            static std::uniform_int_distribution<int> dist(100, 999);
            return dist(rng);
        }

        bool vec2_near(const CityModel::Vec2 &a, const CityModel::Vec2 &b, double eps)
        {
            return a.distanceToSquared(b) <= eps * eps;
        }

        bool merge_polyline(CityModel::Polyline &base, const CityModel::Polyline &other, double eps)
        {
            if (base.points.size() < 2 || other.points.size() < 2)
            {
                return false;
            }

            const auto &a_start = base.points.front();
            const auto &a_end = base.points.back();
            const auto &b_start = other.points.front();
            const auto &b_end = other.points.back();

            if (vec2_near(a_end, b_start, eps))
            {
                base.points.insert(base.points.end(), other.points.begin() + 1, other.points.end());
                return true;
            }
            if (vec2_near(a_end, b_end, eps))
            {
                for (auto it = other.points.rbegin() + 1; it != other.points.rend(); ++it)
                {
                    base.points.push_back(*it);
                }
                return true;
            }
            if (vec2_near(a_start, b_end, eps))
            {
                base.points.insert(base.points.begin(), other.points.begin(), other.points.end() - 1);
                return true;
            }
            if (vec2_near(a_start, b_start, eps))
            {
                std::vector<CityModel::Vec2> prefix;
                prefix.reserve(other.points.size() - 1);
                for (auto it = other.points.rbegin(); it != other.points.rend() - 1; ++it)
                {
                    prefix.push_back(*it);
                }
                base.points.insert(base.points.begin(), prefix.begin(), prefix.end());
                return true;
            }

            return false;
        }

        std::vector<CityModel::Polyline> merge_polylines(const std::vector<CityModel::Polyline> &input, double eps)
        {
            std::vector<CityModel::Polyline> out;
            out.reserve(input.size());

            for (const auto &poly : input)
            {
                if (poly.points.size() < 2)
                {
                    continue;
                }

                bool merged = false;
                for (auto &existing : out)
                {
                    if (merge_polyline(existing, poly, eps))
                    {
                        merged = true;
                        break;
                    }
                }

                if (!merged)
                {
                    out.push_back(poly);
                }
            }

            bool changed = true;
            while (changed)
            {
                changed = false;
                for (std::size_t i = 0; i < out.size() && !changed; ++i)
                {
                    for (std::size_t j = i + 1; j < out.size(); ++j)
                    {
                        if (merge_polyline(out[i], out[j], eps))
                        {
                            out.erase(out.begin() + static_cast<std::ptrdiff_t>(j));
                            changed = true;
                            break;
                        }
                        if (merge_polyline(out[j], out[i], eps))
                        {
                            out[i] = out[j];
                            out.erase(out.begin() + static_cast<std::ptrdiff_t>(j));
                            changed = true;
                            break;
                        }
                    }
                }
            }

            return out;
        }

        struct Mat4
        {
            float m[16];
        };

        Mat4 make_identity()
        {
            Mat4 out{};
            out.m[0] = 1.0f;
            out.m[5] = 1.0f;
            out.m[10] = 1.0f;
            out.m[15] = 1.0f;
            return out;
        }

        Mat4 make_perspective(float fov_y_degrees, float aspect, float near_plane, float far_plane)
        {
            Mat4 out{};
            float fov_rad = fov_y_degrees * (3.1415926f / 180.0f);
            float f = 1.0f / tanf(fov_rad * 0.5f);

            out.m[0] = f / aspect;
            out.m[5] = f;
            out.m[10] = (far_plane + near_plane) / (near_plane - far_plane);
            out.m[11] = -1.0f;
            out.m[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
            return out;
        }

        Mat4 make_ortho(float left, float right, float bottom, float top, float near_plane, float far_plane)
        {
            Mat4 out{};
            out.m[0] = 2.0f / (right - left);
            out.m[5] = 2.0f / (top - bottom);
            out.m[10] = -2.0f / (far_plane - near_plane);
            out.m[12] = -(right + left) / (right - left);
            out.m[13] = -(top + bottom) / (top - bottom);
            out.m[14] = -(far_plane + near_plane) / (far_plane - near_plane);
            out.m[15] = 1.0f;
            return out;
        }

        float cross_2d(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c)
        {
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        }

        float polygon_area_signed(const std::vector<ImVec2> &pts)
        {
            if (pts.size() < 3)
            {
                return 0.0f;
            }
            float area = 0.0f;
            for (std::size_t i = 0; i < pts.size(); ++i)
            {
                const ImVec2 &a = pts[i];
                const ImVec2 &b = pts[(i + 1) % pts.size()];
                area += a.x * b.y - b.x * a.y;
            }
            return area * 0.5f;
        }

        bool point_in_triangle(const ImVec2 &p, const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, bool ccw)
        {
            const float c0 = cross_2d(a, b, p);
            const float c1 = cross_2d(b, c, p);
            const float c2 = cross_2d(c, a, p);
            const float eps = 1e-5f;
            if (ccw)
            {
                return c0 >= -eps && c1 >= -eps && c2 >= -eps;
            }
            return c0 <= eps && c1 <= eps && c2 <= eps;
        }

        bool triangulate_polygon(const std::vector<ImVec2> &pts, std::vector<int> &out_indices)
        {
            out_indices.clear();
            if (pts.size() < 3)
            {
                return false;
            }
            std::vector<int> indices(pts.size());
            for (std::size_t i = 0; i < pts.size(); ++i)
            {
                indices[i] = static_cast<int>(i);
            }
            const bool ccw = polygon_area_signed(pts) >= 0.0f;
            int guard = 0;
            while (indices.size() > 3 && guard < 10000)
            {
                bool ear_found = false;
                for (std::size_t i = 0; i < indices.size(); ++i)
                {
                    const int i0 = indices[(i + indices.size() - 1) % indices.size()];
                    const int i1 = indices[i];
                    const int i2 = indices[(i + 1) % indices.size()];
                    const ImVec2 &a = pts[i0];
                    const ImVec2 &b = pts[i1];
                    const ImVec2 &c = pts[i2];
                    const float cross = cross_2d(a, b, c);
                    if (ccw ? (cross <= 0.0f) : (cross >= 0.0f))
                    {
                        continue;
                    }
                    bool has_point = false;
                    for (std::size_t j = 0; j < indices.size(); ++j)
                    {
                        const int idx = indices[j];
                        if (idx == i0 || idx == i1 || idx == i2)
                        {
                            continue;
                        }
                        if (point_in_triangle(pts[idx], a, b, c, ccw))
                        {
                            has_point = true;
                            break;
                        }
                    }
                    if (has_point)
                    {
                        continue;
                    }
                    out_indices.push_back(i0);
                    out_indices.push_back(i1);
                    out_indices.push_back(i2);
                    indices.erase(indices.begin() + static_cast<long long>(i));
                    ear_found = true;
                    break;
                }
                if (!ear_found)
                {
                    break;
                }
                ++guard;
            }
            if (indices.size() == 3)
            {
                out_indices.push_back(indices[0]);
                out_indices.push_back(indices[1]);
                out_indices.push_back(indices[2]);
            }
            return !out_indices.empty();
        }

        Mat4 make_look_at(const ViewportWindow::Vec3 &eye,
                          const ViewportWindow::Vec3 &target,
                          const ViewportWindow::Vec3 &up)
        {
            ViewportWindow::Vec3 f{target.x - eye.x, target.y - eye.y, target.z - eye.z};
            float f_len = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
            if (f_len < 1e-6f)
                f_len = 1.0f;
            f = {f.x / f_len, f.y / f_len, f.z / f_len};

            ViewportWindow::Vec3 s{
                f.y * up.z - f.z * up.y,
                f.z * up.x - f.x * up.z,
                f.x * up.y - f.y * up.x};
            float s_len = sqrtf(s.x * s.x + s.y * s.y + s.z * s.z);
            if (s_len < 1e-6f)
                s_len = 1.0f;
            s = {s.x / s_len, s.y / s_len, s.z / s_len};

            ViewportWindow::Vec3 u{
                s.y * f.z - s.z * f.y,
                s.z * f.x - s.x * f.z,
                s.x * f.y - s.y * f.x};

            Mat4 out = make_identity();
            out.m[0] = s.x;
            out.m[4] = s.y;
            out.m[8] = s.z;

            out.m[1] = u.x;
            out.m[5] = u.y;
            out.m[9] = u.z;

            out.m[2] = -f.x;
            out.m[6] = -f.y;
            out.m[10] = -f.z;

            out.m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
            out.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
            out.m[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
            return out;
        }

        GLuint compile_shader(GLenum type, const char *source)
        {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);
            GLint status = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
            if (status == GL_FALSE)
            {
                glDeleteShader(shader);
                return 0;
            }
            return shader;
        }

        GLuint create_program(const char *vs_source, const char *fs_source)
        {
            GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_source);
            GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_source);
            if (vs == 0 || fs == 0)
            {
                if (vs != 0)
                    glDeleteShader(vs);
                if (fs != 0)
                    glDeleteShader(fs);
                return 0;
            }
            GLuint program = glCreateProgram();
            glAttachShader(program, vs);
            glAttachShader(program, fs);
            glLinkProgram(program);
            glDeleteShader(vs);
            glDeleteShader(fs);
            GLint status = 0;
            glGetProgramiv(program, GL_LINK_STATUS, &status);
            if (status == GL_FALSE)
            {
                glDeleteProgram(program);
                return 0;
            }
            return program;
        }

        const char *axiom_type_label(int type)
        {
            switch (type)
            {
            case 0:
                return "Radial";
            case 1:
                return "Delta";
            case 2:
                return "Block";
            case 3:
            default:
                return "Grid Corrective";
            }
        }

        void log_axiom_placement(const RCG::ViewportWindow::AxiomData &axiom)
        {
            std::fprintf(stdout, "Axiom %d placed at (%.2f, %.2f) type=%s size=%.2f opacity=%.2f\n",
                         axiom.id, axiom.position.x, axiom.position.y, axiom_type_label(axiom.type),
                         axiom.size, axiom.opacity);

            const std::string log_path = RCG::UserPreferences::get_config_dir() + "/axiom_log.csv";
            std::ofstream log_file(log_path, std::ios::app);
            if (log_file.is_open())
            {
                log_file << axiom.id << "," << axiom.position.x << "," << axiom.position.y << ","
                         << axiom.type << "," << axiom.terminal_type << "," << axiom.mode << ","
                         << axiom.size << "," << axiom.opacity << "\n";
            }
        }

        int find_axiom_index(const std::vector<RCG::ViewportWindow::AxiomData> &axioms, int id)
        {
            for (size_t i = 0; i < axioms.size(); ++i)
            {
                if (axioms[i].id == id)
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        const char *river_type_label(int type)
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

        void log_river_placement(const RCG::ViewportWindow::RiverData &river)
        {
            std::fprintf(stdout, "River %d placed type=%s width=%.2f opacity=%.2f controls=%zu points=%zu\n",
                         river.id, river_type_label(river.type), river.width, river.opacity,
                         river.control_points.size(), river.spline.size());
        }

        int find_river_index(const std::vector<RCG::ViewportWindow::RiverData> &rivers, int id)
        {
            for (size_t i = 0; i < rivers.size(); ++i)
            {
                if (rivers[i].id == id)
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        ImVec2 catmull_rom(const ImVec2 &p0, const ImVec2 &p1, const ImVec2 &p2, const ImVec2 &p3, float t)
        {
            float t2 = t * t;
            float t3 = t2 * t;
            float x = 0.5f * ((2.0f * p1.x) +
                              (-p0.x + p2.x) * t +
                              (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                              (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
            float y = 0.5f * ((2.0f * p1.y) +
                              (-p0.y + p2.y) * t +
                              (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                              (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
            return ImVec2(x, y);
        }

        std::vector<ImVec2> build_river_spline(const std::vector<ImVec2> &controls, int samples_per_segment)
        {
            std::vector<ImVec2> points;
            if (controls.size() < 2)
            {
                return points;
            }
            const int segments = static_cast<int>(controls.size()) - 1;
            points.reserve(segments * samples_per_segment + 1);
            for (int i = 0; i < segments; ++i)
            {
                const ImVec2 &p1 = controls[i];
                const ImVec2 &p2 = controls[i + 1];
                const ImVec2 &p0 = (i == 0) ? p1 : controls[i - 1];
                const ImVec2 &p3 = (i + 2 < static_cast<int>(controls.size())) ? controls[i + 2] : p2;
                for (int s = 0; s <= samples_per_segment; ++s)
                {
                    float t = static_cast<float>(s) / static_cast<float>(samples_per_segment);
                    points.push_back(catmull_rom(p0, p1, p2, p3, t));
                }
            }
            return points;
        }

        float dist_sq(ImVec2 a, ImVec2 b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            return dx * dx + dy * dy;
        }

        int find_control_point_index(const std::vector<ImVec2> &controls, ImVec2 pos, float radius)
        {
            float r2 = radius * radius;
            for (size_t i = 0; i < controls.size(); ++i)
            {
                if (dist_sq(controls[i], pos) <= r2)
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        float distance_to_segment(ImVec2 p, ImVec2 a, ImVec2 b)
        {
            float vx = b.x - a.x;
            float vy = b.y - a.y;
            float wx = p.x - a.x;
            float wy = p.y - a.y;
            float c1 = wx * vx + wy * vy;
            if (c1 <= 0.0f)
                return sqrtf(dist_sq(p, a));
            float c2 = vx * vx + vy * vy;
            if (c2 <= c1)
                return sqrtf(dist_sq(p, b));
            float t = c1 / c2;
            ImVec2 proj = ImVec2(a.x + t * vx, a.y + t * vy);
            return sqrtf(dist_sq(p, proj));
        }

        void update_river_after_edit(RCG::ViewportWindow::RiverData &river, bool live_mode)
        {
            if (live_mode)
            {
                river.spline = build_river_spline(river.control_points, 8);
                river.dirty = false;
            }
            else
            {
                river.spline.clear();
                river.dirty = true;
            }
        }

    } // namespace

    ViewportWindow::ViewportWindow(UserPreferences &prefs)
        : user_prefs(prefs),
          pan(0, 0),
          zoom(1.0f),
          camera_target{0.0f, 0.0f, 0.0f},
          camera_distance(20.0f),
          camera_yaw(0.785398f),   // 45 degrees
          camera_pitch(0.523599f), // 30 degrees
          fov_degrees(45.0f),
          ortho_enabled(false),
          nav_mode(NavigationMode::Orbit),
          axioms(),
          next_axiom_id(1),
          selected_axiom_id(-1),
          hovered_axiom_id(-1),
          selected_axiom_ids(),
          active_axiom_id(-1),
          dragging_axiom(false),
          dragging_axiom_index(-1),
          axiom_revision(0),
          rivers(),
          next_river_id(1),
          selected_river_id(-1),
          selected_river_control_index(-1),
          dragging_river(false),
          dragging_river_index(-1),
          dragging_river_offset(0.0f, 0.0f),
          river_revision(0),
          selected_road_id(-1),
          hovered_road_id(-1),
          selected_road_ids(),
          active_road_id(-1),
          selected_lot_id(-1),
          hovered_lot_id(-1),
          selected_lot_ids(),
          active_lot_id(-1),
          selected_district_id(-1),
          hovered_district_id(-1),
          selected_district_ids(),
          active_district_id(-1),
          last_grid_bounds(1024),
          last_grid_spacing(prefs.tools().grid_size),
          city_water(),
          city_roads_by_type(),
          city_districts(),
          city_lots(),
          city_building_sites(),
          city_block_polygons(),
          city_block_faces(),
          all_roads_indexed(),
          all_roads_refs(),
          user_roads(),
          hidden_generated_road_ids(),
          selected_road_node_index(-1),
          selected_road_is_user(false),
          last_selected_road_id(-1),
          dragging_road_node(false),
          dragging_road_node_index(-1),
          dragging_road_id(-1),
          dragging_road_is_user(false),
          dragging_road_handle(false),
          dragging_road_handle_index(-1),
          dragging_road_handle_in(true),
          dragging_road_handle_road_id(-1),
          road_edit_dirty(false),
          road_edit_pending(false),
          pending_road_edit(),
          city_major_road_count(0),
          city_total_road_count(0),
          city_max_major_roads(0),
          city_max_total_roads(0),
          city_total_lot_count(0),
          city_max_lots(0),
          city_total_building_sites(0),
          city_max_building_sites(0),
          block_road_inputs(0),
          block_faces_found(0),
          block_valid_blocks(0),
          block_intersections(0),
          block_segments(0),
          city_half_extent(0.0f),
          has_city(false),
          clear_city_requested(false),
          clear_axioms_requested(false),
          polyline_scratch(),
          cached_matrices_valid(false),
          texture_id(0),
          texture_width(1),
          texture_height(1),
          hovered(false),
          grid_vao(0),
          grid_vbo(0),
          grid_program(0),
          grid_vertex_count(0),
          grid_line_offset(0),
          grid_line_count(0),
          grid_initialized(false),
          last_light_theme(prefs.app().use_light_theme),
          fbo(0),
          color_texture(0),
          depth_buffer(0),
          framebuffer_width(0),
          framebuffer_height(0)
    {
        viewport_info.world_size = ImVec2(static_cast<float>(prefs.app().grid_bounds),
                                          static_cast<float>(prefs.app().grid_bounds));
        for (int i = 0; i < 16; ++i)
        {
            cached_view[i] = 0.0f;
            cached_proj[i] = 0.0f;
        }
        zoom = camera_distance;
        pan = ImVec2(camera_target.x, camera_target.z);
    }

    ViewportWindow::~ViewportWindow()
    {
        destroy_grid_resources();
        if (color_texture != 0)
        {
            glDeleteTextures(1, &color_texture);
            color_texture = 0;
        }
        if (depth_buffer != 0)
        {
            glDeleteRenderbuffers(1, &depth_buffer);
            depth_buffer = 0;
        }
        if (fbo != 0)
        {
            glDeleteFramebuffers(1, &fbo);
            fbo = 0;
        }
    }

    bool ViewportWindow::render()
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGui::SetNextWindowDockID(ImGui::GetID("MainDockSpace"), ImGuiCond_FirstUseEver);
#endif

        if (!ImGui::Begin("City Viewport", nullptr,
                          ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImGui::End();
            return false;
        }

        ImGui::BeginChild("ViewportToolbar", ImVec2(0, ImGui::GetFrameHeightWithSpacing()), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        int nav_mode_value = (nav_mode == NavigationMode::Orbit) ? 0 : 1;
        if (ImGui::RadioButton("Rotate", nav_mode_value == 0))
        {
            nav_mode = NavigationMode::Orbit;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Pan", nav_mode_value == 1))
        {
            nav_mode = NavigationMode::Pan;
        }
        ImGui::SameLine();
        if (ImGui::Button("Top Ortho"))
        {
            set_top_ortho_view();
        }
        ImGui::SameLine();
        if (ImGui::Button("Perspective"))
        {
            set_perspective_view();
        }
        ImGui::SameLine();
        if (ImGui::Button("Center"))
        {
            center_camera();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Axioms"))
        {
            clear_axioms_requested = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear City"))
        {
            clear_city_requested = true;
        }

        ImGui::EndChild();
        ImGui::Separator();

        viewport_info.canvas_pos = ImGui::GetCursorScreenPos();
        viewport_info.canvas_size = ImGui::GetContentRegionAvail();
        if (viewport_info.canvas_size.x < 1.0f || viewport_info.canvas_size.y < 1.0f)
        {
            ImGui::End();
            return false;
        }

        int grid_bounds = user_prefs.app().grid_bounds;
        if (grid_bounds < 256)
            grid_bounds = 256;
        viewport_info.world_size = ImVec2(static_cast<float>(grid_bounds), static_cast<float>(grid_bounds));

        cached_matrices_valid = false;

        ImGuiIO &io = ImGui::GetIO();
        float scale_x = io.DisplayFramebufferScale.x;
        float scale_y = io.DisplayFramebufferScale.y;
        if (scale_x <= 0.0f)
            scale_x = 1.0f;
        if (scale_y <= 0.0f)
            scale_y = 1.0f;

        int fb_w = static_cast<int>(viewport_info.canvas_size.x * scale_x);
        int fb_h = static_cast<int>(viewport_info.canvas_size.y * scale_y);
        if (fb_w < 1)
            fb_w = 1;
        if (fb_h < 1)
            fb_h = 1;

        ensure_framebuffer_size(fb_w, fb_h);
        render_gl();

        // Cache view/projection for overlay projection
        {
            ViewportWindow::Vec3 cam_pos = get_camera_position();
            Mat4 view = make_look_at(cam_pos, camera_target, {0.0f, 1.0f, 0.0f});
            float aspect = viewport_info.canvas_size.x / viewport_info.canvas_size.y;
            float near_clip = user_prefs.app().enable_culling ? user_prefs.app().cull_near : 0.1f;
            float far_clip = user_prefs.app().enable_culling ? user_prefs.app().cull_far : 5000.0f;
            if (near_clip < 0.001f)
                near_clip = 0.001f;
            if (far_clip <= near_clip + 0.1f)
                far_clip = near_clip + 0.1f;
            Mat4 proj = ortho_enabled
                            ? make_ortho(-camera_distance * aspect, camera_distance * aspect,
                                         -camera_distance, camera_distance, near_clip, far_clip)
                            : make_perspective(fov_degrees, aspect, near_clip, far_clip);
            for (int i = 0; i < 16; ++i)
            {
                cached_view[i] = view.m[i];
                cached_proj[i] = proj.m[i];
            }
            cached_matrices_valid = true;
        }

        ImGui::Image((void *)(intptr_t)color_texture, viewport_info.canvas_size,
                     ImVec2(0, 1), ImVec2(1, 0));

        hovered = ImGui::IsItemHovered();

        if (hovered)
        {
            handle_pan_zoom();
            if (user_prefs.tools().selected_tool == 3 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                ImVec2 mouse_pos = ImGui::GetMousePos();
                int hit_index = -1;
                float hit_radius = 8.0f;
                for (size_t i = 0; i < axioms.size(); ++i)
                {
                    ImVec2 screen_pos = world_to_screen(axioms[i].position);
                    float dx = mouse_pos.x - screen_pos.x;
                    float dy = mouse_pos.y - screen_pos.y;
                    float dist_sq = dx * dx + dy * dy;
                    float radius = std::max(hit_radius, axioms[i].size + 4.0f);
                    if (dist_sq <= radius * radius)
                    {
                        hit_index = static_cast<int>(i);
                        break;
                    }
                }

                if (hit_index >= 0)
                {
                    selected_axiom_id = axioms[hit_index].id;
                    dragging_axiom = true;
                    dragging_axiom_index = hit_index;
                }
                else
                {
                    ImVec2 world_pos = get_mouse_world_pos();
                    if (user_prefs.tools().grid_snap)
                    {
                        float snap = std::max(1.0f, user_prefs.tools().grid_size);
                        world_pos.x = roundf(world_pos.x / snap) * snap;
                        world_pos.y = roundf(world_pos.y / snap) * snap;
                    }

                    float half = static_cast<float>(grid_bounds) * 0.5f;
                    if (fabsf(world_pos.x) <= half && fabsf(world_pos.y) <= half)
                    {
                        int type_index = std::clamp(user_prefs.tools().selected_axiom_type, 0, 3);
                        user_prefs.tools().selected_axiom_type = type_index;
                        float size = std::clamp(user_prefs.tools().axiom_size, 2.0f, 30.0f);
                        float opacity = std::clamp(user_prefs.tools().axiom_opacity, 0.05f, 1.0f);
                        int id = next_axiom_id++;
                        int terminal_type = std::clamp(user_prefs.tools().selected_delta_terminal, 0, 2);
                        int mode = 0;
                        if (type_index == 0)
                            mode = std::clamp(user_prefs.tools().selected_radial_mode, 0, 2);
                        else if (type_index == 2)
                            mode = std::clamp(user_prefs.tools().selected_block_mode, 0, 2);
                        int influencer = std::clamp(user_prefs.tools().selected_influencer_type, 0, 7);
                        AxiomData axiom{id, world_pos, type_index, size, opacity, terminal_type, mode,
                                        std::string("Axiom ") + std::to_string(id), generate_axiom_tag(),
                                        3, 5, false, 5, 15, influencer}; // major=3, minor=5, fuzzy=false, min=5, max=15, influencer
                        axioms.push_back(axiom);
                        selected_axiom_id = axiom.id;
                        axiom_revision++;
                        log_axiom_placement(axiom);
                        request_regeneration();
                    }
                }
            }

            if (user_prefs.tools().selected_tool == 3 && dragging_axiom && dragging_axiom_index >= 0)
            {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    ImVec2 world_pos = get_mouse_world_pos();
                    if (user_prefs.tools().grid_snap)
                    {
                        float snap = std::max(1.0f, user_prefs.tools().grid_size);
                        world_pos.x = roundf(world_pos.x / snap) * snap;
                        world_pos.y = roundf(world_pos.y / snap) * snap;
                    }
                    axioms[dragging_axiom_index].position = world_pos;
                }
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    dragging_axiom = false;
                    dragging_axiom_index = -1;
                    axiom_revision++;
                    request_regeneration();
                }
            }

            if (user_prefs.tools().selected_tool == 4 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                ImVec2 mouse_pos = ImGui::GetMousePos();
                int hit_index = -1;
                int hit_control = -1;
                float hit_radius = 10.0f;
                bool handled_click = false;
                for (size_t i = 0; i < rivers.size(); ++i)
                {
                    if (rivers[i].control_points.empty())
                        continue;
                    for (size_t c = 0; c < rivers[i].control_points.size(); ++c)
                    {
                        ImVec2 screen_pos = world_to_screen(rivers[i].control_points[c]);
                        float dx = mouse_pos.x - screen_pos.x;
                        float dy = mouse_pos.y - screen_pos.y;
                        float dist_sq = dx * dx + dy * dy;
                        float radius = std::max(hit_radius, rivers[i].width * 0.25f + 6.0f);
                        if (dist_sq <= radius * radius)
                        {
                            hit_index = static_cast<int>(i);
                            hit_control = static_cast<int>(c);
                            break;
                        }
                    }
                    if (hit_index >= 0)
                        break;
                }

                if (hit_index >= 0)
                {
                    selected_river_id = rivers[hit_index].id;
                    selected_river_control_index = hit_control;
                    if (ImGui::GetIO().KeyCtrl && hit_control >= 0 &&
                        rivers[hit_index].control_points.size() > 2)
                    {
                        rivers[hit_index].control_points.erase(
                            rivers[hit_index].control_points.begin() + hit_control);
                        update_river_after_edit(rivers[hit_index], user_prefs.app().river_generation_mode == 0);
                        river_revision++;
                        request_regeneration();
                        handled_click = true;
                    }
                    if (!handled_click)
                    {
                        dragging_river = true;
                        dragging_river_index = hit_index;
                    }
                }
                else
                {
                    ImVec2 world_pos = get_mouse_world_pos();
                    if (user_prefs.tools().grid_snap)
                    {
                        float snap = std::max(1.0f, user_prefs.tools().grid_size);
                        world_pos.x = roundf(world_pos.x / snap) * snap;
                        world_pos.y = roundf(world_pos.y / snap) * snap;
                    }

                    float half = static_cast<float>(grid_bounds) * 0.5f;
                    if (fabsf(world_pos.x) <= half && fabsf(world_pos.y) <= half)
                    {
                        if (ImGui::GetIO().KeyShift && selected_river_id >= 0)
                        {
                            int index = find_river_index(rivers, selected_river_id);
                            if (index >= 0)
                            {
                                float best_dist = 1e9f;
                                int best_seg = -1;
                                for (size_t i = 0; i + 1 < rivers[index].control_points.size(); ++i)
                                {
                                    float d = distance_to_segment(world_pos,
                                                                  rivers[index].control_points[i],
                                                                  rivers[index].control_points[i + 1]);
                                    if (d < best_dist)
                                    {
                                        best_dist = d;
                                        best_seg = static_cast<int>(i);
                                    }
                                }
                                float insert_threshold = std::max(20.0f, rivers[index].width * 1.5f);
                                if (best_seg >= 0 && best_dist <= insert_threshold)
                                {
                                    rivers[index].control_points.insert(
                                        rivers[index].control_points.begin() + best_seg + 1, world_pos);
                                    update_river_after_edit(rivers[index], user_prefs.app().river_generation_mode == 0);
                                    river_revision++;
                                    request_regeneration();
                                    handled_click = true;
                                }
                            }
                        }

                        if (!handled_click)
                        {
                            int type_index = std::clamp(user_prefs.tools().selected_river_type, 0, 3);
                            user_prefs.tools().selected_river_type = type_index;
                            float width = std::max(1.0f, user_prefs.tools().river_width);
                            float opacity = std::clamp(user_prefs.tools().river_opacity, 0.05f, 1.0f);
                            float length = std::max(50.0f, user_prefs.tools().grid_size * 8.0f);
                            float amplitude = std::max(10.0f, width * 0.75f);
                            int id = next_river_id++;
                            ImVec2 start = ImVec2(world_pos.x - length * 0.5f, world_pos.y);
                            ImVec2 end = ImVec2(world_pos.x + length * 0.5f, world_pos.y);
                            ImVec2 mid = world_pos;
                            if (type_index == 0)
                            {
                                mid.y += amplitude;
                            }
                            else if (type_index == 2)
                            {
                                mid.y += amplitude * 0.6f;
                            }
                            else if (type_index == 3)
                            {
                                mid.y += amplitude * 0.4f;
                            }
                            std::vector<ImVec2> controls{start, mid, end};
                            bool live_mode = (user_prefs.app().river_generation_mode == 0);
                            RiverData river{
                                id,
                                controls,
                                live_mode ? build_river_spline(controls, 8) : std::vector<ImVec2>{},
                                type_index,
                                width,
                                opacity,
                                std::string("River ") + std::to_string(id),
                                !live_mode};
                            rivers.push_back(river);
                            selected_river_id = river.id;
                            selected_river_control_index = 1;
                            river_revision++;
                            log_river_placement(river);
                            request_regeneration();
                        }
                    }
                }
            }

            if (user_prefs.tools().selected_tool == 4 && dragging_river && dragging_river_index >= 0)
            {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    ImVec2 world_pos = get_mouse_world_pos();
                    ImVec2 target = world_pos;
                    if (user_prefs.tools().grid_snap)
                    {
                        float snap = std::max(1.0f, user_prefs.tools().grid_size);
                        target.x = roundf(target.x / snap) * snap;
                        target.y = roundf(target.y / snap) * snap;
                    }
                    if (!rivers[dragging_river_index].control_points.empty())
                    {
                        int control_index = selected_river_control_index;
                        if (control_index < 0 ||
                            control_index >= static_cast<int>(rivers[dragging_river_index].control_points.size()))
                        {
                            control_index = 0;
                        }
                        rivers[dragging_river_index].control_points[control_index] = target;
                        update_river_after_edit(rivers[dragging_river_index],
                                                user_prefs.app().river_generation_mode == 0);
                    }
                }
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    dragging_river = false;
                    dragging_river_index = -1;
                    river_revision++;
                }
            }

            if (user_prefs.tools().selected_tool == 6)
            {
                ImGuiIO &io = ImGui::GetIO();
                const float node_pick_radius = 8.0f;
                const float handle_pick_radius = 6.0f;

                auto find_user_road_index = [&](int road_id) -> int
                {
                    for (size_t i = 0; i < user_roads.size(); ++i)
                    {
                        if (user_roads[i].id == road_id)
                            return static_cast<int>(i);
                    }
                    return -1;
                };

                auto get_road_points = [&](int road_id, bool is_user) -> std::vector<ImVec2> *
                {
                    if (is_user)
                    {
                        int idx = find_user_road_index(road_id);
                        if (idx >= 0)
                            return &user_roads[idx].points;
                        return nullptr;
                    }
                    if (road_id >= 0 && static_cast<size_t>(road_id) < all_roads_indexed.size())
                    {
                        return &all_roads_indexed[road_id];
                    }
                    return nullptr;
                };

                auto get_road_type = [&](int road_id, bool is_user) -> CityModel::RoadType
                {
                    if (is_user)
                    {
                        int idx = find_user_road_index(road_id);
                        if (idx >= 0)
                            return user_roads[idx].type;
                    }
                    if (road_id >= 0 && static_cast<size_t>(road_id) < all_roads_refs.size())
                    {
                        return all_roads_refs[road_id].type;
                    }
                    return CityModel::RoadType::Street;
                };

                auto update_generated_ref = [&](int road_id)
                {
                    if (road_id < 0 || static_cast<size_t>(road_id) >= all_roads_refs.size())
                        return;
                    const auto &ref = all_roads_refs[road_id];
                    if (ref.type_index >= city_roads_by_type.size())
                        return;
                    auto &bucket = city_roads_by_type[ref.type_index];
                    if (ref.local_index >= bucket.size())
                        return;
                    bucket[ref.local_index] = all_roads_indexed[road_id];
                };

                auto apply_snapping = [&](ImVec2 pos, int road_id, int node_index, bool is_user) -> ImVec2
                {
                    ImVec2 snapped = pos;
                    const float snap_dist = std::max(1.0f, user_prefs.app().road_snap_distance);
                    const float snap_dist_sq = snap_dist * snap_dist;

                    if (user_prefs.app().road_snap_to_grid)
                    {
                        float snap = std::max(1.0f, user_prefs.tools().grid_size);
                        snapped.x = roundf(snapped.x / snap) * snap;
                        snapped.y = roundf(snapped.y / snap) * snap;
                    }

                    if (user_prefs.app().road_snap_to_axioms)
                    {
                        float best_sq = snap_dist_sq;
                        bool found = false;
                        for (const auto &ax : axioms)
                        {
                            float d = dist_sq(ax.position, snapped);
                            if (d <= best_sq)
                            {
                                best_sq = d;
                                snapped = ax.position;
                                found = true;
                            }
                        }
                        (void)found;
                    }

                    if (user_prefs.app().road_dynamic_snap)
                    {
                        float best_sq = snap_dist_sq;
                        ImVec2 best = snapped;
                        bool found = false;
                        for (const auto &road : user_roads)
                        {
                            for (size_t i = 0; i < road.points.size(); ++i)
                            {
                                if (road.id == road_id && is_user && static_cast<int>(i) == node_index)
                                    continue;
                                float d = dist_sq(road.points[i], snapped);
                                if (d <= best_sq)
                                {
                                    best_sq = d;
                                    best = road.points[i];
                                    found = true;
                                }
                            }
                        }
                        for (size_t r = 0; r < all_roads_indexed.size(); ++r)
                        {
                            if (hidden_generated_road_ids.find(static_cast<int>(r)) != hidden_generated_road_ids.end())
                                continue;
                            const auto &line = all_roads_indexed[r];
                            for (size_t i = 0; i < line.size(); ++i)
                            {
                                if (static_cast<int>(r) == road_id && !is_user && static_cast<int>(i) == node_index)
                                    continue;
                                float d = dist_sq(line[i], snapped);
                                if (d <= best_sq)
                                {
                                    best_sq = d;
                                    best = line[i];
                                    found = true;
                                }
                            }
                        }
                        if (found)
                        {
                            snapped = best;
                        }
                    }

                    return snapped;
                };

                auto find_node_hit = [&](int &out_road_id, int &out_node_index, bool &out_is_user) -> bool
                {
                    ImVec2 mouse_pos = ImGui::GetMousePos();

                    auto check_nodes = [&](int road_id, const std::vector<ImVec2> &points, bool is_user) -> bool
                    {
                        for (size_t i = 0; i < points.size(); ++i)
                        {
                            ImVec2 screen_pos = world_to_screen(points[i]);
                            float dx = mouse_pos.x - screen_pos.x;
                            float dy = mouse_pos.y - screen_pos.y;
                            float dist2 = dx * dx + dy * dy;
                            if (dist2 <= node_pick_radius * node_pick_radius)
                            {
                                out_road_id = road_id;
                                out_node_index = static_cast<int>(i);
                                out_is_user = is_user;
                                return true;
                            }
                        }
                        return false;
                    };

                    if (selected_road_id >= 0)
                    {
                        int user_idx = find_user_road_index(selected_road_id);
                        if (user_idx >= 0)
                        {
                            if (check_nodes(user_roads[user_idx].id, user_roads[user_idx].points, true))
                                return true;
                        }
                        if (static_cast<size_t>(selected_road_id) < all_roads_indexed.size() &&
                            hidden_generated_road_ids.find(selected_road_id) == hidden_generated_road_ids.end())
                        {
                            if (check_nodes(selected_road_id, all_roads_indexed[selected_road_id], false))
                                return true;
                        }
                    }

                    for (const auto &road : user_roads)
                    {
                        if (check_nodes(road.id, road.points, true))
                            return true;
                    }
                    for (size_t r = 0; r < all_roads_indexed.size(); ++r)
                    {
                        if (hidden_generated_road_ids.find(static_cast<int>(r)) != hidden_generated_road_ids.end())
                            continue;
                        if (check_nodes(static_cast<int>(r), all_roads_indexed[r], false))
                            return true;
                    }
                    return false;
                };

                auto find_handle_hit = [&](int road_id, int &out_node_index, bool &out_in_handle) -> bool
                {
                    int idx = find_user_road_index(road_id);
                    if (idx < 0)
                    {
                        return false;
                    }
                    auto &road = user_roads[idx];
                    ensure_user_road_handles(road_id, road.points);
                    auto *handles = get_user_road_handles(road_id);
                    if (!handles)
                    {
                        return false;
                    }

                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    float best = handle_pick_radius * handle_pick_radius;
                    bool found = false;

                    for (size_t i = 0; i < road.points.size(); ++i)
                    {
                        if (i > 0)
                        {
                            ImVec2 screen_pos = world_to_screen(handles->in_handles[i]);
                            float dx = mouse_pos.x - screen_pos.x;
                            float dy = mouse_pos.y - screen_pos.y;
                            float dist2 = dx * dx + dy * dy;
                            if (dist2 <= best)
                            {
                                best = dist2;
                                out_node_index = static_cast<int>(i);
                                out_in_handle = true;
                                found = true;
                            }
                        }
                        if (i + 1 < road.points.size())
                        {
                            ImVec2 screen_pos = world_to_screen(handles->out_handles[i]);
                            float dx = mouse_pos.x - screen_pos.x;
                            float dy = mouse_pos.y - screen_pos.y;
                            float dist2 = dx * dx + dy * dy;
                            if (dist2 <= best)
                            {
                                best = dist2;
                                out_node_index = static_cast<int>(i);
                                out_in_handle = false;
                                found = true;
                            }
                        }
                    }

                    return found;
                };

                auto find_nearest_road = [&](int &out_road_id, bool &out_is_user) -> bool
                {
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    float best = 6.0f;
                    bool found = false;

                    auto check_line = [&](int road_id, const std::vector<ImVec2> &line, bool is_user)
                    {
                        for (size_t i = 0; i + 1 < line.size(); ++i)
                        {
                            ImVec2 a = world_to_screen(line[i]);
                            ImVec2 b = world_to_screen(line[i + 1]);
                            float d = distance_to_segment(mouse_pos, a, b);
                            if (d <= best)
                            {
                                best = d;
                                out_road_id = road_id;
                                out_is_user = is_user;
                                found = true;
                            }
                        }
                    };

                    for (const auto &road : user_roads)
                    {
                        const auto &display_points = get_user_road_display_points(road.id, road.points);
                        check_line(road.id, display_points, true);
                    }
                    for (size_t r = 0; r < all_roads_indexed.size(); ++r)
                    {
                        if (hidden_generated_road_ids.find(static_cast<int>(r)) != hidden_generated_road_ids.end())
                            continue;
                        check_line(static_cast<int>(r), all_roads_indexed[r], false);
                    }
                    return found;
                };

                auto queue_edit_event = [&](int road_id, bool is_user)
                {
                    auto *points = get_road_points(road_id, is_user);
                    if (!points)
                        return;
                    RoadEditEvent evt;
                    evt.road_id = road_id;
                    evt.from_user = is_user;
                    evt.type = get_road_type(road_id, is_user);
                    evt.points = *points;
                    pending_road_edit = std::move(evt);
                    road_edit_pending = true;
                };

                if (selected_road_id >= 0)
                {
                    selected_road_is_user = (find_user_road_index(selected_road_id) >= 0);
                }
                else
                {
                    selected_road_is_user = false;
                }
                if (!dragging_road_node && selected_road_id != last_selected_road_id)
                {
                    selected_road_node_index = -1;
                    last_selected_road_id = selected_road_id;
                }

                bool handled_click = false;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    if (selected_road_id >= 0)
                    {
                        int handle_index = -1;
                        bool handle_in = false;
                        if (find_handle_hit(selected_road_id, handle_index, handle_in))
                        {
                            dragging_road_handle = true;
                            dragging_road_handle_index = handle_index;
                            dragging_road_handle_in = handle_in;
                            dragging_road_handle_road_id = selected_road_id;
                            handled_click = true;
                        }
                    }
                }

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.KeyAlt && !handled_click)
                {
                    int hit_road = -1;
                    int hit_node = -1;
                    bool hit_user = false;

                    if (find_node_hit(hit_road, hit_node, hit_user))
                    {
                        auto *points = get_road_points(hit_road, hit_user);
                        if (points && points->size() > 2 && hit_node >= 0 && static_cast<size_t>(hit_node) < points->size())
                        {
                            points->erase(points->begin() + hit_node);
                            if (hit_user)
                            {
                                ensure_user_road_handles(hit_road, *points);
                                auto *handles = get_user_road_handles(hit_road);
                                if (handles && hit_node < static_cast<int>(handles->anchors.size()))
                                {
                                    handles->anchors.erase(handles->anchors.begin() + hit_node);
                                    handles->in_handles.erase(handles->in_handles.begin() + hit_node);
                                    handles->out_handles.erase(handles->out_handles.begin() + hit_node);
                                    handles->broken.erase(handles->broken.begin() + hit_node);
                                    handles->dirty = true;
                                }
                            }
                            if (!hit_user)
                            {
                                update_generated_ref(hit_road);
                            }
                            selected_road_id = hit_road;
                            selected_road_is_user = hit_user;
                            selected_road_node_index = std::min(hit_node, static_cast<int>(points->size()) - 1);
                            queue_edit_event(hit_road, hit_user);
                            request_regeneration();
                            handled_click = true;
                        }
                    }
                }

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.KeyCtrl && !handled_click)
                {
                    int hit_road = -1;
                    int hit_node = -1;
                    bool hit_user = false;
                    if (find_node_hit(hit_road, hit_node, hit_user) && hit_user)
                    {
                        auto *points = get_road_points(hit_road, true);
                        if (points && hit_node >= 0 && static_cast<size_t>(hit_node) < points->size())
                        {
                            ensure_user_road_handles(hit_road, *points);
                            auto *handles = get_user_road_handles(hit_road);
                            if (handles)
                            {
                                ImVec2 anchor = (*points)[hit_node];
                                if (hit_node > 0)
                                {
                                    ImVec2 prev = (*points)[hit_node - 1];
                                    ImVec2 dir{anchor.x - prev.x, anchor.y - prev.y};
                                    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                                    if (len > 1e-4f)
                                    {
                                        dir.x /= len;
                                        dir.y /= len;
                                        handles->in_handles[hit_node] = ImVec2(anchor.x - dir.x * len * 0.33f,
                                                                               anchor.y - dir.y * len * 0.33f);
                                    }
                                    else
                                    {
                                        handles->in_handles[hit_node] = anchor;
                                    }
                                }
                                else
                                {
                                    handles->in_handles[hit_node] = anchor;
                                }

                                if (hit_node + 1 < static_cast<int>(points->size()))
                                {
                                    ImVec2 next = (*points)[hit_node + 1];
                                    ImVec2 dir{next.x - anchor.x, next.y - anchor.y};
                                    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                                    if (len > 1e-4f)
                                    {
                                        dir.x /= len;
                                        dir.y /= len;
                                        handles->out_handles[hit_node] = ImVec2(anchor.x + dir.x * len * 0.33f,
                                                                                anchor.y + dir.y * len * 0.33f);
                                    }
                                    else
                                    {
                                        handles->out_handles[hit_node] = anchor;
                                    }
                                }
                                else
                                {
                                    handles->out_handles[hit_node] = anchor;
                                }

                                handles->broken[hit_node] = false;
                                handles->dirty = true;
                            }
                            selected_road_id = hit_road;
                            selected_road_is_user = true;
                            selected_road_node_index = hit_node;
                            handled_click = true;
                        }
                    }
                }

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !handled_click)
                {
                    int hit_road = -1;
                    int hit_node = -1;
                    bool hit_user = false;
                    if (find_node_hit(hit_road, hit_node, hit_user))
                    {
                        // Clicking on a node: always select the road for editing
                        if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
                        {
                            selected_road_ids.clear();
                            active_road_id = hit_road;
                        }
                        else if (ImGui::GetIO().KeyCtrl)
                        {
                            selected_road_ids.insert(hit_road);
                            active_road_id = hit_road;
                        }
                        selected_road_id = hit_road;
                        selected_road_node_index = hit_node;
                        selected_road_is_user = hit_user;
                        dragging_road_node = true;
                        dragging_road_id = hit_road;
                        dragging_road_node_index = hit_node;
                        dragging_road_is_user = hit_user;
                        road_edit_dirty = false;
                    }
                    else
                    {
                        int picked_road = -1;
                        bool picked_user = false;
                        if (find_nearest_road(picked_road, picked_user))
                        {
                            // Clicked on road body: apply selection rules
                            if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
                            {
                                selected_road_ids.clear();
                                active_road_id = picked_road;
                                selected_road_id = picked_road;
                            }
                            else if (ImGui::GetIO().KeyCtrl)
                            {
                                // Toggle in multi-select
                                if (selected_road_ids.count(picked_road))
                                    selected_road_ids.erase(picked_road);
                                else
                                    selected_road_ids.insert(picked_road);
                                active_road_id = picked_road;
                                selected_road_id = picked_road;
                            }
                            selected_road_node_index = -1;
                            selected_road_is_user = picked_user;
                        }
                    }
                }

                if (dragging_road_node)
                {
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                    {
                        ImVec2 world_pos = get_mouse_world_pos();
                        ImVec2 target = apply_snapping(world_pos, dragging_road_id, dragging_road_node_index, dragging_road_is_user);
                        auto *points = get_road_points(dragging_road_id, dragging_road_is_user);
                        if (points && dragging_road_node_index >= 0 &&
                            dragging_road_node_index < static_cast<int>(points->size()))
                        {
                            ImVec2 prev = (*points)[dragging_road_node_index];
                            (*points)[dragging_road_node_index] = target;
                            if (dragging_road_is_user)
                            {
                                ensure_user_road_handles(dragging_road_id, *points);
                                auto *handles = get_user_road_handles(dragging_road_id);
                                if (handles && dragging_road_node_index < static_cast<int>(handles->anchors.size()))
                                {
                                    ImVec2 delta{target.x - prev.x, target.y - prev.y};
                                    handles->anchors[dragging_road_node_index] = target;
                                    handles->in_handles[dragging_road_node_index].x += delta.x;
                                    handles->in_handles[dragging_road_node_index].y += delta.y;
                                    handles->out_handles[dragging_road_node_index].x += delta.x;
                                    handles->out_handles[dragging_road_node_index].y += delta.y;
                                    handles->dirty = true;
                                }
                            }
                            road_edit_dirty = true;
                            if (!dragging_road_is_user)
                            {
                                update_generated_ref(dragging_road_id);
                            }
                        }
                    }
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                    {
                        if (road_edit_dirty)
                        {
                            queue_edit_event(dragging_road_id, dragging_road_is_user);
                            request_regeneration();
                        }
                        dragging_road_node = false;
                        dragging_road_node_index = -1;
                        dragging_road_id = -1;
                        dragging_road_is_user = false;
                        road_edit_dirty = false;
                    }
                }

                if (dragging_road_handle)
                {
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                    {
                        int idx = find_user_road_index(dragging_road_handle_road_id);
                        if (idx >= 0)
                        {
                            auto &road = user_roads[idx];
                            ensure_user_road_handles(dragging_road_handle_road_id, road.points);
                            auto *handles = get_user_road_handles(dragging_road_handle_road_id);
                            if (handles && dragging_road_handle_index >= 0 &&
                                dragging_road_handle_index < static_cast<int>(road.points.size()))
                            {
                                ImVec2 anchor = road.points[dragging_road_handle_index];
                                ImVec2 target = get_mouse_world_pos();

                                if (dragging_road_handle_in)
                                {
                                    handles->in_handles[dragging_road_handle_index] = target;
                                    if (!io.KeyAlt && !handles->broken[dragging_road_handle_index])
                                    {
                                        ImVec2 delta{anchor.x - target.x, anchor.y - target.y};
                                        handles->out_handles[dragging_road_handle_index] = ImVec2(anchor.x + delta.x, anchor.y + delta.y);
                                    }
                                }
                                else
                                {
                                    handles->out_handles[dragging_road_handle_index] = target;
                                    if (!io.KeyAlt && !handles->broken[dragging_road_handle_index])
                                    {
                                        ImVec2 delta{anchor.x - target.x, anchor.y - target.y};
                                        handles->in_handles[dragging_road_handle_index] = ImVec2(anchor.x + delta.x, anchor.y + delta.y);
                                    }
                                }

                                if (io.KeyAlt)
                                {
                                    handles->broken[dragging_road_handle_index] = true;
                                }
                                handles->dirty = true;
                            }
                        }
                    }
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                    {
                        dragging_road_handle = false;
                        dragging_road_handle_index = -1;
                        dragging_road_handle_road_id = -1;
                        dragging_road_handle_in = true;
                    }
                }

                if (selected_road_node_index >= 0 && io.WantCaptureKeyboard == false &&
                    ImGui::IsKeyPressed(ImGuiKey_Delete))
                {
                    auto *points = get_road_points(selected_road_id, selected_road_is_user);
                    if (points && points->size() > 2)
                    {
                        int deleted_index = selected_road_node_index;
                        points->erase(points->begin() + selected_road_node_index);
                        selected_road_node_index = std::min(selected_road_node_index, static_cast<int>(points->size()) - 1);
                        if (selected_road_is_user)
                        {
                            ensure_user_road_handles(selected_road_id, *points);
                            auto *handles = get_user_road_handles(selected_road_id);
                            if (handles && deleted_index >= 0 &&
                                deleted_index < static_cast<int>(handles->anchors.size()))
                            {
                                handles->anchors.erase(handles->anchors.begin() + deleted_index);
                                handles->in_handles.erase(handles->in_handles.begin() + deleted_index);
                                handles->out_handles.erase(handles->out_handles.begin() + deleted_index);
                                handles->broken.erase(handles->broken.begin() + deleted_index);
                                handles->dirty = true;
                            }
                        }
                        if (!selected_road_is_user)
                        {
                            update_generated_ref(selected_road_id);
                        }
                        queue_edit_event(selected_road_id, selected_road_is_user);
                        request_regeneration();
                    }
                }
            }

            if (user_prefs.tools().selected_tool == 7)
            {
                // Helper to find nearest user lot
                auto find_nearest_user_lot = [&](int &out_id, float &out_dist) -> bool
                {
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    float best = 15.0f;
                    bool found = false;
                    for (const auto &lot : user_lots)
                    {
                        ImVec2 screen_pos = world_to_screen(lot.position);
                        float dx = mouse_pos.x - screen_pos.x;
                        float dy = mouse_pos.y - screen_pos.y;
                        float dist = std::sqrt(dx * dx + dy * dy);
                        if (dist <= best)
                        {
                            best = dist;
                            out_id = lot.id;
                            out_dist = dist;
                            found = true;
                        }
                    }
                    return found;
                };

                // Helper to find nearest user building
                auto find_nearest_user_building = [&](int &out_id, float &out_dist) -> bool
                {
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    float best = 15.0f;
                    bool found = false;
                    for (const auto &building : user_buildings)
                    {
                        ImVec2 screen_pos = world_to_screen(building.position);
                        float dx = mouse_pos.x - screen_pos.x;
                        float dy = mouse_pos.y - screen_pos.y;
                        float dist = std::sqrt(dx * dx + dy * dy);
                        if (dist <= best)
                        {
                            best = dist;
                            out_id = building.id;
                            out_dist = dist;
                            found = true;
                        }
                    }
                    return found;
                };

                // Find nearest generated lot (existing behavior)
                auto find_nearest_lot = [&](int &out_id) -> bool
                {
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    float best = 8.0f;
                    bool found = false;
                    for (const auto &lot : city_lots)
                    {
                        ImVec2 screen_pos = world_to_screen(lot.centroid);
                        float dx = mouse_pos.x - screen_pos.x;
                        float dy = mouse_pos.y - screen_pos.y;
                        float dist2 = dx * dx + dy * dy;
                        if (dist2 <= best * best)
                        {
                            best = std::sqrt(dist2);
                            out_id = static_cast<int>(lot.id);
                            found = true;
                        }
                    }
                    return found;
                };

                // Hover detection for generated lots
                if (has_city && selected_lot_ids.empty())
                {
                    int hover_id = -1;
                    if (find_nearest_lot(hover_id))
                    {
                        hovered_lot_id = hover_id;
                    }
                    else
                    {
                        hovered_lot_id = -1;
                    }
                }

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    ImVec2 world_pos = get_mouse_world_pos();
                    if (user_prefs.tools().grid_snap)
                    {
                        float snap = std::max(1.0f, user_prefs.tools().grid_size);
                        world_pos.x = roundf(world_pos.x / snap) * snap;
                        world_pos.y = roundf(world_pos.y / snap) * snap;
                    }

                    float half = static_cast<float>(grid_bounds) * 0.5f;
                    bool in_bounds = (fabsf(world_pos.x) <= half && fabsf(world_pos.y) <= half);

                    if (ImGui::GetIO().KeyShift)
                    {
                        // Shift+Click: Remove nearest user lot or building
                        int lot_id = -1, building_id = -1;
                        float lot_dist = 1e9f, building_dist = 1e9f;
                        bool found_lot = find_nearest_user_lot(lot_id, lot_dist);
                        bool found_building = find_nearest_user_building(building_id, building_dist);

                        if (found_lot && (!found_building || lot_dist <= building_dist))
                        {
                            remove_user_lot(lot_id);
                            request_regeneration();
                        }
                        else if (found_building)
                        {
                            remove_user_building(building_id);
                            request_regeneration();
                        }
                    }
                    else if (in_bounds)
                    {
                        // Click: Place new lot or building based on mode
                        int placement_mode = user_prefs.tools().lot_placement_mode;
                        if (placement_mode == 0)
                        {
                            // Place lot
                            int lot_type_idx = std::clamp(user_prefs.tools().selected_lot_type, 0, 7);
                            // Map UI index (0-7) to LotType enum (1-8, since 0 is None)
                            CityModel::LotType lot_type = static_cast<CityModel::LotType>(lot_type_idx + 1);
                            bool locked = user_prefs.tools().lock_user_placed_types;
                            add_user_lot(world_pos, lot_type, locked);
                            request_regeneration();
                        }
                        else
                        {
                            // Place building
                            int building_type_idx = std::clamp(user_prefs.tools().selected_building_type, 0, 7);
                            // Map UI index (0-7) to BuildingType enum (1-8, since 0 is None)
                            CityModel::BuildingType building_type = static_cast<CityModel::BuildingType>(building_type_idx + 1);
                            bool locked = user_prefs.tools().lock_user_placed_types;
                            add_user_building(world_pos, building_type, locked);
                            request_regeneration();
                        }
                    }
                    else if (has_city)
                    {
                        // Click on generated lot for selection
                        int picked = -1;
                        if (find_nearest_lot(picked))
                        {
                            if (!ImGui::GetIO().KeyCtrl)
                            {
                                selected_lot_ids.clear();
                                active_lot_id = picked;
                                selected_lot_id = picked;
                            }
                            else
                            {
                                if (selected_lot_ids.count(picked))
                                    selected_lot_ids.erase(picked);
                                else
                                    selected_lot_ids.insert(picked);
                                active_lot_id = picked;
                                selected_lot_id = picked;
                            }
                        }
                        else
                        {
                            selected_lot_ids.clear();
                            selected_lot_id = -1;
                            active_lot_id = -1;
                        }
                    }
                }
            }
        }

        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_end = ImVec2(viewport_info.canvas_pos.x + viewport_info.canvas_size.x,
                                   viewport_info.canvas_pos.y + viewport_info.canvas_size.y);
        draw_list->PushClipRect(viewport_info.canvas_pos, canvas_end, true);
        // Controls hint (offset to avoid overlap with road counters)
        float hint_y_offset = has_city ? 40.0f : 8.0f;
        ImVec2 hint_pos = ImVec2(viewport_info.canvas_pos.x + 8.0f, viewport_info.canvas_pos.y + hint_y_offset);
        draw_list->AddText(hint_pos, IM_COL32(200, 200, 200, 180),
                           "MMB orbit | Shift+MMB pan | Ctrl+MMB zoom | Wheel zoom");

        if (user_prefs.app().show_axioms)
        {
            float half = static_cast<float>(grid_bounds) * 0.5f;
            for (const AxiomData &axiom : axioms)
            {
                if (fabsf(axiom.position.x) > half || fabsf(axiom.position.y) > half)
                {
                    continue;
                }
                ImVec2 screen_pos = world_to_screen(axiom.position);
                if (screen_pos.x < viewport_info.canvas_pos.x || screen_pos.y < viewport_info.canvas_pos.y ||
                    screen_pos.x > canvas_end.x || screen_pos.y > canvas_end.y)
                {
                    continue;
                }

                float size = std::max(2.0f, axiom.size);
                float alpha = std::clamp(axiom.opacity, 0.05f, 1.0f);
                int fill_alpha = static_cast<int>(220.0f * alpha);
                ImU32 highlight = IM_COL32(255, 220, 120, 220);
                if (axiom.id == selected_axiom_id)
                {
                    draw_list->AddCircle(screen_pos, size + 3.0f, highlight, 0, 2.0f);
                    float axis_len = size * 1.6f;
                    draw_list->AddLine(ImVec2(screen_pos.x - axis_len, screen_pos.y),
                                       ImVec2(screen_pos.x + axis_len, screen_pos.y),
                                       highlight, 1.5f);
                    draw_list->AddLine(ImVec2(screen_pos.x, screen_pos.y - axis_len),
                                       ImVec2(screen_pos.x, screen_pos.y + axis_len),
                                       highlight, 1.5f);
                }
                if (user_prefs.app().show_axiom_influence)
                {
                    float influence_radius = size * (alpha * 4.0f);
                    if (influence_radius > 0.5f)
                    {
                        int ring_alpha = static_cast<int>(60.0f + 120.0f * alpha);
                        ImU32 ring_color = IM_COL32(255, 255, 255, ring_alpha);
                        draw_list->AddCircle(screen_pos, influence_radius, ring_color, 0, 1.5f);
                    }
                }

                switch (static_cast<AxiomType>(axiom.type))
                {
                case AxiomType::BlueCircle:
                    draw_list->AddCircleFilled(screen_pos, size, IM_COL32(70, 140, 255, fill_alpha));
                    break;
                case AxiomType::GreenTriangle:
                default:
                    draw_list->AddTriangleFilled(ImVec2(screen_pos.x, screen_pos.y - size * 1.2f),
                                                 ImVec2(screen_pos.x - size, screen_pos.y + size),
                                                 ImVec2(screen_pos.x + size, screen_pos.y + size),
                                                 IM_COL32(80, 220, 120, fill_alpha));
                    break;
                case AxiomType::BlockSquare:
                {
                    float thickness = std::max(3.0f, size * 0.18f);
                    ImU32 block_color = IM_COL32(62, 0, 0, fill_alpha);
                    draw_list->AddRect(ImVec2(screen_pos.x - size, screen_pos.y - size),
                                       ImVec2(screen_pos.x + size, screen_pos.y + size),
                                       block_color, 0.0f, 0, thickness);
                    break;
                }
                case AxiomType::GridCorrective:
                {
                    ImU32 grid_color = IM_COL32(200, 90, 60, fill_alpha);
                    draw_list->AddRect(ImVec2(screen_pos.x - size, screen_pos.y - size),
                                       ImVec2(screen_pos.x + size, screen_pos.y + size),
                                       grid_color, 0.0f, 0, 2.0f);
                    break;
                }
                }
            }
        }

        if (!rivers.empty())
        {
            float river_half_extent = has_city ? city_half_extent : static_cast<float>(grid_bounds) * 0.5f;
            ImVec4 base_color = user_prefs.app().river_spline_color;

            if (user_prefs.app().show_river_splines)
            {
                for (const auto &river : rivers)
                {
                    if (river.spline.size() < 2)
                    {
                        continue;
                    }
                    auto &screen_points = polyline_scratch;
                    screen_points.clear();
                    screen_points.reserve(river.spline.size());
                    for (const auto &pt : river.spline)
                    {
                        ImVec2 screen_pos = world_to_screen(pt);
                        screen_points.push_back(screen_pos);
                    }
                    if (screen_points.size() >= 2)
                    {
                        ImVec4 color = base_color;
                        color.w = std::clamp(color.w * river.opacity, 0.05f, 1.0f);
                        ImU32 river_color = ImGui::ColorConvertFloat4ToU32(color);
                        float thickness = std::max(1.0f, river.width * 0.08f);
                        draw_list->AddPolyline(screen_points.data(), static_cast<int>(screen_points.size()),
                                               river_color, false, thickness);
                    }
                }
            }

            if (user_prefs.app().show_river_markers)
            {
                for (const auto &river : rivers)
                {
                    ImVec4 color = base_color;
                    color.w = std::clamp(color.w * river.opacity, 0.05f, 1.0f);
                    ImU32 river_color = ImGui::ColorConvertFloat4ToU32(color);
                    float marker = std::max(3.0f, river.width * 0.18f);
                    for (size_t c = 0; c < river.control_points.size(); ++c)
                    {
                        ImVec2 cp = river.control_points[c];
                        if (fabsf(cp.x) > river_half_extent || fabsf(cp.y) > river_half_extent)
                        {
                            continue;
                        }
                        ImVec2 cp_screen = world_to_screen(cp);
                        if (cp_screen.x < viewport_info.canvas_pos.x || cp_screen.y < viewport_info.canvas_pos.y ||
                            cp_screen.x > canvas_end.x || cp_screen.y > canvas_end.y)
                        {
                            continue;
                        }
                        ImU32 marker_color = river_color;
                        if (river.id == selected_river_id && static_cast<int>(c) == selected_river_control_index)
                        {
                            marker_color = IM_COL32(255, 220, 120, 230);
                        }
                        draw_list->AddRectFilled(ImVec2(cp_screen.x - marker, cp_screen.y - marker),
                                                 ImVec2(cp_screen.x + marker, cp_screen.y + marker),
                                                 marker_color);
                    }
                }
            }
        }

        if (user_prefs.app().show_roads)
        {
            auto draw_single = [&](const std::vector<ImVec2> &line, ImU32 color, float thickness)
            {
                if (line.size() < 2)
                    return;
                auto &screen_points = polyline_scratch;
                screen_points.clear();
                screen_points.reserve(line.size());
                for (const auto &pt : line)
                {
                    if (has_city && (fabsf(pt.x) > city_half_extent || fabsf(pt.y) > city_half_extent))
                    {
                        continue;
                    }
                    ImVec2 screen_pos = world_to_screen(pt);
                    screen_points.push_back(screen_pos);
                }
                if (screen_points.size() >= 2)
                {
                    draw_list->AddPolyline(screen_points.data(), static_cast<int>(screen_points.size()), color, false, thickness);
                }
            };

            auto to_u32 = [](const ImVec4 &c)
            {
                return IM_COL32(static_cast<int>(c.x * 255.0f),
                                static_cast<int>(c.y * 255.0f),
                                static_cast<int>(c.z * 255.0f),
                                static_cast<int>(c.w * 255.0f));
            };

            auto road_color = [&](CityModel::RoadType type)
            {
                switch (type)
                {
                case CityModel::RoadType::Highway:
                    return to_u32(user_prefs.display().highway_road_color);
                case CityModel::RoadType::Arterial:
                    return to_u32(user_prefs.display().arterial_road_color);
                case CityModel::RoadType::Avenue:
                    return to_u32(user_prefs.display().avenue_road_color);
                case CityModel::RoadType::Boulevard:
                    return to_u32(user_prefs.display().boulevard_road_color);
                case CityModel::RoadType::Street:
                    return to_u32(user_prefs.display().street_road_color);
                case CityModel::RoadType::Lane:
                    return to_u32(user_prefs.display().lane_road_color);
                case CityModel::RoadType::Alleyway:
                    return to_u32(user_prefs.display().alleyway_road_color);
                case CityModel::RoadType::CulDeSac:
                    return to_u32(user_prefs.display().culdesac_road_color);
                case CityModel::RoadType::Drive:
                    return to_u32(user_prefs.display().drive_road_color);
                case CityModel::RoadType::Driveway:
                    return to_u32(user_prefs.display().driveway_road_color);
                case CityModel::RoadType::M_Major:
                    return to_u32(user_prefs.display().user_major_road_color);
                case CityModel::RoadType::M_Minor:
                    return to_u32(user_prefs.display().user_minor_road_color);
                default:
                    return to_u32(user_prefs.display().street_road_color);
                }
            };

            auto road_thickness = [&](CityModel::RoadType type)
            {
                switch (type)
                {
                case CityModel::RoadType::Highway:
                    return 3.5f;
                case CityModel::RoadType::Arterial:
                    return 3.0f;
                case CityModel::RoadType::Avenue:
                    return 2.6f;
                case CityModel::RoadType::Boulevard:
                    return 2.3f;
                case CityModel::RoadType::Street:
                    return 2.0f;
                case CityModel::RoadType::Lane:
                    return 1.7f;
                case CityModel::RoadType::Alleyway:
                    return 1.4f;
                case CityModel::RoadType::CulDeSac:
                    return 1.6f;
                case CityModel::RoadType::Drive:
                    return 1.5f;
                case CityModel::RoadType::Driveway:
                    return 1.2f;
                default:
                    return 1.5f;
                }
            };

            if (has_city)
            {
                for (const auto &line : city_water)
                {
                    draw_single(line, IM_COL32(80, 180, 255, 200), 2.0f);
                }
                for (size_t i = 0; i < all_roads_indexed.size(); ++i)
                {
                    if (hidden_generated_road_ids.find(static_cast<int>(i)) != hidden_generated_road_ids.end())
                    {
                        continue;
                    }
                    const auto &line = all_roads_indexed[i];
                    const auto &ref = all_roads_refs[i];
                    draw_single(line, road_color(ref.type), road_thickness(ref.type));
                }
            }

            for (const auto &road : user_roads)
            {
                const auto &display_points = get_user_road_display_points(road.id, road.points);
                draw_single(display_points, road_color(road.type), road_thickness(road.type) + 0.3f);
            }

            // Draw selected road highlight(s) or hover preview
            if (user_prefs.app().show_selected_road_overlay)
            {
                // Determine what road(s) to highlight
                std::unordered_set<int> roads_to_highlight;

                if (!selected_road_ids.empty())
                {
                    // Selection set is non-empty: highlight all selected roads
                    roads_to_highlight = selected_road_ids;
                }
                else if (hovered_road_id >= 0)
                {
                    // Selection set is empty and we have a hovered road: highlight hover preview
                    roads_to_highlight.insert(hovered_road_id);
                }
                else if (selected_road_id >= 0)
                {
                    // Fallback for backward compatibility: highlight active/selected
                    roads_to_highlight.insert(selected_road_id);
                }

                for (int road_id : roads_to_highlight)
                {
                    const std::vector<ImVec2> *road_points = nullptr;
                    for (const auto &road : user_roads)
                    {
                        if (road.id == road_id)
                        {
                            road_points = &get_user_road_display_points(road.id, road.points);
                            break;
                        }
                    }
                    if (!road_points && static_cast<size_t>(road_id) < all_roads_indexed.size())
                    {
                        road_points = &all_roads_indexed[road_id];
                    }

                    if (road_points)
                    {
                        const ImVec4 &col = user_prefs.app().selected_road_color;
                        ImU32 highlight_color = IM_COL32(
                            static_cast<int>(col.x * 255.0f),
                            static_cast<int>(col.y * 255.0f),
                            static_cast<int>(col.z * 255.0f),
                            static_cast<int>(col.w * 255.0f));

                        std::vector<ImVec2> screen_points;
                        screen_points.reserve(road_points->size());
                        for (const auto &pt : *road_points)
                        {
                            if (has_city && (fabsf(pt.x) > city_half_extent || fabsf(pt.y) > city_half_extent))
                                continue;
                            ImVec2 screen_pos = world_to_screen(pt);
                            if (screen_pos.x >= viewport_info.canvas_pos.x && screen_pos.y >= viewport_info.canvas_pos.y &&
                                screen_pos.x <= canvas_end.x && screen_pos.y <= canvas_end.y)
                            {
                                screen_points.push_back(screen_pos);
                            }
                        }
                        if (screen_points.size() >= 2)
                        {
                            draw_list->AddPolyline(screen_points.data(), static_cast<int>(screen_points.size()),
                                                   highlight_color, false, 4.0f);
                        }
                    }
                }
            }
        }

        if (has_city && user_prefs.app().show_district_borders && !city_districts.empty())
        {
            auto district_color = [](CityModel::DistrictType type) -> ImU32
            {
                switch (type)
                {
                case CityModel::DistrictType::Residential:
                    return IM_COL32(110, 190, 120, 160);
                case CityModel::DistrictType::Commercial:
                    return IM_COL32(220, 170, 90, 160);
                case CityModel::DistrictType::Civic:
                    return IM_COL32(120, 160, 220, 160);
                case CityModel::DistrictType::Industrial:
                    return IM_COL32(180, 180, 120, 160);
                case CityModel::DistrictType::Mixed:
                default:
                    return IM_COL32(140, 140, 140, 140);
                }
            };

            for (const auto &district : city_districts)
            {
                if (district.border.size() < 2)
                {
                    continue;
                }
                auto &screen_points = polyline_scratch;
                screen_points.clear();
                screen_points.reserve(district.border.size());
                for (const auto &pt : district.border)
                {
                    if (has_city && (fabsf(pt.x) > city_half_extent || fabsf(pt.y) > city_half_extent))
                    {
                        continue;
                    }
                    ImVec2 screen_pos = world_to_screen(pt);
                    screen_points.push_back(screen_pos);
                }
                if (screen_points.size() >= 2)
                {
                    draw_list->AddPolyline(screen_points.data(),
                                           static_cast<int>(screen_points.size()),
                                           district_color(district.type),
                                           true,
                                           2.0f);
                }
            }

            std::unordered_set<int> districts_to_highlight;
            if (!selected_district_ids.empty())
            {
                districts_to_highlight = selected_district_ids;
            }
            else if (hovered_district_id >= 0)
            {
                districts_to_highlight.insert(hovered_district_id);
            }
            else if (selected_district_id >= 0)
            {
                districts_to_highlight.insert(selected_district_id);
            }

            if (!districts_to_highlight.empty())
            {
                for (const auto &district : city_districts)
                {
                    if (districts_to_highlight.count(static_cast<int>(district.id)) == 0)
                    {
                        continue;
                    }
                    if (district.border.size() < 2)
                    {
                        continue;
                    }
                    auto &screen_points = polyline_scratch;
                    screen_points.clear();
                    screen_points.reserve(district.border.size());
                    for (const auto &pt : district.border)
                    {
                        ImVec2 screen_pos = world_to_screen(pt);
                        screen_points.push_back(screen_pos);
                    }
                    if (screen_points.size() >= 2)
                    {
                        draw_list->AddPolyline(screen_points.data(),
                                               static_cast<int>(screen_points.size()),
                                               IM_COL32(255, 220, 120, 220),
                                               true,
                                               3.5f);
                    }
                }
            }
        }

        if (has_city && user_prefs.app().show_block_polygons && !city_block_polygons.empty())
        {
            auto block_color = [](CityModel::DistrictType type, bool fill) -> ImU32
            {
                switch (type)
                {
                case CityModel::DistrictType::Residential:
                    return fill ? IM_COL32(110, 190, 120, 90) : IM_COL32(110, 190, 120, 230);
                case CityModel::DistrictType::Commercial:
                    return fill ? IM_COL32(220, 170, 90, 90) : IM_COL32(220, 170, 90, 230);
                case CityModel::DistrictType::Civic:
                    return fill ? IM_COL32(120, 160, 220, 90) : IM_COL32(120, 160, 220, 230);
                case CityModel::DistrictType::Industrial:
                    return fill ? IM_COL32(180, 180, 120, 90) : IM_COL32(180, 180, 120, 230);
                case CityModel::DistrictType::Mixed:
                default:
                    return fill ? IM_COL32(140, 140, 140, 80) : IM_COL32(140, 140, 140, 220);
                }
            };
            std::vector<int> tri_indices;
            for (const auto &poly : city_block_polygons)
            {
                if (poly.outer.size() < 3)
                {
                    continue;
                }
                auto &screen_points = polyline_scratch;
                screen_points.clear();
                screen_points.reserve(poly.outer.size());
                for (const auto &pt : poly.outer)
                {
                    screen_points.push_back(world_to_screen(pt));
                }
                if (screen_points.size() >= 4)
                {
                    const ImVec2 &a = screen_points.front();
                    const ImVec2 &b = screen_points.back();
                    const float dx = a.x - b.x;
                    const float dy = a.y - b.y;
                    if ((dx * dx + dy * dy) < 1e-4f)
                    {
                        screen_points.pop_back();
                    }
                }
                if (screen_points.size() >= 3)
                {
                    const ImU32 outline_color = block_color(poly.type, false);
                    const ImU32 fill_color = block_color(poly.type, true);
                    if (triangulate_polygon(screen_points, tri_indices))
                    {
                        for (std::size_t t = 0; t + 2 < tri_indices.size(); t += 3)
                        {
                            draw_list->AddTriangleFilled(screen_points[tri_indices[t]],
                                                         screen_points[tri_indices[t + 1]],
                                                         screen_points[tri_indices[t + 2]],
                                                         fill_color);
                        }
                    }
                    draw_list->AddPolyline(screen_points.data(),
                                           static_cast<int>(screen_points.size()),
                                           outline_color,
                                           true,
                                           2.5f);
                    if (!poly.holes.empty())
                    {
                        for (const auto &hole : poly.holes)
                        {
                            if (hole.size() < 3)
                            {
                                continue;
                            }
                            std::vector<ImVec2> hole_points;
                            hole_points.reserve(hole.size());
                            for (const auto &pt : hole)
                            {
                                hole_points.push_back(world_to_screen(pt));
                            }
                            draw_list->AddPolyline(hole_points.data(),
                                                   static_cast<int>(hole_points.size()),
                                                   outline_color,
                                                   true,
                                                   2.0f);
                        }
                    }
                    for (const auto &pt : screen_points)
                    {
                        draw_list->AddCircleFilled(pt, 2.0f, outline_color);
                    }
                }
            }
        }

        if (has_city && user_prefs.app().show_block_faces_debug && !city_block_faces.empty())
        {
            auto face_color = [&](std::size_t idx, bool fill, bool closable) -> ImU32
            {
                if (user_prefs.app().show_block_closable_faces)
                {
                    // Color by closability
                    if (closable)
                    {
                        // Green for closable
                        return fill ? IM_COL32(80, 200, 80, 90) : IM_COL32(80, 220, 80, 230);
                    }
                    else
                    {
                        // Red for non-closable
                        return fill ? IM_COL32(220, 80, 80, 90) : IM_COL32(240, 80, 80, 230);
                    }
                }
                else
                {
                    // Original hash-based coloring
                    uint32_t h = static_cast<uint32_t>(idx) + 0x9e3779b9u;
                    h ^= (h >> 16);
                    h *= 0x7feb352du;
                    h ^= (h >> 15);
                    h *= 0x846ca68bu;
                    h ^= (h >> 16);
                    const unsigned char r = static_cast<unsigned char>(60 + (h & 0x9F));
                    const unsigned char g = static_cast<unsigned char>(60 + ((h >> 8) & 0x9F));
                    const unsigned char b = static_cast<unsigned char>(60 + ((h >> 16) & 0x9F));
                    const unsigned char a = fill ? 60 : 200;
                    return IM_COL32(r, g, b, a);
                }
            };

            std::vector<int> tri_indices;
            std::size_t face_index = 0;
            for (const auto &poly : city_block_faces)
            {
                if (poly.outer.size() < 3)
                {
                    face_index++;
                    continue;
                }
                auto &screen_points = polyline_scratch;
                screen_points.clear();
                screen_points.reserve(poly.outer.size());
                for (const auto &pt : poly.outer)
                {
                    screen_points.push_back(world_to_screen(pt));
                }
                if (screen_points.size() >= 4)
                {
                    const ImVec2 &a = screen_points.front();
                    const ImVec2 &b = screen_points.back();
                    const float dx = a.x - b.x;
                    const float dy = a.y - b.y;
                    if ((dx * dx + dy * dy) < 1e-4f)
                    {
                        screen_points.pop_back();
                    }
                }
                if (screen_points.size() >= 3)
                {
                    const ImU32 outline_color = face_color(face_index, false, poly.closable);
                    const ImU32 fill_color = face_color(face_index, true, poly.closable);
                    if (triangulate_polygon(screen_points, tri_indices))
                    {
                        for (std::size_t t = 0; t + 2 < tri_indices.size(); t += 3)
                        {
                            draw_list->AddTriangleFilled(screen_points[tri_indices[t]],
                                                         screen_points[tri_indices[t + 1]],
                                                         screen_points[tri_indices[t + 2]],
                                                         fill_color);
                        }
                    }
                    draw_list->AddPolyline(screen_points.data(),
                                           static_cast<int>(screen_points.size()),
                                           outline_color,
                                           true,
                                           1.5f);
                }
                face_index++;
            }
        }

        // District grid overlay (debug)
        if (has_city && user_prefs.app().show_district_grid_overlay && 
            district_grid_info.width > 0 && district_grid_info.height > 0)
        {
            const ImU32 grid_color = IM_COL32(180, 180, 180, 100);
            const ImU32 border_color = IM_COL32(255, 255, 255, 150);
            
            for (int y = 0; y <= district_grid_info.height; ++y)
            {
                const float world_y = district_grid_info.origin.y + y * district_grid_info.cell_size.y;
                const ImVec2 start = world_to_screen({
                    district_grid_info.origin.x, 
                    world_y
                });
                const ImVec2 end = world_to_screen({
                    district_grid_info.origin.x + district_grid_info.width * district_grid_info.cell_size.x,
                    world_y
                });
                draw_list->AddLine(start, end, grid_color, 0.5f);
            }
            
            for (int x = 0; x <= district_grid_info.width; ++x)
            {
                const float world_x = district_grid_info.origin.x + x * district_grid_info.cell_size.x;
                const ImVec2 start = world_to_screen({
                    world_x,
                    district_grid_info.origin.y
                });
                const ImVec2 end = world_to_screen({
                    world_x,
                    district_grid_info.origin.y + district_grid_info.height * district_grid_info.cell_size.y
                });
                draw_list->AddLine(start, end, grid_color, 0.5f);
            }
        }

        // District IDs overlay (debug)
        if (has_city && user_prefs.app().show_district_ids && 
            district_grid_info.width > 0 && district_grid_info.height > 0)
        {
            for (int y = 0; y < district_grid_info.height; ++y)
            {
                for (int x = 0; x < district_grid_info.width; ++x)
                {
                    const std::size_t idx = static_cast<std::size_t>(y) * district_grid_info.width + x;
                    if (idx < district_grid_info.district_ids.size())
                    {
                        const uint32_t district_id = district_grid_info.district_ids[idx];
                        if (district_id > 0)
                        {
                            const float world_x = district_grid_info.origin.x + (x + 0.5f) * district_grid_info.cell_size.x;
                            const float world_y = district_grid_info.origin.y + (y + 0.5f) * district_grid_info.cell_size.y;
                            const ImVec2 screen_pos = world_to_screen({world_x, world_y});
                            
                            char id_text[16];
                            std::snprintf(id_text, sizeof(id_text), "%u", district_id);
                            
                            const ImVec2 text_size = ImGui::CalcTextSize(id_text);
                            const ImVec2 text_pos(screen_pos.x - text_size.x * 0.5f, screen_pos.y - text_size.y * 0.5f);
                            
                            draw_list->AddText(text_pos, IM_COL32(255, 255, 0, 200), id_text);
                        }
                    }
                }
            }
        }

        if (has_city && !city_lots.empty())
        {
            auto lot_color = [](CityModel::LotType type) -> ImU32
            {
                switch (type)
                {
                case CityModel::LotType::Residential:
                    return IM_COL32(90, 180, 120, 200);
                case CityModel::LotType::RowhomeCompact:
                    return IM_COL32(70, 150, 120, 200);
                case CityModel::LotType::RetailStrip:
                    return IM_COL32(230, 160, 80, 200);
                case CityModel::LotType::MixedUse:
                    return IM_COL32(180, 140, 200, 200);
                case CityModel::LotType::LogisticsIndustrial:
                    return IM_COL32(190, 190, 120, 200);
                case CityModel::LotType::CivicCultural:
                    return IM_COL32(100, 150, 220, 200);
                case CityModel::LotType::LuxuryScenic:
                    return IM_COL32(220, 200, 120, 200);
                case CityModel::LotType::BufferStrip:
                    return IM_COL32(130, 130, 130, 200);
                case CityModel::LotType::None:
                default:
                    return IM_COL32(120, 120, 120, 160);
                }
            };

            for (const auto &lot : city_lots)
            {
                ImVec2 screen_pos = world_to_screen(lot.centroid);
                if (screen_pos.x < viewport_info.canvas_pos.x || screen_pos.y < viewport_info.canvas_pos.y ||
                    screen_pos.x > canvas_end.x || screen_pos.y > canvas_end.y)
                {
                    continue;
                }
                draw_list->AddCircleFilled(screen_pos, 3.0f, lot_color(lot.lot_type));
            }

            std::unordered_set<int> lots_to_highlight;
            if (!selected_lot_ids.empty())
            {
                lots_to_highlight = selected_lot_ids;
            }
            else if (hovered_lot_id >= 0)
            {
                lots_to_highlight.insert(hovered_lot_id);
            }
            else if (selected_lot_id >= 0)
            {
                lots_to_highlight.insert(selected_lot_id);
            }

            for (const auto &lot : city_lots)
            {
                if (lots_to_highlight.count(static_cast<int>(lot.id)) == 0)
                {
                    continue;
                }
                ImVec2 screen_pos = world_to_screen(lot.centroid);
                if (screen_pos.x < viewport_info.canvas_pos.x || screen_pos.y < viewport_info.canvas_pos.y ||
                    screen_pos.x > canvas_end.x || screen_pos.y > canvas_end.y)
                {
                    continue;
                }
                draw_list->AddCircle(screen_pos, 6.0f, IM_COL32(255, 220, 120, 220), 0, 2.0f);
            }
        }

        if (has_city && !city_building_sites.empty())
        {
            auto site_color = [](CityModel::BuildingType type) -> ImU32
            {
                switch (type)
                {
                case CityModel::BuildingType::Residential:
                    return IM_COL32(90, 220, 140, 230);
                case CityModel::BuildingType::Rowhome:
                    return IM_COL32(70, 190, 140, 230);
                case CityModel::BuildingType::Retail:
                    return IM_COL32(240, 170, 90, 230);
                case CityModel::BuildingType::MixedUse:
                    return IM_COL32(190, 150, 210, 230);
                case CityModel::BuildingType::Industrial:
                    return IM_COL32(200, 200, 130, 230);
                case CityModel::BuildingType::Civic:
                    return IM_COL32(120, 170, 240, 230);
                case CityModel::BuildingType::Luxury:
                    return IM_COL32(240, 210, 140, 230);
                case CityModel::BuildingType::Utility:
                    return IM_COL32(170, 170, 170, 230);
                case CityModel::BuildingType::None:
                default:
                    return IM_COL32(160, 160, 160, 200);
                }
            };

            for (const auto &site : city_building_sites)
            {
                ImVec2 screen_pos = world_to_screen(site.position);
                if (screen_pos.x < viewport_info.canvas_pos.x || screen_pos.y < viewport_info.canvas_pos.y ||
                    screen_pos.x > canvas_end.x || screen_pos.y > canvas_end.y)
                {
                    continue;
                }
                draw_list->AddCircleFilled(screen_pos, 2.2f, site_color(site.type));
                draw_list->AddCircle(screen_pos, 3.2f, IM_COL32(20, 20, 20, 200), 0, 1.0f);
            }
        }

        // Draw user-placed lots (diamond shape to distinguish from generated)
        if (!user_lots.empty())
        {
            auto lot_color = [](CityModel::LotType type) -> ImU32
            {
                switch (type)
                {
                case CityModel::LotType::Residential:
                    return IM_COL32(90, 180, 120, 230);
                case CityModel::LotType::RowhomeCompact:
                    return IM_COL32(70, 150, 120, 230);
                case CityModel::LotType::RetailStrip:
                    return IM_COL32(230, 160, 80, 230);
                case CityModel::LotType::MixedUse:
                    return IM_COL32(180, 140, 200, 230);
                case CityModel::LotType::LogisticsIndustrial:
                    return IM_COL32(190, 190, 120, 230);
                case CityModel::LotType::CivicCultural:
                    return IM_COL32(100, 150, 220, 230);
                case CityModel::LotType::LuxuryScenic:
                    return IM_COL32(220, 200, 120, 230);
                case CityModel::LotType::BufferStrip:
                    return IM_COL32(130, 130, 130, 230);
                case CityModel::LotType::None:
                default:
                    return IM_COL32(120, 120, 120, 180);
                }
            };

            for (const auto &lot : user_lots)
            {
                ImVec2 screen_pos = world_to_screen(lot.position);
                if (screen_pos.x < viewport_info.canvas_pos.x || screen_pos.y < viewport_info.canvas_pos.y ||
                    screen_pos.x > canvas_end.x || screen_pos.y > canvas_end.y)
                {
                    continue;
                }
                // Draw diamond shape for user lots
                float size = 5.0f;
                ImVec2 diamond[4] = {
                    ImVec2(screen_pos.x, screen_pos.y - size), // top
                    ImVec2(screen_pos.x + size, screen_pos.y), // right
                    ImVec2(screen_pos.x, screen_pos.y + size), // bottom
                    ImVec2(screen_pos.x - size, screen_pos.y)  // left
                };
                draw_list->AddConvexPolyFilled(diamond, 4, lot_color(lot.lot_type));
                // Add outline
                ImU32 outline_color = lot.locked_type ? IM_COL32(255, 200, 80, 255) : IM_COL32(40, 40, 40, 220);
                draw_list->AddPolyline(diamond, 4, outline_color, ImDrawFlags_Closed, lot.locked_type ? 2.0f : 1.2f);
            }
        }

        // Draw user-placed buildings (square shape to distinguish from generated)
        if (!user_buildings.empty())
        {
            auto building_color = [](CityModel::BuildingType type) -> ImU32
            {
                switch (type)
                {
                case CityModel::BuildingType::Residential:
                    return IM_COL32(90, 220, 140, 240);
                case CityModel::BuildingType::Rowhome:
                    return IM_COL32(70, 190, 140, 240);
                case CityModel::BuildingType::Retail:
                    return IM_COL32(240, 170, 90, 240);
                case CityModel::BuildingType::MixedUse:
                    return IM_COL32(190, 150, 210, 240);
                case CityModel::BuildingType::Industrial:
                    return IM_COL32(200, 200, 130, 240);
                case CityModel::BuildingType::Civic:
                    return IM_COL32(120, 170, 240, 240);
                case CityModel::BuildingType::Luxury:
                    return IM_COL32(240, 210, 140, 240);
                case CityModel::BuildingType::Utility:
                    return IM_COL32(170, 170, 170, 240);
                case CityModel::BuildingType::None:
                default:
                    return IM_COL32(160, 160, 160, 210);
                }
            };

            for (const auto &building : user_buildings)
            {
                ImVec2 screen_pos = world_to_screen(building.position);
                if (screen_pos.x < viewport_info.canvas_pos.x || screen_pos.y < viewport_info.canvas_pos.y ||
                    screen_pos.x > canvas_end.x || screen_pos.y > canvas_end.y)
                {
                    continue;
                }
                // Draw square shape for user buildings
                float size = 4.0f;
                ImVec2 p_min = ImVec2(screen_pos.x - size, screen_pos.y - size);
                ImVec2 p_max = ImVec2(screen_pos.x + size, screen_pos.y + size);
                draw_list->AddRectFilled(p_min, p_max, building_color(building.building_type));
                // Add outline
                ImU32 outline_color = building.locked_type ? IM_COL32(255, 200, 80, 255) : IM_COL32(40, 40, 40, 220);
                draw_list->AddRect(p_min, p_max, outline_color, 0.0f, 0, building.locked_type ? 2.0f : 1.2f);
            }
        }

        if (user_prefs.app().show_road_intersections)
        {
            // Determine which road's intersections to show
            int road_to_show_intersections = -1;
            if (!selected_road_ids.empty())
            {
                // Show intersections for the first selected road (or could be active_road_id)
                road_to_show_intersections = active_road_id >= 0 ? active_road_id : *selected_road_ids.begin();
            }
            else if (hovered_road_id >= 0)
            {
                road_to_show_intersections = hovered_road_id;
            }
            else if (selected_road_id >= 0)
            {
                road_to_show_intersections = selected_road_id;
            }

            if (road_to_show_intersections >= 0)
            {
                const ImVec4 &node_col = user_prefs.app().intersection_highlight_color;
                ImU32 node_color = IM_COL32(static_cast<int>(node_col.x * 255.0f),
                                            static_cast<int>(node_col.y * 255.0f),
                                            static_cast<int>(node_col.z * 255.0f),
                                            static_cast<int>(node_col.w * 255.0f));
                float radius = 3.5f;
                const std::vector<ImVec2> *nodes = nullptr;
                for (const auto &road : user_roads)
                {
                    if (road.id == road_to_show_intersections)
                    {
                        nodes = &road.points;
                        break;
                    }
                }
                if (!nodes && static_cast<size_t>(road_to_show_intersections) < all_roads_indexed.size())
                {
                    nodes = &all_roads_indexed[road_to_show_intersections];
                }
                if (nodes)
                {
                    for (const auto &pt : *nodes)
                    {
                        if (has_city && (fabsf(pt.x) > city_half_extent || fabsf(pt.y) > city_half_extent))
                        {
                            continue;
                        }
                        ImVec2 screen_pos = world_to_screen(pt);
                        if (screen_pos.x < viewport_info.canvas_pos.x || screen_pos.y < viewport_info.canvas_pos.y ||
                            screen_pos.x > canvas_end.x || screen_pos.y > canvas_end.y)
                        {
                            continue;
                        }
                        draw_list->AddCircleFilled(screen_pos, radius, node_color);
                    }
                }
            }
        }

        if (user_prefs.tools().selected_tool == 6)
        {
            int road_to_show_handles = active_road_id >= 0 ? active_road_id : selected_road_id;
            const std::vector<ImVec2> *anchors = nullptr;
            BezierHandles *handles = nullptr;
            if (road_to_show_handles >= 0)
            {
                for (const auto &road : user_roads)
                {
                    if (road.id == road_to_show_handles)
                    {
                        anchors = &road.points;
                        ensure_user_road_handles(road.id, road.points);
                        handles = get_user_road_handles(road.id);
                        break;
                    }
                }
            }

            if (anchors && handles && anchors->size() >= 2)
            {
                ImU32 handle_color = IM_COL32(255, 210, 120, 210);
                ImU32 handle_line = IM_COL32(255, 210, 120, 120);
                ImU32 anchor_color = IM_COL32(110, 180, 255, 220);
                ImU32 outline_color = IM_COL32(20, 20, 20, 220);
                const float handle_radius = 4.0f;
                const float anchor_radius = 4.5f;

                for (size_t i = 0; i < anchors->size(); ++i)
                {
                    const ImVec2 anchor = (*anchors)[i];
                    if (has_city && (fabsf(anchor.x) > city_half_extent || fabsf(anchor.y) > city_half_extent))
                    {
                        continue;
                    }
                    ImVec2 anchor_screen = world_to_screen(anchor);
                    if (anchor_screen.x < viewport_info.canvas_pos.x || anchor_screen.y < viewport_info.canvas_pos.y ||
                        anchor_screen.x > canvas_end.x || anchor_screen.y > canvas_end.y)
                    {
                        continue;
                    }

                    if (i > 0)
                    {
                        ImVec2 h_in = handles->in_handles[i];
                        ImVec2 h_in_screen = world_to_screen(h_in);
                        draw_list->AddLine(anchor_screen, h_in_screen, handle_line, 1.0f);
                        draw_list->AddCircleFilled(h_in_screen, handle_radius, handle_color);
                        draw_list->AddCircle(h_in_screen, handle_radius, outline_color, 0, 1.0f);
                    }
                    if (i + 1 < anchors->size())
                    {
                        ImVec2 h_out = handles->out_handles[i];
                        ImVec2 h_out_screen = world_to_screen(h_out);
                        draw_list->AddLine(anchor_screen, h_out_screen, handle_line, 1.0f);
                        draw_list->AddCircleFilled(h_out_screen, handle_radius, handle_color);
                        draw_list->AddCircle(h_out_screen, handle_radius, outline_color, 0, 1.0f);
                    }

                    draw_list->AddCircleFilled(anchor_screen, anchor_radius, anchor_color);
                    draw_list->AddCircle(anchor_screen, anchor_radius, outline_color, 0, 1.0f);
                }
            }
        }

        if (has_city)
        {
            ImVec2 overlay_pos = ImVec2(viewport_info.canvas_pos.x + 8.0f, viewport_info.canvas_pos.y + 8.0f);
            ImGui::SetCursorScreenPos(overlay_pos);
            ImGui::BeginGroup();
            ImGui::Text("Major: %u / %u", city_major_road_count, city_max_major_roads);
            ImGui::Text("Total: %u / %u", city_total_road_count, city_max_total_roads);
            ImGui::Text("Lots: %u / %u", city_total_lot_count, city_max_lots);
            ImGui::Text("Buildings: %u / %u", city_total_building_sites, city_max_building_sites);
            ImGui::Text("Blocks: %zu", city_block_polygons.size());
            ImGui::Text("Block Inputs: %u", block_road_inputs);
            ImGui::Text("Faces: %u | Valid: %u | I:%u S:%u",
                        block_faces_found, block_valid_blocks, block_intersections, block_segments);
            ImGui::EndGroup();
        }
        draw_list->PopClipRect();

        // Clear hover state if mouse is not over viewport
        if (!hovered)
        {
            hovered_road_id = -1;
            hovered_lot_id = -1;
            hovered_district_id = -1;
        }

        ImGui::End();

        return hovered;
    }

    void ViewportWindow::center_camera()
    {
        camera_target = {0.0f, 0.0f, 0.0f};
        pan = ImVec2(0.0f, 0.0f);
    }

    const std::vector<ViewportWindow::AxiomData> &ViewportWindow::get_axioms() const
    {
        return axioms;
    }

    void ViewportWindow::clear_axioms()
    {
        axioms.clear();
        next_axiom_id = 1;
        selected_axiom_id = -1;
        dragging_axiom = false;
        dragging_axiom_index = -1;
        axiom_revision++;
    }

    void ViewportWindow::clear_rivers()
    {
        rivers.clear();
        next_river_id = 1;
        selected_river_id = -1;
        selected_river_control_index = -1;
        dragging_river = false;
        dragging_river_index = -1;
        river_revision++;
    }

    void ViewportWindow::set_axioms(const std::vector<AxiomData> &new_axioms, int selected_id)
    {
        axioms = new_axioms;
        next_axiom_id = 1;
        int max_id = 0;
        for (const auto &axiom : axioms)
        {
            max_id = std::max(max_id, axiom.id);
        }
        next_axiom_id = std::max(1, max_id + 1);

        selected_axiom_id = -1;
        if (selected_id >= 0)
        {
            int index = find_axiom_index(axioms, selected_id);
            if (index >= 0)
            {
                selected_axiom_id = selected_id;
            }
        }

        dragging_axiom = false;
        dragging_axiom_index = -1;
        axiom_revision++;
    }

    void ViewportWindow::set_rivers(const std::vector<RiverData> &new_rivers, int selected_id)
    {
        rivers = new_rivers;
        next_river_id = 1;
        int max_id = 0;
        for (auto &river : rivers)
        {
            max_id = std::max(max_id, river.id);
            river.spline = build_river_spline(river.control_points, 8);
            river.dirty = false;
        }
        next_river_id = std::max(1, max_id + 1);

        selected_river_id = -1;
        selected_river_control_index = -1;
        if (selected_id >= 0)
        {
            int index = find_river_index(rivers, selected_id);
            if (index >= 0)
            {
                selected_river_id = selected_id;
            }
        }

        dragging_river = false;
        dragging_river_index = -1;
        river_revision++;
    }

    void ViewportWindow::rebuild_rivers()
    {
        for (auto &river : rivers)
        {
            river.spline = build_river_spline(river.control_points, 8);
            river.dirty = false;
        }
        river_revision++;
    }

    // ========== User Lots ==========
    void ViewportWindow::set_user_lots(const std::vector<UserLotData> &lots)
    {
        user_lots = lots;
        int max_id = 0;
        for (const auto &lot : user_lots)
        {
            max_id = std::max(max_id, lot.id);
        }
        next_user_lot_id = std::max(1, max_id + 1);
        user_lot_revision++;
    }

    void ViewportWindow::clear_user_lots()
    {
        user_lots.clear();
        next_user_lot_id = 1;
        user_lot_revision++;
    }

    int ViewportWindow::add_user_lot(ImVec2 position, CityModel::LotType lot_type, bool locked_type)
    {
        UserLotData lot;
        lot.id = next_user_lot_id++;
        lot.position = position;
        lot.lot_type = lot_type;
        lot.locked_type = locked_type;
        user_lots.push_back(lot);
        user_lot_revision++;
        return lot.id;
    }

    bool ViewportWindow::remove_user_lot(int id)
    {
        for (auto it = user_lots.begin(); it != user_lots.end(); ++it)
        {
            if (it->id == id)
            {
                user_lots.erase(it);
                user_lot_revision++;
                return true;
            }
        }
        return false;
    }

    bool ViewportWindow::set_user_lot_type(int id, CityModel::LotType lot_type)
    {
        for (auto &lot : user_lots)
        {
            if (lot.id == id)
            {
                lot.lot_type = lot_type;
                user_lot_revision++;
                return true;
            }
        }
        return false;
    }

    bool ViewportWindow::set_user_lot_locked(int id, bool locked)
    {
        for (auto &lot : user_lots)
        {
            if (lot.id == id)
            {
                lot.locked_type = locked;
                user_lot_revision++;
                return true;
            }
        }
        return false;
    }

    // ========== User Buildings ==========
    void ViewportWindow::set_user_buildings(const std::vector<UserBuildingData> &buildings)
    {
        user_buildings = buildings;
        int max_id = 0;
        for (const auto &building : user_buildings)
        {
            max_id = std::max(max_id, building.id);
        }
        next_user_building_id = std::max(1, max_id + 1);
        user_building_revision++;
    }

    void ViewportWindow::clear_user_buildings()
    {
        user_buildings.clear();
        next_user_building_id = 1;
        user_building_revision++;
    }

    int ViewportWindow::add_user_building(ImVec2 position, CityModel::BuildingType building_type, bool locked_type)
    {
        UserBuildingData building;
        building.id = next_user_building_id++;
        building.position = position;
        building.building_type = building_type;
        building.locked_type = locked_type;
        user_buildings.push_back(building);
        user_building_revision++;
        return building.id;
    }

    bool ViewportWindow::remove_user_building(int id)
    {
        for (auto it = user_buildings.begin(); it != user_buildings.end(); ++it)
        {
            if (it->id == id)
            {
                user_buildings.erase(it);
                user_building_revision++;
                return true;
            }
        }
        return false;
    }

    bool ViewportWindow::set_user_building_type(int id, CityModel::BuildingType building_type)
    {
        for (auto &building : user_buildings)
        {
            if (building.id == id)
            {
                building.building_type = building_type;
                user_building_revision++;
                return true;
            }
        }
        return false;
    }

    bool ViewportWindow::set_user_building_locked(int id, bool locked)
    {
        for (auto &building : user_buildings)
        {
            if (building.id == id)
            {
                building.locked_type = locked;
                user_building_revision++;
                return true;
            }
        }
        return false;
    }

    bool ViewportWindow::set_axiom_position(int id, ImVec2 position)
    {
        int index = find_axiom_index(axioms, id);
        if (index < 0)
            return false;
        axioms[index].position = position;
        axiom_revision++;
        return true;
    }

    bool ViewportWindow::set_axiom_name(int id, const std::string &name)
    {
        int index = find_axiom_index(axioms, id);
        if (index < 0)
            return false;
        axioms[index].name = name;
        axiom_revision++;
        return true;
    }

    bool ViewportWindow::set_axiom_size(int id, float size)
    {
        int index = find_axiom_index(axioms, id);
        if (index < 0)
            return false;
        axioms[index].size = std::clamp(size, 2.0f, 30.0f);
        axiom_revision++;
        return true;
    }

    bool ViewportWindow::set_axiom_opacity(int id, float opacity)
    {
        int index = find_axiom_index(axioms, id);
        if (index < 0)
            return false;
        axioms[index].opacity = std::clamp(opacity, 0.05f, 1.0f);
        axiom_revision++;
        return true;
    }

    bool ViewportWindow::set_axiom_terminal(int id, int terminal_type)
    {
        int index = find_axiom_index(axioms, id);
        if (index < 0)
            return false;
        axioms[index].terminal_type = std::clamp(terminal_type, 0, 2);
        axiom_revision++;
        return true;
    }

    bool ViewportWindow::set_axiom_mode(int id, int mode)
    {
        int index = find_axiom_index(axioms, id);
        if (index < 0)
            return false;
        axioms[index].mode = std::clamp(mode, 0, 2);
        axiom_revision++;
        return true;
    }

    bool ViewportWindow::set_axiom_major_road_count(int id, int count)
    {
        int index = find_axiom_index(axioms, id);
        if (index < 0)
            return false;
        axioms[index].major_road_count = std::clamp(count, 0, 500);
        axiom_revision++;
        return true;
    }

    bool ViewportWindow::set_axiom_minor_road_count(int id, int count)
    {
        int index = find_axiom_index(axioms, id);
        if (index < 0)
            return false;
        axioms[index].minor_road_count = std::clamp(count, 0, 500);
        axiom_revision++;
        return true;
    }

    bool ViewportWindow::set_axiom_fuzzy_mode(int id, bool fuzzy)
    {
        int index = find_axiom_index(axioms, id);
        if (index < 0)
            return false;
        axioms[index].fuzzy_mode = fuzzy;
        axiom_revision++;
        return true;
    }

    bool ViewportWindow::set_axiom_road_range(int id, int min_roads, int max_roads)
    {
        int index = find_axiom_index(axioms, id);
        if (index < 0)
            return false;
        axioms[index].min_roads = std::clamp(min_roads, 1, 500);
        axioms[index].max_roads = std::clamp(max_roads, axioms[index].min_roads, 500);
        axiom_revision++;
        return true;
    }

    bool ViewportWindow::set_river_point(int id, size_t point_index, ImVec2 position)
    {
        int index = find_river_index(rivers, id);
        if (index < 0)
            return false;
        if (point_index >= rivers[index].control_points.size())
            return false;
        rivers[index].control_points[point_index] = position;
        update_river_after_edit(rivers[index], user_prefs.app().river_generation_mode == 0);
        river_revision++;
        return true;
    }

    bool ViewportWindow::insert_river_point(int id, size_t index, ImVec2 position)
    {
        int river_index = find_river_index(rivers, id);
        if (river_index < 0)
            return false;
        if (index > rivers[river_index].control_points.size())
            return false;
        rivers[river_index].control_points.insert(
            rivers[river_index].control_points.begin() + static_cast<int>(index), position);
        update_river_after_edit(rivers[river_index], user_prefs.app().river_generation_mode == 0);
        river_revision++;
        return true;
    }

    bool ViewportWindow::remove_river_point(int id, size_t index)
    {
        int river_index = find_river_index(rivers, id);
        if (river_index < 0)
            return false;
        if (index >= rivers[river_index].control_points.size())
            return false;
        if (rivers[river_index].control_points.size() <= 2)
            return false;
        rivers[river_index].control_points.erase(
            rivers[river_index].control_points.begin() + static_cast<int>(index));
        update_river_after_edit(rivers[river_index], user_prefs.app().river_generation_mode == 0);
        river_revision++;
        return true;
    }

    bool ViewportWindow::set_river_name(int id, const std::string &name)
    {
        int index = find_river_index(rivers, id);
        if (index < 0)
            return false;
        rivers[index].name = name;
        river_revision++;
        return true;
    }

    bool ViewportWindow::set_river_width(int id, float width)
    {
        int index = find_river_index(rivers, id);
        if (index < 0)
            return false;
        rivers[index].width = std::max(1.0f, width);
        river_revision++;
        return true;
    }

    bool ViewportWindow::set_river_opacity(int id, float opacity)
    {
        int index = find_river_index(rivers, id);
        if (index < 0)
            return false;
        rivers[index].opacity = std::clamp(opacity, 0.05f, 1.0f);
        river_revision++;
        return true;
    }

    void ViewportWindow::submit_road_edit(int road_id, CityModel::RoadType type, const std::vector<ImVec2> &points, bool from_user)
    {
        pending_road_edit.road_id = road_id;
        pending_road_edit.type = type;
        pending_road_edit.points = points;
        pending_road_edit.from_user = from_user;
        road_edit_pending = true;
    }

    bool ViewportWindow::consume_clear_city_request()
    {
        if (!clear_city_requested)
            return false;
        clear_city_requested = false;
        return true;
    }

    bool ViewportWindow::consume_clear_axioms_request()
    {
        if (!clear_axioms_requested)
            return false;
        clear_axioms_requested = false;
        return true;
    }

    bool ViewportWindow::consume_regeneration_request()
    {
        if (!regeneration_requested)
        {
            return false;
        }
        regeneration_requested = false;
        return true;
    }

    void ViewportWindow::request_regeneration()
    {
        regeneration_requested = true;
    }

    void ViewportWindow::set_city(const CityModel::City &city, uint32_t max_major_roads, uint32_t max_total_roads)
    {
        city_water.clear();
        for (auto &bucket : city_roads_by_type)
        {
            bucket.clear();
        }
        city_districts.clear();
        city_lots.clear();
        city_building_sites.clear();
        city_block_polygons.clear();
        city_block_faces.clear();
        selected_lot_ids.clear();
        selected_lot_id = -1;
        active_lot_id = -1;
        hovered_lot_id = -1;
        selected_district_ids.clear();
        selected_district_id = -1;
        active_district_id = -1;
        hovered_district_id = -1;
        all_roads_indexed.clear();
        all_roads_refs.clear();

        const float width = static_cast<float>(city.bounds.max.x - city.bounds.min.x);
        const float height = static_cast<float>(city.bounds.max.y - city.bounds.min.y);
        const float half_x = width * 0.5f;
        const float half_y = height * 0.5f;
        city_half_extent = std::max(half_x, half_y);

        auto convert = [&](const CityModel::Polyline &poly)
        {
            std::vector<ImVec2> line;
            line.reserve(poly.points.size());
            for (const auto &p : poly.points)
            {
                line.push_back(ImVec2(static_cast<float>(p.x - half_x), static_cast<float>(p.y - half_y)));
            }
            return line;
        };

        for (const auto &w : city.water)
            city_water.push_back(convert(w));

        const double merge_eps = 1e-3;
        std::array<std::vector<CityModel::Polyline>, CityModel::road_type_count> merged_roads_by_type;
        for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
        {
            merged_roads_by_type[i] = merge_polylines(city.roads_by_type[i], merge_eps);
            for (const auto &r : merged_roads_by_type[i])
            {
                city_roads_by_type[i].push_back(convert(r));
            }
        }

        for (const auto &district : city.districts)
        {
            DistrictOverlay overlay;
            overlay.id = district.id;
            overlay.type = district.type;
            overlay.border.reserve(district.border.size());
            for (const auto &p : district.border)
            {
                overlay.border.push_back(ImVec2(static_cast<float>(p.x - half_x),
                                                static_cast<float>(p.y - half_y)));
            }
            city_districts.push_back(std::move(overlay));
        }

        for (const auto &lot : city.lots)
        {
            LotOverlay overlay;
            overlay.id = lot.id;
            overlay.district_id = lot.district_id;
            overlay.lot_type = lot.lot_type;
            overlay.centroid = ImVec2(static_cast<float>(lot.centroid.x - half_x),
                                      static_cast<float>(lot.centroid.y - half_y));
            city_lots.push_back(std::move(overlay));
        }

        for (const auto &site : city.building_sites)
        {
            BuildingSiteOverlay overlay;
            overlay.id = site.id;
            overlay.lot_id = site.lot_id;
            overlay.district_id = site.district_id;
            overlay.type = site.type;
            overlay.position = ImVec2(static_cast<float>(site.position.x - half_x),
                                      static_cast<float>(site.position.y - half_y));
            city_building_sites.push_back(std::move(overlay));
        }

        for (const auto &poly : city.block_polygons)
        {
            BlockPolygonOverlay overlay;
            overlay.outer.reserve(poly.outer.size());
            if (poly.district_id > 0 && poly.district_id <= city.districts.size())
            {
                overlay.type = city.districts[poly.district_id - 1].type;
            }
            else
            {
                overlay.type = CityModel::DistrictType::Mixed;
            }
            for (const auto &p : poly.outer)
            {
                overlay.outer.push_back(ImVec2(static_cast<float>(p.x - half_x),
                                               static_cast<float>(p.y - half_y)));
            }
            if (!poly.holes.empty())
            {
                overlay.holes.reserve(poly.holes.size());
                for (const auto &hole : poly.holes)
                {
                    std::vector<ImVec2> hole_ring;
                    hole_ring.reserve(hole.size());
                    for (const auto &p : hole)
                    {
                        hole_ring.push_back(ImVec2(static_cast<float>(p.x - half_x),
                                                   static_cast<float>(p.y - half_y)));
                    }
                    overlay.holes.push_back(std::move(hole_ring));
                }
            }
            city_block_polygons.push_back(std::move(overlay));
        }

        for (const auto &poly : city.block_faces)
        {
            BlockPolygonOverlay overlay;
            overlay.outer.reserve(poly.points.size());
            overlay.type = CityModel::DistrictType::Mixed;
            for (const auto &p : poly.points)
            {
                overlay.outer.push_back(ImVec2(static_cast<float>(p.x - half_x),
                                               static_cast<float>(p.y - half_y)));
            }
            city_block_faces.push_back(std::move(overlay));
        }

        // Build indexed lookup for all roads (same order as road_index population)
        for (auto type : CityModel::generated_road_order)
        {
            std::size_t type_index = CityModel::road_type_index(type);
            const auto &roads = merged_roads_by_type[type_index];
            std::size_t local_index = 0;
            for (const auto &r : roads)
            {
                all_roads_indexed.push_back(convert(r));
                all_roads_refs.push_back(RoadPolylineRef{type_index, local_index, type});
                local_index++;
            }
        }
        rebuild_hidden_generated_roads();

        city_major_road_count = 0;
        city_total_road_count = 0;
        city_total_lot_count = 0;
        city_total_building_sites = 0;
        for (auto type : CityModel::generated_road_order)
        {
            const auto count = static_cast<uint32_t>(merged_roads_by_type[CityModel::road_type_index(type)].size());
            if (CityModel::is_major_group(type))
            {
                city_major_road_count += count;
            }
            city_total_road_count += count;
        }
        city_max_major_roads = max_major_roads;
        city_max_total_roads = max_total_roads;
        city_total_lot_count = static_cast<uint32_t>(city.lots.size());
        city_total_building_sites = static_cast<uint32_t>(city.building_sites.size());
        city_max_lots = max_total_roads > 0 ? (max_total_roads / 2) : 0;
        city_max_building_sites = city_max_lots * 6;
        block_road_inputs = city.block_stats.road_inputs;
        block_faces_found = city.block_stats.faces_found;
        block_valid_blocks = city.block_stats.valid_blocks;
        block_intersections = city.block_stats.intersections;
        block_segments = city.block_stats.segments;

        has_city = true;
    }

    void ViewportWindow::clear_city()
    {
        city_water.clear();
        for (auto &bucket : city_roads_by_type)
        {
            bucket.clear();
        }
        city_districts.clear();
        city_lots.clear();
        city_building_sites.clear();
        city_block_polygons.clear();
        city_block_faces.clear();
        all_roads_indexed.clear();
        all_roads_refs.clear();
        hidden_generated_road_ids.clear();
        city_major_road_count = 0;
        city_total_road_count = 0;
        city_max_major_roads = 0;
        city_max_total_roads = 0;
        city_total_lot_count = 0;
        city_max_lots = 0;
        city_total_building_sites = 0;
        city_max_building_sites = 0;
        block_road_inputs = 0;
        block_faces_found = 0;
        block_valid_blocks = 0;
        block_intersections = 0;
        block_segments = 0;
        selected_lot_id = -1;
        hovered_lot_id = -1;
        selected_lot_ids.clear();
        active_lot_id = -1;
        selected_district_id = -1;
        hovered_district_id = -1;
        selected_district_ids.clear();
        active_district_id = -1;
        has_city = false;
        city_half_extent = 0.0f;
    }

    void ViewportWindow::set_district_field(const DistrictGenerator::DistrictField &field)
    {
        district_grid_info.width = field.width;
        district_grid_info.height = field.height;
        
        const float width_units = static_cast<float>(field.cell_size.x * field.width);
        const float height_units = static_cast<float>(field.cell_size.y * field.height);
        const float half_x = width_units * 0.5f;
        const float half_y = height_units * 0.5f;
        
        district_grid_info.origin = ImVec2(
            static_cast<float>(field.origin.x) - half_x,
            static_cast<float>(field.origin.y) - half_y
        );
        district_grid_info.cell_size = ImVec2(
            static_cast<float>(field.cell_size.x),
            static_cast<float>(field.cell_size.y)
        );
        district_grid_info.district_ids = field.district_ids;
    }

    bool road_overlays_equal(const std::vector<ViewportWindow::RoadOverlay> &a,
                             const std::vector<ViewportWindow::RoadOverlay> &b)
    {
        if (a.size() != b.size())
        {
            return false;
        }
        for (std::size_t idx = 0; idx < a.size(); ++idx)
        {
            const auto &lhs = a[idx];
            const auto &rhs = b[idx];
            if (lhs.id != rhs.id || lhs.type != rhs.type || lhs.from_generated != rhs.from_generated)
            {
                return false;
            }
            if (lhs.points.size() != rhs.points.size())
            {
                return false;
            }
            for (std::size_t pt = 0; pt < lhs.points.size(); ++pt)
            {
                if (lhs.points[pt].x != rhs.points[pt].x || lhs.points[pt].y != rhs.points[pt].y)
                {
                    return false;
                }
            }
        }
        return true;
    }

    void ViewportWindow::set_user_roads(const std::vector<RoadOverlay> &roads)
    {
        bool roads_changed = !road_overlays_equal(user_roads, roads);
        user_roads = roads;
        std::unordered_set<int> keep_ids;
        keep_ids.reserve(user_roads.size());
        for (const auto &road : user_roads)
        {
            keep_ids.insert(road.id);
            ensure_user_road_handles(road.id, road.points);
        }
        for (auto it = user_road_handles.begin(); it != user_road_handles.end();)
        {
            if (!keep_ids.count(it->first))
            {
                it = user_road_handles.erase(it);
                continue;
            }
            ++it;
        }
        for (auto it = user_road_display_cache.begin(); it != user_road_display_cache.end();)
        {
            if (!keep_ids.count(it->first))
            {
                it = user_road_display_cache.erase(it);
                continue;
            }
            ++it;
        }
        rebuild_hidden_generated_roads();
        if (roads_changed)
        {
            user_road_revision++;
        }
    }

    void ViewportWindow::clear_user_roads()
    {
        user_roads.clear();
        user_road_handles.clear();
        user_road_display_cache.clear();
        hidden_generated_road_ids.clear();
        user_road_revision++;
    }

    void ViewportWindow::ensure_user_road_handles(int road_id, const std::vector<ImVec2> &anchors)
    {
        auto &state = user_road_handles[road_id];
        if (state.anchors.size() != anchors.size() || state.in_handles.size() != anchors.size() ||
            state.out_handles.size() != anchors.size() || state.broken.size() != anchors.size())
        {
            state.anchors = anchors;
            state.in_handles.assign(anchors.size(), ImVec2(0.0f, 0.0f));
            state.out_handles.assign(anchors.size(), ImVec2(0.0f, 0.0f));
            state.broken.assign(anchors.size(), false);

            for (size_t i = 0; i < anchors.size(); ++i)
            {
                const ImVec2 anchor = anchors[i];
                if (i > 0)
                {
                    const ImVec2 prev = anchors[i - 1];
                    ImVec2 dir{anchor.x - prev.x, anchor.y - prev.y};
                    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                    if (len > 1e-4f)
                    {
                        dir.x /= len;
                        dir.y /= len;
                        state.in_handles[i] = ImVec2(anchor.x - dir.x * len * 0.33f,
                                                     anchor.y - dir.y * len * 0.33f);
                    }
                    else
                    {
                        state.in_handles[i] = anchor;
                    }
                }
                else
                {
                    state.in_handles[i] = anchor;
                }

                if (i + 1 < anchors.size())
                {
                    const ImVec2 next = anchors[i + 1];
                    ImVec2 dir{next.x - anchor.x, next.y - anchor.y};
                    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                    if (len > 1e-4f)
                    {
                        dir.x /= len;
                        dir.y /= len;
                        state.out_handles[i] = ImVec2(anchor.x + dir.x * len * 0.33f,
                                                      anchor.y + dir.y * len * 0.33f);
                    }
                    else
                    {
                        state.out_handles[i] = anchor;
                    }
                }
                else
                {
                    state.out_handles[i] = anchor;
                }
            }
            state.dirty = true;
            return;
        }

        for (size_t i = 0; i < anchors.size(); ++i)
        {
            ImVec2 delta{anchors[i].x - state.anchors[i].x, anchors[i].y - state.anchors[i].y};
            state.anchors[i] = anchors[i];
            state.in_handles[i].x += delta.x;
            state.in_handles[i].y += delta.y;
            state.out_handles[i].x += delta.x;
            state.out_handles[i].y += delta.y;
        }
        state.dirty = true;
    }

    ViewportWindow::BezierHandles *ViewportWindow::get_user_road_handles(int road_id)
    {
        auto it = user_road_handles.find(road_id);
        if (it == user_road_handles.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    const std::vector<ImVec2> &ViewportWindow::get_user_road_display_points(int road_id, const std::vector<ImVec2> &anchors)
    {
        auto *handles = get_user_road_handles(road_id);
        if (!handles || anchors.size() < 2)
        {
            return anchors;
        }

        auto &cache = user_road_display_cache[road_id];
        if (!handles->dirty && !cache.empty())
        {
            return cache;
        }

        auto bezier_point = [](const ImVec2 &p0, const ImVec2 &p1, const ImVec2 &p2, const ImVec2 &p3, float t)
        {
            float u = 1.0f - t;
            float tt = t * t;
            float uu = u * u;
            float uuu = uu * u;
            float ttt = tt * t;
            ImVec2 p;
            p.x = uuu * p0.x;
            p.y = uuu * p0.y;
            p.x += 3.0f * uu * t * p1.x;
            p.y += 3.0f * uu * t * p1.y;
            p.x += 3.0f * u * tt * p2.x;
            p.y += 3.0f * u * tt * p2.y;
            p.x += ttt * p3.x;
            p.y += ttt * p3.y;
            return p;
        };

        cache.clear();
        const int samples = 12;
        for (size_t i = 0; i + 1 < anchors.size(); ++i)
        {
            const ImVec2 p0 = anchors[i];
            const ImVec2 p1 = handles->out_handles[i];
            const ImVec2 p2 = handles->in_handles[i + 1];
            const ImVec2 p3 = anchors[i + 1];
            for (int s = 0; s <= samples; ++s)
            {
                if (i > 0 && s == 0)
                {
                    continue;
                }
                float t = static_cast<float>(s) / static_cast<float>(samples);
                cache.push_back(bezier_point(p0, p1, p2, p3, t));
            }
        }

        handles->dirty = false;
        return cache;
    }

    bool ViewportWindow::consume_road_edit(RoadEditEvent &out)
    {
        if (!road_edit_pending)
        {
            return false;
        }
        out = pending_road_edit;
        road_edit_pending = false;
        return true;
    }

    void ViewportWindow::rebuild_hidden_generated_roads()
    {
        hidden_generated_road_ids.clear();
        for (const auto &road : user_roads)
        {
            if (!road.from_generated)
            {
                continue;
            }
            if (road.id >= 0 && static_cast<size_t>(road.id) < all_roads_indexed.size())
            {
                hidden_generated_road_ids.insert(road.id);
            }
        }
    }

    void ViewportWindow::set_top_ortho_view()
    {
        ortho_enabled = true;
        camera_pitch = 1.55334f; // ~89 degrees down
        camera_yaw = 0.0f;
        if (camera_distance < 10.0f)
        {
            camera_distance = 10.0f;
        }
        zoom = camera_distance;
    }

    void ViewportWindow::set_perspective_view()
    {
        ortho_enabled = false;
    }

    void ViewportWindow::update_texture(GLuint new_texture, int width, int height)
    {
        texture_id = new_texture;
        texture_width = width;
        texture_height = height;
    }

    ImVec2 ViewportWindow::get_mouse_world_pos() const
    {
        return screen_to_world(ImGui::GetMousePos());
    }

    void ViewportWindow::reset_view()
    {
        camera_target = {0.0f, 0.0f, 0.0f};
        camera_distance = 20.0f;
        camera_yaw = 0.785398f;
        camera_pitch = 0.523599f;
        ortho_enabled = false;
        zoom = camera_distance;
    }

    ImVec2 ViewportWindow::screen_to_world(ImVec2 screen_pos) const
    {
        Vec3 forward{};
        Vec3 right{};
        Vec3 up{};
        get_camera_basis(forward, right, up);

        ImVec2 ndc = ImVec2(
            (screen_pos.x - viewport_info.canvas_pos.x) / viewport_info.canvas_size.x * 2.0f - 1.0f,
            1.0f - (screen_pos.y - viewport_info.canvas_pos.y) / viewport_info.canvas_size.y * 2.0f);

        float aspect = viewport_info.canvas_size.x / viewport_info.canvas_size.y;
        Vec3 cam_pos = get_camera_position();
        Vec3 world_dir{};
        Vec3 ray_origin = cam_pos;

        if (ortho_enabled)
        {
            float size = camera_distance;
            ray_origin.x += right.x * (ndc.x * size * aspect) + up.x * (ndc.y * size);
            ray_origin.y += right.y * (ndc.x * size * aspect) + up.y * (ndc.y * size);
            ray_origin.z += right.z * (ndc.x * size * aspect) + up.z * (ndc.y * size);
            world_dir = forward;
        }
        else
        {
            float tan_half_fov = tanf((fov_degrees * 0.5f) * (3.1415926f / 180.0f));
            Vec3 ray_dir{
                ndc.x * tan_half_fov * aspect,
                ndc.y * tan_half_fov,
                1.0f};

            world_dir = {
                right.x * ray_dir.x + up.x * ray_dir.y + forward.x * ray_dir.z,
                right.y * ray_dir.x + up.y * ray_dir.y + forward.y * ray_dir.z,
                right.z * ray_dir.x + up.z * ray_dir.y + forward.z * ray_dir.z};
        }

        float denom = world_dir.y;
        if (fabsf(denom) < 1e-5f)
        {
            return ImVec2(cam_pos.x, cam_pos.z);
        }

        float t = -ray_origin.y / denom;
        if (t < 0.0f)
        {
            return ImVec2(ray_origin.x, ray_origin.z);
        }

        Vec3 hit{
            ray_origin.x + world_dir.x * t,
            0.0f,
            ray_origin.z + world_dir.z * t};
        return ImVec2(hit.x, hit.z);
    }

    ImVec2 ViewportWindow::world_to_screen(ImVec2 world_pos) const
    {
        if (viewport_info.canvas_size.x < 1.0f || viewport_info.canvas_size.y < 1.0f)
        {
            return ImVec2(-10000.0f, -10000.0f);
        }

        float view_m[16];
        float proj_m[16];
        if (cached_matrices_valid)
        {
            for (int i = 0; i < 16; ++i)
            {
                view_m[i] = cached_view[i];
                proj_m[i] = cached_proj[i];
            }
        }
        else
        {
            Vec3 cam_pos = get_camera_position();
            Mat4 view = make_look_at(cam_pos, camera_target, {0.0f, 1.0f, 0.0f});
            float aspect = viewport_info.canvas_size.x / viewport_info.canvas_size.y;
            float near_clip = user_prefs.app().enable_culling ? user_prefs.app().cull_near : 0.1f;
            float far_clip = user_prefs.app().enable_culling ? user_prefs.app().cull_far : 5000.0f;
            if (near_clip < 0.001f)
                near_clip = 0.001f;
            if (far_clip <= near_clip + 0.1f)
                far_clip = near_clip + 0.1f;
            Mat4 proj = ortho_enabled
                            ? make_ortho(-camera_distance * aspect, camera_distance * aspect,
                                         -camera_distance, camera_distance, near_clip, far_clip)
                            : make_perspective(fov_degrees, aspect, near_clip, far_clip);
            for (int i = 0; i < 16; ++i)
            {
                view_m[i] = view.m[i];
                proj_m[i] = proj.m[i];
            }
        }

        float x = world_pos.x;
        float y = 0.0f;
        float z = world_pos.y;

        float vx = view_m[0] * x + view_m[4] * y + view_m[8] * z + view_m[12];
        float vy = view_m[1] * x + view_m[5] * y + view_m[9] * z + view_m[13];
        float vz = view_m[2] * x + view_m[6] * y + view_m[10] * z + view_m[14];
        float vw = view_m[3] * x + view_m[7] * y + view_m[11] * z + view_m[15];

        float cx = proj_m[0] * vx + proj_m[4] * vy + proj_m[8] * vz + proj_m[12] * vw;
        float cy = proj_m[1] * vx + proj_m[5] * vy + proj_m[9] * vz + proj_m[13] * vw;
        float cz = proj_m[2] * vx + proj_m[6] * vy + proj_m[10] * vz + proj_m[14] * vw;
        float cw = proj_m[3] * vx + proj_m[7] * vy + proj_m[11] * vz + proj_m[15] * vw;

        if (fabsf(cw) < 1e-6f)
        {
            return ImVec2(-10000.0f, -10000.0f);
        }

        float ndc_x = cx / cw;
        float ndc_y = cy / cw;

        ImVec2 screen;
        screen.x = viewport_info.canvas_pos.x + (ndc_x * 0.5f + 0.5f) * viewport_info.canvas_size.x;
        screen.y = viewport_info.canvas_pos.y + (-ndc_y * 0.5f + 0.5f) * viewport_info.canvas_size.y;
        (void)cz;
        return screen;
    }

    void ViewportWindow::handle_pan_zoom()
    {
        ImGuiIO &io = ImGui::GetIO();

        const float orbit_sensitivity = 0.005f;
        const float pan_sensitivity = 0.0025f * camera_distance;

        Vec3 forward{};
        Vec3 right{};
        Vec3 up{};
        get_camera_basis(forward, right, up);

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            if (nav_mode == NavigationMode::Pan || io.KeyShift)
            {
                camera_target.x += (-right.x * io.MouseDelta.x + up.x * io.MouseDelta.y) * pan_sensitivity;
                camera_target.y += (-right.y * io.MouseDelta.x + up.y * io.MouseDelta.y) * pan_sensitivity;
                camera_target.z += (-right.z * io.MouseDelta.x + up.z * io.MouseDelta.y) * pan_sensitivity;
            }
            else if (io.KeyCtrl)
            {
                camera_distance *= powf(1.01f, io.MouseDelta.y);
            }
            else
            {
                camera_yaw -= io.MouseDelta.x * orbit_sensitivity;
                camera_pitch -= io.MouseDelta.y * orbit_sensitivity;
            }
        }

        if (io.MouseWheel != 0.0f)
        {
            camera_distance *= powf(1.1f, -io.MouseWheel);
        }

        const float max_pitch = 1.55334f; // ~89 degrees
        camera_pitch = std::max(-max_pitch, std::min(max_pitch, camera_pitch));
        camera_distance = std::max(1.0f, std::min(1000.0f, camera_distance));

        pan = ImVec2(camera_target.x, camera_target.z);
        zoom = camera_distance;
    }

    ViewportWindow::Vec3 ViewportWindow::get_camera_position() const
    {
        float cos_pitch = cosf(camera_pitch);
        float sin_pitch = sinf(camera_pitch);
        float cos_yaw = cosf(camera_yaw);
        float sin_yaw = sinf(camera_yaw);

        Vec3 offset{
            cos_pitch * cos_yaw,
            sin_pitch,
            cos_pitch * sin_yaw};

        return Vec3{
            camera_target.x + offset.x * camera_distance,
            camera_target.y + offset.y * camera_distance,
            camera_target.z + offset.z * camera_distance};
    }

    void ViewportWindow::get_camera_basis(Vec3 &forward, Vec3 &right, Vec3 &up) const
    {
        Vec3 cam_pos = get_camera_position();
        Vec3 to_target{
            camera_target.x - cam_pos.x,
            camera_target.y - cam_pos.y,
            camera_target.z - cam_pos.z};

        float len = sqrtf(to_target.x * to_target.x + to_target.y * to_target.y + to_target.z * to_target.z);
        if (len < 1e-5f)
        {
            forward = {0.0f, 0.0f, 1.0f};
        }
        else
        {
            forward = {to_target.x / len, to_target.y / len, to_target.z / len};
        }

        Vec3 world_up{0.0f, 1.0f, 0.0f};
        right = {
            forward.y * world_up.z - forward.z * world_up.y,
            forward.z * world_up.x - forward.x * world_up.z,
            forward.x * world_up.y - forward.y * world_up.x};

        float right_len = sqrtf(right.x * right.x + right.y * right.y + right.z * right.z);
        if (right_len < 1e-5f)
        {
            right = {1.0f, 0.0f, 0.0f};
        }
        else
        {
            right = {right.x / right_len, right.y / right_len, right.z / right_len};
        }

        up = {
            right.y * forward.z - right.z * forward.y,
            right.z * forward.x - right.x * forward.z,
            right.x * forward.y - right.y * forward.x};
    }

    void ViewportWindow::init_grid_resources()
    {
        if (grid_initialized)
        {
            return;
        }

        const char *vs_source =
            "#version 130\n"
            "in vec3 a_pos;\n"
            "in vec4 a_color;\n"
            "uniform mat4 u_view;\n"
            "uniform mat4 u_proj;\n"
            "out vec4 v_color;\n"
            "void main() {\n"
            "    v_color = a_color;\n"
            "    gl_Position = u_proj * u_view * vec4(a_pos, 1.0);\n"
            "}\n";

        const char *fs_source =
            "#version 130\n"
            "in vec4 v_color;\n"
            "uniform float u_brightness;\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "    vec3 rgb = v_color.rgb * u_brightness;\n"
            "    FragColor = vec4(rgb, v_color.a);\n"
            "}\n";

        grid_program = create_program(vs_source, fs_source);
        if (grid_program == 0)
        {
            return;
        }

        struct Vertex
        {
            float x, y, z;
            float r, g, b, a;
        };

        int grid_bounds = user_prefs.app().grid_bounds;
        if (grid_bounds < 256)
            grid_bounds = 256;
        const float grid_spacing = std::max(1.0f, user_prefs.tools().grid_size);
        const float extent = grid_bounds * 0.5f;
        const int grid_half = static_cast<int>(extent / grid_spacing);
        // High-contrast viewport colors (base #838486)
        const float minor_color[4] = {0.16f, 0.16f, 0.17f, 1.0f};
        const float major_color[4] = {0.92f, 0.92f, 0.92f, 1.0f};
        const float plane_color[4] = {0.514f, 0.518f, 0.525f, 1.0f}; // #838486
        const float plane_y = -0.01f;

        std::vector<Vertex> vertices;
        vertices.reserve((grid_half * 2 + 1) * 4 + 12);

        // Ground plane (two triangles)
        vertices.push_back({-extent, plane_y, -extent, plane_color[0], plane_color[1], plane_color[2], plane_color[3]});
        vertices.push_back({extent, plane_y, -extent, plane_color[0], plane_color[1], plane_color[2], plane_color[3]});
        vertices.push_back({extent, plane_y, extent, plane_color[0], plane_color[1], plane_color[2], plane_color[3]});
        vertices.push_back({-extent, plane_y, -extent, plane_color[0], plane_color[1], plane_color[2], plane_color[3]});
        vertices.push_back({extent, plane_y, extent, plane_color[0], plane_color[1], plane_color[2], plane_color[3]});
        vertices.push_back({-extent, plane_y, extent, plane_color[0], plane_color[1], plane_color[2], plane_color[3]});

        grid_line_offset = static_cast<int>(vertices.size());

        for (int i = -grid_half; i <= grid_half; ++i)
        {
            float x = i * grid_spacing;
            float z = i * grid_spacing;
            const float *color = (i % 5 == 0) ? major_color : minor_color;

            vertices.push_back({x, 0.0f, -extent, color[0], color[1], color[2], color[3]});
            vertices.push_back({x, 0.0f, extent, color[0], color[1], color[2], color[3]});

            vertices.push_back({-extent, 0.0f, z, color[0], color[1], color[2], color[3]});
            vertices.push_back({extent, 0.0f, z, color[0], color[1], color[2], color[3]});
        }

        // Axes
        const float axis_len = std::max(12.0f, extent * 0.05f);
        vertices.push_back({0.0f, 0.0f, 0.0f, 0.86f, 0.31f, 0.31f, 1.0f});
        vertices.push_back({axis_len, 0.0f, 0.0f, 0.86f, 0.31f, 0.31f, 1.0f});

        vertices.push_back({0.0f, 0.0f, 0.0f, 0.31f, 0.86f, 0.31f, 1.0f});
        vertices.push_back({0.0f, axis_len, 0.0f, 0.31f, 0.86f, 0.31f, 1.0f});

        vertices.push_back({0.0f, 0.0f, 0.0f, 0.31f, 0.47f, 0.86f, 1.0f});
        vertices.push_back({0.0f, 0.0f, axis_len, 0.31f, 0.47f, 0.86f, 1.0f});

        grid_vertex_count = static_cast<int>(vertices.size());
        grid_line_count = grid_vertex_count - grid_line_offset;

        glGenVertexArrays(1, &grid_vao);
        glGenBuffers(1, &grid_vbo);

        glBindVertexArray(grid_vao);
        glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                     vertices.data(),
                     GL_STATIC_DRAW);

        GLint pos_loc = glGetAttribLocation(grid_program, "a_pos");
        GLint color_loc = glGetAttribLocation(grid_program, "a_color");

        glEnableVertexAttribArray(static_cast<GLuint>(pos_loc));
        glVertexAttribPointer(static_cast<GLuint>(pos_loc), 3, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex), reinterpret_cast<void *>(0));

        glEnableVertexAttribArray(static_cast<GLuint>(color_loc));
        glVertexAttribPointer(static_cast<GLuint>(color_loc), 4, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex), reinterpret_cast<void *>(sizeof(float) * 3));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        grid_initialized = true;
    }

    void ViewportWindow::destroy_grid_resources()
    {
        if (grid_vbo != 0)
        {
            glDeleteBuffers(1, &grid_vbo);
            grid_vbo = 0;
        }
        if (grid_vao != 0)
        {
            glDeleteVertexArrays(1, &grid_vao);
            grid_vao = 0;
        }
        if (grid_program != 0)
        {
            glDeleteProgram(grid_program);
            grid_program = 0;
        }
        grid_initialized = false;
    }

    void ViewportWindow::render_gl()
    {
        if (viewport_info.canvas_size.x < 1.0f || viewport_info.canvas_size.y < 1.0f)
        {
            return;
        }
        if (last_light_theme != user_prefs.app().use_light_theme ||
            last_grid_bounds != user_prefs.app().grid_bounds ||
            fabsf(last_grid_spacing - user_prefs.tools().grid_size) > 0.01f)
        {
            destroy_grid_resources();
            last_light_theme = user_prefs.app().use_light_theme;
            last_grid_bounds = user_prefs.app().grid_bounds;
            last_grid_spacing = user_prefs.tools().grid_size;
        }
        if (!grid_initialized)
        {
            init_grid_resources();
        }
        if (!grid_initialized)
        {
            return;
        }

        if (framebuffer_width <= 0 || framebuffer_height <= 0)
        {
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, framebuffer_width, framebuffer_height);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);

        float brightness = user_prefs.app().viewport_brightness;
        if (brightness < 0.3f)
            brightness = 0.3f;
        if (brightness > 2.5f)
            brightness = 2.5f;

        glClearColor(0.514f * brightness, 0.518f * brightness, 0.525f * brightness, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ViewportWindow::Vec3 cam_pos = get_camera_position();
        Mat4 view = make_look_at(cam_pos, camera_target, {0.0f, 1.0f, 0.0f});
        float aspect = viewport_info.canvas_size.x / viewport_info.canvas_size.y;
        float near_clip = user_prefs.app().enable_culling ? user_prefs.app().cull_near : 0.1f;
        float far_clip = user_prefs.app().enable_culling ? user_prefs.app().cull_far : 5000.0f;
        if (near_clip < 0.001f)
            near_clip = 0.001f;
        if (far_clip <= near_clip + 0.1f)
            far_clip = near_clip + 0.1f;
        Mat4 proj = ortho_enabled
                        ? make_ortho(-camera_distance * aspect, camera_distance * aspect,
                                     -camera_distance, camera_distance, near_clip, far_clip)
                        : make_perspective(fov_degrees, aspect, near_clip, far_clip);

        glUseProgram(grid_program);
        GLint view_loc = glGetUniformLocation(grid_program, "u_view");
        GLint proj_loc = glGetUniformLocation(grid_program, "u_proj");
        GLint brightness_loc = glGetUniformLocation(grid_program, "u_brightness");
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.m);
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.m);
        glUniform1f(brightness_loc, brightness);

        glBindVertexArray(grid_vao);
        glDrawArrays(GL_TRIANGLES, 0, grid_line_offset);
        if (user_prefs.app().show_grid)
        {
            glLineWidth(1.0f);
            glDrawArrays(GL_LINES, grid_line_offset, grid_line_count);
        }
        glBindVertexArray(0);
        glUseProgram(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void ViewportWindow::ensure_framebuffer_size(int width, int height)
    {
        if (width == framebuffer_width && height == framebuffer_height && fbo != 0)
        {
            return;
        }

        if (color_texture != 0)
        {
            glDeleteTextures(1, &color_texture);
            color_texture = 0;
        }
        if (depth_buffer != 0)
        {
            glDeleteRenderbuffers(1, &depth_buffer);
            depth_buffer = 0;
        }
        if (fbo == 0)
        {
            glGenFramebuffers(1, &fbo);
        }

        framebuffer_width = width;
        framebuffer_height = height;

        glGenTextures(1, &color_texture);
        glBindTexture(GL_TEXTURE_2D, color_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebuffer_width, framebuffer_height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glGenRenderbuffers(1, &depth_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framebuffer_width, framebuffer_height);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

} // namespace RCG
