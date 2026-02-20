// FILE: CommandHistory.hpp
// PURPOSE: Editor command history for undo/redo.
#pragma once

#include <memory>
#include <vector>

namespace RogueCity::App {

class ICommand { // Interface for editor commands that can be executed, undone, and redone.
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual const char* GetDescription() const = 0;
};

class CommandHistory { // Manages a history of editor commands for undo/redo functionality.
public:
    void Execute(std::unique_ptr<ICommand> cmd);
    void Commit(std::unique_ptr<ICommand> cmd);

    void Undo();
    void Redo();

    [[nodiscard]] bool CanUndo() const;
    [[nodiscard]] bool CanRedo() const;

    [[nodiscard]] const ICommand* PeekUndo() const;
    [[nodiscard]] const ICommand* PeekRedo() const;

    void Clear();

private:
    void TrimRedo();

    std::vector<std::unique_ptr<ICommand>> history_{};
    size_t cursor_{ 0 };
};

/// Shared editor-wide command history for panels/tools that participate in global undo/redo.
CommandHistory& GetEditorCommandHistory(); 
void ResetEditorCommandHistory();

} // namespace RogueCity::App
