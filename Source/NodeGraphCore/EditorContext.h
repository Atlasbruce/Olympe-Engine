/**
 * @file EditorContext.h
 * @brief Editor mode and capabilities management
 * @author Olympe Engine - NodeGraph Core v2.0
 * @date 2025-02-19
 *
 * @details
 * Manages editor context (mode and capabilities) for node graph editors.
 * Inspired by Unreal Engine's editor mode system.
 */

#pragma once

namespace Olympe
{
namespace NodeGraph
{

/**
 * @enum EditorMode
 * @brief Mode d'édition du graphe
 */
enum class EditorMode
{
    Editor,      // Édition complète (CRUD)
    Visualizer,  // Visualisation seule (read-only)
    Debug        // Debug runtime (read-only + highlighting)
};

/**
 * @struct EditorCapabilities
 * @brief Capacités disponibles selon le mode
 */
struct EditorCapabilities
{
    bool canCreateNodes = false;
    bool canDeleteNodes = false;
    bool canMoveNodes = false;
    bool canConnectPins = false;
    bool canEditProperties = false;
    bool canUndo = false;
    bool canRedo = false;
    bool canSave = false;
    bool showRuntimeState = false;
};

/**
 * @class EditorContext
 * @brief Contexte d'édition pour un graphe
 *
 * Defines what operations are allowed based on the current mode:
 * - Editor: Full CRUD operations
 * - Visualizer: Read-only view
 * - Debug: Read-only + runtime state visualization
 */
class EditorContext
{
public:
    /**
     * @brief Créer un contexte en mode édition
     */
    static EditorContext CreateEditor();
    
    /**
     * @brief Créer un contexte en mode visualisation
     */
    static EditorContext CreateVisualizer();
    
    /**
     * @brief Créer un contexte en mode debug
     */
    static EditorContext CreateDebugger();
    
    /**
     * @brief Obtenir le mode actuel
     */
    EditorMode GetMode() const { return m_mode; }
    
    /**
     * @brief Obtenir les capacités
     */
    const EditorCapabilities& GetCapabilities() const { return m_capabilities; }
    
    /**
     * @brief Vérifier si une action est autorisée
     */
    bool CanCreateNodes() const { return m_capabilities.canCreateNodes; }
    bool CanDeleteNodes() const { return m_capabilities.canDeleteNodes; }
    bool CanMoveNodes() const { return m_capabilities.canMoveNodes; }
    bool CanConnectPins() const { return m_capabilities.canConnectPins; }
    bool CanEditProperties() const { return m_capabilities.canEditProperties; }
    bool CanUndo() const { return m_capabilities.canUndo; }
    bool CanRedo() const { return m_capabilities.canRedo; }
    bool CanSave() const { return m_capabilities.canSave; }
    bool ShowRuntimeState() const { return m_capabilities.showRuntimeState; }
    
private:
    EditorContext(EditorMode mode);
    
    EditorMode m_mode = EditorMode::Editor;
    EditorCapabilities m_capabilities;
};

} // namespace NodeGraph
} // namespace Olympe
