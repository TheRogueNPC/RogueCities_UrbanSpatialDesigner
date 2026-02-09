#include "RogueCity/App/Tools/ContextWindowPopup.hpp"
#include <cstdio>
#include <cmath>

namespace RogueCity::App {

ContextWindowPopup::ContextWindowPopup() = default;
ContextWindowPopup::~ContextWindowPopup() = default;

void ContextWindowPopup::open(const ImVec2& screen_pos, float current_value,
                              float min_value, float max_value, const std::string& label) {
    is_open_ = true;
    popup_pos_ = screen_pos;
    current_value_ = current_value;
    min_value_ = min_value;
    max_value_ = max_value;
    label_ = label;
    
    // Initialize input buffer with current value
    snprintf(input_buffer_, sizeof(input_buffer_), "%.2f", current_value);
    
    ImGui::OpenPopup("##ContextValueInput");
}

void ContextWindowPopup::close() {
    is_open_ = false;
    ImGui::CloseCurrentPopup();
}

void ContextWindowPopup::update() {
    if (!is_open_) return;

    // Position popup at stored screen position (only when appearing)
    ImGui::SetNextWindowPos(popup_pos_, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(200, 0), ImGuiCond_Appearing);

    // Y2K styling: hard edges, warning stripe border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 200, 0, 255));  // Amber warning stripe
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(20, 20, 20, 240));  // Dark background

    if (ImGui::BeginPopup("##ContextValueInput", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        render_popup();
        ImGui::EndPopup();
    } else {
        is_open_ = false;
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void ContextWindowPopup::set_on_apply(OnApplyCallback callback) {
    on_apply_ = callback;
}

bool ContextWindowPopup::is_open() const {
    return is_open_;
}

void ContextWindowPopup::render_popup() {
    // Label with Y2K mono font style
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 200, 255));  // Cyan
    ImGui::Text("%s", label_.c_str());
    ImGui::PopStyleColor();
    
    ImGui::Separator();
    ImGui::Spacing();

    // Numeric input with auto-focus
    ImGui::SetNextItemWidth(-1);
    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }
    
    const bool enter_pressed = ImGui::InputText("##ValueInput", input_buffer_, sizeof(input_buffer_),
                                                 ImGuiInputTextFlags_EnterReturnsTrue | 
                                                 ImGuiInputTextFlags_CharsDecimal);

    // Min/Max hints
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
    ImGui::Text("Range: %.2f - %.2f", min_value_, max_value_);
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Apply/Cancel buttons (Y2K capsule style)
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 200, 100, 255));  // Green
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 255, 120, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 180, 90, 255));
    
    if (ImGui::Button("APPLY", ImVec2(90, 0)) || enter_pressed) {
        // Parse and apply value
        float new_value = 0.0f;
        if (sscanf_s(input_buffer_, "%f", &new_value) == 1) {
            new_value = std::max(min_value_, std::min(new_value, max_value_));
            apply_with_interpolation(new_value);
        }
        close();
    }
    
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(200, 0, 0, 255));  // Red
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 0, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 0, 0, 255));
    
    if (ImGui::Button("CANCEL", ImVec2(90, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        close();
    }
    
    ImGui::PopStyleColor(3);
}

void ContextWindowPopup::apply_with_interpolation(float new_value) {
    if (on_apply_) {
        on_apply_(new_value);
        
        // TODO: Trigger smooth interpolation animation (0.3s ease-in-out)
        // This would be handled by the AxiomVisual's ring update logic
    }
}

} // namespace RogueCity::App
