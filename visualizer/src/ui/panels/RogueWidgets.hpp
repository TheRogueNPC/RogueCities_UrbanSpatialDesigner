#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>
#include <string>

namespace RogueCity::Visualizer::UI {

    // Configuration for the unified "Cockpit" style
    struct ButtonStyle {
        ImVec4 col_idle   = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
        ImVec4 col_hover  = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
        ImVec4 col_active = ImVec4(0.0f, 0.6f, 0.6f, 1.0f); // Cyan base
        ImVec4 col_border = ImVec4(1.0f, 1.0f, 1.0f, 0.15f);
        float rounding    = 4.0f;
    };

    // A state-reactive, icon-centric button that feels "alive"
    inline bool RogueToolButton(const char* id, const char* label, bool is_active, const ImVec2& size_arg = ImVec2(0, 0)) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        const ImGuiID imgui_id = window->GetID(id);
        const ImGuiStyle& style = ImGui::GetStyle();
        
        ImVec2 size = ImGui::CalcItemSize(size_arg, 80.0f, 40.0f);
        const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
        
        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, imgui_id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, imgui_id, &hovered, &held);

        // --- Rendering (The "Pro" Look) ---
        ImDrawList* draw_list = window->DrawList;
        ButtonStyle btn_style;

        // 1. Background Logic
        ImVec4 bg_col = hovered ? btn_style.col_hover : btn_style.col_idle;
        
        // "Alive" Pulse Effect for Active State
        if (is_active) {
            // 4Hz pulse for that "reactor core" feel
            float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 4.0f);
            
            // Lerp between Cyan and a brighter Highlight
            bg_col = ImVec4(
                btn_style.col_active.x, 
                btn_style.col_active.y + (0.2f * pulse), 
                btn_style.col_active.z + (0.2f * pulse), 
                1.0f
            );
        }

        draw_list->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(bg_col), btn_style.rounding);

        // 2. Technical Border (Y2K Style)
        // Active/Hover gets a sharper border
        if (is_active || hovered) {
            draw_list->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(1,1,1, is_active ? 0.6f : 0.3f)), btn_style.rounding, 0, 1.5f);
        } else {
            draw_list->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(btn_style.col_border), btn_style.rounding);
        }

        // 3. Label Rendering
        // Center text
        ImVec2 text_size = ImGui::CalcTextSize(label);
        ImVec2 text_pos = ImVec2(
            bb.Min.x + (size.x - text_size.x) * 0.5f,
            bb.Min.y + (size.y - text_size.y) * 0.5f
        );
        
        // Active text is black (contrast against cyan), Idle is white
        ImU32 text_col = is_active ? 0xFF000000 : 0xFFE0E0E0;
        draw_list->AddText(text_pos, text_col, label);

        // 4. Smart Tooltip (The "Informative" part)
        if (hovered && !is_active) {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0, 1, 1, 1), "ACTIVATE: %s", label);
            ImGui::Separator();
            ImGui::TextDisabled("Click to switch mode");
            ImGui::EndTooltip();
        }

        return pressed;
    }
}