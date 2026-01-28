/*
 * Olympe Tilemap Editor - Editor State & Command Pattern
 * 
 * Manages editor history with Undo/Redo functionality using the Command pattern.
 */

#pragma once

#include "LevelManager.h"
#include <memory>
#include <vector>
#include <string>
#include <deque>

namespace Olympe {
namespace Editor {

    // Forward declaration
    class LevelManager;

    // ========================================================================
    // Command Pattern - Abstract Base Class
    // ========================================================================

    class Command
    {
    public:
        virtual ~Command() = default;

        // Execute the command
        virtual void Execute(LevelManager& levelManager) = 0;

        // Undo the command
        virtual void Undo(LevelManager& levelManager) = 0;

        // Get a human-readable description of the command
        virtual std::string GetDescription() const = 0;
    };

    // ========================================================================
    // Concrete Commands
    // ========================================================================

    // Command to place/create a new entity
    class PlaceEntityCommand : public Command
    {
    public:
        PlaceEntityCommand(const std::string& prefabPath, const Vec2& position);
        ~PlaceEntityCommand() override = default;

        void Execute(LevelManager& levelManager) override;
        void Undo(LevelManager& levelManager) override;
        std::string GetDescription() const override;

    private:
        std::string m_prefabPath;
        Vector m_position;
        std::string m_entityId;  // Stored after execution
    };

    // Command to set a tile at a specific position
    class SetTileCommand : public Command
    {
    public:
        SetTileCommand(int x, int y, int newTileId);
        ~SetTileCommand() override = default;

        void Execute(LevelManager& levelManager) override;
        void Undo(LevelManager& levelManager) override;
        std::string GetDescription() const override;

    private:
        int m_x;
        int m_y;
        int m_newTileId;
        int m_oldTileId;
    };

    // Command to move an existing entity
    class MoveEntityCommand : public Command
    {
    public:
        MoveEntityCommand(const std::string& entityId, const Vector& newPosition);
        ~MoveEntityCommand() override = default;

        void Execute(LevelManager& levelManager) override;
        void Undo(LevelManager& levelManager) override;
        std::string GetDescription() const override;

    private:
        std::string m_entityId;
        Vector m_newPosition;
        Vector m_oldPosition;
    };

    // Command to delete an entity
    class DeleteEntityCommand : public Command
    {
    public:
        DeleteEntityCommand(const std::string& entityId);
        ~DeleteEntityCommand() override = default;

        void Execute(LevelManager& levelManager) override;
        void Undo(LevelManager& levelManager) override;
        std::string GetDescription() const override;

    private:
        std::string m_entityId;
        std::unique_ptr<EntityInstance> m_savedEntity;
    };

    // Command to set collision at a specific position
    class SetCollisionCommand : public Command
    {
    public:
        SetCollisionCommand(int x, int y, uint8_t newMask);
        ~SetCollisionCommand() override = default;

        void Execute(LevelManager& levelManager) override;
        void Undo(LevelManager& levelManager) override;
        std::string GetDescription() const override;

    private:
        int m_x;
        int m_y;
        uint8_t m_newMask;
        uint8_t m_oldMask;
    };

    // ========================================================================
    // EditorState - Manages Command History and Undo/Redo
    // ========================================================================

    class EditorState
    {
    public:
        EditorState();
        ~EditorState();

        // Execute a command and add it to history
        void ExecuteCommand(std::unique_ptr<Command> command, LevelManager& levelManager);

        // Undo the last command
        bool Undo(LevelManager& levelManager);

        // Redo a previously undone command
        bool Redo(LevelManager& levelManager);

        // Check if undo/redo is available
        bool CanUndo() const;
        bool CanRedo() const;

        // Get description of the next undo/redo operation
        std::string GetUndoDescription() const;
        std::string GetRedoDescription() const;

        // Clear all history
        void ClearHistory();

        // Get history information
        size_t GetHistorySize() const { return m_commandHistory.size(); }
        size_t GetHistoryIndex() const { return m_historyIndex; }

        // Set maximum history size (default: 100)
        void SetMaxHistorySize(size_t maxSize) { m_maxHistorySize = maxSize; }

    private:
        void TrimHistory();

        std::deque<std::unique_ptr<Command>> m_commandHistory;
        size_t m_historyIndex;  // Current position in history
        size_t m_maxHistorySize;
    };

} // namespace Editor
} // namespace Olympe
