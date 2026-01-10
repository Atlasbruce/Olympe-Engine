/*
 * Olympe Blueprint Editor - Editor Context Implementation
 */

#include "EditorContext.h"
#include <iostream>

namespace Olympe
{
    EditorContext& EditorContext::Instance()
    {
        static EditorContext instance;
        return instance;
    }
    
    EditorContext::EditorContext()
    {
        // Default to Standalone mode
        m_Capabilities = EditorCapabilities::Standalone();
        std::cout << "[EditorContext] Initialized in Standalone mode\n";
    }
    
    EditorContext::~EditorContext()
    {
    }
    
    void EditorContext::InitializeRuntime()
    {
        m_Capabilities = EditorCapabilities::Runtime();
        std::cout << "[EditorContext] Switched to Runtime mode (read-only)\n";
    }
    
    void EditorContext::InitializeStandalone()
    {
        m_Capabilities = EditorCapabilities::Standalone();
        std::cout << "[EditorContext] Switched to Standalone mode (full CRUD)\n";
    }
}
