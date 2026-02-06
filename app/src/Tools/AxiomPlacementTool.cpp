#include "RogueCity/App/Tools/AxiomPlacementTool.hpp"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/App/Tools/ContextWindowPopup.hpp"
#include <imgui.h>

namespace RogueCity::App {

AxiomPlacementTool::AxiomPlacementTool() = default;
AxiomPlacementTool::~AxiomPlacementTool() = default;

void AxiomPlacementTool::update(float delta_time, PrimaryViewport& viewport) {
    // Update all axiom animations
    for (auto& axiom : axioms_) {
        axiom->update(delta_time);
    }

    // Update hover state
    if (viewport.is_hovered()) {
        const Core::Vec2 mouse_world = viewport.screen_to_world(ImGui::GetMousePos());
        hovered_axiom_id_ = -1;

        // Check for axiom hover (reverse order for top-most)
        for (auto it = axioms_.rbegin(); it != axioms_.rend(); ++it) {
            if ((*it)->is_hovered(mouse_world, 15.0f)) {  // 15m interaction radius
                hovered_axiom_id_ = (*it)->id();
                break;
            }
        }

        for (auto& axiom : axioms_) {
            axiom->set_hovered(axiom->id() == hovered_axiom_id_);
            axiom->set_selected(axiom->id() == selected_axiom_id_);
        }

        // Update knob hover state if axiom selected (interaction begins in on_mouse_down)
        if (selected_axiom_id_ != -1) {
            if (auto* selected = get_selected_axiom()) {
                (void)selected->get_hovered_knob(mouse_world);
            }
        }
    }
}

void AxiomPlacementTool::render(ImDrawList* draw_list, const PrimaryViewport& viewport) {
    // Render all axioms
    for (auto& axiom : axioms_) {
        axiom->render(draw_list, viewport);
    }

    // Render ghost preview when placing
    if (mode_ == Mode::Placing || mode_ == Mode::DraggingSize) {
        const ImVec2 screen_center = viewport.world_to_screen(placement_start_pos_);
        const float screen_radius = viewport.world_to_screen_scale(ghost_radius_);
        
        // Y2K style ghost: dashed circle in white
        const ImU32 ghost_color = IM_COL32(255, 255, 255, 150);
        draw_list->AddCircle(screen_center, screen_radius, ghost_color, 64, 2.0f);
        
        // Draw radius text
        char radius_text[32];
        snprintf(radius_text, sizeof(radius_text), "%.0fm", ghost_radius_);
        const ImVec2 text_size = ImGui::CalcTextSize(radius_text);
        draw_list->AddText(
            ImVec2(screen_center.x - text_size.x * 0.5f, screen_center.y - screen_radius - 20.0f),
            IM_COL32(255, 255, 255, 255),
            radius_text
        );
    }
}

void AxiomPlacementTool::on_mouse_down(const Core::Vec2& world_pos) {
    if (mode_ == Mode::Idle) {
        // If a knob is hovered on the selected axiom, drag radius.
        if (auto* selected = get_selected_axiom()) {
            if (auto* knob = selected->get_hovered_knob(world_pos)) {
                dragging_knob_ = knob;
                knob_drag_start_ = world_pos;
                knob->is_dragging = true;
                mode_ = Mode::DraggingKnob;
                return;
            }
        }

        // If clicking an existing axiom, select (and drag to move).
        for (auto it = axioms_.rbegin(); it != axioms_.rend(); ++it) {
            if ((*it)->is_hovered(world_pos, 15.0f)) {
                selected_axiom_id_ = (*it)->id();
                axiom_drag_offset_ = world_pos - (*it)->position();
                mode_ = Mode::DraggingAxiom;
                return;
            }
        }

        // Otherwise, start placing a new axiom.
        placement_start_pos_ = world_pos;
        ghost_radius_ = 100.0f;
        mode_ = Mode::DraggingSize;
    }
}

void AxiomPlacementTool::on_mouse_up(const Core::Vec2& world_pos) {
    if (mode_ == Mode::Placing || mode_ == Mode::DraggingSize) {
        // Finalize axiom placement
        auto axiom = std::make_unique<AxiomVisual>(next_axiom_id_++, default_type_);
        axiom->set_position(placement_start_pos_);
        axiom->set_radius(ghost_radius_);
        axiom->set_animation_enabled(animation_enabled_);
        axiom->trigger_placement_animation();
        
        selected_axiom_id_ = axiom->id();
        axioms_.push_back(std::move(axiom));
        dirty_ = true;
        
        mode_ = Mode::Idle;
    } else if (mode_ == Mode::DraggingAxiom) {
        (void)world_pos;
        dirty_ = true;
        mode_ = Mode::Idle;
    } else if (mode_ == Mode::DraggingKnob) {
        if (dragging_knob_) {
            dragging_knob_->is_dragging = false;
            dragging_knob_ = nullptr;
        }
        dirty_ = true;
        mode_ = Mode::Idle;
    }
}

void AxiomPlacementTool::on_mouse_move(const Core::Vec2& world_pos) {
    if (mode_ == Mode::DraggingSize) {
        // Update ghost radius based on drag distance
        const float dx = world_pos.x - placement_start_pos_.x;
        const float dy = world_pos.y - placement_start_pos_.y;
        ghost_radius_ = sqrtf(dx * dx + dy * dy);
        ghost_radius_ = std::max(50.0f, std::min(ghost_radius_, 1000.0f));
    } else if (mode_ == Mode::DraggingAxiom) {
        auto* axiom = get_selected_axiom();
        if (axiom) {
            axiom->set_position(world_pos - axiom_drag_offset_);
        }
    } else if (mode_ == Mode::DraggingKnob && dragging_knob_) {
        // Update ring radius via knob drag
        auto* axiom = get_selected_axiom();
        if (axiom) {
            const float dx = world_pos.x - axiom->position().x;
            const float dy = world_pos.y - axiom->position().y;
            const float new_radius = sqrtf(dx * dx + dy * dy);
            
            // Update ring radius (clamped)
            const float clamped_radius = std::max(50.0f, std::min(new_radius, 1000.0f));
            axiom->set_radius(clamped_radius);
        }
    }
}

void AxiomPlacementTool::on_right_click(const Core::Vec2& world_pos) {
    // Delete top-most axiom under cursor
    for (auto it = axioms_.rbegin(); it != axioms_.rend(); ++it) {
        if ((*it)->is_hovered(world_pos, 15.0f)) {
            remove_axiom((*it)->id());
            break;
        }
    }
}

void AxiomPlacementTool::add_axiom(std::unique_ptr<AxiomVisual> axiom) {
    axioms_.push_back(std::move(axiom));
}

void AxiomPlacementTool::remove_axiom(int axiom_id) {
    axioms_.erase(
        std::remove_if(axioms_.begin(), axioms_.end(),
            [axiom_id](const auto& a) { return a->id() == axiom_id; }),
        axioms_.end()
    );
    dirty_ = true;
    
    if (selected_axiom_id_ == axiom_id) {
        selected_axiom_id_ = -1;
    }
    if (hovered_axiom_id_ == axiom_id) {
        hovered_axiom_id_ = -1;
    }
}

void AxiomPlacementTool::clear_axioms() {
    axioms_.clear();
    selected_axiom_id_ = -1;
    hovered_axiom_id_ = -1;
    dirty_ = true;
}

const std::vector<std::unique_ptr<AxiomVisual>>& AxiomPlacementTool::axioms() const {
    return axioms_;
}

AxiomVisual* AxiomPlacementTool::get_selected_axiom() {
    for (auto& axiom : axioms_) {
        if (axiom->id() == selected_axiom_id_) {
            return axiom.get();
        }
    }
    return nullptr;
}

void AxiomPlacementTool::set_default_axiom_type(AxiomVisual::AxiomType type) {
    default_type_ = type;
}

AxiomVisual::AxiomType AxiomPlacementTool::default_axiom_type() const {
    return default_type_;
}

void AxiomPlacementTool::set_animation_enabled(bool enabled) {
    animation_enabled_ = enabled;
}

std::vector<Generators::CityGenerator::AxiomInput> AxiomPlacementTool::get_axiom_inputs() const {
    std::vector<Generators::CityGenerator::AxiomInput> inputs;
    inputs.reserve(axioms_.size());
    
    for (const auto& axiom : axioms_) {
        inputs.push_back(axiom->to_axiom_input());
    }
    
    return inputs;
}

bool AxiomPlacementTool::consume_dirty() {
    const bool was_dirty = dirty_;
    dirty_ = false;
    return was_dirty;
}

bool AxiomPlacementTool::is_interacting() const {
    return mode_ == Mode::Placing || mode_ == Mode::DraggingSize || mode_ == Mode::DraggingAxiom || mode_ == Mode::DraggingKnob;
}

} // namespace RogueCity::App
