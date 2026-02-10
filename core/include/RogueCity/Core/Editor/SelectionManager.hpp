// FILE: SelectionManager.hpp
// PURPOSE: Multi-select aware selection manager for editor entities.
#pragma once

#include "RogueCity/Core/Editor/ViewportIndex.hpp"

#include <algorithm>
#include <span>
#include <vector>

namespace RogueCity::Core::Editor {

struct SelectionItem {
    VpEntityKind kind{ VpEntityKind::Unknown };
    uint32_t id{ 0 };
};

class SelectionManager {
public:
    void Clear() { items_.clear(); }

    void SetItems(std::vector<SelectionItem> items) { items_ = std::move(items); }

    void Select(VpEntityKind kind, uint32_t id) {
        items_.clear();
        items_.push_back({ kind, id });
    }

    void Add(VpEntityKind kind, uint32_t id) {
        if (IsSelected(kind, id)) {
            return;
        }
        items_.push_back({ kind, id });
    }

    void Toggle(VpEntityKind kind, uint32_t id) {
        auto it = std::find_if(items_.begin(), items_.end(),
            [kind, id](const SelectionItem& item) {
                return item.kind == kind && item.id == id;
            });
        if (it != items_.end()) {
            items_.erase(it);
        } else {
            items_.push_back({ kind, id });
        }
    }

    [[nodiscard]] bool IsSelected(VpEntityKind kind, uint32_t id) const {
        return std::any_of(items_.begin(), items_.end(),
            [kind, id](const SelectionItem& item) {
                return item.kind == kind && item.id == id;
            });
    }

    [[nodiscard]] size_t Count() const { return items_.size(); }
    [[nodiscard]] bool IsMultiSelect() const { return items_.size() > 1; }

    [[nodiscard]] const SelectionItem* Primary() const {
        if (items_.empty()) {
            return nullptr;
        }
        return &items_.front();
    }

    [[nodiscard]] std::span<const SelectionItem> Items() const {
        return std::span<const SelectionItem>(items_.data(), items_.size());
    }

private:
    std::vector<SelectionItem> items_{};
};

} // namespace RogueCity::Core::Editor
