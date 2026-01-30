/*
 * Olympe Tilemap Editor - Editor State Implementation
 */

#include "../include/EditorState.h"
#include <iostream>

namespace Olympe {
namespace Editor {

    // ========================================================================
    // PlaceEntityCommand Implementation
    // ========================================================================

    PlaceEntityCommand::PlaceEntityCommand(const std::string& prefabPath, const Vector& position)
        : m_prefabPath(prefabPath), m_position(position), m_entityId("")
    {
    }

    void PlaceEntityCommand::Execute(LevelManager& levelManager)
    {
        EntityInstance* entity = levelManager.CreateEntity(m_prefabPath);
        if (entity)
        {
            entity->position = m_position;
            m_entityId = entity->id;
            levelManager.MarkDirty();
        }
    }

    void PlaceEntityCommand::Undo(LevelManager& levelManager)
    {
        if (!m_entityId.empty())
        {
            levelManager.DeleteEntity(m_entityId);
            levelManager.MarkDirty();
        }
    }

    std::string PlaceEntityCommand::GetDescription() const
    {
        return "Place Entity: " + m_prefabPath;
    }

    // ========================================================================
    // SetTileCommand Implementation
    // ========================================================================

    SetTileCommand::SetTileCommand(int x, int y, int newTileId)
        : m_x(x), m_y(y), m_newTileId(newTileId), m_oldTileId(-1)
    {
    }

    void SetTileCommand::Execute(LevelManager& levelManager)
    {
        m_oldTileId = levelManager.GetTile(m_x, m_y);
        levelManager.SetTile(m_x, m_y, m_newTileId);
        levelManager.MarkDirty();
    }

    void SetTileCommand::Undo(LevelManager& levelManager)
    {
        if (m_oldTileId != -1)
        {
            levelManager.SetTile(m_x, m_y, m_oldTileId);
            levelManager.MarkDirty();
        }
    }

    std::string SetTileCommand::GetDescription() const
    {
        return "Set Tile at (" + std::to_string(m_x) + ", " + std::to_string(m_y) + ")";
    }

    // ========================================================================
    // MoveEntityCommand Implementation
    // ========================================================================

    MoveEntityCommand::MoveEntityCommand(const std::string& entityId, const Vector& newPosition)
        : m_entityId(entityId), m_newPosition(newPosition), m_oldPosition(0, 0, 0)
    {
    }

    void MoveEntityCommand::Execute(LevelManager& levelManager)
    {
        EntityInstance* entity = levelManager.GetEntity(m_entityId);
        if (entity)
        {
            m_oldPosition = entity->position;
            levelManager.UpdateEntityPosition(m_entityId, m_newPosition);
            levelManager.MarkDirty();
        }
    }

    void MoveEntityCommand::Undo(LevelManager& levelManager)
    {
        levelManager.UpdateEntityPosition(m_entityId, m_oldPosition);
        levelManager.MarkDirty();
    }

    std::string MoveEntityCommand::GetDescription() const
    {
        return "Move Entity: " + m_entityId;
    }

    // ========================================================================
    // DeleteEntityCommand Implementation
    // ========================================================================

    DeleteEntityCommand::DeleteEntityCommand(const std::string& entityId)
        : m_entityId(entityId), m_savedEntity(nullptr)
    {
    }

    void DeleteEntityCommand::Execute(LevelManager& levelManager)
    {
        EntityInstance* entity = levelManager.GetEntity(m_entityId);
        if (entity)
        {
            // Save a copy of the entity before deleting
            m_savedEntity = std::make_unique<EntityInstance>();
            m_savedEntity->id = entity->id;
            m_savedEntity->prefabPath = entity->prefabPath;
            m_savedEntity->name = entity->name;
            m_savedEntity->position = entity->position;
            m_savedEntity->overrides = entity->overrides;

            levelManager.DeleteEntity(m_entityId);
            levelManager.MarkDirty();
        }
    }

    void DeleteEntityCommand::Undo(LevelManager& levelManager)
    {
        if (m_savedEntity)
        {
            // Recreate the entity
            EntityInstance* entity = levelManager.CreateEntity(m_savedEntity->prefabPath);
            if (entity)
            {
                // Restore saved properties
                entity->id = m_savedEntity->id;
                entity->name = m_savedEntity->name;
                entity->position = m_savedEntity->position;
                entity->overrides = m_savedEntity->overrides;
                levelManager.MarkDirty();
            }
        }
    }

    std::string DeleteEntityCommand::GetDescription() const
    {
        return "Delete Entity: " + m_entityId;
    }

    // ========================================================================
    // SetCollisionCommand Implementation
    // ========================================================================

    SetCollisionCommand::SetCollisionCommand(int x, int y, uint8_t newMask)
        : m_x(x), m_y(y), m_newMask(newMask), m_oldMask(0)
    {
    }

    void SetCollisionCommand::Execute(LevelManager& levelManager)
    {
        m_oldMask = levelManager.GetCollision(m_x, m_y);
        levelManager.SetCollision(m_x, m_y, m_newMask);
        levelManager.MarkDirty();
    }

    void SetCollisionCommand::Undo(LevelManager& levelManager)
    {
        levelManager.SetCollision(m_x, m_y, m_oldMask);
        levelManager.MarkDirty();
    }

    std::string SetCollisionCommand::GetDescription() const
    {
        return "Set Collision at (" + std::to_string(m_x) + ", " + std::to_string(m_y) + ")";
    }

    // ========================================================================
    // EditorState Implementation
    // ========================================================================

    EditorState::EditorState()
        : m_historyIndex(0), m_maxHistorySize(100)
    {
    }

    EditorState::~EditorState()
    {
    }

    void EditorState::ExecuteCommand(std::unique_ptr<Command> command, LevelManager& levelManager)
    {
        if (!command)
        {
            std::cerr << "[EditorState] Attempted to execute null command" << std::endl;
            return;
        }

        // Execute the command
        command->Execute(levelManager);

        // Remove any commands after the current position (redo history)
        if (m_historyIndex < m_commandHistory.size())
        {
            m_commandHistory.erase(
                m_commandHistory.begin() + m_historyIndex,
                m_commandHistory.end()
            );
        }

        // Add the command to history
        m_commandHistory.push_back(std::move(command));
        m_historyIndex = m_commandHistory.size();

        // Trim history if it exceeds max size
        TrimHistory();

        std::cout << "[EditorState] Command executed: " << m_commandHistory.back()->GetDescription() << std::endl;
    }

    bool EditorState::Undo(LevelManager& levelManager)
    {
        if (!CanUndo())
        {
            return false;
        }

        m_historyIndex--;
        m_commandHistory[m_historyIndex]->Undo(levelManager);

        std::cout << "[EditorState] Undo: " << m_commandHistory[m_historyIndex]->GetDescription() << std::endl;
        return true;
    }

    bool EditorState::Redo(LevelManager& levelManager)
    {
        if (!CanRedo())
        {
            return false;
        }

        m_commandHistory[m_historyIndex]->Execute(levelManager);
        m_historyIndex++;

        std::cout << "[EditorState] Redo: " << m_commandHistory[m_historyIndex - 1]->GetDescription() << std::endl;
        return true;
    }

    bool EditorState::CanUndo() const
    {
        return m_historyIndex > 0;
    }

    bool EditorState::CanRedo() const
    {
        return m_historyIndex < m_commandHistory.size();
    }

    std::string EditorState::GetUndoDescription() const
    {
        if (CanUndo())
        {
            return m_commandHistory[m_historyIndex - 1]->GetDescription();
        }
        return "";
    }

    std::string EditorState::GetRedoDescription() const
    {
        if (CanRedo())
        {
            return m_commandHistory[m_historyIndex]->GetDescription();
        }
        return "";
    }

    void EditorState::ClearHistory()
    {
        m_commandHistory.clear();
        m_historyIndex = 0;
        std::cout << "[EditorState] History cleared" << std::endl;
    }

    void EditorState::TrimHistory()
    {
        // Remove oldest commands if we exceed max size
        while (m_commandHistory.size() > m_maxHistorySize)
        {
            m_commandHistory.pop_front();
            if (m_historyIndex > 0)
            {
                m_historyIndex--;
            }
        }
    }

} // namespace Editor
} // namespace Olympe
