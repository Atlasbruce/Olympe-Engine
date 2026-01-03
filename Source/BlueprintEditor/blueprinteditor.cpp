/*
 * Olympe Blueprint Editor - Backend Implementation
 * 
 * Singleton backend managing business logic, state, and data
 * Completely separated from UI rendering (handled by BlueprintEditorGUI)
 */

#include "BlueprintEditor.h"
#include "EntityBlueprint.h"

using namespace Olympe::Blueprint;

namespace Olympe
{
    // ========================================================================
    // BlueprintEditor Singleton Backend Implementation
    // ========================================================================
    
    BlueprintEditor& BlueprintEditor::Instance()
    {
        static BlueprintEditor instance;
        return instance;
    }

    BlueprintEditor::BlueprintEditor()
        : m_IsActive(false)
        , m_HasUnsavedChanges(false)
        , m_AssetRootPath("Blueprints")
    {
    }

    BlueprintEditor::~BlueprintEditor()
    {
    }

    void BlueprintEditor::Initialize()
    {
        // Initialize backend state
        m_IsActive = false;
        m_HasUnsavedChanges = false;
        m_CurrentBlueprint = Blueprint::EntityBlueprint();
        m_CurrentFilepath = "";
    }

    void BlueprintEditor::Shutdown()
    {
        // Clean up backend resources
        m_CurrentBlueprint = Blueprint::EntityBlueprint();
        m_CurrentFilepath = "";
        m_HasUnsavedChanges = false;
    }

    void BlueprintEditor::Update(float deltaTime)
    {
        // Backend update logic (non-UI)
        // This is called by GameEngine when the editor is active
        // Can be used for background tasks, auto-save, etc.
        
        // For now, this is a placeholder for future backend logic
        // such as:
        // - Auto-save timer
        // - Asset watching/hot-reload
        // - Background compilation
        // - Validation
    }

    // Blueprint operations
    void BlueprintEditor::NewBlueprint(const std::string& name, const std::string& description)
    {
        m_CurrentBlueprint = Blueprint::EntityBlueprint(name);
        m_CurrentBlueprint.description = description;
        m_CurrentFilepath = "";
        m_HasUnsavedChanges = true;
    }

    bool BlueprintEditor::LoadBlueprint(const std::string& filepath)
    {
        Blueprint::EntityBlueprint loaded = Blueprint::EntityBlueprint::LoadFromFile(filepath);
        
        if (loaded.name.empty())
        {
            return false;
        }
        
        m_CurrentBlueprint = loaded;
        m_CurrentFilepath = filepath;
        m_HasUnsavedChanges = false;
        return true;
    }

    bool BlueprintEditor::SaveBlueprint()
    {
        if (m_CurrentBlueprint.name.empty())
        {
            return false;
        }
        
        if (m_CurrentFilepath.empty())
        {
            // No filepath set - caller should use SaveBlueprintAs
            return false;
        }
        
        bool success = m_CurrentBlueprint.SaveToFile(m_CurrentFilepath);
        
        if (success)
        {
            m_HasUnsavedChanges = false;
        }
        
        return success;
    }

    bool BlueprintEditor::SaveBlueprintAs(const std::string& filepath)
    {
        if (m_CurrentBlueprint.name.empty())
        {
            return false;
        }
        
        bool success = m_CurrentBlueprint.SaveToFile(filepath);
        
        if (success)
        {
            m_CurrentFilepath = filepath;
            m_HasUnsavedChanges = false;
        }
        
        return success;
    }

} // namespace Olympe
