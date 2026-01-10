/*
 * Olympe Blueprint Editor - Editor Context
 * 
 * Defines capability-driven editor modes (Runtime vs Standalone)
 * Controls what operations are available in each mode
 */

#pragma once

namespace Olympe
{
    /**
     * EditorCapabilities - Defines what operations are enabled
     * Used to gate UI elements and functionality
     */
    struct EditorCapabilities
    {
        bool isRuntime;          // True for Runtime Editor, false for Standalone
        bool canCreate;          // Can create new nodes/links
        bool canEdit;            // Can edit existing nodes
        bool canDelete;          // Can delete nodes/links
        bool canLink;            // Can create/remove links
        bool canSave;            // Can save changes to disk
        bool showEntityContext;  // Show entity selection context
        
        // Default constructor: Standalone mode (full capabilities)
        EditorCapabilities()
            : isRuntime(false)
            , canCreate(true)
            , canEdit(true)
            , canDelete(true)
            , canLink(true)
            , canSave(true)
            , showEntityContext(false)
        {
        }
        
        // Runtime mode: read-only visualization
        static EditorCapabilities Runtime()
        {
            EditorCapabilities caps;
            caps.isRuntime = true;
            caps.canCreate = false;
            caps.canEdit = false;
            caps.canDelete = false;
            caps.canLink = false;
            caps.canSave = false;
            caps.showEntityContext = true;
            return caps;
        }
        
        // Standalone mode: full CRUD operations
        static EditorCapabilities Standalone()
        {
            EditorCapabilities caps;
            caps.isRuntime = false;
            caps.canCreate = true;
            caps.canEdit = true;
            caps.canDelete = true;
            caps.canLink = true;
            caps.canSave = true;
            caps.showEntityContext = false;
            return caps;
        }
    };
    
    /**
     * EditorContext - Singleton managing editor mode and capabilities
     * Provides global access to current editor configuration
     */
    class EditorContext
    {
    public:
        static EditorContext& Instance();
        static EditorContext& Get() { return Instance(); }
        
        // Initialize editor mode
        void InitializeRuntime();
        void InitializeStandalone();
        
        // Query capabilities
        const EditorCapabilities& GetCapabilities() const { return m_Capabilities; }
        bool IsRuntime() const { return m_Capabilities.isRuntime; }
        bool IsStandalone() const { return !m_Capabilities.isRuntime; }
        
        bool CanCreate() const { return m_Capabilities.canCreate; }
        bool CanEdit() const { return m_Capabilities.canEdit; }
        bool CanDelete() const { return m_Capabilities.canDelete; }
        bool CanLink() const { return m_Capabilities.canLink; }
        bool CanSave() const { return m_Capabilities.canSave; }
        bool ShowEntityContext() const { return m_Capabilities.showEntityContext; }
        
    private:
        EditorContext();
        ~EditorContext();
        
        EditorContext(const EditorContext&) = delete;
        EditorContext& operator=(const EditorContext&) = delete;
        
    private:
        EditorCapabilities m_Capabilities;
    };
}
