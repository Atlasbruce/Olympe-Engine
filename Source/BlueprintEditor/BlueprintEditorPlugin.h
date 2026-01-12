/*
 * Olympe Blueprint Editor - Plugin Interface
 * 
 * Extensible plugin system for different blueprint types
 * Each plugin handles a specific blueprint type (BehaviorTree, HFSM, EntityPrefab, etc.)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../../Source/third_party/nlohmann/json.hpp"
#include "BlueprintValidator.h"

namespace Olympe
{
    // Editor context for plugins
    struct EditorContext_st
    {
        bool isDirty = false;
        float deltaTime = 0.0f;

    };/**/

    /**
     * BlueprintEditorPlugin - Base interface for blueprint type plugins
     * Each plugin handles a specific blueprint type with custom rendering and validation
     */
    class BlueprintEditorPlugin
    {
    public:
        virtual ~BlueprintEditorPlugin() = default;
        
        // ===== Identification =====
        virtual std::string GetBlueprintType() const = 0;
        virtual std::string GetDisplayName() const = 0;
        virtual std::string GetDescription() const = 0;
        virtual std::string GetIconPath() const = 0;
        virtual std::string GetDefaultFolder() const = 0;
        
        // ===== Lifecycle =====
        virtual nlohmann::json CreateNew(const std::string& name) = 0;
        virtual bool CanHandle(const nlohmann::json& blueprint) const = 0;
        virtual std::vector<ValidationError> Validate(const nlohmann::json& blueprint) = 0;
        
        // ===== Rendering =====
        virtual void RenderEditor(nlohmann::json& blueprintData, EditorContext_st& ctx) = 0;
        virtual void RenderProperties(const nlohmann::json& blueprintData) = 0;
        virtual void RenderToolbar(nlohmann::json& blueprintData) = 0;
    };
}
