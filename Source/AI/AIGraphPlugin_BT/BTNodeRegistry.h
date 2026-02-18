/**
 * @file BTNodeRegistry.h
 * @brief Registry of all Behavior Tree node types for AIGraphPlugin_BT
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Central registry that stores metadata for all BT node types (Composites,
 * Decorators, Conditions, Actions). Provides type information including
 * display names, categories, colors, child constraints, and parameters.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace Olympe {
namespace AI {

/**
 * @enum BTNodeCategory
 * @brief Categories of behavior tree nodes
 */
enum class BTNodeCategory : uint8_t {
    Composite,   ///< Flow control nodes (Selector, Sequence, Parallel)
    Decorator,   ///< Modifiers (Inverter, Repeater, Cooldown, etc.)
    Condition,   ///< Boolean checks (HasTarget, InRange, etc.)
    Action       ///< Leaf execution nodes (Wait, Move, Attack, etc.)
};

/**
 * @struct BTNodeTypeInfo
 * @brief Metadata for a behavior tree node type
 */
struct BTNodeTypeInfo {
    std::string typeName;              ///< Type identifier (e.g., "BT_Selector")
    std::string displayName;           ///< Human-readable name (e.g., "Selector")
    std::string description;           ///< Description of functionality
    BTNodeCategory category;           ///< Node category
    uint32_t color = 0xFFFFFFFF;       ///< RGBA color (0xAABBGGRR format)
    std::string icon;                  ///< Unicode icon or symbol
    int minChildren = -1;              ///< Minimum children (-1 = no limit)
    int maxChildren = -1;              ///< Maximum children (-1 = no limit)
    bool allowsDecorator = false;      ///< Can this node be decorated?
    std::vector<std::string> parameterNames; ///< Parameter names for this type
};

/**
 * @class BTNodeRegistry
 * @brief Singleton registry for all BT node types
 *
 * @details
 * Manages metadata for all behavior tree node types. Automatically initializes
 * built-in types on first access. Provides queries by type name or category.
 */
class BTNodeRegistry {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to global registry
     */
    static BTNodeRegistry& Get();
    
    /**
     * @brief Register a new node type
     * @param info Node type metadata
     */
    void RegisterNodeType(const BTNodeTypeInfo& info);
    
    /**
     * @brief Get metadata for a node type
     * @param typeName Node type identifier
     * @return Pointer to metadata or nullptr if not found
     */
    const BTNodeTypeInfo* GetNodeTypeInfo(const std::string& typeName) const;
    
    /**
     * @brief Get all registered node type names
     * @return Vector of type names
     */
    std::vector<std::string> GetAllNodeTypes() const;
    
    /**
     * @brief Get node types in a specific category
     * @param category Category to filter by
     * @return Vector of type names in the category
     */
    std::vector<std::string> GetNodeTypesByCategory(BTNodeCategory category) const;
    
    /**
     * @brief Check if a node type exists
     * @param typeName Type name to check
     * @return true if registered, false otherwise
     */
    bool IsValidNodeType(const std::string& typeName) const;
    
    /**
     * @brief Check if a node type can have children
     * @param typeName Type name to check
     * @return true if it can have children
     */
    bool CanHaveChildren(const std::string& typeName) const;
    
    /**
     * @brief Get minimum number of children for a node type
     * @param typeName Type name to query
     * @return Minimum children count (-1 = no limit)
     */
    int GetMinChildren(const std::string& typeName) const;
    
    /**
     * @brief Get maximum number of children for a node type
     * @param typeName Type name to query
     * @return Maximum children count (-1 = no limit)
     */
    int GetMaxChildren(const std::string& typeName) const;
    
private:
    BTNodeRegistry();
    ~BTNodeRegistry() = default;
    
    BTNodeRegistry(const BTNodeRegistry&) = delete;
    BTNodeRegistry& operator=(const BTNodeRegistry&) = delete;
    
    /**
     * @brief Initialize all built-in BT node types
     */
    void InitializeBuiltInTypes();
    
    std::map<std::string, BTNodeTypeInfo> m_nodeTypes;
};

} // namespace AI
} // namespace Olympe
