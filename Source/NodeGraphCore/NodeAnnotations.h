/**
 * @file NodeAnnotations.h
 * @brief Per-node annotations manager for node graphs (Phase 2.0)
 * @author Olympe Engine
 * @date 2026-02-19
 *
 * @details
 * Provides a serializable system for per-node annotations, including:
 * - Breakpoints (for debugging)
 * - Text comments
 * - Custom node colors (RGBA floats)
 */

#pragma once

#include "../json_helper.h"
#include <string>
#include <map>

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// NodeAnnotation struct
// ============================================================================

/**
 * @struct NodeAnnotation
 * @brief Holds annotation data for a single node
 */
struct NodeAnnotation {
    int nodeId = 0;
    bool hasBreakpoint = false;
    std::string comment;
    float colorR = 1.0f;
    float colorG = 1.0f;
    float colorB = 1.0f;
    float colorA = 1.0f;
};

// ============================================================================
// NodeAnnotationsManager class
// ============================================================================

/**
 * @class NodeAnnotationsManager
 * @brief Manages per-node annotations (breakpoints, comments, colors)
 */
class NodeAnnotationsManager {
public:
    NodeAnnotationsManager() = default;
    ~NodeAnnotationsManager() = default;

    /**
     * @brief Check if a node has any annotation
     * @param nodeId Node identifier
     * @return true if an annotation exists for this node
     */
    bool HasAnnotation(int nodeId) const;

    /**
     * @brief Get annotation for a node (non-const)
     * @param nodeId Node identifier
     * @return Pointer to annotation, or nullptr if not found
     */
    NodeAnnotation* GetAnnotation(int nodeId);

    /**
     * @brief Get annotation for a node (const)
     * @param nodeId Node identifier
     * @return Const pointer to annotation, or nullptr if not found
     */
    const NodeAnnotation* GetAnnotation(int nodeId) const;

    /**
     * @brief Set breakpoint state for a node
     * @param nodeId Node identifier
     * @param enabled Whether breakpoint is active
     */
    void SetBreakpoint(int nodeId, bool enabled);

    /**
     * @brief Set comment for a node
     * @param nodeId Node identifier
     * @param text Comment text
     */
    void SetComment(int nodeId, const std::string& text);

    /**
     * @brief Set custom color for a node
     * @param nodeId Node identifier
     * @param r Red component [0,1]
     * @param g Green component [0,1]
     * @param b Blue component [0,1]
     * @param a Alpha component [0,1]
     */
    void SetColor(int nodeId, float r, float g, float b, float a);

    /**
     * @brief Remove all annotation data for a node
     * @param nodeId Node identifier
     */
    void ClearAnnotation(int nodeId);

    /**
     * @brief Get all annotations (for serialization / rendering)
     * @return Const reference to internal map
     */
    const std::map<int, NodeAnnotation>& GetAll() const;

    /**
     * @brief Serialize all annotations to JSON
     * @return JSON array of annotation objects
     */
    json ToJson() const;

    /**
     * @brief Deserialize annotations from JSON
     * @param j JSON array of annotation objects
     */
    void FromJson(const json& j);

private:
    std::map<int, NodeAnnotation> m_annotations;

    /**
     * @brief Get or create annotation for a node
     * @param nodeId Node identifier
     * @return Reference to annotation
     */
    NodeAnnotation& GetOrCreate(int nodeId);
};

} // namespace NodeGraph
} // namespace Olympe
