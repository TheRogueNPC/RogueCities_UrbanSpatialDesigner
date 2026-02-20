// FILE: CommandHistory.cpp
// PURPOSE: Editor command history implementation.
#include "RogueCity/App/Editor/CommandHistory.hpp"

namespace RogueCity::App {

namespace {
CommandHistory g_editor_command_history{};
}

void CommandHistory::TrimRedo() {
    if (cursor_ < history_.size()) {
        history_.erase(history_.begin() + static_cast<std::ptrdiff_t>(cursor_), history_.end());
    }
}

void CommandHistory::Execute(std::unique_ptr<ICommand> cmd) {
    if (!cmd) {
        return;
    }
    TrimRedo();
    cmd->Execute();
    history_.push_back(std::move(cmd));
    cursor_ = history_.size();
}

void CommandHistory::Commit(std::unique_ptr<ICommand> cmd) {
    if (!cmd) {
        return;
    }
    TrimRedo();
    history_.push_back(std::move(cmd));
    cursor_ = history_.size();
}

void CommandHistory::Undo() {
    if (!CanUndo()) {
        return;
    }
    cursor_ -= 1;
    history_[cursor_]->Undo();
}

void CommandHistory::Redo() {
    if (!CanRedo()) {
        return;
    }
    history_[cursor_]->Execute();
    cursor_ += 1;
}

bool CommandHistory::CanUndo() const {
    return cursor_ > 0;
}

bool CommandHistory::CanRedo() const {
    return cursor_ < history_.size();
}

const ICommand* CommandHistory::PeekUndo() const {
    if (!CanUndo()) {
        return nullptr;
    }
    return history_[cursor_ - 1].get();
}

const ICommand* CommandHistory::PeekRedo() const {
    if (!CanRedo()) {
        return nullptr;
    }
    return history_[cursor_].get();
}

void CommandHistory::Clear() {
    history_.clear();
    cursor_ = 0;
}

CommandHistory& GetEditorCommandHistory() {
    return g_editor_command_history;
}

void ResetEditorCommandHistory() {
    g_editor_command_history.Clear();
}

} // namespace RogueCity::App
